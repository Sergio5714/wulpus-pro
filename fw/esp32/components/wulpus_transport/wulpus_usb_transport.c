/*
Copyright (C) 2026 Sergei Vostrikov
SPDX-License-Identifier: Apache-2.0
*/

#include "wulpus_usb_transport.h"

#include <driver/usb_serial_jtag.h>
#include <esp_check.h>

#define USB_RX_BUFFER_SIZE 2048
#define USB_TX_BUFFER_SIZE 2048

static int usb_read(void *context, void *buffer, size_t length, TickType_t timeout)
{
    (void)context;
    return usb_serial_jtag_read_bytes(buffer, length, timeout);
}

static int usb_write(void *context, const void *buffer, size_t length, TickType_t timeout)
{
    (void)context;
    return usb_serial_jtag_write_bytes(buffer, length, timeout);
}

static esp_err_t usb_close(void *context)
{
    (void)context;
    // The fixed USB Serial/JTAG device remains enumerated for flashing/JTAG.
    // Closing a WULPUS session only releases protocol ownership.
    return ESP_OK;
}

static bool usb_connected(void *context)
{
    (void)context;
    return usb_serial_jtag_is_connected();
}

esp_err_t wulpus_usb_transport_install(wulpus_transport_t *transport)
{
    if (transport == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    usb_serial_jtag_driver_config_t configuration = {
        .rx_buffer_size = USB_RX_BUFFER_SIZE,
        .tx_buffer_size = USB_TX_BUFFER_SIZE,
    };
    ESP_RETURN_ON_ERROR(usb_serial_jtag_driver_install(&configuration),
                        "wulpus_usb", "Could not install USB Serial/JTAG driver");

    *transport = (wulpus_transport_t){
        .kind = WULPUS_TRANSPORT_USB_CDC,
        .read = usb_read,
        .write = usb_write,
        .close = usb_close,
        .is_connected = usb_connected,
    };
    return wulpus_transport_init(transport);
}
