#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/mount.h>
#include <time.h>
#include <utime.h>
#include <syslog.h>

#include <hydracom.h>
#include <mycurses.h>
#include <ddcommon.h>

#ifdef HAVE_SYS_STATVFS_H
#include <sys/statvfs.h>
#endif
#ifdef HAVE_SYS_VFS_H
#include <sys/vfs.h>
#endif

static int initdevice(void);

int startncurses(void);
int devfdi;
int devfdo;

WINDOW *trwin = 0;
WINDOW *logwin = 0;
WINDOW *chatwin1 = 0;
WINDOW *chatwin2 = 0;

static struct termios tty;
static struct termios oldtty;

void sys_init(void)
{
	if (!startncurses())
		exit(1);
	if (!initdevice())
		exit(1);
}

void sys_idle(void)
{
	fd_set setti;
	struct timeval tv;

	FD_ZERO(&setti);
	FD_SET(STDIN_FILENO, &setti);
	FD_SET(devfdi, &setti);
	tv.tv_sec = 0;
	tv.tv_usec = 30000;
	select(devfdi + 1, &setti, 0, 0, &tv);
}

int kbhit(void)
{
	fd_set setti;
	struct timeval tv;

	FD_ZERO(&setti);
	FD_SET(STDIN_FILENO, &setti);
	tv.tv_sec = 0;
	tv.tv_usec = 0;
	select(STDIN_FILENO + 1, &setti, 0, 0, &tv);
	if (FD_ISSET(STDIN_FILENO, &setti))
		return 1;
	return 0;
}

void com_putbyte(byte c)
{
	write(devfdo, &c, 1);
}

int carrier(void)
{
	int kelmas;

	if (nocarrier)
		return 1;
	else {
		ioctl(devfdi, TIOCMGET, &kelmas);
		if (kelmas & TIOCM_CD)
			return 1;
		return 0;
	}
	return 1;
}

int com_outfull(void)
{
	int queue = 0;

	ioctl(devfdo, TIOCOUTQ, &queue);
	return queue;
}

void sys_reset(void)
{
	endwin();
}

int wildcmp(char *nam, char *pat)
{
	char *p;

	for (;;) {
		if (tolower(*nam) == tolower(*pat)) {
			if (*nam++ == '\0')
				return (1);
			pat++;
		} else if (*pat == '?' && *nam != 0) {
			nam++;
			pat++;
		} else
			break;
	}

	if (*pat != '*')
		return (0);

	while (*pat == '*') {
		if (*++pat == '\0')
			return (1);
	}

	for (p = nam + strlen(nam) - 1; p >= nam; p--) {
		if (tolower(*p) == tolower(*pat))
			if (wildcmp(p, pat) == 1)
				return (1);
	}
	return (0);
}

uint64_t freespace(const char *path)
{
#ifdef HAVE_STATVFS
	struct statvfs freest;
#else
	struct statfs freest;
#endif

	if (path == NULL || *path == '\0')
		return 0;
#ifdef HAVE_STATVFS
	if (statvfs(path, &freest) == -1) {
		syslog(LOG_WARNING, "cannot statfs(\"%s\"...): %m", path);
		return 0;
	}
#else
	if (statfs(path, &freest) == -1) {
		syslog(LOG_WARNING, "cannot statfs(\"%s\"...): %m", path);
		return 0;
	}
#endif
	return ((uint64_t) freest.f_bsize * (uint64_t) freest.f_bavail);
}

void setstamp(char *name, long timeh)
{
	struct utimbuf tb;

	tb.actime = timeh;
	tb.modtime = timeh;
	utime(name, &tb);
}

void dtr_out(byte flag)
{
	if (!flag) {
		tcgetattr(devfdi, &tty);
		cfsetispeed(&tty, 0);
		tcsetattr(devfdi, TCSANOW, &tty);

	}
}
void com_setspeed(word kerma)
{
}

/*

int com_getbyte(void)
{
	fd_set setti;
	struct timeval tv;
	char c;
	
	beep();
	FD_ZERO(&setti);
	FD_SET(devfdi,&setti);
	tv.tv_sec=0;
	tv.tv_usec=0;
	select(devfdi+1,&setti,0,0,&tv);

	if (FD_ISSET(devfdi,&setti)) {
		read(devfdi,&c,1);
		return c;
	}
	return EOF;
}

*/

int com_getbyte()
{
	fd_set rset;
	struct timeval tv;
	unsigned char b;

	FD_ZERO(&rset);
	FD_SET(devfdi, &rset);
	tv.tv_usec = 0;
	tv.tv_sec = 0;

	if (select(devfdi + 1, &rset, NULL, NULL, &tv) <= 0)
		return -1;

	while (read(devfdi, &b, 1) < 0)
		if (errno != EINTR)
			return -1;
	return b;
}

void com_dump(void)
{
	tcflush(devfdo, TCOFLUSH);
}

void com_flush(void)
{
	tcdrain(devfdo);
}

void com_purge(void)
{
	tcflush(devfdi, TCIFLUSH);
}

void com_putblock(byte * s, word len)
{
	write(devfdo, s, len);
}

static int initdevice(void)
{
	devfdi = open(device, O_RDWR | O_NONBLOCK);
	if (devfdi == -1)
		return 0;

	devfdo = devfdi;

	tcgetattr(devfdi, &tty);
	oldtty = tty;

	cfmakeraw(&tty);
	if (device[0] || nocarrier)
		tty.c_cflag |= CLOCAL;
	if (flowflags & 0x02 || flowflags & 0x0b)
		tty.c_cflag |= CRTSCTS;
	if (parity)
		tty.c_cflag |= (CS7 | PARENB);

	tcsetattr(devfdi, TCSANOW, &tty);
	tcsetattr(devfdo, TCSANOW, &tty);
	return cfgetispeed(&tty);
}

void killui(void)
{
	sleep(2);
	endwin();
}

int startncurses(void)
{
	int winsizes;
	int i;

	initscr();
	timeout(0);
	atexit(killui);
	start_color();
	noecho();
	cbreak();
/*	if (LINES < 25) return 0; */
	winsizes = (LINES - 4) / 3;
	trwin = newwin(4, COLS, 0, 0);
	keypad(stdscr, TRUE);
	logwin = newwin(winsizes, COLS, 4, 0);
	chatwin1 = newwin(winsizes, COLS, 4 + winsizes, 0);
	chatwin2 = newwin(winsizes, COLS, 4 + (2 * winsizes), 0);
	start_color();

	werase(trwin);
	init_pair(1, COLOR_WHITE, COLOR_BLUE);
	init_pair(3, COLOR_WHITE, COLOR_BLACK);
	wattrset(trwin, COLOR_PAIR(1));
	wmove(trwin, 0, 0);
	waddstr(trwin,
		"---( HydraCom for Unix. Ported by Antti Häyrynen )");
	for (i = COLS - 50; i; i--)
		waddch(trwin, '-');
	wmove(trwin, 1, 0);
	wrefresh(trwin);
	wattrset(trwin, COLOR_PAIR(3));

	werase(logwin);
	init_pair(2, COLOR_WHITE, COLOR_RED);
	wattrset(logwin, COLOR_PAIR(2));
	wmove(logwin, 0, 0);
	waddstr(logwin, "---( Transfer log )");
	for (i = COLS - 19; i; i--)
		waddch(logwin, '-');
	wmove(logwin, 1, 0);
	/* FIXME: curses in netbsd won't have wsetscrreg() */
#ifdef HAVE_WSETSCRREG
	wsetscrreg(logwin, 1, winsizes - 1);
#endif
	scrollok(logwin, 1);
	wrefresh(logwin);
	wattrset(logwin, COLOR_PAIR(3));

	werase(chatwin1);
	wattrset(chatwin1, COLOR_PAIR(1));
	wmove(chatwin1, 0, 0);
	waddstr(chatwin1, "---( Remote )");
	for (i = COLS - 13; i; i--)
		waddch(chatwin1, '-');
	wmove(chatwin1, 1, 0);
	/* FIXME: curses in netbsd won't have wsetscrreg() */
#ifdef HAVE_WSETSCRREG
	wsetscrreg(chatwin1, 1, winsizes - 1);
#endif
	scrollok(chatwin1, 1);
	wrefresh(chatwin1);
	wattrset(chatwin1, COLOR_PAIR(3));

	werase(chatwin2);
	wattrset(chatwin2, COLOR_PAIR(2));
	wmove(chatwin2, 0, 0);
	waddstr(chatwin2, "---( Local - Ctrl-E to chat )");
	for (i = COLS - 30; i; i--)
		waddch(chatwin2, '-');
	wmove(chatwin2, 1, 0);
	/* FIXME: curses in netbsd won't have wsetscrreg() */
#ifdef HAVE_WSETSCRREG
	wsetscrreg(chatwin2, 1, winsizes - 1);
#endif
	scrollok(chatwin2, 1);
	wrefresh(chatwin2);
	wattrset(chatwin2, COLOR_PAIR(3));
	refresh();
	touchwin(trwin);
	wrefresh(trwin);
	touchwin(logwin);
	wrefresh(logwin);
	touchwin(chatwin1);
	wrefresh(chatwin1);
	touchwin(chatwin2);
	wrefresh(chatwin2);
	/***
	redrawwin(trwin);
	redrawwin(logwin);
	redrawwin(chatwin1);
	redrawwin(chatwin2);
	***/

	return 1;
}
