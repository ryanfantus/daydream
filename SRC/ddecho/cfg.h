typedef struct MainCfg_ {
	bsList Akas;
	int DefZone;
	int MaxArcSize;
	int MaxPktSize;
	char AreafixHelpfile[128];
	char BeforeToss[128];
	char AfterToss[128];
	char Inbound[128];
	char Outbound[128];
	char SecInbound[128];
	char LocInbound[128];
	char BadPktDir[128];
	bool StrictInboundChecking;
	char DupeLog[128];
	char LogFile[128];
	char StarMsgNetmail[128];
	bool KillTransit;
	bool KillDupes;
	bool KillAreafixReq;
	bool KillAreafixResp;
	bool PackNetmail;
	bool AddTid;
	bool InsecurePkt;
	bool InsecureLink;
	bool CheckSeenbys;
	char Conferences[64][128];
	bsList AreafixNames;
	bsList ForwardFrom;
	bsList ForwardTo;
	bsList Passwords;
	bsList AreafixPws;
	bsList Groups;
	bsList Routes;
	bsList Packers;
	bsList PackRules;
	bsList Areas;
	bsList TinySeenbys;
	bsList ForwardLists;
} MainCfg;

typedef struct Password_ {
	FidoAddr Aka;
	char Password[8];
} Password;

typedef struct Group_ {
	FidoAddr Aka;
	char Group;
	int Send;
	int Recv;
} Group;

typedef struct ForwardList_ {
	char List[64];
	FidoAddr ourAka;
	FidoAddr Aka;
	char Group;
	int Conference;
} ForwardList;

typedef enum {NORMAL, HOLD, CRASH} Flav;
typedef enum {ROUTE, SEND} Route_Type;

typedef struct Route_ {
	Route_Type Type;
	FidoAddr Dest;
	Flav Flavour;
	char Pattern[128];
} Route;

typedef struct Packer_ {
	char Name[16];
	char Compress[256];
} Packer;

typedef struct PackRule_ {
	char Pattern[64];
	Packer* PackWith;
} PackRule;

typedef enum {ECHOMAIL, NETMAIL, BAD, DUPE, KILL /* Not really an areatype */} Area_Type;

typedef struct Area_ {
	char Area[64];
	int Conference;
	int Base;
	char Group;
	Area_Type Type;
	FidoAddr ourAka;
	bsList SendTo;
} Area;

typedef struct Export_ {
	FidoAddr Aka;
	bool Send;
	bool Recv;
	bool NoRemove;
} Export;
