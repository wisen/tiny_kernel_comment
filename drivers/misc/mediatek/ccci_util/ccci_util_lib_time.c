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

#include <linux/module.h>
#include <linux/device.h>
#include <linux/spinlock.h>
#include <mt-plat/mt_ccci_common.h>
#include <linux/wait.h>
static wait_queue_head_t time_update_notify_queue_head;
static spinlock_t wait_count_lock;
static volatile unsigned int wait_count;
static volatile unsigned int get_update;
static volatile unsigned int api_ready;

void ccci_timer_for_md_init(void)
{
	init_waitqueue_head(&time_update_notify_queue_head);
	spin_lock_init(&wait_count_lock);
	wait_count = 0;
	get_update = 0;
	mb();
	api_ready = 1;
}

int wait_time_update_notify(void)
{				/* Only support one wait currently */
	int ret = -1;
	unsigned long flags;

	if (api_ready) {
		/* Update wait count ++ */
		spin_lock_irqsave(&wait_count_lock, flags);
		wait_count++;
		spin_unlock_irqrestore(&wait_count_lock, flags);

		ret = wait_event_interruptible(time_update_notify_queue_head, get_update);
		if (ret != -ERESTARTSYS)
			get_update = 0;

		/* Update wait count -- */
		spin_lock_irqsave(&wait_count_lock, flags);
		wait_count--;
		spin_unlock_irqrestore(&wait_count_lock, flags);
	}

	return ret;
}

void notify_time_update(void)
{
	unsigned long flags;

	if (!api_ready)
		return;		/* API not ready */
	get_update = 1;
	spin_lock_irqsave(&wait_count_lock, flags);
	if (wait_count)
		wake_up_all(&time_update_notify_queue_head);
	spin_unlock_irqrestore(&wait_count_lock, flags);
}
