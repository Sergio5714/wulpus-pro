/*
Copyright (C) 2026 Sergei Vostrikov
SPDX-License-Identifier: Apache-2.0
*/

#ifndef WULPUS_TRANSPORT_H
#define WULPUS_TRANSPORT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <esp_err.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

/** Physical link currently owning the single WULPUS command session. */
typedef enum
{
    WULPUS_TRANSPORT_NONE = 0,
    WULPUS_TRANSPORT_TCP,
    WULPUS_TRANSPORT_USB_CDC,
} wulpus_transport_kind_t;

struct wulpus_transport;
typedef struct wulpus_transport wulpus_transport_t;

typedef int (*wulpus_transport_read_fn)(void *context, void *buffer, size_t length,
                                        TickType_t timeout);
typedef int (*wulpus_transport_write_fn)(void *context, const void *buffer, size_t length,
                                         TickType_t timeout);
typedef esp_err_t (*wulpus_transport_close_fn)(void *context);
typedef bool (*wulpus_transport_connected_fn)(void *context);

/**
 * Byte-stream transport used by the WULPUS protocol.
 *
 * TCP and USB CDC have different connection mechanics but expose the same
 * ordered byte stream. The small prefetch buffer preserves the first command
 * read by a listener while it is competing for session ownership.
 */
struct wulpus_transport
{
    wulpus_transport_kind_t kind;
    void *context;
    wulpus_transport_read_fn read;
    wulpus_transport_write_fn write;
    wulpus_transport_close_fn close;
    wulpus_transport_connected_fn is_connected;
    SemaphoreHandle_t tx_mutex;
    TickType_t io_timeout;
    uint8_t prefetch[9];
    size_t prefetch_length;
    size_t prefetch_offset;
};

esp_err_t wulpus_transport_init(wulpus_transport_t *transport);
esp_err_t wulpus_transport_read_exact(wulpus_transport_t *transport, void *buffer,
                                      size_t length);
esp_err_t wulpus_transport_write_all_locked(wulpus_transport_t *transport,
                                            const void *buffer, size_t length);
esp_err_t wulpus_transport_tx_begin(wulpus_transport_t *transport);
void wulpus_transport_tx_end(wulpus_transport_t *transport);
esp_err_t wulpus_transport_close(wulpus_transport_t *transport);
bool wulpus_transport_is_connected(wulpus_transport_t *transport);

#endif
