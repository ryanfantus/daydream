#ifndef _DDCOMMON_H_INCLUDED
#define _DDCOMMON_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>
#include <string.h>
#include <termios.h>
#include <stdio.h>
#include <stdarg.h>
	
#include <config.h>
#include <ddutmpx.h>

#define UMASK S_IRWXO

/* FIXME: some machines don't have correct snprintf() */
#define ssnprintf(__b__, __fmt__, __args__...) ({			\
	int __rv__;							\
	__rv__ = snprintf(__b__, sizeof(__b__), __fmt__, ## __args__);	\
 	if (__rv__ >= 0) 						\
 		__rv__ = (((unsigned int) __rv__) > sizeof(__b__)) 	\
 			? -1 : 0;					\
 	if (__rv__ < 0)							\
 		__rv__ = -1;						\
 	__rv__;								\
})

/* libddcommon.c */
/*off_t dd_lseek(int fd, off_t offset, int whence);*/
#define dd_lseek lseek

char *strlwr(char *);
char *strupr(char *);

/* vasprintf.c */
#ifndef HAVE_VASPRINTF
int vasprintf(char **, const char *, va_list);
#endif

/* strlcpy.c */
#ifndef HAVE_STRLCPY
size_t strlcpy(char *, const char *, size_t);
#endif

/* strlcat.c */
#ifndef HAVE_STRLCAT
size_t strlcat(char *, const char *, size_t);
#endif

#ifndef HAVE_SETENV
int setenv(const char *name, const char *value, int overwrite);
#endif
#ifndef HAVE_UNSETENV
void unsetenv(const char *name);
#endif

#ifndef HAVE_CFMAKERAW
int cfmakeraw(struct termios *t);
#endif

#ifdef MAINTENANCE_DEBUG
void dump_file_descriptors(const char * const);
#endif

int create_directory(const char * const, uid_t, gid_t, mode_t);

int pathcat2(char *, size_t, const char *, const char *)
	__attr_bounded__ (__string__, 1, 2);

int deldir(const char *);

int lock_and_open(const char *, int, mode_t);
int lock_file(int, int);
int unlock_file(int);
int unlock_and_close(int fd);

int setperm(const char *, mode_t);
int fsetperm(int, mode_t);

ssize_t safe_read(int, void *, size_t)
	__attr_bounded__ (__buffer__, 2, 3);
ssize_t safe_write(int, const void *, size_t)
	__attr_bounded__ (__buffer__, 2, 3);

#ifdef __cplusplus
};
#endif

#endif 
