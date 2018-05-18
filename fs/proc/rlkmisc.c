/*Transsion Top Secret*/
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <asm/uaccess.h>

/* Must define in lcm drvier,
 * indicate lcm inversion for flicker test at *#87#
 */
extern unsigned int g_lcm_inversion;

static int lcm_inv_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%d\n", g_lcm_inversion);
	return 0;
}

static int lcm_inv_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, lcm_inv_proc_show, NULL);
}

static ssize_t lcm_inv_proc_write(struct file *filp, const char __user *ubuf, size_t cnt, loff_t *data)
{
    char buf[64];
    int val;
    int ret;
    if (cnt >= sizeof(buf))
        return -EINVAL;

    if (copy_from_user(&buf, ubuf, cnt))
        return -EFAULT;

    buf[cnt] = 0;

    ret = kstrtoul(buf, 10, (unsigned long*)&val);
    if (ret < 0)
        return ret;

	g_lcm_inversion = (unsigned int)val;
    pr_err("write lcm invertion to %d\n", g_lcm_inversion);
    return cnt;
}

static const struct file_operations lcm_inv_proc_fops = {
	.open		= lcm_inv_proc_open,
	.read		= seq_read,
	.write      = lcm_inv_proc_write,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static int __init proc_rlk_init(void)
{
	printk("create /proc/lcm_inversion");
	if (NULL == proc_create("lcm_inversion", 0, NULL, &lcm_inv_proc_fops))
		pr_err(" Fail\n");
	else
		pr_err(" OK\n");

	return 0;
}

module_init(proc_rlk_init);
