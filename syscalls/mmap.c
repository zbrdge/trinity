/*
 * SYSCALL_DEFINE6(mmap, unsigned long, addr, unsigned long, len,
	unsigned long, prot, unsigned long, flags,
	unsigned long, fd, unsigned long, off)
 */
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include "maps.h"
#include "sanitise.h"
#include "shm.h"
#include "arch.h"
#include "compat.h"
#include "random.h"
#include "utils.h"	//ARRAY_SIZE
#include "utils.h"

#ifdef __x86_64__
#define NUM_FLAGS 13
#else
#define NUM_FLAGS 12
#endif

// need this to actually get MAP_UNINITIALIZED defined
#define CONFIG_MMAP_ALLOW_UNINITIALIZED

static void do_anon(int childno)
{
	/* no fd if anonymous mapping. */
	shm->a5[childno] = -1;
	shm->a6[childno] = 0;
}

void sanitise_mmap(int childno)
{
	unsigned int i;
	unsigned int flagvals[NUM_FLAGS] = { MAP_FIXED, MAP_ANONYMOUS,
			MAP_GROWSDOWN, MAP_DENYWRITE, MAP_EXECUTABLE, MAP_LOCKED,
			MAP_NORESERVE, MAP_POPULATE, MAP_NONBLOCK, MAP_STACK,
			MAP_HUGETLB, MAP_UNINITIALIZED,
#ifdef __x86_64__
			MAP_32BIT,
#endif
	};
	unsigned int numflags = rand() % NUM_FLAGS;
	unsigned long sizes[] = {
		-1,	/* over-written with page_size below */
		1 * MB, 2 * MB, 4 * MB, 10 * MB,
		1 * GB,
	};

	sizes[0] = page_size;

	/* Don't actually set a hint right now. */
	shm->a1[childno] = 0;

	shm->a2[childno] = sizes[rand() % ARRAY_SIZE(sizes)];

	// set additional flags
	for (i = 0; i < numflags; i++)
		shm->a4[childno] |= flagvals[rand() % NUM_FLAGS];

	if (shm->a4[childno] & MAP_ANONYMOUS) {
		do_anon(childno);
	} else {
		/* page align non-anonymous mappings. */
		shm->a6[childno] &= PAGE_MASK;
	}
}

static void post_mmap(int childno)
{
	char *p;
	struct list_head *list;
	struct map *new;

	p = (void *) shm->retval[childno];
	if (p == MAP_FAILED)
		return;

	new = zmalloc(sizeof(struct map));
	new->name = strdup("misc");
	new->size = shm->a2[childno];
	new->prot = shm->a3[childno];
	new->ptr = p;
	new->type = MAP_LOCAL;

	// Add this to a list for use by subsequent syscalls.
	list = &shm->mappings[childno]->list;
	list_add_tail(&new->list, list);
	shm->num_mappings[childno]++;

	/* Sometimes dirty the mapping. */
	if (rand_bool())
		dirty_mapping(new);
}

static char * decode_mmap(int argnum, int childno)
{
	char *buf;

	if (argnum == 3) {
		int flags = shm->a3[childno];
		char *p;

		p = buf = zmalloc(80);
		p += sprintf(buf, "[");

		if (flags == 0) {
			p += sprintf(p, "PROT_NONE]");
			return buf;
		}
		if (flags & PROT_READ)
			p += sprintf(p, "PROT_READ|");
		if (flags & PROT_WRITE)
			p += sprintf(p, "PROT_WRITE|");
		if (flags & PROT_EXEC)
			p += sprintf(p, "PROT_EXEC|");
		if (flags & PROT_SEM)
			p += sprintf(p, "PROT_SEM ");
		p--;
		sprintf(p, "]");

		return buf;
	}
	return NULL;
}

struct syscallentry syscall_mmap = {
	.name = "mmap",
	.num_args = 6,

	.sanitise = sanitise_mmap,
	.post = post_mmap,
	.decode = decode_mmap,

	.arg1name = "addr",
	.arg2name = "len",
	.arg2type = ARG_LEN,
	.arg3name = "prot",
	.arg3type = ARG_LIST,
	.arg3list = {
		.num = 4,
		.values = { PROT_READ, PROT_WRITE, PROT_EXEC, PROT_SEM },
	},
	.arg4name = "flags",
	.arg4type = ARG_OP,
	.arg4list = {
		.num = 2,
		.values = { MAP_SHARED, MAP_PRIVATE },
	},
	.arg5name = "fd",
	.arg5type = ARG_FD,
	.arg6name = "off",
	.arg6type = ARG_LEN,

	.group = GROUP_VM,
	.flags = NEED_ALARM,
};
