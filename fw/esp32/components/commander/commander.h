/*
Copyright (C) 2025 ETH Zurich. All rights reserved.

Author: Cedric Hirschi, ETH Zurich

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

#ifndef COMMANDER_H
#define COMMANDER_H

#include <stdint.h>

#include "esp_err.h"

#include "wulpus_transport.h"

#define HEADER_LEN sizeof(wulpus_command_header_t)
#define MIN_COMMAND_ID 0x57
#define MAX_COMMAND_ID 0x5F

typedef enum
{
    SET_CONFIG = 0x57,
    GET_DATA = 0x58,
    PING = 0x59,
    PONG = 0x5A,
    RESET = 0x5B,
    CLOSE = 0x5C,
    START_RX = 0x5D,
    STOP_RX = 0x5E,
    BUSY = 0x5F,
} wulpus_command_type_e;

typedef struct __attribute__((packed))
{
    char magic[6];             // Magic string "wulpus"
    uint8_t command : 8;       // Command type
    uint16_t data_length : 16; // Length of data
} wulpus_command_header_t;

typedef struct
{
    uint8_t *data;        // Pointer to data buffer
    uint16_t data_length; // Length of data
} wulpus_command_data_t;

esp_err_t command_recv(wulpus_transport_t *transport, wulpus_command_header_t *header,
                       void *data, size_t *len);
esp_err_t command_send(wulpus_transport_t *transport,
                       const wulpus_command_header_t *header,
                       const void *data, size_t len);

char *command_name(wulpus_command_type_e command);

#endif
