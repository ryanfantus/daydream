#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>

#include <daydream.h>
#include <ddcommon.h>

int pageflag;
char reason[100];

int pagesysop(const char *reas)
{
	struct DayDream_PageMsg pm;
	struct sockaddr_un name;
	int sock;

	time_t ctim;
	int i;

	changenodestatus("Paging SysOp");

	reason[0] = 0;
	if (reas) 
		strlcpy(reason, reas, sizeof reason);

	TypeFile("pagesysop", TYPE_MAKE | TYPE_WARN);

	if (maincfg.CFG_FLAGS & (1L << 0)) {
		if (reason[0] == 0) {
			DDPut(sd[psreasonstr]);
			if (!(Prompt(reason, 75, 0)))
				return 0;
			if (reason[0] == 0) {
				DDPut("\n");
				return 0;
			}
		}
	}
	clog.cl_flags |= CL_PAGEDSYSOP;


	sock = socket(AF_UNIX, SOCK_DGRAM, 0);
	if (sock < 0) 
		return 0;

	pm.pm_cmd = 2;
	ctim = time(0);
	*pm.pm_string = 0;
	if (onlinestat)
		snprintf(pm.pm_string, sizeof pm.pm_string, 
				"\n\n%s / %s (node %d) paged you on %s\n", 
				user.user_realname, user.user_handle, 
				node, ctime(&ctim));
	if (reason[0]) {
		strlcat(pm.pm_string, "Reason: ", sizeof pm.pm_string);
		strlcat(pm.pm_string, reason, sizeof pm.pm_string);
		strlcat(pm.pm_string, "\n\n", sizeof pm.pm_string);
	}

	name.sun_family = AF_UNIX;
	strlcpy(name.sun_path, YELLDSOCK, sizeof name.sun_path);

	if (sendto(sock, &pm, sizeof(struct DayDream_PageMsg), 0, (struct sockaddr *) &name, sizeof(struct sockaddr_un)) < 0) {
		DDPut(sd[pspageoffstr]);
		close(sock);
		return 0;
	}

	pages--;
	ddprintf(sd[pspagingstr], maincfg.CFG_SYSOPNAME);

	pageflag = 0;
	for (i = 0; i < 20; i++) {
		unsigned char c;
		DDPut(".");
		delayt = 1;
		while ((c = HotKey(HOT_QUICK))) {
			if (c == 255)
				break;
			if (c == 3) {
				DDPut(sd[psabortstr]);
				i = 21;
				break;
			}
		}
		if (pageflag)
			break;

		pm.pm_cmd = 1;

		name.sun_family = AF_UNIX;
		strlcpy(name.sun_path, YELLDSOCK, sizeof name.sun_path);
		sendto(sock, &pm, sizeof(struct DayDream_PageMsg), 0, (struct sockaddr *) &name, sizeof(struct sockaddr_un));

		sleep(1);
	}
	if (i == 20)
		DDPut(sd[psnosysopstr]);

	close(sock);

	return 0;

}
