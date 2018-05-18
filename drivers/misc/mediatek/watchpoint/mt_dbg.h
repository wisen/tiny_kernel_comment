#ifndef __MT_DBG_H
#define __MT_DBG_H

/* #define DBG_REG_DUMP */

unsigned int *mt_save_dbg_regs(unsigned int *p, unsigned int cpuid);
void mt_restore_dbg_regs(unsigned int *p, unsigned int cpuid);
void mt_copy_dbg_regs(int to, int from);
#ifdef DBG_REG_DUMP
void dump_dbgregs(int cpuid);
void print_dbgregs(int cpuid);
#endif
extern int get_cluster_core_count(void);
#endif				/* !__HW_BREAKPOINT_H */

#if 1//#ifdef CONFIG_SMP//CONFIG_SMP=y
int __cpuinit dbgregs_hotplug_callback(struct notifier_block *nfb, unsigned long action, void *hcpu);
#endif
