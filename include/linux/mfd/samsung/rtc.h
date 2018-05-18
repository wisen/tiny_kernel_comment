/* rtc.h
 *
 * Copyright (c) 2011-2014 Samsung Electronics Co., Ltd
 *              http://www.samsung.com
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __LINUX_MFD_SEC_RTC_H
#define __LINUX_MFD_SEC_RTC_H

enum s5m_rtc_reg {
	S5M_RTC_SEC,
	S5M_RTC_MIN,
	S5M_RTC_HOUR,
	S5M_RTC_WEEKDAY,
	S5M_RTC_DATE,
	S5M_RTC_MONTH,
	S5M_RTC_YEAR1,
	S5M_RTC_YEAR2,
	S5M_ALARM0_SEC,
	S5M_ALARM0_MIN,
	S5M_ALARM0_HOUR,
	S5M_ALARM0_WEEKDAY,
	S5M_ALARM0_DATE,
	S5M_ALARM0_MONTH,
	S5M_ALARM0_YEAR1,
	S5M_ALARM0_YEAR2,
	S5M_ALARM1_SEC,
	S5M_ALARM1_MIN,
	S5M_ALARM1_HOUR,
	S5M_ALARM1_WEEKDAY,
	S5M_ALARM1_DATE,
	S5M_ALARM1_MONTH,
	S5M_ALARM1_YEAR1,
	S5M_ALARM1_YEAR2,
	S5M_ALARM0_CONF,
	S5M_ALARM1_CONF,
	S5M_RTC_STATUS,
	S5M_WTSR_SMPL_CNTL,
	S5M_RTC_UDR_CON,

	S5M_RTC_REG_MAX,
};

enum s2mps_rtc_reg {
	S2MPS_RTC_CTRL,
	S2MPS_WTSR_SMPL_CNTL,
	S2MPS_RTC_UDR_CON,
	S2MPS_RSVD,
	S2MPS_RTC_SEC,
	S2MPS_RTC_MIN,
	S2MPS_RTC_HOUR,
	S2MPS_RTC_WEEKDAY,
	S2MPS_RTC_DATE,
	S2MPS_RTC_MONTH,
	S2MPS_RTC_YEAR,
	S2MPS_ALARM0_SEC,
	S2MPS_ALARM0_MIN,
	S2MPS_ALARM0_HOUR,
	S2MPS_ALARM0_WEEKDAY,
	S2MPS_ALARM0_DATE,
	S2MPS_ALARM0_MONTH,
	S2MPS_ALARM0_YEAR,
	S2MPS_ALARM1_SEC,
	S2MPS_ALARM1_MIN,
	S2MPS_ALARM1_HOUR,
	S2MPS_ALARM1_WEEKDAY,
	S2MPS_ALARM1_DATE,
	S2MPS_ALARM1_MONTH,
	S2MPS_ALARM1_YEAR,
	S2MPS_OFFSRC,

	S2MPS_RTC_REG_MAX,
};

#define RTC_I2C_ADDR		(0x0C >> 1)

#define HOUR_12			(1 << 7)
#define HOUR_AMPM		(1 << 6)
#define HOUR_PM			(1 << 5)
#define S5M_ALARM0_STATUS	(1 << 1)
#define S5M_ALARM1_STATUS	(1 << 2)
#define S5M_UPDATE_AD		(1 << 0)

#define S2MPS_ALARM0_STATUS	(1 << 2)
#define S2MPS_ALARM1_STATUS	(1 << 1)

/* RTC Control Register */
#define BCD_EN_SHIFT		0
#define BCD_EN_MASK		(1 << BCD_EN_SHIFT)
#define MODEL24_SHIFT		1
#define MODEL24_MASK		(1 << MODEL24_SHIFT)
/* RTC Update Register1 */
#define S5M_RTC_UDR_SHIFT	0
#define S5M_RTC_UDR_MASK	(1 << S5M_RTC_UDR_SHIFT)
#define S2MPS_RTC_WUDR_SHIFT	4
#define S2MPS_RTC_WUDR_MASK	(1 << S2MPS_RTC_WUDR_SHIFT)
#define S2MPS_RTC_RUDR_SHIFT	0
#define S2MPS_RTC_RUDR_MASK	(1 << S2MPS_RTC_RUDR_SHIFT)
#define RTC_TCON_SHIFT		1
#define RTC_TCON_MASK		(1 << RTC_TCON_SHIFT)
#define S5M_RTC_TIME_EN_SHIFT	3
#define S5M_RTC_TIME_EN_MASK	(1 << S5M_RTC_TIME_EN_SHIFT)
/*
 * UDR_T field in S5M_RTC_UDR_CON register determines the time needed
 * for updating alarm and time registers. Default is 7.32 ms.
 */
#define S5M_RTC_UDR_T_SHIFT	6
#define S5M_RTC_UDR_T_MASK	(0x3 << S5M_RTC_UDR_T_SHIFT)
#define S5M_RTC_UDR_T_7320_US	(0x0 << S5M_RTC_UDR_T_SHIFT)
#define S5M_RTC_UDR_T_1830_US	(0x1 << S5M_RTC_UDR_T_SHIFT)
#define S5M_RTC_UDR_T_3660_US	(0x2 << S5M_RTC_UDR_T_SHIFT)
#define S5M_RTC_UDR_T_450_US	(0x3 << S5M_RTC_UDR_T_SHIFT)

/* RTC Hour register */
#define HOUR_PM_SHIFT		6
#define HOUR_PM_MASK		(1 << HOUR_PM_SHIFT)
/* RTC Alarm Enable */
#define ALARM_ENABLE_SHIFT	7
#define ALARM_ENABLE_MASK	(1 << ALARM_ENABLE_SHIFT)

#define SMPL_ENABLE_SHIFT	7
#define SMPL_ENABLE_MASK	(1 << SMPL_ENABLE_SHIFT)

#define WTSR_ENABLE_SHIFT	6
#define WTSR_ENABLE_MASK	(1 << WTSR_ENABLE_SHIFT)

enum {
	RTC_SEC = 0,
	RTC_MIN,
	RTC_HOUR,
	RTC_WEEKDAY,
	RTC_DATE,
	RTC_MONTH,
	RTC_YEAR1,
	RTC_YEAR2,
};

#endif /*  __LINUX_MFD_SEC_RTC_H */
