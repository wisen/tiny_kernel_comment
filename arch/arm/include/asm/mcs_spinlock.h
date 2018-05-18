#ifndef __ASM_MCS_LOCK_H
#define __ASM_MCS_LOCK_H

#if 1//#ifdef CONFIG_SMP//CONFIG_SMP=y
#include <asm/spinlock.h>

/* MCS spin-locking. */
#define arch_mcs_spin_lock_contended(lock)				\
do {									\
	/* Ensure prior stores are observed before we enter wfe. */	\
	smp_mb();							\
	while (!(smp_load_acquire(lock)))				\
		wfe();							\
} while (0)								\

#define arch_mcs_spin_unlock_contended(lock)				\
do {									\
	smp_store_release(lock, 1);					\
	dsb_sev();							\
} while (0)

#endif	/* CONFIG_SMP */
#endif	/* __ASM_MCS_LOCK_H */
