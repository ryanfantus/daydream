/*
 *
 *  Rev 05-05-1988
 *  This file contains Unix specific code for setting terminal modes,
 *  very little is specific to ZMODEM or YMODEM per se (that code is in
 *  sz.c and rz.c).  The CRC-16 routines used by XMODEM, YMODEM, and ZMODEM
 *  are also in this file, a fast table driven macro version
 *
 *	V7/BSD HACKERS:  SEE NOTES UNDER mode(2) !!!
 *
 *   This file is #included so the main file can set parameters such as HOWMANY.
 *   See the main files (rz.c/sz.c) for compile instructions.
 */

#include <config.h>

#include <zmodem.h>
#include <zm.h>

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <termios.h>
#define USE_TERMIO

#include <unistd.h>
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

static struct termios oldtty;
static struct termios tty;

int iofd = 0;		/* File descriptor for ioctls & reads */

/*
 * mode(n)
 *  3: save old tty stat, set raw mode with flow control
 *  2: set XON/XOFF for sb/sz with ZMODEM or YMODEM-g
 *  1: save old tty stat, set raw mode 
 *  0: restore original tty mode
 */
int mode(int n)
{
	static int did0 = FALSE;

	vfile("mode:%d", n);
	switch(n) {
	case 2:		/* Un-raw mode used by sz, sb when -g detected */
		if(!did0)
			tcgetattr(iofd, &oldtty);
		tty = oldtty;

		tty.c_iflag = BRKINT|IXON;

		tty.c_oflag = 0;	/* Transparent output */

		tty.c_cflag &= ~PARENB;	/* Disable parity */
		tty.c_cflag |= CS8;	/* Set character size = 8 */
		tty.c_lflag = Zmodem ? 0 : ISIG;
		tty.c_cc[VINTR] = Zmodem ? -1:030;	/* Interrupt char */
		tty.c_cc[VQUIT] = -1;			/* Quit char */
		tty.c_cc[VMIN] = 1;
		tty.c_cc[VTIME] = 1;	/* or in this many tenths of seconds */

		tcsetattr(iofd, TCSADRAIN, &tty);
		did0 = TRUE;
		return OK;
	case 1:
	case 3:
		if(!did0)
			tcgetattr(iofd, &oldtty);
		tty = oldtty;

		tty.c_iflag = n==3 ? (IGNBRK|IXOFF) : IGNBRK;

		 /* No echo, crlf mapping, INTR, QUIT, delays, no erase/kill */
		tty.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);

		tty.c_oflag = 0;	/* Transparent output */

		tty.c_cflag &= ~PARENB;	/* Same baud rate, disable parity */
		tty.c_cflag |= CS8;	/* Set character size = 8 */
		tty.c_cc[VMIN] = 1; /* This many chars satisfies reads */
		tty.c_cc[VTIME] = 1;	/* or in this many tenths of seconds */
		tcsetattr(iofd, TCSADRAIN, &tty);
		did0 = TRUE;
		Baudrate = cfgetispeed(&tty);
		return OK;
	case 0:
		if(!did0)
			return ERROR;
		tcsendbreak(iofd, 1);
		tcflush(iofd, TCIFLUSH);
		tcsetattr(iofd, TCSADRAIN, &oldtty);
		tcflow(iofd, TCOON);
		return OK;
	default:
		return ERROR;
	}
}
/* End of rbsb.c */
