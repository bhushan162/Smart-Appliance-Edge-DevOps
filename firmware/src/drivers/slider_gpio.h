#ifndef SLIDER_GPIO_H
#define SLIDER_GPIO_H

#include <stdint.h>

int slider_gpio_init(void);
uint8_t slider_gpio_read(void);

#endif /* SLIDER_GPIO_H */
