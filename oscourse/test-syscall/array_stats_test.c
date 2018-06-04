#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <errno.h>

#define _ARRAY_STATS_ 341

struct array_stats{
	long min;
	long max;
	long sum;
};

struct array_stats stats = {1000, 0, 0};

long data[3] = {1, 50, 100};
int size = 3;
 

int main(int argc, char *argv[])
{	
	printf("\nDiving to kernel level\n\n");
	int result = syscall(_ARRAY_STATS_, &stats, data, size);
	printf("\nRising to user level w/ result = %d\n\n", result);

	return 0;
}