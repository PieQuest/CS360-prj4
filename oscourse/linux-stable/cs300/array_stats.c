#include "array_stats.h"
#include <linux/kernel.h>
#include <linux/uaccess.h>


asmlinkage long sys_array_stats(struct array_stats *stats, long data[], long size) {
	struct array_stats copied = {10000000, 0, 0};
	long dataCopied = 0;
	long result = 0;
	int count = 0;
	
	while (count < size) {
		//cpy src -> buf
		if (copy_from_user(&dataCopied, &data[count], sizeof(data[count])))
		{
			return -EFAULT;
		}
		
		//cpy buf -> dst		
		if (copied.min > dataCopied)
		{
			copied.min = dataCopied;
		}
		
		printk("--min %ld\n", copied.min);
		if (copied.max < dataCopied)
		{
			copied.max = dataCopied;
		}
		
		printk("--max %ld\n", copied.max);
		
		copied.sum = (copied.sum + dataCopied);
		
		printk("--sum %ld\n", copied.sum);
		printk("--data %ld\n", dataCopied);
		
		count++;
	}
	
	//cpy buf -> dst
	if (copy_to_user(stats, &copied, sizeof(copied)))
	{
		return -EFAULT;
	}
	
	printk("--STATSmax %ld\n", stats->min);
	printk("--STATSmax %ld\n", stats->max);
	printk("--STATSmax %ld\n", stats->sum);
	
	return result;
}