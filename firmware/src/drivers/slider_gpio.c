#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include "slider_gpio.h"

#define MAX_SPEED CONFIG_APP_MAX_SPEED_LEVELS

#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpiob), okay)
#define HAS_GPIOB 1
static const struct device *const gpiob_dev = DEVICE_DT_GET(DT_NODELABEL(gpiob));
#else
#define HAS_GPIOB 0
static const struct device *const gpiob_dev = NULL;
#endif

int slider_gpio_init(void)
{
#if HAS_GPIOB
    if (!device_is_ready(gpiob_dev)) {
        return -1;
    }
    for (int pin = 0; pin <= MAX_SPEED; pin++) {
        gpio_pin_configure(gpiob_dev, pin, GPIO_INPUT | GPIO_PULL_DOWN);
    }
    printk("[HW] Configured PB0-PB10 as Slider GPIO Inputs (Pull-Down enabled).\n");
    return 0;
#else
    return -1;
#endif
}

uint8_t slider_gpio_read(void)
{
#if HAS_GPIOB
    if (!device_is_ready(gpiob_dev)) return 0;
    for (int pin = MAX_SPEED; pin >= 1; pin--) {
        if (gpio_pin_get(gpiob_dev, pin) > 0) return (uint8_t)pin;
    }
#endif
    return 0;
}
