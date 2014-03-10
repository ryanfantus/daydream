#include <ddcommon.h>

#ifndef HAVE_UNSETENV
void unsetenv(const char *name)
{
	setenv(name, "", 1);
}
#endif
