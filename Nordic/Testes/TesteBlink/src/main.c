/* Includes */
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>

/* Defines */

/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS   250

/* The devicetree node identifier for the "led0" alias. */
#define LED0_NODE DT_ALIAS(led0)
#define LED_EXT0_NODE DT_ALIAS(ledexternal0)

static const struct gpio_dt_spec led0_spec = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
static const struct gpio_dt_spec ledExt0_spec = GPIO_DT_SPEC_GET(LED_EXT0_NODE, gpios);

int main(void)
{
	int ret;

	ret = gpio_pin_configure_dt(&led0_spec, GPIO_OUTPUT_ACTIVE);
	ret = gpio_pin_configure_dt(&ledExt0_spec, GPIO_OUTPUT_ACTIVE);

	if (ret < 0) 
		return 0;
	
	while (1) 
	{
		ret = gpio_pin_toggle_dt(&led0_spec);
		ret = gpio_pin_toggle_dt(&ledExt0_spec);

		if (ret < 0) 
			return 0;
		
		printk("Hello World!");
		k_msleep(SLEEP_TIME_MS);
	}
	return 0;
}
