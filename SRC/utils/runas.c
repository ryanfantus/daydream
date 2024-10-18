#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <dirent.h>
#include <syslog.h>

#include <ddcommon.h>

static void usage(void)
{
	fputs("Usage: runas [-v] command [options-and-arguments]\n", stderr);
	fputs("\n", stderr);
}

static int run_archiver(const char *path, char * const argv[])
{
	pid_t child;
	int status, retcode;

	if ((child = fork()) == -1)
		return -1;
	if (child != 0) {
		for (;;) {
			retcode = waitpid(child, &status, 0);
			if (retcode == -1)
				return 1;
			else if (retcode != 0)
				return WEXITSTATUS(status);
			usleep(10000);
		}
	} else {
		execv(path, argv);
		syslog(LOG_ERR, "cannot execute %s (%d)", path, errno);
		fprintf(stderr, "cannot execute %s (%d)\r\n", path, errno);
		exit(1);
	}

	return 0;
}	

static int chown_tree(const char *pathname)
{
	char fn[4096];
	struct stat st;
	DIR *dir;
	struct dirent *dent;

	if ((dir = opendir(pathname)) == NULL)
		return -1;
	while ((dent = readdir(dir))) {
		if (!strcmp(dent->d_name, ".") || !strcmp(dent->d_name, ".."))
			continue;
		if (strlcpy(fn, pathname, sizeof(fn)) >= sizeof(fn) ||
			strlcat(fn, "/", sizeof(fn)) >= sizeof(fn) ||
			strlcat(fn, dent->d_name, sizeof(fn)) >= sizeof(fn))
			goto error;
		if (stat(fn, &st) == -1) 
			goto error;
		/* we don't like symbolic links */
		if (st.st_mode & S_IFLNK)
			continue;

		st.st_mode &= ~S_IRWXG;
		st.st_mode |= (st.st_mode & S_IRWXU) >> 3;

		if (seteuid(st.st_uid) == -1 || chmod(fn, st.st_mode) == -1) 
			return -1;
		if (st.st_mode & S_IFDIR)
			if (chown_tree(fn) == -1)
				goto error;
	}
	return closedir(dir);

error:
	closedir(dir);
	return -1;
}
		
int main(int argc, char *argv[])
{
	/* FIXME: facility should be configurable */
	openlog("runas", LOG_PID, LOG_LOCAL2);

	if (argc < 2) {
		usage();
		exit(1);
	}

	if (!strcmp(argv[1], "-v")) {
		if (argc < 3) {
			usage();
			exit(1);
		}
		if (run_archiver(argv[2], argv + 2) == -1) {
			fputs("archive viewing failed\n", stderr);
			exit(1);
		}
	} else {
		if (run_archiver(argv[1], argv + 1) == -1 || 
			chown_tree(".") == -1) {
			fputs("archive check failed\n", stderr);
			exit(1);
		}
	}

	return 0;	
}
