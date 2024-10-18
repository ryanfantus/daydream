#include <time.h>
#include <fcntl.h>
#include <unistd.h>

#include <ddutmpx.h>

static int utmp_fd = -1;

#ifndef HAVE_SETUTXENT
void setutxent(void)
{
#ifdef HAVE_SETUTENT
	setutent();
#else
	if (utmp_fd != -1)
		endutxent();
	utmp_fd = open(_PATH_UTMP, O_RDONLY);
#endif
}
#endif
#ifndef HAVE_GETUTXENT
UTMPX *getutxent(void)
{
#ifdef HAVE_GETUTENT
	return getutent();
#else
	static struct utmp utmp;
	if (utmp_fd == -1)
		return NULL;
	if (read(utmp_fd, &utmp, sizeof(struct utmp)) != sizeof(struct utmp))
		return NULL;
	return &utmp;
#endif
}
#endif
#ifndef HAVE_ENDUTXENT
void endutxent(void)
{
#ifdef HAVE_ENDUTENT
	endutent();
#else
	close(utmp_fd);
	utmp_fd = -1;
#endif
}
#endif
