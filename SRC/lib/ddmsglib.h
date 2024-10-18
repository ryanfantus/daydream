#include "config.h"

#if HAVE_INTTYPES_H 
# include <inttypes.h> 
#else 
# if HAVE_STDINT_H 
#  include <stdint.h> 
# endif 
#endif 

struct DayDream_Message {
	uint16_t MSG_NUMBER;
	uint16_t MSG_NEXTREPLY;
	uint16_t MSG_FLAGS;
	char MSG_AUTHOR[26];
	char MSG_RECEIVER[26];
	char MSG_SUBJECT[68];
	time_t MSG_CREATION;
	time_t MSG_RECEIVED;
	uint16_t MSG_READCOUNT;
	uint16_t MSG_ORIGINAL;
	char MSG_PASSWORD[16];
	uint16_t MSG_FN_PACKET_ORIG_ZONE;
	uint16_t MSG_FN_PACKET_ORIG_NET;
	uint16_t MSG_FN_PACKET_ORIG_NODE;
	uint16_t MSG_FN_PACKET_ORIG_POINT;
	uint16_t MSG_FN_ORIG_ZONE;
	uint16_t MSG_FN_ORIG_NET;
	uint16_t MSG_FN_ORIG_NODE;
	uint16_t MSG_FN_ORIG_POINT;
	uint32_t MSG_FN_MSGID;
	uint16_t MSG_FN_DEST_ZONE;
	uint16_t MSG_FN_DEST_NET;
	uint16_t MSG_FN_DEST_NODE;
	uint16_t MSG_FN_DEST_POINT;
	char MSG_ATTACH[33];
	char MSG_FREEBLOCK[85];
};

#define MSG_FLAGS_PRIVATE (1L<<0)
#define MSG_FLAGS_DELETED (1L<<1)
#define MSG_FLAGS_LOCAL (1L<<2)
#define MSG_FLAGS_EXPORTED (1L<<3)
#define MSG_FLAGS_KILL_SENT (1L<<4)
#define MSG_FLAGS_CRASH (1L<<5)
#define MSG_FLAGS_ATTACH (1L<<6)
#define MSG_FLAGS_RECV (1L<<7)
#define MSG_FLAGS_SENT (1L<<8)

struct DayDream_MsgPointers {
	uint16_t msp_low;
	uint16_t msp_high;
};


int ddmsg_open_base(char* conf, int msgbase_num, int flags, int mode);
int ddmsg_close_base(int);
int ddmsg_setptrs(char* conf, int msgbase_num, struct DayDream_MsgPointers* ptrs);
int ddmsg_getptrs(char* conf, int msgbase_num, struct DayDream_MsgPointers* ptrs);
int ddmsg_getfidounique();
int ddmsg_open_msg(char* conf, int msgbase_num, int msgnum, int flags, int mode);
int ddmsg_close_msg(int fd);
int ddmsg_delete_msg(char* conf, int msgbase_num, int msgnum);
int ddmsg_rename_msg(char* conf, int msgbase_num, int old_num, int new_num);
