#include <ddcommon.h>

#ifndef HAVE_VASPRINTF
int vasprintf(char **ret, const char *fmt, va_list args)
{
	char *p;
	size_t size;
	int n;

	size = 100;
	for (;;) {
		if (!(p = (char *) malloc(size)))
			return -1;
		n = vsnprintf(p, size, fmt, args);
		/* handle glibc 2.0 and 2.1 */
		if (n > -1 && n < size) {
			*ret = p;
			return n;
		}
		if (n > -1) 
			size = n + 1;
		else
			size *= 2;
		free(p);
	}
}
#endif
