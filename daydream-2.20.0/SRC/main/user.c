#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include <daydream.h>
#include <ddcommon.h>

static int userbase_open(int mode)
{
	char filename[PATH_MAX];

	if (ssnprintf(filename, "%s/data/userbase.dat", origdir))
		return -1;
   	return lock_and_open(filename, mode | O_CREAT, 0666);
}

int getubentbyid(int id, struct userbase *user)
{
	int fd;

	if ((fd = userbase_open(O_RDONLY)) == -1)
		return -1;
	if (pread(fd, user, sizeof(struct userbase), 
		sizeof(struct userbase) * id) != sizeof(struct userbase)) {
		unlock_and_close(fd);
		return -1;
	}
	return unlock_and_close(fd);
}

int getubentbyname(const char *name, struct userbase *user)
{
	int fd;

	if ((fd = userbase_open(O_RDONLY)) == -1)
		return -1;
	while (read(fd, user, sizeof(struct userbase)) == 
		sizeof(struct userbase)) {
		if ((user->user_toggles & UBENT_STAT_MASK) == 
			UBENT_STAT_DELETED)
			continue;
		if (!strncasecmp(user->user_realname, name, 25) ||
			!strncasecmp(user->user_handle, name, 25)) 
			return unlock_and_close(fd);
	}
	unlock_and_close(fd);
	return -1;
}	

int writeubent(const struct userbase *user)
{
	int fd;

	if ((fd = userbase_open(O_WRONLY)) == -1)
		return -1;
	if (lseek(fd, sizeof(struct userbase) * user->user_account_id,
		SEEK_SET) == -1) {
		unlock_and_close(fd);
		return -1;
	}
	if (safe_write(fd, user, sizeof(struct userbase)) == -1) {
		unlock_and_close(fd);
		return -1;
	}
	return unlock_and_close(fd);
}
