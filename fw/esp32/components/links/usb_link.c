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

#include "usb_link.h"
#include "driver/usb_serial_jtag.h"
#include "esp_check.h"

static int read_bytes(void *context, void *buffer, size_t length, TickType_t timeout) { (void)context; return usb_serial_jtag_read_bytes(buffer, length, timeout); }
static int write_bytes(void *context, const void *buffer, size_t length, TickType_t timeout) { (void)context; return usb_serial_jtag_write_bytes(buffer, length, timeout); }
static esp_err_t close_link(void *context) { (void)context; return ESP_OK; }
static bool connected(void *context) { (void)context; return usb_serial_jtag_is_connected(); }

esp_err_t usb_link_create(link_t *link)
{
    usb_serial_jtag_driver_config_t config = {.rx_buffer_size = 2048, .tx_buffer_size = 2048};
    ESP_RETURN_ON_ERROR(usb_serial_jtag_driver_install(&config), "usb_link", "driver install failed");
    *link = (link_t){.kind = LINK_USB, .read = read_bytes, .write = write_bytes, .close = close_link, .connected = connected};
    return link_init(link);
}
