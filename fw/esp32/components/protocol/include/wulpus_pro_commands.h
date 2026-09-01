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
#include <stdint.h>
typedef enum {
    WULPUS_PRO_SET_CONFIG = 0x57,
    WULPUS_PRO_GET_DATA = 0x58,
    WULPUS_PRO_PING = 0x59,
    WULPUS_PRO_PONG = 0x5A,
    WULPUS_PRO_RESET = 0x5B,
    WULPUS_PRO_RESET_MSP = 0x63,
    WULPUS_PRO_CLOSE = 0x5C,
    WULPUS_PRO_START_RX = 0x5D,
    WULPUS_PRO_STOP_RX = 0x5E,
    WULPUS_PRO_BUSY = 0x5F,
    WULPUS_PRO_GET_STATUS = 0x60,
    WULPUS_PRO_STATUS = 0x61,
    WULPUS_PRO_CLEAR_STATUS = 0x62,
} wulpus_pro_command_t;
const char *wulpus_pro_command_name(wulpus_pro_command_t command);
