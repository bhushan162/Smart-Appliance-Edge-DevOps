#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include "power_btn.h"

static const struct gpio_dt_spec pwr_btn = GPIO_DT_SPEC_GET_OR(DT_ALIAS(mixer_power), gpios, {0});

int power_btn_init(void)
{
    if (pwr_btn.port != NULL) {
        if (!gpio_is_ready_dt(&pwr_btn)) {
            return -1;
        }
        printk("[HW] Master ON/OFF Power button configured.\n");
        return gpio_pin_configure_dt(&pwr_btn, GPIO_INPUT);
    }
    return 0;
}

bool power_btn_is_on(void)
{
    if (pwr_btn.port != NULL) {
        return gpio_pin_get_dt(&pwr_btn) > 0;
    }
    return true;
}
