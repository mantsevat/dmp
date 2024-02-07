#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/bio.h>
#include <linux/device-mapper.h>
#include <linux/kobject.h>
#include <linux/string.h>

#include "dmp_stat.h"

static struct dmp_stat stat = {
	.r_count = 0,
	.w_count = 0,
	.r_avg_size = 0,
	.w_avg_size = 0,
	.t_avg_size = 0,
};

static void change_stat(struct bio *bio)
{
	ulong mult_avg;

	if (bio_op(bio) == REQ_OP_READ)	{
		mult_avg = stat.r_count * stat.r_avg_size;
		stat.r_count += 1;
		mult_avg += bio->bi_iter.bi_size;
		stat.r_avg_size = (mult_avg + (stat.r_count / 2)) / stat.r_count;
	} else {
		mult_avg = stat.w_count * stat.w_avg_size;
		stat.w_count += 1;
		mult_avg += bio->bi_iter.bi_size;
		stat.w_avg_size = (mult_avg + (stat.w_count / 2)) / stat.w_count;
	}
	stat.t_avg_size = (stat.r_avg_size + stat.w_avg_size + 1) / 2;

}
static int dmp_ctr(struct dm_target *ti, unsigned int argc, char **argv)
{
	struct dm_dev **h_dm_pointer;

	if (argc != 1) {
		printk(KERN_CRIT "Invalid args number\n");
		ti->error = "Invalid args number";
		return -EINVAL;
	}

	h_dm_pointer = kmalloc(sizeof(struct dm_dev *), GFP_KERNEL);

	if (h_dm_pointer == NULL) {
		printk(KERN_CRIT "No memory for dmp pointer\n");
		ti->error = "No memory for dmp pointer";
		return -ENOMEM;
	}

	if (dm_get_device(ti, argv[0], dm_table_get_mode(ti->table), h_dm_pointer))	{
		printk(KERN_CRIT "Block device hasn't been found\n");
		ti->error = "Block device hasn't been found";
		return -EINVAL;
	}

	ti->private = h_dm_pointer;

	return 0;
}

static int dmp_map(struct dm_target *ti, struct bio *bio)
{

	struct dm_dev **h_dm_pointer = ti->private;

	if ((bio_op(bio) == REQ_OP_WRITE) | (bio_op(bio) == REQ_OP_READ))
		change_stat(bio);

	bio_set_dev(bio, (*h_dm_pointer)->bdev);
	submit_bio(bio);

	return DM_MAPIO_SUBMITTED;
}

static void dmp_dtr(struct dm_target *ti)
{
	struct dm_dev **h_dm_pointer = ti->private;

	dm_put_device(ti, *h_dm_pointer);
	kfree(h_dm_pointer);
}

static struct target_type dmp = {
	.name = "dmp",
	.version = {1, 0, 0},
	.module = THIS_MODULE,
	.ctr = dmp_ctr,
	.dtr = dmp_dtr,
	.map = dmp_map
};

static ssize_t show_stat(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "read:\n reqs: %lu\n avg size: %lu\nwrite:\n reqs: %lu\n avg size: %lu\ntotal:\n reqs: %lu\n avg size: %lu\n",
	 stat.r_count, stat.r_avg_size, stat.w_count, stat.w_avg_size, stat.r_count + stat.w_count, stat.t_avg_size);
}

static ssize_t store_stat(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
	return count;
}

struct kobject module_dir;
struct kobject *stat_dir, *stat_file;
struct kobj_attribute dmp_attr = __ATTR(volume, 0664, show_stat, store_stat);

static int init_dmp_module(void)
{
	int result;
	result = dm_register_target(&dmp);

	if (result < 0)
		printk(KERN_CRIT "DMP register error\n");
	module_dir = (((struct module *)(THIS_MODULE))->mkobj).kobj;
	stat_dir = kobject_create_and_add("stat", &module_dir);

	if (stat_dir == NULL)
		printk(KERN_CRIT "Can't create dir for stat file\n");

	if (sysfs_create_file(stat_dir, &dmp_attr.attr) < 0)
		printk(KERN_CRIT "Can't create sysfs stat file\n");

	return 0;
}

static void exit_dmp_module(void)
{
	kobject_put(stat_file);
	kobject_put(stat_dir);
	dm_unregister_target(&dmp);
}

module_init(init_dmp_module);
module_exit(exit_dmp_module);
MODULE_LICENSE("GPL");
