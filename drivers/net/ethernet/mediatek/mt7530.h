/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Copyright (C) 2013 John Crispin <blogic@openwrt.org>
 */

#ifndef _MT7530_H__
#define _MT7530_H__

#ifdef CONFIG_SUPPORT_OPENWRT
#include <linux/switch.h>
#else
#include "switch.h"
#endif
int mt7530_probe(struct device *dev, void __iomem *base, struct mii_bus *bus,
		 int vlan);

#endif
