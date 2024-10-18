#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include <daydream.h>
#include <ddcommon.h>
#include <utility.h>

struct MsgScanDat {
	int ms_seekp;
	int ms_number;
	char ms_sender[26];
	char ms_subject[60];
	int ms_status;
};

static int sfnmjoin(int confn)
{
	if (!isanybasestagged(confn))
		return 0;
	if (!joinconf(confn, JC_QUICK | JC_SHUTUP | JC_NOUPDATE))
		return 0;
	return 1;
}

int scanfornewmail(void)
{
	int oldconf;
	int datafd;
	char mailbuf[300];
	struct MsgScanDat dat;
	struct iterator *iterator;
	conference_t *mc;

	changenodestatus("Scanning for new mail");

	snprintf(mailbuf, sizeof mailbuf, "%s/daydream%d.mail", DDTMP, node);
	unlink(mailbuf);
	datafd = open(mailbuf, O_RDWR | O_CREAT, 0666);
	if (datafd == -1) {
		DDPut("Can't open datafile! Whip root!\n\n");
		return 0;
	}
	fsetperm(datafd, 0666);
	close(datafd);

	datafd = -1;

	oldconf = user.user_joinconference;

	DDPut(sd[msheadstr]);

	iterator = conference_iterator();
	while ((mc = (conference_t *) iterator_next(iterator))) {
		msgbase_t *mbase;
		int newcnt;
		int delcnt = 0;
		int ddir = 0;
		int seekp, i;
		struct stat st;
		struct DayDream_Message msg;

		newcnt = 0;
		if (!sfnmjoin(mc->conf.CONF_NUMBER)) 
			continue;
		
		if (!conference()->conf.CONF_MSGBASES)
			continue;
		
		ddprintf(sd[ms1str], conference()->conf.CONF_NAME);
		
		for (i = 0; i < conference()->conf.CONF_MSGBASES; i++) {
			int basefd;

			mbase = conference()->msgbases[i];
			
			if (!isbasetagged(conference()->conf.CONF_NUMBER, mbase->MSGBASE_NUMBER))
				continue;
			
			if (delcnt)
				multibackspace(delcnt);
			changemsgbase(mbase->MSGBASE_NUMBER, MC_QUICK | MC_NOSTAT);
			ddprintf(sd[ms2str], current_msgbase->MSGBASE_NAME);
			delcnt = 22;
			if (lrp > lsp)
				lsp = lrp;
			
			if (lsp >= highest) {
				lsp = highest;
			} else {
				basefd = ddmsg_open_base(conference()->conf.CONF_PATH, current_msgbase->MSGBASE_NUMBER, O_RDONLY, 0);

				if (basefd != -1) {
					fstat(basefd, &st);
					seekp = st.st_size - (highest - lsp + 2) * sizeof(struct DayDream_Message);
					if (seekp < 0)
						seekp = 0;
					lseek(basefd, seekp, SEEK_SET);
					while (read(basefd, &msg, sizeof(struct DayDream_Message))) {
						if (ddir) {
							if (delcnt == 22) {
								ddir = 0;
								DDPut("*");
								delcnt++;
							} else {
								DDPut("[D [D");
								delcnt--;
							}
						} else {
							if (delcnt == 30) {
								ddir = 1;
								DDPut("[D [D");
								delcnt--;
							} else {
								DDPut("*");
								delcnt++;
							}
						}
						if (msg.MSG_NUMBER > lsp) {
							if (msg.MSG_RECEIVER[0] == -1 || (!strcasecmp(msg.MSG_RECEIVER, user.user_handle)) || (!strcasecmp(msg.MSG_RECEIVER, user.user_realname))) {
								int msgfd;

								msgfd = ddmsg_open_msg(conference()->conf.CONF_PATH, current_msgbase->MSGBASE_NUMBER, msg.MSG_NUMBER, O_RDONLY, 0);
								if (msgfd != -1) {
									ddmsg_close_msg(msgfd);
									newcnt++;
									if (datafd == -1) {
										snprintf(mailbuf, sizeof mailbuf, "%s/daydream%d.mail", DDTMP, node);
										datafd = open(mailbuf, O_RDWR | O_CREAT | O_TRUNC, 0666);
										fsetperm(datafd, 0666);
									}
									dat.ms_seekp = seekp;
									dat.ms_number = msg.MSG_NUMBER;
									strlcpy(dat.ms_sender, msg.MSG_AUTHOR, sizeof dat.ms_sender);
									strlcpy(dat.ms_subject, msg.MSG_SUBJECT, sizeof dat.ms_subject);
									dat.ms_status = msg.MSG_FLAGS;
									safe_write(datafd, &dat, sizeof(struct MsgScanDat));
								}
							} else {
								if (!(msg.MSG_FLAGS & (1L << 0)))
									newcnt++;
							}
						}
						seekp += sizeof(struct DayDream_Message);
						
					}
					ddmsg_close_base(basefd);
					if (datafd != -1) {
						int screenl = user.user_screenlength;
						int hot;
						DDPut(sd[mslheadstr]);
						lseek(datafd, 0, SEEK_SET);
						
						while (read(datafd, &dat, sizeof(struct MsgScanDat))) {
							const char *sta;
							
							if (dat.ms_status & (1L << 0))
								sta = "Private";
							else
								sta = "Public";
							
							screenl--;
							ddprintf(sd[mslliststr], dat.ms_number, dat.ms_sender, dat.ms_subject, sta);
							if (screenl == 0) {
								
								DDPut(sd[morepromptstr]);
								hot = HotKey(0);
								DDPut("\r                                                         \r");
								if (hot == 'N' || hot == 'n')
									break;
								if (hot == 'C' || hot == 'c') {
									screenl = -1;
								} else {
									screenl = user.user_screenlength;
								}
							}
						}
						DDPut(sd[mslpromptstr]);
						hot = HotKey(0);
						if (hot == 'Y' || hot == 'y' || hot == 13 || hot == 10) {
							lseek(datafd, 0, SEEK_SET);
							while (read(datafd, &dat, sizeof(struct MsgScanDat))) {
								if (dat.ms_number >= lsp) {
									if ((readmessages(dat.ms_seekp, dat.ms_number, NULL)) == 2)
										break;
								}
							}
						} else if (hot == 'M' || hot == 'm') {
							lsp = highest;
						}
						DDPut(sd[msheadstr]);
						ddprintf(sd[ms3str], conference()->conf.CONF_NAME, current_msgbase->MSGBASE_NAME);
						delcnt = 22;
						close(datafd);
						datafd = -1;
						ddir = 0;
						
					} else
						lsp = highest;
				}
			}
		}

		multibackspace(delcnt - 22);
		if (newcnt) 
			ddprintf(sd[msnewstr], newcnt);
		else 
			DDPut(sd[msnonewstr]);
	}
	iterator_discard(iterator);
	
	DDPut(sd[mstailstr]);
	
	joinconf(oldconf, JC_QUICK | JC_SHUTUP | JC_NOUPDATE);
	return 1;
}

void multibackspace(int bscnt)
{
	char mbs[500];
	int kelas;

	if (bscnt < 1)
		return;

	mbs[0] = 0;
	for (kelas = bscnt; kelas; kelas--) {
		strlcat(mbs, "[D [D", sizeof mbs);
	}
	DDPut(mbs);
}
