/******************************************************************************
 *
 * Copyright(c) 2009-2014  Realtek Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * The full GNU General Public License is included in this distribution in the
 * file called LICENSE.
 *
 * Contact Information:
 * wlanfae <wlanfae@realtek.com>
 * Realtek Corporation, No. 2, Innovation Road II, Hsinchu Science Park,
 * Hsinchu 300, Taiwan.
 *
 * Created on  2010/ 5/18,  1:41
 *
 * Larry Finger <Larry.Finger@lwfinger.net>
 *
 *****************************************************************************/

#ifndef __RTL92E_TABLE__H_
#define __RTL92E_TABLE__H_

#include <linux/types.h>
#define RTL8192EE_PHY_REG_ARRAY_LEN	448
extern u32 RTL8192EE_PHY_REG_ARRAY[];
#define RTL8192EE_PHY_REG_ARRAY_PG_LEN	168
extern u32 RTL8192EE_PHY_REG_ARRAY_PG[];
#define	RTL8192EE_RADIOA_ARRAY_LEN	238
extern u32 RTL8192EE_RADIOA_ARRAY[];
#define	RTL8192EE_RADIOB_ARRAY_LEN	198
extern u32 RTL8192EE_RADIOB_ARRAY[];
#define RTL8192EE_MAC_ARRAY_LEN		202
extern u32 RTL8192EE_MAC_ARRAY[];
#define RTL8192EE_AGC_TAB_ARRAY_LEN	532
extern u32 RTL8192EE_AGC_TAB_ARRAY[];

#endif
