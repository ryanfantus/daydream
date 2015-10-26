#ifndef _DDLIB_H_INCLUDED
#define _DDLIB_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <signal.h>

#include <dd.h>
#include <ddmsglib.h>

struct dif {
	struct DayDream_DoorMsg dif_dm;
	int dif_sockfd;
};

struct dif *dd_initdoor(char *);
void dd_close(struct dif *);
void dd_sendstring(struct dif *, const char *);
int dd_sendfmt(struct dif *, const char *, ...);
int dd_prompt(struct dif *, char *, int, int);
int dd_hotkey(struct dif *, int);
int dd_typefile(struct dif *, const char *, int);
int dd_changestatus(struct dif *, char *);
void dd_pause(struct dif *);
int dd_joinconf(struct dif *, int, int);
int dd_isfreedl(struct dif *, char *);
int dd_flagfile(struct dif *, char *, int);
void dd_getstrval(struct dif *, char *, int);
void dd_getstrlval(struct dif *, char *, size_t, int);
void dd_setstrval(struct dif *, char *, int);
int dd_getintval(struct dif *, int);
void dd_setintval(struct dif *, int, int);
uint64_t dd_getlintval(struct dif *, int);
void dd_setlintval(struct dif *, int, uint64_t);
struct DayDream_Conference *dd_getconf(int);
struct DayDream_MsgBase *dd_getbase(int, int);
void dd_getlprs(struct dif *, struct DayDream_LRP *);
void dd_setlprs(struct dif *, struct DayDream_LRP *);
struct DayDream_Conference *dd_getconfdata(void);
int dd_isconfaccess(struct dif *, int);
int dd_isanybasestagged(struct dif *, int);
int dd_isconftagged(struct dif *, int);
int dd_isbasetagged(struct dif *, int, int);
void dd_sendfiles(struct dif *, char *);
void dd_getfiles(struct dif *, char *);
int dd_unflagfile(struct dif *, char *);
int dd_findfilestolist(struct dif *, char *, char *);
int dd_dumpfilestofile(struct dif *, char *);
int dd_changemsgbase(struct dif *, int, int);
int dd_getfidounique(void);
int dd_fileattach(struct dif *);
int dd_findusername(struct dif *, char *);
int dd_flagsingle(struct dif *, char *, int);
int dd_docmd(struct dif *, char *);
int dd_isfiletagged(struct dif *, char *);
int dd_system(struct dif *, char *, int);
int dd_writelog(struct dif *, char *);
void dd_getmprs(struct dif *, struct DayDream_MsgPointers *);
void dd_setmprs(struct dif *, struct DayDream_MsgPointers *);
void dd_outputmask(struct dif *, int);
void dd_ansi_fg(char *, int);
void dd_ansi_bg(char *, int);
void dd_parsepipes(char *);
void dd_strippipes(char *);
void dd_stripansi(char *);
void dd_ansipos(struct dif *, int, int);
void dd_clrscr(struct dif *);
void dd_center(struct dif *, char *);
int dd_strlenansi(char *);
char *dd_stripcrlf(char *);
void dd_cursoron(struct dif *);
void dd_cursoroff(struct dif *);

#define BBS_NAME 100
#define BBS_SYSOP 101
#define USER_REALNAME 102
#define USER_HANDLE 103
#define USER_ORGANIZATION 104
#define USER_ZIPCITY 105
#define USER_VOICEPHONE 106
#define USER_COMPUTERMODEL 107
#define USER_SIGNATURE 108
#define USER_SCREENLENGTH 109
#define USER_TOGGLES 110
#define USER_ULFILES 113
#define USER_DLFILES 114
#define USER_PUBMESSAGES 115
#define USER_PVTMESSAGES 116
#define USER_CONNECTIONS 117
#define USER_FILERATIO 118
#define USER_BYTERATIO 119
#define USER_FREEDLBYTES 120
#define USER_FREEDLFILES 121
#define USER_SECURITYLEVEL 122
#define USER_JOINCONFERENCE 123
#define USER_CONFERENCEACC1 124
#define USER_CONFERENCEACC2 125
#define USER_DAILYTIMELIMIT 126
#define USER_ACCOUNT_ID 127
#define USER_TIMELEFT 128
#define DOOR_PARAMS 129
#define DD_ORIGDIR 130
#define VCONF_NUMBER 131
#define USER_ULBYTES 132
#define USER_DLBYTES 133
#define USER_FIRSTCALL 134
#define USER_FLINES 135
#define USER_LASTCALL 136
#define USER_PROTOCOL 137
#define USER_FAKEDFILES 138
#define USER_FAKEDBYTES 139
#define SYS_FLAGGEDFILES 140
#define SYS_FLAGGEDBYTES 141
#define SYS_FLAGERROR 142
#define SYS_MSGBASE 143
#define SYS_CONF 131

#ifdef __cplusplus
};
#endif

#endif
