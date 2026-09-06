/* Copyright (C) 2026 Sergei Vostrikov, SPDX-License-Identifier: Apache-2.0 */
#pragma once
#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"
#include "esp_partition.h"

#define MSP430_IMAGE_MAGIC 0x3150534dU /* MSP1 */
#define MSP430_IMAGE_VERSION 1
#define MSP430_IMAGE_MAX_SECTIONS 64

typedef struct __attribute__((packed)) {
    uint32_t magic;
    uint16_t version;
    uint16_t header_size;
    uint32_t target_id;
    uint32_t total_size;
    uint32_t image_crc32;
    uint16_t section_count;
    uint16_t flags;
} msp430_image_header_t;

typedef struct __attribute__((packed)) {
    uint32_t address;
    uint32_t length;
    uint32_t data_crc32;
} msp430_image_section_t;

typedef struct {
    const esp_partition_t *partition;
    msp430_image_header_t header;
    msp430_image_section_t sections[MSP430_IMAGE_MAX_SECTIONS];
    uint32_t data_offsets[MSP430_IMAGE_MAX_SECTIONS];
} msp430_image_t;

uint32_t msp430_crc32(uint32_t crc, const void *data, size_t length);
esp_err_t msp430_image_open(const esp_partition_t *partition, uint32_t staged_size,
                            msp430_image_t *image);
esp_err_t msp430_image_read(const msp430_image_t *image, unsigned section,
                            uint32_t offset, void *data, size_t length);
