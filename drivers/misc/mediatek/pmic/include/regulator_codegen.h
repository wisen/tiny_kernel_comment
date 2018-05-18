/*
 * Copyright (C) 2016 MediaTek Inc.

 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#ifndef _REGULATOR_CODEGEN_H_
#define _REGULATOR_CODEGEN_H_

#if 1//#ifdef CONFIG_OF//CONFIG_OF=y
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/regulator/of_regulator.h>
#include <linux/of_device.h>
#include <linux/of_fdt.h>
#endif
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/regulator/consumer.h>

#include <mach/upmu_hw.h>
#include <mach/upmu_sw.h>

#ifdef CONFIG_MTK_PMIC_CHIP_MT6335
#include "mt6335/mt_regulator_codegen.h"
#endif

#ifdef CONFIG_MTK_PMIC_CHIP_MT6353
#include "mt6353/mt_regulator_codegen.h"
#endif

/*****************************************************************************
 * PMIC extern function
 ******************************************************************************/
extern int mtk_regulator_enable(struct regulator_dev *rdev);
extern int mtk_regulator_disable(struct regulator_dev *rdev);
extern int mtk_regulator_is_enabled(struct regulator_dev *rdev);
extern int mtk_regulator_set_voltage_sel(struct regulator_dev *rdev, unsigned selector);
extern int mtk_regulator_get_voltage_sel(struct regulator_dev *rdev);
extern int mtk_regulator_list_voltage(struct regulator_dev *rdev, unsigned selector);

#endif				/* _REGULATOR_CODEGEN_H_ */
