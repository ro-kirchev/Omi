#pragma once

#include <zephyr/kernel.h>

typedef void (*callback_t)(void);

int button_init(void);

void set_button_short_press_callback(callback_t callback);
void set_pre_sleep_callback(callback_t callback);