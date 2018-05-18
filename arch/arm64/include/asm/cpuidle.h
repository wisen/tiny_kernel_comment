#ifndef __ASM_CPUIDLE_H
#define __ASM_CPUIDLE_H

#if 1//#ifdef //
extern int cpu_init_idle(unsigned int cpu);
#else
static inline int cpu_init_idle(unsigned int cpu)
{
	return -EOPNOTSUPP;
}
#endif

#endif
