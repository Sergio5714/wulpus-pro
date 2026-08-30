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
#include "esp_err.h"
#include "link.h"
typedef struct { link_t *link; uint32_t generation; link_kind_t kind; } wulpus_pro_session_ref_t;
esp_err_t wulpus_pro_session_init(void);
bool wulpus_pro_session_try_claim(link_t *link, wulpus_pro_session_ref_t *session);
void wulpus_pro_session_release(wulpus_pro_session_ref_t session);
wulpus_pro_session_ref_t wulpus_pro_session_current(void);
bool wulpus_pro_session_is_current(wulpus_pro_session_ref_t session);
