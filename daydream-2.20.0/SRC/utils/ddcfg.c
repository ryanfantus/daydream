#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <math.h>
#include <pwd.h>
#include <grp.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <dd.h>

static char *findtxt(char *, const char*);
static char *cfgcopy2(char *, int, const char *);
static char cfgchar2(char *);
static int cfgflag2(const char *, const char *);
static int cfgint2(const char *);
static int cfgconf2(char *);
static char *nextspace(char *);
static char *cfgcopy(char *, int);
static char *findstrinblock(const char *);
static void cfgerror(const char *);

static int mymkdir(char *, mode_t);
static char * cfgfnadd3(unsigned short *, unsigned short *, 
	unsigned short *, unsigned short *, char *);
static void removespaces(char *);

static char homedir[512];
static char group[512];
static char zipgroup[512];
static char user[512];
static char olddir[1024];

static char gerror[1024];

static gid_t zipgid = -1;
static gid_t gid = -1;
static uid_t uid = -1;
 
static char *blockstart;
static char *src;
static char *ecfg(char *, const char *);
static void backtoold(void);

static void backtoold(void)
{
	chdir(olddir);
}

static char *endblock(void)
{
	char *buffb;
	buffb=blockstart;
	for (;;) {
		while(*buffb!=10) {
			if (buffb==0) return 0;
			buffb++;
		}
		buffb++;
		while(*buffb==10||*buffb==13) buffb++;
		if (*buffb=='~'||*buffb=='+'||*buffb=='*') {
			blockstart=buffb;
			return buffb;
		}
	}
}

static void remove_trailing_slash(char *pname)
{
	char *p;
	for (p = pname + strlen(pname); 
		p > pname && p[-1] == '/'; *--p = '\0');
}

int main(int argc, char *argv[])
{
	int infile;
	int outfile;
	char *inmem;
	struct stat fib;
	struct DayDream_MainConfig maincfg;
	struct DayDream_Conference conf;
	struct DayDream_MsgBase base;
	struct DD_ExternalCommand extcom;
	struct DayDream_AccessPreset preset;
	struct DayDream_Archiver arc;
	struct DayDream_DisplayMode disp;
	struct DayDream_Multinode node;
	struct DayDream_Protocol proto;
	struct DD_Seclevel sec;
	uint8_t selcfg[2056];
	int dodirs=1;
	struct passwd *pw;
	struct group *gr;	
	int i;
	int j;
	char *s;
	
	memset(selcfg, 0, sizeof(selcfg));
	memset(&maincfg, 0, sizeof(maincfg));
	
	if (argc < 2) {
		printf("usage: ddcfg [configfile] <-d>\n\n");
		exit(0);
	}

	infile = open(argv[1],O_RDONLY);
	if (infile == -1) {
		printf("Unknown configfile!\n\n");
		exit(0);
	}

	if (argc == 3 && !strcasecmp(argv[2],"-d")) {
		dodirs=0;
	}
	fstat(infile,&fib);
	inmem=(char *)malloc(fib.st_size+2);
	inmem[fib.st_size]=0;
	inmem[fib.st_size+1]=0;
	read(infile,inmem,fib.st_size);
	close(infile);
	
	src=ecfg(inmem,"HOMEDIR ");
	if (!src) {
		*homedir=0;
	} else {
		cfgcopy(homedir,500);
	}

	src=ecfg(inmem,"OWNERUSER ");
	if (!src) {
		*user=0;
	} else {
		cfgcopy(user,500);
	}

	src=ecfg(inmem,"OWNERGROUP ");
	if (!src) {
		*group=0;
	} else {
		cfgcopy(group,500);
	}

	src=ecfg(inmem,"ZIPGROUP ");
	if (!src) {
		*zipgroup=0;
	} else {
		cfgcopy(zipgroup,500);
	}

	if (*homedir) {
		getcwd(olddir,1024);
		chdir(homedir);
		atexit(backtoold);
	}

	if (*user == 0 || (pw = getpwnam(user)) == NULL) {
		fputs("OWNERUSER missing.\n", stderr);
		exit(1);
	} else
		uid = pw->pw_uid;

	if (*group == 0 || (gr = getgrnam(group)) == NULL) {
		fputs("OWNERGROUP missing.\n", stderr);
		exit(1);
	} else
		gid = gr->gr_gid;

	if (*zipgroup == 0 || (gr = getgrnam(zipgroup)) == NULL) {
		fputs("ZIPGROUP missing.\n", stderr);
		exit(1);
	} else if (gr->gr_gid == gid) {
		fputs("ZIPGROUP equals to OWNERGROUP. This is a security risk.\n", stderr);
		fputs("Please read \"docs/SECURITY\".\n", stderr);
		exit(1);
	} else
		zipgid = gr->gr_gid;
			

	src=findtxt(inmem,"!DAYDREAM.DAT");

	if (src==0) {
		printf("Can't find definitions for DAYDREAM.DAT!\n\n");
		exit(0);
	}

	strcpy(gerror,"DAYDREAM.DAT");

	cfgcopy2(maincfg.CFG_BOARDNAME,26,"BBSNAME");
	cfgcopy2(maincfg.CFG_SYSOPNAME,26,"SYSOP");
	cfgcopy2(maincfg.CFG_SYSOPFIRST,26,"SYSOPFIRST");
	cfgcopy2(maincfg.CFG_SYSOPLAST,26,"SYSOPLAST");
	cfgcopy2(maincfg.CFG_CHATDLPATH, 41, "CHATDLPATH");
	remove_trailing_slash(maincfg.CFG_CHATDLPATH);
	cfgcopy2(maincfg.CFG_SYSTEMPW,16,"SYSTEM_PW");
	cfgcopy2(maincfg.CFG_PYTHON,54,"PYTHON");
	cfgcopy2(maincfg.CFG_NEWUSERPW,16,"NEWUSER_PW");
	cfgcopy2(maincfg.CFG_OLUSEREDPW,16,"USERED_PW");
	maincfg.CFG_NEWUSERPRESETID=cfgint2("NEW_PRESET");
	maincfg.CFG_IDLETIMEOUT=cfgint2("IDLE");
	maincfg.CFG_JOINIFAUTOJOINFAILS=cfgint2("AUTOCONF");
	maincfg.CFG_FREEHDDSPACE=cfgint2("FREEFORUL");
	if (cfgflag2("Y","ASK_REASON")) maincfg.CFG_FLAGS |= (1L << 0);
	if (cfgflag2("H","NAMEMODE")) maincfg.CFG_FLAGS |= (1L << 1);
	if (cfgflag2("O","LOCATION")) maincfg.CFG_FLAGS |= (1L << 2);
	if (cfgflag2("Y","ASKHANDLE1ST")) maincfg.CFG_FLAGS |= (1L << 7);
	if (cfgflag2("Y","ASKDISPLAY")) 
		maincfg.CFG_FLAGS |= (1L << 16);
	cfgcopy2(maincfg.CFG_COLORSYSOP,11,"SYSOP_COL");
	cfgcopy2(maincfg.CFG_COLORUSER,11,"USER_COL");
	cfgcopy2(maincfg.CFG_ALIENS,40,"ALIENS");
	cfgcopy2(maincfg.CFG_DOORLIBPATH,80,"DOORLIBPATH");
	maincfg.CFG_LINEEDCHAR=cfgchar2("LINE_EDCOM");
	cfgcopy2(maincfg.CFG_FSEDCOMMAND,70,"FSED");
	maincfg.CFG_LOCALSCREEN=cfgint2("SCREENLENGTH");
	cfgcopy2(maincfg.CFG_FREEDLLINE,100,"FREEDLLINE");
	maincfg.CFG_COSYSOPLEVEL=cfgint2("COSYSOPACCESS");
	cfgcopy2(maincfg.CFG_HOLDDIR,60,"HOLDDIR");
	cfgcopy2(maincfg.CFG_TELNETPAT,80,"TELNETNODEPAT");
	maincfg.CFG_TELNET1ST=cfgint2("TELNETNODE1ST");
	maincfg.CFG_TELNETMAX=cfgint2("TELNETNODEMAX");
	cfgcopy2(maincfg.CFG_LOCALPAT,80,"LOCALNODEPAT");
	maincfg.CFG_LOCAL1ST=cfgint2("LOCALNODE1ST");
	maincfg.CFG_LOCALMAX=cfgint2("LOCALNODEMAX");
	if (cfgflag2("Y","ALLOW2LOGINS")) maincfg.CFG_FLAGS |= (1L << 8);

	switch (cfgchar2("ASKNEWUSER")) {
	case 'N':
		break;
	case 'Y':
		maincfg.CFG_FLAGS |= (1L << 9);
		break;
	case 'D':
		maincfg.CFG_FLAGS |= (1L << 17);
		break;
	}	
	if (cfgflag2("Y","LOGINSTYLE")) maincfg.CFG_FLAGS |= (1L << 18);
		
	if (cfgflag2("Y","DEF_EXPERT")) maincfg.CFG_DEFAULTS |= (1L<<4);
	switch(cfgchar2("DEF_MAILSCAN"))
	{
		case 'N':
			maincfg.CFG_DEFAULTS |= (1L<<5);
		break;
		case 'A':
			maincfg.CFG_DEFAULTS |= (1L<<12);
		break;
	}

	switch(cfgchar2("DEF_NEWFILESC"))
	{
		case 'Y':
			maincfg.CFG_DEFAULTS |= (1L<<6);
		break;
		case 'A':
			maincfg.CFG_DEFAULTS |= (1L<<13);
		break;
	}

	if (cfgflag2("N","DEF_ALLOWNODE")) maincfg.CFG_DEFAULTS |= (1L<<9);

	switch(cfgchar2("DEF_EDITOR"))
	{
		case 'F':
			maincfg.CFG_DEFAULTS |= (1L<<0);
		break;
		case 'A':
			maincfg.CFG_DEFAULTS |= (1L<<11);
		break;
	}

	if (cfgflag2("Y","DEF_AUTOQUICK")) maincfg.CFG_DEFAULTS |= (1L<<14);
	if (cfgflag2("N","DEF_BGCHECKER")) maincfg.CFG_DEFAULTS |= (1L<<15);
	if (cfgflag2("Y","CATALOGDUPECK")) maincfg.CFG_FLAGS |= (1L<<10);
	if (cfgflag2("Y","BGCHECKER")) maincfg.CFG_FLAGS |= (1L<<11);
	maincfg.CFG_MAXFTPUSERS=cfgint2("MAXFTPUSERS");
	maincfg.CFG_FLOODKILLTRIG=cfgint2("FLOODKILLTRIG");
	maincfg.CFG_FTPSTART=cfgint2("FTPSTART");
	maincfg.CFG_FTPEND=cfgint2("FTPEND");

	maincfg.CFG_BBSUID = uid;
	maincfg.CFG_BBSGID = gid;
	maincfg.CFG_ZIPGID = zipgid;

	outfile = open("data/daydream.dat", O_CREAT|O_TRUNC|O_RDWR, 0640);
	if (outfile == -1) {
		perror("data/daydream.dat");
		exit(0);
	}
	fchmod(outfile, 0640);
		
	write(outfile,&maincfg,sizeof(struct DayDream_MainConfig));
	close(outfile);

	chown("data/daydream.dat",uid,gid);
	
	src=findtxt(inmem,"!CONFERENCES.DAT");
	if (src==0) {
		printf("Can't find definitions for CONFERENCES.DAT!\n\n");
		exit(0);
	}
	
	outfile = open("data/conferences.dat", O_CREAT|O_TRUNC|O_RDWR, 0640);
	if (outfile == -1) {
		perror("data/conferences.dat");
		exit(0);
	}
	fchmod(outfile, 0640);
nextconf:
	strcpy(gerror,"CONFERENCES.DAT");

	s=(char *)&conf;
	for(i=0; i < sizeof(struct DayDream_Conference)-1 ; i++) {
		s[i]=0;
	}

	conf.CONF_NUMBER=cfgint2("CONF_NUMB");
	sprintf(gerror,"CONFERENCES.DAT: Conference %d",conf.CONF_NUMBER);

	cfgcopy2(conf.CONF_NAME,40,"CONF_NAME");
	cfgcopy2(conf.CONF_PATH,40,"CONF_PATH");
	remove_trailing_slash(conf.CONF_PATH);
	if (dodirs) {
		char tbu[600];

		mymkdir(conf.CONF_PATH, 0750);
		sprintf(tbu,"%s/data",conf.CONF_PATH);
		mymkdir(tbu, 0770);
		sprintf(tbu,"%s/messages",conf.CONF_PATH);
		mymkdir(tbu, 0750);
		sprintf(tbu,"%s/display",conf.CONF_PATH);
		mymkdir(tbu, 0750);
	}
	conf.CONF_FILEAREAS=cfgint2("CONF_FILEAREAS");
	conf.CONF_UPLOADAREA=cfgint2("CONF_UPLOADSTO");
	conf.CONF_MSGBASES=cfgint2("CONF_MSGBASES");
	conf.CONF_COMMENTAREA=cfgint2("CONF_COMMENTS");
	cfgcopy2(conf.CONF_ULPATH,50,"CONF_ULPATH");
	if (dodirs && *conf.CONF_ULPATH) {
		mymkdir(conf.CONF_ULPATH, 0770);
	}
	cfgcopy2(conf.CONF_NEWSCANAREAS,30,"CONF_NSAREAS");
	if (cfgflag2("Y","CONF_DEFFSCAN")) {
		conf.CONF_ATTRIBUTES |= (1L << 10);
		selcfg[2056+((conf.CONF_NUMBER-1)/8)] |= (1L<<(conf.CONF_NUMBER-1)%8);
	}
	if (cfgflag2("Y","CONF_FREELEECH")) conf.CONF_ATTRIBUTES |= (1L << 0);
	if (cfgflag2("Y","CONF_NOCREDS")) conf.CONF_ATTRIBUTES |= (1L << 1);
	if (cfgflag2("Y","CONF_SENTBY")) conf.CONF_ATTRIBUTES |= (1L << 2);
	if (cfgflag2("Y","CONF_LONGNAMES")) conf.CONF_ATTRIBUTES |= (1L << 3);
	if (cfgflag2("Y","CONF_ASKDEST")) conf.CONF_ATTRIBUTES |= (1L << 4);
	if (cfgflag2("Y","CONF_NOWILDS")) conf.CONF_ATTRIBUTES |= (1L << 5);
	if (cfgflag2("Y","CONF_NOWILDFLG")) conf.CONF_ATTRIBUTES |= (1L << 6);
	if (cfgflag2("Y","CONF_NODUPECHK")) conf.CONF_ATTRIBUTES |= (1L << 7);
	if (cfgflag2("Y","CONF_NOFILECHK")) conf.CONF_ATTRIBUTES |= (1L << 8);
	if (cfgflag2("Y","CONF_GLOBALDCK")) conf.CONF_ATTRIBUTES |= (1L << 9);
	cfgcopy2(conf.CONF_PASSWD,16,"CONF_PASSWORD");

	write(outfile,&conf,sizeof(struct DayDream_Conference));
	j=0;
crossconf:	
	src=endblock();
	if (*src=='~') goto confsdone;
	if (*src=='+') goto dobase;
	if (*src=='*') {
		if (j!=conf.CONF_MSGBASES) {
			printf("Number of msgbases mismatch in conf %d!\n",conf.CONF_NUMBER);
			exit(0);
		}
		goto nextconf;
	}
	printf("Error in conferences.dat!\n");
	printf("%30.30s",src);
	exit(0);

dobase:
	sprintf(gerror,"CONFERENCES.DAT, Conference %d",conf.CONF_NUMBER);

	s=(char *)&base;
	for(i=0; i < sizeof(struct DayDream_MsgBase)-1 ; i++) {
		s[i]=0;
	}

	base.MSGBASE_NUMBER=cfgint2("BASE_NUMBER");
	sprintf(gerror,"CONFERENCES.DAT, Conference %d, Msgbase %d",conf.CONF_NUMBER,base.MSGBASE_NUMBER);

	if (dodirs) {
		char tbu[600];

		sprintf(tbu, "%s/messages/base%3.3d", conf.CONF_PATH, base.MSGBASE_NUMBER);
		mymkdir(tbu, 0770);
	}
	base.MSGBASE_FN_FLAGS=cfgchar2("BASE_TYPE");
	base.MSGBASE_MSGLIMIT=cfgint2("BASE_MAXMSGS");
	cfgcopy2(base.MSGBASE_NAME,21,"BASE_NAME");
	cfgcopy2(base.MSGBASE_FN_TAG,26,"BASE_FIDOTAG");
	cfgcopy2(base.MSGBASE_FN_ORIGIN,58,"BASE_ORIGIN");
	cfgfnadd3(&base.MSGBASE_FN_ZONE,&base.MSGBASE_FN_NET,&base.MSGBASE_FN_NODE,&base.MSGBASE_FN_POINT,"BASE_FIDOADD");
	if (cfgflag2("N","BASE_ALLOWPBLC")) base.MSGBASE_FLAGS |= (1L << 0);
	if (cfgflag2("N","BASE_ALLOWPRIV")) base.MSGBASE_FLAGS |= (1L << 1);
	if (cfgflag2("Y","BASE_FILEATTAC")) base.MSGBASE_FLAGS |= (1L << 5);
	if (cfgflag2("H","BASE_NAMES")) base.MSGBASE_FLAGS |= (1L << 2);
	if (cfgflag2("Y","BASE_ALL=EALL")) base.MSGBASE_FLAGS |= (1L << 6);
	{
		char huora;
		huora=cfgchar2("BASE_QUOTE");
		if (huora=='>') {
			base.MSGBASE_FLAGS |= (1L << 3);
		}
		if (huora=='I') {
			base.MSGBASE_FLAGS |= (1L << 3);
			base.MSGBASE_FLAGS |= (1L << 4);
		}
	}
	
	if (cfgflag2("Y","BASE_DEFGRAB")) {
		base.MSGBASE_FLAGS |= (1L << 7);
		selcfg[((conf.CONF_NUMBER-1)*32)+(base.MSGBASE_NUMBER-1)/8] |= (1L<<(base.MSGBASE_NUMBER-1)%8);
	}
	write(outfile,&base,sizeof(struct DayDream_MsgBase));
	j++;
	goto crossconf;
	
confsdone:
	close(outfile);

	chown("data/conferences.dat",uid,gid);
	
	outfile = open("data/selected.dat", O_CREAT|O_TRUNC|O_RDWR, 0640);
	if (outfile == -1) {
		perror("data/selected.dat");
		exit(0);
	}
	fchmod(outfile, 0640);

	write(outfile,&selcfg,2056);
	close(outfile);

	chown("data/selected.dat",uid,gid);

	src=findtxt(inmem,"!EXTERNALCOMMANDS.DAT");
	if (src==0) {
		printf("Can't find definitions for EXTERNALCOMMANDS.DAT!\n\n");
		exit(0);
	}

	outfile = open("data/externalcommands.dat", 
		O_CREAT|O_TRUNC|O_RDWR, 0640);
	if (outfile == -1) {
		perror("data/externalcommands.dat");
		exit(0);
	}
	fchmod(outfile, 0640);
nextdoor:

	strcpy(gerror,"EXTERNALCOMMANDS.DAT");
	s=(char *)&extcom;
	for(i=0; i < sizeof(struct DD_ExternalCommand)-1 ; i++) {
		s[i]=0;
	}
	cfgcopy2(extcom.EXT_NAME,11,"DOOR_COMMAND");
	sprintf(gerror,"EXTERNALCOMMANDS.DAT, cmd %s",extcom.EXT_NAME);
	extcom.EXT_CMDTYPE=cfgint2("DOOR_TYPE");
	extcom.EXT_SECLEVEL=cfgint2("DOOR_SECURITY");
	cfgcopy2(extcom.EXT_COMMAND,87,"DOOR_EXECUTE");
	extcom.EXT_CONF1=cfgconf2("DOOR_CONFS1");
	extcom.EXT_CONF2=cfgconf2("DOOR_CONFS2");
	cfgcopy2(extcom.EXT_PASSWD, 16, "DOOR_PASSWD");
	
	write(outfile,&extcom,sizeof(struct DD_ExternalCommand));
	src=endblock();
	if (*src=='+') goto nextdoor;
	if (*src!='~') {
		printf("Error in externalcommands.dat!\n\n%30.30s",src);
		close(outfile);
		exit(0);
	}	
	close(outfile);

	chown("data/externalcommands.dat",uid,gid);

	src=findtxt(inmem,"!ARCHIVERS.DAT");
	if (src==0) {
		printf("Can't find definitions for ARCHIVERS.DAT!\n\n");
		exit(0);
	}

	outfile = open("data/archivers.dat", 
		O_CREAT|O_TRUNC|O_RDWR, 0640);
	if (outfile == -1) {
		perror("data/archivers.dat");
		exit(0);
	}
	fchmod(outfile, 0640);
nextarc:

	strcpy(gerror,"ARCHIVERS.DAT");
	s=(char *)&arc;
	for(i=0; i < sizeof(struct DayDream_Archiver) ; i++) {
		s[i]=0;
	}
	cfgcopy2(arc.ARC_NAME,21,"ARC_NAME");
	sprintf(gerror,"ARCHIVERS.DAT: Archiver %s\n",arc.ARC_NAME);
	cfgcopy2(arc.ARC_PATTERN,20,"ARC_PATTERN");
	cfgcopy2(arc.ARC_CMD_TEST,80,"ARC_TEST");
	cfgcopy2(arc.ARC_CORRUPTED1,16,"ARC_FAIL1");
	cfgcopy2(arc.ARC_CORRUPTED2,16,"ARC_FAIL2");
	cfgcopy2(arc.ARC_CORRUPTED3,16,"ARC_FAIL3");
	cfgcopy2(arc.ARC_EXTRACTFILEID,80,"ARC_FIDEXTRACT");
	cfgcopy2(arc.ARC_ADDFILEID,80,"ARC_FIDADD");
	cfgcopy2(arc.ARC_VIEW,80,"ARC_VIEW");
	if (cfgflag2("Y","ARC_GETDATE")) arc.ARC_FLAGS |= (1L<<2);
	if (cfgflag2("Y","ARC_DELETECOR")) arc.ARC_FLAGS |= (1L<<3);
	if (cfgflag2("Y","ARC_DISPLAY")) arc.ARC_FLAGS |= (1L<<4);
	arc.ARC_VIEWLEVEL=cfgint2("ARC_VIEWSEC");

	write(outfile,&arc,sizeof(struct DayDream_Archiver));
	src=endblock();
	if (*src=='+') goto nextarc;
	if (*src!='~') {
		printf("Error in archiver.dat!\n\n%30.30s",src);
		close(outfile);
		exit(0);
	}	
	close(outfile);

	chown("data/archivers.dat",uid,gid);

	src=findtxt(inmem,"!DISPLAY.DAT");
	if (src==0) {
		printf("Can't find definitions for DISPLAY.DAT!\n\n");
		exit(0);
	}

	outfile = open("data/display.dat", O_CREAT|O_TRUNC|O_RDWR, 0640);
	if (outfile == -1) {
		perror("data/display.dat");
		exit(0);
	}
	fchmod(outfile, 0640);
nextdisp:

	strcpy(gerror,"DISPLAY.DAT");
	s=(char *)&disp;
	for(i=0; i < sizeof(struct DayDream_DisplayMode) ; i++) {
		s[i]=0;
	}

	disp.DISPLAY_ID=cfgint2("DPL_ID");
	sprintf(gerror,"DISPLAY.DAT, ID: %d",disp.DISPLAY_ID);

	cfgcopy2(disp.DISPLAY_PATH,9,"DPL_PATH");
	remove_trailing_slash(disp.DISPLAY_PATH);
	if (cfgflag2("Y","DPL_ANSI")) disp.DISPLAY_ATTRIBUTES |= (1L<<0);
	if (cfgflag2("Y","DPL_INCONV")) disp.DISPLAY_ATTRIBUTES |= (1L<<1);
	if (cfgflag2("Y","DPL_OUTCONV")) disp.DISPLAY_ATTRIBUTES |= (1L<<2);
	if (cfgflag2("Y","DPL_FILESCONV")) disp.DISPLAY_ATTRIBUTES |= (1L<<3);
	if (cfgflag2("Y","DPL_STRIPANSI")) disp.DISPLAY_ATTRIBUTES |= (1L<<4);
	if (cfgflag2("Y","DPL_TRYTXT")) disp.DISPLAY_ATTRIBUTES |= (1L<<5);
	disp.DISPLAY_INCOMING_TABLEID=cfgint2("DPL_INTABLE");	
	disp.DISPLAY_OUTGOING_TABLEID=cfgint2("DPL_OUTTABLE");
	disp.DISPLAY_STRINGS=cfgint2("DPL_STRINGS");

	write(outfile,&disp,sizeof(struct DayDream_DisplayMode));
	src=endblock();
	if (*src=='+') goto nextdisp;
	if (*src!='~') {
		printf("Error in display.dat!\n\n%30.30s",src);
		close(outfile);
		exit(0);
	}	
	close(outfile);

	chown("data/display.dat",uid,gid);

	src=findtxt(inmem,"!MULTINODE.DAT");
	if (src==0) {
		printf("Can't find definitions for MULTINODE.DAT!\n\n");
		exit(0);
	}

	outfile = open("data/multinode.dat", O_CREAT|O_TRUNC|O_RDWR, 0640);
	if (outfile == -1) {
		perror("data/multinode.dat");
		exit(0);
	}
	fchmod(outfile, 0640);
nextnode:

	strcpy(gerror,"MULTINODE.DAT");

	s=(char *)&node;
	for(i=0; i < sizeof(struct DayDream_Multinode) ; i++) {
		s[i]=0;
	}

	{
		char buf[10];
		cfgcopy2(buf,4,"MNODE_NODE");
		if (*buf=='T' || *buf=='t') {
			node.MULTI_NODE=-2;
		} else if (*buf=='L' || *buf=='l') {
			node.MULTI_NODE=-3;
		} else if (*buf=='F' || *buf=='f') {
			node.MULTI_NODE=-4;
		} else {
			node.MULTI_NODE=atoi(buf);
		}
	}
	node.MULTI_MINBAUD=cfgint2("MNODE_MBAUD");
	node.MULTI_MINBAUDNEW=cfgint2("MNODE_MNBAUD");
	if (cfgflag2("Y","MNODE_OWNDIR")) node.MULTI_SCREENFLAGS |= (1L<<1);
	if (cfgflag2("Y","MNODE_NOTIFY")) node.MULTI_OTHERFLAGS |= (1L<<1);
	if (cfgflag2("Y","MNODE_HIDEINAC")) node.MULTI_OTHERFLAGS |= (1L<<2);
	if (cfgflag2("Y","MNODE_HIDEWAIT")) node.MULTI_OTHERFLAGS |= (1L<<3);
	if (cfgflag2("Y","MNODE_HIDECALL")) node.MULTI_OTHERFLAGS |= (1L<<4);
	if (cfgflag2("Y","MNODE_NOPASSWD")) node.MULTI_OTHERFLAGS |= (1L<<5);
	cfgcopy2(node.MULTI_TEMPORARY,33,"MNODE_TEMP");
	remove_trailing_slash(node.MULTI_TEMPORARY);
	cfgcopy2(node.MULTI_TTYNAME,20,"MNODE_TTYNAME");
	node.MULTI_TTYTYPE=cfgint2("MNODE_TTYTYPE");
	node.MULTI_TTYSPEED=cfgint2("MNODE_TTYSPEED");
				
	write(outfile,&node,sizeof(struct DayDream_Multinode));
	src=endblock();
	if (*src=='+') goto nextnode;
	if (*src!='~') {
		printf("Error in multinode.dat!\n\n%30.30s",src);
		close(outfile);
		exit(0);
	}	
	close(outfile);

	chown("data/multinode.dat",uid,gid);

	src=findtxt(inmem,"!SECURITY.DAT");
	if (src==0) {
		printf("Can't find definitions for SECURITY.DAT!\n\n");
		exit(0);
	}

	outfile = open("data/security.dat", O_CREAT|O_TRUNC|O_RDWR, 0640);
	if (outfile == -1) {
		perror("data/security.dat");
		exit(0);
	}
	fchmod(outfile, 0640);
nextsec:
	strcpy(gerror,"SECURITY.DAT");

	s=(char *)&sec;
	for(i=0; i < sizeof(struct DD_Seclevel) ; i++) {
		s[i]=0;
	}

	sec.SEC_SECLEVEL=cfgint2("SEC_LEVEL");
	sprintf(gerror,"SECURITY.DAT, Level %d",sec.SEC_SECLEVEL);
	sec.SEC_FILERATIO=cfgint2("SEC_FRATIO");
	sec.SEC_BYTERATIO=cfgint2("SEC_BRATIO");
	sec.SEC_PAGESPERCALL=cfgint2("SEC_PAGES");
	sec.SEC_DAILYTIME=cfgint2("SEC_TIMELIMIT");
	sec.SEC_CONFERENCEACC1=cfgconf2("SEC_CONFACC1");
	sec.SEC_CONFERENCEACC2=cfgconf2("SEC_CONFACC2");
	if (cfgflag2("Y","SECF_DOWNLOAD")) sec.SEC_ACCESSBITS1 |= (1L<<0);
	if (cfgflag2("Y","SECF_UPLOAD")) sec.SEC_ACCESSBITS1 |= (1L<<1);
	if (cfgflag2("Y","SECF_READMSGS")) sec.SEC_ACCESSBITS1 |= (1L<<2);
	if (cfgflag2("Y","SECF_ENTERMSG")) sec.SEC_ACCESSBITS1 |= (1L<<3);
	if (cfgflag2("Y","SECF_PAGE")) sec.SEC_ACCESSBITS1 |= (1L<<4);
	if (cfgflag2("Y","SECF_COMMENT")) sec.SEC_ACCESSBITS1 |= (1L<<5);
	if (cfgflag2("Y","SECF_BULLETINS")) sec.SEC_ACCESSBITS1 |= (1L<<6);
	if (cfgflag2("Y","SECF_LISTFILES")) sec.SEC_ACCESSBITS1 |= (1L<<7);
	if (cfgflag2("Y","SECF_NEWFILES")) sec.SEC_ACCESSBITS1 |= (1L<<8);
	if (cfgflag2("Y","SECF_ZIPPY")) sec.SEC_ACCESSBITS1 |= (1L<<9);
	if (cfgflag2("Y","SECF_RUNDOOR")) sec.SEC_ACCESSBITS1 |= (1L<<10);
	if (cfgflag2("Y","SECF_JOINCONF")) sec.SEC_ACCESSBITS1 |= (1L<<11);
	if (cfgflag2("Y","SECF_MSGAREAC")) sec.SEC_ACCESSBITS1 |= (1L<<12);
	if (cfgflag2("Y","SECF_CHANGEINF")) sec.SEC_ACCESSBITS1 |= (1L<<13);
	if (cfgflag2("Y","SECF_RELOGIN")) sec.SEC_ACCESSBITS1 |= (1L<<14);
	if (cfgflag2("Y","SECF_TAGED")) sec.SEC_ACCESSBITS1 |= (1L<<15);
	if (cfgflag2("Y","SECF_VIEWSTATS")) sec.SEC_ACCESSBITS1 |= (1L<<16);
	if (cfgflag2("Y","SECF_VIEWTIME")) sec.SEC_ACCESSBITS1 |= (1L<<17);
	/* 18 hydra - deprecated and not telnetable */
	if (cfgflag2("Y","SECF_EXPERT")) sec.SEC_ACCESSBITS1 |= (1L<<19);
	if (cfgflag2("Y","SECF_EALL")) sec.SEC_ACCESSBITS1 |= (1L<<20);
	if (cfgflag2("Y","SECF_FIDOMSG")) sec.SEC_ACCESSBITS1 |= (1L<<21);
	if (cfgflag2("Y","SECF_PRIVMSG")) sec.SEC_ACCESSBITS1 |= (1L<<22);
	if (cfgflag2("Y","SECF_READALL")) sec.SEC_ACCESSBITS1 |= (1L<<23);
	if (cfgflag2("Y","SECF_USERED")) sec.SEC_ACCESSBITS1 |= (1L<<24);
	if (cfgflag2("Y","SECF_VIEWLOG")) sec.SEC_ACCESSBITS1 |= (1L<<25);
	if (cfgflag2("Y","SECF_SYSOPDL")) sec.SEC_ACCESSBITS1 |= (1L<<26);
	if (cfgflag2("Y","SECF_USERLIST")) sec.SEC_ACCESSBITS1 |= (1L<<27);
	if (cfgflag2("Y","SECF_DELETEANY")) sec.SEC_ACCESSBITS1 |= (1L<<28);
	if (cfgflag2("Y","SECF_SHELL")) sec.SEC_ACCESSBITS1 |= (1L<<29);
	if (cfgflag2("Y","SECF_WHO")) sec.SEC_ACCESSBITS1 |= (1L<<30);
	if (cfgflag2("Y","SECF_MOVE")) sec.SEC_ACCESSBITS1 |= (1L<<31);
	if (cfgflag2("Y","SECF_SFCONFS")) sec.SEC_ACCESSBITS2 |= (1L<<0);
	if (cfgflag2("Y","SECF_SMBASES")) sec.SEC_ACCESSBITS2 |= (1L<<1);
	if (cfgflag2("Y","SECF_NETMAIL")) sec.SEC_ACCESSBITS2 |= (1L<<2);
	if (cfgflag2("Y","SECF_OLM")) sec.SEC_ACCESSBITS2 |= (1L<<3);
	if (cfgflag2("Y","SECF_PRVATTACH")) sec.SEC_ACCESSBITS2 |= (1L<<4);
	if (cfgflag2("Y","SECF_PUBATTACH")) sec.SEC_ACCESSBITS2 |= (1L<<5);
	if (cfgflag2("Y","SECF_VIEWFILE")) sec.SEC_ACCESSBITS2 |= (1L<<6);
	if (cfgflag2("Y","SECF_REALNAME")) sec.SEC_ACCESSBITS2 |= (1L<<7);
	if (cfgflag2("Y","SECF_HANDLE")) sec.SEC_ACCESSBITS2 |= (1L<<8);
	if (cfgflag2("Y","SECF_CRASH")) sec.SEC_ACCESSBITS2 |= (1L<<9);	
	
	write(outfile,&sec,sizeof(struct DD_Seclevel));
	src=endblock();
	if (*src=='+') goto nextsec;
	if (*src!='~') {
		printf("Error in security.dat!\n\n%30.30s",src);
		close(outfile);
		exit(0);
	}	
	close(outfile);

	chown("data/security.dat",uid,gid);

	src=findtxt(inmem,"!ACCESS.DAT");
	if (src==0) {
		printf("Can't find definitions for ACCESS.DAT!\n\n");
		exit(0);
	}

	outfile = open("data/access.dat", O_CREAT|O_TRUNC|O_RDWR, 0640);
	if (outfile == -1) {
		perror("data/access.dat");
		exit(0);
	}
	fchmod(outfile, 0640);
nextpreset:

	s=(char *)&preset;
	for(i=0; i < sizeof(struct DayDream_AccessPreset) ; i++) {
		s[i]=0;
	}

	strcpy(gerror,"ACCESS.DAT");
	preset.ACCESS_PRESETID=cfgint2("PRESET_NUMBER");
	sprintf(gerror,"ACCESS.DAT, ID %d",preset.ACCESS_PRESETID);

	preset.ACCESS_SECLEVEL=cfgint2("PRESET_SEC");
	preset.ACCESS_FREEFILES=cfgint2("PRESET_FFILES");
	preset.ACCESS_FREEBYTES=cfgint2("PRESET_FBYTES");
	cfgcopy2(preset.ACCESS_DESCRIPTION,29,"PRESET_DESC");
	preset.ACCESS_STATUS=cfgint2("PRESET_STATUS");

	write(outfile,&preset,sizeof(struct DayDream_AccessPreset));
	src=endblock();
	if (*src=='+') goto nextpreset;
	if (*src!='~') {
		printf("Error in access.dat!\n\n%30.30s",src);
		close(outfile);
		exit(0);
	}	
	close(outfile);

	chown("data/access.dat",uid,gid);

	src=findtxt(inmem,"!PROTOCOLS.DAT");
	if (src==0) {
		printf("Can't find definitions for PROTOCOLS.DAT!\n\n");
		exit(0);
	}

	outfile = open("data/protocols.dat", O_CREAT|O_TRUNC|O_RDWR, 0640);
	if (outfile == -1) {
		perror("data/protocols.dat");
		exit(0);
	}
	fchmod(outfile, 0640);
nextproto:

	strcpy(gerror,"PROTOCOLS.DAT");
	s=(char *)&proto;
	for(i=0; i < sizeof(struct DayDream_AccessPreset) ; i++) {
		s[i]=0;
	}

	proto.PROTOCOL_ID=cfgchar2("XPR_ID");
	sprintf(gerror,"PROTOCOLS.DAT, ID %c",proto.PROTOCOL_ID);
	cfgcopy2(proto.PROTOCOL_NAME,20,"XPR_NAME");
	proto.PROTOCOL_EFFICIENCY=cfgint2("XPR_EFFICIENCY");
	proto.PROTOCOL_TYPE=cfgint2("XPR_TYPE");

	write(outfile,&proto,sizeof(struct DayDream_Protocol));
	src=endblock();
	if (*src=='+') goto nextproto;
	if (*src!='~') {
		printf("Error in protocols.dat!\n\n%30.30s",src);
		close(outfile);
		exit(0);
	}	
	close(outfile);

	chown("data/protocols.dat",uid,gid);

	mymkdir("bulletins", 0750);
	mymkdir("logfiles", 0770);
	mymkdir("questionnaire", 0750);
	mymkdir("users", 0770);
	mymkdir("temp", 0770);
	mymkdir("configs", 0750);

	return 0;
}

static int cfgconf2(char *id)
{
	int confs=0;
	int i;

	src=findstrinblock(id);
	if (!src) cfgerror(id);

	for (i=0; i != 32 ; i++)
	{
		if(*src=='X') confs |= (1L << i);
		src++;
	}
	return confs;
}

static char cfgchar2(char *id)
{
	char charru;

	src=findstrinblock(id);
	if (!src) cfgerror(id);
	
	charru=src[0];
	return charru;
}
 
static int cfgflag2(const char *on, const char *id)
{
	int res = 0;
	
	src = findstrinblock(id);
	if (!src) 
		cfgerror(id);

	if (src[0] == on[0]) 
		res = 1;
	return res;
}

static int cfgint2(const char *id)
{
	char	intb[50];
	int i;
	src=findstrinblock(id);
	if (!src) cfgerror(id);

	for(i=0;i<50; i++) {
		if (src[i]==10 || src[i]==13) break;
		intb[i]=src[i];
	}
	intb[i]=0;
	
	return atoi(intb);

}

static char * cfgfnadd3(unsigned short *zone, unsigned short *net, 
	unsigned short *node, unsigned short *point, char *id)
{
	char cb[1024];
	char *t, *s;
	*zone=*net=*node=*point=0;
	
	s=findstrinblock(id);
	if (!s) cfgerror(id);

	if (*s=='-'||*s==10||*s==13||*s < '0' || *s > '9') {
		return s;
	}
	
	t=cb;
	while(*s!=':' && *s) *t++=*s++;
	*t=0;
	*zone=atoi(cb);
	if (!*s) return s; else s++;
	
	t=cb;
	while(*s!='/' && *s) *t++=*s++;
	*t=0;
	*net=atoi(cb);
	if (!*s) return s; else s++;

	t=cb;
	while(*s!='.' && *s && *s!=13 && *s!=10) *t++=*s++;
	*t=0;
	*node=atoi(cb);
	if (!*s || *s==13 || *s==10) return s; else s++;

	t=cb;
	while(*s && *s!=10 && *s!=13) *t++=*s++;
	*t=0;
	*point=atoi(cb);
	return s;

}
 
static char *cfgcopy(char *dest, int max)
{
	int i;

	if (src[0]=='-' && src[1]==10) {
		dest[0]=0;
		src=nextspace(&src[1]);
		return 0;
	}
	
	for (i=0;i < max ; i++) {
		if (src[i]==10 || src[i]==13) break;
		dest[i]=src[i];
	}
	dest[i]=0;
	if (i==max) {
		src=&src[i];
		while(*src!=10 && *src!=13) src++;
		i=0;
	}
	removespaces(dest);
	src=nextspace(&src[i]);
	return 0;
}

static void cfgerror(const char *id)
{
	fprintf(stderr, "Error in %s: %s missing!\n", gerror, id);
	exit(1);
}

static char *cfgcopy2(char *dest, int max, const char *ident)
{
	int i;
	src=findstrinblock(ident);
	if (!src) cfgerror(ident);

	if (src[0]=='-' && src[1]==10) {
		dest[0]=0;
		return 0;
	}
	
	for (i=0;i < max ; i++) {
		if (src[i]==10 || src[i]==13) break;
		dest[i]=src[i];
	}
	dest[i]=0;
	if (i==max) {
		src=&src[i];
		while(*src!=10 && *src!=13) src++;
		i=0;
	}
	removespaces(dest);
	return 0;
}

static void removespaces(char *strh)
{
	char *s;
	s=strh;
	if (!*s) return;
	while(*s) s++;
	s--;
	while (*s==' ') s--;
	*(s+1)=0;
}

static char *nextspace(char *s)
{
	while (*s!=' ') {
		if (*s==10 && s[1] == '~') break;
		if (*s==10 && s[1] == '+') break;
		if (*s==10 && s[1] == '*') break;
		s++;
	}
	s++;
	return s;
}

static char *findstrinblock(const char *text)
{
	char *buffb;
	
	buffb = blockstart;
	for (;;) {
		while(*buffb != 10) {
			if (*buffb == 0) 
				return 0;
			buffb++;
		}
		buffb++;
		if (*buffb == '~' || *buffb == '+' || *buffb == '*') {
			return 0;
		}
		if (!strncmp(text, buffb, strlen(text))) { 
			while(*buffb != ' ') 
				buffb++;
			return ++buffb;
		}
	}
}

static char *findtxt(char *buffer, const char *text)
{
	const char *s;
	char *buffb;
nextline:
	while (*buffer != 10) {
		if (*buffer == 0) 
			return 0;
		buffer++;
	}
	
	buffer++;
	s = text;
	buffb = buffer;
	
	for(;;) {
		if (*s == 0) {
			blockstart = buffer;
			return buffer;
		}
		if (*buffer != *s) 
			goto nextline;
		s++; 
		buffer++;
	}
}

static char *ecfg(char *hay, const char *need)
{
	const char *s;
	for (;;) {
		s = need;
		if (*hay == 0) 
			return 0;
		if (*hay == ';') { 
			while (*hay != 10) {
				if(*hay == 0) 
					return 0;
				hay++;
			}
			continue;
		}
		for (;;) {
			if (*s++ == *hay++) {
				if (*s == 0) 
				return hay;
			} else {
				break;
			}
		}
	}
}

static int mymkdir(char *dir, mode_t mode)
{
	char *p, ch;

	if (*dir == '@')
		return 1;
	
	p = strrchr(dir, '/');
	if (p == dir || !p)
		return 1;
	
	ch = *p;
	*p = 0;
	
	mymkdir(dir, mode);
	
	*p = ch;
	if (!mkdir(dir, mode)) {
		chown(dir, uid, gid);
		if (chmod(dir, mode) == -1) 
			fprintf(stderr, "cannot chmod(): %s\n", 
				strerror(errno));
	}
	
	return 1;
}
