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

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_err.h"

esp_err_t board_init(void);
esp_err_t board_msp_reset(bool asserted);
bool board_data_ready(void);
esp_err_t board_data_ready_set_isr(gpio_isr_t handler, void *argument);

esp_err_t board_spi_receive_dma(void *buffer, size_t length);
esp_err_t board_spi_transmit(const void *buffer, size_t length);

esp_err_t board_usb_no_sleep_acquire(void);
esp_err_t board_usb_no_sleep_release(void);
