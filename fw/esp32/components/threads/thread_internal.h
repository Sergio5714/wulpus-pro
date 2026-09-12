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

/**
 * @file thread_internal.h
 * @brief Internal task startup and inter-task communication interfaces.
 */

#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "wulpus_pro_session.h"

/**
 * @brief Create edge tracking and the acquisition task, then register the GPIO ISR.
 */
esp_err_t acquisition_thread_start(void);
/**
 * @brief Set acquisition state and notify the task if DATA_READY is already high.
 */
void acquisition_thread_set_enabled(bool enabled);
/**
 * @brief Consume one recorded DATA_READY event, waiting up to the supplied timeout.
 *
 * @param timeout FreeRTOS ticks to wait.
 * @return ESP_OK if an event is consumed; ESP_ERR_TIMEOUT otherwise.
 */
esp_err_t acquisition_thread_wait_for_edge(TickType_t timeout);
/**
 * @brief Drain recorded DATA_READY events without waiting.
 */
void acquisition_thread_clear_edges(void);
/**
 * @brief Transmit a block of bytes to the MSP430 over SPI.
 */
esp_err_t acquisition_thread_send_block(const void* data, size_t length);
/**
 * @brief Stop acquisition, discard ready frames, and perform the MSP restart handshake.
 */
esp_err_t acquisition_thread_graceful_shutdown(void);

/**
 * @brief Create the control-response queue and packet transmission task.
 */
esp_err_t packet_tx_thread_start(void);
/**
 * @brief Queue a control response that requires the supplied session to remain current.
 *
 * @note Payloads are copied into the queue and must not exceed 64 bytes.
 * @param timeout FreeRTOS ticks allowed separately for queueing and completion.
 * @return Transmission result, ESP_ERR_INVALID_ARG, or ESP_ERR_TIMEOUT.
 * @note A completion timeout does not cancel an already queued request.
 */
esp_err_t packet_tx_submit_control(wulpus_pro_session_ref_t session, uint8_t command,
                                   const void* payload, uint16_t length, TickType_t timeout);
/**
 * @brief Queue a control response without requiring ownership of the current session.
 *
 * @note Payloads are copied into the queue and must not exceed 64 bytes.
 * @param timeout FreeRTOS ticks allowed separately for queueing and completion.
 * @return Transmission result, ESP_ERR_INVALID_ARG, or ESP_ERR_TIMEOUT.
 * @note A completion timeout does not cancel an already queued request.
 */
esp_err_t packet_tx_submit_to_link(link_t* link, uint8_t command, const void* payload,
                                   uint16_t length, TickType_t timeout);
/**
 * @brief Wake the packet transmission task if it has been created.
 */
void packet_tx_notify_frame_ready(void);
/**
 * @brief Discard all ready frames; the session argument is currently unused.
 */
void packet_tx_discard_session(wulpus_pro_session_ref_t session);

/**
 * @brief Create the session queue and protocol processing task.
 */
esp_err_t protocol_thread_start(void);
/**
 * @brief Queue a session for protocol processing, waiting at most 100 milliseconds.
 */
esp_err_t protocol_thread_submit_session(wulpus_pro_session_ref_t session);

/**
 * @brief Create the USB connection task.
 */
esp_err_t usb_thread_start(void);
/**
 * @brief Create the TCP listener task.
 */
esp_err_t tcp_thread_start(void);
/**
 * @brief Create the task that runs Wi-Fi provisioning and connection management.
 */
esp_err_t provisioning_thread_start(bool reset);
