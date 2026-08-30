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

#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "wulpus_pro_session.h"

esp_err_t acquisition_thread_start(void);
void acquisition_thread_set_enabled(bool enabled);
esp_err_t acquisition_thread_wait_for_edge(TickType_t timeout);
void acquisition_thread_clear_edges(void);
esp_err_t acquisition_thread_send_block(const void *data, size_t length);
esp_err_t acquisition_thread_graceful_shutdown(void);

esp_err_t packet_tx_thread_start(void);
esp_err_t packet_tx_submit_control(wulpus_pro_session_ref_t session, uint8_t command,
                                   const void *payload, uint16_t length, TickType_t timeout);
esp_err_t packet_tx_submit_to_link(link_t *link, uint8_t command,
                                   const void *payload, uint16_t length, TickType_t timeout);
void packet_tx_notify_frame_ready(void);
void packet_tx_discard_session(wulpus_pro_session_ref_t session);

esp_err_t protocol_thread_start(void);
esp_err_t protocol_thread_submit_session(wulpus_pro_session_ref_t session);

esp_err_t usb_thread_start(void);
esp_err_t tcp_thread_start(void);
esp_err_t provisioning_thread_start(bool reset);
