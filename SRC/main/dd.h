#ifndef _DD_H_INCLUDED
#define _DD_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include <config.h>

#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <limits.h>

#if HAVE_INTTYPES_H
# include <inttypes.h>
#else
# if HAVE_STDINT_H
#  include <stdint.h>
# endif
#endif

#ifndef PATH_MAX
#define PATH_MAX 512
#endif

#define versionstring VERSION
#define DDTMP "/tmp/dd"

struct savedflag {
	char fname[256];
	int32_t conf;
};

struct List {
	struct Node *lh_Head;
	struct Node *lh_Tail;
	struct Node *lh_TailPred;
};


struct Node {
	struct Node *ln_Succ;
	struct Node *ln_Pred;
};

struct FFlag {
	struct Node fhead;
	char f_filename[256];
	char f_path[PATH_MAX];
	uint32_t f_size;
	uint16_t f_conf;
	uint32_t f_flags;
};

struct dd_nodemessage {
	uint16_t dn_command;
	uint32_t dn_data1;
	uint32_t dn_data2;
	uint32_t dn_data3;
	char dn_string[200];
};

#define JC_LIST 	(1L << 0)
#define JC_SHUTUP	(1L << 1)
#define JC_QUICK  	(1L << 2)
#define JC_NOUPDATE	(1L << 3)

#define FLAG_FREE (1L<<0)

#define HOT_CURSOR (1L<<0)
#define HOT_YESNO  (1L<<1)
#define HOT_NOYES  (1L<<2)
#define HOT_QUICK  (1L<<3)
#define HOT_RE     (1L<<4)
#define HOT_DELAY  (1L<<5)
#define HOT_MAIN   (1L<<6)

#define PROMPT_SECRET (1L<<0)
#define PROMPT_NOCRLF (1L<<1)
#define PROMPT_FILE   (1L<<2)
#define PROMPT_WRAP   (1L<<3)
#define PROMPT_MAIN   (1L<<4)

#define TYPE_MAKE (1L<<0)
#define TYPE_WARN (1L<<1)
#define TYPE_NOCODES (1L<<2)
#define TYPE_CONF (1L<<3)
#define TYPE_NOSTRIP (1L<<4)
#define TYPE_SEC (1L<<5)
#define TYPE_PATH (1L << 6)
#define TYPE_MENU (1L << 7)

#define MC_QUICK (1L<<0)
#define MC_NOSTAT (1L<<1)

#define YELLDSOCK DDTMP "/yelld"
#define YELLDLOCK DDTMP "/yelld.lock"

struct lcfile {
	int32_t lc_conf;
	char lc_name[256];
};

struct DayDream_NodeInfo {
	pid_t ddn_pid;
	int32_t ddn_userslot;	/* -1 == no user on node */
	char ddn_activity[80];
	int32_t ddn_bpsrate;
	char ddn_pagereason[80];
	char ddn_path[80];
	int32_t ddn_timeleft;
	int32_t ddn_flags;
};

struct DayDream_PageMsg {
	uint16_t pm_cmd;
	char pm_string[300];
};

struct DayDream_LRP {
	uint16_t lrp_read;
	uint16_t lrp_scan;
};

struct callerslog {
	uint16_t cl_userid;
	time_t cl_firstcall;	/* Just to be sure we have the right user */
	time_t cl_logon;
	time_t cl_logoff;
	uint32_t cl_ulbytes;
	uint32_t cl_dlbytes;
	uint16_t cl_ulfiles;
	uint16_t cl_dlfiles;
	uint16_t cl_pvtmessages;
	uint16_t cl_pubmessages;
	int32_t cl_bpsrate;
	int32_t cl_flags;
};


#define CL_CARRIERLOST (1L<<0)
#define CL_NEWUSER     (1L<<1)
#define CL_CHAT        (1L<<2)
#define CL_PASSWDFAIL  (1L<<3)
#define CL_PAGEDSYSOP  (1L<<4)
#define CL_RELOGIN     (1L<<5)

struct gcallerslog {
	uint16_t cl_node;
	struct callerslog cl;
};

struct userbase {
	char user_realname[26];
	char user_handle[26];
	char user_organization[26];
	char user_zipcity[21];
	char user_voicephone[21];
	uint8_t user_password[16];
	uint8_t user_screenlength;
	uint8_t user_protocol;
	uint32_t user_toggles;
	char user_signature[45];
	uint8_t user_flines;
	uint64_t user_ulbytes;	/* 64 bit ulbyte counter. Warez! */
	uint64_t user_dlbytes;
	uint16_t user_ulfiles;
	uint16_t user_dlfiles;
	uint16_t user_pubmessages;
	uint16_t user_pvtmessages;
	uint16_t user_connections;
	uint8_t user_fileratio;
	uint8_t user_byteratio;
	char user_computermodel[21];
	uint8_t freeslot2;
	uint32_t user_freedlbytes;
	uint8_t user_failedlogins;
	uint8_t user_securitylevel;
	uint8_t user_joinconference;
	uint8_t freeslot3;
	time_t user_firstcall;
	time_t user_lastcall;
	uint32_t user_conferenceacc1;
	uint32_t user_conferenceacc2;
	uint16_t user_dailytimelimit;
	uint16_t user_account_id;
	uint16_t user_timeremaining;
	uint16_t user_freedlfiles;
	uint16_t user_fakedfiles;
	uint32_t user_fakedbytes;
	char user_inetname[9];
	char user_freeblock[23];
};

#define UBENT_STAT_ACTIVE	0x00000000
#define UBENT_STAT_DELETED	0x40000000
#define UBENT_STAT_FROZEN	0x80000000
#define UBENT_STAT_NEW		0xc0000000
#define UBENT_STAT_MASK		UBENT_STAT_NEW

/* user_toggles:
   BIT  0:MEANING                    1:MEANING                  ON/OFF TOGGLES #1
   ==============================================================================
   000  Line Editor                  Full Screen Editor
   004  Novice User Mode             Expert User Mode
   005  Personal Mail Check ON       Personal Mail Check Off
   006  New Files Scan Off           New Files Scan On
   008                               User has answered the NUQ
   009  Allow nodemessages           Disallow nodemessages
   010  CLS's in filelists                No CLS's in filelists
   011                               Ask FSEd/LineED
   012                               Ask MailScan/NoMailScan
   013                               Ask FileScan/NoFileScan
   014                               Autoquick uploading
   015  Background checker           Normal checker
   ...
   030  Account Status #1 \ 0 = Active  0 = Frozen  1 = Deleted  1 = New Account
   031  Account Status #2 / 0           1           0            1
   ==============================================================================
 */

struct DayDream_DoorMsg {
	uint16_t ddm_command;
	uint32_t ddm_data1;
	uint32_t ddm_data2;
	uint32_t ddm_data3;
	uint64_t ddm_ldata;
	char ddm_string[300];
};

typedef struct DayDream_MsgBase {
	uint8_t MSGBASE_FLAGS;
	uint8_t MSGBASE_NUMBER;
	uint16_t MSGBASE_LOWEST;
	uint16_t MSGBASE_HIGHEST;
	uint16_t MSGBASE_MSGLIMIT;
	char MSGBASE_NAME[21];
	char MSGBASE_FREEBLOCK2[8];
	char MSGBASE_FN_TAG[26];
	char MSGBASE_FN_ORIGIN[58];
	uint8_t MSGBASE_FN_FLAGS;
	uint16_t MSGBASE_FN_ZONE;
	uint16_t MSGBASE_FN_NET;
	uint16_t MSGBASE_FN_NODE;
	uint16_t MSGBASE_FN_POINT;
	uint8_t MSGBASE_READACCESS;
	uint8_t MSGBASE_POSTACCESS;
	char MSGBASE_FREEBLOCK1[168];
} msgbase_t;

/* MSGBASE_FLAGS:
   BIT   MEANING WHEN SET AS PRESENTED BELOW                   MESSAGE BASE FLAGS
   ==============================================================================
   0    0: ALLOW PUBLIC MESSAGES         1: DO NOT ALLOW PUBLIC MESSAGES
   1    0: ALLOW PRIVATE MESSAGES        1: DO NOT ALLOW PRIVATE MESSAGES
   2    0: MESSAGES USE REAL NAMES       1: MESSAGES USE HANDLES / ALIASES
   3    0: QUOTE LINE AFTER ACTUAL QUOTE 1: USE THE "> " QUOTING METHOD
   4    0: A PLAIN "> " WILL DO          1: INSERT INITIALS BEFORE THE "> "
   5    0: DO NOT ALLOW FILE ATTACHS     1: ALLOW FILE ATTACHS
   7    PRIVATE FLAG -- DO NOT USE
   ==============================================================================
 */

struct DayDream_Conference {
	uint8_t CONF_NUMBER;
	char CONF_NAME[40];
	char CONF_PATH[40];
	uint8_t CONF_FILEAREAS;
	uint8_t CONF_UPLOADAREA;
	uint8_t CONF_MSGBASES;
	uint8_t CONF_COMMENTAREA;
	uint8_t CONF_UNUSED1;
	uint16_t CONF_ATTRIBUTES;
	char CONF_ULPATH[50];
	char CONF_NEWSCANAREAS[30];
	char CONF_PASSWD[16];
	char CONF_DOWNPATH[50];
	char CONF_GLFINALPATH[50];
	char CONF_FREEBLOCK[16];
};

/* PROJECT OLDSCHOOL REVENGE CHANGES TO CONFERENCE.DAT:

   glftpd root will be $DAYDREAM/glftpd

   CONF_DOWNPATH	- Download path relative to glftpd root
   CONF_GLFINALPATH	- Upload path relative to glftpd root

   So if CONF_DOWNPATH is /site/tags, and $DAYDREAM is /home/bbs, it will be:
   /home/bbs/glftpd/site/tags

 */

typedef struct conference_tag {
	struct conference_tag *next;
	struct conference_tag *prev;

	msgbase_t **msgbases;

	struct DayDream_Conference conf;
} conference_t;

/* CONF_ATTRIBUTES:
   BIT   MEANING WHEN SET AS PRESENTED BELOW                     CONFERENCE FLAGS
   ==============================================================================
   000   FREE-DOWNLOAD ON ALL FILES IN THIS CONFERENCE
   001   NO FILE OR BYTE CREDITS FOR UPLOADING
   002   DISABLE SENT-BY LINES
   003   34 CHARACTERS LONG FILENAMES
   004   ASK DESTINATION FILE AREA
   005   DO NOT ALLOW WILDCARDS IN FILENAMES (UPLOAD)
   006   DO NOT ALLOW WILDCARD FILEFLAGGING
   007   NO DUPECHECK (SKIP THIS CONF IN GLOBAL DUPE CHECK)
   008   NO FILECHECK
   010   PRIVATE FLAG -- DO NOT USE
   :
   :
   ·
   .
   015
   ==============================================================================
 */

struct DayDream_Protocol {
	uint8_t PROTOCOL_ID;
	char PROTOCOL_NAME[20];
	uint8_t PROTOCOL_EFFICIENCY;
	char PROTOCOL_XPRLIBRARY[30];
	char PROTOCOL_INITSTRING[30];
	uint8_t PROTOCOL_TYPE;
	char PROTOCOL_FREEBLOCK[117];
};


struct DayDream_DisplayMode {
	uint8_t DISPLAY_ID;
	char DISPLAY_PATH[9];
	uint16_t DISPLAY_ATTRIBUTES;
	uint8_t DISPLAY_INCOMING_TABLEID;
	uint8_t DISPLAY_OUTGOING_TABLEID;
	char DISPLAY_FONT[20];
	uint16_t DISPLAY_FONTSIZE;
	uint8_t DISPLAY_STRINGS;
	char DISPLAY_FREEBLOCK[63];
};

/* DISPLAY_ATTRIBUTES:
   ==============================================================================
   000   0: NO ANSI SUPPORT               1: ANSI CONTROL CODES ENABLED
   001   0: NO INCOMING CONVERSION        1: ACTIVATE INCOMING CONVERSION
   002   0: NO OUTGOING CONVERSION        1: ACTIVATE OUTGOING CONVERSION
   003   0: NO CONVERSION IN FILES        1: ACTIVATE CONVERSION IN FILES
   004   0: NO STRIP                      1: STRIP ANSICODES FROM TXT FILES
   005   1: IF GFX DOESN'T EXIST, CHECK IF TXT DOES
   .
   .
   015                                                                 
   ==============================================================================
 */

struct DayDream_AccessPreset {
	uint8_t ACCESS_SECLEVEL;
	uint8_t ACCESS_PRESETID;
	uint16_t ACCESS_FREEFILES;
	uint32_t ACCESS_FREEBYTES;
	char ACCESS_DESCRIPTION[29];
	uint8_t ACCESS_STATUS;
	char ACCESS_FREEBLOCK[12];
};

struct DayDream_MainConfig {
	char CFG_BOARDNAME[26];
	char CFG_SYSOPNAME[26];
	char CFG_SYSOPFIRST[26];
	char CFG_SYSOPLAST[26];
	uint8_t reserved;
	uint8_t CFG_LOCALSCREEN;
	char reserved2[9];
	char reserved3[21];
	char CFG_CHATDLPATH[41];
	uint8_t reserved4;
	uint32_t reserved5;
	uint8_t CFG_JOINIFAUTOJOINFAILS;
	char CFG_COLORSYSOP[11];
	char CFG_COLORUSER[11];
	uint8_t CFG_LINEEDCHAR;
	char CFG_SYSTEMPW[16];
	char CFG_NEWUSERPW[16];
	char CFG_OLUSEREDPW[16];
	uint32_t reserved6;
	uint8_t reserved7;
	uint8_t CFG_NEWUSERPRESETID;
	uint32_t CFG_IDLETIMEOUT;
	uint32_t CFG_FREEHDDSPACE;
	uint32_t CFG_FLAGS;
	char reserved8[16];
	char CFG_ALIENS[40];
	char CFG_FSEDCOMMAND[71];
	char CFG_FREEDLLINE[100];
	char reserved9[41];
	uint8_t CFG_COSYSOPLEVEL;
	uint8_t reserved10;
	char reserved11[40];
	uint16_t reserved12;
	gid_t CFG_ZIPGID;
	char reserved14[71];
	char CFG_HOLDDIR[60];
	char CFG_TELNETPAT[80];
	uint8_t CFG_TELNET1ST;
	uint8_t CFG_TELNETMAX;
	char CFG_LOCALPAT[80];
	uint8_t CFG_LOCAL1ST;
	uint8_t CFG_LOCALMAX;
	uid_t CFG_BBSUID;
	gid_t CFG_BBSGID;
	int32_t CFG_DEFAULTS;
	int32_t CFG_MAXFTPUSERS;
	char CFG_PYTHON[54];
	int32_t CFG_FLOODKILLTRIG;
	char CFG_DOORLIBPATH[80];
	uint16_t CFG_FTPSTART;
        uint16_t CFG_FTPEND;
	char CFG_FREESLOT1[3016];
};

/* CFG_FLAGS:
   BIT   MEANING WHEN SET AS PRESENTED BELOW                         SYSTEM FLAGS
   ==============================================================================
   0    0: DO NOT ASK CHAT REASON          1: ASK CHAT REASON
   1    0: REAL NAMES IN WHO ETC           1: HANDLES IN WHO ETC
   2    0: LOCATION IN WHO ETC             1: ORGANIZATION IN WHO ETC
   3    0: ENABLE WILDCARDS IN LOGON       1: DISABLE WILDCARDS IN LOGON
   4    0: QUESTIONAIRE ONLY FOR NEW USERS 1: ASK Q IF NOT ANSWERED YET
   6    0: $ ENTERS COMMAND MODE IN MSG-ED 1: CR ENTERS COMMAND MODE IN MSGED
   7    0: ASK REAL NAME FIRST (NEW USERS) 1: ASK HANDLE FIRST
   10   0: NORMAL DUPECHECKER              1: CATALOG DUPECHECKER
   11   0: NORMAL CHECKER                  1: BACKGROUND CHECKER
   * 
   ==============================================================================
 */

struct DayDream_Multinode {
	uint8_t MULTI_NODE;
	uint8_t MULTI_DEVICE;
	uint16_t MULTI_MINBAUD;
	uint16_t MULTI_MINBAUDNEW;
	uint32_t MULTI_TTYSPEED;
	char MULTI_TTYNAME[20];
	uint16_t MULTI_TTYTYPE;
	uint8_t MULTI_SCREENFLAGS;
	uint8_t MULTI_OTHERFLAGS;
	char MULTI_COMMAND[32];
	uint8_t MULTI_PRIORITY;
	char MULTI_TEMPORARY[33];
	uint32_t MULTI_UNUSED;
	uint8_t MULTI_PEN5;
	uint8_t MULTI_PEN6;
	uint8_t MULTI_PEN7;
	uint8_t MULTI_PEN8;
	uint8_t MULTI_PEN9;
	uint8_t MULTI_PEN10;
	uint8_t MULTI_PEN11;
	uint8_t MULTI_PEN12;
	char MULTI_FREE[88];
};

struct DayDream_Archiver {
	uint8_t ARC_FLAGS;
	char ARC_PATTERN[20];
	char ARC_NAME[21];
	char ARC_CMD_TEST[80];
	char ARC_EXTRACTFILEID[80];
	char ARC_ADDFILEID[80];
	char ARC_VIEW[80];
	char ARC_CORRUPTED1[16];
	char ARC_CORRUPTED2[16];
	char ARC_CORRUPTED3[16];
	uint8_t ARC_VIEWLEVEL;
	char ARC_FREEBLOCK[3];
};

struct DD_ExternalCommand {
	char EXT_NAME[11];
	uint8_t EXT_CMDTYPE;
	uint8_t EXT_SECLEVEL;
	char EXT_COMMAND[87];
	uint32_t EXT_CONF1;
	uint32_t EXT_CONF2;
	char EXT_PASSWD[16];
	char EXT_FREEBLOCK1[76];
};

struct DD_Seclevel {
	uint8_t SEC_SECLEVEL;
	uint8_t SEC_FILERATIO;
	uint8_t SEC_BYTERATIO;
	uint8_t SEC_PAGESPERCALL;
	uint16_t SEC_DAILYTIME;
	uint32_t SEC_CONFERENCEACC1;
	uint32_t SEC_CONFERENCEACC2;
	uint32_t SEC_ACCESSBITS1;
	uint32_t SEC_ACCESSBITS2;
	uint32_t SEC_ACCESSBITS3;
	uint32_t SEC_ACCESSBITS4;
	char SEC_FREE[20];
};

struct DD_UploadLog {
	uint16_t UL_SLOT;
	char UL_FILENAME[40];
	uint32_t UL_FILESIZE;
	time_t UL_TIME;
	uint16_t UL_BPSRATE;
	uint8_t UL_NODE;
	uint8_t UL_CONF;
	uint32_t UL_FREE;
};

struct DD_DownloadLog {
	uint16_t DL_SLOT;
	char DL_FILENAME[40];
	uint32_t DL_FILESIZE;
	time_t DL_TIME;
	uint16_t DL_BPSRATE;
	uint8_t DL_NODE;
	uint8_t DL_CONF;
	uint32_t DL_FREE;
};

struct ftpinfo {
        pid_t pid;
        int userid;
        char filename[256];
        int filesize;
        int transferred;
        int cps;
        int mode;
};

#define SECB_DOWNLOAD			0
#define SECB_UPLOAD			1
#define SECB_READMSG			2
#define SECB_ENTERMSG			3
#define SECB_PAGE			4
#define SECB_COMMENT			5
#define SECB_BULLETINS			6
#define SECB_FILESCAN			7
#define SECB_NEWFILES			8
#define SECB_ZIPPYSEARCH		9
#define SECB_RUNDOOR			10
#define SECB_JOINCONF			11
#define SECB_CHANGEMSGAREA		12
#define SECB_CHANGEINFO			13
#define SECB_RELOGIN			14
#define SECB_TAGEDITOR			15
#define SECB_USERSTATS			16
#define SECB_VIEWTIME			17
#define SECB_HYDRATRANSFER		18
#define SECB_EXPERTMODE			19
#define SECB_EALLMESSAGE		20
#define SECB_FIDOMESSAGE		21
#define SECB_PUBLICMESSAGE		22
#define SECB_READALL			23
#define SECB_USERED			24
#define SECB_VIEWLOG			25
#define SECB_SYSOPDL			26
#define SECB_USERLIST			27
#define SECB_DELETEANY                  28
#define SECB_REMOTESHELL                29
#define SECB_WHO                        30
#define SECB_MOVEFILE			31

#define SECB_SELECTFILECONFS		0
#define SECB_SELECTMSGBASES		1
#define SECB_SENDNETMAIL        	2
#define SECB_OLM			3
#define SECB_PVTATTACH                  4
#define SECB_PUBATTACH                  5
#define SECB_VIEWFILE                   6
#define SECB_REALNAME                   7
#define SECB_HANDLE                     8
#define SECB_CRASH 9

#define DPORT_OUTPUT_MASK		150

#define OUTPUT_REMOTE	0x01
#define OUTPUT_LOCAL	0x02

#ifdef __cplusplus
};
#endif

#endif
