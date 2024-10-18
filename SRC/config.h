/*
 * The following indicate the availability of a specific header
 */
#if !(defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__APPLE__))
#define HAVE_PTY_H 1
#define HAVE_STROPTS_H 1
#define HAVE_SYS_VFS_H 1
#define HAVE_UTMPX_H 1
#else
#define HAVE_SYS_MOUNT_H 1	// BSD has fstatfs in here instead of sys/vfs.h
#endif
#define HAVE_FCNTL_H 1
#define HAVE_INTTYPES_H 1
#define HAVE_STDINT_H 1
#define HAVE_SYS_IOCTL_H 1
#define HAVE_SYS_MMAN_H 1
#define HAVE_SYS_STATVFS_H 1
#define HAVE_UTMP_H 1

/*
 * The following indicate the availability of a specific function
 */
#define HAVE_ALPHASORT 1	// 4.2 BSD convenience function
#define HAVE_SCANDIR 1		// 4.2 BSD convenience function
#define HAVE_CFMAKERAW 1	// Termios extension
#define HAVE_MMAP 1		// The only file which uses this (ddsz) undefines it first.
#define HAVE_SETENV 1		// POSIX.1 (via Version 7 AT&T UNIX)
#define HAVE_UNSETENV 1		// POSIX.1 (via Version 7 AT&T UNIX)
#define HAVE_VASPRINTF 1	// GNU C library extension
#define HAVE_STATVFS 1		// POSIX.1 (Free to return no useful data however)
#ifdef __linux__
#define HAVE_GETPT		// GNU extension (opens /dev/ptmx even with it located elsewhere)
#else
#define HAVE_STRLCPY 1		// OpenBSD extension (Ulrich Drepper of glibc fame hates these)
#define HAVE_STRLCAT 1		// OpenBSD extension (Ulrich Drepper of glibc fame hates these)
#endif

// utmp stuff.
#ifdef HAVE_UTMPX_H		// POSIX.1, but not yet in BSDs
#define HAVE_ENDUTXENT 1
#define HAVE_GETUTXENT 1
#define HAVE_SETUTXENT 1
#endif

// These are deprecated from POSIX.1 and not supported by BSDs
#if defined(HAVE_UTMP_H) && (!(defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__APPLE__)))
#define HAVE_ENDUTENT 1
#define HAVE_GETUTENT 1
#define HAVE_SETUTENT 1
#endif

/*
 * The following indicate the availability of a specific API
 */
#define HAVE_PTY_UTILS 1	// Triggers use of unix98 PTYs for some reason

/*
 * The following are values used in compilation
 */
#ifdef __linux__
#define LOGIN_PROGRAM "/bin/login"
#else
#define LOGIN_PROGRAM "/usr/bin/login"
#endif
#define VERSION "2.15a"

#ifdef HAVE_BOUNDED
#define __attr_bounded__(__type__, __buf__, __size__) \
  __attribute__ ((__bounded__ (__type__, __buf__, __size__)))
#else
#define __attr_bounded__(__type__, __buf__, __size__)
#endif

