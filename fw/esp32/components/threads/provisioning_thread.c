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

#include "thread_internal.h"

#include "freertos/task.h"
#include "provisioner.h"

static void provisioning_task(void *argument)
{
    bool reset = (bool)(uintptr_t)argument;
    ESP_ERROR_CHECK(provisioner_start(reset));
    ESP_ERROR_CHECK(provisioner_wait());
    ESP_ERROR_CHECK(provisioner_twt_setup());
    ESP_ERROR_CHECK(tcp_thread_start());
    vTaskDelete(NULL);
}

esp_err_t provisioning_thread_start(bool reset)
{
    return xTaskCreate(provisioning_task, "provisioning", CONFIG_WP_PROVISIONING_STACK_SIZE,
                       (void *)(uintptr_t)reset, CONFIG_WP_PROVISIONING_PRIORITY, NULL) == pdPASS
               ? ESP_OK : ESP_ERR_NO_MEM;
}
