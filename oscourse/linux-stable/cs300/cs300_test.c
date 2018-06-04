#include <linux/kernel.h>

//Implement a HelloWorld system call
//Argument is passed from call in user space.

asmlinkage long sys_cs300_test(int argument)
{

	long result = 0;

	printk("Hello World!\n");
	printk("--syscall argument %d\n", argument);

	result = argument + 1;
	printk("--returning %d + 1 = %ld\n", argument, result);
	return result;
}
