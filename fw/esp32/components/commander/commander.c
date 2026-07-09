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

#include "commander.h"

#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#include <esp_log.h>

#define TAG "commander"

#include <string.h>

#include "sock.h"

esp_err_t command_recv(socket_instance_t *socket, wulpus_command_header_t *header, void *data, size_t *len)
{
    esp_log_level_set(TAG, LOG_LOCAL_LEVEL);

    ESP_LOGD(TAG, "Receiving command...");
    esp_err_t err = ESP_OK;

    size_t recv_len = HEADER_LEN;
    err = sock_recv(socket, header, &recv_len);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to receive header");
        return err;
    }
    if (recv_len != HEADER_LEN)
    {
        ESP_LOGW(TAG, "Header length mismatch: expected %d, got %d", HEADER_LEN, recv_len);
        return ESP_FAIL;
    }

    if (strncmp(header->magic, "wulpus", 6) != 0)
    {
        ESP_LOGW(TAG, "Invalid magic: Expected 'wulpus'");
        return ESP_FAIL;
    }

    // Send back header as response
    wulpus_command_header_t resp_header = *header;
    resp_header.data_length = 0; // Set data length to 0 for the header response

    err = command_send(socket, &resp_header, NULL, 0);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to send response");
        return err;
    }
    ESP_LOGD(TAG, "Header echoed back");

    if (header->command < MIN_COMMAND_ID || header->command > MAX_COMMAND_ID)
    {
        ESP_LOGW(TAG, "Invalid command: %d", header->command);
        return ESP_FAIL;
    }

    if (header->data_length != 0)
    {
        if (header->data_length > *len)
        {
            ESP_LOGE(TAG, "Data length exceeds buffer size: %d > %d", header->data_length, *len);
            return ESP_FAIL;
        }

        // Receive data
        *len = header->data_length;
        err = sock_recv(socket, data, len);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to receive data");
            return err;
        }
        else if (*len != header->data_length)
        {
            ESP_LOGW(TAG, "Data length mismatch: expected %d, got %d", header->data_length, *len);
            return ESP_FAIL;
        }
    }

    ESP_LOGI(TAG, "Received command: %s, Data length: %d", command_name(header->command), header->data_length);
    return err;
}

esp_err_t command_send(socket_instance_t *socket, wulpus_command_header_t *header, const void *data, size_t len)
{
    ESP_LOGD(TAG, "Sending command...");
    esp_err_t err = ESP_OK;

    if (header->data_length != len)
    {
        ESP_LOGE(TAG, "Data length mismatch: expected %d, got %d", header->data_length, len);
        return ESP_FAIL;
    }

    socket->persist = true; // Set socket to persist mode (keep mutex locked)
    err = sock_send(socket, header, HEADER_LEN);
    socket->persist = false; // Reset socket to non-persist mode for future operations
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to send header");
        return err;
    }

    if (len > 0 && data != NULL)
    {
        err = sock_send(socket, data, len);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to send data");
            return err;
        }
    }
    ESP_LOGD(TAG, "Command sent successfully");

    return err;
}

char *command_name(wulpus_command_type_e command)
{
    switch (command)
    {
    case SET_CONFIG:
        return "SET_CONFIG";
    case GET_DATA:
        return "GET_DATA";
    case PING:
        return "PING";
    case PONG:
        return "PONG";
    case RESET:
        return "RESET";
    case CLOSE:
        return "CLOSE";
    case START_RX:
        return "START_RX";
    case STOP_RX:
        return "STOP_RX";
    default:
        return "UNKNOWN_COMMAND";
    }

    return "UNKNOWN_COMMAND";
}