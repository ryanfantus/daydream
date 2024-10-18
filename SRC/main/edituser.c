#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <syslog.h>

#include <daydream.h>
#include <ddcommon.h>
#include <symtab.h>

static void switches(void);

/* FIXME! buffer overflows? */
static int handle_choice(const char *askbuf)
{
	char lesbabuf[30];
	struct userbase muser = user;
	int leps;
		
	if (!(strcasecmp(askbuf, "1"))) {
		for (;;) {
			if (!isaccess(SECB_REALNAME, access2))
				break;
			DDPut(sd[eu1str]);
			strlcpy(lesbabuf, user.user_realname, sizeof lesbabuf);
			if (!(Prompt(lesbabuf, 25, 0)))
				return 1;
			removespaces(lesbabuf);
			if (strcasecmp(lesbabuf, user.user_realname)) {
				leps = findusername(lesbabuf);
				if (leps == user.user_account_id || leps == -1) {
					if (lesbabuf[0])
						strlcpy(user.user_realname, lesbabuf, sizeof user.user_realname);
				} else {
					DDPut(sd[newalreadystr]);
					continue;
				}
			}
			break;
		}
	} else if (!(strcasecmp(askbuf, "2"))) {
		for (;;) {
			if (!isaccess(SECB_HANDLE, access2))
				break;
			DDPut(sd[eu2str]);
			strlcpy(lesbabuf, user.user_handle, sizeof lesbabuf);
			if (!(Prompt(lesbabuf, 25, 0)))
				return 1;
			removespaces(lesbabuf);
			if (strcasecmp(lesbabuf, user.user_handle)) {
				leps = findusername(lesbabuf);
				if (leps == user.user_account_id || leps == -1) {
					if (lesbabuf[0])
						strlcpy(user.user_handle, lesbabuf, sizeof user.user_handle);
				} else {
					DDPut(sd[newalreadystr]);
					continue;
				}
			}
			break;
		}
	} else if (!(strcasecmp(askbuf, "3"))) {
		DDPut(sd[eu3str]);
		if (!(Prompt(user.user_organization, 25, 0)))
			return 1;
	} else if (!(strcasecmp(askbuf, "4"))) {
		DDPut(sd[eu4str]);
		if (!(Prompt(user.user_zipcity, 20, 0)))
			return 1;
	} else if (!(strcasecmp(askbuf, "5"))) {
		DDPut(sd[eu5str]);
		if (!(Prompt(user.user_voicephone, 20, 0)))
			return 1;
	} else if (!(strcasecmp(askbuf, "6"))) {
		MD_CTX context;
		char verifypw[32];
		DDPut(sd[eu6str]);
		lesbabuf[0] = 0;
		if (!(Prompt(lesbabuf, 15, PROMPT_SECRET)))
			return 1;
		if (lesbabuf[0] == 0)
			return 0;
		*verifypw = 0;
		DDPut(sd[euverifypwstr]);
		if (!(Prompt(verifypw, 15, PROMPT_SECRET)))
			return 1;
		if (strcasecmp(lesbabuf, verifypw)) {
			DDPut(sd[eunomatchstr]);
			return 0;
		}
		strupr(lesbabuf);
		MDInit(&context);
		MDUpdate(&context, (unsigned char *) lesbabuf, 
			strlen(lesbabuf));
		MDFinal(user.user_password, &context);
	} else if (!strcasecmp(askbuf, "7")) {
		for (;;) {
			int fallos;
			
			DDPut(sd[eu7str]);
			lesbabuf[0] = 0;
			if (!(Prompt(lesbabuf, 3, 0)))
				return 1;
			if (lesbabuf[0] == 't' || lesbabuf[0] == 'T') {
				testscreenl();
				continue;
			}
			fallos = atoi(lesbabuf);
			if (fallos < 10) {
				DDPut(sd[newminslstr]);
				continue;
			}
			user.user_screenlength = fallos;
			break;
		}
	} else if (!(strcasecmp(askbuf, "8"))) {
		struct DayDream_Protocol *tp;

		TypeFile("protocols", TYPE_MAKE | TYPE_WARN);
		DDPut(sd[eu8str]);
		*lesbabuf = 0;
		if (user.user_protocol) {
			*lesbabuf = user.user_protocol;
			lesbabuf[1] = 0;
		}
		if (!(Prompt(lesbabuf, 3, 0)))
			return 1;
		*lesbabuf = toupper(*lesbabuf);
		if (!*lesbabuf)
			return 0;
		tp = protocols;
		for (;;) {
			if (tp->PROTOCOL_ID == 0)
				return 0;
			if (tp->PROTOCOL_ID == *lesbabuf) {
				protocol = tp;
				user.user_protocol = *lesbabuf;
				return 0;
			}
			tp++;
		}
	} else if (!(strcasecmp(askbuf, "9"))) {
		DDPut(sd[eu9str]);
		if (!(Prompt(user.user_signature, 44, 0)))
			return 1;
	} else if (!(strcasecmp(askbuf, "10"))) {
		DDPut(sd[eu10str]);
		if (!(Prompt(user.user_computermodel, 20, 0)))
			return 1;
	} else if (!(strcasecmp(askbuf, "11"))) {
		DDPut(sd[eu11str]);
		snprintf(lesbabuf, sizeof lesbabuf, "%d", user.user_flines);
		if (!(Prompt(lesbabuf, 3, 0)))
			return 1;
		user.user_flines = atoi(lesbabuf);
        } else if (!(strcasecmp(askbuf, "12"))) {
		rundoor("doors/autosig %N", 0);
		return 1;
	} else if (!(strcasecmp(askbuf, "a"))) {
		DDPut(sd[euabortedstr]);
		user = muser;
		return 1;
	} else if (!(strcasecmp(askbuf, "v"))) {
		TypeFile("edituser", TYPE_MAKE | TYPE_WARN);
	} else if (!(strcasecmp(askbuf, "s"))) {
		switches();
	} else if ((!(strcasecmp(askbuf, "c")) || (askbuf[0] == 0))) {
		DDPut(sd[eusavedstr]);
		saveuserbase(&user);
		return 1;
	}
	return 0;
}

void edituser(void)
{
	char askbuf[20];
	
	changenodestatus("Editing personal data");
	
	TypeFile("edituser", TYPE_MAKE | TYPE_WARN);

	for (;;) {
		DDPut(sd[eupromptstr]);
		askbuf[0] = 0;
		if (!(Prompt(askbuf, 3, 0)))
			return;
		
		if (handle_choice(askbuf))
			return;
	}		
}

static void switches(void)
{
	char inp[82];
	const char *s;
	char tok[82];
	uint32_t togbak;
	int poro;
	togbak = user.user_toggles;

	for (;;) {
		char buffa[200];
		char *togf;

		DDPut("\n");
		if (user.user_toggles & (1L << 12))
			togf = sd[tsaskstr];
		else if (user.user_toggles & (1L << 5))
			togf = sd[tsnostr];
		else
			togf = sd[tsyesstr];
		ddprintf(sd[togglinestr], 1, sd[ts1str], togf);

		if (user.user_toggles & (1L << 13))
			togf = sd[tsaskstr];
		else if (user.user_toggles & (1L << 6))
			togf = sd[tsyesstr];
		else
			togf = sd[tsnostr];
		ddprintf(sd[togglinestr], 2, sd[ts2str], togf);
		DDPut("\n");

		if (user.user_toggles & (1L << 11))
			togf = sd[tsaskstr];
		else if (user.user_toggles & (1L << 0))
			togf = sd[tsyesstr];
		else
			togf = sd[tsnostr];
		ddprintf(sd[togglinestr], 3, sd[ts3str], togf);

		if (user.user_toggles & (1L << 9))
			togf = sd[tsnostr];
		else
			togf = sd[tsyesstr];
		ddprintf(sd[togglinestr], 4, sd[ts4str], togf);
		DDPut("\n");

		if (user.user_toggles & (1L << 14))
			togf = sd[tsyesstr];
		else
			togf = sd[tsnostr];
		ddprintf(sd[togglinestr], 5, sd[ts5str], togf);

		if (user.user_toggles & (1L << 15))
			togf = sd[tsnostr];
		else
			togf = sd[tsyesstr];
		ddprintf(sd[togglinestr], 6, sd[ts6str], togf);
		DDPut("\n");

		poro = 1;
		while (poro) {
			DDPut(sd[tspromptstr]);
			inp[0] = 0;
			if (!(Prompt(inp, 80, 0)))
				return;
			s = inp;
			if (!*inp) {
				strlcpy(inp, "s", sizeof inp);
			}
			
			for (;;) {
				int numb;
				
				if (strtoken(tok, &s, sizeof tok) > sizeof tok)
					continue;
				if (!*tok)
					break;

				if (!strcasecmp("s", tok)) {
					return;
				} else if (!strcasecmp("a", tok)) {
					user.user_toggles = togbak;
					return;
				} else if (!strcasecmp("v", tok)) {
					poro = 0;
					break;
				} else {
					numb = atoi(tok);
					switch (numb) {
					case 1:
						if (user.user_toggles & (1L << 5)) {
							user.user_toggles |= (1L << 12);
							user.user_toggles &= ~(1L << 5);
							snprintf(buffa, sizeof buffa, sd[asktlinestr], sd[ts1str]);
						} else if (user.user_toggles & (1L << 12)) {
							user.user_toggles &= ~(1L << 12);
							user.user_toggles &= ~(1L << 5);
							snprintf(buffa, sizeof buffa, sd[ontlinestr], sd[ts1str]);
						} else {
							user.user_toggles &= ~(1L << 12);
							user.user_toggles |= (1L << 5);
							snprintf(buffa, sizeof buffa, sd[offtlinestr], sd[ts1str]);
						}
						DDPut(buffa);
						break;
					case 2:
						if (user.user_toggles & (1L << 6)) {
							user.user_toggles |= (1L << 13);
							user.user_toggles &= ~(1L << 6);
							snprintf(buffa, sizeof buffa, sd[asktlinestr], sd[ts2str]);
						} else if (user.user_toggles & (1L << 13)) {
							user.user_toggles &= ~(1L << 13);
							user.user_toggles &= ~(1L << 6);
							snprintf(buffa, sizeof buffa, sd[offtlinestr], sd[ts2str]);
						} else {
							user.user_toggles &= ~(1L << 13);
							user.user_toggles |= (1L << 6);
							snprintf(buffa, sizeof buffa, sd[ontlinestr], sd[ts2str]);
						}
						DDPut(buffa);
						break;
					case 3:
						if (user.user_toggles & (1L << 0)) {
							user.user_toggles |= (1L << 11);
							user.user_toggles &= ~(1L << 0);
							snprintf(buffa, sizeof buffa, sd[asktlinestr], sd[ts3str]);
						} else if (user.user_toggles & (1L << 11)) {
							user.user_toggles &= ~(1L << 11);
							user.user_toggles &= ~(1L << 0);
							snprintf(buffa, sizeof buffa, sd[offtlinestr], sd[ts3str]);
						} else {
							user.user_toggles &= ~(1L << 11);
							user.user_toggles |= (1L << 0);
							snprintf(buffa, sizeof buffa, sd[ontlinestr], sd[ts3str]);
						}
						DDPut(buffa);
						break;
					case 4:
						if (user.user_toggles & (1L << 9)) {
							user.user_toggles &= ~(1L << 9);
							snprintf(buffa, sizeof buffa, sd[ontlinestr], sd[ts4str]);
						} else {
							user.user_toggles |= (1L << 9);
							snprintf(buffa, sizeof buffa, sd[offtlinestr], sd[ts4str]);
						}
						DDPut(buffa);
						break;
					case 5:
						if (user.user_toggles & (1L << 14)) {
							user.user_toggles &= ~(1L << 14);
							snprintf(buffa, sizeof buffa, sd[offtlinestr], sd[ts5str]);
						} else {
							user.user_toggles |= (1L << 14);
							snprintf(buffa, sizeof buffa, sd[ontlinestr], sd[ts5str]);
						}
						DDPut(buffa);
						break;
					case 6:
						if (user.user_toggles & (1L << 15)) {
							user.user_toggles &= ~(1L << 15);
							snprintf(buffa, sizeof buffa, sd[ontlinestr], sd[ts6str]);
						} else {
							user.user_toggles |= (1L << 15);
							snprintf(buffa, sizeof buffa, sd[offtlinestr], sd[ts6str]);
						}
						DDPut(buffa);
						break;
					}
				}
			}
		}
	}
}

int tagmessageareas(void)
{
	uint8_t backup[32];
	char tbuf[500];
	const char *sta;
	msgbase_t *mb;
	int bcnt;
	char inp[90];
	const char *s;
	char tok[90];
        int screenl;        

	int i, j;

	memcpy(backup, &selcfg[(conference()->conf.CONF_NUMBER - 1) * 32],
	       sizeof backup);
	       
	vagain:
	DDPut("[2J[H");

	screenl = user.user_screenlength;
	bcnt = conference()->conf.CONF_MSGBASES;

	for (i = j = 0; j < bcnt; j++) {
		mb = conference()->msgbases[j];
		
		i++;
		if (i == 3) {
			DDPut("\n");
			i = 1;
			screenl--;
			if (screenl == 1) {
				int hot;

				DDPut(sd[morepromptstr]);
				hot = HotKey(0);
				DDPut("\r                                                         \r");
				if (hot == 'N' || hot == 'n' || !checkcarrier())
					break;
				if (hot == 'C' || hot == 'c') {
					screenl = 20000000;
				} else {
					screenl = user.user_screenlength;
				}
			}
		}
		if (isbasetagged(conference()->conf.CONF_NUMBER, mb->MSGBASE_NUMBER)) {
			sta = "ON";
		} else
			sta = "OFF";

		ddprintf(sd[tbclinestr], mb->MSGBASE_NUMBER, mb->MSGBASE_NAME, sta);
	}

	DDPut("\n");
	for (;;) {
		DDPut(sd[tbpromptstr]);
		inp[0] = 0;
		if (!(Prompt(inp, 80, 0)))
			return 0;
		s = inp;
		if (!*inp) {
			strlcpy(inp, "s", sizeof inp);
		}
		
		for (;;) {
			if (strtoken(tok, &s, sizeof tok) > sizeof tok)
				continue;
			if (!*tok)
				break;

			if (!strcasecmp(tok, "c")) {
				memcpy(&selcfg[(conference()->conf.CONF_NUMBER - 1) * 32],
				       backup, sizeof backup);
				
				return 0;
			} else if (!strcasecmp(tok, "v")) {
				goto vagain;
			} else if (!strcasecmp(tok, "s")) {
				int selfd;
				snprintf(tbuf, sizeof tbuf,
					"users/%d/selected.dat", 
					user.user_account_id);
				selfd = open(tbuf, O_WRONLY | O_CREAT, 0666);
				if (selfd != -1) {
					fsetperm(selfd, 0666);
					safe_write(selfd, &selcfg, 2056);
					close(selfd);
				}
				return 0;
			} else if (!strcasecmp(tok, "-")) {
				for (i = 0; i < 32; i++) {
					selcfg[((conference()->conf.CONF_NUMBER - 1) * 32) + i] = 0;
				}
				DDPut(sd[tballoffstr]);
			} else if (!strcasecmp(tok, "+")) {
				bcnt = conference()->conf.CONF_MSGBASES;
				
				for (i = 0; i < bcnt; i++) {
					mb = conference()->msgbases[i];
					selcfg[((conference()->conf.CONF_NUMBER - 1) * 32) + (mb->MSGBASE_NUMBER - 1) / 8] |= (1L << (mb->MSGBASE_NUMBER - 1) % 8);
				}
				DDPut(sd[tballonstr]);
			} else {
				i = atoi(tok);
				if (i) {
					for (j = 0; j < conference()->conf.CONF_MSGBASES; j++) {
						mb = conference()->msgbases[j];
						if (i == mb->MSGBASE_NUMBER) {
							if (selcfg[((conference()->conf.CONF_NUMBER - 1) * 32) + (mb->MSGBASE_NUMBER - 1) / 8] & (1L << (mb->MSGBASE_NUMBER - 1) % 8)) {
								selcfg[((conference()->conf.CONF_NUMBER - 1) * 32) + (mb->MSGBASE_NUMBER - 1) / 8] &= ~(1L << (mb->MSGBASE_NUMBER - 1) % 8);
							} else {
								selcfg[((conference()->conf.CONF_NUMBER - 1) * 32) + (mb->MSGBASE_NUMBER - 1) / 8] |= (1L << (mb->MSGBASE_NUMBER - 1) % 8);
							}
							break;
						}
					}

				}
			}
		}
	}
}

int tagconfs(void)
{
	uint8_t backup[8];
	char tbuf[500];
	const char *sta;

	char inp[90];
	const char *s;
	char tok[90];
	conference_t *mc;
	struct iterator *iterator;
	int i;
	int screenl;
	
	

	for (i = 0; i < 8; i++) {
		backup[i] = selcfg[2048 + i];
	}
      vagain:
	DDPut("[2J[H");

	i = 0;
	screenl = user.user_screenlength;
	
	iterator = conference_iterator();
	while ((mc = (conference_t *) iterator_next(iterator))) {
		if (checkconfaccess(mc->conf.CONF_NUMBER, &user)) {
			i++;
			if (i == 3) {
				DDPut("\n");
				i = 1;
				screenl--;
	    			if (screenl == 1) {
					int hot;

					DDPut(sd[morepromptstr]);
					hot = HotKey(0);
					DDPut("\r                                                         \r");
					if (hot == 'N' || hot == 'n' || !checkcarrier())
						break;
					if (hot == 'C' || hot == 'c') {
						screenl = 20000000;
					} else {
						screenl = user.user_screenlength;
					}
				}
			}
			if (isconftagged(mc->conf.CONF_NUMBER)) {
				sta = "ON";
			} else
				sta = "OFF";

			ddprintf(sd[togglinestr], mc->conf.CONF_NUMBER, mc->conf.CONF_NAME, sta);
		}
	}
	iterator_discard(iterator);
	
	DDPut("\n");

	for (;;) {
		DDPut(sd[tcpromptstr]);
		inp[0] = 0;
		if (!(Prompt(inp, 80, 0)))
			return 0;
		s = inp;
		if (!*inp) {
			strlcpy(inp, "s", sizeof inp);
		}
		
		for (;;) {
			if (strtoken(tok, &s, sizeof tok) > sizeof tok)
				continue;
			if (!*tok)
				break;

			if (!strcasecmp(tok, "c")) {
				for (i = 0; i < 8; i++) {
					selcfg[2048 + i] = backup[i];
				}
				return 0;
			} else if (!strcasecmp(tok, "v")) {
				goto vagain;
			} else if (!strcasecmp(tok, "s")) {
				int selfd;
				snprintf(tbuf, sizeof tbuf,
					"users/%d/selected.dat", 
					user.user_account_id);
				selfd = open(tbuf, O_WRONLY | O_CREAT, 0666);
				if (selfd != -1) {
					fsetperm(selfd, 0666);
					safe_write(selfd, &selcfg, 2056);
					close(selfd);
				}
				return 0;
			} else if (!strcasecmp(tok, "-")) {
				for (i = 0; i < 8; i++) {
					selcfg[2048 + i] = 0;
				}
				DDPut(sd[tcalloffstr]);
			} else if (!strcasecmp(tok, "+")) {
				for (i = 0; i < 8; i++) {
					selcfg[2048 + i] = 255;
				}
				DDPut(sd[tcallonstr]);
			} else {
				i = atoi(tok);
				if (i > 0 && i < 65 && checkconfaccess(i, &user)) {
					iterator = conference_iterator();
					while ((mc = (conference_t *) iterator_next(iterator))) {
						if (mc->conf.CONF_NUMBER == i) {
							if (selcfg[2048 + (mc->conf.CONF_NUMBER - 1) / 8] & (1L << (mc->conf.CONF_NUMBER - 1) % 8)) {
								selcfg[2048 + (mc->conf.CONF_NUMBER - 1) / 8] &= ~(1L << (mc->conf.CONF_NUMBER - 1) % 8);
							} else {
								selcfg[2048 + (mc->conf.CONF_NUMBER - 1) / 8] |= (1L << (mc->conf.CONF_NUMBER - 1) % 8);
							}
							break;
						}
					}
					iterator_discard(iterator);
				}
			}
		}
	}
}

int isbasetagged(int tconf, int tbase)
{
	if (selcfg[((tconf - 1) * 32) + (tbase - 1) / 8] & (1L << (tbase - 1) % 8))
		return 1;
	return 0;
}

int isconftagged(int tconf)
{
	if (selcfg[2048 + ((tconf - 1) / 8)] & (1L << (tconf - 1) % 8))
		return 1;
	return 0;
}

int isanybasestagged(int tconf)
{
	int i = 0;
	for (i = 0; i < 32; i++) {
		if (selcfg[((tconf - 1) * 32) + i])
			return 1;
	}
	return 0;
}

void saveuserbase(const struct userbase *usr)
{
	/* maybe this is the wisest thing to do */
	if (writeubent(usr)) {
		syslog(LOG_ALERT, "cannot save userbase: %m");
		DDPut("Cannot save userbase.\n");
		exit(1);
	}
}
