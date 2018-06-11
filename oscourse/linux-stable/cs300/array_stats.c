/*
Michael McMurray & Ryan Edson
CISS360 // Dr.Rrushi
Project 4
array_stats
*/

#include <linux/uaccess.h>
#include <linux/kernel.h>
#include "array_stats.h"

asmlinkage long sys_array_stats(struct array_stats *stats, long data[], long size) {
	struct array_stats statsArray = {10000000, 0, 0};
	long dataValue = 0;
	long count;
	if (copy_from_user(&statsArray.min, &data[0], sizeof(data[count]))) {
		return -EFAULT;
	}
	for (count=0; count < size; count++) {
		if (copy_from_user(&dataValue, &data[count], sizeof(data[count]))) {
			return -EFAULT;
		}
		if(dataValue > statsArray.max) {
			statsArray.max = dataValue;
		}
		if(dataValue < statsArray.min) {
			statsArray.min = dataValue;
		}
		statsArray.sum += dataValue;
	}
	if (copy_to_user(stats, &statsArray, sizeof(statsArray))) {
		return -EFAULT;
	}
	return 0;
}