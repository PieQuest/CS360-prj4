#include <linux/types.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>

#define _PROCESS_ANCESTORS_ 342
#define ANCESTOR_NAME_LEN 16

struct process_info {
    long pid; /* Process ID */
    char name[ANCESTOR_NAME_LEN]; /* Program name of process */
    long state; /* Current process state */
    long uid; /* User ID of process owner */
    long nvcsw; /* # voluntary context switches */
    long nivcsw; /* # involuntary context switches */
    long num_children; /* # children process has */
    long num_siblings; /* # sibling process has */
};


void dump(struct process_info *info, int size) {
    struct process_info process;
    for (int i = 0; i < size; i++) {
        process = info[i];
        printf("info->pid: %ld\n", process.pid);
        printf("info->name: %s\n", process.name);
        printf("info->state: %ld\n", process.state);
        printf("info->uid: %ld\n", process.uid);
        printf("info->nvcsw: %ld\n", process.nvcsw);
        printf("info->nivcsw: %ld\n", process.nivcsw);
        printf("info->num_children: %ld\n", process.num_children);
        printf("info->num_siblings: %ld\n", process.num_siblings);
    }
}

int main(int argc, char *argv[]) {

    long size = 2;
    long *num = 0;
    int result;

    printf("\nDiving to kernel level\n\n");

    struct process_info *info;
    result = syscall(_PROCESS_ANCESTORS_, info, size, num);
    dump(info, size);

    printf("\nRising to user level w/ result = %d\n\n", result);

    return 0;
}