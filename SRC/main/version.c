#include <sys/utsname.h>

#include <daydream.h>

void versioninfo(void)
{
	struct utsname pilu;
	uname(&pilu);
	
	ddprintf("\033[2J\033[H\033[0;33mDayDream/UNiX BBS %s\n"
		"\033[32mCopyright (C) 1996, 1997 Antti Häyrynen\n"
		"\033[32mCopyright (C) 1998, 1999, 2000, 2001, 2002, 2003, 2004 DayDream Development Team\n"
                "\033[32mCopyright (C) 2008, 2009, 2010 Bo Simonsen\n"
		"\033[32mCopyright (C) 2010, 2011 Ryan Fantus\n"
		"\033[0mRunning on %s/%s %s\n\n",
		versionstring, pilu.sysname, pilu.machine, pilu.release);
}
