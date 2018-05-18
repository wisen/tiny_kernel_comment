#ifndef __ASM_ARM_CP15_H
#define __ASM_ARM_CP15_H

#include <asm/barrier.h>

/*
 * CR1 bits (CP#15 CR1)
 */
#define CR_M	(1 << 0)	/* MMU enable				*/
#define CR_A	(1 << 1)	/* Alignment abort enable		*/
#define CR_C	(1 << 2)	/* Dcache enable			*/
#define CR_W	(1 << 3)	/* Write buffer enable			*/
#define CR_P	(1 << 4)	/* 32-bit exception handler		*/
#define CR_D	(1 << 5)	/* 32-bit data address range		*/
#define CR_L	(1 << 6)	/* Implementation defined		*/
#define CR_B	(1 << 7)	/* Big endian				*/
#define CR_S	(1 << 8)	/* System MMU protection		*/
#define CR_R	(1 << 9)	/* ROM MMU protection			*/
#define CR_F	(1 << 10)	/* Implementation defined		*/
#define CR_Z	(1 << 11)	/* Implementation defined		*/
#define CR_I	(1 << 12)	/* Icache enable			*/
#define CR_V	(1 << 13)	/* Vectors relocated to 0xffff0000	*/
#define CR_RR	(1 << 14)	/* Round Robin cache replacement	*/
#define CR_L4	(1 << 15)	/* LDR pc can set T bit			*/
#define CR_DT	(1 << 16)
#if 1//#ifdef CONFIG_MMU//CONFIG_MMU=y
#define CR_HA	(1 << 17)	/* Hardware management of Access Flag   */
#else
#define CR_BR	(1 << 17)	/* MPU Background region enable (PMSA)  */
#endif
#define CR_IT	(1 << 18)
#define CR_ST	(1 << 19)
#define CR_FI	(1 << 21)	/* Fast interrupt (lower latency mode)	*/
#define CR_U	(1 << 22)	/* Unaligned access operation		*/
#define CR_XP	(1 << 23)	/* Extended page tables			*/
#define CR_VE	(1 << 24)	/* Vectored interrupts			*/
#define CR_EE	(1 << 25)	/* Exception (Big) Endian		*/
#define CR_TRE	(1 << 28)	/* TEX remap enable			*/
#define CR_AFE	(1 << 29)	/* Access flag enable			*/
#define CR_TE	(1 << 30)	/* Thumb exception enable		*/

#ifndef __ASSEMBLY__

#if __LINUX_ARM_ARCH__ >= 4
#define vectors_high()	(get_cr() & CR_V)
#else
#define vectors_high()	(0)
#endif

#if 1//#ifdef //

extern unsigned long cr_alignment;	/* defined in entry-armv.S */

static inline unsigned long get_cr(void)
{
	unsigned long val;
	asm("mrc p15, 0, %0, c1, c0, 0	@ get CR" : "=r" (val) : : "cc");//wisen: ��CP15�ļĴ���C1��ֵ����val��
	//wisen: CP15��C1�Ĵ����ĸ�λ���壺
	/*
CP15�еļĴ���C1�ı����ʽ������˵��

31 16| 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0
���� | L4 RR V  I  Z  F  R S B L D P W C A M

λ |                  ˵��
M  | 0����ֹMMU           1��ʹ��MMU
A  | 0: ��ֹ��ַ������  1��ʹ�ܵ�ַ������
C  | 0����ֹ����cache     1��ʹ��cache
W  | 0����ֹд����        1��ʹ��д����
P  | 0���쳣�жϴ���������32λ��ַģʽ 1���쳣�жϴ���������26λ��ַģʽ
D  | 0: ��ֹ26λ��ַ�쳣��� 1��ʹ��26λ��ַ�쳣���
L  | 0��ѡ��������ֹģ��  1�� ѡ�������ֹģʽ
B  | 0��little endian     1�� big endian
S  | �ڻ���MMU�Ĵ洢ϵͳ�У���λ����ϵͳ����
R  | �ڻ���MMU�Ĵ洢ϵͳ�У���λ����ROM����
F  | �������̶���
Z  | 0����ֹ��תԤ�⹦��  1��ʹ����תԤ��
I  | 0����ָֹ��cache     1��ʹ��ָ��cache
V  | 0��ѡ��Ͷ��쳣�ж�����0x0~0x1c 1: ѡ��߶��쳣�ж�����0xffff0000~0xffff001c
	*/
	return val;
}

static inline void set_cr(unsigned long val)
{
	asm volatile("mcr p15, 0, %0, c1, c0, 0	@ set CR"
	  : : "r" (val) : "cc");
	isb();
}

static inline unsigned int get_auxcr(void)
{
	unsigned int val;
	asm("mrc p15, 0, %0, c1, c0, 1	@ get AUXCR" : "=r" (val));
	return val;
}

static inline void set_auxcr(unsigned int val)
{
	asm volatile("mcr p15, 0, %0, c1, c0, 1	@ set AUXCR"
	  : : "r" (val));
	isb();
}

#define CPACC_FULL(n)		(3 << (n * 2))
#define CPACC_SVC(n)		(1 << (n * 2))
#define CPACC_DISABLE(n)	(0 << (n * 2))

static inline unsigned int get_copro_access(void)
{
	unsigned int val;
	asm("mrc p15, 0, %0, c1, c0, 2 @ get copro access"
	  : "=r" (val) : : "cc");
	return val;
}

static inline void set_copro_access(unsigned int val)
{
	asm volatile("mcr p15, 0, %0, c1, c0, 2 @ set copro access"
	  : : "r" (val) : "cc");
	isb();
}

#else /* ifdef CONFIG_CPU_CP15 */

/*
 * cr_alignment is tightly coupled to cp15 (at least in the minds of the
 * developers). Yielding 0 for machines without a cp15 (and making it
 * read-only) is fine for most cases and saves quite some #ifdeffery.
 */
#define cr_alignment	UL(0)

static inline unsigned long get_cr(void)
{
	return 0;
}

#endif /* ifdef CONFIG_CPU_CP15 / else */

#endif /* ifndef __ASSEMBLY__ */

#endif
