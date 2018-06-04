// Test application for the process_ancestor syscall.
// By Brian Fraser

#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <semaphore.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <pwd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>
#include "process_ancestors.h"

#define _PROCESS_ANCESTOR 332

/**
 * Prototypes
 */
static int do_syscall(struct process_info array[], long size, long *num_filled);
static void do_syscall_working(long size);
static void do_syscall_failing(struct process_info array[], long size, long *num_filled, long ret_code);
static void test_internal(_Bool success, int lineNum, char* argStr);
static void test_print_summary(void);
static int single_child_testing(int spawn_array[], int depth);
static int spawn_children_and_test(int spawn_array[], int depth);
static void context_switch_test_once();
static void sleep_ms(int duration_ms);
static void print_process_info_array(struct process_info array[], long size);



/***********************************************************
 * Custom testing framework
 ***********************************************************/
static const char *current_test_function = "";
#define print_test_header() do{ \
		current_test_function = __func__;                      \
		printf("\n\n");                                        \
		printf("*****************************************\n"); \
		printf("   Testing in funct '%s()'\n",  __func__);     \
		printf("*****************************************\n"); \
	} while (0)


// Track results:
static int numTests = 0;
static int numTestPassed = 0;

static int current_syscall_test_num = 0;
static int last_syscall_test_num_failed = -1;
static int num_syscall_tests_failed = 0;

// Macro to allow us to get the line number, and argument's text:
#define TEST(arg) test_internal((arg), __LINE__, #arg)

// Actual function used to check success/failure:
static void test_internal(_Bool success, int lineNum, char* argStr)
{
	numTests++;
	if (!success) {
		if (current_syscall_test_num != last_syscall_test_num_failed) {
			last_syscall_test_num_failed = current_syscall_test_num;
			num_syscall_tests_failed++;
		}
		printf("-------> ERROR %4d: test on line %d failed: %s\n",
				numTestPassed, lineNum, argStr);
		fprintf(stderr, "Test %d: in testing function '%s'\n", current_syscall_test_num, current_test_function);
		fprintf(stderr, "-------> ERROR %4d: test on line %d failed: %s\n",
				numTestPassed, lineNum, argStr);
	} else {
		numTestPassed++;
	}
}

static void test_print_summary(void)
{
	printf("\nExecution finished.\n");
	printf("%4d/%d tests passed.\n", numTestPassed, numTests);
	printf("%4d/%d tests FAILED.\n", numTests - numTestPassed, numTests);
	printf("%4d/%d unique sys-calls FAILED.\n", num_syscall_tests_failed, current_syscall_test_num);
}




/***********************************************************
 * High-Level Tests & main()
 ***********************************************************/
void test_successes()
{
	print_test_header();

	do_syscall_working(1);
	do_syscall_working(2);
	do_syscall_working(100);
}
void test_failures()
{
	print_test_header();

	long num_filled = -1;
	struct process_info *array = malloc(10 * sizeof(struct process_info));

	// Test size:
	do_syscall_failing(array, 0, &num_filled, EINVAL);
	do_syscall_failing(array, -1, &num_filled, EINVAL);
	do_syscall_failing(array, -12345, &num_filled, EINVAL);

	// Test bad pointers to process:
	do_syscall_failing(NULL, 10, &num_filled, EFAULT);
	do_syscall_failing((void*)1, 10, &num_filled, EFAULT);

	// Test bad pointers to num filled
	do_syscall_failing(array, 10, NULL, EFAULT);
	do_syscall_failing(array, 10, (void*)1, EFAULT);
	do_syscall_failing(array, 10, (void*)test_failures, EFAULT);

	// Test not enough memory allocated
	do_syscall_failing(array, 1000000, NULL, EFAULT);

	free(array);
}
void test_children_siblings()
{
	print_test_header();

	TEST(spawn_children_and_test((int[]){1, 0}, 0) == 0);
	TEST(spawn_children_and_test((int[]){4, 0}, 0) == 0);
	TEST(spawn_children_and_test((int[]){4, 3, 2, 1, 0}, 0) == 0);
	TEST(spawn_children_and_test((int[]){2, 2, 0}, 0) == 0);
	TEST(spawn_children_and_test((int[]){5, 5, 5, 5, 5, 5, 5, 5, 0}, 0) == 0);
	TEST(spawn_children_and_test((int[]){50, 1, 50, 0}, 0) == 0);
	TEST(spawn_children_and_test((int[]){1, 2, 3, 1, 2, 3, 1, 2, 3, 1, 4, 2, 5, 3, 2, 4, 5, 1, 3, 1, 2, 2, 3, 3, 4, 1, 0}, 0) == 0);
}

void test_pid()
{
	print_test_header();

	const long SIZE = 30;
	const long SWAPPER_PID = 0;
	const long INIT_PID = 1;

	struct process_info *array = malloc(sizeof(*array) * SIZE);

	// Call sys-call
	long num_filled = -1;
	int result = do_syscall(array, SIZE, &num_filled);
	TEST(result == 0);
	if (result != 0) {
		return;
	}
	print_process_info_array(array, num_filled);


	// Check that my PID is correct:
	long my_pid = (long) getpid();
	TEST(my_pid == array[0].pid);

	// Check than nobody else has PID same as mine:
	for (int i = 1; i < num_filled; i++) {
		TEST(my_pid != array[i].pid);
	}

	// Check that swapper and init are 0 and 1:
	int swapper_idx = num_filled - 1;
	TEST(array[swapper_idx].pid == SWAPPER_PID);
	TEST(array[swapper_idx-1].pid == INIT_PID);

	// Cleanup
	free(array);
}

void test_context_switch()
{
	print_test_header();

	for (int i = 0; i < 3; i++) {
		context_switch_test_once();
	}
}

void test_name()
{
	print_test_header();

	const long SIZE = 30;

	// Get base line
	struct process_info array[SIZE];
	long num_filled = -1;
	int result = do_syscall(array, SIZE, &num_filled);
	TEST(result == 0 && num_filled > 0);
	if (result != 0) {
		return;
	}

	// Check my name:
	TEST(strncmp(array[0].name, program_invocation_short_name, ANCESTOR_NAME_LEN - 1) == 0);

	// Check swapper and init:
	int swapper_idx = num_filled - 1;
	TEST(strncmp(array[swapper_idx].name, "swapper/0", ANCESTOR_NAME_LEN) == 0);
	TEST(strncmp(array[swapper_idx - 1].name, "init", ANCESTOR_NAME_LEN) == 0);
}

void test_state()
{
	print_test_header();

	const long SIZE = 30;
	#define TASK_RUNNING            0
	#define TASK_INTERRUPTIBLE      1
	#define TASK_UNINTERRUPTIBLE    2
	#define TASK_ZOMBIE             4
	#define TASK_STOPPED            8

	// Get base line
	struct process_info array[SIZE];
	long num_filled = -1;
	int result = do_syscall(array, SIZE, &num_filled);
	TEST(result == 0 && num_filled > 0);
	if (result != 0) {
		return;
	}
	print_process_info_array(array, num_filled);

	// Test that I am running
	TEST(array[0].state == TASK_RUNNING);

	// Test that all my ancestors are OK:
	for (int i = 1; i < num_filled; i++) {
		TEST(
				(array[i].state == TASK_RUNNING)
				|| (array[i].state == TASK_INTERRUPTIBLE)
				|| (array[i].state == TASK_UNINTERRUPTIBLE)
			);
	}
	// Test that not all are RUNNING:
	_Bool all_running = true;
	for (int i = 1; i < num_filled; i++) {
		if (array[i].state != TASK_RUNNING) {
			all_running = false;
		}
	}
	TEST(!all_running);
}

void user_id_check(int expected_uid)
{
	// Get info from sys-call
	const long SIZE = 30;
	struct process_info array[SIZE];
	long num_filled = -1;
	int result = do_syscall(array, SIZE, &num_filled);
	TEST(result == 0 && num_filled > 0);
	if (result != 0) {
		return;
	}
	print_process_info_array(array, num_filled);

	// Check that current process matches UID:
	TEST(array[0].uid == expected_uid);
}

/**
 * This test must be run as root because we change to another user.
 * If not running as root, this will crash the test program.
 */
#define UID_FOR_USER 1000
#define UID_FOR_ROOT 0
void test_user()
{
	print_test_header();
	// Check that I'm currently "root"
	assert(getuid() == UID_FOR_ROOT);

	// Check syscall for returning UID as root
	user_id_check(UID_FOR_ROOT);

	// Switch:
	if (setuid(UID_FOR_USER) != 0) {
	    printf("setuid() to %d failed", UID_FOR_USER);
	    assert(false);
	    exit(1);
	}
	assert(getuid() == UID_FOR_USER);

	// check getting correct UID:
	user_id_check(UID_FOR_USER);
}

void display_printk_help(void)
{
	printf("\n");
	printf("Help limiting spew from sys-call's printk():\n");
	printf("  View current printk level (1st num): cat /proc/sys/kernel/printk\n");
	printf("  Turn on printk's:                    echo 7 > /proc/sys/kernel/printk\n");
	printf("  Turn off printk's:                   echo 1 > /proc/sys/kernel/printk\n");
	printf("  Store stderr to file (clean!):       ./process_ancestor_test 2> errors\n");
	printf("                                       cat errors\n");
	printf("\n");
}



int main(int argc, char *argv[])
{
	test_successes();
	test_failures();
	test_children_siblings();
	test_pid();
	test_context_switch();
	test_name();
	test_state();

	// Must be run last: changes uid.
	test_user();

	display_printk_help();
	test_print_summary();
	return 0;
}







/***********************************************************
 * Children and Siblings: Fork processes
 ***********************************************************/
#define TEST_NAME "/test_semaphore"
#define END_NAME "/end_semaphore"
static int spawn_children_and_test(int spawn_array[], int depth)
{
	_Bool first_in_tree = (depth == 0);
	_Bool first_in_level = (depth > 0);
	int tests_failed = 0;

	// Shared (named) semaphores.
	sem_t *start_testing;
	sem_t *ok_to_end;
	if (first_in_tree) {
		if (sem_unlink(TEST_NAME) != 0 && errno != ENOENT) {
			printf("ERROR: Unable to unlink the TEST semaphore!\n");
			exit(1);
		}
		if (sem_unlink(END_NAME) != 0 && errno != ENOENT) {
			printf("ERROR: Unable to unlink the END semaphore!\n");
			exit(1);
		}
		start_testing = sem_open(TEST_NAME, O_CREAT, 0644, 0);
		ok_to_end = sem_open(END_NAME, O_CREAT, 0664, 0);
	}

	if (first_in_tree) {
		printf("\n\n");
	}

	int num_to_spawn = spawn_array[depth];
//	printf("-> Depth %d spawning %d processes.\n", depth, num_to_spawn);
	for (int i = 0; i < num_to_spawn; i++) {
		pid_t pid = fork();
		// First child recurses:
		if (pid == 0 && i == 0) {
			return spawn_children_and_test(spawn_array, depth + 1);
		}
		// Child breaks out of loop (to avoid n^2 processes!)
		if (pid == 0) {
			first_in_tree = false;
			first_in_level = false;
			break;
		}
	}

	if (first_in_tree) {
		// Signal to test
		sleep(1);
//		printf("-> Signaling to start testing.\n");
		sem_post(start_testing);
		tests_failed += single_child_testing(spawn_array, depth);

		// Signal to exit
		sleep(1);
//		printf("-> Signaling to end processes.\n");
		sem_post(ok_to_end);
	} else {
		// re-open semaphores in shared space:
		start_testing = sem_open(TEST_NAME, 0);
		ok_to_end = sem_open(END_NAME, 0);
		if (first_in_level) {
			// Wait on master to begin test
			sem_wait(start_testing);
			sem_post(start_testing);

			// Test
			// DO SOMETHING HERE
			tests_failed = single_child_testing(spawn_array, depth);
		}
		// Wait on master to end
		sem_wait(ok_to_end);
		sem_post(ok_to_end);

		sem_close(ok_to_end);
		sem_close(start_testing);
//		printf("  << Exiting level %d\n", depth);
	}

	// Cleanup
	if (first_in_tree || first_in_level) {
		for (int i = 0; i < num_to_spawn; i++) {
			int status = 0;
			pid_t pid_wait = wait(&status);
			if (WIFEXITED(status)) {
//				printf("  Process pid %d returned # %d.\n", pid_wait, WEXITSTATUS(status));
				tests_failed += WEXITSTATUS(status);
			} else {
				printf("Wait error: Process with pid %d returned 0x%x.\n", pid_wait, status);
				tests_failed ++;
			}
		}
	}
	if (first_in_tree) {
		sem_close(ok_to_end);
		sem_close(start_testing);

		if (sem_unlink(TEST_NAME) != 0 && errno != ENOENT) {
			printf("ERROR: Unable to unlink the TEST semaphore at end!\n");
			exit(1);
		}
		if (sem_unlink(END_NAME) != 0 && errno != ENOENT) {
			printf("ERROR: Unable to unlink the END semaphore at end!\n");
			exit(1);
		}
	}

	if (!first_in_tree) {
		exit(tests_failed);
	}


	return tests_failed;
}
static int single_child_testing(int spawn_array[], int depth)
{
	int count_failed = 0;
	//
	const long size = depth + 10;
	struct process_info *array = malloc(sizeof(*array) * size);


	// Call sys-call
	long num_filled = -1;
	int result = do_syscall(array, size, &num_filled);
	if (result != 0) {
		return 1;
	}

	// If we are at the bottom, then print out data:
	if (spawn_array[depth] == 0) {
		print_process_info_array(array, num_filled);
	}

	// Check all levels above us are correct.
	for (int i = depth; i >= 0; i--) {
		int process_info_idx = depth - i;

		// Check siblings only for not at root of tree.
		if (i > 0) {
			int num_siblings_expected = spawn_array[i - 1];
			int num_siblings_got = array[process_info_idx].num_siblings;
//			printf("   %d: Siblings @ depth %d: exp %d == got %d?\n",
//					depth, i, num_siblings_expected, num_siblings_got);
			count_failed += (num_siblings_expected != num_siblings_got);
		}

		int num_children_expected = spawn_array[i];
		int num_children_got = array[process_info_idx].num_children;
//		printf("   %d: Children @ depth %d: exp %d == got %d?\n",
//				depth, i, num_children_expected, num_children_got);
		count_failed += (num_children_expected != num_children_got);
	}

	free(array);
	return count_failed;
}



/***********************************************************
 * Voluntary and involuntary context switch count
 ***********************************************************/
static void context_switch_test_once()
{
	const long SIZE = 30;

	// Get base line
	struct process_info array_start[SIZE];
	long num_filled = -1;
	int result = do_syscall(array_start, SIZE, &num_filled);
	TEST(result == 0 && num_filled > 0);
	if (result != 0) {
		return;
	}
	print_process_info_array(array_start, num_filled);

	// Sleep
	sleep_ms(10);

	// Delay
	const long BUSY_WAIT_COUNT = 100000000;
	volatile long count = 0;
	for (int i = 0; i < BUSY_WAIT_COUNT; i++) {
		count++;
	}
	// Print to prevent any optimization by the compiler.
	printf("Did busy wait up to %ld\n", count);

	// Get new reading
	struct process_info array_end[SIZE];
	result = do_syscall(array_end, SIZE, &num_filled);
	TEST(result == 0 && num_filled > 0);
	if (result != 0) {
		return;
	}
	print_process_info_array(array_end, num_filled);

	// Check differences
	TEST(array_start[0].nivcsw < array_end[0].nivcsw);
	TEST(array_start[0].nvcsw < array_end[0].nvcsw);
}


/***********************************************************
 * sys-call and test results
 ***********************************************************/
static int do_syscall(struct process_info array[], long size, long *num_filled)
{
	current_syscall_test_num++;
	printf("\nTest %d: ..Diving to kernel level\n", current_syscall_test_num);
	int result = syscall(_PROCESS_ANCESTOR, array, size, num_filled);
	int my_errno = errno;
	printf("..Rising to user level w/ result = %d", result);
	if (result < 0) {
		printf(", errno = %d\n", my_errno);
	} else {
		printf("\n");
		my_errno = 0;
	}

	return my_errno;

}
static void do_syscall_working(long size)
{
	const int FILL = 0xFFFFFFFFL;
	// Allocate 2 extra structs: one at beginning and at end.
	struct process_info array[size + 2];
	memset(array, FILL, (size + 2) * sizeof(struct process_info));

	// Have sys-call fill starting at idx 1 (not 0!) to catch if overflowing the buffer.
	long num_filled = -1;
	int result = do_syscall(&array[1], size, &num_filled);
	print_process_info_array(&array[1], num_filled);

	TEST(result == 0);
	struct process_info blank;
	memset(&blank, FILL, sizeof(blank));
	// Check that before the first element is unchanged
	TEST(memcmp(&blank, &array[0], sizeof(blank)) == 0);
	// Check that after the last element is unchanged
	TEST(memcmp(&blank, &array[size+1], sizeof(blank)) == 0);
	TEST(num_filled > 0 && num_filled <= size);
}
static void do_syscall_failing(struct process_info array[], long size, long *num_filled, long ret_code)
{
	int result = do_syscall(array, size, num_filled);
	TEST(result == ret_code);
}




/***********************************************************
 * Utility Functions
 ***********************************************************/
static void sleep_ms(int duration_ms)
{
	struct timespec req;
	req.tv_nsec = (duration_ms % 1000) * 1000 * 1000;
	req.tv_sec = (duration_ms / 1000);
	nanosleep(&req, NULL);
}

static void print_process_info_array(struct process_info array[], long size)
{
	sleep_ms(100);
	const int DEF_WIDTH = 7;
	const int NAME_WIDTH = ANCESTOR_NAME_LEN + 1;
	// Print Headers
	printf("%*s%*s %-*s%*s%*s%*s%*s%*s%*s\n",
			DEF_WIDTH, "Idx#",
			DEF_WIDTH, "PID",
			NAME_WIDTH, "Name",
			DEF_WIDTH, "State",
			DEF_WIDTH, "UID",
			DEF_WIDTH, "#VCSW",
			DEF_WIDTH, "#IVCSW",
			DEF_WIDTH, "#Child",
			DEF_WIDTH, "#Sib"
			);

	// Print Data
	for (int i = size - 1; i >= 0; i--) {
		printf("%*d%*ld %-*s%*ld%*ld%*ld%*ld%*ld%*ld\n",
				DEF_WIDTH, i,
				DEF_WIDTH, array[i].pid,
				NAME_WIDTH, array[i].name,
				DEF_WIDTH, array[i].state,
				DEF_WIDTH, array[i].uid,
				DEF_WIDTH, array[i].nvcsw,
				DEF_WIDTH, array[i].nivcsw,
				DEF_WIDTH, array[i].num_children,
				DEF_WIDTH, array[i].num_siblings
		);
	}
}
