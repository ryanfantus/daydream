#ifndef ZM_H_INCLUDED
#define ZM_H_INCLUDED

/* Ward Christensen / CP/M parameters - Don't change these! */
#define ENQ 005
#define CAN ('X'&037)
#define XOFF ('s'&037)
#define XON ('q'&037)
#define SOH 1
#define STX 2
#define EOT 4
#define ACK 6
#define NAK 025
#define CPMEOF 032
#define WANTCRC 0103		/* send C not NAK to get crc not checksum */
#define TIMEOUT (-2)
#define RCDO (-3)
#define OK 0
#define FALSE 0
#define TRUE 1
#define ERROR (-1)

extern int iofd;
extern int Zmodem;
extern int Verbose;
extern int Zctlesc;
extern unsigned int Baudrate;
extern int errors;

int readline(int);
void vfile(const char *, ...) __attribute__ ((__format__ (__printf__, 1, 2)));
void zperr(const char *, ...) __attribute__ ((__format__ (__printf__, 1, 2)));
int zgethdr(char *, int);
void stohdr(long);
int zrdata(char *, int);
void zsdata(char *, int, int);
void zshhdr(int, char *);
void zsbhdr(int, char *);
void xsendline(char);

#ifdef DDRZ
extern char *readline_ptr;
extern int Lleft;

static inline int READLINE_PF(int timeout)
{
	return (--Lleft >= 0? (*readline_ptr++ & 0377) : readline(timeout));
}
#elif defined(DDSZ)
static inline int READLINE_PF(int timeout)
{
	return readline(timeout);
}
#else
#error Either DDSZ or DDRZ preprocessor macro must be defined
#endif

double timing(int reset);

#endif /* ZM_H_INCLUDED */
