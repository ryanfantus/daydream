#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <ddcommon.h>

#ifndef HAVE_SETENV
int setenv(const char *name, const char *value, int overwrite)
{
	char *buffer;
	if (getenv(name) && !overwrite)
		return 0;

	buffer = (char *) malloc(strlen(name) + strlen(value) + 2);
	if (!buffer)
		return -1;
	sprintf(buffer, "%s=%s", name, value);
	putenv(buffer);
	return 0;
}
#endif
