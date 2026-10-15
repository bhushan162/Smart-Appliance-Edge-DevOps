#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include "motor_pwm.h"

static const struct gpio_dt_spec motor_gate = GPIO_DT_SPEC_GET_OR(DT_ALIAS(mixer_motor), gpios, {0});

int motor_pwm_init(void)
{
    if (motor_gate.port != NULL) {
        if (!gpio_is_ready_dt(&motor_gate)) {
            return -1;
        }
        printk("[HW] Configured PA5 as Motor Driver output.\n");
        return gpio_pin_configure_dt(&motor_gate, GPIO_OUTPUT_INACTIVE);
    }
    return 0;
}

void motor_pwm_set_duty(uint8_t duty_percent)
{
    if (motor_gate.port != NULL) {
        gpio_pin_set_dt(&motor_gate, duty_percent > 0 ? 1 : 0);
    }
}
