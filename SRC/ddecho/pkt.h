/*#include "fidoaddr.h"
#include "bsList.h"*/

#include <time.h>

typedef struct PKT_ {
	short origNode;
	short destNode;
	unsigned short year;
	unsigned short month;
	unsigned short day;
	unsigned short hour;
	unsigned short minute;
	unsigned short second;
	unsigned short baud;
	unsigned short type;
	short origNet;
	short destNet;
	char prodcode;
	char rev;
	char password[8];
	unsigned short origZone;
	unsigned short destZone;
	unsigned short AuxNet;
	unsigned short cw2;
	unsigned char prodcode2;
	unsigned char rev2;
	unsigned short cw;
	unsigned short origZoneq;
	unsigned short destZoneq;	
	unsigned short origPoint;
	unsigned short destPoint;
	char prdata[4];	
} PKT;

typedef struct PKT_MSG_ {
	unsigned short version;
	unsigned short origNode;
	unsigned short destNode;
	unsigned short origNet;
	unsigned short destNet;
	unsigned short Attribute;
	unsigned short Cost;
} PKT_MSG;

typedef struct MEM_MSG_ {
	int PktType;
	FidoAddr PktOrig;
	FidoAddr PktDest;
	char PktPw[8];
	struct tm PktDate;

	unsigned int Product;
	unsigned int Major_ver;
	unsigned int Minor_ver;

	int MsgNum;

	char MsgFrom[64];
	char MsgTo[64];
	char Subject[72];

	char Area[64];

	int Flags;
	int Stoneage;

	struct tm MsgDate;

	FidoAddr MsgOrig;
	FidoAddr MsgDest;

	bsList Text;

} MEM_MSG;
