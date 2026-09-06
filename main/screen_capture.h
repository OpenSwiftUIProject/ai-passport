#pragma once
#include "esp_err.h"

// Start the USB screenshot command worker after the UI has been initialized.
esp_err_t screen_capture_start(void);
