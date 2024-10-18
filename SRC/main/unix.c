#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <daydream.h>
#include <ddcommon.h>
#include <symtab.h>
#include <console.h>

/* FIXME: sertty is never used in anything sensible. Maybe it
 * is necessary to write something in wfc.c? */
static struct termios oldtty;
static struct termios sertty;

int dsockfd;
static struct sockaddr_un ddsock;
int idleon = 1;
int fnode = 0;
int bgmode = 0;

static list_t *input_queue;
static void smallstatus(void);
static void sysopsend(void);
static int getfreelnode(void);
static int getfreetnode(void);
static int getnodeinfo(void);
static void showhelp(void);

int is_telnet_connection(void)
{
	return !lmode && !fnode;
}

/* The interprocess communication between nodes (for example, 
 * online messages) is implemented with sockets.
 */  
static void create_internode_socket(void)
{
	char socket_name[4096];
	if ((dsockfd = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0) {
		perror("cannot create communication socket");
		exit(1);
	}
	snprintf(socket_name, sizeof socket_name, "%s/dd_sock%d", DDTMP, node);
	unlink(socket_name);
	/* FIXME: I don't know whether this could be a possible
	 * victim of symlink attack.
	 */
	strncpy(ddsock.sun_path, socket_name, sizeof ddsock.sun_path);
	ddsock.sun_path[sizeof ddsock.sun_path - 1] = 0;
	ddsock.sun_family = AF_UNIX;
	if (bind(dsockfd, (struct sockaddr *) &ddsock, sizeof ddsock) < 0) {
		perror("cannot bind communication socket");
		close(dsockfd);
		exit(1);
	}
}

int initterm(void)
{
	struct termios tty;
	char *envstr;

	if (getnodeinfo() == 0)
		exit(1);

	if (is_telnet_connection()) {
		envstr = (char *) xmalloc(strlen(origdir) + 10);
		sprintf(envstr, "DAYDREAM=%s", origdir);	
		putenv(envstr);
		serhandle = open(ttyname(0), O_RDWR);

		/* Create communication fifos for DDSnoop */

	}

	create_internode_socket();
	if (init_console() == -1) {
		log_error(__FILE__, __LINE__,
			"cannot initialize console: %s (%d)",
			strerror(errno), errno);
		exit(1);
	}

	tcgetattr(0, &tty);
	oldtty = tty;
	
	tty.c_iflag &= ~(IGNBRK | IGNCR | INLCR | ICRNL |
			 IXANY | IXON | IXOFF | INPCK | ISTRIP);
	
#ifdef HAVE_IUCLC
	tty.c_iflag &= ~IUCLC;
#endif
	tty.c_iflag |= (BRKINT | IGNPAR);
	tty.c_oflag &= ~OPOST;
	tty.c_lflag &= ~(ECHONL | NOFLSH);
#ifdef HAVE_XCASE
	tty.c_lflag &= ~XCASE;
#endif
	tty.c_lflag &= ~(ICANON | ISIG | ECHO);
	tty.c_cflag |= CREAD;
	tty.c_cc[VTIME] = 5;
	tty.c_cc[VMIN] = 1;
	tcsetattr(0, TCSANOW, &tty);

	atexit(clear);
	return 1;
}

void clear(void)
{
	char puskuri[512];

/* FIXME: this is bogus. Maybe in wfc.c? */
/*	if (conon)
		tcsetattr(conin, TCSANOW, &sertty);
	if (conout)
		tcsetattr(conout, TCSANOW, &sertty);*/

	/* what the heck? */
	tcsetattr(0, TCSANOW, &oldtty);

	finalize_console();
	snprintf(puskuri, sizeof puskuri, "%s/daydream%dr", DDTMP, node);
	unlink(puskuri);
	close(dsockfd);
	snprintf(puskuri, sizeof puskuri, "%s/dd_sock%d", DDTMP, node);
	unlink(puskuri);
	snprintf(puskuri, sizeof puskuri, "%s/daydream%dw", DDTMP, node);
	unlink(puskuri);
	snprintf(puskuri, sizeof puskuri, "%s/nodeinfo%d.data", DDTMP, node);
	unlink(puskuri);

}

/* In case a node gets stuck and the socket is not in non blocking mode,
 * the send buffer of the corresponding socket would become full and the
 * sendto() function would wait forever. Ultimately, the every node of the
 * board would cease to function properly.
 *
 * Setting the non blocking mode on a socket is not the perfect solution.
 * Under heavy load, it is possible that a nodemessage will be lost.
 */
int sendtosock(int dnode, struct dd_nodemessage *dn)
{
	int sock, fl;
	struct sockaddr_un name;

	snprintf(name.sun_path, sizeof name.sun_path, "%s/dd_sock%d", DDTMP,
	     dnode);
	name.sun_family = PF_UNIX;
	sock = socket(AF_UNIX, SOCK_DGRAM, 0);
	if (sock < 0)
		return 0;

	fl = fcntl(sock, F_GETFL, 0);
	fcntl(sock, F_SETFL, fl | O_NONBLOCK);
	
	sendto(sock, dn, sizeof(struct dd_nodemessage), 0,
	        (struct sockaddr *) &name, sizeof(struct sockaddr_un));
	close(sock);
	return 1;
}

static int getnodeinfo(void)
{
	char *mytty;
	struct DayDream_Multinode *danode;
	UTMPX *ut_rec;
	char parbuf[80];
	const char *s;

	if (lmode)
		return getfreelnode();

	mytty = ttyname(serhandle);
	if (!mytty) {
		printf("Unknown tty: %s\n\nexiting...", mytty);
		return 0;
	}		

	danode = nodes;

	if (fnode) {
		while (danode->MULTI_NODE) {
			if (danode->MULTI_NODE == fnode) {
				currnode = danode;
				node = fnode;
				bpsrate = danode->MULTI_TTYSPEED;
				return 1;
			} else {
				danode++;
			}
		}
		return 0;
	}
	while (danode->MULTI_NODE) {
		if (!(strcmp(danode->MULTI_TTYNAME, mytty))) {
			node = danode->MULTI_NODE;
			currnode = danode;
			if (danode->MULTI_TTYTYPE == 0) {
				setutxent();
				while ((ut_rec = getutxent())) {
					if (!(strcmp(ut_rec->ut_line, &mytty[5]))) {
#ifdef USER_PROCESS
						if (ut_rec->ut_type == USER_PROCESS) {
#else
						{
#endif
							bpsrate = atoi(ut_rec->ut_host);
							endutxent();
							return 1;
						}
					}
				}
				endutxent();
				printf("Can't parse utmp!\n\nexiting...");
				return 0;
			} else {
				bpsrate = danode->MULTI_TTYSPEED;
				return 1;
			}
		} else {
			danode++;
		}
	}
	s = maincfg.CFG_TELNETPAT;

	for (;;) {
		if (strtoken(parbuf, &s, sizeof parbuf) > sizeof parbuf)
			continue;
		if (!*parbuf)
			break;
		if (wildcmp(mytty, parbuf)) 
			return getfreetnode();
	}

	s = maincfg.CFG_LOCALPAT;
	for (;;) {
		if (strtoken(parbuf, &s, sizeof parbuf) > sizeof parbuf)
			continue;
		if (!*parbuf)
			break;
		if (wildcmp(mytty, parbuf)) 
			return getfreelnode();
	}
		
	printf("Unknown tty: %s\n\nexiting...", mytty);
	return 0;
}

static int getfreetnode(void)
{
	struct DayDream_Multinode *danode;

	danode = nodes;

	while (danode->MULTI_NODE) {
		if (danode->MULTI_NODE == 254) {
			int i, j;
			i = maincfg.CFG_TELNETMAX;
			j = maincfg.CFG_TELNET1ST;
			while (i) {
				struct DayDream_NodeInfo ni;
				if (!isnode(j, &ni)) {
					currnode = (struct DayDream_Multinode *) xmalloc(sizeof(struct DayDream_Multinode));
					memcpy(currnode, danode, sizeof(struct DayDream_Multinode));
					node = j;
					snprintf(currnode->MULTI_TEMPORARY, 
					    sizeof currnode->MULTI_TEMPORARY,
					    danode->MULTI_TEMPORARY, j);
					currnode->MULTI_NODE = node;
					bpsrate = danode->MULTI_TTYSPEED;
					changenodestatus("Logging in..");
					return 1;
				}
				i--;
				j++;
			}
			printf("All telnet nodes in use. Call back later.\n");
			return 0;
		}
		danode++;
	}
	printf("We don't support telnet. Get out.\n");
	return 0;
}
	
static int getfreelnode(void)
{
	struct DayDream_Multinode *danode;

	danode = nodes;

	while (danode->MULTI_NODE) {
		if (danode->MULTI_NODE == 253) {
			int i, j;
			i = maincfg.CFG_LOCALMAX;
			j = maincfg.CFG_LOCAL1ST;
			while (i) {
				struct DayDream_NodeInfo ni;
				if (!isnode(j, &ni)) {
					currnode = (struct DayDream_Multinode *) xmalloc(sizeof(struct DayDream_Multinode));
					memcpy(currnode, danode, sizeof(struct DayDream_Multinode));
					node = j;
					snprintf(currnode->MULTI_TEMPORARY, 
					    sizeof currnode->MULTI_TEMPORARY,
					    danode->MULTI_TEMPORARY, j);
					currnode->MULTI_NODE = node;
					bpsrate = danode->MULTI_TTYSPEED;
					changenodestatus("Logging in..");
					return 1;
				}
				i--;
				j++;
			}
			printf("All local nodes in use. \n");
			return 0;
		}
		danode++;
	}
	printf("No local nodes\n");
	return 0;
}

int input_queue_empty(void)
{
	return input_queue == NULL;
}

int input_queue_get(void)
{
	return shift(int, input_queue);
}	
	
void keyboard_stuff(char *what)
{
	while (*what) 
		cons(input_queue, (void *)(int) *what++);
}

void init_keyboard(void)
{
	input_queue = NULL;
}

void fini_keyboard(void)
{
	while (input_queue)
		shift(void *, input_queue);
}

int handlectrl(int params)
{
	int pala;
	int oldansi;

	pala = HotKey(params | HOT_RE);
	if (pala == 2)
		return pala;
	else if (pala == 1) {
		open_console();
		smallstatus();
		pala = HotKey(params | HOT_RE);
		if (pala)
			maincfg.CFG_LOCALSCREEN = pala;
		pala = HotKey(params);
		return pala;
	} else if (pala == 4) {
		close_console();
		pala = HotKey(params);
		return pala;
	} else if (pala == 'c' || pala == 'C') {
		LineChat();
		pala = HotKey(params);
		return pala;
	} else if (pala == '1') {
		DDPut(sd[accountedstr]);
		oldansi = ansi;
		ansi = 1;
		userinput = 0;
		usered();
		ansi = oldansi;
		userinput = 1;
		pala = HotKey(params);
		return pala;
	} else if (pala == '2') {
		usered();
		pala = HotKey(params);
		return pala;
	} else if (pala == 'h' || pala == 'H') {
		oldansi = ansi;
		ansi = 1;
		userinput = 0;

		DDPut("[33m\nSnooping commands:\n\n");
		DDPut("[36mCtrl-b-h [34m- [0mHelp\n");
		DDPut("[36mCtrl-b-1 [34m- [0mUser editor (SysOp only)\n");
		DDPut("[36mCtrl-b-2 [34m- [0mUser editor (SysOp + user)\n");
		DDPut("[36mCtrl-b-c [34m- [0mChat\n");
		DDPut("[36mCtrl-b-s [34m- [0mWho's online\n");
		DDPut("[36mCtrl-b-u [34m- [0mSend files (free download)\n");
		DDPut("[36mCtrl-b-r [34m- [0mReceive files (-> chat download)\n");
		ansi = oldansi;
		userinput = 1;
		pala = HotKey(params);
		return pala;
	} else if (pala == 's' || pala == 'S') {
		smallstatus();
		pala = HotKey(params);
		return pala;
	} else if (pala == 'u' || pala == 'U') {
		sysopsend();
		pala = HotKey(params);
		return pala;
	} else if (pala == 'r' || pala == 'R') {
		recfiles(maincfg.CFG_CHATDLPATH, 0);
		pala = HotKey(params);
		return pala;
	} else
		return pala;
}

static void smallstatus(void)
{
	int oldansi;
	oldansi = ansi;
	ansi = 1;
	userinput = 0;

	if (onlinestat) 
		ddprintf("\n[36mUser online\n\n[0mName: %s (%s)\nFrom: %s (%s)\nUls : %Lu / %d\nDls : %Lu / %d\nCalls/Slot/Sec/Time left: %d / %d / %d /%d\n", user.user_realname, user.user_handle, user.user_zipcity, user.user_organization, user.user_ulbytes, user.user_ulfiles, user.user_dlbytes, user.user_dlfiles, user.user_connections, user.user_account_id, user.user_securitylevel, timeleft / 60);
	else 
		DDPut("\nNo user online!\n");
	ansi = oldansi;
	userinput = 1;

}

static void sysopsend(void)
{
	int oldansi;
	char sbu[500];
	char flname[80];

	oldansi = ansi;
	ansi = 1;
	userinput = 0;

	DDPut("[0m\nFiles to send: [36m");
	*sbu = 0;
	if (!(Prompt(sbu, 500, 0)))
		return;

	snprintf(flname, sizeof flname, "%s/filelist.%d", DDTMP, node);
	if (makeflist(flname, sbu)) {
		sendfiles(flname, sbu, sizeof flname);
	}
	ansi = oldansi;
	userinput = 1;
}

int getcmdline(int args, char *argi[])
{
	char *cp;

	ul_user = -1;

	while (--args) {
		cp = *++argi;
		if (*cp == '-') {
			while (*++cp) {
				switch (*cp) {
				case 'd':
					idleon = 0;
					break;
				case 'l':
					lmode = 1;
					break;
				case 'n':
					if (--args < 1) {
						showhelp();
						return 0;
					}
					fnode = atoi(*++argi);
					break;
				case 's':
					if (--args < 1) {
						showhelp();
						return 0;
					}
					forcebps = atoi(*++argi);
					break;
				case 'u':
					if (--args < 3) {
						showhelp();
						return 0;
					}
					ul_user = atoi(*++argi);
					args -= 2;
					ul_conf = atoi(*++argi);
					ul_file = *++argi;
					break;
				default:
					showhelp();
					return 0;
				}

			}
		} else {
			showhelp();
			return 0;
		}
	}
	return 1;
}

static void showhelp(void)
{
	printf("DayDream BBS " versionstring " by Antti Häyrynen\n\n"
	       "Usage: daydream [-d] [-l] [-n node] [-s bps]\n\n"
	       "      -d -> disable idle\n"
	       "      -l -> local login\n"
	       "      -n -> force node\n"
	       "      -s -> call has been answered by mailer -> enter the BBS as\n"
	       "            <bps> connection.\n"
	       "      -u -> local upload (-u <usernum> <confnum> </path/to/filename>\n"
	       "            Filename must contain a diz or it won't be imported.\n");
}
