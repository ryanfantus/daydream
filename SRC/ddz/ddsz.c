#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <setjmp.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>
#include <termios.h>

#include <config.h>
#include <dd.h>
#include <ddcommon.h>
#include <zmodem.h>
#include <zm.h>
#include <rbsb.h>
#include <crctab.h>

#undef HAVE_MMAP

#if defined(HAVE_SYS_MMAN_H) && defined(HAVE_MMAP)
#  include <sys/mman.h>
static size_t mm_size;
static void *mm_addr=NULL;
#else
#  undef HAVE_MMAP
#endif

/* Ward Christensen / CP/M parameters - Don't change these! */
#define WANTG 0107	/* Send G not NAK to get nonstop batch xmsn */
#define RETRYMAX 10


#define HOWMANY 2

#define MAX_BLOCK 8192
/*
 * Attention string to be executed by receiver to interrupt streaming data
 *  when an error is detected.  A pause (0336) may be needed before the
 *  ^C (03) or after it.
 */
static char Myattn[] = { 0 };
static unsigned Txwindow;	/* Control the size of the transmitted window */
static unsigned Txwspac;	/* Spacing between zcrcq requests */
static unsigned Txwcnt;	/* Counter used to space ack requests */
static long Lrxpos;		/* Receiver's last reported offset */
static int Canseek=1; /* 1: can; 0: only rewind, -1: neither */
static int Filesleft;
static long Totalleft;
static long Filesize;
static time_t lup;
static char txbuf[MAX_BLOCK];
static long vpos = 0;		/* Number of bytes read from file */
static struct DayDream_NodeInfo nin;
static int Quiet=0;	/* overrides logic that would otherwise set verbose */
static int Ascii=0;	/* Add CR's for brain damaged programs */
static int firstsec;
static int errcnt=0;	/* number of files unreadable */
static int blklen=128;	/* length of transmitted records */
static int Optiong;	/* Let it rip no wait for sector ACK's */
static int Eofseen;	/* EOF seen on input set by zfilbuf */
static int Totsecs;	/* total number of sectors this file */
static int Filcnt=0;	/* count of number of files opened */
static unsigned Rxbuflen = 16384;	/* Receiver's max buffer length */
static int Tframlen = 0;	/* Override for tx frame length */
static int blkopt=0;	/* Override value for zmodem blklen */
static int Rxflags = 0;
static int Rxflags2 = 0;
static long bytcnt;
static int Wantfcs32 = TRUE;	/* want to send 32 bit FCS */
static char Lzconv;	/* Local ZMODEM file conversion request */
static char Lzmanag;	/* Local ZMODEM file management request */
static int Lskipnocor;
static char Lztrans;
static int Command;	/* Send a command, then exit. */
static char *Cmdstr;	/* Pointer to the command string */
static int Cmdtries = 11;
static int Cmdack1;	/* Rx ACKs command, then do it */
static int Exitcode;
static long Lastsync;	/* Last offset to which we got a ZRPOS */
static int Beenhereb4;	/* How many times we've been ZRPOS'd same place */
static int no_timeout=FALSE;
static int max_blklen = 1024;
static int start_blklen = 1024;
static int error_count;
static int dszlog;
static char myname[256];
static char listname[256];
static jmp_buf intrjmp;	/* For the interrupt on RX CAN */

#define OVERHEAD 18
#define OVER_ERR 20
#define MK_STRING(x) MK_STRING2(x)
#define MK_STRING2(x) #x

static int zsendcmd(char *buf, int blen);
static int getnak(void);
static int wcsend(const char *list);
static int wcs(char *oname);
static int wctxpn(char *name);;
static int zsendfdata(void);
static int getzrxinit(void);
static int zsendfile(char *buf, int blen);
static int calc_blklen(long total_sent);
static int sendzsinit(void);
static void usage(void);
static void saybibi(void);
static int getinsync(int);
static void countem(char *);
static void chkinvok(char *);
static int rdchk(int);
static int zfilbuf(void);

int nodeinf;
int Zmodem=0;		/* ZMODEM protocol requested by receiver */
FILE *in;
char *Progname = "sz";
int Zrwindow = 1400;	/* RX window size (controls garbage count) */
char Lastrx;
char Crcflg;
int Verbose=0;
void purgeline(void);
void canit(void);

/* called by signal interrupt or terminate to clean things up */
static void bibi(int signum)
{
	canit(); 
	mode(0);
	fprintf(stderr, "sz: caught signal %d; exiting\n", signum);
	fflush(stderr);
	exit(128 + signum);
}

int main(int argc, char **argv)
{
	char *cp;
	int npats;
	int dm;

	setbuf(stderr,0);
	dszlog=-1;

	chkinvok(argv[0]);

	Rxtimeout = 600;
	npats=0;
	if (argc<2)
		usage();
	while (--argc) {
		cp = *++argv;
		if (*cp++ == '-' && *cp) {
			while ( *cp) {
				switch(*cp++) {
				case '\\':
					 *cp = toupper(*cp);  continue;
				case '+':
					Lzmanag = ZMAPND; break;
				case '8':
					if (max_blklen==8192)
						start_blklen=8192;
					else
						max_blklen=8192;
					break;
				case 'a':
					Lzconv = ZCNL;
					Ascii = TRUE; break;
				case 'b':
					Lzconv = ZCBIN; break;
				case '@':
					if (--argc < 1) {
						usage();
					}
					strcpy(listname,*++argv);
					break;
				case 'C':
					if (--argc < 1) {
						usage();
					}
					Cmdtries = atoi(*++argv);
					break;
				case 'i':
					Cmdack1 = ZCACK1;
					/* **** FALL THROUGH TO **** */
				case 'c':
					if (--argc != 1) {
						usage();
					}
					Command = TRUE;
					Cmdstr = *++argv;
					break;
				case 'e':
					Zctlesc = 1; break;
				case 'g':
					if (--argc < 1) {
						usage();
					}
					
					iofd=open(*++argv,O_RDWR);
					break;
				case 'h':
					usage(); break;
				case 'H':
					if (--argc < 1) {
						usage();
					}
					dszlog=open(*++argv,O_WRONLY|O_TRUNC|O_CREAT,0644);
					break;
				case 'k':
					start_blklen=1024; break;
				case 'L':
					if (--argc < 1) {
						usage();
					}
					blkopt = atoi(*++argv);
					if (blkopt<24 || blkopt>MAX_BLOCK)
						usage();
					break;
				case 'l':
					if (--argc < 1) {
						usage();
					}
					Tframlen = atoi(*++argv);
					if (Tframlen<32 || Tframlen>MAX_BLOCK)
						usage();
					break;
				case 'N':
					Lzmanag = ZMNEWL;  break;
				case 'n':
					Lzmanag = ZMNEW;  break;
				case 'o':
					Wantfcs32 = FALSE; break;
				case 'O':
					no_timeout = TRUE; break;
				case 'p':
					Lzmanag = ZMPROT;  break;
				case 'r':
					Lzconv = ZCRESUM; break;
				case 'q':
					Quiet=TRUE; Verbose=0; break;
				case 't':
					if (--argc < 1) {
						usage();
					}
					Rxtimeout = atoi(*++argv);
					if (Rxtimeout<10 || Rxtimeout>1000)
						usage();
					break;
				case 'v':
					++Verbose; break;
				case 'w':
					if (--argc < 1) {
						usage();
					}
					Txwindow = atoi(*++argv);
					if (Txwindow < 256)
						Txwindow = 256;
					Txwindow = (Txwindow/64) * 64;
					Txwspac = Txwindow/4;
					if (blkopt > Txwspac
					 || (!blkopt && Txwspac < MAX_BLOCK))
						blkopt = Txwspac;
					break;
				case 'Y':
					Lskipnocor = TRUE;
					/* **** FALLL THROUGH TO **** */
				case 'y':
					Lzmanag = ZMCLOB; break;
				case 'I':
					if (--argc < 1) {
						usage();
					}
					nodeinf=open(*++argv,O_RDWR);
					read(nodeinf,&nin,sizeof(struct DayDream_NodeInfo));
					nin.ddn_flags |= (1L<<1);
					break;
				default:
					usage();
				}
			}
		}
	}

	if (!Quiet) {
		if (Verbose == 0)
			Verbose = 2;
	}
	vfile("%s %s for\n", Progname, VERSION);

	{
		/* we write max_blocklen (data) + 18 (ZModem protocol overhead)
		 * + escape overhead (about 4 %), so buffer has to be
		 * somewhat larger than max_blklen 
		 */
		char *s=malloc(max_blklen+1024);
		if (!s)
		{
			fprintf(stderr,"lsz: out of memory\n");
			exit(1);
		}
#ifdef SETVBUF_REVERSED
		setvbuf(stdout,_IOFBF,s,max_blklen+1024);
#else
		setvbuf(stdout,s,_IOFBF,max_blklen+1024);
#endif
	}
	blklen=start_blklen;

	mode(1);

	signal(SIGTERM, bibi);

	printf("rz\r");  
	fflush(stdout);

	countem(listname);
	stohdr(0L);
	if (Command)
		Txhdr[ZF0] = ZCOMMAND;
	zshhdr(ZRQINIT, Txhdr);
	fflush(stdout);

	if (Command) {
		if (getzrxinit()) {
			Exitcode=0200; canit();
		} else if (zsendcmd(Cmdstr, 1+strlen(Cmdstr))) {
			Exitcode=0200; canit();
		}
	} else if (wcsend(listname)==ERROR) {
		Exitcode=0200;
		canit();
	}
	fflush(stdout);
	mode(0);
	dm = ((errcnt != 0) | Exitcode);
	if (dm) 
		exit(dm);
	
	exit(0);
	/*NOTREACHED*/
}

static char *fgetsm(char *a1, int a2, FILE *a3)
{
	if (!(fgets(a1,a2,a3))) return 0;
	while (*a1) if (*a1==13 || *a1==10) *a1=0; else a1++;
	return a1;
}

static int wcsend(const char *list)
{
	char munb[500];
	FILE *listh;
	
	if (!(listh=fopen(list,"r"))) return ERROR;
		
	Crcflg=FALSE;
	firstsec=TRUE;
	bytcnt = -1;

	while(fgetsm(munb,500,listh)) {
		Totsecs = 0;
		if (wcs(munb)==ERROR)
			return ERROR;
	}
	fclose(listh);
	Totsecs = 0;
	if (Filcnt==0) {	/* bitch if we couldn't open ANY files */
		Command = TRUE;
		Cmdstr = "echo \"lsz: Can't open any requested files\"";
		if (getnak()) {
			Exitcode=0200; canit();
		}
		if (!Zmodem)
			canit();
		else if (zsendcmd(Cmdstr, 1+strlen(Cmdstr))) {
			Exitcode=0200; canit();
		}
		Exitcode = 1; 
		return OK;
	}
	if (Zmodem)
		saybibi();
	else 
		wctxpn("");
	return OK;
}

static int wcs(char *oname)
{
	int c;
	char *p;
	struct stat f;
	char name[PATH_MAX];

	if (strlcpy(name, oname, sizeof name) >= sizeof name)
		return ERROR;

	/* restrict pathnames to current tree or uucppublic */
	if (strstr(name, "../")) {
		canit();
		fprintf(stderr,"\r\nlsz:\tSecurity Violation\r\n");
		return ERROR;
	}

	if ( !strcmp(oname, "-")) {
		if ((p = getenv("ONAME")) && *p)
			strcpy(name, p);
		else
			sprintf(name, "s%d.lsz", getpid());
		in = stdin;
	}
	else if ((in=fopen(oname, "r"))==NULL) {
		++errcnt;
		return OK;	/* pass over it, there may be others */
	}
	{
		static char *s=NULL;
		if (!s) {
			s=malloc(16384);
			if (!s) {
				fprintf(stderr,"lsz: out of memory\n");
				exit(1);
			}
		}
#ifdef SETVBUF_REVERSED
		setvbuf(in,_IOFBF,s,16384);
#else
		setvbuf(in,s,_IOFBF,16384);
#endif
	}
	timing(1);
	Eofseen = 0;  vpos = 0;
	/* Check for directory or block special files */
	fstat(fileno(in), &f);
	c = f.st_mode & S_IFMT;
	if (c == S_IFDIR || c == S_IFBLK) {
		fclose(in);
		return OK;
	}

	++Filcnt;
	switch (wctxpn(name)) {
	case ERROR:
		return ERROR;
	case ZSKIP:
		return OK;
	}
	return 0;
}

/*
 * generate and transmit pathname block consisting of
 *  pathname (null terminated),
 *  file length, mode time and file mode in octal
 *  as provided by the Unix fstat call.
 *  N.B.: modifies the passed name, may extend it!
 */
static int wctxpn(char *name)
{
	char *p, *q;
	char *s;
	struct stat f;

	s=name;
	while(*s) s++;
	while(s!=name && *s!='/') s--;
	if (*s=='/') s++;
	strcpy(myname,s);
	
	if (!Zmodem) 
		if (getnak())
			return ERROR;

	q = (char *) 0;
	for (p=name, q=txbuf ; *p; )
		if ((*q++ = *p++) == '/')
			q = txbuf;
	*q++ = 0;
	p=q;
	while (q < (txbuf + MAX_BLOCK))
		*q++ = 0;
	if (!Ascii && (in!=stdin) && *name && fstat(fileno(in), &f)!= -1)
		sprintf(p, "%lu %lo %o 0 %d %ld", (long) f.st_size, f.st_mtime,
		  f.st_mode, Filesleft, Totalleft);
	fprintf(stderr, "Sending: %s\n",name);
	fflush(stderr);

	Totalleft -= f.st_size;
	Filesize = f.st_size;
	if (--Filesleft <= 0)
		Totalleft = 0;
	if (Totalleft < 0)
		Totalleft = 0;

	/* force 1k blocks if name won't fit in 128 byte block */
	if (txbuf[125])
		blklen=1024;
	else {		/* A little goodie for IMP/KMD */
		txbuf[127] = (f.st_size + 127) >>7;
		txbuf[126] = (f.st_size + 127) >>15;
	}
	return zsendfile(txbuf, 1+strlen(p)+(p-txbuf));
}

static int getnak(void)
{
	int firstch;

	Lastrx = 0;
	for (;;) {
		switch (firstch = readline(800)) {
		case ZPAD:
			if (getzrxinit())
				return ERROR;
			Ascii = 0;	/* Receiver does the conversion */
			return FALSE;
		case TIMEOUT:
			zperr("Timeout on pathname");
			return TRUE;
		case WANTG:
			mode(2);	/* Set cbreak, XON/XOFF, etc. */
			Optiong = TRUE;
			blklen=1024;
		case WANTCRC:
			Crcflg = TRUE;
		case NAK:
			return FALSE;
		case CAN:
			if ((firstch = readline(20)) == CAN && Lastrx == CAN)
				return TRUE;
		default:
			break;
		}
		Lastrx = firstch;
	}
}

/* Fill buffer with blklen chars */
static int zfilbuf(void)
{
	int n;

	n = fread(txbuf, 1, blklen, in);
	if (n < blklen)
		Eofseen = 1;
	return n;
}

static void alrm(int signum)
{
}

/*
 * readline(timeout) reads character(s) from file descriptor 0
 * timeout is in tenths of seconds
 */
int readline(int timeout)
{
	int c;
	static char buf[64];
	static char *bufptr=buf;
	static int bufleft=0;

	if (timeout==-1)
	{
		bufleft=0;
		return 0;
	}

	if (bufleft)
	{
		c=*bufptr & 0377;
		bufptr++;
		bufleft--;
		if (Verbose>5)
			fprintf(stderr, "ret %x\n", c);
		return c;
	}

	if (!no_timeout) {
		c = timeout/10;
		if (c<2)
			c=2;
		if (Verbose>5) {
			fprintf(stderr, "Timeout=%d Calling alarm(%d) ", timeout, c);
		}
		signal(SIGALRM, alrm); alarm(c);
	} else if (Verbose>5) 
		fprintf(stderr, "Calling read ");
	bufleft=read(iofd, buf, sizeof(buf));
	if (!no_timeout)
		alarm(0);
	if (Verbose>5)
		fprintf(stderr, "ret %x\n", buf[0]);
	if (bufleft<1)
		return TIMEOUT;
	bufptr=buf+1;
	bufleft--;

	return (buf[0]&0377);
}

void purgeline(void)
{
	readline(-1);
	tcflush(iofd, TCIFLUSH);
}

/* send cancel string to get the other end to shut up */
void canit(void)
{
	static char canistr[] = {
	 24,24,24,24,24,24,24,24,24,24,8,8,8,8,8,8,8,8,8,8,0
	};

	printf(canistr);
	fflush(stdout);
}

static const char *babble[] = {
	"Send file(s) with ZMODEM/YMODEM/XMODEM Protocol",
	"	(Y) = Option applies to YMODEM only",
	"	(Z) = Option applies to ZMODEM only",
	"Usage:	lsz [-2+abdefkLlNnquvwYy] [-] file ...",
	"	lsz [-2Ceqv] -c COMMAND",
	"	lsb [-2adfkquv] [-] file ...",
	"	lsx [-2akquv] [-] file",
	"	+ Append to existing destination file (Z)",
	"	a (ASCII) change NL to CR/LF",
	"	b Binary file transfer override",
	"	c send COMMAND (Z)",
	"	e Escape all control characters (Z)",
	"	i send COMMAND, ack Immediately (Z)",
	"	h Print this usage message",
	"	k Send 1024 byte packets (Y)",
	"	L N Limit subpacket length to N bytes (Z)",
	"	l N Limit frame length to N bytes (l>=L) (Z)",
	"	n send file if source newer (Z)",
	"	N send file if source newer or longer (Z)",
	"	o Use 16 bit CRC instead of 32 bit CRC (Z)",
	"	p Protect existing destination file (Z)",
	"	r Resume/Recover interrupted file transfer (Z)",
	"	q Quiet (no progress reports)",
	"	u Unlink file after transmission",
	"	v Verbose - provide debugging information",
	"	w N Window is N bytes (Z)",
	"	Y Yes, overwrite existing file, skip if not present at rx (Z)",
	"	y Yes, overwrite existing file (Z)",
	"	- as pathname sends standard input",
	""
};

static void usage(void)
{
	const char **pp;

	for (pp=babble; **pp; ++pp)
		fprintf(stderr, "%s\n", *pp);
	fprintf(stderr, "\t%s version %s\n", Progname, VERSION);
		
	exit(0);
}

/*
 * Get the receiver's init parameters
 */
static int getzrxinit(void)
{
	int n;
	struct stat f;

	for (n=10; --n>=0; ) {
		
		switch (zgethdr(Rxhdr, 1)) {
		case ZCHALLENGE:	/* Echo receiver's challenge numbr */
			stohdr(Rxpos);
			zshhdr(ZACK, Txhdr);
			continue;
		case ZCOMMAND:		/* They didn't see out ZRQINIT */
			stohdr(0L);
			zshhdr(ZRQINIT, Txhdr);
			continue;
		case ZRINIT:
			Rxflags = 0377 & Rxhdr[ZF0];
			Rxflags2 = 0377 & Rxhdr[ZF1];
			Txfcs32 = (Wantfcs32 && (Rxflags & CANFC32));
			Zctlesc |= Rxflags & TESCCTL;
			Rxbuflen = (0377 & Rxhdr[ZP0])+((0377 & Rxhdr[ZP1])<<8);
			if ( !(Rxflags & CANFDX))
				Txwindow = 0;
			vfile("Rxbuflen=%d Tframlen=%d", Rxbuflen, Tframlen);
			mode(2);	/* Set cbreak, XON/XOFF, etc. */
			/* Override to force shorter frame length */
			if (Rxbuflen && (Rxbuflen>Tframlen) && (Tframlen>=32))
				Rxbuflen = Tframlen;
			if ( !Rxbuflen && (Tframlen>=32) && (Tframlen<=MAX_BLOCK))
				Rxbuflen = Tframlen;
			vfile("Rxbuflen=%d", Rxbuflen);

			/* If using a pipe for testing set lower buf len */
			fstat(iofd, &f);
			if ((f.st_mode & S_IFMT) != S_IFCHR) {
				Rxbuflen = MAX_BLOCK;
			}
			/*
			 * If input is not a regular file, force ACK's to
			 *  prevent running beyond the buffer limits
			 */
			if ( !Command) {
				fstat(fileno(in), &f);
				if ((f.st_mode & S_IFMT) != S_IFREG) {
					Canseek = -1;
					return ERROR;
				}
			}
			/* Set initial subpacket length */
			if (blklen < 1024) {	/* Command line override? */
				if (Baudrate > 300)
					blklen = 256;
				if (Baudrate > 1200)
					blklen = 512;
				if (Baudrate > 2400)
					blklen = 1024;
			}
			if (Rxbuflen && blklen>Rxbuflen)
				blklen = Rxbuflen;
			if (blkopt && blklen > blkopt)
				blklen = blkopt;
			vfile("Rxbuflen=%d blklen=%d", Rxbuflen, blklen);
			vfile("Txwindow = %u Txwspac = %d", Txwindow, Txwspac);

			return (sendzsinit());
		case ZCAN:
		case TIMEOUT:
			return ERROR;
		case ZRQINIT:
			if (Rxhdr[ZF0] == ZCOMMAND)
				continue;
		default:
			zshhdr(ZNAK, Txhdr);
			continue;
		}
	}
	return ERROR;
}

/* Send send-init information */
static int sendzsinit(void)
{
	int c;

	if (Myattn[0] == '\0' && (!Zctlesc || (Rxflags & TESCCTL)))
		return OK;
	errors = 0;
	for (;;) {
		stohdr(0L);
		if (Zctlesc) {
			Txhdr[ZF0] |= TESCCTL; zshhdr(ZSINIT, Txhdr);
		}
		else
			zsbhdr(ZSINIT, Txhdr);
		zsdata(Myattn, 1+strlen(Myattn), ZCRCW);
		c = zgethdr(Rxhdr, 1);
		switch (c) {
		case ZCAN:
			return ERROR;
		case ZACK:
			return OK;
		default:
			if (++errors > 19)
				return ERROR;
			continue;
		}
	}
}

/* Send file name and related info */
static int zsendfile(char *buf, int blen)
{
	int c;
	unsigned long crc;

	for (;;) {
		Txhdr[ZF0] = Lzconv;	/* file conversion request */
		Txhdr[ZF1] = Lzmanag;	/* file management request */
		if (Lskipnocor)
			Txhdr[ZF1] |= ZMSKNOLOC;
		Txhdr[ZF2] = Lztrans;	/* file transport request */
		Txhdr[ZF3] = 0;
		zsbhdr(ZFILE, Txhdr);
		zsdata(buf, blen, ZCRCW);
again:
		c = zgethdr(Rxhdr, 1);
		switch (c) {
		case ZRINIT:
			while ((c = readline(50)) > 0)
				if (c == ZPAD) {
					goto again;
				}
			/* **** FALL THRU TO **** */
		default:
			continue;
		case ZCAN:
		case TIMEOUT:
		case ZABORT:
		case ZFIN:
			return ERROR;
		case ZCRC:
			crc = 0xFFFFFFFFL;
#ifdef HAVE_MMAP
			if (mm_addr) {
				size_t i;
				char *p=mm_addr;
				for (i=0;i<Rxpos && i<mm_size;i++,p++) {
					crc = UPDC32(*p, crc);
				}
				crc = ~crc;
			} else
#endif
			if (Canseek >= 0) {
				while (((c = getc(in)) != EOF) && --Rxpos)
					crc = UPDC32(c, crc);
				crc = ~crc;
				clearerr(in);	/* Clear EOF */
				fseek(in, 0L, 0);
			}
			stohdr(crc);
			zsbhdr(ZCRC, Txhdr);
			goto again;
		case ZSKIP:
			if (in)
				fclose(in);
			return c;
		case ZRPOS:
			/*
			 * Suppress zcrcw request otherwise triggered by
			 * lastyunc==bytcnt
			 */
#ifdef HAVE_MMAP
			if (!mm_addr)
#endif
			if (Rxpos && fseek(in, Rxpos, 0))
				return ERROR;
			bytcnt = Txpos = Rxpos;
			Lastsync = Rxpos -1;
			return zsendfdata();
		}
	}
}

/* Send the data in the file */
static int zsendfdata(void)
{
	int e, n;
	int c = 0;
	int newcnt;
	long tcount = 0;
	int junkcount;		/* Counts garbage chars received by TX */
	long last_txpos=0;
	long last_bps=0;
	long not_printed=0;
	static long total_sent=0;

#ifdef HAVE_MMAP
	{
		struct stat st;
		if (fstat(fileno(in),&st)==0)
		{
			mm_size=st.st_size;
	    		mm_addr = mmap (0, mm_size, PROT_READ, 
	    			MAP_SHARED, fileno(in), 0);
	    		if ((caddr_t) mm_addr==(caddr_t) -1)
	    			mm_addr=NULL;
	    		else {
	    			fclose(in);
	    			in=NULL;
	    		}
		}
	}
#endif

	Lrxpos = 0;
	junkcount = 0;
	Beenhereb4 = 0;
somemore:
	if (setjmp(intrjmp)) {
waitack:
		junkcount = 0;
		c = getinsync(0);
gotack:
		switch (c) {
		default:
		case ZCAN:
			if (in)
				fclose(in);
			return ERROR;
		case ZSKIP:
			if (in)
				fclose(in);
			return c;
		case ZACK:
		case ZRPOS:
			break;
		case ZRINIT:
			return OK;
		}
		/*
		 * If the reverse channel can be tested for data,
		 *  this logic may be used to detect error packets
		 *  sent by the receiver, in place of setjmp/longjmp
		 *  rdchk(fdes) returns non 0 if a character is available
		 */
		while (rdchk(iofd)) {
			switch (readline(1)) {
			case CAN:
			case ZPAD:
				c = getinsync(1);
				goto gotack;
			case XOFF:		/* Wait a while for an XON */
				readline(100);
			}
		}
	}

	newcnt = Rxbuflen;
	Txwcnt = 0;
	stohdr(Txpos);
	zsbhdr(ZDATA, Txhdr);

	do {
		int old=blklen;
		blklen=calc_blklen(total_sent);
		total_sent+=blklen+OVERHEAD;
		if (Verbose >2 && blklen!=old)
			fprintf(stderr,"blklen now %d\n",blklen);
#ifdef HAVE_MMAP
		if (mm_addr) {
			if (Txpos+blklen<mm_size) 
				n=blklen;
			else {
				n=mm_size-Txpos;
				Eofseen=1;
			}
		} else 
#endif
		n = zfilbuf();
		if (Eofseen)
			e = ZCRCE;
		else if (junkcount > 3)
			e = ZCRCW;
		else if (bytcnt == Lastsync)
			e = ZCRCW;
		else if (Rxbuflen && (newcnt -= n) <= 0)
			e = ZCRCW;
		else if (Txwindow && (Txwcnt += n) >= Txwspac) {
			Txwcnt = 0;  e = ZCRCQ;
		}
		else
			e = ZCRCG;
		if (Verbose>1
			&& (not_printed > 5 || Txpos > last_bps / 2 + last_txpos)) {
			int minleft =  0;
			int secleft =  0;
			last_bps=(Txpos/timing(0));
			if (last_bps > 0) {
				minleft =  (Filesize-Txpos)/last_bps/60;
				secleft =  ((Filesize-Txpos)/last_bps)%60;
			}
			fprintf(stderr, "\rBytes Sent:%7ld/%7ld   BPS:%-6ld ETA %02d:%02d  ",
			 Txpos, Filesize, last_bps, minleft, secleft);
			fflush(stderr);
			if ( (nodeinf!=-1) && ((time(0)-lup) > 5)) {  
				lseek(nodeinf,0,SEEK_SET);
				nin.ddn_bpsrate=last_bps;
				sprintf(nin.ddn_activity,"DL: %-34.34s",myname);
				write(nodeinf,&nin,sizeof(struct DayDream_NodeInfo));
				lup=time(0);
			}

			last_txpos=Txpos;
		} else if (Verbose)
			not_printed++;
#ifdef HAVE_MMAP
		if (mm_addr)
			zsdata(mm_addr+Txpos,n,e);
		else
#endif
		zsdata(txbuf, n, e);
		bytcnt = Txpos += n;
		if (e == ZCRCW)
			goto waitack;
		/*
		 * If the reverse channel can be tested for data,
		 *  this logic may be used to detect error packets
		 *  sent by the receiver, in place of setjmp/longjmp
		 *  rdchk(fdes) returns non 0 if a character is available
		 */
		while (rdchk(iofd)) {
			switch (readline(1)) {
			case CAN:
			case ZPAD:
				c = getinsync(1);
				if (c == ZACK)
					break;
				tcflush(iofd, TCOFLUSH);
				/* zcrce - dinna wanna starta ping-pong game */
				zsdata(txbuf, 0, ZCRCE);
				goto gotack;
			case XOFF:		/* Wait a while for an XON */
				readline(100);
			default:
				++junkcount;
			}
		}
		if (Txwindow) {
			while ((tcount = Txpos - Lrxpos) >= Txwindow) {
				vfile("%ld window >= %u", tcount, Txwindow);
				if (e != ZCRCQ)
					zsdata(txbuf, 0, e = ZCRCQ);
				c = getinsync(1);
				if (c != ZACK) {
					tcflush(iofd, TCOFLUSH);
					zsdata(txbuf, 0, ZCRCE);
					goto gotack;
				}
			}
			vfile("window = %ld", tcount);
		}
	} while (!Eofseen);
	if (Verbose > 1)
		fprintf(stderr, "\rBytes Sent:%7ld   BPS:%-6ld                       \n",
		Filesize,last_bps);
		fflush(stderr);

		if (dszlog!=-1) {
			char kbu[512];
			sprintf(kbu,"s %6ld %5d bps %4ld cps   0 errors     0 %4d %s %u\n",Filesize,0,last_bps,1024,myname,0);
			write(dszlog,kbu,strlen(kbu));
		}

	for (;;) {
		stohdr(Txpos);
		zsbhdr(ZEOF, Txhdr);
		switch (getinsync(0)) {
		case ZACK:
			continue;
		case ZRPOS:
			goto somemore;
		case ZRINIT:
			return OK;
		case ZSKIP:
			if (in)
				fclose(in);
			return c;
		default:
			if (in)
				fclose(in);
			return ERROR;
		}
	}
}

static int calc_blklen(long total_sent)
{
	static long total_bytes=0;
	static int calcs_done=0;
	static long last_error_count=0;
	static int last_blklen=0;
	static long last_bytes_per_error=0;
	long best_bytes=0;
	long best_size=0;
	long bytes_per_error;
	long d;
	int i;
	if (total_bytes==0)
	{
		/* called from countem */
		total_bytes=total_sent;
		return 0;
	}

	/* it's not good to calc blklen too early */
	if (calcs_done++ < 5) {
		if (error_count && start_blklen >1024)
			return last_blklen=1024;
		else 
			last_blklen/=2;
		return last_blklen=start_blklen;
	}

	if (!error_count) {
		/* that's fine */
		if (start_blklen==max_blklen)
			return start_blklen;
		bytes_per_error=LONG_MAX;
		goto calcit;
	}

	if (error_count!=last_error_count) {
		/* the last block was bad. shorten blocks until one block is
		 * ok. this is because very often many errors come in an
		 * short period */
		if (error_count & 2)
		{
			last_blklen/=2;
			if (last_blklen < 32)
				last_blklen = 32;
			else if (last_blklen > 512)
				last_blklen=512;
			if (Verbose > 3)
				fprintf(stderr,"calc_blklen: reduced to %d due to error\n",
					last_blklen);
			last_error_count=error_count;
			last_bytes_per_error=0; /* force recalc */
		}
		return last_blklen;
	}

	bytes_per_error=total_sent / error_count;
		/* we do not get told about every error! 
		 * from my experience the value is ok */
	bytes_per_error/=2;
	/* there has to be a margin */
	if (bytes_per_error<100)
		bytes_per_error=100;

	/* be nice to the poor machine and do the complicated things not
	 * too often
	 */
	if (last_bytes_per_error>bytes_per_error)
		d=last_bytes_per_error-bytes_per_error;
	else
		d=bytes_per_error-last_bytes_per_error;
	if (d<4)
	{
		if (Verbose > 3)
		{
			fprintf(stderr,"calc_blklen: returned old value %d due to low bpe diff\n",
				last_blklen);
			fprintf(stderr,"calc_blklen: old %ld, new %ld, d %ld\n",
				last_bytes_per_error,bytes_per_error,d );
		}
		return last_blklen;
	}
	last_bytes_per_error=bytes_per_error;

calcit:
	if (Verbose > 3)
		fprintf(stderr,"calc_blklen: calc total_bytes=%ld, bpe=%ld\n",
			total_bytes,bytes_per_error);
	for (i=32;i<=max_blklen;i*=2) {
		long ok; /* some many ok blocks do we need */
		long failed; /* and that's the number of blocks not transmitted ok */
		long transmitted;
		ok=total_bytes / i + 1;
		failed=((long) i + OVERHEAD) * ok / bytes_per_error;
		transmitted=ok * ((long) i+OVERHEAD)  
			+ failed * ((long) i+OVERHEAD+OVER_ERR);
		if (Verbose > 4)
			fprintf(stderr,"calc_blklen: blklen %d, ok %ld, failed %ld -> %ld\n",
				i,ok,failed,transmitted);
		if (transmitted < best_bytes || !best_bytes)
		{
			best_bytes=transmitted;
			best_size=i;
		}
	}
	if (best_size > 2*last_blklen)
		best_size=2*last_blklen;
	last_blklen=best_size;
	if (Verbose > 3)
		fprintf(stderr,"calc_blklen: returned %d as best\n",
			last_blklen);
	return last_blklen;
}

/*
 * Respond to receiver's complaint, get back in sync with receiver
 */
static int getinsync(int flag)
{
	int c;

	for (;;) {
		c = zgethdr(Rxhdr, 0);
		switch (c) {
		case ZCAN:
		case ZABORT:
		case ZFIN:
		case TIMEOUT:
			return ERROR;
		case ZRPOS:
			/* ************************************* */
			/*  If sending to a buffered modem, you  */
			/*   might send a break at this point to */
			/*   dump the modem's buffer.		 */
			clearerr(in);	/* In case file EOF seen */
#ifdef HAVE_MMAP
			if (!mm_addr)
#endif
			if (fseek(in, Rxpos, 0))
				return ERROR;
			Eofseen = 0;
			bytcnt = Lrxpos = Txpos = Rxpos;
			if (Lastsync == Rxpos) 
				error_count++;
			
			Lastsync = Rxpos-1;
			return c;
		case ZACK:
			Lrxpos = Rxpos;
			if (flag || Txpos == Rxpos)
				return ZACK;
			continue;
		case ZRINIT:
		case ZSKIP:
			if (in)
				fclose(in);
			return c;
		case ERROR:
		default:
			error_count++;
			zsbhdr(ZNAK, Txhdr);
			continue;
		}
	}
}


/* Say "bibi" to the receiver, try to do it cleanly */
static void saybibi(void)
{
	for (;;) {
		stohdr(0L);		/* CAF Was zsbhdr - minor change */
		zshhdr(ZFIN, Txhdr);	/*  to make debugging easier */
		switch (zgethdr(Rxhdr, 0)) {
		case ZFIN:
			xsendline('O'); xsendline('O'); 
		case ZCAN:
		case TIMEOUT:
			return;
		}
	}
}

/* Send command and related info */
static int zsendcmd(char *buf, int blen)
{
	int c;
	long cmdnum;

	cmdnum = getpid();
	errors = 0;
	for (;;) {
		stohdr(cmdnum);
		Txhdr[ZF0] = Cmdack1;
		zsbhdr(ZCOMMAND, Txhdr);
		zsdata(buf, blen, ZCRCW);
listen:
		Rxtimeout = 100;		/* Ten second wait for resp. */
		c = zgethdr(Rxhdr, 1);

		switch (c) {
		case ZRINIT:
			goto listen;	/* CAF 8-21-87 */
		case ERROR:
		case TIMEOUT:
			if (++errors > Cmdtries)
				return ERROR;
			continue;
		case ZCAN:
		case ZABORT:
		case ZFIN:
		case ZSKIP:
		case ZRPOS:
			return ERROR;
		default:
			if (++errors > 20)
				return ERROR;
			continue;
		case ZCOMPL:
			Exitcode = Rxpos;
			saybibi();
			return OK;
		case ZRQINIT:
			vfile("******** RZ *******");
			system("rz");
			vfile("******** SZ *******");
			goto listen;
		}
	}
}

/*
 * If called as lsb use YMODEM protocol
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
		Verbose=1; ++s;
	}
	Progname = s;
}

static void countem(char *list)
{
	int c;
	struct stat f;
	FILE *listh;
	char munb[300];
	Totalleft = 0; Filesleft = 0; 
	if (!(listh=fopen(list,"r"))) return;
	while(fgetsm(munb,300,listh))
	{
		f.st_size = -1;
		if (Verbose>2) {
			fprintf(stderr, "\nCountem: %s ", munb);
			fflush(stderr);
		}
		if (access(munb, 04) >= 0 && stat(munb, &f) >= 0) {
			c = f.st_mode & S_IFMT;
			if (c != S_IFDIR && c != S_IFBLK) {
				++Filesleft;  Totalleft += f.st_size;
			}
		}
		if (Verbose>2)
			fprintf(stderr, " %ld", (long) f.st_size);
	}
	fclose(listh);
	if (Verbose>2)
		fprintf(stderr, "\ncountem: Total %d %ld\n",
		  Filesleft, Totalleft);
	calc_blklen(Totalleft);
}

static int rdchk(int fd)
{
	fd_set rset;
	struct timeval tv;

	FD_ZERO(&rset);
	FD_SET(fd, &rset);
	tv.tv_sec = 0;
	tv.tv_usec = 0;
	return select(fd + 1, &rset, NULL, NULL, &tv) > 0;
}
