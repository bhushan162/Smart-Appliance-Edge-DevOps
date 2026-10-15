#ifndef MOTOR_PWM_H
#define MOTOR_PWM_H

#include <stdint.h>

int motor_pwm_init(void);
void motor_pwm_set_duty(uint8_t duty_percent);

#endif /* MOTOR_PWM_H */
