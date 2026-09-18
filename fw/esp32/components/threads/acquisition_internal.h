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

#ifdef _MSC_VER
#define __attribute__(x)
#endif

#include <stdbool.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "thread_internal.h"
#include "wulpus_pro_frame_pool.h"

#define ACQUISITION_HANDSHAKE_TIMEOUT pdMS_TO_TICKS(2000)
#define ACQUISITION_BUFFER_WAIT pdMS_TO_TICKS(100)

typedef enum {
    ACQ_STATE_RESET,
    ACQ_STATE_WAIT_CONFIG,
    ACQ_STATE_CONFIGURING,
    ACQ_STATE_CONFIGURED,
    ACQ_STATE_ACQUIRING,
    ACQ_STATE_QUIESCENT,
    ACQ_STATE_RESTARTING,
} acq_state_t;

typedef struct {
    acq_command_type_t type;
    wulpus_pro_session_ref_t session;
    uint8_t config[CONFIG_WP_DATA_RX_LENGTH];
    SemaphoreHandle_t done;
    esp_err_t result;
    unsigned references;
    bool cancelled;
} acq_request_t;

extern TaskHandle_t acquisition_task_handle;
extern QueueHandle_t acquisition_command_queue;
extern portMUX_TYPE acquisition_lock;
extern acq_state_t acquisition_state;
extern bool acquisition_configured;
extern bool acquisition_reset_asserted;
extern wulpus_pro_session_ref_t acquisition_owner;
extern wulpus_pro_frame_slot_t* acquisition_pending_frame;
extern uint32_t acquisition_rising_edges;
extern uint32_t acquisition_consumed_rise;
extern bool acquisition_assertion_consumed;

bool acquisition_request_cancelled(acq_request_t* request);
void acquisition_report_error(esp_err_t result);
void acquisition_data_ready_isr(void* argument);
uint32_t acquisition_rise_count(void);
bool acquisition_ready(void);
esp_err_t acquisition_wait_ready(acq_request_t* request);
void acquisition_consume_assertion(void);
esp_err_t acquisition_wait_transfer_low(acq_request_t* request);
esp_err_t acquisition_restart_msp(acq_request_t* request);
void acquisition_discard_pending(void);
void acquisition_quiesce(void);
void acquisition_publish_pending(void);
void acquisition_receive_frame(void);
esp_err_t acquisition_execute(acq_request_t* request);
bool acquisition_process_work(void);
void acquisition_task(void* argument);
