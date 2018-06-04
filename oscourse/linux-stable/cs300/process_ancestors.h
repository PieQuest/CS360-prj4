//Structure to hold values returned by process_ancestors sys-call
#ifndef _PROCESS_ANCESTORS_H
#define _PROCESS_ANCESTORS_H

#define ANCESTOR_NAME_LEN 16

struct process_info {
	long pid;	/* Process ID */
	char name[ANCESTOR_NAME_LEN];	/* Program name of process */
	long state;	/* Current process state */
	long uid;	/* User ID of process owner */
	long nvcsw;	/* # voluntary context switches */
	long nivcsw;	/* # involuntary context switches */
	long num_children;	/* # children process has */
	long num_siblings;	/* # sibling process has */
};

#endif