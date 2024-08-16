#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include "button.h"
#include <zephyr/pm/pm.h>
#include <zephyr/sys/poweroff.h>
#include <zephyr/pm/device.h>
#include "utils.h"
#include "led.h"

#define SW0_NODE DT_ALIAS(sw0)
#define GND_PIN_NODE DT_ALIAS(sw0gnd)

static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(SW0_NODE, gpios);
static const struct gpio_dt_spec gnd_pin = GPIO_DT_SPEC_GET(GND_PIN_NODE, gpios);
static struct gpio_callback button_cb_data;

static int64_t press_time;
static bool is_pressed = false;

static callback_t short_press_callback = NULL;

static callback_t g_pre_sleep_callback = NULL;

void set_button_short_press_callback(callback_t callback)
{
    short_press_callback = callback;
}

void set_pre_sleep_callback(callback_t callback)
{
    g_pre_sleep_callback = callback;
}

static void cooldown_expired(struct k_work *work)
{
    ARG_UNUSED(work);

    int val = gpio_pin_get_dt(&button);
    if (val && !is_pressed) {
        // Button is pressed
        is_pressed = true;
        press_time = k_uptime_get();
    } else if (!val && is_pressed) {
        // Button is released
        is_pressed = false;
        int64_t duration = k_uptime_delta(&press_time);
        if (duration >= 2000) {
            // Long press detected
            g_pre_sleep_callback();

            // Configure button for wake-up
            gpio_pin_interrupt_configure_dt(&button, GPIO_INT_LEVEL_ACTIVE);

            sys_poweroff();
        } else if (duration >= 30 && duration < 2000) {
            // Short press detected
            short_press_callback();
        }
    }
}

static K_WORK_DELAYABLE_DEFINE(cooldown_work, cooldown_expired);

static void button_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    k_work_reschedule(&cooldown_work, K_MSEC(15));
}

int button_init(void)
{
    ASSERT_TRUE(gpio_is_ready_dt(&gnd_pin));
    ASSERT_OK(gpio_pin_configure_dt(&gnd_pin, GPIO_OUTPUT_LOW));

    ASSERT_TRUE(gpio_is_ready_dt(&button));
    ASSERT_OK(gpio_pin_configure_dt(&button, GPIO_INPUT));

    // ASSERT_OK(pm_device_action_run(button.port, PM_DEVICE_ACTION_RESUME));

    ASSERT_OK(gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_BOTH));

    gpio_init_callback(&button_cb_data, button_pressed, BIT(button.pin));
    ASSERT_OK(gpio_add_callback(button.port, &button_cb_data));

    return 0;
}