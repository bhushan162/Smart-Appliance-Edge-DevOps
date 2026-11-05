#ifndef MOTOR_PWM_H
#define MOTOR_PWM_H

#include <stdint.h>

/* Standard Motor Switching Frequency: 10 kHz (Period = 100 microseconds = 100,000 nanoseconds) */
#define MOTOR_PWM_PERIOD_NS 100000U

/* Hardware-mapped or simulated PWM registers (inspectable by Renode/HIL without printk) */
extern volatile uint32_t g_motor_pwm_pulse_ns;
extern volatile uint32_t g_motor_pwm_period_ns;

int motor_pwm_init(void);
void motor_pwm_set_duty(uint8_t duty_percent);
uint32_t motor_pwm_get_pulse_ns(void);
uint32_t motor_pwm_get_period_ns(void);

#endif /* MOTOR_PWM_H */
