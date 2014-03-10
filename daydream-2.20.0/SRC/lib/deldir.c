#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <limits.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include <ddcommon.h>

static int rm(const char *dname, DIR *dir)
{
	char buf[PATH_MAX];
	struct dirent *dent;
	struct stat st;

	while ((dent = readdir(dir))) {
		if (!strcmp(dent->d_name, ".") || !strcmp(dent->d_name, ".."))
			continue;
		if (pathcat2(buf, sizeof buf, dname, dent->d_name) == -1)
			return -1;
		if (lstat(buf, &st) == -1)
			return -1;
		if (S_ISDIR(st.st_mode)) {
			if (deldir(buf) == -1 || rmdir(buf) == -1)
				return -1;
		} else if (unlink(buf))
			return -1;
	}

	return 0;
}

int deldir(const char *dname)
{
	DIR *dir;
	int saved_errno;

	if ((dir = opendir(dname)) == NULL)
		return -1;

	if (rm(dname, dir) == -1) {
		saved_errno = errno;
		closedir(dir);
		errno = saved_errno;
		return -1;
	}

	closedir(dir);
	return 0;
}
