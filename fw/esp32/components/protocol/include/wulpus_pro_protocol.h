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
#include "link.h"
#include "wulpus_pro_commands.h"

#define WULPUS_PRO_HEADER_SIZE 9
#define WULPUS_PRO_MAGIC "wulpus"

typedef struct __attribute__((packed)) {
    char magic[6];
    uint8_t command;
    uint16_t data_length;
} wulpus_pro_header_t;

typedef struct __attribute__((packed)) {
    uint32_t error_mask;
    uint8_t clear_counters;
} wulpus_pro_clear_status_t;

void wulpus_pro_protocol_make_header(wulpus_pro_header_t *header, wulpus_pro_command_t command, uint16_t length);
bool wulpus_pro_protocol_header_valid(const wulpus_pro_header_t *header);
esp_err_t wulpus_pro_protocol_wait_for_header(link_t *link);
esp_err_t wulpus_pro_protocol_receive(link_t *link, wulpus_pro_header_t *header, void *payload, size_t capacity);
esp_err_t wulpus_pro_protocol_discard_prefetched_payload(link_t *link);
