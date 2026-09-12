#pragma once
#include <stdbool.h>
#include "esp_err.h"
#include "msp430_image.h"
#include "msp430_programmer.h"
typedef esp_err_t (*msp430_jtag_progress_fn)(uint32_t, uint32_t, bool, void *);
esp_err_t msp430_jtag_program(const msp430_image_t *, msp430_jtag_progress_fn,
                              void *, uint32_t *, msp430_diagnostics_t *);
void msp430_jtag_release(void);
