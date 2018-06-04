#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>

#if _LP64 == 1
#define _CS300_TEST_ 330
#else
#define _CS300_TEST_ 360
#endif
int main(int argc, char *argv[])
{
	printf("\nDiving to kernel level\n\n");
	int result = syscall(_CS300_TEST_ , 12345);
	printf("\nRising to user level w/ result = %d\n\n", result);
    
	return 0;
}

