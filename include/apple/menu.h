/*
 * Copyright (C) 2007-2008 Apple Inc. All rights reserved.
 * Copyright (C) 2006 Apple Computer, Inc. All rights reserved.
 *
 * This document is the property of Apple Computer, Inc.
 * It is considered confidential and proprietary.
 *
 * This document may not be reproduced or transmitted in any form,
 * in whole or in part, without the express written permission of
 * Apple Computer, Inc.
 */
#ifndef __SYS_MENU_H
#define __SYS_MENU_H

#include <sys/types.h>
#include <sys/security.h>

__BEGIN_DECLS

struct cmd_arg {
	int n;
	size_t u;
	size_t h;
	bool b;
	char *str;
};

typedef int (*cmd_func)(int argc, struct cmd_arg *args);

struct cmd_menu_item {
	const char *cmd;
	cmd_func func;
	const char *help;
	void *meta;
};

extern utime_t gMenuLastActivityTime;
extern struct task_event gMenuTaskReadyEvent;

bool process_command_line(char *line, int *last_command_rval);
int menu_task(void *arg);
char *menu_prompt(char *new_prompt);

extern struct bgcolor g_bgcolor;

/*
 * New-style linker magic menu command registration.
 */
#include <sys/linker_set.h>
#if WITH_MENU

#define MENU_COMMAND_PROTOTYPE(_function)		\
int _function(int argc, struct cmd_arg *args)

#define MENU_COMMAND_HELPER(_command, _function, _help, _meta)	\
MENU_COMMAND_PROTOTYPE(_function);			\
static const struct cmd_menu_item __attribute__((used))	\
__menu_entry_##_command = {				\
	#_command,					\
	_function,					\
	_help,						\
	_meta						\
};							\
LINKER_SET_ENTRY(menu, __menu_entry_##_command)

// MENU_COMMAND always gets you a menu command, but when using
// the simple menu (release builds and memory-tight accessories)
// don't compile in the help strings
#if !WITH_SIMPLE_MENU
#define MENU_COMMAND(_c, _f, _h, _m)			MENU_COMMAND_HELPER(_c, _f, _h, _m)
#else
#define MENU_COMMAND(_c, _f, _h, _m)			MENU_COMMAND_HELPER(_c, _f, "", _m)
#endif

/* MENU_COMMAND_DEVELOPMENT gets you a menu command in DEVELOPMENT_BUILD and DEBUG_BUILD */
#if (DEVELOPMENT_BUILD || DEBUG_BUILD)
# define MENU_COMMAND_DEVELOPMENT(_c, _f, _h, _m)	MENU_COMMAND(_c, _f, _h, _m)
#else
# define MENU_COMMAND_DEVELOPMENT(_c, _f, _h, _m)	struct _hack
#endif

/* MENU_COMMAND_DEBUG gets you a menu command in DEBUG_BUILD only */
#if DEBUG_BUILD
# define MENU_COMMAND_DEBUG(_c, _f, _h, _m)		MENU_COMMAND(_c, _f, _h, _m)
#else
# define MENU_COMMAND_DEBUG(_c, _f, _h, _m)		struct _hack
#endif

#else
/* no WITH_MENU, nop these out and cause the linker to delete everything else */
#define MENU_COMMAND(_c, _f, _h, _m)			struct _hack
#define MENU_COMMAND_DEVELOPMENT(_c, _f, _h, _m)	struct _hack
#define MENU_COMMAND_DEBUG(_c, _f, _h, _m)		struct _hack
#endif

__END_DECLS

#endif
