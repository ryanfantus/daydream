/*
 *   Z M . C
 *    ZMODEM protocol primitives
 *    05-09-88  Chuck Forsberg Omen Technology Inc
 *
 * Entry point Functions:
 *	zsbhdr(type, hdr) send binary header
 *	zshhdr(type, hdr) send hex header
 *	zgethdr(hdr, eflag) receive header - binary or hex
 *	zsdata(buf, len, frameend) send data
 *	zrdata(buf, len) receive data
 *	stohdr(pos) store position data in Txhdr
 *	long rclhdr(hdr) recover position offset from header
 */

#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>
#include <termios.h>

#include <zm.h>
#include <zmodem.h>
#include <crctab.h>

int Rxtimeout = 100;		/* Tenths of seconds to wait for something */

/* Globals used by ZMODEM functions */
int Rxcount;		/* Count of data bytes received */
char Rxhdr[4];		/* Received header */
char Txhdr[4];		/* Transmitted header */
long Rxpos;		/* Received file position */
long Txpos;		/* Transmitted file position */
int Txfcs32;		/* TURE means send binary frames with 32 bit FCS */
char Attn[ZATTNLEN+1];	/* Attention string rx sends to tx on err */

static int lastsent;	/* Last char we sent */
static int Not8bit;	/* Seven bits seen on header */

int Zctlesc;			/* Encode control characters */
unsigned int Baudrate = 2400;
int errors;

static const char *frametypes[] = {
	"Carrier Lost",		/* -3 */
	"TIMEOUT",		/* -2 */
	"ERROR",		/* -1 */
#define FTOFFSET 3
	"ZRQINIT",
	"ZRINIT",
	"ZSINIT",
	"ZACK",
	"ZFILE",
	"ZSKIP",
	"ZNAK",
	"ZABORT",
	"ZFIN",
	"ZRPOS",
	"ZDATA",
	"ZEOF",
	"ZFERR",
	"ZCRC",
	"ZCHALLENGE",
	"ZCOMPL",
	"ZCAN",
	"ZFREECNT",
	"ZCOMMAND",
	"ZSTDERR",
	"xxxxx"
#define FRTYPES 22	/* Total number of frame types in this array */
			/*  not including psuedo negative entries */
};

static char badcrc[] = "Bad CRC";

static unsigned char sendbuffer[16384];
static int sendptr = 0;
static int force_no_flush = 0;

static int Rxframeind;	/* ZBIN ZBIN32, or ZHEX type of frame received */
static int Rxtype;	/* Type of header received */
static int Crc32;	/* Display flag indicating 32 bit CRC being received */
static int Crc32t;	/* Display flag indicating 32 bit CRC being sent */

static int zrdat32(char *buf, int length);
static void zsda32(char *buf, int length, int frameend);
static void zsbh32(char *hdr, int type);
static int zrhhdr(char *hdr);
static int zgethex(void);
static int zrbhdr32(char *hdr);
static void zputhex(int c, char *pos);
static int zgeth1(void);
static int zrbhdr(char *hdr);
static int noxrd7(void);
static int zdlread2(int);
static inline int zdlread(void);
static void bttyout(char);

static inline void flush_send_buffer(void)
{
	unsigned char *ptr;
	int bytes_written;

	if (force_no_flush && sendptr < sizeof(sendbuffer))
	       return;	

	ptr = sendbuffer;
	while (sendptr) {
		bytes_written = write(iofd, ptr, sendptr);
		if (bytes_written < 0)
			break;
		ptr += bytes_written;
		sendptr -= bytes_written;
	}

	tcdrain(iofd);
}

static inline void xsendline_optimized(int c)
{
	sendbuffer[sendptr++] = c & 0377;
	if (sendptr == sizeof(sendbuffer)) 
		flush_send_buffer();
}

static void zsendline_init(char *);
/*
 * Send character c with ZMODEM escape sequence encoding.
 *  Escape XON, XOFF. Escape CR following @ (Telenet net escape)
 */
static inline void zsendline(int c)
{
	static int last_esc=-2;
	static char tab[256];
	if (Zctlesc!=last_esc) {
		zsendline_init(tab);
		last_esc=Zctlesc;
	}

	switch(tab[(unsigned) (c&=0377)])
	{
	case 0: 
		xsendline_optimized(lastsent = c); 
		break;
	case 1:
		xsendline_optimized(ZDLE);
		c ^= 0100;
		xsendline_optimized(lastsent = c);
		break;
	case 2:
		if ((lastsent & 0177) != '@') {
			xsendline_optimized(lastsent = c);
		} else {
			xsendline_optimized(ZDLE);
			c ^= 0100;
			xsendline_optimized(lastsent = c);
		}
		break;
	}
	flush_send_buffer();
}

/* Send ZMODEM binary header hdr of type type */
void zsbhdr(int type, char *hdr)
{
	int n;
	unsigned short crc;

	vfile("zsbhdr: %s %lx", frametypes[type+FTOFFSET], rclhdr(hdr));
	xsendline(ZPAD); xsendline(ZDLE);

	if ((Crc32t = Txfcs32))
		zsbh32(hdr, type);
	else {
		xsendline(ZBIN); zsendline(type); crc = updcrc(type, 0);

		for (n=4; --n >= 0; ++hdr) {
			zsendline(*hdr);
			crc = updcrc((0377& *hdr), crc);
		}
		crc = updcrc(0,updcrc(0,crc));
		zsendline(crc>>8);
		zsendline(crc);
	}
}


/* Send ZMODEM binary header hdr of type type */
static void zsbh32(char *hdr, int type)
{
	int n;
	unsigned long crc;

	xsendline(ZBIN32);  zsendline(type);
	crc = 0xFFFFFFFFL; crc = UPDC32(type, crc);

	for (n=4; --n >= 0; ++hdr) {
		crc = UPDC32((0377 & *hdr), crc);
		zsendline(*hdr);
	}
	crc = ~crc;
	for (n=4; --n >= 0;) {
		zsendline((int)crc);
		crc >>= 8;
	}
}

/* Send ZMODEM HEX header hdr of type type */
void zshhdr(int type, char *hdr)
{
	int n;
	unsigned short crc;
	char s[30];
	size_t len;

	vfile("zshhdr: %s %lx", frametypes[type+FTOFFSET], rclhdr(hdr));
	s[0]=ZPAD;
	s[1]=ZPAD;
	s[2]=ZDLE;
	s[3]=ZHEX;
	zputhex(type,s+4);
	len=6;
	Crc32t = 0;

	crc = updcrc(type, 0);
	for (n=4; --n >= 0; ++hdr) {
		zputhex(*hdr,s+len); 
		len += 2;
		crc = updcrc((0377 & *hdr), crc);
	}
	crc = updcrc(0,updcrc(0,crc));
	zputhex(crc>>8,s+len); 
	zputhex(crc,s+len+2);
	len+=4;

	/* Make it printable on remote machine */
	s[len++]=015;
	s[len++]=0212;
	/*
	 * Uncork the remote in case a fake XOFF has stopped data flow
	 */
	if (type != ZFIN && type != ZACK)
	{
		s[len++]=021;
	}
	write(iofd,s,len);
}

/*
 * Send binary array buf of length length, with ending ZDLE sequence frameend
 */
static const char *Zendnames[] = { "ZCRCE", "ZCRCG", "ZCRCQ", "ZCRCW"};

void zsdata(char *buf, int length, int frameend)
{
	unsigned short crc;

	vfile("zsdata: %d %s", length, Zendnames[frameend - (ZCRCE & 3)]);
	if (Crc32t)
		zsda32(buf, length, frameend);
	else {
		crc = 0;
		for (;--length >= 0; ++buf) {
			zsendline(*buf); crc = updcrc((0377 & *buf), crc);
		}
		xsendline_optimized(ZDLE); xsendline_optimized(frameend);
		crc = updcrc(frameend, crc);

		crc = updcrc(0,updcrc(0,crc));
		zsendline(crc>>8); zsendline(crc);
	}
	if (frameend == ZCRCW) {
		xsendline_optimized(XON);  
	}
	flush_send_buffer();
}

static void zsda32(char *buf, int length, int frameend)
{
	int c;
	unsigned long crc;

	force_no_flush = 1;

	crc = 0xFFFFFFFFL;
	for (;--length >= 0; ++buf) {
		c = *buf & 0377;
		if (c & 0140)
			xsendline_optimized(lastsent = c);
		else
			zsendline(c);
		crc = UPDC32(c, crc);
	}
	xsendline_optimized(ZDLE); 
	xsendline_optimized(frameend);
	crc = UPDC32(frameend, crc);

	crc = ~crc;
	for (length=4; --length >= 0;) {
		c=(int) crc;
		if (c & 0140)
			xsendline_optimized(lastsent = c);
		else
			zsendline(c);
		crc >>= 8;
	}

	force_no_flush = 0;
	flush_send_buffer();
}

/*
 * Receive array buf of max length with ending ZDLE sequence
 *  and CRC.  Returns the ending character or error code.
 *  NB: On errors may store length+1 bytes!
 */
int zrdata(char *buf, int length)
{
	int c;
	unsigned short crc;
	char *end;
	int d;

	if (Rxframeind == ZBIN32)
		return zrdat32(buf, length);

	crc = Rxcount = 0;  end = buf + length;
	while (buf <= end) {
		if ((c = zdlread()) & ~0377) {
crcfoo:
			switch (c) {
			case GOTCRCE:
			case GOTCRCG:
			case GOTCRCQ:
			case GOTCRCW:
				crc = updcrc((d=c)&0377, crc);
				if ((c = zdlread()) & ~0377)
					goto crcfoo;
				crc = updcrc(c, crc);
				if ((c = zdlread()) & ~0377)
					goto crcfoo;
				crc = updcrc(c, crc);
				if (crc & 0xFFFF) {
					zperr(badcrc);
					return ERROR;
				}
				Rxcount = length - (end - buf);
				vfile("zrdata: %d  %s", Rxcount,
				 Zendnames[d - (GOTCRCE & 3)]);
				return d;
			case GOTCAN:
				zperr("Sender Canceled");
				return ZCAN;
			case TIMEOUT:
				zperr("TIMEOUT");
				return c;
			default:
				zperr("Bad data subpacket");
				return c;
			}
		}
		*buf++ = c;
		crc = updcrc(c, crc);
	}
	zperr("Data subpacket too long");
	return ERROR;
}

static int zrdat32(char *buf, int length)
{
	int c;
	unsigned long crc;
	char *end;
	int d;

	crc = 0xFFFFFFFFL;  Rxcount = 0;  end = buf + length;
	while (buf <= end) {
		if ((c = zdlread()) & ~0377) {
crcfoo:
			switch (c) {
			case GOTCRCE:
			case GOTCRCG:
			case GOTCRCQ:
			case GOTCRCW:
				d = c;  c &= 0377;
				crc = UPDC32(c, crc);
				if ((c = zdlread()) & ~0377)
					goto crcfoo;
				crc = UPDC32(c, crc);
				if ((c = zdlread()) & ~0377)
					goto crcfoo;
				crc = UPDC32(c, crc);
				if ((c = zdlread()) & ~0377)
					goto crcfoo;
				crc = UPDC32(c, crc);
				if ((c = zdlread()) & ~0377)
					goto crcfoo;
				crc = UPDC32(c, crc);
				if (crc != 0xDEBB20E3) {
					zperr(badcrc);
					return ERROR;
				}
				Rxcount = length - (end - buf);
				vfile("zrdat32: %d %s", Rxcount,
				 Zendnames[d-(GOTCRCE&3)]);
				return d;
			case GOTCAN:
				zperr("Sender Canceled");
				return ZCAN;
			case TIMEOUT:
				zperr("TIMEOUT");
				return c;
			default:
				zperr("Bad data subpacket");
				return c;
			}
		}
		*buf++ = c;
		crc = UPDC32(c, crc);
	}
	zperr("Data subpacket too long");
	return ERROR;
}


/*
 * Read a ZMODEM header to hdr, either binary or hex.
 *  eflag controls local display of non zmodem characters:
 *	0:  no display
 *	1:  display printing characters only
 *	2:  display all non ZMODEM characters
 *  On success, set Zmodem to 1, set Rxpos and return type of header.
 *   Otherwise return negative on error.
 *   Return ERROR instantly if ZCRCW sequence, for fast error recovery.
 */
int zgethdr(char *hdr, int eflag)
{
	int c, n, cancount;

	n = Zrwindow + Baudrate;	/* Max bytes before start of frame */
	Rxframeind = Rxtype = 0;

startover:
	cancount = 5;
again:
	/* Return immediate ERROR if ZCRCW sequence seen */
	switch (c = readline(Rxtimeout)) {
	case RCDO:
	case TIMEOUT:
		goto fifi;
	case CAN:
gotcan:
		if (--cancount <= 0) {
			c = ZCAN; goto fifi;
		}
		switch (c = readline(1)) {
		case TIMEOUT:
			goto again;
		case ZCRCW:
			c = ERROR;
		/* **** FALL THRU TO **** */
		case RCDO:
			goto fifi;
		default:
			break;
		case CAN:
			if (--cancount <= 0) {
				c = ZCAN; goto fifi;
			}
			goto again;
		}
	/* **** FALL THRU TO **** */
	default:
agn2:
		if ( --n == 0) {
			zperr("Garbage count exceeded");
			return(ERROR);
		}
		if (eflag && ((c &= 0177) & 0140))
			bttyout(c);
		else if (eflag > 1)
			bttyout(c);

		fflush(stderr);

		goto startover;
	case ZPAD|0200:		/* This is what we want. */
		Not8bit = c;
	case ZPAD:		/* This is what we want. */
		break;
	}
	cancount = 5;
splat:
	switch (c = noxrd7()) {
	case ZPAD:
		goto splat;
	case RCDO:
	case TIMEOUT:
		goto fifi;
	default:
		goto agn2;
	case ZDLE:		/* This is what we want. */
		break;
	}

	switch (c = noxrd7()) {
	case RCDO:
	case TIMEOUT:
		goto fifi;
	case ZBIN:
		Rxframeind = ZBIN;  Crc32 = FALSE;
		c =  zrbhdr(hdr);
		break;
	case ZBIN32:
		Crc32 = Rxframeind = ZBIN32;
		c =  zrbhdr32(hdr);
		break;
	case ZHEX:
		Rxframeind = ZHEX;  Crc32 = FALSE;
		c =  zrhhdr(hdr);
		break;
	case CAN:
		goto gotcan;
	default:
		goto agn2;
	}
	Rxpos = hdr[ZP3] & 0377;
	Rxpos = (Rxpos<<8) + (hdr[ZP2] & 0377);
	Rxpos = (Rxpos<<8) + (hdr[ZP1] & 0377);
	Rxpos = (Rxpos<<8) + (hdr[ZP0] & 0377);
fifi:
	switch (c) {
	case GOTCAN:
		c = ZCAN;
	/* **** FALL THRU TO **** */
	case ZNAK:
	case ZCAN:
	case ERROR:
	case TIMEOUT:
	case RCDO:
		zperr("Got %s", frametypes[c+FTOFFSET]);
	/* **** FALL THRU TO **** */
	default:
		if (c >= -3 && c <= FRTYPES)
			vfile("zgethdr: %s %lx", frametypes[c+FTOFFSET], Rxpos);
		else
			vfile("zgethdr: %d %lx", c, Rxpos);
	}
	return c;
}

/* Receive a binary style header (type and position) */
static int zrbhdr(char *hdr)
{
	int c, n;
	unsigned short crc;

	if ((c = zdlread()) & ~0377)
		return c;
	Rxtype = c;
	crc = updcrc(c, 0);

	for (n=4; --n >= 0; ++hdr) {
		if ((c = zdlread()) & ~0377)
			return c;
		crc = updcrc(c, crc);
		*hdr = c;
	}
	if ((c = zdlread()) & ~0377)
		return c;
	crc = updcrc(c, crc);
	if ((c = zdlread()) & ~0377)
		return c;
	crc = updcrc(c, crc);
	if (crc & 0xFFFF) {
		zperr(badcrc); 
		return ERROR;
	}
#ifdef ZMODEM
	Protocol = ZMODEM;
#endif
	Zmodem = 1;
	return Rxtype;
}

/* Receive a binary style header (type and position) with 32 bit FCS */
static int zrbhdr32(char *hdr)
{
	int c, n;
	unsigned long crc;

	if ((c = zdlread()) & ~0377)
		return c;
	Rxtype = c;
	crc = 0xFFFFFFFFL; crc = UPDC32(c, crc);
#ifdef DEBUGZ
	vfile("zrbhdr32 c=%X  crc=%lX", c, crc);
#endif

	for (n=4; --n >= 0; ++hdr) {
		if ((c = zdlread()) & ~0377)
			return c;
		crc = UPDC32(c, crc);
		*hdr = c;
#ifdef DEBUGZ
		vfile("zrbhdr32 c=%X  crc=%lX", c, crc);
#endif
	}
	for (n=4; --n >= 0;) {
		if ((c = zdlread()) & ~0377)
			return c;
		crc = UPDC32(c, crc);
#ifdef DEBUGZ
		vfile("zrbhdr32 c=%X  crc=%lX", c, crc);
#endif
	}
	if (crc != 0xDEBB20E3) {
		zperr(badcrc);
		return ERROR;
	}
#ifdef ZMODEM
	Protocol = ZMODEM;
#endif
	Zmodem = 1;
	return Rxtype;
}

/* Receive a hex style header (type and position) */
static int zrhhdr(char *hdr)
{
	int c;
	unsigned short crc;
	int n;

	if ((c = zgethex()) < 0)
		return c;
	Rxtype = c;
	crc = updcrc(c, 0);

	for (n=4; --n >= 0; ++hdr) {
		if ((c = zgethex()) < 0)
			return c;
		crc = updcrc(c, crc);
		*hdr = c;
	}
	if ((c = zgethex()) < 0)
		return c;
	crc = updcrc(c, crc);
	if ((c = zgethex()) < 0)
		return c;
	crc = updcrc(c, crc);
	if (crc & 0xFFFF) {
		zperr(badcrc); return ERROR;
	}
	switch ( c = readline(1)) {
	case 0215:
		Not8bit = c;
		/* **** FALL THRU TO **** */
	case 015:
	 	/* Throw away possible cr/lf */
		switch (c = readline(1)) {
		case 012:
			Not8bit |= c;
		}
	}
#ifdef ZMODEM
	Protocol = ZMODEM;
#endif
	Zmodem = 1; return Rxtype;
}

/* Send a byte as two hex digits */
static void zputhex(int c, char *pos)
{
	static char digits[] = "0123456789abcdef";

	if (Verbose>8)
		vfile("zputhex: %02X", c);
	pos[0]=digits[(c&0xF0)>>4];
	pos[1]=digits[c&0x0F];
}

static void
zsendline_init(char *tab)
{
	int i;
	for (i=0;i<256;i++) {	
		if (i & 0140)
			tab[i]=0;
		else {
			switch(i)
			{
			case ZDLE:
			case 020:
			case 021:
			case 023:
			case 0220:
			case 0221:
			case 0223:
				tab[i]=1;
				break;
			case 015:
			case 0215:
				if (Zctlesc)
					tab[i]=1;
				else
					tab[i]=2;
				break;
			default:
				if (Zctlesc)
					tab[i]=1;
				else
					tab[i]=0;
			}
		}
	}
}

/* Decode two lower case hex digits into an 8 bit byte value */
static int zgethex(void)
{
	int c;

	c = zgeth1();
	if (Verbose>8)
		vfile("zgethex: %02X", c);
	return c;
}

static int zgeth1(void)
{
	int c, n;

	if ((c = noxrd7()) < 0)
		return c;
	n = c - '0';
	if (n > 9)
		n -= ('a' - ':');
	if (n & ~0xF)
		return ERROR;
	if ((c = noxrd7()) < 0)
		return c;
	c -= '0';
	if (c > 9)
		c -= ('a' - ':');
	if (c & ~0xF)
		return ERROR;
	c += (n<<4);
	return c;
}

/*
 * Read a byte, checking for ZMODEM escape encoding
 *  including CAN*5 which represents a quick abort
 */
static inline int zdlread(void)
{
	int c;

	if ((c = READLINE_PF(Rxtimeout)) & 0140)
		return c;

	return zdlread2(c);
}

static int zdlread2(int c)
{
	goto jumpover;
again:
	/* Quick check for non control characters */
	if ((c = READLINE_PF(Rxtimeout)) & 0140)
		return c;
jumpover:
	switch (c) {
	case ZDLE:
		break;
	case 023:
	case 0223:
	case 021:
	case 0221:
		goto again;
	default:
		if (Zctlesc && !(c & 0140)) {
			goto again;
		}
		return c;
	}
again2:
	if ((c = READLINE_PF(Rxtimeout)) < 0)
		return c;
	if (c == CAN && (c = READLINE_PF(Rxtimeout)) < 0)
		return c;
	if (c == CAN && (c = READLINE_PF(Rxtimeout)) < 0)
		return c;
	if (c == CAN && (c = READLINE_PF(Rxtimeout)) < 0)
		return c;
	switch (c) {
	case CAN:
		return GOTCAN;
	case ZCRCE:
	case ZCRCG:
	case ZCRCQ:
	case ZCRCW:
		return (c | GOTOR);
	case ZRUB0:
		return 0177;
	case ZRUB1:
		return 0377;
	case 023:
	case 0223:
	case 021:
	case 0221:
		goto again2;
	default:
		if (Zctlesc && ! (c & 0140)) {
			goto again2;
		}
		if ((c & 0140) ==  0100)
			return (c ^ 0100);
		break;
	}
	if (Verbose>1)
		zperr("Bad escape sequence %x", c);
	return ERROR;
}

/*
 * Read a character from the modem line with timeout.
 *  Eat parity, XON and XOFF characters.
 */
static int noxrd7(void)
{
	int c;

	for (;;) {
		if ((c = readline(Rxtimeout)) < 0)
			return c;
		switch (c &= 0177) {
		case XON:
		case XOFF:
			continue;
		default:
			if (Zctlesc && !(c & 0140))
				continue;
		case '\r':
		case '\n':
		case ZDLE:
			return c;
		}
	}
}

/* Store long integer pos in Txhdr */
void stohdr(long pos)
{
	Txhdr[ZP0] = pos;
	Txhdr[ZP1] = pos>>8;
	Txhdr[ZP2] = pos>>16;
	Txhdr[ZP3] = pos>>24;
}

/* Recover a long integer from a header */
long rclhdr(char *hdr)
{
	long l;

	l = (hdr[ZP3] & 0377);
	l = (l << 8) | (hdr[ZP2] & 0377);
	l = (l << 8) | (hdr[ZP1] & 0377);
	l = (l << 8) | (hdr[ZP0] & 0377);
	return l;
}

void vfile(const char *fmt, ...)
{
	va_list args;
	if (Verbose > 2) {
		va_start(args, fmt);
		vfprintf(stderr, fmt, args);
		va_end(args);
		fputs("\n", stderr);
		fflush(stderr);
	}
}

void zperr(const char *fmt, ...)
{
	va_list args;
	if (Verbose <= 0)
		return;
	va_start(args, fmt);
	fprintf(stderr, "Retry %d: ", errors);
	vfprintf(stderr, fmt, args);
	va_end(args);
	fprintf(stderr, "\n");
}

static void bttyout(char c)
{
	if (Verbose)
		putc(c, stderr);
}

void xsendline(char c)
{
	write(iofd, &c, 1);
}

/* End of zm.c */
