#ifndef CPLD_GPIO_H_
#define CPLD_GPIO_H_


extern int cpld_gpio_request(unsigned gpio);
extern void cpld_gpio_free(unsigned gpio);
extern int cpld_gpio_direction_input(unsigned gpio);
extern int cpld_gpio_direction_output(unsigned gpio, int value);
extern int cpld_gpio_get_value(unsigned gpio);
extern void cpld_gpio_set_value(unsigned gpio, int value);

extern int cpld_gpio_to_irq(unsigned gpio);
extern int cpld_gpio_irq_to_gpio(unsigned irq);

#endif
