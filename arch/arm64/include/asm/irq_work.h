#ifndef __ASM_IRQ_WORK_H
#define __ASM_IRQ_WORK_H

#if 1//#ifdef CONFIG_SMP//CONFIG_SMP=y

#include <asm/smp.h>

static inline bool arch_irq_work_has_interrupt(void)
{
	return !!__smp_cross_call;
}

#else

static inline bool arch_irq_work_has_interrupt(void)
{
	return false;
}

#endif

#endif /* __ASM_IRQ_WORK_H */
