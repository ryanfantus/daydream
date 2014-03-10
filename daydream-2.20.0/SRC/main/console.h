#ifndef _CONSOLE_H_INCLUDED
#define _CONSOLE_H_INCLUDED

#include <sys/types.h>
#include <sys/time.h>
#include <string.h>
#include <unistd.h>

#include <config.h>

/* console.c */
int console_active(void);
int init_console(void);
void open_console(void);
void close_console(void);
int console_select_input(int, fd_set *);
int console_pending_input(fd_set *);
int console_getc(void);
int console_putsn(void *, size_t) __attr_bounded__ (__buffer__, 1, 2);
void finalize_console(void);

#endif
