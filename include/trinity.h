#ifndef _TRINITY_H
#define _TRINITY_H 1

#include "types.h"

#define UNLOCKED 0
#define LOCKED 1

#define __unused__ __attribute((unused))

extern char *progname;

void * alloc_shared(unsigned int size);

void do_main_loop(void);

extern unsigned int page_size;

extern unsigned char exit_reason;

extern bool biarch;

extern bool ignore_tainted;
int check_tainted(void);

void init_watchdog(void);

extern unsigned int user_specified_children;

#define UNUSED(x) (void)(x)

enum exit_reasons {
	STILL_RUNNING = 0,
	EXIT_NO_SYSCALLS_ENABLED = 1,
	EXIT_REACHED_COUNT = 2,
	EXIT_NO_FDS = 3,
	EXIT_LOST_PID_SLOT = 4,
	EXIT_PID_OUT_OF_RANGE = 5,
	EXIT_SIGINT = 6,
	EXIT_KERNEL_TAINTED = 7,
	EXIT_SHM_CORRUPTION = 8,
	EXIT_REPARENT_PROBLEM = 9,
};

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

#define max(x, y) ((x) >= (y) ? (x) : (y))
#define min(x, y) ((x) <= (y) ? (x) : (y))

#endif	/* _TRINITY_H */