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

/*
 * Mutable state shared by the acquisition implementation files. Keep state
 * that belongs to only one file static instead of adding it here.
 */
/* Used by the DATA_READY ISR to wake the owner task. */
extern TaskHandle_t acquisition_task_handle;
/* Serializes request cancellation and ISR edge accounting. */
extern portMUX_TYPE acquisition_lock;
/* State-machine fields used by command, handshake, and frame handling. */
extern acq_state_t acquisition_state;
extern bool acquisition_configured;
extern bool acquisition_reset_asserted;
extern wulpus_pro_session_ref_t acquisition_owner;
/* Frame retained while configured but not actively publishing. */
extern wulpus_pro_frame_slot_t* acquisition_pending_frame;

/* Request and error helpers shared by the command and worker modules. */
bool acquisition_request_cancelled(acq_request_t* request);
void acquisition_report_error(esp_err_t result);

/* DATA_READY interrupt and MSP430 handshake operations. */
void acquisition_data_ready_isr(void* argument);
uint32_t acquisition_data_ready_rise_count(void);
bool acquisition_data_ready_pending(void);
esp_err_t acquisition_wait_ready(acq_request_t* request);
void acquisition_consume_assertion(void);
void acquisition_reset_handshake(void);
esp_err_t acquisition_wait_transfer_low(acq_request_t* request);
esp_err_t acquisition_restart_msp(acq_request_t* request);

/* Frame ownership operations invoked by the state-machine owner. */
void acquisition_quiesce(void);
void acquisition_publish_pending(void);
void acquisition_receive_frame(void);

/* Owner-task entry points. */
bool acquisition_process_work(void);
