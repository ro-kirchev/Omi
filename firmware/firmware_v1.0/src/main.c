#include <zephyr/kernel.h>
#include "transport.h"
#include "mic.h"
#include "utils.h"
#include "led.h"
#include "config.h"
#include "audio.h"
#include "codec.h"
#include "button.h"

#ifdef TRIANGLE_V2
#include <zephyr/pm/pm.h>
#include <zephyr/pm/device.h>
#endif

static void codec_handler(uint8_t *data, size_t len)
{
	broadcast_audio_packets(data, len); // Errors are logged inside
}

static void mic_handler(int16_t *buffer)
{
	codec_receive_pcm(buffer, MIC_BUFFER_SAMPLES); // Errors are logged inside
}

void bt_ctlr_assert_handle(char *name, int type)
{
	if (name != NULL)
	{
		printk("Bt assert-> %s", name);
	}
}

#ifdef TRIANGLE_V2
static void pre_sleep_callback(void)
{
    printk("Preparing for deep sleep...\n");

	// Disable USB
	// const struct device *usb_dev = DEVICE_DT_GET(DT_NODELABEL(usbd));
    // if (device_is_ready(usb_dev)) {
    //     pm_device_action_run(usb_dev, PM_DEVICE_ACTION_SUSPEND);
    // }
    
    set_led_red(false);
    set_led_green(false);
    set_led_blue(false);

	const struct device *const cons = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
		if (device_is_ready(cons)) {
			pm_device_action_run(cons, PM_DEVICE_ACTION_SUSPEND);
		}
}

void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id)
{
    // Any necessary state restoration here
}

static void button_short_press_handler(void) {
    printk("Button short press detected!\n");
    // Add your custom short press logic here
}
#endif

bool is_connected = false;
bool is_charging = false;

void set_led_state()
{
	// Recording and connected state - BLUE
	if (is_connected)
	{
		set_led_red(false);
		set_led_green(false);
		set_led_blue(true);
		return;
	}

	// Recording but lost connection - RED
	if (!is_connected)
	{
		set_led_red(true);
		set_led_green(false);
		set_led_blue(false);
		return;
	}

	// Not recording, but charging - WHITE
	if (is_charging)
	{
		set_led_red(true);
		set_led_green(true);
		set_led_blue(true);
		return;
	}

	// Not recording - OFF
	set_led_red(false);
	set_led_green(false);
	set_led_blue(false);
}

// Main loop
int main(void)
{
	// Led start
	ASSERT_OK(led_start());
	set_led_blue(true);

#ifdef TRIANGLE_V2
	ASSERT_OK(button_init());
    set_pre_sleep_callback(pre_sleep_callback);
	set_button_short_press_callback(button_short_press_handler);
#endif

	// Transport start
	ASSERT_OK(transport_start());

	// Codec start
	set_codec_callback(codec_handler);
	ASSERT_OK(codec_start());

	// Mic start
	set_mic_callback(mic_handler);
	ASSERT_OK(mic_start());

	while (1)
	{
		set_led_state();
		k_msleep(500);
	}

	// Unreachable
	return 0;
}
