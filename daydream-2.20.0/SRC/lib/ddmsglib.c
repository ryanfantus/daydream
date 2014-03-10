#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

#include "ddcommon.h"
#include "ddmsglib.h"

int ddmsg_open_base(char* conf, int msgbase_num, int flags, int mode) {
	char buf[128];
	int fd;

	snprintf(buf, sizeof buf, "%s/messages/base%3.3d/msgbase.dat", conf, msgbase_num);

	if (mode != 0) {
		fd = open(buf, flags, mode);
	} else {
		fd = open(buf, flags);
	}

	if(fd == -1) {
		return -1;
        }

	if(lock_file(fd, flags) == -1) {
		return -1;
	}

	if(mode != 0)
		fsetperm(fd, mode);
	
	return fd;
}

int ddmsg_open_msg(char* conf, int msgbase_num, int msgnum, int flags, int mode) {
	char buf[128];
	int fd;

	snprintf(buf, sizeof buf, "%s/messages/base%3.3d/msg%5.5d", conf, msgbase_num, msgnum);

	if(mode != 0) {
		fd = open(buf, flags, mode);
	} else {
		fd = open(buf, flags);
	}
	
	if(lock_file(fd, flags) == -1) {
		return -1;
	}

	if(mode != 0)
		fsetperm(fd, mode);

	return fd;
}	

int ddmsg_close_base(int fd) {
	unlock_file(fd);

	return close(fd);
}

int ddmsg_close_msg(int fd) {
	unlock_file(fd);

	return close(fd);
}

int ddmsg_getptrs(char* conf, int msgbase_num, struct DayDream_MsgPointers* ptrs) {
	int msgfd;

	char buf[128];

	snprintf(buf, sizeof buf, "%s/messages/base%3.3d/msgbase.ptr", conf, msgbase_num);

	if ((msgfd = open(buf, O_RDONLY)) < 0) {
		ptrs->msp_high = 0;
		ptrs->msp_low = 0;
	}
	else {
		lock_file(msgfd, O_RDONLY);
		read(msgfd, ptrs, sizeof(struct DayDream_MsgPointers));
		unlock_file(msgfd);
		close(msgfd);
	}

	return 0;
}
int ddmsg_setptrs(char* conf, int msgbase_num, struct DayDream_MsgPointers* ptrs) {
	int msgfd;
	char buf[128];

	snprintf(buf, sizeof buf, "%s/messages/base%3.3d/msgbase.ptr", conf, msgbase_num);

	if ((msgfd = open(buf, O_RDWR | O_CREAT, 0666)) == -1) {
		return -1;
	}

	lock_file(msgfd, O_RDWR);

	fsetperm(msgfd, 0666);
	safe_write(msgfd, ptrs, sizeof(struct DayDream_MsgPointers));

	unlock_and_close(msgfd);
	return 0;

}
int ddmsg_getfidounique(char* origdir) {
	static int i=0;
	char buf[128];
	int fn;

	snprintf(buf, sizeof buf, "%s/data/fidocnt.dat", origdir);

	fn = open(buf, O_CREAT | O_RDWR, 0666);
	if (fn == -1)
		return 0;

	lock_file(fn, O_RDWR);
	
	fsetperm(fn, 0666);
	read(fn, &i, sizeof(int));
	i++;
	lseek(fn, 0, SEEK_SET);
	safe_write(fn, &i, sizeof(int));
	unlock_file(fn);
	close(fn);

	return i;
}

int ddmsg_delete_msg(char* conf, int msgbase_num, int msgnum) {
	
	struct DayDream_Message d;
	int fd;
	int found = 0;
	char buf[128];
	
	fd = ddmsg_open_base(conf, msgbase_num, O_RDONLY, 0);

	if(fd < 0)
		return 0;

	while(read(fd, &d, sizeof(struct DayDream_Message))) {
		if(d.MSG_NUMBER == msgnum) {
			found = 1;
			break;
		}
	}
	
	ddmsg_close_base(fd);

	if(!found)
		return 0;

	snprintf(buf, sizeof buf, "%s/messages/base%3.3d/msg%5.5d", conf, msgbase_num, msgnum);
	unlink(buf);
	if (d.MSG_ATTACH[0] == 1) {
		snprintf(buf, sizeof buf, "%s/messages/base%3.3d/fa%5.5d", conf, msgbase_num, msgnum); 
		deldir(buf);
		unlink(buf);
		snprintf(buf, sizeof buf, "%s/messages/base%3.3d/msf%5.5d", conf, msgbase_num, msgnum);
		unlink(buf);
	}

	return 1;
}

int ddmsg_rename_msg(char* conf, int msgbase_num, int old_num, int new_num) {
	FILE* fh_r, *fh_w;
	char old_fn[128], new_fn[128];
	char buf[256];
	snprintf(old_fn, sizeof old_fn, "%s/messages/base%3.3d/msg%5.5d", conf, msgbase_num, old_num);
	snprintf(new_fn, sizeof new_fn, "%s/messages/base%3.3d/msg%5.5d", conf, msgbase_num, new_num);

	fh_r = fopen(old_fn, "r");
	lock_file(fileno(fh_r), O_RDONLY);
	fh_w = fopen(new_fn, "w");
	lock_file(fileno(fh_w), O_WRONLY);

	while(fgets(buf, 256, fh_r)) {
		fputs(buf, fh_w);
	}

	unlock_file(fileno(fh_r));
	fclose(fh_r);
	unlock_file(fileno(fh_w));
	fclose(fh_w);

	return 0;
}

