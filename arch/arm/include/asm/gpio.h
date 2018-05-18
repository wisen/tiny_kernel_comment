#ifndef _ARCH_ARM_GPIO_H
#define _ARCH_ARM_GPIO_H

#if 0/*CONFIG_ARCH_NR_GPIO=0*/ > 0
#define ARCH_NR_GPIOS 0/*CONFIG_ARCH_NR_GPIO=0*/
#endif

/* Note: this may rely upon the value of ARCH_NR_GPIOS set in mach/gpio.h */
#include <asm-generic/gpio.h>

/* The trivial gpiolib dispatchers */
#define gpio_get_value  __gpio_get_value
#define gpio_set_value  __gpio_set_value
#define gpio_cansleep   __gpio_cansleep

/*
 * Provide a default gpio_to_irq() which should satisfy every case.
 * However, some platforms want to do this differently, so allow them
 * to override it.
 */
#ifndef gpio_to_irq
#define gpio_to_irq	__gpio_to_irq
#endif

#endif /* _ARCH_ARM_GPIO_H */
