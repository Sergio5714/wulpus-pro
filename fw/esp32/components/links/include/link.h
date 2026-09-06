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

typedef enum { LINK_NONE = 0, LINK_TCP, LINK_USB } link_kind_t;
typedef struct link link_t;

typedef int (*link_read_fn)(void *, void *, size_t, TickType_t);
typedef int (*link_write_fn)(void *, const void *, size_t, TickType_t);
typedef esp_err_t (*link_close_fn)(void *);
typedef bool (*link_connected_fn)(void *);

struct link {
    link_kind_t kind;
    void *context;
    link_read_fn read;
    link_write_fn write;
    link_close_fn close;
    link_connected_fn connected;
    TickType_t timeout;
    uint8_t prefetch[9];
    size_t prefetch_length;
    size_t prefetch_offset;
};

esp_err_t link_init(link_t *link);
esp_err_t link_read_exact(link_t *link, void *buffer, size_t length);
esp_err_t link_write_all(link_t *link, const void *buffer, size_t length);
bool link_is_connected(link_t *link);
esp_err_t link_close(link_t *link);
void link_reset_prefetch(link_t *link);
