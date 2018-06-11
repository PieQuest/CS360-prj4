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

asmlinkage long sys_process_ancestors(struct process_info   info_array[],   long size, long *num_filled)
{
  struct task_struct* currentProcess = current;
  struct process_info infoStruct;
  struct list_head circLL_curr;
  long structSize = sizeof(struct process_info);
  long i;
  long numChildren;
  long numSiblings;
  if(size <= 0) { return -EINVAL; }
  for(i = 0; i < size; i++)
  {
    infoStruct.pid = currentProcess->pid;
    strncpy(infoStruct.name, currentProcess->comm, ANCESTOR_NAME_LEN);
    infoStruct.state = currentProcess->state;
    infoStruct.uid = (long)currentProcess->cred->uid.val;
    infoStruct.nvcsw = currentProcess->nvcsw;
    infoStruct.nivcsw = currentProcess->nivcsw;
    numChildren = 0;
    numSiblings = 0;
    circLL_curr = currentProcess->children;
    while (circLL_curr.next!= &currentProcess->children)
    {
      numChildren++;
      circLL_curr = *circLL_curr.next;
    }
    printk("number of children: %ld", infoStruct.num_children);
    circLL_curr = currentProcess->sibling;
    while (circLL_curr.next != &currentProcess->sibling)
    {
      numSiblings++;
      circLL_curr = *circLL_curr.next;
    }
    printk("number of siblings: %ld", infoStruct.num_siblings);
    infoStruct.num_children = numChildren;
    infoStruct.num_siblings = numSiblings;
    if( copy_to_user(&info_array[i], &infoStruct, structSize) )
      return -EFAULT;
    if(currentProcess == currentProcess->parent)
    {
      i++;
      break;
    }
    currentProcess = (currentProcess->parent);
  }
  if( copy_to_user(num_filled, &i, sizeof(long)) ) { return -EFAULT; }
  return 0;
}