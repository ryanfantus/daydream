#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include <daydream.h>
#include <menucmd.h>

#define require_message_bases() {			\
	if (!conference()->conf.CONF_MSGBASES) {	\
		DDPut(sd[msgreadnmb]);			\
		return 1;				\
	}						\
}

void dpause(void)
{
	DDPut(sd[pause2str]);
	HotKey(0);
	DDPut("\n");
}

int primitive_docmd(int command, char *params)
{	
	switch (command) {
	case CMD_TEXT_SEARCH:
		return textsearch(params);
	case CMD_VER:
		versioninfo();
		return 1;
	case CMD_EXPERT_MODE:
		if (!isaccess(SECB_EXPERTMODE, access1))
			return 1;

		if (user.user_toggles & (1L << 4)) {
			DDPut(sd[expertoffstr]);
			user.user_toggles &= ~(1L << 4);
		} else {
			DDPut(sd[expertonstr]);
			user.user_toggles |= (1L << 4);
		}
		return 1;
	case CMD_LOGOFF:
		return logoff(params);
	case CMD_CHANGE_INFO:
		if (isaccess(SECB_CHANGEINFO, access1))
			edituser();
		return 1;
	case CMD_FILE_SCAN:
		if (isaccess(SECB_FILESCAN, access1))
			filelist(1, params);
		return 1;
	case CMD_DOWNLOAD:
		if (isaccess(SECB_DOWNLOAD, access1))
			if (download(params) == 2)
				return 0;
		return 1;
	case CMD_TAG_EDITOR:		
		if (isaccess(SECB_TAGEDITOR, access1))
			taged(params);
		return 1;
	case CMD_BULLETINS:
		if (isaccess(SECB_BULLETINS, access1))
			bulletins(params);
		return 1;
	case CMD_NEW_FILES:			
		if (isaccess(SECB_NEWFILES, access1))
			filelist(2, params);
		return 1;
	case CMD_ZIPPY_SEARCH:
		if (isaccess(SECB_ZIPPYSEARCH, access1))
			filelist(3, params);
		return 1;
	case CMD_TIME: {
		time_t aika;
		char *aikas;

		if (!isaccess(SECB_VIEWTIME, access1))
			return 1;
		aika = time(0);

		aikas = ctime(&aika);
		aikas[24] = 0;
		ddprintf(sd[ltimestr], aikas);
		return 1;
	}
	case CMD_STATS:
		if (isaccess(SECB_USERSTATS, access1))
			userstats();
		return 1;
	case CMD_ENTER_MSG:
		require_message_bases();
		if (isaccess(SECB_ENTERMSG, access1))
			entermsg(0, 0, params);
		return 1;
	case CMD_COMMENT:
		require_message_bases();
		if (isaccess(SECB_COMMENT, access1))
			comment();
		return 1;
	case CMD_READ_MSGS:
		require_message_bases();
		DDPut("\n");
		if (isaccess(SECB_READMSG, access1))
			readmessages(-1, 0, NULL);
		return 1;
	case CMD_GLOBAL_READ:
		if (isaccess(SECB_READMSG, access1))
			globalread();
		return 1;
	case CMD_JOIN_CONF:
		if (isaccess(SECB_JOINCONF, access1))
			joinconfmenu(params);
		return 1;
	case CMD_CHANGE_MSGBASE:
		require_message_bases();
		if (isaccess(SECB_CHANGEMSGAREA, access1))
			cmbmenu(params);
		return 1;
	case CMD_LOCAL_UPLOAD:
		localupload();
		return 1;
	case CMD_UPLOAD:
		if (isaccess(SECB_UPLOAD, access1))
			if (upload(0) == 2)
				return 0;
		return 1;
	case CMD_UPLOAD_RZ:
		if (isaccess(SECB_UPLOAD, access1))
			if (upload(3) == 2)
				return 0;
		return 1;
	case CMD_VIEW_FILE:
		if (isaccess(SECB_VIEWFILE, access2))
			viewfile(params);
		return 1;
	case CMD_SCAN_MAIL:
		scanfornewmail();
		return 1;
	case CMD_GLOBAL_FSCAN:
		if (isaccess(SECB_NEWFILES, access1))
			globalnewscan();
		return 1;
	case CMD_TAG_MSGBASES:
		require_message_bases();
		if (isaccess(SECB_SELECTMSGBASES, access1))
			tagmessageareas();
		return 1;
	case CMD_TAG_CONFS:
		if (isaccess(SECB_SELECTFILECONFS, access1))
			tagconfs();
		return 1;
	case CMD_NEXT_CONF:
		nextconf();
		return 1;
	case CMD_PREV_CONF:
		prevconf();
		return 1;
	case CMD_NEXT_BASE:
		require_message_bases();
		nextbase();
		return 1;
	case CMD_PREV_BASE:
		require_message_bases();
		prevbase();
		return 1;
	case CMD_WHO:
		if (isaccess(SECB_WHO, access1))
			who();
		return 1;
	case CMD_HELP:
		TypeFile("commands", TYPE_MAKE | TYPE_WARN);
		return 2;
	case CMD_USERLIST:
		if (isaccess(SECB_USERLIST, access1))
			userlist(params);
		return 1;
	case CMD_MODE:
		getdisplaymode(params, 1);
		return 1;
	case CMD_PAGE:
		if (isaccess(SECB_PAGE, access1))
			pagesysop(params);
		return 1;
	case CMD_OLM:
		if (isaccess(SECB_OLM, access2))
			olmsg(params);
		return 1;
	case CMD_MOVE:
		if (isaccess(SECB_MOVEFILE, access1))
			movefile(params, 0);
		return 1;
	case CMD_COPY:
		if (isaccess(SECB_MOVEFILE, access1))
			movefile(params, 2);
		return 1;
	case CMD_LINK:
		if (isaccess(SECB_MOVEFILE, access1))
			movefile(params, 1);
		return 1;
	case CMD_SYSOP_DOWNLOAD:
		if (isaccess(SECB_SYSOPDL, access1))
			sysopdownload(params);
		return 1;
	case CMD_USERED:
		if (isaccess(SECB_USERED, access1)) {
			if (maincfg.CFG_OLUSEREDPW[0]) {
				char pwbuf[20];
				pwbuf[0] = 0;
				DDPut(sd[uepwstr]);
				if (!(Prompt(pwbuf, 15, PROMPT_SECRET)))
					return 0;
				if (strcasecmp(pwbuf, maincfg.CFG_OLUSEREDPW))
					return 1;
			}
			usered();
		}
		return 1;
	}
	
	return 3;
}
