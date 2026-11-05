#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include "motor_pwm.h"

#if DT_NODE_HAS_STATUS(DT_ALIAS(mixer_pwm), okay)
#include <zephyr/drivers/pwm.h>
static const struct pwm_dt_spec motor_pwm_dev = PWM_DT_SPEC_GET(DT_ALIAS(mixer_pwm));
#define HAS_HW_PWM 1
#elif DT_NODE_HAS_STATUS(DT_ALIAS(pwm_led0), okay)
#include <zephyr/drivers/pwm.h>
static const struct pwm_dt_spec motor_pwm_dev = PWM_DT_SPEC_GET(DT_ALIAS(pwm_led0));
#define HAS_HW_PWM 1
#else
#define HAS_HW_PWM 0
#endif

static const struct gpio_dt_spec motor_gate = GPIO_DT_SPEC_GET_OR(DT_ALIAS(mixer_motor), gpios, {0});

/* Hardware-mapped or simulated PWM registers (inspectable by Renode/HIL without printk) */
volatile uint32_t g_motor_pwm_pulse_ns = 0;
volatile uint32_t g_motor_pwm_period_ns = MOTOR_PWM_PERIOD_NS;

int motor_pwm_init(void)
{
#if HAS_HW_PWM
    if (pwm_is_ready_dt(&motor_pwm_dev)) {
        printk("[HW-PWM] Initialized Hardware PWM Controller (Period: %u ns).\n", MOTOR_PWM_PERIOD_NS);
        pwm_set_dt(&motor_pwm_dev, MOTOR_PWM_PERIOD_NS, 0);
        return 0;
    }
#endif

    if (motor_gate.port != NULL && gpio_is_ready_dt(&motor_gate)) {
        printk("[HW-GPIO] Initialized Motor Gate Output pin.\n");
        gpio_pin_configure_dt(&motor_gate, GPIO_OUTPUT_INACTIVE);
    } else {
        printk("[HW-PWM] Initialized Virtual Motor PWM Channel (10 kHz, Period: %u ns).\n", MOTOR_PWM_PERIOD_NS);
    }

    g_motor_pwm_pulse_ns = 0;
    g_motor_pwm_period_ns = MOTOR_PWM_PERIOD_NS;
    return 0;
}

void motor_pwm_set_duty(uint8_t duty_percent)
{
    if (duty_percent > 100) {
        duty_percent = 100;
    }

    /* Calculate exact nanosecond pulse width: pulse = (period * duty) / 100 */
    uint32_t pulse_ns = (MOTOR_PWM_PERIOD_NS * (uint32_t)duty_percent) / 100U;
    g_motor_pwm_pulse_ns = pulse_ns;
    g_motor_pwm_period_ns = MOTOR_PWM_PERIOD_NS;

#if HAS_HW_PWM
    if (pwm_is_ready_dt(&motor_pwm_dev)) {
        pwm_set_dt(&motor_pwm_dev, MOTOR_PWM_PERIOD_NS, pulse_ns);
    }
#endif

    if (motor_gate.port != NULL && gpio_is_ready_dt(&motor_gate)) {
        gpio_pin_set_dt(&motor_gate, duty_percent > 0 ? 1 : 0);
    }
}

uint32_t motor_pwm_get_pulse_ns(void)
{
    return g_motor_pwm_pulse_ns;
}

uint32_t motor_pwm_get_period_ns(void)
{
    return g_motor_pwm_period_ns;
}
