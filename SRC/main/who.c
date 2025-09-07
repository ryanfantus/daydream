#include <fcntl.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>

#include <daydream.h>
#include <ddcommon.h>
#include <symtab.h>
#include <ctype.h>

static int who_show(struct DayDream_Multinode *, int);

void who(void)
{
	struct DayDream_Multinode *cn;

	cn = nodes;

	changenodestatus("Viewing who's online");

	TypeFile("who", TYPE_MAKE | TYPE_WARN);
	DDPut(sd[whheadstr]);

	/* Process all nodes in the configuration file, not just until first zero.
	 * The nodes array is terminated by a zero byte at the end, so we continue
	 * until we find that terminator, not just the first zero MULTI_NODE value.
	 */
	while (1) {
		/* Check if we've reached the end marker (zero byte) */
		if (*(unsigned char *)cn == 0)
			break;
			
		if (cn->MULTI_NODE == 254) {
			int j;
			int i = maincfg.CFG_TELNET1ST;
			j = maincfg.CFG_TELNETMAX;

			while (j) {
				j--;
				who_show(cn, i);
				i++;
			}
		} else if (cn->MULTI_NODE == 253) {
			int j;
			int i = maincfg.CFG_LOCAL1ST;
			j = maincfg.CFG_LOCALMAX;

			while (j) {
				j--;
				who_show(cn, i);
				i++;
			}
		} else if (cn->MULTI_NODE != 252 && cn->MULTI_NODE != 0) {
			who_show(cn, cn->MULTI_NODE);
		}
		cn++;
	}
	DDPut(sd[whtailstr]);
}

static int who_show(struct DayDream_Multinode *cn, int num)
{
	struct DayDream_NodeInfo myn;
	const char *activity;
	char dabps[30];
	struct userbase tmpuser;
	char usern[26];
	char orgn[26];
	
	int showent = 1;

	memset(&myn, 0, sizeof(struct DayDream_NodeInfo));

	if (isnode(num, &myn)) {
		if (cn->MULTI_OTHERFLAGS & (1L << 4)) {
			showent = 0;
		} else {
			if (getubentbyid(myn.ddn_userslot, &tmpuser) == -1) {
				*usern = 0;
				*orgn = 0;
				activity = myn.ddn_activity;
				snprintf(dabps, sizeof dabps, "%d",
					myn.ddn_bpsrate);
			} else {
				strlcpy(orgn, maincfg.CFG_FLAGS & (1L << 2) ?
					tmpuser.user_organization : 
					tmpuser.user_zipcity, sizeof orgn);
				strlcpy(usern, maincfg.CFG_FLAGS & (1L << 1) ?
					tmpuser.user_handle : tmpuser.user_realname, 
					sizeof usern);
			}
			activity = myn.ddn_activity;
			snprintf(dabps, sizeof dabps, "%d", 
			myn.ddn_bpsrate);
		}			
	} else {
		if (cn->MULTI_OTHERFLAGS & (1L << 3))
			showent = 0;
		else {
			*usern = 0;
			*orgn = 0;
			activity = "Waiting for a call...";
			snprintf(dabps, sizeof dabps, "%d", 0);
		}
	}
	if (showent) {
		/* Determine connection type based on node number range */
		if (num >= maincfg.CFG_TELNET1ST && num < maincfg.CFG_TELNET1ST + maincfg.CFG_TELNETMAX) {
			/* Node is in telnet range */
			if (isnode(num, &myn) && myn.ddn_bpsrate > 0) {
				snprintf(dabps, sizeof dabps, "%d", myn.ddn_bpsrate);
			} else {
				strlcpy(dabps, "TELNET", sizeof dabps);
			}
		} else if (num >= maincfg.CFG_LOCAL1ST && num < maincfg.CFG_LOCAL1ST + maincfg.CFG_LOCALMAX) {
			/* Node is in local range */
			strlcpy(dabps, "LOCAL", sizeof dabps);
		} else {
			/* Fall back to original logic for other node types */
			if (isnode(num, &myn) && myn.ddn_bpsrate == 0) {
				/* Zero bps rate typically indicates telnet connection */
				strlcpy(dabps, "TELNET", sizeof dabps);
			} else {
				switch (cn->MULTI_TTYTYPE) {
				case 1:
					strlcpy(dabps, "LOCAL", sizeof dabps);
					break;
				case 2:
					if (myn.ddn_flags & (1L << 1)) {
						snprintf(dabps, sizeof dabps, "%d", 
							myn.ddn_bpsrate);
					} else {
						strlcpy(dabps, "TELNET", sizeof dabps);
					}
					break;
				default:
					if (myn.ddn_bpsrate > 0) {
						snprintf(dabps, sizeof dabps, "%d", 
							myn.ddn_bpsrate);
					} else {
						strlcpy(dabps, "TELNET", sizeof dabps);
					}
					break;
				}
			}
		}
		ddprintf(sd[whlinestr], num, usern, orgn, 
			 activity, dabps);
	}
	
	return 1;
}

int ispid(pid_t pid)
{
	return kill(pid, 0) != -1;
}

int isnode(int nod, struct DayDream_NodeInfo *ndnfo)
{
	char infoname[PATH_MAX];
	int nodefd;

	snprintf(infoname, sizeof infoname, "%s/nodeinfo%d.data", DDTMP, nod);
	nodefd = open(infoname, O_RDONLY);
	if (nodefd != -1) {
		read(nodefd, ndnfo, sizeof(struct DayDream_NodeInfo));
		close(nodefd);
		if (ispid(ndnfo->ddn_pid))
			return 1;
	}
	return 0;
}
