#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <daydream.h>
#include <ddcommon.h>
#include <symtab.h>

static int nuaskh(void);
static int nuaskr(void);
static void questionnaire(void);

int CreateNewAccount(void)
{
	char passwd[20];
	char *media;
	struct userbase tmpuser;
	int hotti, tmp;
	char buf[512];

	MD_CTX context;

	changenodestatus("Creating an account");

	if (*maincfg.CFG_NEWUSERPW) {
		char b[80];
		int i;

		TypeFile("newuserpassword", TYPE_MAKE);
		for (i = 2; i; i--) {
			*b = 0;
			DDPut(sd[nupstr]);
			if (!(Prompt(b, 16, PROMPT_SECRET)))
				return 0;
			if (!strcasecmp(b, maincfg.CFG_NEWUSERPW))
				break;
		}
		if (i == 0)
			return 0;
	}
	media = (char *) &user;
	for (hotti = 0; hotti < 300; hotti++)
		media[hotti] = 0;

	TypeFile("newuser", TYPE_MAKE | TYPE_WARN);
	DDPut(sd[newsure1str]);

	hotti = HotKey(HOT_YESNO);
	if (hotti == 0 || hotti == 2)
		return 0;

	if (maincfg.CFG_FLAGS & (1L << 7)) {
		if (!nuaskh())
			return 0;
		if (!nuaskr())
			return 0;
	} else {
		if (!nuaskr())
			return 0;
		if (!nuaskh())
			return 0;
	}

	TypeFile("reg_organization", TYPE_MAKE);
	DDPut(sd[neworgstr]);
	if (!(Prompt(user.user_organization, 25, 0)))
		return 0;

	TypeFile("reg_zipcode", TYPE_MAKE);
	DDPut(sd[newzipstr]);
	if (!(Prompt(user.user_zipcity, 20, 0)))
		return 0;

	TypeFile("reg_voicephone", TYPE_MAKE);
	DDPut(sd[newphonestr]);
	if (!(Prompt(user.user_voicephone, 20, 0)))
		return 0;

	for (;;) {
		TypeFile("reg_password", TYPE_MAKE);
		DDPut(sd[newpasswdstr]);
		*passwd = 0;
		if (!(Prompt(passwd, 15, PROMPT_SECRET)))
			return 0;
		if (!passwd[0])
			continue;
		DDPut(sd[retrypasswdstr]);
		*buf = 0;
		if (!(Prompt(buf, 15, PROMPT_SECRET)))
			return 0;
		if (strcasecmp(buf, passwd)) {
			DDPut(sd[nomatchretrystr]);
			continue;
		}
		break;
	}
	
	strupr(passwd);

	MDInit(&context);
	MD5Update(&context, (unsigned char *) passwd, strlen(passwd));
	MDFinal(user.user_password, &context);

	TypeFile("reg_computer", TYPE_MAKE);
	DDPut(sd[newcpustr]);
	if (!(Prompt(user.user_computermodel, 20, 0)))
		return 0;

	TypeFile("reg_screenlength", TYPE_MAKE);
	for (;;) {
		passwd[0] = 0;
		DDPut(sd[newslstr]);
		if (!(Prompt(passwd, 3, 0)))
			return 0;

		if (passwd[0] == 0)
			continue;
		if (passwd[0] == 'T' || passwd[0] == 't') {
			testscreenl();
			continue;
		}
		user.user_screenlength = atoi(passwd);
		if (user.user_screenlength < 10) {
			DDPut(sd[newminslstr]);
			continue;
		}
		break;
	}
	snprintf(user.user_signature, sizeof user.user_signature,
		"-%s", user.user_handle);
	setpreset(maincfg.CFG_NEWUSERPRESETID, &user);

	hotti = 0;

	questionnaire();

	DDPut(sd[newsavingstr]);
	
	/* FIXME: this really does not belong here */
	for (tmp = 0;; tmp++) {
		if (getubentbyid(tmp, &tmpuser) == -1)
			break;
		if ((tmpuser.user_toggles & UBENT_STAT_MASK) == 
			UBENT_STAT_DELETED)
			break;
	}
	user.user_account_id = tmp;
	
	if (!user.user_account_id)
		user.user_securitylevel = 255;
	
	user.user_toggles |= maincfg.CFG_DEFAULTS;
	user.user_joinconference = 1;

	saveuserbase(&user);
		
	DDPut(sd[newsaveokstr]);

	snprintf(buf, sizeof buf, "%s/users/%d", 
		origdir, user.user_account_id);
	deldir(buf);

	return 1;
}

void testscreenl(void)
{
	int hotti;

	for (hotti = 66; hotti > 2; hotti--) 
		ddprintf("%d\n", hotti);
	DDPut(sd[newslhelpstr]);
}

int setpreset(int newpreset, struct userbase *suser)
{
	struct DayDream_AccessPreset *currpre;
	int i = 1;

	currpre = presets;
	while (currpre->ACCESS_SECLEVEL != 0) {
		if (currpre->ACCESS_PRESETID == newpreset) {
			suser->user_securitylevel = currpre->ACCESS_SECLEVEL;
			suser->user_freedlfiles = currpre->ACCESS_FREEFILES;
			suser->user_freedlbytes = currpre->ACCESS_FREEBYTES;
			switch (currpre->ACCESS_STATUS) {
			case 0:
				suser->user_toggles &= ~(1L << 30);
				suser->user_toggles &= ~(1L << 31);
				break;
			case 1:
				suser->user_toggles |= (1L << 30);
				suser->user_toggles &= ~(1L << 31);
				break;
			case 2:
				suser->user_toggles &= ~(1L << 30);
				suser->user_toggles |= (1L << 31);
				break;
			case 3:
				suser->user_toggles |= (1L << 30);
				suser->user_toggles |= (1L << 31);
				break;
			}
			return 1;
		} else {
			currpre = presets + i;
			i++;
		}
	}
	return 0;
}

static void questionnaire(void)
{
	FILE *questfd;
	FILE *ansfd;
	char qbuffer[1000];
	char abuffer[100];
	char *s, *t;
	time_t fallos;
	int hins;

      askag:

	TypeFile("questionnaire", TYPE_MAKE);

	questfd = fopen("questionnaire/questions", "r");
	if (questfd == 0) {
		DDPut(sd[newqerrorstr]);
		return;
	}
	snprintf(qbuffer, sizeof qbuffer, "%s/quest.%d", 
		DDTMP, user.user_account_id);

	ansfd = fopen(qbuffer, "w+");

	if (ansfd == 0) {
		fclose(questfd);
		return;
	}
	fallos = time(0);
	snprintf(qbuffer, sizeof qbuffer, "\n==========================================================================\nAnswers from %s (%s) %s\n", user.user_realname, user.user_handle, ctime(&fallos));
	fputs(qbuffer, ansfd);

	while (fgets(qbuffer, 1000, questfd)) {
		s = qbuffer;
		t = s;
		while (*t) {
			if (*t == '~') {
				*t = 0;
				DDPut(s);
				stripansi(s);
				fputs(s, ansfd);
				t++;
				s = t;

				abuffer[0] = 0;
				if (!(Prompt(abuffer, 80, 0))) {
					fclose(ansfd);
					fclose(questfd);
				}
				fputs(abuffer, ansfd);
				fputs("\n", ansfd);
				break;
			} else
				t++;
		}
		if (*t == 0) {
			DDPut(s);
			stripansi(s);
			fputs(s, ansfd);
		}
	}
	fclose(ansfd);
	fclose(questfd);

	DDPut(sd[newverifystr]);
	hins = HotKey(HOT_YESNO);
	if (hins == 0)
		return;

	if (hins == 2)
		goto askag;

	if (hins == 1) {
		snprintf(qbuffer, sizeof qbuffer, "%s/quest.%d", 
			DDTMP, user.user_account_id);
		/* FIXME: quest == NULL */
		questfd = fopen(qbuffer, "r");

		ansfd = fopen("questionnaire/answers", "a+");
		if (ansfd == 0) {
			fclose(questfd);
			return;
		}
		while (fgets(qbuffer, 1000, questfd)) {
			fputs(qbuffer, ansfd);
		}
		fclose(questfd);
		fclose(ansfd);
	}
}

static int nuaskr(void)
{
	for (;;) {
		TypeFile("reg_realname", TYPE_MAKE);
		DDPut(sd[newrealstr]);
		if (!(Prompt(user.user_realname, 25, 0)))
			return 0;
		removespaces(user.user_realname);
		if (!user.user_realname[0])
			return 0;
		if (findusername(user.user_realname) != -1) {
			DDPut(sd[newalreadystr]);
			continue;
		}
		break;
	}
	return 1;
}

static int nuaskh(void)
{
	for (;;) {
		TypeFile("reg_handle", TYPE_MAKE);
		DDPut(sd[newhandlestr]);
		if (!(Prompt(user.user_handle, 25, 0)))
			return 0;
		removespaces(user.user_handle);
		if (!user.user_handle[0])
			return 0;
		if (findusername(user.user_handle) != -1) {
			DDPut(sd[newalreadystr]);
			continue;
		}
		break;
	}
	return 1;
}
