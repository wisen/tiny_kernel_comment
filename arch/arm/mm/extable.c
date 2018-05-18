/*
 *  linux/arch/arm/mm/extable.c
 */
#include <linux/module.h>
#include <linux/uaccess.h>

int fixup_exception(struct pt_regs *regs)
{
	const struct exception_table_entry *fixup;

	fixup = search_exception_tables(instruction_pointer(regs));
	if (fixup) {
		regs->ARM_pc = fixup->fixup;
#if 0//#ifdef CONFIG_THUMB2_KERNEL//CONFIG_THUMB2_KERNEL is not set
		/* Clear the IT state to avoid nasty surprises in the fixup */
		regs->ARM_cpsr &= ~PSR_IT_MASK;
#endif
	}

	return fixup != NULL;
}
