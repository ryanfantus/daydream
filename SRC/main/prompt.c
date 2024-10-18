#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <daydream.h>
#include <ddcommon.h>

static char hisbuf[1024];
static int hoffset;
static int hpoffset;

static int clearprompt(int, int);

void reset_history(void)
{
	hoffset = 0;
	hpoffset = 0;
}

int Prompt(char *buffer, int len, int flags)
{
	int i = 0;
	int j = 0;
	unsigned char pala;
	int jatka = 1;
	char buff[40];
	int kalake;

	while ((waitpid(-1, NULL, WNOHANG)) > 0);

	if (bgmode)
		return 0;

	if ((flags & PROMPT_WRAP) && *wrapbuf) {
		strlcpy(buffer, wrapbuf, len);
		wrapbuf[0] = 0;
	}
	if ((j = strlen(buffer))) {
		DDPut(buffer);
		i = j;
	}
	while (jatka) {
		if (flags & PROMPT_MAIN && !i) {
			struct olm *myolm;
			struct olm *oldolm;
			myolm = (struct olm *) olms->lh_Head;
			if (myolm->olm_head.ln_Succ) {
				DDPut("");

				while (myolm->olm_head.ln_Succ) {
					DDPut(myolm->olm_msg);
					if (myolm->olm_number) {
						char olmb[1024];

						snprintf(olmb, sizeof olmb, "%s/olm%d.%d", DDTMP, node, myolm->olm_number);
						TypeFile(olmb, TYPE_NOCODES);
						unlink(olmb);
					}
					oldolm = myolm;
					myolm = (struct olm *) myolm->olm_head.ln_Succ;
					Remove((struct Node *) oldolm);
					free(oldolm);
				}
				return 1;
			}
		}
		if (!checkcarrier())
			return 0;

		pala = HotKey(HOT_CURSOR);
		if (!checkcarrier())
			return 0;

		switch (pala) {
		case 13:
		case 10:
			jatka = 0;
			buffer[i] = 0;
			break;

		case 8:
		case 127:
			if (j) {
				if (i == j) {
					i--;
					j--;
					buffer[i] = 0;
					DDPut("\033[D \033[D");
				} else {
					DDPut("\033[D\033[P");
					strlcpy(&buffer[j - 1], &buffer[j], len);
					i--;
					j--;
				}
			}
			break;

		case 253:
			if (j) {
				j--;
				DDPut("\033[D");
			}
			break;

		case 252:
			if (j != i) {
				j++;
				DDPut("\033[C");
			}
			break;
		case 250:
			if (flags & PROMPT_MAIN && hoffset && (hpoffset > 0)) {
				char *ss;
				clearprompt(i, j);

				ss = &hisbuf[hpoffset] - 1;
				while (*ss && (ss != hisbuf))
					ss--;
				if (!*ss)
					ss++;
				strlcpy(buffer, ss, len);
				DDPut(ss);
				j = i = strlen(buffer);
				hpoffset -= (j + 1);
			}
			break;
		case 251:
			if (flags & PROMPT_MAIN)
				if (hoffset && (hpoffset != hoffset)) {
					char *ss;
					ss = &hisbuf[hpoffset + 1];
					while (*ss) {
						ss++;
						hpoffset++;
					}
					ss++;
					hpoffset++;
					if (*ss != -1) {
						clearprompt(i, j);
						strlcpy(buffer, ss, len);
						DDPut(ss);
						j = i = strlen(buffer);
					} else {
						clearprompt(i, j);
						*buffer = j = i = 0;
					}
				}
			break;
		case 0:
			break;
		default:
			if (flags & PROMPT_FILE && (pala == '/' || pala == '\\'))
				break;
			if (i != len) {
				if (pala > 31) {
					if (i == j) {
						buffer[j] = pala;
						i++;
						j++;
						if (flags & PROMPT_SECRET) {
							strlcpy(buff, "*", sizeof buff);
						} else {
							snprintf(buff, sizeof buff, "%c", pala);
						}
						DDPut(buff);
					} else {
						kalake = i;
						buffer[i + 1] = 0;
						while (kalake != j) {
							buffer[kalake] = buffer[kalake - 1];
							kalake--;
						}
						if (flags & PROMPT_SECRET) {
							strlcpy(buff, "\033[@*", sizeof buff);
						} else {
							snprintf(buff, sizeof buff, "\033[@%c", pala);
						}
						DDPut(buff);
						buffer[j] = pala;
						i++;
						j++;
					}
				}
			} else if (flags & PROMPT_WRAP && i == j) {
				char *s;
				s = &buffer[j];
				while (s != buffer) {
					if (*s == ' ') {
						char silma[15];
						*s = 0;
						strlcpy(wrapbuf, &s[1], sizeof wrapbuf);
						silma[0] = pala;
						silma[1] = 0;
						strlcat(wrapbuf, silma, sizeof wrapbuf);
						jatka = 0;
						ddprintf("[%dD[K", strlen(&s[1]));
						break;
					} else
						s--;
				}
			}
			if (flags & PROMPT_MAIN &&
			    try_hotkey_match(buffer, j)) {
				buffer[j] = 0;
				return 1;
			}
			break;
		}
	}
	if (flags & PROMPT_MAIN && i) {
		char *ss;
		char *t;
		int ii;

		while (strlen(buffer) + hoffset + 3 > 1024) {

			ss = hisbuf;
			while (*ss)
				ss++;
			ss++;
			hoffset = ss - hisbuf;
			t = hisbuf;
			for (ii = hoffset; i; i--)
				*t++ = *ss++;

		}
		t = &hisbuf[hoffset] + 1;
		while (*buffer)
			*t++ = *buffer++;
		*t++ = 0;
		*t = -1;
		hoffset += t - &hisbuf[hoffset] - 1;
		hpoffset = hoffset;
	}
	if (!(flags & PROMPT_NOCRLF))
		DDPut("\n");
	return 1;
}

static int clearprompt(int i, int j)
{
	int k;
	if (!j)
		return 0;
	if (j) 
		ddprintf("\033[%dD", j);
	for (k = i; k; k--) 
		DDPut(" ");
	ddprintf("\033[%dD", i);
	return 1;
}
