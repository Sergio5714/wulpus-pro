/*
Copyright (C) 2026 Sergei Vostrikov
SPDX-License-Identifier: Apache-2.0
*/

#include "wulpus_tcp_transport.h"

static int tcp_read(void *context, void *buffer, size_t length, TickType_t timeout)
{
    (void)timeout; // The socket component currently owns its receive timeout policy.
    socket_instance_t *socket = context;
    size_t received = length;
    return sock_recv(socket, buffer, &received) == ESP_OK ? (int)received : -1;
}

static int tcp_write(void *context, const void *buffer, size_t length, TickType_t timeout)
{
    (void)timeout;
    return sock_send((socket_instance_t *)context, buffer, length) == ESP_OK
               ? (int)length
               : -1;
}

static esp_err_t tcp_close(void *context)
{
    return sock_close((socket_instance_t *)context);
}

static bool tcp_connected(void *context)
{
    return ((socket_instance_t *)context)->fd >= 0;
}

esp_err_t wulpus_tcp_transport_create(wulpus_transport_t *transport,
                                      socket_instance_t *socket)
{
    if (transport == NULL || socket == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    *transport = (wulpus_transport_t){
        .kind = WULPUS_TRANSPORT_TCP,
        .context = socket,
        .read = tcp_read,
        .write = tcp_write,
        .close = tcp_close,
        .is_connected = tcp_connected,
    };
    return wulpus_transport_init(transport);
}
