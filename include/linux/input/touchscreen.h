/*
 * Copyright (c) 2014 Sebastian Reichel <sre@kernel.org>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 */

#ifndef _TOUCHSCREEN_H
#define _TOUCHSCREEN_H

#include <linux/input.h>

#if 1//#ifdef CONFIG_OF//CONFIG_OF=y
void touchscreen_parse_of_params(struct input_dev *dev);
#else
static inline void touchscreen_parse_of_params(struct input_dev *dev)
{
}
#endif

#endif
