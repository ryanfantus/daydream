/*
   Example door for DayDream BBS 
 */

#include <ddlib.h>
#include <dd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static struct dif *d;

static int mlines;

static int moreck(void);
static void scanfile(char *);
static void genago(time_t t1, time_t t2, char *);

int main(int argc, char *argv[])
{
	char buf[1024];
	char tbuf[1024];
	char *s;

	if (argc == 1) {
		printf("This program MUST be run from DayDream BBS!\n");
		exit(1);
	}
	d = dd_initdoor(argv[1]);
	if (d == 0) {
		printf("Couldn't find socket!\n");
		exit(1);
	}
	dd_changestatus(d, "Whoffing");
	dd_getstrval(d, tbuf, USER_HANDLE);
	sprintf(buf, "\n\033[35mDD-Whof V1.0 by Antti Häyrynen\n\n"
		"[0mServing %s...\n", tbuf);
	dd_sendstring(d, buf);

	dd_getstrval(d, tbuf, DOOR_PARAMS);
	s = tbuf;
	for (; *s; s++)
		if (*s == ' ')
			*s = 0;
	if (*tbuf)
		scanfile(tbuf);
	while (1) {
		dd_sendstring(d, "\n[36mFilename to scan: [0m");
		*tbuf = 0;
		if (!(dd_prompt(d, tbuf, 35, PROMPT_FILE)))
			break;
		if (!*tbuf)
			break;
		scanfile(tbuf);
	}
	dd_close(d);
	return 0;
}

static void scanfile(char *file)
{
	char buf[1024];
	struct DD_UploadLog loge1;
	int logfd, userfd;
	struct userbase muser;
	char agot[16];
	int ulok = 0;
	int dlok = 0;

	mlines = dd_getintval(d, USER_SCREENLENGTH);
	dd_sendstring(d, "\n[0mLooking for uploader...");

	sprintf(buf, "%s/logfiles/uploadlog.dat", getenv("DAYDREAM"));
	logfd = open(buf, O_RDONLY);
	if (logfd < 0) {
		dd_sendstring(d, "\r[35m**ERROR** Can't read logfile. Tell SysOp.\n");
		mlines--;
	} else {
		while (read(logfd, &loge1, sizeof(struct DD_UploadLog))) {
			if (!strcasecmp(file, loge1.UL_FILENAME)) {
				sprintf(buf, "%s/data/userbase.dat", getenv("DAYDREAM"));
				userfd = open(buf, O_RDONLY);
				if ((userfd > -1) && (lseek(userfd, loge1.UL_SLOT * sizeof(struct userbase), SEEK_SET) > -1)) {
					dd_sendstring(d, "\r[0m                                   UPLOADER\n\n"
						      "[36mNAME               ORGANIZATION              AGO DDD:HH:MM        NODE   BAUD[0m\n"
						      "[34m-----------------------------------------------------------------------------[0m\n");
					mlines -= 5;
					read(userfd, &muser, sizeof(struct userbase));
					close(userfd);
					genago(time(0), loge1.UL_TIME, agot);
					sprintf(buf, "[32m%-18s [33m%-27s   [35m%s          [36m%2d [0m%6d\n", muser.user_handle, muser.user_organization, agot, loge1.UL_NODE, loge1.UL_BPSRATE);
					dd_sendstring(d, buf);
					ulok++;
				}
			}
		}
		close(logfd);
		if (!ulok) {
			dd_sendstring(d, "\r[35mCouldn't find uploader!        \n");
			mlines++;
		}
	}

	sprintf(buf, "%s/logfiles/downloadlog.dat", getenv("DAYDREAM"));
	logfd = open(buf, O_RDONLY);
	if (logfd < 0) {
		dd_sendstring(d, "\r[35m**ERROR** Can't read logfile. Tell SysOp.\n");
	} else {
		while (read(logfd, &loge1, sizeof(struct DD_UploadLog))) {
			if (!strcasecmp(file, loge1.UL_FILENAME)) {
				sprintf(buf, "%s/data/userbase.dat", getenv("DAYDREAM"));
				userfd = open(buf, O_RDONLY);
				if ((userfd > -1) && (lseek(userfd, loge1.UL_SLOT * sizeof(struct userbase), SEEK_SET) > -1)) {
					if (!dlok) {
						dd_sendstring(d, "\r[0m                                 DOWNLOADERS\n\n"
							      "[36mNAME               ORGANIZATION              AGO DDD:HH:MM        NODE   BAUD[0m\n"
							      "[34m-----------------------------------------------------------------------------[0m\n");
						mlines -= 4;
					}
					read(userfd, &muser, sizeof(struct userbase));
					genago(time(0), loge1.UL_TIME, agot);
					sprintf(buf, "[32m%-18s [33m%-27s   [35m%s          [36m%2d [0m%6d\n", muser.user_handle, muser.user_organization, agot, loge1.UL_NODE, loge1.UL_BPSRATE);
					dd_sendstring(d, buf);
					dlok++;
					if (!moreck())
						break;
				}
			}
		}
		close(logfd);
		if (!dlok) {
			dd_sendstring(d, "\r[35mCouldn't find downloaders!        \n");
		}
	}


}

static void genago(time_t t1, time_t t2, char *de)
{
	int days = (t1 - t2) / (60 * 60 * 24);
	int hours = ((t1 - t2) % (60 * 60 * 24)) % 24;
	int secs = ((t1 - t2) % (60 * 60 * 24)) % 60;
	sprintf(de, "%03d:%02d:%02d", days, hours, secs);

}

static int moreck(void)
{
	int hot;
	mlines--;
	if (!mlines) {
		dd_sendstring(d, "[0mMore: [35mY[36m)[0mes, [35mN[36m)[0mo, [35mC[36m)[0montinuous?: ");
		hot = dd_hotkey(d, 0);

		dd_sendstring(d, "\r                                                         \r");
		if (hot == 'N' || hot == 'n' || hot == 'q' || hot == 'Q')
			return 0;
		if (hot == 'C' || hot == 'c') {
			mlines = -1;
		} else {
			mlines = dd_getintval(d, USER_SCREENLENGTH);
		}
	}
	return 1;
}
