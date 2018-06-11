/*
Michael McMurray & Ryan Edson
CISS360 // Dr.Rrushi
Project 4
*/

#include "process_ancestors.h"
#include <linux/kernel.h>
#include <linux/unistd.h>
#include <linux/sched.h>
#include <linux/cred.h>
#include <linux/compiler.h>
#include <linux/uaccess.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <asm/percpu.h>
#include <asm-generic/errno-base.h>


asmlinkage long sys_process_ancestors(struct process_info   info_array[],   long size, long *num_filled) {
  struct task_struct* tastStruct = current;
  struct process_info procInfo;
  struct list_head currentNode;
  long structSize = sizeof(struct process_info);
  if(size <= 0) {
	  return -EINVAL;
  }
  long childCount;
  long siblingCount;
  long i;
  for(i = 0; i < size; i++) {
    procInfo.pid = tastStruct->pid;
    strncpy(procInfo.name, tastStruct->comm, ANCESTOR_NAME_LEN);
    procInfo.state = tastStruct->state;
    procInfo.uid = (long)tastStruct->cred->uid.val;
    procInfo.nvcsw = tastStruct->nvcsw;
    procInfo.nivcsw = tastStruct->nivcsw;
    childCount = 0;
    siblingCount = 0;
    currentNode = tastStruct->children;
    while (currentNode.next!= &tastStruct->children) {
      childCount++;
      currentNode = *currentNode.next;
    }
    currentNode = tastStruct->sibling;
    while (currentNode.next != &tastStruct->sibling) {
      siblingCount++;
      currentNode = *currentNode.next;
    }
    procInfo.num_children = childCount;
    procInfo.num_siblings = siblingCount;
    if(copy_to_user(&info_array[i], &procInfo, structSize)) {
      return -EFAULT;
	}
    if(tastStruct == tastStruct->parent) {
      i++;
      break;
    }
    tastStruct = tastStruct->parent;
  }
  if(copy_to_user(num_filled, &i, sizeof(long))) {
	  return -EFAULT;
  }
  return 0;
}