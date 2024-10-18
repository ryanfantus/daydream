#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <sys/ioctl.h>
#include <termios.h>

#include <config.h>
#include <dd.h>
#include <zm.h>
#include <zmodem.h>
#include <crctab.h>
#include <rbsb.h>

#define MAX_BLOCK 8192

/* Ward Christensen / CP/M parameters - Don't change these! */
#define ERRORMAX 5
#define RETRYMAX 5
#define WCEOT (-10)
#define PATHLEN 257		/* ready for 4.2 bsd ? */
#define UNIXFILE 0xF000		/* The S_IFMT file mask bit for stat */
int Zmodem = 0;			/* ZMODEM protocol requested */

#define HOWMANY MAX_BLOCK

#include <dirent.h>

static char *fgetsnolf(char *, int, FILE *) __attr_bounded__ (__string__, 1, 2);

static FILE *fout;
static int dszlog;
static int last_bps;

/*
 * Routine to calculate the free bytes on the current file system
 *  ~0 means many free bytes (unknown)
 */
static long getfree()
{
	return (~0L);		/* many free bytes ... */
}

int Lastrx;
int Crcflg;
static int Firstsec;
static int Eofseen;		/* indicates cpm eof (^Z) has been received */
static int Readnum = HOWMANY;	/* Number of bytes to ask for in read() from modem */

static char pathsname[PATHLEN];
static char tempsname[PATHLEN];
static char myname[256];

#define DEFBYTL 2000000000L	/* default rx file size */
static long Bytesleft;		/* number of bytes of incoming file left */
static long Modtime;		/* Unix style mod time for incoming file */
static int Filemode;		/* Unix style mode for incoming file */
static char Pathname[PATHLEN];
static int Batch = 0;
static int Topipe = 0;
static int MakeLCPathname = TRUE;	/* make received pathname lower case */
int Verbose = 0;
static int Quiet = 0;		/* overrides logic that would otherwise set verbose */
static int Nflag = 0;		/* Don't really transfer files */
static int Rxclob = FALSE;	/* Clobber existing file */
static int Rxascii = FALSE;	/* receive files in ascii (translate) mode */
static int Rxbinary = FALSE;	/* receive files in binary mode */
static int Thisbinary;		/* current file is to be received in bin mode */
static int Blklen;		/* record length of received packets */
static long rxbytes;
static int try_resume = FALSE;
static char *rbmsg = "%s waiting to receive.";
static int no_timeout = FALSE;
static char secbuf[MAX_BLOCK + 1];
static char linbuf[HOWMANY];
static char Lzmanag;		/* Local file management request */
static char zconv;		/* ZMODEM file conversion request */
static char zmanag;		/* ZMODEM file management request */
static char ztrans;		/* ZMODEM file transport request */
static time_t lup = 0;
static struct DayDream_NodeInfo nin;
static int closeok = 0;
static int tryzhdrtype = ZRINIT;	/* Header type to send corresponding to Last rx close */

int nodeinf;
char *Progname;			/* the name by which we were called */
int Lleft = 0;			/* number of characters in linbuf */
int Zrwindow = 1400;		/* RX window size (controls garbage count) */
char *readline_ptr;	        /* pointer for removing chars from linbuf */
void purgeline(void);

static void ackbibi(void);
static void zmputs(char *s);
static int rzfiles(void);
static int rzfile(void);
static int tryz(void);
static int closeit(void);
static int wcgetsec(char *rxbuf, int maxtime);
static int putsec(char *buf, int n);
static void usage(void);
static int findfile(char *file, char *pa);
static int wcrx(void);
static int wcrxpn(char *rpn);
static void checkpath(char *name);
static void report(int sct);
static void uncaps(char *s);
static void canit(void);
static int IsAnyLower(const char *s);
static void chkinvok(char *s);
static int wcreceive(int argc, char **argp);
static void sendbrk(void);
static int procheader(char *);

/* called by signal interrupt or terminate to clean things up */
static void bibi(int signum)
{
	if (Zmodem)
		zmputs(Attn);
	canit();
	mode(0);
	closeit();
	fprintf(stderr, "lrz: caught signal %d; exiting", signum);
	exit(128 + signum);
}

int main(int argc, char *argv[])
{
	char *cp;
	int npats;
	char *virgin, **patts = NULL;
	int exitcode = 0;

	*pathsname = 0;
	*tempsname = 0;
	nodeinf = -1;

	dszlog = -1;

	Rxtimeout = 1000;
	setbuf(stderr, NULL);

	chkinvok(virgin = argv[0]);	/* if called as [-]rzCOMMAND set flag */
	npats = 0;
	while (--argc) {
		cp = *++argv;
		if (*cp == '-') {
			while (*++cp) {
				switch (*cp) {
				case '\\':
					cp[1] = toupper(cp[1]);
					continue;
				case '+':
					Lzmanag = ZMAPND;
					break;
				case 'a':
					Rxascii = TRUE;
					break;
				case 'b':
					Rxbinary = TRUE;
					break;
				case 'c':
					Crcflg = TRUE;
					break;
				case 'g':
					if (--argc < 1) {
						usage();
					}

					iofd = open(*++argv, O_RDWR);
					break;
				case 'D':
					Nflag = TRUE;
					break;
				case 'e':
					Zctlesc = 1;
					break;
				case 'h':
					usage();
					break;
				case 'O':
					no_timeout = TRUE;
					break;
				case 'p':
					Lzmanag = ZMPROT;
					break;
				case 'q':
					Quiet = TRUE;
					Verbose = 0;
					break;
				case 'r':
					try_resume = TRUE;
					break;
				case 't':
					if (--argc < 1) {
						usage();
					}
					Rxtimeout = atoi(*++argv);
					if (Rxtimeout < 10
					    || Rxtimeout > 1000) usage();
					break;
				case 'f':
					if (--argc < 1) {
						usage();
					}
					strcpy(pathsname, *++argv);
					break;
				case 'F':
					if (--argc < 1) {
						usage();
					}
					strcpy(tempsname, *++argv);
					break;
				case 'n':
					if (--argc < 1) {
						usage();
					}
					nodeinf = open(*++argv, O_RDWR);
					read(nodeinf, &nin,
					     sizeof(struct
						    DayDream_NodeInfo));
					nin.ddn_flags |= (1L << 1);
					break;
				case 'l':
					if (--argc < 1) {
						usage();
					}
					dszlog =
					    open(*++argv,
						 O_WRONLY | O_CREAT, 0755);
					break;
				case 'w':
					if (--argc < 1) {
						usage();
					}
					Zrwindow = atoi(*++argv);
					break;
				case 'u':
					MakeLCPathname = FALSE;
					break;
				case 'v':
					++Verbose;
					break;
				case 'y':
					Rxclob = TRUE;
					break;
				default:
					usage();
				}
			}
		} else if (!npats && argc > 0) {
			if (argv[0][0]) {
				npats = argc;
				patts = argv;
			}
		}
	}
	if (npats > 1)
		usage();
	if (Batch && npats)
		usage();
	if (Verbose) 
		setbuf(stderr, NULL);
	
	if (!Quiet) {
		if (Verbose == 0)
			Verbose = 2;
	}
	vfile("%s %s\n", Progname, VERSION);
	mode(1);
	signal(SIGTERM, bibi);
	if (wcreceive(npats, patts) == ERROR) {
		exitcode = 0200;
		canit();
	}
	mode(0);
	if (exitcode && !Zmodem)	/* bellow again with all thy might. */
		canit();
	exit(exitcode);

	return 0; /* compilers would complain */
}

static void usage(void)
{
	fprintf(stderr, "Usage: lrz [-abeuvy] [-L FILE] (ZMODEM)\n");
	fprintf(stderr, "or     lrb [-abuvy] [-L FILE]      (YMODEM)\n");
	fprintf(stderr,
		"or     lrx [-abcv] [-L FILE] file  (XMODEM or XMODEM-1k)\n");
	fprintf(stderr, "	  -a ASCII transfer (strip CR)\n");
	fprintf(stderr, "	  -b Binary transfer for all files\n");
	fprintf(stderr, "	  -c Use 16 bit CRC	(XMODEM)\n");
	fprintf(stderr,
		"	  -e Escape control characters	(ZMODEM)\n");
	fprintf(stderr, "          -h Help, print this usage message\n");
	fprintf(stderr, "	  -R restricted, more secure mode\n");
	fprintf(stderr, "	  -v Verbose more v's give more info\n");
	fprintf(stderr, "	  -y Yes, clobber existing file if any\n");
	fprintf(stderr, "\t%s version %s\n", Progname, VERSION);
	exit(0);
}

static int wcreceive(int argc, char **argp)
{
	int c;

	if (Batch || argc == 0) {
		Crcflg = 1;
		if (!Quiet)
			fprintf(stderr, rbmsg, Progname, "sz");
		if ((c = tryz())) {
			if (c == ZCOMPL)
				return OK;
			if (c == ERROR)
				goto fubar;
			c = rzfiles();
			if (c)
				goto fubar;
		} else {
			for (;;) {
				if (wcrxpn(secbuf) == ERROR)
					goto fubar;
				if (secbuf[0] == 0)
					return OK;
				if (procheader(secbuf) == ERROR)
					goto fubar;
				if (wcrx() == ERROR)
					goto fubar;
			}
		}
	} else {
		Bytesleft = DEFBYTL;
		Filemode = 0;
		Modtime = 0L;

		procheader("");
		strcpy(Pathname, *argp);
		checkpath(Pathname);
		fprintf(stderr, "\n%s: ready to receive %s\r\n", Progname,
			Pathname);
		if ((fout = fopen(Pathname, "w")) == NULL)
			return ERROR;
		if (wcrx() == ERROR)
			goto fubar;
		timing(1);
	}
	return OK;
      fubar:
	canit();
	if (Topipe && fout) {
		pclose(fout);
		return ERROR;
	}
	if (fout)
		closeit();
	return ERROR;
}


/*
 * Fetch a pathname from the other end as a C ctyle ASCIZ string.
 * Length is indeterminate as long as less than Blklen
 * A null string represents no more files (YMODEM)
 */
static int wcrxpn(char *rpn)
{
	int c;

	readline(1);

      et_tu:
	Firstsec = TRUE;
	Eofseen = FALSE;
	xsendline(Crcflg ? WANTCRC : NAK);
	Lleft = 0;		/* Do read next time ... */
	while ((c = wcgetsec(rpn, 100)) != 0) {
		if (c == WCEOT) {
			zperr("Pathname fetch returned %d", c);
			xsendline(ACK);
			Lleft = 0;	/* Do read next time ... */
			readline(1);
			goto et_tu;
		}
		return ERROR;
	}
	xsendline(ACK);
	return OK;
}

/*
 * Adapted from CMODEM13.C, written by
 * Jack M. Wierda and Roderick W. Hart
 */

static int wcrx(void)
{
	int sectnum, sectcurr;
	char sendchar;
	int cblklen;		/* bytes to dump this block */

	Firstsec = TRUE;
	sectnum = 0;
	Eofseen = FALSE;
	sendchar = Crcflg ? WANTCRC : NAK;

	for (;;) {
		xsendline(sendchar);	/* send it now, we're ready! */
		Lleft = 0;	/* Do read next time ... */
		sectcurr = wcgetsec(secbuf, (sectnum & 0177) ? 50 : 130);
		report(sectcurr);
		if (sectcurr == ((sectnum + 1) & 0377)) {
			sectnum++;
			cblklen = Bytesleft > Blklen ? Blklen : Bytesleft;
			if (putsec(secbuf, cblklen) == ERROR)
				return ERROR;
			if ((Bytesleft -= cblklen) < 0)
				Bytesleft = 0;
			sendchar = ACK;
		} else if (sectcurr == (sectnum & 0377)) {
			zperr("Received dup Sector");
			sendchar = ACK;
		} else if (sectcurr == WCEOT) {
			if (closeit())
				return ERROR;
			xsendline(ACK);
			Lleft = 0;	/* Do read next time ... */
			return OK;
		} else if (sectcurr == ERROR)
			return ERROR;
		else {
			zperr("Sync Error");
			return ERROR;
		}
	}
}

/*
 * Wcgetsec fetches a Ward Christensen type sector.
 * Returns sector number encountered or ERROR if valid sector not received,
 * or CAN CAN received
 * or WCEOT if eot sector
 * time is timeout for first char, set to 4 seconds thereafter
 ***************** NO ACK IS SENT IF SECTOR IS RECEIVED OK **************
 *    (Caller must do that when he is good and ready to get next sector)
 */

static int wcgetsec(char *rxbuf, int maxtime)
{
	int checksum, wcj, firstch;
	unsigned short oldcrc;
	char *p;
	int sectcurr;

	for (Lastrx = errors = 0; errors < RETRYMAX; errors++) {

		if ((firstch = readline(maxtime)) == STX) {
			Blklen = 1024;
			goto get2;
		}
		if (firstch == SOH) {
			Blklen = 128;
		      get2:
			sectcurr = readline(1);
			if ((sectcurr + (oldcrc = readline(1))) == 0377) {
				oldcrc = checksum = 0;
				for (p = rxbuf, wcj = Blklen; --wcj >= 0;) {
					if ((firstch = readline(1)) < 0)
						goto bilge;
					oldcrc = updcrc(firstch, oldcrc);
					checksum += (*p++ = firstch);
				}
				if ((firstch = readline(1)) < 0)
					goto bilge;
				if (Crcflg) {
					oldcrc = updcrc(firstch, oldcrc);
					if ((firstch = readline(1)) < 0)
						goto bilge;
					oldcrc = updcrc(firstch, oldcrc);
					if (oldcrc & 0xFFFF)
						zperr("CRC");
					else {
						Firstsec = FALSE;
						return sectcurr;
					}
				}
					else
				    if (((checksum - firstch) & 0377) ==
					0) {
					Firstsec = FALSE;
					return sectcurr;
				} else
					zperr("Checksum");
			} else
				zperr("Sector number garbled");
		}
		/* make sure eot really is eot and not just mixmash */
		else if (firstch == EOT && readline(1) == TIMEOUT)
			return WCEOT;
		else if (firstch == CAN) {
			if (Lastrx == CAN) {
				zperr("Sender CANcelled");
				return ERROR;
			} else {
				Lastrx = CAN;
				continue;
			}
		} else if (firstch == TIMEOUT) {
			if (Firstsec)
				goto humbug;
		      bilge:
			zperr("TIMEOUT");
		} else
			zperr("Got 0%o sector header", firstch);

	      humbug:
		Lastrx = 0;
		while (readline(1) != TIMEOUT);
		if (Firstsec) {
			xsendline(Crcflg ? WANTCRC : NAK);
			Lleft = 0;	/* Do read next time ... */
		} else {
			maxtime = 40;
			xsendline(NAK);
			Lleft = 0;	/* Do read next time ... */
		}
	}
	/* try to stop the bubble machine. */
	canit();
	return ERROR;
}

/*
 * This version of readline is reasoably well suited for
 * reading many characters.
 *  (except, currently, for the Regulus version!)
 *
 * timeout is in tenths of seconds
 */
int readline(int timeout)
{
	fd_set rset;
	struct timeval tv;

	if (--Lleft >= 0) {
		if (Verbose > 8) {
			fprintf(stderr, "%02x ", *readline_ptr & 0377);
		}
		return (*readline_ptr++ & 0377);
	}

	FD_ZERO(&rset);
	FD_SET(iofd, &rset);
	if (timeout < 20) 
		timeout = 30;
	tv.tv_sec = timeout / 10;
	tv.tv_usec = (timeout % 10) * 100000;
	if (select(iofd + 1, &rset, NULL, NULL, &tv) == 0)
		return TIMEOUT;

	Lleft = read(iofd, readline_ptr = linbuf, Readnum);
	if (Lleft < 1)
		return TIMEOUT;
	--Lleft;
	if (Verbose > 8) {
		fprintf(stderr, "%02x ", *readline_ptr & 0377);
	}
	return (*readline_ptr++ & 0377);
}

/*
 * Purge the modem input queue of all characters
 */
void purgeline(void)
{
	Lleft = 0;
	tcflush(iofd, TCIFLUSH);
}

/*
 * Process incoming file information header
 */
static int procheader(char *name)
{
	char *openmode, *p;

	closeok = 0;

	/* set default parameters and overrides */
	openmode = "w";
	Thisbinary = (!Rxascii) || Rxbinary;
	if (Lzmanag)
		zmanag = Lzmanag;

	/*
	 *  Process ZMODEM remote file management requests
	 */
	if (!Rxbinary && zconv == ZCNL)	/* Remote ASCII override */
		Thisbinary = 0;
	if (zconv == ZCBIN)	/* Remote Binary override */
		Thisbinary = TRUE;
	else if (zmanag == ZMAPND)
		openmode = "a";
	if (try_resume)
		zconv = ZCRESUM;

	/* Check for existing file */

	if (zconv != ZCRESUM && !Rxclob && (zmanag & ZMMASK) != ZMCLOB
	    && (fout = fopen(name, "r"))) {
		fclose(fout);
		return ERROR;
	}

	Bytesleft = DEFBYTL;
	Filemode = 0;
	Modtime = 0L;

	p = name + 1 + strlen(name);
	if (*p) {		/* file coming from Unix or DOS system */
		sscanf(p, "%ld%lo%o", &Bytesleft, &Modtime, &Filemode);
		if (Filemode & UNIXFILE)
			++Thisbinary;
	} else {		/* File coming from CP/M system */
		for (p = name; *p; ++p)	/* change / to _ */
			if (*p == '/')
				*p = '_';

		if (*--p == '.')	/* zap trailing period */
			*p = 0;
	}

	if (!Zmodem && MakeLCPathname && !IsAnyLower(name)
	    && !(Filemode & UNIXFILE))
		uncaps(name);

	if (Topipe > 0) {
		sprintf(Pathname, "%s %s", Progname + 2, name);
		if (Verbose)
			fprintf(stderr, "Topipe: %s %s\n",
				Pathname, Thisbinary ? "BIN" : "ASCII");
		if ((fout = popen(Pathname, "w")) == NULL)
			return ERROR;
	} else {
		strcpy(Pathname, name);
		if (Verbose)
			fprintf(stderr, "\nReceiving: %s\n", name);
		timing(1);
		checkpath(name);
		if (Nflag)
			name = "/dev/null";

		if (Thisbinary && zconv == ZCRESUM) {
			struct stat st;
			fout = fopen(name, "r+");
			if (fout && 0 == fstat(fileno(fout), &st)) {
				/* retransfer whole blocks */
				rxbytes = st.st_size & ~(1024);
				/* Bytesleft == filelength on remote */
				if (rxbytes < Bytesleft) {
					if (fseek(fout, rxbytes, 0)) {
						fclose(fout);
						return ZFERR;
					}
				}
				goto buffer_it;
			}
			rxbytes = 0;
			if (fout)
				fclose(fout);
		}

		if (*pathsname) {
			if (findfile(name, pathsname))
				return ERROR;
		}
		if (*tempsname) {
			char puskuri[300];

			FILE *pathsfd;

			if ((pathsfd = fopen(tempsname, "r")) != NULL) {
				while (fgets(puskuri, 300, pathsfd)) {
					char *s;
					int filefd;

					s = puskuri;
					while (*s) {
						if (*s == 13 || *s == 10)
							*s = 0;
						else
							s++;
					}
					strcat(puskuri, name);
					filefd = open(puskuri, O_RDONLY);
					if (filefd != -1) {
						close(filefd);
						fclose(pathsfd);
						return ERROR;
					}
				}
				fclose(pathsfd);
			}
		}
		strcpy(myname, name);
		fout = fopen(name, openmode);
		if (!fout) {
			int e = errno;
			fprintf(stderr, "lrz: cannot open %s: %s\n", name,
				strerror(e));
			return ERROR;
		}
	}
      buffer_it:
	closeok = 0;
	if (Topipe == 0) {
		static char *s = NULL;
		if (!s) {
			s = malloc(16384);
			if (!s) {
				fprintf(stderr, "lrz: out of memory\r\n");
				exit(1);
			}
#ifdef SETVBUF_REVERSED
			setvbuf(fout, _IOFBF, s, 16384);
#else
			setvbuf(fout, s, _IOFBF, 16384);
#endif
		}
	}

	return OK;
}

/*
 * Putsec writes the n characters of buf to receive file fout.
 *  If not in binary mode, carriage returns, and all characters
 *  starting with CPMEOF are discarded.
 */
static int putsec(char *buf, int n)
{
	char *p;

	if (n == 0)
		return OK;
	if (Thisbinary) {
		if (fwrite(buf, n, 1, fout) != 1)
			return ERROR;
	} else {
		if (Eofseen)
			return OK;
		for (p = buf; --n >= 0; ++p) {
			if (*p == '\r')
				continue;
			if (*p == CPMEOF) {
				Eofseen = TRUE;
				return OK;
			}
			putc(*p, fout);
		}
	}
	return OK;
}

/* make string s lower case */
static void uncaps(char *s)
{
	for (; *s; ++s)
		if (isupper(*s))
			*s = tolower(*s);
}
/*
 * IsAnyLower returns TRUE if string s has lower case letters.
 */
static int IsAnyLower(const char *s)
{
	for (; *s; ++s)
		if (islower(*s))
			return TRUE;
	return FALSE;
}

/* send cancel string to get the other end to shut up */
static void canit(void)
{
	static char canistr[] = {
		24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 8, 8, 8, 8, 8, 8,
		    8, 8, 8, 8, 0
	};

	printf(canistr);
	Lleft = 0;		/* Do read next time ... */
	fflush(stdout);
}

static void report(int sct)
{
	if (Verbose > 1)
		fprintf(stderr, "Blocks received: %d\r", sct);
}

/*
 * If called as [-][dir/../]vrzCOMMAND set Verbose to 1
 * If called as [-][dir/../]rzCOMMAND set the pipe flag
 * If called as rb use YMODEM protocol
 */
static void chkinvok(char *s)
{
	char *p;

	p = s;
	while (*p == '-')
		s = ++p;
	while (*p)
		if (*p++ == '/')
			s = p;
	if (*s == 'v') {
		Verbose = 1;
		++s;
	}
	Progname = s;
	if (*s == 'l') {
		/* lrz -> rz */
		++s;
	}
	if (s[0] == 'r' && s[1] == 'z')
		Batch = TRUE;
	if (s[2] && s[0] == 'r' && s[1] == 'z')
		Topipe = 1;
}

static void checkpath(char *name)
{
	/* restrict pathnames to current tree or uucppublic */
	if (strstr(name, "../")) {
		canit();
		fprintf(stderr, "\r\nlrz:\tSecurity Violation\r\n");
		bibi(-1);
	}
	if (name[0] == '.' || strstr(name, "/.")) {
		canit();
		fprintf(stderr, "\r\nlrz:\tSecurity Violation\r\n");
		bibi(-1);
	}
}

/*
 * Initialize for Zmodem receive attempt, try to activate Zmodem sender
 *  Handles ZSINIT frame
 *  Return ZFILE if Zmodem filename received, -1 on error,
 *   ZCOMPL if transaction finished,  else 0
 */
static int tryz(void)
{
	int c, n;
	int cmdzack1flg;

	for (n = Zmodem ? 15 : 5; --n >= 0;) {
		/* Set buffer length (0) and capability flags */
		stohdr(0L);
		Txhdr[ZF0] = CANFC32 | CANFDX | CANOVIO | CANBRK;
		if (Zctlesc)
			Txhdr[ZF0] |= TESCCTL;	/* TESCCTL == ESCCTL */
		zshhdr(tryzhdrtype, Txhdr);
		if (tryzhdrtype == ZSKIP)	/* Don't skip too far */
			tryzhdrtype = ZRINIT;	/* CAF 8-21-87 */
	      again:
		switch (zgethdr(Rxhdr, 0)) {
		case ZRQINIT:
			continue;
		case ZEOF:
			continue;
		case TIMEOUT:
			continue;
		case ZFILE:
			zconv = Rxhdr[ZF0];
			zmanag = Rxhdr[ZF1];
			ztrans = Rxhdr[ZF2];
			tryzhdrtype = ZRINIT;
			c = zrdata(secbuf, MAX_BLOCK);
			mode(3);
			if (c == GOTCRCW)
				return ZFILE;
			zshhdr(ZNAK, Txhdr);
			goto again;
		case ZSINIT:
			Zctlesc = TESCCTL & Rxhdr[ZF0];
			if (zrdata(Attn, ZATTNLEN) == GOTCRCW) {
				stohdr(1L);
				zshhdr(ZACK, Txhdr);
				goto again;
			}
			zshhdr(ZNAK, Txhdr);
			goto again;
		case ZFREECNT:
			stohdr(getfree());
			zshhdr(ZACK, Txhdr);
			goto again;
		case ZCOMMAND:
			cmdzack1flg = Rxhdr[ZF0];
			if (zrdata(secbuf, MAX_BLOCK) == GOTCRCW) {
				if (Verbose) {
					fprintf(stderr,
						"lrz: remote requested command\n");
					fprintf(stderr, "lrz: %s\n",
						secbuf);
				}
				if (Verbose)
					fprintf(stderr,
						"lrz: not executed\n");
				zshhdr(ZCOMPL, Txhdr);
				return ZCOMPL;
			}
			goto again;
		case ZCOMPL:
			goto again;
		default:
			continue;
		case ZFIN:
			ackbibi();
			return ZCOMPL;
		case ZCAN:
			return ERROR;
		}
	}
	return 0;
}

/*
 * Receive 1 or more files with ZMODEM protocol
 */
static int rzfiles(void)
{
	int c;

	for (;;) {
		switch (c = rzfile()) {
		case ZEOF:
		case ZSKIP:
			switch (tryz()) {
			case ZCOMPL:
				return OK;
			default:
				return ERROR;
			case ZFILE:
				break;
			}
			continue;
		default:
			return c;
		case ERROR:
			return ERROR;
		}
	}
}

/*
 * Receive a file with ZMODEM protocol
 *  Assumes file name frame is in secbuf
 */
static int rzfile(void)
{
	int c, n;
	long last_rxbytes = 0;
	long not_printed = 0;

	last_bps = 0;
	Eofseen = FALSE;

	n = 20;
	rxbytes = 0l;

	if (procheader(secbuf) == ERROR) {
		return (tryzhdrtype = ZSKIP);
	}


	for (;;) {
		stohdr(rxbytes);
		zshhdr(ZRPOS, Txhdr);
	      nxthdr:
		switch (c = zgethdr(Rxhdr, 0)) {
		default:
			vfile("lrzfile: zgethdr returned %d", c);
			return ERROR;
		case ZNAK:
		case TIMEOUT:
			if (--n < 0) {
				vfile("lrzfile: zgethdr returned %d", c);
				return ERROR;
			}
		case ZFILE:
			zrdata(secbuf, MAX_BLOCK);
			continue;
		case ZEOF:
			if (rclhdr(Rxhdr) != rxbytes) {
				/*
				 * Ignore eof if it's at wrong place - force
				 *  a timeout because the eof might have gone
				 *  out before we sent our zrpos.
				 */
				errors = 0;
				goto nxthdr;
			}
			if (Verbose > 1) {
				int minleft = 0;
				int secleft = 0;
				last_bps = (rxbytes / timing(0));
				if (last_bps > 0) {
					minleft =
					    (Bytesleft -
					     rxbytes) / last_bps / 60;
					secleft =
					    ((Bytesleft - rxbytes) /
					     last_bps) % 60;
				}
				fprintf(stderr,
					"\rBytes Received: %7ld/%7ld   BPS:%-6d                   \r\n",
					rxbytes, Bytesleft, last_bps);
			}
			closeok = 1;
			if (closeit()) {
				tryzhdrtype = ZFERR;
				vfile("lrzfile: closeit returned <> 0");
				return ERROR;
			}
			vfile("lrzfile: normal EOF");
			return c;
		case ERROR:	/* Too much garbage in header search error */
			if (--n < 0) {
				vfile("lrzfile: zgethdr returned %d", c);
				return ERROR;
			}
			zmputs(Attn);
			continue;
		case ZSKIP:
			closeit();
			vfile("lrzfile: Sender SKIPPED file");
			return c;
		case ZDATA:
			if (rclhdr(Rxhdr) != rxbytes) {
				if (--n < 0) {
					return ERROR;
				}
				zmputs(Attn);
				continue;
			}
		      moredata:
			if (Verbose > 1
			    && (not_printed > 7
				|| rxbytes >
				last_bps / 2 + last_rxbytes)) {
				int minleft = 0;
				int secleft = 0;
				last_bps = (rxbytes / timing(0));
				if (last_bps > 0) {
					minleft =
					    (Bytesleft -
					     rxbytes) / last_bps / 60;
					secleft =
					    ((Bytesleft - rxbytes) /
					     last_bps) % 60;
				}
				fprintf(stderr,
					"\rBytes Received: %7ld/%7ld   BPS:%-6d ETA %02d:%02d  ",
					rxbytes, Bytesleft, last_bps,
					minleft, secleft);
				last_rxbytes = rxbytes;
				not_printed = 0;
				if (nodeinf != -1 && ((time(0) - lup) > 5)) {
					lseek(nodeinf, 0, SEEK_SET);
					nin.ddn_bpsrate = last_bps;
					sprintf(nin.ddn_activity,
						"UL: %-34.34s", myname);
					write(nodeinf, &nin,
					      sizeof(struct
						     DayDream_NodeInfo));
					lup = time(0);
				}
			} else if (Verbose)
				not_printed++;
			switch (c = zrdata(secbuf, MAX_BLOCK)) {
			case ZCAN:
				vfile("lrzfile: zgethdr returned %d", c);
				return ERROR;
			case ERROR:	/* CRC error */
				if (--n < 0) {
					vfile
					    ("lrzfile: zgethdr returned %d",
					     c);
					return ERROR;
				}
				zmputs(Attn);
				continue;
			case TIMEOUT:
				if (--n < 0) {
					vfile
					    ("lrzfile: zgethdr returned %d",
					     c);
					return ERROR;
				}
				continue;
			case GOTCRCW:
				n = 20;
				putsec(secbuf, Rxcount);
				rxbytes += Rxcount;
				stohdr(rxbytes);
				zshhdr(ZACK, Txhdr);
				xsendline(XON);
				goto nxthdr;
			case GOTCRCQ:
				n = 20;
				putsec(secbuf, Rxcount);
				rxbytes += Rxcount;
				stohdr(rxbytes);
				zshhdr(ZACK, Txhdr);
				goto moredata;
			case GOTCRCG:
				n = 20;
				putsec(secbuf, Rxcount);
				rxbytes += Rxcount;
				goto moredata;
			case GOTCRCE:
				n = 20;
				putsec(secbuf, Rxcount);
				rxbytes += Rxcount;
				goto nxthdr;
			}
		}
	}
}

/*
 * Send a string to the modem, processing for \336 (sleep 1 sec)
 *   and \335 (break signal)
 */
static void zmputs(char *s)
{
	char *p;

	while (s && *s) {
		p = strpbrk(s, "\335\336");
		if (!p) {
			write(iofd, s, strlen(s));
			return;
		}
		if (p != s) {
			write(iofd, s, p - s);
			s = p;
		}
		if (*p == '\336')
			sleep(1);
		else
			sendbrk();
		p++;
	}
}

/*
 * Close the receive dataset, return OK or ERROR
 */
static int closeit(void)
{
	struct timeval timep[2];
	char dszb[512];

	if (Topipe) {
		if (pclose(fout)) {
			return ERROR;
		}
		return OK;
	}
	if (fclose(fout)) {
		fprintf(stderr, "file close error: %s\n", strerror(errno));
		/* this may be any sort of error, including random data corruption */
		unlink(Pathname);
		return ERROR;
	}
	if (closeok) {
		if (dszlog != -1) {
			sprintf(dszb,
				"S %6ld %5d bps %4d cps   0 errors     0 %4d %s %u\n",
				rxbytes, 0, last_bps, 1024, myname, 0);
			write(dszlog, dszb, strlen(dszb));
		}
	} else {
		if (dszlog != -1) {
			sprintf(dszb,
				"E %6ld %5d bps %4d cps   0 errors     0 %4d %s %u\n",
				rxbytes, 0, last_bps, 1024, myname, 0);
			write(dszlog, dszb, strlen(dszb));
		}
	}
	if (Modtime) {
		timep[0].tv_sec = time(NULL);
		timep[1].tv_usec = 0;
		timep[1].tv_sec = Modtime;
		timep[1].tv_usec = 0;
		utimes(Pathname, timep);
	}
	if ((Filemode & S_IFMT) == S_IFREG)
		chmod(Pathname, (07777 & Filemode));
	return OK;
}

/*
 * Ack a ZFIN packet, let byegones be byegones
 */
static void ackbibi(void)
{
	int n;

	vfile("ackbibi:");
	Readnum = 1;
	stohdr(0L);
	for (n = 3; --n >= 0;) {
		purgeline();
		zshhdr(ZFIN, Txhdr);
		switch (readline(100)) {
		case 'O':
			readline(1);	/* Discard 2nd 'O' */
			vfile("ackbibi complete");
			return;
		case RCDO:
			return;
		case TIMEOUT:
		default:
			break;
		}
	}
}

static int findfile(char *file, char *pa)
{
	char buf1[1024];
	char buf2[1024];
	int fd1;
	FILE *plist;
	char *s;
	DIR *dh;
	struct dirent *dent;
	char de[1];
	*de = 0;

	s = file;
	while (*s) {
		if (*s == '/')
			return 0;
		s++;
	}

	if ((plist = fopen(pa, "r"))) {
		while (fgetsnolf(buf1, 512, plist)) {
			sprintf(buf2, "%s%s", buf1, file);
			if (*buf1 && ((fd1 = open(buf2, O_RDONLY)) > -1)) {
				fclose(plist);
				close(fd1);
				return 1;
			}
		}
		fclose(plist);
	}

	if ((plist = fopen(pa, "r"))) {
		while (fgetsnolf(buf1, 512, plist)) {
			if (*buf1 && (dh = opendir(buf1))) {
				while ((dent = readdir(dh))) {
					if (!strcmp(dent->d_name, ".")
					    ||
					    (!strcmp(dent->d_name, "..")))
						    continue;
					if (!strcasecmp
					    (dent->d_name, file)) {
						*de = 1;
						break;
					}
				}
				closedir(dh);
			}
		}
		fclose(plist);
	}
	if (*de)
		return 1;
	return 0;
}

static char *fgetsnolf(char *buf, int n, FILE * fh)
{
	char *hih;
	char *s;

	hih = fgets(buf, n, fh);
	if (!hih)
		return 0;
	s = buf;
	while (*s) {
		if (*s == 13 || *s == 10) {
			*s = 0;
			break;
		}
		s++;
	}
	return hih;
}

static void sendbrk(void)
{
	tcsendbreak(iofd, 1);
}
