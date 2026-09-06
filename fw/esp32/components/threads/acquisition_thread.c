/*
Copyright (C) 2026 Sergei Vostrikov

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include "thread_internal.h"

#include "board.h"
#include "esp_check.h"
#include "esp_timer.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "wulpus_pro_frame_pool.h"
#include "wulpus_pro_state.h"
#include "wulpus_pro_status.h"

#define BUFFER_WAIT pdMS_TO_TICKS(100)
#define MSP_RESTART_COMMAND 0xFB

static TaskHandle_t task_handle;
static SemaphoreHandle_t edge_semaphore;

static void IRAM_ATTR data_ready_isr(void *argument)
{
    (void)argument;
    BaseType_t wake = pdFALSE;
    vTaskNotifyGiveFromISR(task_handle, &wake);
    if (wake) portYIELD_FROM_ISR();
}

static void acquisition_task(void *argument)
{
    (void)argument;
    while (true) {
        uint32_t edges = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        while (edges-- > 0) {
            wulpus_pro_status_increment_data_ready();
            xSemaphoreGive(edge_semaphore);
            if (!wulpus_pro_state_is_acquiring()) continue;

            wulpus_pro_frame_slot_t *slot = wulpus_pro_frame_pool_acquire_for_spi(BUFFER_WAIT);
            if (slot == NULL) {
                wulpus_pro_status_set_error(WULPUS_PRO_ERROR_ACQ_BUFFER_OVERFLOW);
                wulpus_pro_status_increment_overflow();
                wulpus_pro_state_set_acquiring(false);
                continue;
            }
            wulpus_pro_session_ref_t session = wulpus_pro_session_current();
            slot->session_generation = session.generation;
            slot->data_ready_time_us = esp_timer_get_time();
            esp_err_t result = board_spi_receive_dma(slot->payload, slot->length);
            slot->spi_complete_time_us = esp_timer_get_time();
            if (result != ESP_OK) {
                wulpus_pro_status_set_error(result == ESP_ERR_TIMEOUT ? WULPUS_PRO_ERROR_SPI_TIMEOUT : WULPUS_PRO_ERROR_SPI_FAILURE);
                wulpus_pro_status_increment_spi_error();
                wulpus_pro_frame_pool_release(slot);
                continue;
            }
            wulpus_pro_status_increment_spi_complete();
            wulpus_pro_frame_pool_mark_ready(slot);
            packet_tx_notify_frame_ready();
        }
    }
}

esp_err_t acquisition_thread_start(void)
{
    edge_semaphore = xSemaphoreCreateCounting(8, 0);
    if (edge_semaphore == NULL) return ESP_ERR_NO_MEM;
    if (xTaskCreate(acquisition_task, "acquisition", CONFIG_WP_ACQUISITION_STACK_SIZE,
                    NULL, CONFIG_WP_ACQUISITION_PRIORITY, &task_handle) != pdPASS) return ESP_ERR_NO_MEM;
    return board_data_ready_set_isr(data_ready_isr, NULL);
}

void acquisition_thread_set_enabled(bool enabled)
{
    wulpus_pro_state_set_acquiring(enabled);
    if (enabled && board_data_ready()) xTaskNotifyGive(task_handle);
}

esp_err_t acquisition_thread_wait_for_edge(TickType_t timeout)
{
    return xSemaphoreTake(edge_semaphore, timeout) == pdTRUE ? ESP_OK : ESP_ERR_TIMEOUT;
}

void acquisition_thread_clear_edges(void)
{
    while (xSemaphoreTake(edge_semaphore, 0) == pdTRUE) {}
}

esp_err_t acquisition_thread_send_block(const void *data, size_t length)
{
    return board_spi_transmit(data, length);
}

esp_err_t acquisition_thread_graceful_shutdown(void)
{
    acquisition_thread_set_enabled(false);
    wulpus_pro_frame_pool_discard_ready();
    uint8_t restart[CONFIG_WP_DATA_RX_LENGTH] = {MSP_RESTART_COMMAND};
    xSemaphoreTake(edge_semaphore, 0);
    if (!board_data_ready()) {
        ESP_RETURN_ON_ERROR(acquisition_thread_wait_for_edge(pdMS_TO_TICKS(2000)), "acquisition", "MSP shutdown edge timeout");
    }
    ESP_RETURN_ON_ERROR(board_spi_transmit(restart, sizeof(restart)), "acquisition", "MSP restart transfer failed");
    TickType_t started = xTaskGetTickCount();
    while (board_data_ready()) {
        if (xTaskGetTickCount() - started > pdMS_TO_TICKS(2000)) return ESP_ERR_TIMEOUT;
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    xSemaphoreTake(edge_semaphore, 0);
    return acquisition_thread_wait_for_edge(pdMS_TO_TICKS(2000));
}
