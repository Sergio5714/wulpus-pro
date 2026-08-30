/*
Copyright (C) 2025 ETH Zurich. All rights reserved.

Author: Cedric Hirschi, ETH Zurich
        Sergei Vostrikov, GitHub: @Sergio5714

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

/* Wi-Fi Provisioning Manager Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <string.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <esp_event.h>
#include <esp_pm.h>
#include <esp_system.h>

#include <driver/gpio.h>
#include <driver/spi_master.h>

#include "bsp.h"
#include "provisioner.h"
#include "mdns_manager.h"
#include "double_reset.h"
#include "commander.h"
#include "sock.h"
#include "wulpus_tcp_transport.h"
#include "wulpus_usb_transport.h"

#include "helpers.h"

#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#include <esp_log.h>

#define TCP_PORT_MUTEX_TIMEOUT pdMS_TO_TICKS(1000)
#define SPI_MUTEX_TIMEOUT pdMS_TO_TICKS(1000)
#define DATA_READY_TIMEOUT pdMS_TO_TICKS(1000)
#define MSP_SHUTDOWN_TIMEOUT pdMS_TO_TICKS(2000)
#define MSP_BOOT_DELAY_MS 100
#define MSP_RESTART_COMMAND 0xFB

static const char *TAG = "main";

spi_device_handle_t spi = NULL;

TaskHandle_t tcp_server_task_handle = NULL;
TaskHandle_t usb_server_task_handle = NULL;
TaskHandle_t data_handler_task_handle = NULL;

static QueueHandle_t gpio_evt_queue = NULL;

SemaphoreHandle_t data_ready_semaphore = NULL;
SemaphoreHandle_t shutdown_data_ready_semaphore = NULL;
SemaphoreHandle_t tcp_port_mutex = NULL;
SemaphoreHandle_t spi_mutex = NULL;
static SemaphoreHandle_t session_mutex = NULL;
#if CONFIG_WP_ENABLE_PM
static esp_pm_lock_handle_t usb_session_pm_lock = NULL;
#endif

// Exactly one byte stream owns the MSP430 and receives RF frames. Listener
// tasks may block independently, but this pointer changes only under the
// session mutex.
static wulpus_transport_t *active_transport = NULL;

bool transmits_enabled = false;
static volatile bool msp_shutdown_in_progress = false;

uint8_t spi_rx_buffer[CONFIG_WP_DATA_RX_LENGTH + HEADER_LEN];

static void tcp_server_task(void *pvParameters);
static void usb_server_task(void *pvParameters);
static void data_handler_task(void *pvParameters);
static esp_err_t msp_graceful_shutdown(void);
static void wulpus_session_run(wulpus_transport_t *transport);

static esp_err_t msp_reset_set(bool reset_active)
{
    return gpio_set_level(CONFIG_WP_GPIO_MSP_RST_N, reset_active ? 0 : 1);
}

static esp_err_t msp_graceful_shutdown(void)
{
    uint8_t restart_buffer[CONFIG_WP_DATA_RX_LENGTH] = {MSP_RESTART_COMMAND};
    spi_transaction_t restart = {
        .length = sizeof(restart_buffer) * 8,
        .tx_buffer = restart_buffer,
        .rx_buffer = NULL,
    };

    transmits_enabled = false;
    msp_shutdown_in_progress = true;
    xSemaphoreTake(shutdown_data_ready_semaphore, 0);

    // A high DATA_READY means the MSP430 is already waiting for an SPI
    // transaction. Otherwise wait for the current acquisition to complete.
    if (gpio_get_level(CONFIG_WP_GPIO_DATA_READY) == 0 &&
        xSemaphoreTake(shutdown_data_ready_semaphore, MSP_SHUTDOWN_TIMEOUT) != pdTRUE)
    {
        ESP_LOGE(TAG, "Timed out waiting to send MSP430 shutdown command");
        return ESP_ERR_TIMEOUT;
    }

    if (xSemaphoreTake(spi_mutex, SPI_MUTEX_TIMEOUT) != pdTRUE)
    {
        ESP_LOGE(TAG, "Failed to take SPI mutex for MSP430 shutdown");
        return ESP_ERR_TIMEOUT;
    }
    esp_err_t ret = spi_device_transmit(spi, &restart);
    xSemaphoreGive(spi_mutex);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to send MSP430 shutdown command: %s", esp_err_to_name(ret));
        return ret;
    }

    // Discard the edge associated with the transaction above. The next rising
    // edge is the MSP430 requesting a new configuration after disableAll().
    TickType_t wait_started = xTaskGetTickCount();
    while (gpio_get_level(CONFIG_WP_GPIO_DATA_READY) != 0)
    {
        if ((xTaskGetTickCount() - wait_started) >= MSP_SHUTDOWN_TIMEOUT)
        {
            ESP_LOGE(TAG, "Timed out waiting for MSP430 DATA_READY to clear");
            return ESP_ERR_TIMEOUT;
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    xSemaphoreTake(shutdown_data_ready_semaphore, 0);

    if (xSemaphoreTake(shutdown_data_ready_semaphore, MSP_SHUTDOWN_TIMEOUT) != pdTRUE)
    {
        ESP_LOGE(TAG, "Timed out waiting for MSP430 safe-state acknowledgement");
        return ESP_ERR_TIMEOUT;
    }

    ESP_LOGI(TAG, "MSP430 reached safe state");
    return ESP_OK;
}

static void IRAM_ATTR data_ready_handler(void *arg)
{
    uint32_t gpio_num = (uint32_t)arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

void app_main(void)
{
    ESP_LOGI(TAG, "Entering app_main");
    ESP_ERROR_CHECK(bsp_init());

    // Keep the MSP430 in reset throughout ESP32 startup and until either TCP
    // or USB CDC owns a session. Reset replaces the former dedicated host-ready
    // signal on the integrated XIAO host PCB.
    gpio_config_t gpio_cfg = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT_OD,
        .pin_bit_mask = (1ULL << CONFIG_WP_GPIO_MSP_RST_N),
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&gpio_cfg));
    ESP_ERROR_CHECK(msp_reset_set(true));
    ESP_ERROR_CHECK(gpio_sleep_sel_dis(CONFIG_WP_GPIO_MSP_RST_N));
    ESP_LOGI(TAG, "Hold MSP430 in reset on GPIO %d", CONFIG_WP_GPIO_MSP_RST_N);

#if CONFIG_WP_DOUBLE_RESET
    // Check double reset
    bool reset_provisioning = false;
    ESP_ERROR_CHECK(double_reset_start(&reset_provisioning, CONFIG_WP_DOUBLE_RESET_TIMEOUT));
    if (reset_provisioning)
    {
        ESP_LOGI(TAG, "Double reset detected! Provisioning will be reset.");
    }
#endif

#if CONFIG_WP_ENABLE_PM
    // Configure power management (DFS and auto light sleep)
    esp_pm_config_t pm_config = {
        .max_freq_mhz = CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ,
        .min_freq_mhz = 10,
        .light_sleep_enable = true,
    };
    ESP_ERROR_CHECK(esp_pm_configure(&pm_config));
    // Native USB must remain responsive for the complete binary session. The
    // lock is acquired only by a USB owner, so idle Wi-Fi operation retains
    // automatic light-sleep behavior.
    ESP_ERROR_CHECK(esp_pm_lock_create(ESP_PM_NO_LIGHT_SLEEP, 0,
                                       "wulpus_usb", &usb_session_pm_lock));
    // Keep USB enumeration alive from the moment automatic light sleep is
    // enabled. Ownership of this initial lock is transferred to the USB task,
    // which releases it only after confirming that no USB host is connected.
    ESP_ERROR_CHECK(esp_pm_lock_acquire(usb_session_pm_lock));
#endif

    esp_log_level_set(TAG, LOG_LOCAL_LEVEL);

    // Initialize provisioner and thus wifi
    ESP_ERROR_CHECK(provisioner_init());

    // Initialize mDNS
    ESP_ERROR_CHECK(mdns_manager_init("wulpus"));
    ESP_ERROR_CHECK(mdns_manager_add("wulpus", MDNS_PROTO_TCP, CONFIG_WP_SOCKET_PORT));

    // Configure the MSP430 data-ready input.
    gpio_cfg.intr_type = GPIO_INTR_POSEDGE;
    gpio_cfg.mode = GPIO_MODE_INPUT;
    gpio_cfg.pin_bit_mask = (1ULL << CONFIG_WP_GPIO_DATA_READY);
    ESP_ERROR_CHECK(gpio_config(&gpio_cfg));
    ESP_ERROR_CHECK(gpio_sleep_set_direction(CONFIG_WP_GPIO_DATA_READY, GPIO_MODE_INPUT));
    ESP_ERROR_CHECK(gpio_sleep_set_pull_mode(CONFIG_WP_GPIO_DATA_READY, GPIO_FLOATING));

    // Create semaphore
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    if (gpio_evt_queue == NULL)
    {
        ESP_LOGE(TAG, "Failed to create semaphore");
        return;
    }

    data_ready_semaphore = xSemaphoreCreateBinary();
    if (data_ready_semaphore == NULL)
    {
        ESP_LOGE(TAG, "Failed to create semaphore");
        return;
    }

    shutdown_data_ready_semaphore = xSemaphoreCreateBinary();
    if (shutdown_data_ready_semaphore == NULL)
    {
        ESP_LOGE(TAG, "Failed to create shutdown semaphore");
        return;
    }

    tcp_port_mutex = xSemaphoreCreateMutex();
    if (tcp_port_mutex == NULL)
    {
        ESP_LOGE(TAG, "Failed to create mutex");
        return;
    }
    spi_mutex = xSemaphoreCreateMutex();
    if (spi_mutex == NULL)
    {
        ESP_LOGE(TAG, "Failed to create mutex");
        return;
    }
    session_mutex = xSemaphoreCreateMutex();
    if (session_mutex == NULL)
    {
        ESP_LOGE(TAG, "Failed to create transport-session mutex");
        return;
    }

    // Create data handler
    xTaskCreate(data_handler_task, "data_handler", CONFIG_WP_HANDLER_STACK_SIZE, NULL, CONFIG_WP_HANDLER_PRIORITY, &data_handler_task_handle);
    if (data_handler_task_handle == NULL)
    {
        ESP_LOGE(TAG, "Failed to create data handler task");
        return;
    }

    // Initialize interrupt
    ESP_ERROR_CHECK(gpio_install_isr_service(0));
    ESP_ERROR_CHECK(gpio_isr_handler_add(CONFIG_WP_GPIO_DATA_READY, data_ready_handler, (void *)CONFIG_WP_GPIO_DATA_READY));

    // Initialize SPI
    spi_bus_config_t spi_cfg = {
        .miso_io_num = CONFIG_WP_SPI_MISO,
        .mosi_io_num = CONFIG_WP_SPI_MOSI,
        .sclk_io_num = CONFIG_WP_SPI_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = CONFIG_WP_SPI_MAX_TRANSFER_SIZE,
    };
    spi_device_interface_config_t dev_cfg = {
        .clock_speed_hz = CONFIG_WP_SPI_CLOCK_SPEED,
        .mode = 1,
        .spics_io_num = CONFIG_WP_SPI_CS,
        .queue_size = 3,
        .cs_ena_pretrans = 16,
        .cs_ena_posttrans = 16,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(CONFIG_WP_SPI_INSTANCE - 1, &spi_cfg, SPI_DMA_CH_AUTO));
    ESP_ERROR_CHECK(spi_bus_add_device(CONFIG_WP_SPI_INSTANCE - 1, &dev_cfg, &spi));

    ESP_ERROR_CHECK(gpio_sleep_set_direction(CONFIG_WP_SPI_CLK, GPIO_MODE_OUTPUT));
    ESP_ERROR_CHECK(gpio_sleep_set_pull_mode(CONFIG_WP_SPI_CLK, GPIO_PULLDOWN_ONLY));

    ESP_ERROR_CHECK(gpio_sleep_set_direction(CONFIG_WP_SPI_MOSI, GPIO_MODE_OUTPUT));
    ESP_ERROR_CHECK(gpio_sleep_set_pull_mode(CONFIG_WP_SPI_MOSI, GPIO_PULLDOWN_ONLY));

    ESP_ERROR_CHECK(gpio_sleep_set_direction(CONFIG_WP_SPI_MISO, GPIO_MODE_INPUT));
    ESP_ERROR_CHECK(gpio_sleep_set_pull_mode(CONFIG_WP_SPI_MISO, GPIO_FLOATING));

    ESP_ERROR_CHECK(gpio_sleep_set_direction(CONFIG_WP_SPI_CS, GPIO_MODE_OUTPUT));
    ESP_ERROR_CHECK(gpio_sleep_set_pull_mode(CONFIG_WP_SPI_CS, GPIO_PULLUP_ONLY));

    // Start provisioning
#if CONFIG_WP_DOUBLE_RESET
    ESP_ERROR_CHECK(provisioner_start(reset_provisioning));
#else
    ESP_ERROR_CHECK(provisioner_start(false));
#endif

    // USB must remain usable while Wi-Fi is unprovisioned or reconnecting.
    // provisioner_wait() can block indefinitely, so create the USB listener
    // before waiting for the network connection.
    xTaskCreate(usb_server_task, "usb_server", CONFIG_WP_SERVER_STACK_SIZE, NULL,
                CONFIG_WP_SERVER_PRIORITY, &usb_server_task_handle);
    if (usb_server_task_handle == NULL)
    {
        ESP_LOGE(TAG, "Failed to create USB server task");
        return;
    }

    ESP_ERROR_CHECK(provisioner_wait());

    // Start but suspend TWT
    provisioner_twt_setup();

    // Start TCP server
    xTaskCreate(tcp_server_task, "tcp_server", CONFIG_WP_SERVER_STACK_SIZE, NULL, CONFIG_WP_SERVER_PRIORITY, &tcp_server_task_handle);
    if (tcp_server_task_handle == NULL)
    {
        ESP_LOGE(TAG, "Failed to create server task");
        return;
    }

    ESP_LOGI(TAG, "Returning from app_main()");
}

static bool session_try_acquire(wulpus_transport_t *transport)
{
    bool acquired = false;
    xSemaphoreTake(session_mutex, portMAX_DELAY);
    if (active_transport == NULL)
    {
        active_transport = transport;
        acquired = true;
    }
    xSemaphoreGive(session_mutex);
    return acquired;
}

static void session_release(wulpus_transport_t *transport)
{
    xSemaphoreTake(session_mutex, portMAX_DELAY);
    if (active_transport == transport)
    {
        active_transport = NULL;
    }
    xSemaphoreGive(session_mutex);
}

/*
 * Wait for the first valid protocol header without claiming the session.
 *
 * This makes arbitration demand-driven: an attached USB cable used only for
 * power/JTAG, or an idle TCP connection, cannot block the other transport.
 * The header is copied into transport->prefetch so command_recv() processes it
 * normally after this listener wins ownership.
 */
static esp_err_t transport_wait_for_header(wulpus_transport_t *transport)
{
    static const uint8_t magic[] = "wulpus";
    size_t matched = 0;

    while (wulpus_transport_is_connected(transport))
    {
        uint8_t byte = 0;
        int received = transport->read(transport->context, &byte, 1, pdMS_TO_TICKS(100));
        if (received <= 0)
        {
            continue;
        }

        if (byte == magic[matched])
        {
            transport->prefetch[matched++] = byte;
        }
        else
        {
            matched = byte == magic[0] ? 1 : 0;
            if (matched == 1)
            {
                transport->prefetch[0] = byte;
            }
        }

        if (matched == sizeof(magic) - 1)
        {
            size_t header_bytes = sizeof(magic) - 1;
            while (header_bytes < HEADER_LEN && wulpus_transport_is_connected(transport))
            {
                int result = transport->read(
                    transport->context, transport->prefetch + header_bytes,
                    HEADER_LEN - header_bytes, pdMS_TO_TICKS(100));
                if (result > 0)
                {
                    header_bytes += (size_t)result;
                }
            }
            if (header_bytes != HEADER_LEN)
            {
                return ESP_FAIL;
            }

            wulpus_command_header_t header;
            memcpy(&header, transport->prefetch, HEADER_LEN);
            if (header.command >= MIN_COMMAND_ID && header.command <= MAX_COMMAND_ID)
            {
                transport->prefetch_length = HEADER_LEN;
                transport->prefetch_offset = 0;
                return ESP_OK;
            }
            matched = 0;
            transport->prefetch_length = 0;
        }
    }
    return ESP_FAIL;
}

static void transport_send_busy(wulpus_transport_t *transport)
{
    const wulpus_command_header_t busy = {
        .magic = {'w', 'u', 'l', 'p', 'u', 's'},
        .command = BUSY,
        .data_length = 0,
    };
    if (command_send(transport, &busy, NULL, 0) != ESP_OK)
    {
        ESP_LOGW(TAG, "Could not report busy transport");
    }
}

/*
 * Reject the command whose header was consumed during arbitration.
 *
 * USB is a persistent byte stream rather than an accepted connection. If a
 * losing SET_CONFIG payload were left unread, its bytes could later be mistaken
 * for the start of another header. Drain exactly the advertised payload before
 * returning the USB listener to its magic-word scanner.
 */
static void transport_reject_prefetched_command(wulpus_transport_t *transport)
{
    wulpus_command_header_t rejected;
    memcpy(&rejected, transport->prefetch, HEADER_LEN);
    transport_send_busy(transport);
    transport->prefetch_length = 0;
    transport->prefetch_offset = 0;

    uint8_t discard[64];
    size_t remaining = rejected.data_length;
    while (remaining > 0 && wulpus_transport_is_connected(transport))
    {
        size_t requested = remaining < sizeof(discard) ? remaining : sizeof(discard);
        int received = transport->read(transport->context, discard, requested,
                                       transport->io_timeout);
        if (received <= 0)
        {
            ESP_LOGW(TAG, "Timed out while discarding rejected command payload");
            break;
        }
        remaining -= (size_t)received;
    }
}

static void wulpus_session_run(wulpus_transport_t *transport)
{
    uint8_t rx_buffer[CONFIG_WP_SERVER_RX_BUFFER_SIZE];

    provisioner_twt_suspend(1);
    msp_shutdown_in_progress = false;
    ESP_ERROR_CHECK(msp_reset_set(false));
    ESP_LOGI(TAG, "Boot MSP430 for %s session",
             transport->kind == WULPUS_TRANSPORT_TCP ? "TCP" : "USB CDC");
    vTaskDelay(pdMS_TO_TICKS(MSP_BOOT_DELAY_MS));
    xSemaphoreTake(data_ready_semaphore, 0);

    bool run = true;
    while (run && wulpus_transport_is_connected(transport))
    {
        wulpus_command_header_t recv_header;
        size_t data_len = sizeof(rx_buffer);
        esp_err_t err = command_recv(transport, &recv_header, rx_buffer, &data_len);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Transport command receive failed");
            break;
        }

        switch (recv_header.command)
        {
        case SET_CONFIG:
        {
            ESP_LOGI(TAG, "Received set config command");
            if (xSemaphoreTake(data_ready_semaphore, DATA_READY_TIMEOUT) != pdTRUE)
            {
                ESP_LOGE(TAG, "Failed to take data ready semaphore");
                break;
            }

            uint8_t spi_tx_buffer[CONFIG_WP_DATA_RX_LENGTH] = {0};
            memcpy(spi_tx_buffer, rx_buffer, data_len);
            spi_transaction_t tx = {
                .length = sizeof(spi_tx_buffer) * 8,
                .tx_buffer = spi_tx_buffer,
                .rx_buffer = NULL,
            };
            if (xSemaphoreTake(spi_mutex, SPI_MUTEX_TIMEOUT) != pdTRUE)
            {
                ESP_LOGE(TAG, "Failed to take SPI mutex");
                break;
            }
            esp_err_t result = spi_device_transmit(spi, &tx);
            xSemaphoreGive(spi_mutex);
            if (result != ESP_OK)
            {
                ESP_LOGE(TAG, "SPI configuration transfer failed: %s",
                         esp_err_to_name(result));
            }
            break;
        }
        case GET_DATA:
            ESP_LOGW(TAG, "GET_DATA is not implemented");
            break;
        case PING:
        {
            const wulpus_command_header_t response = {
                .magic = {'w', 'u', 'l', 'p', 'u', 's'},
                .command = PONG,
                .data_length = 4,
            };
            if (command_send(transport, &response, "pong", 4) != ESP_OK)
            {
                run = false;
            }
            break;
        }
        case RESET:
            if (msp_graceful_shutdown() != ESP_OK)
            {
                ESP_LOGW(TAG, "Graceful MSP430 shutdown failed; using reset fallback");
            }
            ESP_ERROR_CHECK(msp_reset_set(true));
            esp_restart();
            break;
        case CLOSE:
            run = false;
            break;
        case START_RX:
            transmits_enabled = true;
            if (xSemaphoreTake(data_ready_semaphore, 0) == pdTRUE)
            {
                uint32_t io_num = CONFIG_WP_GPIO_DATA_READY;
                xQueueSend(gpio_evt_queue, &io_num, 0);
            }
            break;
        case STOP_RX:
            transmits_enabled = false;
            break;
        case PONG:
        case BUSY:
            ESP_LOGW(TAG, "Ignoring host-only command %s",
                     command_name(recv_header.command));
            break;
        }
        ESP_LOGI(TAG, "Command %s processed", command_name(recv_header.command));
    }

    transmits_enabled = false;
    if (msp_graceful_shutdown() != ESP_OK)
    {
        ESP_LOGW(TAG, "Graceful MSP430 shutdown failed; using reset fallback");
    }
    ESP_ERROR_CHECK(msp_reset_set(true));
    wulpus_transport_close(transport);
    transport->prefetch_length = 0;
    transport->prefetch_offset = 0;
    session_release(transport);
    provisioner_twt_suspend(0);
    ESP_LOGI(TAG, "Released WULPUS transport session");
}

static void tcp_server_task(void *pvParameters)
{
    (void)pvParameters;
    ESP_LOGI(TAG, "TCP listener task started");

    socket_instance_t listen_socket = sock_create();
    socket_instance_t client_socket = sock_create();
    if (listen_socket.mutex == NULL || client_socket.mutex == NULL)
    {
        ESP_LOGE(TAG, "Failed to create TCP sockets");
        vTaskDelete(NULL);
        return;
    }

    ESP_ERROR_CHECK(sock_init(&listen_socket));
    ESP_ERROR_CHECK(sock_listen(&listen_socket, INADDR_ANY, CONFIG_WP_SOCKET_PORT));
    ESP_ERROR_CHECK(sock_init(&client_socket));

    wulpus_transport_t transport;
    ESP_ERROR_CHECK(wulpus_tcp_transport_create(&transport, &client_socket));

    while (true)
    {
        if (sock_accept(&listen_socket, &client_socket) != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to accept TCP connection");
            continue;
        }
        if (transport_wait_for_header(&transport) != ESP_OK)
        {
            wulpus_transport_close(&transport);
            continue;
        }
        if (!session_try_acquire(&transport))
        {
            ESP_LOGW(TAG, "Rejecting TCP client: another transport is active");
            transport_send_busy(&transport);
            wulpus_transport_close(&transport);
            continue;
        }
        wulpus_session_run(&transport);
    }
}

static void usb_server_task(void *pvParameters)
{
    (void)pvParameters;
    ESP_LOGI(TAG, "USB CDC listener task started");

    static wulpus_transport_t transport;
    ESP_ERROR_CHECK(wulpus_usb_transport_install(&transport));
#if CONFIG_WP_ENABLE_PM
    // app_main() acquires the initial lock before USB task creation so the
    // device cannot enter light sleep during USB enumeration.
    bool usb_pm_lock_held = true;
#endif

    while (true)
    {
#if CONFIG_WP_ENABLE_PM
        if (!usb_pm_lock_held)
        {
            ESP_ERROR_CHECK(esp_pm_lock_acquire(usb_session_pm_lock));
            usb_pm_lock_held = true;
        }

        // Wake briefly before sampling the USB connection monitor. If the
        // status is checked while automatic light sleep is already active,
        // USB SOF packets cannot update it and the listener never reaches the
        // lock acquisition needed to receive the first command.
        vTaskDelay(pdMS_TO_TICKS(10));
        if (!wulpus_transport_is_connected(&transport))
        {
            ESP_ERROR_CHECK(esp_pm_lock_release(usb_session_pm_lock));
            usb_pm_lock_held = false;
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        // ESP32-C6 cannot keep USB Serial/JTAG responsive during automatic
        // light sleep. Acquire the lock before waiting for the first protocol
        // header; acquiring it only after the header creates a deadlock where
        // the host cannot deliver the bytes needed to start a USB session.
#endif
        if (transport_wait_for_header(&transport) != ESP_OK)
        {
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }
        if (!session_try_acquire(&transport))
        {
            ESP_LOGW(TAG, "Rejecting USB command: another transport is active");
            transport_reject_prefetched_command(&transport);
            continue;
        }
        // Keep the lock after session cleanup while the USB host remains
        // physically connected. The next loop releases it on disconnection.
        wulpus_session_run(&transport);
    }
}

static void data_handler_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Data handler task started");

    uint32_t io_num;

    spi_transaction_t rx = {
        .length = CONFIG_WP_DATA_RX_LENGTH * 8,
        .tx_buffer = NULL,
        .rx_buffer = spi_rx_buffer + HEADER_LEN,
    };
    wulpus_command_header_t response = {
        .magic = {'w', 'u', 'l', 'p', 'u', 's'},
        .command = GET_DATA,
        .data_length = CONFIG_WP_DATA_RX_LENGTH,
    };
    memcpy(spi_rx_buffer, &response, HEADER_LEN);

    while (1)
    {
        // Wait for data ready signal
        if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY) == pdTRUE)
        {
            // Data is ready, handle it here
            ESP_LOGD(TAG, "Data ready signal received on GPIO %lu", io_num);

            // Give data ready semaphore
            xSemaphoreGive(data_ready_semaphore);

            if (msp_shutdown_in_progress)
            {
                xSemaphoreGive(shutdown_data_ready_semaphore);
                continue;
            }

            wulpus_transport_t *transport = NULL;
            xSemaphoreTake(session_mutex, portMAX_DELAY);
            transport = active_transport;
            xSemaphoreGive(session_mutex);

            // A command acknowledgement and an RF frame may be emitted from
            // different tasks. command_send() holds the transport TX mutex for
            // the complete header+payload packet, preventing interleaving.
            if (transmits_enabled && transport != NULL &&
                wulpus_transport_is_connected(transport))
            {
                // Read data from the device
                if (xSemaphoreTake(spi_mutex, SPI_MUTEX_TIMEOUT) != pdTRUE)
                {
                    ESP_LOGE(TAG, "Failed to take SPI mutex");
                    continue;
                }
                esp_err_t ret = spi_device_transmit(spi, &rx);
                xSemaphoreGive(spi_mutex);
                if (ret != ESP_OK)
                {
                    ESP_LOGE(TAG, "Error occurred during SPI reception: %s", esp_err_to_name(ret));
                    continue;
                }

                ret = command_send(transport, &response, spi_rx_buffer + HEADER_LEN,
                                   CONFIG_WP_DATA_RX_LENGTH);
                if (ret != ESP_OK)
                {
                    ESP_LOGE(TAG, "Failed to send data: %s", esp_err_to_name(ret));
                    transmits_enabled = false;
                    msp_shutdown_in_progress = true;
                    ESP_ERROR_CHECK(msp_reset_set(true));
                    continue;
                }
            }
        }
    }
}
