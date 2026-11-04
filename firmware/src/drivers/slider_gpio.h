#ifndef SLIDER_GPIO_H
#define SLIDER_GPIO_H

#include <stdint.h>

int slider_gpio_init(void);
uint8_t slider_gpio_read(void);
void slider_gpio_set_simulated_speed(uint8_t speed);

#endif /* SLIDER_GPIO_H */
