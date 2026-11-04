#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include "slider_gpio.h"

#define MAX_SPEED CONFIG_APP_MAX_SPEED_LEVELS

/* Detect GPIO controller port: Works across NXP RW610 (gpio0), STM32 (gpiob), or generic */
#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpiob), okay)
static const struct device *const slider_dev = DEVICE_DT_GET(DT_NODELABEL(gpiob));
#define SLIDER_PORT_NAME "PB"
#elif DT_NODE_HAS_STATUS(DT_NODELABEL(gpio0), okay)
static const struct device *const slider_dev = DEVICE_DT_GET(DT_NODELABEL(gpio0));
#define SLIDER_PORT_NAME "GPIO0_PIN"
#elif DT_NODE_HAS_STATUS(DT_NODELABEL(gpio1), okay)
static const struct device *const slider_dev = DEVICE_DT_GET(DT_NODELABEL(gpio1));
#define SLIDER_PORT_NAME "GPIO1_PIN"
#else
static const struct device *const slider_dev = NULL;
#define SLIDER_PORT_NAME "SIM_PIN"
#endif

/* Virtual simulation injection register for DevOps testing */
static volatile uint8_t s_simulated_speed = 0;

int slider_gpio_init(void)
{
    if (slider_dev != NULL && device_is_ready(slider_dev)) {
        for (int pin = 0; pin <= MAX_SPEED; pin++) {
            gpio_pin_configure(slider_dev, pin, GPIO_INPUT | GPIO_PULL_DOWN);
        }
        printk("[HW] Configured %s0-%s%d as Slider GPIO Inputs (Pull-Down enabled).\n", 
               SLIDER_PORT_NAME, SLIDER_PORT_NAME, MAX_SPEED);
        return 0;
    }

    printk("[HW-SIM] No hardware GPIO controller found. Initialized Slider Virtual Test Port.\n");
    return 0;
}

uint8_t slider_gpio_read(void)
{
    if (slider_dev != NULL && device_is_ready(slider_dev)) {
        for (int pin = MAX_SPEED; pin >= 1; pin--) {
            if (gpio_pin_get(slider_dev, pin) > 0) {
                return (uint8_t)pin;
            }
        }
        return 0;
    }

    /* Fallback to virtual test register for Renode simulation */
    return s_simulated_speed;
}

/* API for DevOps test runners to inject speed changes in virtual simulation */
void slider_gpio_set_simulated_speed(uint8_t speed)
{
    if (speed > MAX_SPEED) {
        speed = MAX_SPEED;
    }
    s_simulated_speed = speed;
}
