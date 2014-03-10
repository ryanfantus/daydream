#include <unistd.h>
#include <sys/time.h>

#include <zm.h>

double timing (int reset)
{
	static double starttime, stoptime;
	double yet;
	struct timeval tv;
	struct timezone tz;

	tz.tz_dsttime = 0;
	gettimeofday(&tv, &tz);
	yet = tv.tv_sec + tv.tv_usec / 1000000.0;

	if (reset) {
		starttime = yet;
		return starttime;
	} else {
		stoptime = yet;
		return stoptime - starttime;
	}
}
