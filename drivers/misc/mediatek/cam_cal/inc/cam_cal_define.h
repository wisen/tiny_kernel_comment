/*Transsion Top Secret*/
/*
* Copyright (C) 2016 MediaTek Inc.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* See http://www.gnu.org/licenses/gpl-2.0.html for more details.
*/
#ifndef _CAM_CAL_DATA_H
#define _CAM_CAL_DATA_H

#if 0//#ifdef CONFIG_COMPAT//CONFIG_COMPAT is not set
/* 64 bit */
#include <linux/fs.h>
#include <linux/compat.h>
#endif

typedef struct{
	u32 u4Offset;
	u32 u4Length;
	//u32 sensorID;//yang.li3 add  do not match with hal
	//u32 deviceID;//yang.li3 add  do not match with hal
	u8 *pu1Params;
} stCAM_CAL_INFO_STRUCT, *stPCAM_CAL_INFO_STRUCT;

#if 0//#ifdef CONFIG_COMPAT//CONFIG_COMPAT is not set

typedef struct{
	u32 u4Offset;
	u32 u4Length;
	//u32 sensorID; //yang.li3 add  do not match with hal
	//u32 deviceID; //yang.li3 add  do not match with hal
	compat_uptr_t pu1Params;
} COMPAT_stCAM_CAL_INFO_STRUCT;
#endif

#endif/*_CAM_CAL_DATA_H*/
