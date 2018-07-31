/*
 * Copyright (C) 2015 MediaTek Inc.
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

#ifndef __MT_SLEEP_H__
#define __MT_SLEEP_H__

#if 0/*defined()*/ || 0/*defined()*/ || defined(CONFIG_ARCH_MT6797)

#include "spm_v2/mt_sleep.h"

#elif 0/*defined()*/ || 0/*defined()*/ || 0/*defined()*/

#include "../mt6735/mt_sleep.h"

#elif 0/*defined()*/ || 1/*defined()*/

#include "spm_v1/mt_sleep.h"

#elif defined(CONFIG_ARCH_ELBRUS)

#include "spm_v3/mt_sleep.h"

#endif

#endif /* __MT_SLEEP_H__ */
