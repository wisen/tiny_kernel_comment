/*
 * Copyright (C) 2016 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/of_fdt.h>

#if 1//#ifdef CONFIG_OF//CONFIG_OF=y
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#endif

#include <mach/irqs.h>
#if 0 /* defined(CONFIG_MTK_SYS_CIRQ) */
#include <mt-plat/mt_cirq.h>
#endif
#include "mt_spm_idle.h"
#include "mt_cpuidle.h"
#ifdef CONFIG_MTK_WD_KICKER
#include <mach/wd_api.h>
#endif
#include <mach/mt_gpt.h>
#include <mt-plat/mt_ccci_common.h>
#include "mt_spm_misc.h"
#include <mt-plat/upmu_common.h>

#include "mt_spm_dpidle.h"
#include "mt_spm_internal.h"
#include "mt_spm_pmic_wrap.h"

#include <mt-plat/mt_io.h>

/*
 * only for internal debug
 */
#define DPIDLE_TAG     "[DP] "
#define dpidle_dbg(fmt, args...)	pr_debug(DPIDLE_TAG fmt, ##args)

#define SPM_PWAKE_EN            1
#define SPM_PCMWDT_EN           1
#define SPM_BYPASS_SYSPWREQ     0

#define WAKE_SRC_FOR_MD32  0

#define I2C_CHANNEL 2

#define spm_is_wakesrc_invalid(wakesrc)     (!!((u32)(wakesrc) & 0xc0003803))

#define CA70_BUS_CONFIG          0xF020002C  /* (CA7MCUCFG_BASE + 0x1C) - 0x1020011c */
#define CA71_BUS_CONFIG          0xF020022C  /* (CA7MCUCFG_BASE + 0x1C) - 0x1020011c */

#ifdef CONFIG_MTK_RAM_CONSOLE
#define SPM_AEE_RR_REC 1
#else
#define SPM_AEE_RR_REC 0
#endif
#define SPM_USE_TWAM_DEBUG	0

#define	DPIDLE_LOG_PRINT_TIMEOUT_CRITERIA	20
#define	DPIDLE_LOG_DISCARD_CRITERIA			5000	/* ms */

#define reg_read(addr)         __raw_readl(IOMEM(addr))

#if 1//#ifdef CONFIG_OF//CONFIG_OF=y
#define MCUCFG_BASE             spm_mcucfg
#else
#define MCUCFG_BASE             (0xF0200000)	/* 0x1020_0000 */
#endif
#define MP0_AXI_CONFIG          (MCUCFG_BASE + 0x2C)
#define MP1_AXI_CONFIG          (MCUCFG_BASE + 0x22C)
#define MP2_AXI_CONFIG          (MCUCFG_BASE + 0x220C)
#define ACINACTM                (1 << 4)

#if SPM_AEE_RR_REC
enum spm_deepidle_step {
	SPM_DEEPIDLE_ENTER = 0x00000001,
	SPM_DEEPIDLE_ENTER_UART_SLEEP = 0x00000003,
	SPM_DEEPIDLE_ENTER_WFI = 0x000000ff,
	SPM_DEEPIDLE_LEAVE_WFI = 0x000001ff,
	SPM_DEEPIDLE_ENTER_UART_AWAKE = 0x000003ff,
	SPM_DEEPIDLE_LEAVE = 0x000007ff,
	SPM_DEEPIDLE_SLEEP_DPIDLE = 0x80000000
};
#endif

static inline void spm_dpidle_footprint(enum spm_deepidle_step step)
{
#if SPM_AEE_RR_REC
	aee_rr_rec_deepidle_val(step);
#endif
}

static const u32 dpidle_binary[] = {
	0xa1d58407, 0xa1d70407, 0x81f68407, 0x80358400, 0x1b80001f, 0x20000000,
	0x80300400, 0x80328400, 0xa1d28407, 0x81f20407, 0x81f88407, 0xe8208000,
	0x108c0610, 0x20000000, 0x81431801, 0xd8000245, 0x17c07c1f, 0x80318400,
	0x81f00407, 0xa1dd0407, 0x1b80001f, 0x20000424, 0x81fd0407, 0x18c0001f,
	0x108c0604, 0x1910001f, 0x108c0604, 0x813f8404, 0xe0c00004, 0xc28055c0,
	0x1290041f, 0xa19d8406, 0xa1dc0407, 0x1880001f, 0x00000006, 0xc0c05fc0,
	0x17c07c1f, 0xa0800c02, 0x1300081f, 0xf0000000, 0x17c07c1f, 0x81fc0407,
	0x1b00001f, 0xffffffff, 0x81fc8407, 0x1b80001f, 0x20000004, 0x88c0000c,
	0xffffffff, 0xd8200703, 0x17c07c1f, 0xa1dc0407, 0x1b00001f, 0x00000000,
	0xd0000ba0, 0x17c07c1f, 0xe8208000, 0x108c0610, 0x10000000, 0x1880001f,
	0x108c0028, 0xc0c054a0, 0xe080000f, 0xd82008e3, 0x17c07c1f, 0x81f00407,
	0xa1dc0407, 0x1b00001f, 0x00000006, 0xd0000ba0, 0x17c07c1f, 0xe080001f,
	0x81431801, 0xd8000985, 0x17c07c1f, 0xa0118400, 0x81bd8406, 0xa0128400,
	0xa1dc0407, 0x1b00001f, 0x00000001, 0xc28055c0, 0x129f841f, 0xc28055c0,
	0x1290841f, 0xa1d88407, 0xa1d20407, 0x81f28407, 0xa1d68407, 0xa0100400,
	0xa0158400, 0x81f70407, 0x81f58407, 0xf0000000, 0x17c07c1f, 0x81409801,
	0xd8000d45, 0x17c07c1f, 0x18c0001f, 0x108c0318, 0xc0c04b40, 0x1200041f,
	0x80310400, 0x1b80001f, 0x2000000e, 0xa0110400, 0xe8208000, 0x108c0610,
	0x40000000, 0xe8208000, 0x108c0610, 0x00000000, 0xc28055c0, 0x1291041f,
	0x18c0001f, 0x108c01d0, 0x1910001f, 0x108c01d0, 0xa1100404, 0xe0c00004,
	0xa19e0406, 0xa1dc0407, 0x1880001f, 0x00000058, 0xc0c05fc0, 0x17c07c1f,
	0xa0800c02, 0x1300081f, 0xf0000000, 0x17c07c1f, 0x18c0001f, 0x108c01d0,
	0x1910001f, 0x108c01d0, 0x81300404, 0xe0c00004, 0x81409801, 0xd80011e5,
	0x17c07c1f, 0x18c0001f, 0x108c0318, 0xc0c04fe0, 0x17c07c1f, 0xc28055c0,
	0x1291841f, 0x81be0406, 0xa1dc0407, 0x1880001f, 0x00000006, 0xc0c05fc0,
	0x17c07c1f, 0xa0800c02, 0x1300081f, 0xf0000000, 0x17c07c1f, 0xc0804240,
	0x17c07c1f, 0xc0803f80, 0x17c07c1f, 0xe8208000, 0x102101a4, 0x00010000,
	0x18c0001f, 0x10230068, 0x1910001f, 0x10230068, 0x89000004, 0xffffff0f,
	0xe0c00004, 0x1910001f, 0x10230068, 0xe8208000, 0x108c0408, 0x00001fff,
	0xa1d80407, 0xc28055c0, 0x1292041f, 0xa19e8406, 0x80cf1801, 0xa1dc0407,
	0xd8001743, 0x17c07c1f, 0x1880001f, 0x00010060, 0xd0001780, 0x17c07c1f,
	0x1880001f, 0x000100a0, 0xc0c05fc0, 0x17c07c1f, 0xa0800c02, 0x1300081f,
	0xf0000000, 0x17c07c1f, 0x1b80001f, 0x20000fdf, 0x81f80407, 0xe8208000,
	0x108c0408, 0x0000ffff, 0x1910001f, 0x108c0170, 0xd8201903, 0x80c41001,
	0x80cf9801, 0xd8001a23, 0x17c07c1f, 0xc0c03f80, 0x17c07c1f, 0x18c0001f,
	0x10230068, 0x1910001f, 0x10230068, 0xa9000004, 0x000000f0, 0xe0c00004,
	0x1910001f, 0x10230068, 0x1880001f, 0x108c0028, 0xc0c051a0, 0xe080000f,
	0xd8201c63, 0x17c07c1f, 0xa1d80407, 0xd0002100, 0x17c07c1f, 0xe080001f,
	0x1890001f, 0x108c0604, 0x80c68801, 0x81429801, 0xa1400c05, 0xd8001de5,
	0x17c07c1f, 0xc0c05780, 0x17c07c1f, 0xc28055c0, 0x1296841f, 0xe8208000,
	0x102101a8, 0x00070000, 0xc0803c00, 0x17c07c1f, 0xc0803d20, 0x17c07c1f,
	0xc28055c0, 0x1292841f, 0x81be8406, 0xa19f8406, 0x80cf1801, 0xa1dc0407,
	0xd8002043, 0x17c07c1f, 0x1880001f, 0x00000058, 0xd0002080, 0x17c07c1f,
	0x1880001f, 0x00000090, 0xc0c05fc0, 0x17c07c1f, 0xa0800c02, 0x1300081f,
	0xf0000000, 0x17c07c1f, 0x814e9801, 0xd82021e5, 0x17c07c1f, 0xc0c03f80,
	0x17c07c1f, 0x18c0001f, 0x108c038c, 0xe0e00011, 0xe0e00031, 0xe0e00071,
	0xe0e000f1, 0xe0e001f1, 0xe0e003f1, 0xe0e007f1, 0xe0e00ff1, 0xe0e01ff1,
	0xe0e03ff1, 0xe0e07ff1, 0xe0f07ff1, 0x1b80001f, 0x20000020, 0xe0f07ff3,
	0xe0f07ff2, 0x80350400, 0x1b80001f, 0x2000001a, 0x80378400, 0x1b80001f,
	0x20000208, 0x80338400, 0x1b80001f, 0x2000001a, 0x81f98407, 0xc28055c0,
	0x1293041f, 0xa19f0406, 0x80ce9801, 0xa1dc0407, 0xd80026c3, 0x17c07c1f,
	0x1880001f, 0x00000090, 0xd00027e0, 0x17c07c1f, 0x80cf9801, 0xd80027a3,
	0x17c07c1f, 0x1880001f, 0x000100a0, 0xd00027e0, 0x17c07c1f, 0x1880001f,
	0x000080a0, 0xc0c05fc0, 0x17c07c1f, 0xa0800c02, 0x1300081f, 0xc0c04780,
	0x17c07c1f, 0xf0000000, 0x17c07c1f, 0x814e9801, 0xd8202985, 0x17c07c1f,
	0xc0c03f80, 0x17c07c1f, 0xa1d98407, 0xa0138400, 0xa0178400, 0xa0150400,
	0x18c0001f, 0x108c038c, 0xe0f07ff3, 0xe0f07ff1, 0xe0e00ff1, 0xe0e000f1,
	0xe0e00001, 0xc28055c0, 0x1293841f, 0x81bf0406, 0x80ce9801, 0xa1dc0407,
	0xd8002c43, 0x17c07c1f, 0x1880001f, 0x00000058, 0xd0002d60, 0x17c07c1f,
	0x80cf9801, 0xd8202d23, 0x17c07c1f, 0x1880001f, 0x00010060, 0xd0002d60,
	0x17c07c1f, 0x1880001f, 0x00008060, 0xc0c05fc0, 0x17c07c1f, 0xa0800c02,
	0x1300081f, 0xc0c04780, 0x17c07c1f, 0xf0000000, 0x17c07c1f, 0xc0c03f80,
	0x17c07c1f, 0xc28055c0, 0x1294041f, 0xa19f8406, 0x80cf1801, 0xa1dc0407,
	0xd8003003, 0x17c07c1f, 0x1880001f, 0x00010060, 0xd0003040, 0x17c07c1f,
	0x1880001f, 0x000100a0, 0xc0c05fc0, 0x17c07c1f, 0xa0800c02, 0x1300081f,
	0xf0000000, 0x17c07c1f, 0x1880001f, 0x108c0028, 0xc0c051a0, 0xe080000f,
	0xd8003443, 0x17c07c1f, 0xe080001f, 0xc0803c00, 0x17c07c1f, 0xc28055c0,
	0x1294841f, 0x81bf8406, 0x80cf1801, 0xa1dc0407, 0xd8003383, 0x17c07c1f,
	0x1880001f, 0x00008060, 0xd00033c0, 0x17c07c1f, 0x1880001f, 0x000080a0,
	0xc0c05fc0, 0x17c07c1f, 0xa0800c02, 0x1300081f, 0xf0000000, 0x17c07c1f,
	0x814e9801, 0xd8203525, 0x17c07c1f, 0xc0c03f80, 0x17c07c1f, 0xc0c05cc0,
	0x17c07c1f, 0x18c0001f, 0x108c0404, 0x1910001f, 0x108c0404, 0xa1108404,
	0xe0c00004, 0x1b80001f, 0x20000104, 0xc0c05ea0, 0x17c07c1f, 0xc28055c0,
	0x1295041f, 0xa19d0406, 0xa1dc0407, 0x1890001f, 0x108c0148, 0xa0978402,
	0x80b70402, 0x1300081f, 0xc0c04780, 0x17c07c1f, 0xf0000000, 0x17c07c1f,
	0x814e9801, 0xd82038e5, 0x17c07c1f, 0xc0c03f80, 0x17c07c1f, 0xc0c05cc0,
	0x17c07c1f, 0x18c0001f, 0x108c0404, 0x1910001f, 0x108c0404, 0x81308404,
	0xe0c00004, 0x1b80001f, 0x20000104, 0xc0c05ea0, 0x17c07c1f, 0xc28055c0,
	0x1295841f, 0x81bd0406, 0xa1dc0407, 0x1890001f, 0x108c0148, 0x80b78402,
	0xa0970402, 0x1300081f, 0xc0c04780, 0x17c07c1f, 0xf0000000, 0x17c07c1f,
	0xa0188400, 0xa0190400, 0xa0110400, 0x80398400, 0x803a0400, 0x803a8400,
	0x803b0400, 0xf0000000, 0x17c07c1f, 0xa1da0407, 0xa1dd8407, 0x803b8400,
	0xa0168400, 0xa0140400, 0xe8208000, 0x108c0320, 0x00000f0d, 0xe8208000,
	0x108c0320, 0x00000f0f, 0xe8208000, 0x108c0320, 0x00000f1e, 0xe8208000,
	0x108c0320, 0x00000f12, 0xf0000000, 0x17c07c1f, 0xa01b0400, 0xa01a8400,
	0xa01c0400, 0xa01a0400, 0x1b80001f, 0x20000004, 0xa0198400, 0x1b80001f,
	0x20000004, 0x80388400, 0x803c0400, 0x80310400, 0x1b80001f, 0x20000004,
	0x80390400, 0x81f08407, 0x808ab401, 0xd8004182, 0x17c07c1f, 0x80380400,
	0xf0000000, 0x17c07c1f, 0xe8208000, 0x108c0320, 0x00000f16, 0xe8208000,
	0x108c0320, 0x00000f1e, 0xe8208000, 0x108c0320, 0x00000f0e, 0x1b80001f,
	0x2000001b, 0xe8208000, 0x108c0320, 0x00000f0c, 0xe8208000, 0x108c0320,
	0x00000f0d, 0xe8208000, 0x108c0320, 0x00000e0d, 0xe8208000, 0x108c0320,
	0x00000c0d, 0xe8208000, 0x108c0320, 0x0000080d, 0xe8208000, 0x108c0320,
	0x0000000d, 0x80340400, 0x80368400, 0x1b80001f, 0x20000209, 0xa01b8400,
	0x1b80001f, 0x20000209, 0x1b80001f, 0x20000209, 0x81fa0407, 0x81fd8407,
	0xf0000000, 0x17c07c1f, 0x816f9801, 0x814e9805, 0xd8204b05, 0x17c07c1f,
	0x1880001f, 0x108c0028, 0xe080000f, 0x1111841f, 0xa1d08407, 0xd8204924,
	0x80eab401, 0xd80048a3, 0x01200404, 0x81f08c07, 0x1a00001f, 0x108c00b0,
	0xe2000003, 0xd8204aa3, 0x17c07c1f, 0xa1dc0407, 0x1b00001f, 0x00000000,
	0x81fc0407, 0xd0004b00, 0x17c07c1f, 0xe080001f, 0xc0c03c00, 0x17c07c1f,
	0xf0000000, 0x17c07c1f, 0xe0f07f16, 0x1380201f, 0xe0f07f1e, 0x1380201f,
	0xe0f07f0e, 0xe0f07f0c, 0x1900001f, 0x108c0374, 0xe120003e, 0xe120003c,
	0xe1200038, 0xe1200030, 0xe1200020, 0xe1200000, 0x1880001f, 0x108c03a4,
	0x1900001f, 0x0400fffc, 0xe0800004, 0x1900001f, 0x0000fffc, 0xe0800004,
	0x1b80001f, 0x20000104, 0xe0f07f0d, 0xe0f07e0d, 0x1b80001f, 0x20000104,
	0xe0f07c0d, 0x1b80001f, 0x20000104, 0xe0f0780d, 0x1b80001f, 0x20000104,
	0xe0f0700d, 0xf0000000, 0x17c07c1f, 0x1900001f, 0x108c0374, 0xe120003f,
	0x1900001f, 0x108c03a4, 0x1880001f, 0x0c00fffc, 0xe1000002, 0xe0f07f0d,
	0xe0f07f0f, 0xe0f07f1e, 0xe0f07f12, 0xf0000000, 0x17c07c1f, 0xa0180400,
	0x1111841f, 0xa1d08407, 0xd8205284, 0x80eab401, 0xd8005203, 0x01200404,
	0xd82053c3, 0x17c07c1f, 0xa1dc0407, 0x1b00001f, 0x00000000, 0x81fc0407,
	0x81f08407, 0xe8208000, 0x108c00b0, 0x00000001, 0xf0000000, 0x17c07c1f,
	0xa1d10407, 0x1b80001f, 0x20000020, 0xf0000000, 0x17c07c1f, 0xa1d00407,
	0x1b80001f, 0x20000208, 0x10c07c1f, 0x1900001f, 0x108c00b0, 0xe1000003,
	0xf0000000, 0x17c07c1f, 0x18c0001f, 0x108c0604, 0x1910001f, 0x108c0604,
	0xa1002804, 0xe0c00004, 0xf0000000, 0x17c07c1f, 0xa1d40407, 0x1391841f,
	0xa1d90407, 0x1392841f, 0xf0000000, 0x17c07c1f, 0x18c0001f, 0x100040f4,
	0x19300003, 0x17c07c1f, 0x813a0404, 0xe0c00004, 0x1b80001f, 0x20000003,
	0x18c0001f, 0x10004110, 0x19300003, 0x17c07c1f, 0xa11e8404, 0xe0c00004,
	0x1b80001f, 0x20000004, 0x18c0001f, 0x100041ec, 0x19300003, 0x17c07c1f,
	0x81380404, 0xe0c00004, 0x18c0001f, 0x100040f4, 0x19300003, 0x17c07c1f,
	0xa11a8404, 0xe0c00004, 0x18c0001f, 0x100041dc, 0x19300003, 0x17c07c1f,
	0x813d0404, 0xe0c00004, 0x18c0001f, 0x10004110, 0x19300003, 0x17c07c1f,
	0x813e8404, 0xe0c00004, 0xf0000000, 0x17c07c1f, 0x814f1801, 0xd8005e65,
	0x17c07c1f, 0x80350400, 0x1b80001f, 0x2000001a, 0x80378400, 0x1b80001f,
	0x20000208, 0x80338400, 0x1b80001f, 0x2000001a, 0x81f98407, 0xf0000000,
	0x17c07c1f, 0x814f1801, 0xd8005f85, 0x17c07c1f, 0xa1d98407, 0xa0138400,
	0xa0178400, 0xa0150400, 0xf0000000, 0x17c07c1f, 0x10c07c1f, 0x810d1801,
	0x810a1804, 0x816d1801, 0x814a1805, 0xa0d79003, 0xa0d71403, 0xf0000000,
	0x17c07c1f, 0x1b80001f, 0x20000300, 0xf0000000, 0x17c07c1f, 0xe8208000,
	0x110e0014, 0x00000002, 0xe8208000, 0x110e0020, 0x00000001, 0xe8208000,
	0x110e0004, 0x000000d6, 0x1a00001f, 0x110e0000, 0x1880001f, 0x110e0024,
	0x18c0001f, 0x20000152, 0xd820658a, 0x17c07c1f, 0xe220000a, 0xe22000f6,
	0xe8208000, 0x110e0024, 0x00000001, 0x1b80001f, 0x20000152, 0xe220008a,
	0xe2200001, 0xe8208000, 0x110e0024, 0x00000001, 0x1b80001f, 0x20000152,
	0xd0006740, 0x17c07c1f, 0xe220008a, 0xe2200000, 0xe8208000, 0x110e0024,
	0x00000001, 0x1b80001f, 0x20000152, 0xe220000a, 0xe22000f4, 0xe8208000,
	0x110e0024, 0x00000001, 0x1b80001f, 0x20000152, 0xf0000000, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x1840001f, 0x00000001, 0xa1d48407, 0xe8208000,
	0x108c0620, 0x0000000e, 0x1990001f, 0x108c0600, 0xa9800006, 0xfe000000,
	0x1890001f, 0x108c061c, 0x80800402, 0xa19c0806, 0xe8208000, 0x108c0604,
	0x00000000, 0x81fc0407, 0x1b00001f, 0xffffffff, 0xa1dc0407, 0x1b00001f,
	0x00000000, 0x81401801, 0xd80103c5, 0x17c07c1f, 0x1b80001f, 0x5000aaaa,
	0xd00104a0, 0x17c07c1f, 0x1b80001f, 0xd00f0000, 0x81fc8407, 0x8880000c,
	0xffffffff, 0xd8011b02, 0x17c07c1f, 0xe8208000, 0x108c0408, 0x00001fff,
	0xe8208000, 0x108c040c, 0xe0003fff, 0xc0c05400, 0x81401801, 0xd8010885,
	0x17c07c1f, 0x80c41801, 0xd8010663, 0x17c07c1f, 0x81f60407, 0x18d0001f,
	0x108c0254, 0xc0c11f20, 0x17c07c1f, 0x18d0001f, 0x108c0250, 0xc0c12000,
	0x17c07c1f, 0x18c0001f, 0x108c0200, 0xc0c120c0, 0x17c07c1f, 0xe8208000,
	0x108c0260, 0x00000007, 0xc28055c0, 0x1296041f, 0x81401801, 0xd8010a25,
	0x17c07c1f, 0xa1dc0407, 0x1b00001f, 0x00000000, 0x1b80001f, 0x30000004,
	0x81fc8407, 0x8880000c, 0xffffffff, 0xd80117c2, 0x17c07c1f, 0x81489801,
	0xd8010b25, 0x17c07c1f, 0x18c0001f, 0x108c037c, 0xe0e00013, 0xe0e00011,
	0xe0e00001, 0x81481801, 0xd8010c65, 0x17c07c1f, 0x18c0001f, 0x108c0370,
	0xe0f07ff3, 0xe0f07ff1, 0xe0e00ff1, 0xe0e000f1, 0xe0e00001, 0x81449801,
	0xd8010cc5, 0x17c07c1f, 0xc0c056c0, 0x17c07c1f, 0xa1d38407, 0xa0108400,
	0xa0120400, 0xa0130400, 0xa0170400, 0xa0148400, 0xe8208000, 0x108c0080,
	0x0000fff3, 0xa1dc0407, 0x18c0001f, 0x000100a0, 0x814a1801, 0xa0d79403,
	0x814a9801, 0xa0d89403, 0x1b00001f, 0xffffffdf, 0x1b80001f, 0x90100000,
	0x80cd9801, 0xc8e00003, 0x17c07c1f, 0x80ce1801, 0xc8e00be3, 0x17c07c1f,
	0x80cf1801, 0xc8e02143, 0x17c07c1f, 0x80cf9801, 0xc8e02e63, 0x17c07c1f,
	0x80ce9801, 0xc8e01363, 0x17c07c1f, 0x80cd1801, 0xc8e03483, 0x17c07c1f,
	0x81481801, 0xd8011465, 0x17c07c1f, 0x18c0001f, 0x108c0370, 0xe0e00011,
	0xe0e00031, 0xe0e00071, 0xe0e000f1, 0xe0e001f1, 0xe0e003f1, 0xe0e007f1,
	0xe0e00ff1, 0xe0e01ff1, 0xe0e03ff1, 0xe0e07ff1, 0xe0f07ff1, 0x1b80001f,
	0x20000020, 0xe0f07ff3, 0xe0f07ff2, 0x81489801, 0xd80115a5, 0x17c07c1f,
	0x18c0001f, 0x108c037c, 0xe0e00011, 0x1b80001f, 0x20000020, 0xe0e00013,
	0xe0e00012, 0x80348400, 0x1b80001f, 0x20000300, 0x80370400, 0x1b80001f,
	0x20000300, 0x80330400, 0x1b80001f, 0x20000104, 0x80308400, 0x80320400,
	0x81f38407, 0x81f90407, 0x81f40407, 0x81449801, 0xd80117c5, 0x17c07c1f,
	0x81401801, 0xd8011b05, 0x17c07c1f, 0x18d0001f, 0x108e0e10, 0x1890001f,
	0x108c0260, 0x81400c01, 0x81020c01, 0x80b01402, 0x80b09002, 0x18c0001f,
	0x108c0260, 0xe0c00002, 0x18c0001f, 0x108c0200, 0xc0c128a0, 0x17c07c1f,
	0x18d0001f, 0x108c0250, 0xc0c12760, 0x17c07c1f, 0x18d0001f, 0x108c0254,
	0xc0c12620, 0x17c07c1f, 0xe8208000, 0x108c0034, 0x000f0000, 0x81f48407,
	0xa1d60407, 0x81f10407, 0xe8208000, 0x108c0620, 0x0000000f, 0x18c0001f,
	0x108c041c, 0x1910001f, 0x108c0164, 0xe0c00004, 0x18c0001f, 0x108c0420,
	0x1910001f, 0x108c0150, 0xe0c00004, 0x18c0001f, 0x108c0614, 0x1910001f,
	0x108c0614, 0x09000004, 0x00000001, 0xe0c00004, 0x1ac0001f, 0x55aa55aa,
	0xe8208000, 0x108e0e00, 0x00000001, 0xd0013040, 0x17c07c1f, 0x1900001f,
	0x108c0278, 0xe100001f, 0x17c07c1f, 0xe2e01041, 0xf0000000, 0x17c07c1f,
	0x1900001f, 0x108c026c, 0xe100001f, 0xe2e01041, 0xf0000000, 0x17c07c1f,
	0x1900001f, 0x108c0204, 0x1940001f, 0x0000104d, 0xa1528405, 0xe1000005,
	0x81730405, 0xe1000005, 0xa1540405, 0xe1000005, 0x1950001f, 0x108c0204,
	0x808c1401, 0xd8212202, 0x17c07c1f, 0xa1538405, 0xe1000005, 0xa1508405,
	0xe1000005, 0x81700405, 0xe1000005, 0xa1520405, 0xe1000005, 0x81710405,
	0xe1000005, 0x81718405, 0xe1000005, 0x1880001f, 0x0000104d, 0xa0908402,
	0xe2c00002, 0x80b00402, 0xe2c00002, 0xa0920402, 0xe2c00002, 0x80b10402,
	0xe2c00002, 0x80b18402, 0xe2c00002, 0x1b80001f, 0x2000001a, 0xf0000000,
	0x17c07c1f, 0xe2e01045, 0x1910001f, 0x108e0e10, 0x1950001f, 0x108c0188,
	0x81001404, 0xd8212644, 0x17c07c1f, 0xf0000000, 0x17c07c1f, 0xe2e01045,
	0x1910001f, 0x108e0e14, 0x1950001f, 0x108c0188, 0x81001404, 0xd8212784,
	0x17c07c1f, 0xf0000000, 0x17c07c1f, 0x18b0000b, 0x17c07c1f, 0xa0910402,
	0xe2c00002, 0xa0918402, 0xe2c00002, 0x80b08402, 0xe2c00002, 0x1900001f,
	0x108c0204, 0x1950001f, 0x108c0204, 0xa1510405, 0xe1000005, 0xa1518405,
	0xe1000005, 0x81708405, 0xe1000005, 0x1950001f, 0x108c0188, 0x81081401,
	0x81079404, 0x1950001f, 0x108c018c, 0x81081404, 0x81079404, 0xd8212ae4,
	0x17c07c1f, 0x1900001f, 0x108c0204, 0x1950001f, 0x108c0204, 0x81740405,
	0xe1000005, 0x1950001f, 0x108c0204, 0x814c1401, 0xd8012ce5, 0x17c07c1f,
	0x1b80001f, 0x2000001a, 0x1950001f, 0x108c0204, 0xa1530405, 0xe1000005,
	0x1b80001f, 0x20000003, 0x81728405, 0xe1000005, 0x80b20402, 0xe2c00002,
	0xa0900402, 0xe2c00002, 0x81720405, 0xe1000005, 0xa1500405, 0xe1000005,
	0x81738405, 0xe1000005, 0xf0000000, 0x17c07c1f, 0xf0000000, 0x17c07c1f
};
static struct pcm_desc dpidle_pcm = {
	.version	= "pcm_suspend_v42.0_r12_0xffffffff",
	.base		= dpidle_binary,
	.size		= 2436,
	.sess		= 2,
	.replace	= 0,
	.addr_2nd	= 0,
	.vec0		= EVENT_VEC(32, 1, 0, 0),	/* FUNC_26M_WAKEUP */
	.vec1		= EVENT_VEC(33, 1, 0, 41),	/* FUNC_26M_SLEEP */
	.vec4		= EVENT_VEC(34, 1, 0, 95),	/* FUNC_INFRA_WAKEUP */
	.vec5		= EVENT_VEC(35, 1, 0, 130),	/* FUNC_INFRA_SLEEP */
	.vec6		= EVENT_VEC(46, 1, 0, 420),	/* FUNC_NFC_CLK_BUF_ON */
	.vec7		= EVENT_VEC(47, 1, 0, 450),	/* FUNC_NFC_CLK_BUF_OFF */
	.vec8		= EVENT_VEC(36, 1, 0, 155),	/* FUNC_APSRC_WAKEUP */
	.vec9		= EVENT_VEC(37, 1, 0, 194),	/* FUNC_APSRC_SLEEP */
	.vec12		= EVENT_VEC(38, 1, 0, 266),	/* FUNC_VRF18_WAKEUP */
	.vec13		= EVENT_VEC(39, 1, 0, 327),	/* FUNC_VRF18_SLEEP */
	.vec14		= EVENT_VEC(47, 1, 0, 371),	/* FUNC_DDREN_WAKEUP */
	.vec15		= EVENT_VEC(48, 1, 0, 392),	/* FUNC_DDREN_SLEEP */
};

static struct pwr_ctrl dpidle_ctrl = {
	.wake_src			= WAKE_SRC_FOR_DPIDLE,
	.wake_src_md32		= WAKE_SRC_FOR_MD32,

#if SPM_BYPASS_SYSPWREQ
	.syspwreq_mask = 1,
#endif

	/* Auto-gen Start */

	/* SPM_CLK_CON */
	.reg_srcclken0_ctl = 0,
	.reg_srcclken1_ctl = 0,
	.reg_spm_lock_infra_dcm = 0,
	.reg_srcclken_mask = 0,
	.reg_md1_c32rm_en = 0,
	.reg_md2_c32rm_en = 0,
	.reg_clksq0_sel_ctrl = 0,
	.reg_clksq1_sel_ctrl = 0,
	.reg_srcclken0_en = 0,
	.reg_srcclken1_en = 0,
	.reg_sysclk0_src_mask_b = 0,
	.reg_sysclk1_src_mask_b = 0,

	/* SPM_AP_STANDBY_CON */
	.reg_mpwfi_op = WFI_OP_AND,
	.reg_mp0_cputop_idle_mask = 1,
	.reg_mp1_cputop_idle_mask = 1,
	.reg_debugtop_idle_mask = 1,
	.reg_mp_top_idle_mask = 1,
	.reg_mcusys_idle_mask = 1,
	.reg_md_ddr_en_0_dbc_en = 0,
	.reg_md_ddr_en_1_dbc_en = 0,
	.reg_conn_ddr_en_dbc_en = 0,
	.reg_md32_mask_b = 0,
	.reg_md_0_mask_b = 0,
	.reg_md_1_mask_b = 0,
	.reg_scp_mask_b = 0,
	.reg_srcclkeni0_mask_b = 0,
	.reg_srcclkeni1_mask_b = 0,
	.reg_md_apsrc_1_sel = 0,
	.reg_md_apsrc_0_sel = 0,
	.reg_conn_mask_b = 0,
	.reg_conn_apsrc_sel = 0,

	/* SPM_SRC_REQ */
	.reg_spm_apsrc_req = 0,
	.reg_spm_f26m_req = 0,
	.reg_spm_infra_req = 0,
	.reg_spm_ddren_req = 0,
	.reg_spm_vrf18_req = 0,
	.reg_spm_dvfs_level0_req = 0,
	.reg_spm_dvfs_level1_req = 0,
	.reg_spm_dvfs_level2_req = 0,
	.reg_spm_dvfs_level3_req = 0,
	.reg_spm_dvfs_level4_req = 0,
	.reg_spm_pmcu_mailbox_req = 0,
	.reg_spm_sw_mailbox_req = 0,
	.reg_spm_cksel2_req = 0,
	.reg_spm_cksel3_req = 0,

	/* SPM_SRC_MASK */
	.reg_csyspwreq_mask = 0,
	.reg_md_srcclkena_0_infra_mask_b = 0,
	.reg_md_srcclkena_1_infra_mask_b = 0,
	.reg_md_apsrc_req_0_infra_mask_b = 0,
	.reg_md_apsrc_req_1_infra_mask_b = 0,
	.reg_conn_srcclkena_infra_mask_b = 0,
	.reg_conn_infra_req_mask_b = 0,
	.reg_md32_srcclkena_infra_mask_b = 0,
	.reg_md32_infra_req_mask_b = 0,
	.reg_scp_srcclkena_infra_mask_b = 0,
	.reg_scp_infra_req_mask_b = 0,
	.reg_srcclkeni0_infra_mask_b = 0,
	.reg_srcclkeni1_infra_mask_b = 0,
	.reg_ccif0_md_event_mask_b = 0,
	.reg_ccif0_ap_event_mask_b = 0,
	.reg_ccif1_md_event_mask_b = 0,
	.reg_ccif1_ap_event_mask_b = 0,
	.reg_ccif2_md_event_mask_b = 0,
	.reg_ccif2_ap_event_mask_b = 0,
	.reg_ccif3_md_event_mask_b = 0,
	.reg_ccif3_ap_event_mask_b = 0,
	.reg_ccifmd_md1_event_mask_b = 0,
	.reg_ccifmd_md2_event_mask_b = 0,
	.reg_c2k_ps_rccif_wake_mask_b = 0,
	.reg_c2k_l1_rccif_wake_mask_b = 0,
	.reg_ps_c2k_rccif_wake_mask_b = 0,
	.reg_l1_c2k_rccif_wake_mask_b = 0,
	.reg_dqssoc_req_mask_b = 0,
	.reg_disp2_req_mask_b = 0,
	.reg_md_ddr_en_0_mask_b = 0,
	.reg_md_ddr_en_1_mask_b = 0,
	.reg_conn_ddr_en_mask_b = 0,

	/* SPM_SRC2_MASK */
	.reg_disp0_req_mask_b = 0,
	.reg_disp1_req_mask_b = 0,
	.reg_disp_od_req_mask_b = 0,
	.reg_mfg_req_mask_b = 0,
	.reg_vdec0_req_mask_b = 0,
	.reg_gce_vrf18_req_mask_b = 0,
	.reg_gce_req_mask_b = 0,
	.reg_lpdma_req_mask_b = 0,
	.reg_srcclkeni1_cksel2_mask_b = 0,
	.reg_conn_srcclkena_cksel2_mask_b = 0,
	.reg_srcclkeni0_cksel3_mask_b = 0,
	.reg_md32_apsrc_req_ddren_mask_b = 0,
	.reg_scp_apsrc_req_ddren_mask_b = 0,
	.reg_md_vrf18_req_0_mask_b = 0,
	.reg_md_vrf18_req_1_mask_b = 0,
	.reg_next_dvfs_level0_mask_b = 0,
	.reg_next_dvfs_level1_mask_b = 0,
	.reg_next_dvfs_level2_mask_b = 0,
	.reg_next_dvfs_level3_mask_b = 0,
	.reg_next_dvfs_level4_mask_b = 0,
	.reg_msdc1_dvfs_halt_mask = 0,
	.reg_msdc2_dvfs_halt_mask = 0,
	.reg_msdc3_dvfs_halt_mask = 0,
	.reg_sw2spm_int0_mask_b = 0,
	.reg_sw2spm_int1_mask_b = 0,
	.reg_sw2spm_int2_mask_b = 0,
	.reg_sw2spm_int3_mask_b = 0,
	.reg_pmcu2spm_int0_mask_b = 0,
	.reg_pmcu2spm_int1_mask_b = 0,
	.reg_pmcu2spm_int2_mask_b = 0,
	.reg_pmcu2spm_int3_mask_b = 0,

	/* SPM_WAKEUP_EVENT_MASK */
	.reg_wakeup_event_mask = 0,

	/* SPM_EXT_WAKEUP_EVENT_MASK */
	.reg_ext_wakeup_event_mask = 0,

	/* MP0_CPU0_WFI_EN */
	.mp0_cpu0_wfi_en = 0,

	/* MP0_CPU1_WFI_EN */
	.mp0_cpu1_wfi_en = 0,

	/* MP0_CPU2_WFI_EN */
	.mp0_cpu2_wfi_en = 0,

	/* MP0_CPU3_WFI_EN */
	.mp0_cpu3_wfi_en = 0,

	/* MP1_CPU0_WFI_EN */
	.mp1_cpu0_wfi_en = 0,

	/* MP1_CPU1_WFI_EN */
	.mp1_cpu1_wfi_en = 0,

	/* MP1_CPU2_WFI_EN */
	.mp1_cpu2_wfi_en = 0,

	/* MP1_CPU3_WFI_EN */
	.mp1_cpu3_wfi_en = 0,

	/* DEBUG0_WFI_EN */
	.debug0_wfi_en = 0,

	/* DEBUG1_WFI_EN */
	.debug1_wfi_en = 0,

	/* DEBUG2_WFI_EN */
	.debug2_wfi_en = 0,

	/* DEBUG3_WFI_EN */
	.debug3_wfi_en = 0,

	/* Auto-gen End */
};

struct spm_lp_scen __spm_dpidle = {
#if 0//#ifdef //
	.pcmdesc	= &dpidle_pcm,
#endif
	.pwrctrl	= &dpidle_ctrl,
};

static unsigned int dpidle_log_discard_cnt;
static unsigned int dpidle_log_print_prev_time;

#if defined(CONFIG_MTK_TINYSYS_SSPM_SUPPORT)
static void spm_trigger_wfi_for_dpidle(struct pwr_ctrl *pwrctrl)
{
	if (is_cpu_pdn(pwrctrl->pcm_flags)) {
		if (mt_cpu_dormant(CPU_DEEPIDLE_MODE) < 0)
			BUG();
	} else {
		wfi_with_sync();
	}
}

static void spm_dpidle_pcm_setup_before_wfi(u32 cpu, struct pcm_desc *pcmdesc,
		struct pwr_ctrl *pwrctrl, bool sleep_dpidle)
{
	int ret;
	struct spm_data spm_d;

	memset(&spm_d, 0, sizeof(struct spm_data));
	spm_d.u.suspend.cpu = cpu;
	spm_d.u.suspend.pcm_flags = pwrctrl->pcm_flags;
	spm_d.u.suspend.sleep_dpidle = sleep_dpidle;
	spm_d.u.suspend.spm_pcmwdt_en = SPM_PCMWDT_EN;
	ret = spm_to_sspm_command(SPM_DPIDLE_ENTER, &spm_d);
	if (ret < 0)
		BUG();
}

static void spm_dpidle_pcm_setup_after_wfi(bool sleep_dpidle)
{
	int ret;
	struct spm_data spm_d;

	memset(&spm_d, 0, sizeof(struct spm_data));
	spm_d.u.suspend.sleep_dpidle = sleep_dpidle;
	spm_d.u.suspend.spm_pcmwdt_en = SPM_PCMWDT_EN;
	ret = spm_to_sspm_command(SPM_DPIDLE_LEAVE, &spm_d);
	if (ret < 0)
		BUG();
}

#else

static void spm_trigger_wfi_for_dpidle(struct pwr_ctrl *pwrctrl)
{
	u32 v0, v1, v2;

	if (is_cpu_pdn(pwrctrl->pcm_flags)) {
		if (mt_cpu_dormant(CPU_DEEPIDLE_MODE) < 0)
			BUG();
	} else {
		/* backup MPx_AXI_CONFIG */
		v0 = reg_read(MP0_AXI_CONFIG);
		v1 = reg_read(MP1_AXI_CONFIG);
		v2 = reg_read(MP2_AXI_CONFIG);

		/* disable snoop function */
		spm_write(MP0_AXI_CONFIG, v0 | ACINACTM);
		spm_write(MP1_AXI_CONFIG, v1 | ACINACTM);
		spm_write(MP2_AXI_CONFIG, v2 | (ACINACTM + 1));

		wfi_with_sync();

		/* restore MPx_AXI_CONFIG */
		spm_write(MP0_AXI_CONFIG, v0);
		spm_write(MP1_AXI_CONFIG, v1);
		spm_write(MP2_AXI_CONFIG, v2);
	}
}

static void spm_dpidle_pcm_setup_before_wfi(u32 cpu, struct pcm_desc *pcmdesc,
		struct pwr_ctrl *pwrctrl, bool sleep_dpidle)
{
	__spm_set_cpu_status(cpu);

	__spm_reset_and_init_pcm(pcmdesc);

	__spm_kick_im_to_fetch(pcmdesc);

	__spm_init_pcm_register();

	__spm_init_event_vector(pcmdesc);

	/* TODO: modify it according to SPM scenario switch flow */
#if 0
	__spm_sync_vcore_dvfs_power_control(pwrctrl, __spm_vcore_dvfs.pwrctrl);
#endif

	__spm_set_power_control(pwrctrl);

	__spm_set_wakeup_event(pwrctrl);

#if SPM_PCMWDT_EN
	if (sleep_dpidle)
		__spm_set_pcm_wdt(1);
#endif

	spm_dpidle_pre_process();

	__spm_kick_pcm_to_run(pwrctrl);
}

static void spm_dpidle_pcm_setup_after_wfi(bool sleep_dpidle)
{
	spm_dpidle_post_process();

#if SPM_PCMWDT_EN
	if (sleep_dpidle)
		__spm_set_pcm_wdt(0);
#endif

	__spm_clean_after_wakeup();
}

#endif /* CONFIG_MTK_TINYSYS_SSPM_SUPPORT */


/*
 * wakesrc: WAKE_SRC_XXX
 * enable : enable or disable @wakesrc
 * replace: if true, will replace the default setting
 */
int spm_set_dpidle_wakesrc(u32 wakesrc, bool enable, bool replace)
{
	unsigned long flags;

	if (spm_is_wakesrc_invalid(wakesrc))
		return -EINVAL;

	spin_lock_irqsave(&__spm_lock, flags);
	if (enable) {
		if (replace)
			__spm_dpidle.pwrctrl->wake_src = wakesrc;
		else
			__spm_dpidle.pwrctrl->wake_src |= wakesrc;
	} else {
		if (replace)
			__spm_dpidle.pwrctrl->wake_src = 0;
		else
			__spm_dpidle.pwrctrl->wake_src &= ~wakesrc;
	}
	spin_unlock_irqrestore(&__spm_lock, flags);

	return 0;
}

static wake_reason_t spm_output_wake_reason(struct wake_status *wakesta, struct pcm_desc *pcmdesc, u32 dump_log)
{
	wake_reason_t wr = WR_NONE;
	unsigned long int dpidle_log_print_curr_time = 0;
	bool log_print = false;
	static bool timer_out_too_short;

	if (dump_log == DEEPIDLE_LOG_FULL) {
		wr = __spm_output_wake_reason(wakesta, pcmdesc, false);
	} else if (dump_log == DEEPIDLE_LOG_REDUCED) {
		/* Determine print SPM log or not */
		dpidle_log_print_curr_time = spm_get_current_time_ms();

		if (wakesta->assert_pc != 0)
			log_print = true;
#if 0
		/* Not wakeup by GPT */
		else if ((wakesta->r12 & (0x1 << 4)) == 0)
			log_print = true;
		else if (wakesta->timer_out <= DPIDLE_LOG_PRINT_TIMEOUT_CRITERIA)
			log_print = true;
#endif
		else if ((dpidle_log_print_curr_time - dpidle_log_print_prev_time) > DPIDLE_LOG_DISCARD_CRITERIA)
			log_print = true;

		if (wakesta->timer_out <= DPIDLE_LOG_PRINT_TIMEOUT_CRITERIA)
			timer_out_too_short = true;

		/* Print SPM log */
		if (log_print == true) {
			dpidle_dbg("dpidle_log_discard_cnt = %d, timer_out_too_short = %d\n",
						dpidle_log_discard_cnt,
						timer_out_too_short);
			wr = __spm_output_wake_reason(wakesta, pcmdesc, false);

			dpidle_log_print_prev_time = dpidle_log_print_curr_time;
			dpidle_log_discard_cnt = 0;
			timer_out_too_short = false;
		} else {
			dpidle_log_discard_cnt++;

			wr = WR_NONE;
		}
	}

#ifdef CONFIG_MTK_ECCCI_DRIVER
	if (wakesta->r12 & WAKE_SRC_R12_CLDMA_EVENT_B)
		exec_ccci_kern_func_by_md_id(0, ID_GET_MD_WAKEUP_SRC, NULL, 0);
#endif

	return wr;
}

wake_reason_t spm_go_to_dpidle(u32 spm_flags, u32 spm_data, u32 dump_log)
{
	struct wake_status wakesta;
	unsigned long flags;
#if defined(CONFIG_MTK_GIC_V3_EXT)
	struct mtk_irq_mask mask;
#endif
	wake_reason_t wr = WR_NONE;
	/* struct pcm_desc *pcmdesc = __spm_dpidle.pcmdesc; */
	struct pcm_desc *pcmdesc;
	struct pwr_ctrl *pwrctrl = __spm_dpidle.pwrctrl;
	u32 cpu = spm_data;

	spm_dpidle_footprint(SPM_DEEPIDLE_ENTER);

#if 0
	if (dyna_load_pcm[DYNA_LOAD_PCM_DEEPIDLE].ready)
		pcmdesc = &(dyna_load_pcm[DYNA_LOAD_PCM_DEEPIDLE].desc);
	else
		BUG();
#endif
	pcmdesc = __spm_dpidle.pcmdesc;
	pwrctrl = __spm_dpidle.pwrctrl;

	update_pwrctrl_pcm_flags(&spm_flags);
	set_pwrctrl_pcm_flags(pwrctrl, spm_flags);

	lockdep_off();
	spin_lock_irqsave(&__spm_lock, flags);

#if defined(CONFIG_MTK_GIC_V3_EXT)
	mt_irq_mask_all(&mask);
	mt_irq_unmask_for_sleep(SPM_IRQ0_ID);
	mt_irq_unmask_for_sleep(223); /* for kp only */
#endif

#if 0 /* defined(CONFIG_MTK_SYS_CIRQ) */
	mt_cirq_clone_gic();
	mt_cirq_enable();
#endif

	spm_dpidle_footprint(SPM_DEEPIDLE_ENTER_UART_SLEEP);

#if 1//#ifndef //
	if (request_uart_to_sleep()) {
		wr = WR_UART_BUSY;
		goto RESTORE_IRQ;
	}
#endif

	spm_dpidle_pcm_setup_before_wfi(cpu, pcmdesc, pwrctrl, false);

	spm_dpidle_footprint(SPM_DEEPIDLE_ENTER_WFI);

#ifdef SPM_DEEPIDLE_PROFILE_TIME
	gpt_get_cnt(SPM_PROFILE_APXGPT, &dpidle_profile[1]);
#endif

	spm_trigger_wfi_for_dpidle(pwrctrl);

#ifdef SPM_DEEPIDLE_PROFILE_TIME
	gpt_get_cnt(SPM_PROFILE_APXGPT, &dpidle_profile[2]);
#endif

	spm_dpidle_footprint(SPM_DEEPIDLE_LEAVE_WFI);

	__spm_get_wakeup_status(&wakesta);

	spm_dpidle_pcm_setup_after_wfi(false);

	spm_dpidle_footprint(SPM_DEEPIDLE_ENTER_UART_AWAKE);

#if 1//#ifndef //
	request_uart_to_wakeup();
#endif

	wr = spm_output_wake_reason(&wakesta, pcmdesc, dump_log);

#if 1//#ifndef //
RESTORE_IRQ:
#if 0 /* defined(CONFIG_MTK_SYS_CIRQ) */
	mt_cirq_flush();
	mt_cirq_disable();
#endif
#endif

#if defined(CONFIG_MTK_GIC_V3_EXT)
	mt_irq_mask_restore(&mask);
#endif

	spin_unlock_irqrestore(&__spm_lock, flags);
	lockdep_on();

	spm_dpidle_footprint(0);

	return wr;
}

/*
 * cpu_pdn:
 *    true  = CPU dormant
 *    false = CPU standby
 * pwrlevel:
 *    0 = AXI is off
 *    1 = AXI is 26M
 * pwake_time:
 *    >= 0  = specific wakeup period
 */
wake_reason_t spm_go_to_sleep_dpidle(u32 spm_flags, u32 spm_data)
{
	u32 sec = 0;
	u32 dpidle_timer_val = 0;
	u32 dpidle_wake_src = 0;
	struct wake_status wakesta;
	unsigned long flags;
#if defined(CONFIG_MTK_GIC_V3_EXT)
	struct mtk_irq_mask mask;
#endif
#if 1//#ifndef //
#ifdef CONFIG_MTK_WD_KICKER
	struct wd_api *wd_api;
	int wd_ret;
#endif
#endif
	static wake_reason_t last_wr = WR_NONE;
	/* struct pcm_desc *pcmdesc = __spm_dpidle.pcmdesc; */
	struct pcm_desc *pcmdesc;
	struct pwr_ctrl *pwrctrl = __spm_dpidle.pwrctrl;
	int cpu = smp_processor_id();

	spm_dpidle_footprint(SPM_DEEPIDLE_SLEEP_DPIDLE | SPM_DEEPIDLE_ENTER);

#if 1//#ifndef //
#ifndef CONFIG_MTK_TINYSYS_SSPM_SUPPORT
	if (dyna_load_pcm[DYNA_LOAD_PCM_DEEPIDLE].ready)
		pcmdesc = &(dyna_load_pcm[DYNA_LOAD_PCM_DEEPIDLE].desc);
	else
		BUG();
#endif /* CONFIG_MTK_TINYSYS_SSPM_SUPPORT */
#else
	pcmdesc = __spm_dpidle.pcmdesc;
	pwrctrl = __spm_dpidle.pwrctrl;
#endif

	/* backup original dpidle setting */
	dpidle_timer_val = pwrctrl->timer_val;
	dpidle_wake_src = pwrctrl->wake_src;

	update_pwrctrl_pcm_flags(&spm_flags);
	set_pwrctrl_pcm_flags(pwrctrl, spm_flags);

#if SPM_PWAKE_EN
	sec = _spm_get_wake_period(-1, last_wr);
#endif
	pwrctrl->timer_val = sec * 32768;

	pwrctrl->wake_src = spm_get_sleep_wakesrc();
	pwrctrl->wake_src |= WAKE_SRC_R12_SYS_CIRQ_IRQ_B;

#ifdef CONFIG_MTK_WD_KICKER
	wd_ret = get_wd_api(&wd_api);
	if (!wd_ret)
		wd_api->wd_suspend_notify();
#endif

	lockdep_off();
	spin_lock_irqsave(&__spm_lock, flags);

#if defined(CONFIG_MTK_GIC_V3_EXT)
	mt_irq_mask_all(&mask);
	mt_irq_unmask_for_sleep(SPM_IRQ0_ID);
	mt_irq_unmask_for_sleep(223); /* for kp only */
#endif

#if 0 /* defined(CONFIG_MTK_SYS_CIRQ) */
	mt_cirq_clone_gic();
	mt_cirq_enable();
#endif

	spm_dpidle_footprint(SPM_DEEPIDLE_SLEEP_DPIDLE | SPM_DEEPIDLE_ENTER_UART_SLEEP);

	spm_crit2("sleep_deepidle, sec = %u, wakesrc = 0x%x [%u][%u]\n",
		sec, pwrctrl->wake_src,
		is_cpu_pdn(pwrctrl->pcm_flags),
		is_infra_pdn(pwrctrl->pcm_flags));

#if 1//#ifndef //
	if (request_uart_to_sleep()) {
		last_wr = WR_UART_BUSY;
		goto RESTORE_IRQ;
	}
#endif

	spm_dpidle_pcm_setup_before_wfi(cpu, pcmdesc, pwrctrl, true);

	spm_dpidle_footprint(SPM_DEEPIDLE_SLEEP_DPIDLE | SPM_DEEPIDLE_ENTER_WFI);

	spm_trigger_wfi_for_dpidle(pwrctrl);

	spm_dpidle_footprint(SPM_DEEPIDLE_SLEEP_DPIDLE | SPM_DEEPIDLE_LEAVE_WFI);

	__spm_get_wakeup_status(&wakesta);

	spm_dpidle_pcm_setup_after_wfi(true);

	spm_dpidle_footprint(SPM_DEEPIDLE_SLEEP_DPIDLE | SPM_DEEPIDLE_ENTER_UART_AWAKE);

#if 1//#ifndef //
	request_uart_to_wakeup();
#endif

	last_wr = __spm_output_wake_reason(&wakesta, pcmdesc, true);

#if 1//#ifndef //
RESTORE_IRQ:
#endif
#if 0 /* defined(CONFIG_MTK_SYS_CIRQ) */
	mt_cirq_flush();
	mt_cirq_disable();
#endif

#if defined(CONFIG_MTK_GIC_V3_EXT)
	mt_irq_mask_restore(&mask);
#endif

	spin_unlock_irqrestore(&__spm_lock, flags);
	lockdep_on();

#ifdef CONFIG_MTK_WD_KICKER
	if (!wd_ret)
		wd_api->wd_resume_notify();
#endif

	/* restore original dpidle setting */
	pwrctrl->timer_val = dpidle_timer_val;
	pwrctrl->wake_src = dpidle_wake_src;

	spm_dpidle_footprint(0);

	return last_wr;
}

void spm_deepidle_init(void)
{
	spm_deepidle_chip_init();
}

MODULE_DESCRIPTION("SPM-DPIdle Driver v0.1");
