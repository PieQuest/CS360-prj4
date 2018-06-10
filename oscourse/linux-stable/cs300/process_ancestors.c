#include <linux/kernel.h>
#include <linux/uaccess.h>
#include <linux/unistd.h>
#include <linux/sched.h>
#include <linux/cred.h>
#include <linux/compiler.h>
#include <asm/percpu.h>
#include <asm-generic/errno-base.h>
#include <linux/errno.h>
#include <linux/string.h>

#include "process_ancestors.h"


//asmlinkage long sys_process_ancestors(struct process_info info_array[], long size, long *num_filled);

asmlinkage long sys_process_ancestors(struct process_info info_array[], long size, long *num_filled) {
    struct process_info process;
    //struct task_struct task_struct;
    struct task_struct *task;
    struct list_head *list;

    //long count=0;
    long count_children = 0;
    long count_sibling = 0;
    long count = 0;
    //int j=0;

    if (size <= 0) return -EINVAL;
    if (!info_array || !num_filled) return -EFAULT;
	
	/*
	if (copy_from_user(info_array[i], process, sizeof(info_array[])))
	{
		return -EFAULT;
	}
	*/
	printk("Inilized.\n");
    for (task = current; task != &init_task; task = task->parent) {

        process.pid = (long) task->pid;
        /printk("Process ID: %ld\n", process.pid);

        process.state = task->state;
        //printk("Process State: %ld\n", process.state);

        //printk("Process UID: %ld\n", process.uid);
        //printk("Process Name: ");
		printk("Pass1.\n");
        memset(process.name, '\0', sizeof(process.name));
        strcpy(process.name, task->comm);

		printk("Pass2.\n");
        list_for_each(list, &current->children);
        {
            //task = list_entry(list, struct task_struct, sibling);
            //task now points to one of current’s children
            count_children++;
            //if (task==task_struct) break;
        }
		printk("Pass3.\n");
        list_for_each(list, &current->sibling);
        {
            //task = list_entry(list, struct task_struct, sibling);
            //task now points to one of current’s children
            count_sibling++;
            //if (task==task_struct) break;
        }
		printk("Pass4.\n");
        process.num_children = count_children;
        process.num_siblings = count_sibling;
        //printk("Process num_children: %ld\n", process.num_children);

		/*
		if (copy_to_user(&info_array[count], &process, sizeof(process)))
		{
			return -EFAULT;
		}
		*/
		printk("Pass5.\n");
        info_array[count] = process;
		
		free(process.name);
		
        count++;
		printk("Pass6.\n");
    }

    return 0;

}