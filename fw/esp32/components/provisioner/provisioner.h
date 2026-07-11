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

#ifndef PROVISIONER_H
#define PROVISIONER_H

#include <stdbool.h>

#include "esp_err.h"

esp_err_t provisioner_init(void);
esp_err_t provisioner_reset(void);

esp_err_t provisioner_start(bool reset);
esp_err_t provisioner_stop(void);

esp_err_t provisioner_wait(void);

esp_err_t provisioner_twt_setup(void);
esp_err_t provisioner_twt_suspend(int time);

#endif
