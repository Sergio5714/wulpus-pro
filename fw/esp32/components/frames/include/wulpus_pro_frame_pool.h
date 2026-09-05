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

#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"
#include "freertos/FreeRTOS.h"

#define WULPUS_PRO_FRAME_SLOT_COUNT CONFIG_WP_FRAME_SLOT_COUNT

typedef struct wulpus_pro_frame_slot {
    uint8_t *payload;
    size_t length;
    uint32_t session_generation;
    int64_t data_ready_time_us;
    int64_t spi_complete_time_us;
    uint8_t private_index;
} wulpus_pro_frame_slot_t;

esp_err_t wulpus_pro_frame_pool_init(size_t payload_size);
wulpus_pro_frame_slot_t *wulpus_pro_frame_pool_acquire_for_spi(TickType_t timeout);
void wulpus_pro_frame_pool_mark_ready(wulpus_pro_frame_slot_t *slot);
wulpus_pro_frame_slot_t *wulpus_pro_frame_pool_acquire_for_tx(TickType_t timeout);
void wulpus_pro_frame_pool_release(wulpus_pro_frame_slot_t *slot);
void wulpus_pro_frame_pool_discard_ready(void);
uint16_t wulpus_pro_frame_pool_usage(void);
uint16_t wulpus_pro_frame_pool_max_usage(void);
void wulpus_pro_frame_pool_reset_max_usage(void);
