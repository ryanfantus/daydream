#ifndef _DAYDREAM_H_INCLUDED
#define _DAYDREAM_H_INCLUDED

#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <string.h>

#include <utility.h>
#include <dd.h>
#include <global.h>
#include <md5.h>
#include <strf.h>

#include <ddmsglib.h>
#include "config.h"


struct olm {
	struct Node olm_head;
	char olm_msg[200];
	int olm_user;
	int olm_number;
};

int cmdlineupload(void);
void freefstr(char *, size_t) __attr_bounded__ (__string__, 1, 2);
void freebstr(char *, size_t) __attr_bounded__ (__string__, 1, 2);
int initbgchecker(void);
int getreplyid(int, char *, size_t) __attr_bounded__ (__string__, 2, 3);
int askqlines(void);
int ispid(pid_t);
int findfilestolist(char *, char *);
int dropcarrier(void);
int checkcarrier(void);
int clearlist(struct List *);
int visitbbs(int);
void dpause(void);
char *currt(void);
int getsec(void);
int handlectrl(int);
int changemsgbase(int, int);
void changenodestatus(const char *);
void DDPut(const char *);
int TypeFile(const char *, int);
int Prompt(char *, int, int);
void StripAnsi(unsigned char *);
void StripPipes(unsigned char *);
int CreateNewAccount (void);
int wildcmp(const char *, const char *);
int iswilds(const char *);
int findusername(const char *);
int cmppasswds(char *, unsigned char *);
void clear(void);
void enterbbs(void);
void userstats(void);
void stripansi(char *);
void versioninfo(void);
void edituser(void);
void testscreenl(void);
void userlist(char *);
void usered(void);
int joinconf(int, int);
int nextconf(void);
int entermsg(struct DayDream_Message *, int, char *);
int checkconfaccess(int, struct userbase *);
void getmsgptrs(void);
int setmsgptrs(void);
int readmessages (int, int, char *);
int lineed(char *, size_t, int, struct DayDream_Message *)
	__attr_bounded__ (__string__, 1, 2);
int cmbmenu(char *);
int nextbase(void);
int prevbase(void);
int pagesysop(const char *);
int globalread(void);
int isbasetagged(int, int);
int isanybasestagged(int);
int upload(int);
int freespace(void);
int localupload(void);
int filelist(int, char *);
void multibackspace(int bscnt);
int scanfornewmail(void);

int listtags(void);
void recountfiles(void);
void killflood(void);
int getdisplaymode(const char *, int);
void removespaces(char *);
int runlogoffbatch(void);
void recfiles(char *, char *);
int makeflist(char *, char *);
int sendfiles(char *, char *, size_t) __attr_bounded__ (__string__, 2, 3);
void Remove(struct Node *);
int isnode(int, struct DayDream_NodeInfo *);
int setprotocol(void);
int rundoorbatch(const char *batch, char *fn);
int globalnewscan(void);
int replymessage(struct DayDream_Message *);
int checkforpartialuploads(int);
int isaccess(int, int);
int bulletins(char *);
int comment(void);
int viewfile(const char *);
int tagmessageareas(void);
int tagconfs(void);
int prevconf(void);
void who(void);
int olmsg(char *);
int movefile(char *, int);
int isconftagged(int);
int genstdiocmdline(char *, const char *, const char *, const char *);
int flagsingle(char *, int);
int isfreedl(char *);
int flagfile(char *, int);
int fileattach(void);
int edfile(char *, size_t, int, struct DayDream_Message *)
	__attr_bounded__ (__string__, 1, 2);
int strlena(char *);
int fsed(char *, size_t, int, struct DayDream_Message *)
	__attr_bounded__ (__string__, 1, 2);
int analyzedszlog(char *, char *, size_t) __attr_bounded__ (__string__, 2, 3);
int maketmplist(void);
int autodisconnect(void);
char *find_file(char *, conference_t *);
int estimsecs(int);
int olmall(int, const char *);
int sendtosock(int, struct dd_nodemessage *);
int mf(char *, int);
int ddprintf(const char *, ...) __attribute__ ((format (printf, 1, 2)));

void initstringspace(void);

extern char hangupstring[512];
extern int hupmode;

extern struct List *NewList(void);
extern void AddTail(struct List *, struct Node *);


extern void MD5Init (MD5_CTX *);                  /* context */
extern void MD5Update (MD5_CTX *, unsigned char *,unsigned int);
extern void MD5Final (unsigned char[16],MD5_CTX *);

/*
 * Global Variables
 */
extern int   bgmode;
extern int   dsockfd;
extern int   serhandle;
extern int   node;
extern char  keysrc;
extern int   ansi;
extern int   carrier;
extern int   timeleft;
extern time_t endtime;
extern int   bpsrate;
extern int   lrp;
extern int   oldlrp;
extern int   lsp;
extern int   oldlsp;
extern int   highest;
extern int   lowest;
extern struct DayDream_MainConfig maincfg;

extern msgbase_t *current_msgbase;

extern struct DayDream_AccessPreset *presets;
extern struct DayDream_DisplayMode *displays;
extern struct DayDream_DisplayMode *display;
extern struct DD_Seclevel *secs;
extern struct DayDream_Archiver *arcs;
extern struct DayDream_Archiver *arc; 
extern struct DayDream_Multinode *nodes;
extern struct DayDream_Multinode *currnode;
extern struct DayDream_Protocol *protocols;
extern struct DayDream_Protocol *protocol;
extern struct callerslog clog;
extern char reason[100];

extern int output_mask;

extern int pages;
extern int access1;
extern int access2;
extern int onlinestat;
extern int userinput;
extern struct DD_ExternalCommand *exts;
extern char lrpdatname[80];
extern int delayt;
extern struct userbase user;
extern int pageflag;
extern uint8_t selcfg[2056];
extern time_t last;
extern struct List *flaggedfiles;
extern int bytestagged, fbytestagged;
extern uint16_t filestagged, ffilestagged;
extern char wrapbuf[80];
extern unsigned char inconvtab[256];
extern unsigned char outconvtab[256];
extern char *sd[MAXSTRS];
extern struct List *olms;
extern int fnode;
extern int lmode;
extern int ul_user;
extern int ul_conf;
extern char *ul_file;
extern int bgrun, wasbg; 

#define MD_CTX MD5_CTX
#define MDInit MD5Init
#define MDUpdate MD5Update
#define MDFinal MD5Final

extern int setpreset(int, struct userbase *);

/* conference.c */
conference_t *conference(void);
struct iterator *conference_iterator(void);
conference_t *findconf(int);
void read_conference_data(void);
void set_conference(conference_t *);

/* daydream.c */
extern int forcebps;
extern char *origdir;

int is_telnet_connection(void);	

/* default.c */
int load_default_commands(void);

/* domenu.c */
int primitive_docmd(int, char *);

/* doorport.c */
extern list_t *child_pid_list;

int rundoor(const char *, const char *);
int rundosdoor(const char *, int);
int runpython(const char *, const char *);

/* download.c */
int download(const char *);
int sysopdownload(const char *);

/* edituser.c */
void saveuserbase(const struct userbase *);

/* enterbbs.c */
int checkforftp(int, const char *, const char *);

/* goodbye.c */
int logoff(const char *);

/* hotkey.c */
void kill_child(pid_t pid);
unsigned char HotKey(int);

/* joinconf.c */
int joinconfmenu(const char *);

/* linechat.c */
void LineChat(void);

/* menus.c */
void init_menu_system(void);
void fini_menu_system(void);
int docmd(const char *, size_t, int) __attr_bounded__ (__string__, 1, 2);
int domenu(int);
int try_hotkey_match(const char *, size_t) __attr_bounded__ (__string__, 1, 2);
void parse_menu_command(const char **);

/* misc.c */
int isonline(int);
void *xmalloc(size_t);
void *xrealloc(void *, size_t);
size_t strspace(char *, const char *, size_t);
size_t strtoken(char *, const char **, size_t);
const char *strspa(const char *, char *, size_t);
int set_blocking_mode(int fd, int mode);
char *fgetsnolf(char *, int, FILE *) __attr_bounded__ (__string__, 1, 2);
long int str2uint(const char *, unsigned long int, unsigned long int);
const char *filepart(const char *);
void writelog(const char *);
int checklogon(const char *);

/* prompt.c */
void reset_history(void);

/* stdiohan.c */
int runstdio(const char *, int, int);
void stdioout(const char *);

/* tagedit.c */
extern int flagerror;

int addtag(const char *, const char *, int, int, int);
int dumpfilestofile(const char *);
int flagres(int, const char *, int);
int isfiletagged(const char *);
int unflagfile(const char *);
int taged(const char *);

/* textsearch.c */
int textsearch(char *);

/* typetext.c */
void ddput(const char *, size_t) __attr_bounded__ (__buffer__, 1, 2);
int formatted_print(const char *, size_t, int)
	__attr_bounded__ (__buffer__, 1, 2);
int processmsg(struct dd_nodemessage *);

/* unix.c */
extern int idleon;

int getcmdline(int, char **);
void init_keyboard(void);
void fini_keyboard(void);
int input_queue_empty(void);
int input_queue_get(void);
void keyboard_stuff(char *);
int initterm(void);

/* upload.c */
int cleantemp(void);
int getarchiver(const char *);
int newcopy(const char *, const char *);
int handleupload(const char *);
int newrename(const char *, const char *);
int getfreeulp(const char *, char *, size_t, int)
	__attr_bounded__ (__string__, 2, 3);

/* user.c */
int getubentbyid(int, struct userbase *);
int getubentbyname(const char *, struct userbase *);
int writeubent(const struct userbase *);

/* wfc.c */
int writetomodem(const char *);
void log_error(const char *, int, const char *, ...);
int waitforcall(int);

#endif /* _DAYDREAM_H_INCLUDED */
