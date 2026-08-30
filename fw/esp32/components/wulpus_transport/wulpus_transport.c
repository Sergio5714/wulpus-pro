/*
Copyright (C) 2026 Sergei Vostrikov
SPDX-License-Identifier: Apache-2.0
*/

#include "wulpus_transport.h"

#include <string.h>

#define DEFAULT_IO_TIMEOUT pdMS_TO_TICKS(5000)

esp_err_t wulpus_transport_init(wulpus_transport_t *transport)
{
    if (transport == NULL || transport->read == NULL || transport->write == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    transport->tx_mutex = xSemaphoreCreateMutex();
    if (transport->tx_mutex == NULL)
    {
        return ESP_ERR_NO_MEM;
    }
    transport->io_timeout = DEFAULT_IO_TIMEOUT;
    transport->prefetch_length = 0;
    transport->prefetch_offset = 0;
    return ESP_OK;
}

esp_err_t wulpus_transport_read_exact(wulpus_transport_t *transport, void *buffer,
                                      size_t length)
{
    if (transport == NULL || buffer == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t *destination = buffer;
    size_t received = 0;

    // A listener may consume the first header before acquiring ownership.
    while (received < length && transport->prefetch_offset < transport->prefetch_length)
    {
        destination[received++] = transport->prefetch[transport->prefetch_offset++];
    }
    if (transport->prefetch_offset == transport->prefetch_length)
    {
        transport->prefetch_offset = 0;
        transport->prefetch_length = 0;
    }

    while (received < length)
    {
        int result = transport->read(transport->context, destination + received,
                                     length - received, transport->io_timeout);
        if (result == 0 && wulpus_transport_is_connected(transport))
        {
            // A read timeout is not a session timeout. Acquisition can run for
            // longer than one I/O interval while no host command is pending.
            continue;
        }
        if (result <= 0)
        {
            return ESP_FAIL;
        }
        received += (size_t)result;
    }
    return ESP_OK;
}

esp_err_t wulpus_transport_tx_begin(wulpus_transport_t *transport)
{
    if (transport == NULL || transport->tx_mutex == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    return xSemaphoreTake(transport->tx_mutex, transport->io_timeout) == pdTRUE
               ? ESP_OK
               : ESP_ERR_TIMEOUT;
}

void wulpus_transport_tx_end(wulpus_transport_t *transport)
{
    if (transport != NULL && transport->tx_mutex != NULL)
    {
        xSemaphoreGive(transport->tx_mutex);
    }
}

esp_err_t wulpus_transport_write_all_locked(wulpus_transport_t *transport,
                                            const void *buffer, size_t length)
{
    if (transport == NULL || buffer == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    const uint8_t *source = buffer;
    size_t written = 0;
    while (written < length)
    {
        int result = transport->write(transport->context, source + written,
                                      length - written, transport->io_timeout);
        if (result <= 0)
        {
            return result == 0 ? ESP_ERR_TIMEOUT : ESP_FAIL;
        }
        written += (size_t)result;
    }
    return ESP_OK;
}

esp_err_t wulpus_transport_close(wulpus_transport_t *transport)
{
    return transport != NULL && transport->close != NULL
               ? transport->close(transport->context)
               : ESP_OK;
}

bool wulpus_transport_is_connected(wulpus_transport_t *transport)
{
    return transport != NULL && transport->is_connected != NULL &&
           transport->is_connected(transport->context);
}
