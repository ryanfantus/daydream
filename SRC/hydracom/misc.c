/*=============================================================================

                              HydraCom Version 1.00

                         A sample implementation of the
                   HYDRA Bi-Directional File Transfer Protocol

                             HydraCom was written by
                   Arjen G. Lentz, LENTZ SOFTWARE-DEVELOPMENT
                  COPYRIGHT (C) 1991-1993; ALL RIGHTS RESERVED

                       The HYDRA protocol was designed by
                 Arjen G. Lentz, LENTZ SOFTWARE-DEVELOPMENT and
                             Joaquim H. Homrighausen
                  COPYRIGHT (C) 1991-1993; ALL RIGHTS RESERVED


  Revision history:
  06 Sep 1991 - (AGL) First tryout
  .. ... .... - Internal development
  11 Jan 1993 - HydraCom version 1.00, Hydra revision 001 (01 Dec 1992)


  For complete details of the Hydra and HydraCom licensing restrictions,
  please refer to the license agreements which are published in their entirety
  in HYDRACOM.C and LICENSE.DOC, and also contained in the documentation file
  HYDRACOM.DOC

  Use of this file is subject to the restrictions contained in the Hydra and
  HydraCom licensing agreements. If you do not find the text of this agreement
  in any of the aforementioned files, or if you do not have these files, you
  should immediately contact LENTZ SOFTWARE-DEVELOPMENT and/or Joaquim
  Homrighausen at one of the addresses listed below. In no event should you
  proceed to use this file without having accepted the terms of the Hydra and
  HydraCom licensing agreements, or such other agreement as you are able to
  reach with LENTZ SOFTWARE-DEVELOMENT and Joaquim Homrighausen.


  Hydra protocol design and HydraCom driver:         Hydra protocol design:
  Arjen G. Lentz                                     Joaquim H. Homrighausen
  LENTZ SOFTWARE-DEVELOPMENT                         389, route d'Arlon
  Langegracht 7B                                     L-8011 Strassen
  3811 BT  Amersfoort                                Luxembourg
  The Netherlands
  FidoNet 2:283/512, AINEX-BBS +31-33-633916         FidoNet 2:270/17
  arjen_lentz@f512.n283.z2.fidonet.org               joho@ae.lu

  Please feel free to contact us at any time to share your comments about our
  software and/or licensing policies.

=============================================================================*/

#include <hydracom.h>
#include <mycurses.h>

extern WINDOW *trwin;
extern WINDOW *logwin;
extern WINDOW *chatwin1;
extern WINDOW *chatwin2;

static char *chatstart = "\007\007 * Chat mode start\r\n";
static char *chatend = "\007\007\r\n * Chat mode end\r\n";
static char *chattime = "\007\007\r\n * Chat mode end - timeout\r\n";

#ifndef HAVE_REDRAWWIN
void reddrawwin(WINDOW *);
#endif

static void loc_puts(char *s)
{
	while (*s) {
		if (*s == '\007')
			putc(7, stderr);
		else if (*s != '\r')
			waddch(chatwin2, *s);
		wrefresh(chatwin2);
		s++;
	}
}

int keyabort(void)
{
#define CHATLEN 256
	static byte chatbuf1[CHATLEN + 5],
	    chatbuf2[CHATLEN + 5], *curbuf = chatbuf1;
	static boolean warned = false;
	boolean esc = false;
	char *p;
	word c;

	if (chattimer > 0L) {
		if (time(NULL) > chattimer) {
			chattimer = lasttimer = 0L;
			hydra_devsend("CON", (byte *) chattime,
				      strlen(chattime));
			loc_puts(&chattime[2]);
		} else if ((time(NULL) + 10L) > chattimer && !warned) {
			loc_puts
			    ("\007\r\n * Warning: chat mode timeout in 10 seconds\r\n");
			warned = true;
		}
	} else if (chattimer != lasttimer) {
		if (chattimer == 0L) {
			if (nobell)
				p = " * Remote has chat facility with bell disabled\n";
			else
				p = " * Remote has chat facility with bell enabled\n";
			hydra_devsend("CON", (byte *) p, (int) strlen(p));
			loc_puts
			    (" * Hydra session in progress, chat facility now available\r\n");
		} else if (chattimer == -1L)
			loc_puts
			    (" * Hydra session in init state, can't chat yet\r\n");
		else if (chattimer == -2L)
			loc_puts
			    (" * Remote has no chat facility available\r\n");
		else if (chattimer == -3L) {
			if (lasttimer > 0L)
				loc_puts("\r\n");
			loc_puts
			    (" * Hydra session in exit state, can't chat anymore\r\n");
		}
		lasttimer = chattimer;
	}
	while (kbhit()) {
		switch (c = get_key()) {
		case Esc:
			esc = true;
			break;

		case 12:
			redrawwin(curscr);
			break;
		case 5:
			if (chattimer == 0L) {
				hydra_devsend("CON", (byte *) chatstart,
					      strlen(chatstart));
				loc_puts(&chatstart[2]);
				chattimer = lasttimer =
				    time(NULL) + CHAT_TIMEOUT;
			} else if (chattimer > 0L) {
				chattimer = lasttimer = 0L;
				hydra_devsend("CON", (byte *) chatend,
					      strlen(chatend));
				loc_puts(&chatend[2]);
			} else
				loc_puts("\007");
			break;

		default:
			if (c < ' ' || c > 128)
				break;

		case '\r':
		case '\a':
		case '\b':
		case '\n':
		case 127:
			if (chattimer <= 0L)
				break;

			chattimer = time(NULL) + CHAT_TIMEOUT;
			warned = false;

			if (chatfill >= CHATLEN)
				loc_puts("\007");
			else {
				switch (c) {
				case '\r':
					curbuf[chatfill++] = '\n';
					loc_puts("\r\n");
					break;

				case 127:
				case '\b':
					if (chatfill > 0
					    && curbuf[chatfill - 1] !=
					    '\n')
						chatfill--;
					else {
						curbuf[chatfill++] = '\b';
						curbuf[chatfill++] = ' ';
						curbuf[chatfill++] = '\b';
					}
					loc_puts("\b \b");
					break;

				default:
					curbuf[chatfill++] = (byte) c;
					if (c != 7)
						waddch(chatwin2, c);
					wrefresh(chatwin2);

					break;
				}
			}
			break;
		}
	}

	if (chatfill > 0 && hydra_devsend("CON", curbuf, chatfill)) {
		curbuf = (curbuf == chatbuf1) ? chatbuf2 : chatbuf2;
		chatfill = 0;
	}

	return (esc);
}

void rem_chat(byte *data, word len)
{
	while (*data) {
		switch (*data) {
		case '\a':
			if (!nobell) {
				putc(7, stderr);
			}
			break;

		case '\n':
			waddch(chatwin1, '\n');
			wrefresh(chatwin1);
			break;

		default:
			waddch(chatwin1, *data);
			wrefresh(chatwin1);
			break;
		}
		data++;
	}

}


int parse(char *string)
{
	int ac = 0;
	char *p;

	p = strchr(string, ';');
	if (p)
		*p = '\0';

	av[ac] = strtok(string, " \t\r\n\032");

	while (av[ac]) {
		if (++ac > MAXARGS) {
			message(6, "!Too many arguments!");
			endprog(2);
		}
		av[ac] = strtok(NULL, " \t\r\n\032");
	}
	return (ac);
}

void splitpath(char *filepath, char *path, char *file)
{
	char *p, *q;

	for (p = filepath; *p; p++);
	while (p != filepath && *p != ':' && *p != '\\' && *p != '/')
		--p;
	if (*p == ':' || *p == '\\' || *p == '/')
		++p;		/* begin     */
	q = filepath;
	while (q != p)
		*path++ = *q++;	/* copy path */
	*path = '\0';
	strcpy(file, p);
}

void mergepath(char *filepath, char *path, char *file)
{
	strcpy(filepath, path);
	strcat(filepath, file);
}

int fexist(char *filename)
{
	struct stat f;

	return ((stat(filename, &f) != -1) ? 1 : 0);
}

int get_key(void)
{
	int c = getch();

	return c ? c : getch() | 0x100;
}

void any_key(void)
{
	char buffer[30];
	fputs("Press any key to continue", stderr);
	fgets(buffer, 30, stdin);
/*        get_key(); */
	fputs("\r                          \r", stderr);
}				/*any_key() */


int get_str(char *prompt, char *s, int maxlen)
{
	int i = (int) strlen(s), c;

	cprint("\r%s: %s", prompt, s);
	for (;;) {
		switch (c = get_key()) {
		case 13:
			s[i] = '\0';
			cprint("\n");
			return (i);

		case 27:
			if (i) {
				do
					cprint("\b \b");
				while (--i);
			}
			s[0] = '\0';
			cprint("<aborted>\n");
			return (-1);

		case 8:
		case 127:
			if (i) {
				--i;
				cprint("\b \b");
			}
			break;

		default:
			if (i == maxlen || c < 32 || c > 126) 
				putc(7, stderr);
			else {
				cprint("%c", c);
				s[i++] = c;
			}
			break;
		}		/*switch */
	}			/*for */
}				/*get_str() */


void resultlog(boolean xmit, char *fname, long bytes, long xfertime)
{				/* Omen's DSZ compatible logfile - for RBBS-PC XFER-?.DEF reports */
	FILE *fp;

	if (opuslog) {
		if ((fp = sfopen(opuslog, "at", DENY_WRITE)) != NULL) {
			if (fname) {
				fprintf(fp, "%s %s%s %ld",
					xmit ? "Sent" : "Got",
					xmit ? "" : download, fname,
					bytes);
				if (mailer)
					fprintf(fp, " %ld", xfertime);
				fprintf(fp, "\n");
			}
			fclose(fp);
		} else
			message(3, "-Couldn't append opus log-file %s",
				opuslog);
	}

	if (result) {
		if ((fp = sfopen(result, "at", DENY_WRITE)) != NULL) {
			if (fname) {
				fprintf(fp,
					"%c %6ld %5u bps %4ld cps 0 errors     0 1024 %s -1\n",
					xmit ? 'H' : 'R', bytes, cur_speed,
					xfertime ? (bytes /
						    xfertime) : 9999L,
					fname);
			}
			fclose(fp);
		} else
			message(3, "-Couldn't append result-file %s",
				result);
	}
}				/*resultlog() */


static char *mon[12] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
	"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

char *h_revdate(long revstamp)
{
	static char buf[12];
	struct tm *t;

	t = localtime(&revstamp);
	sprintf(buf, "%02d %s %d",
		t->tm_mday, mon[t->tm_mon], t->tm_year + 1900);

	return (buf);
}				/*h_revdate() */


void message(int level, char *fmt, ...)
{
	char buf[255];
	long tim;
	struct tm *t;
	va_list arg_ptr;

	tim = time(NULL);
	t = localtime(&tim);

	va_start(arg_ptr, fmt);
	sprintf(buf, "%c %02d %3s %02d:%02d:%02d %-4s ",
		*fmt, t->tm_mday, mon[t->tm_mon],
		t->tm_hour, t->tm_min, t->tm_sec, LOGID);
	vsprintf(&buf[23], &fmt[1], arg_ptr);
	va_end(arg_ptr);

	if (level >= loglevel && logfp)
		fprintf(logfp, "%s\n", buf);

	if (!trwin)
		cprint("%s\n", buf);
	else {
		waddstr(logwin, buf);
		waddch(logwin, '\n');
		wrefresh(logwin);
	}
}


void cprint(char *fmt, ...)
{
	char buf[255];
	va_list arg_ptr;

	va_start(arg_ptr, fmt);
	vsprintf(buf, fmt, arg_ptr);
	va_end(arg_ptr);

	if (logwin) {
		waddstr(logwin, buf);
		wrefresh(logwin);
	}
}


void hydra_gotoxy(int x, int y)
{
	wmove(trwin, y, x);
}

void hydra_printf(char *fmt, ...)
{
	char buf[255];
	va_list arg_ptr;

	va_start(arg_ptr, fmt);
	vsprintf(buf, fmt, arg_ptr);
	va_end(arg_ptr);

	waddstr(trwin, buf);
	wrefresh(trwin);
}

void hydra_clreol(void)
{
}
