#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/time.h>
#include <time.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include <daydream.h>
#include <ddcommon.h>

static int bigolm(int, int, const char *);
static int lineolm(int, const char *);
static int miniolm(int, const char *);

int olmsg(char *params)
{
	char obuf[1024];
	int destnode;
	const char *srcstrh;
	char parbuf[1024];

	srcstrh = params;


	if (!(srcstrh = strspa(srcstrh, parbuf, 1024))) {
	      asknode:
		*obuf = 0;
		DDPut(sd[olmdstr]);
		if (!(Prompt(obuf, 80, 0)))
			return 0;
		srcstrh = strspa(obuf, parbuf, 1024);
	}
	if (*parbuf == '*') {
		destnode = -1;
	} else if (toupper(*parbuf) == 'V') {
		who();
		goto asknode;
	} else
		destnode = atoi(parbuf);

	if (destnode == 0)
		return 0;

	if (destnode > 0) {
		struct DayDream_NodeInfo ninfo;

		if (!isnode(destnode, &ninfo)) {
			DDPut(sd[olmnouserstr]);
			return 1;
		}
	}
	if (srcstrh && *srcstrh && *++srcstrh) {
		if (destnode == -1) {
			olmall(0, srcstrh);
		} else {
			if (!lineolm(destnode, srcstrh)) {
				DDPut(sd[olmnomsgstr]);
			}
		}

	} else {
		/* XXX: replace size with a constant */
		char mem[40000];
		int lins;
		lins = edfile(mem, sizeof mem, 0, 0);
		if (!lins)
			return 1;
		if (destnode == -1) {
			olmall(lins, mem);
		} else {
			if (!bigolm(destnode, lins, mem)) {
				DDPut(sd[olmnomsgstr]);
			}
		}

	}
	return 1;
}

int olmall(int l, const char *ms)
{
	struct DayDream_Multinode *cn;

	cn = nodes;

	while (cn->MULTI_NODE) {
		struct DayDream_NodeInfo nin;
		if (cn->MULTI_NODE == 254) {
			int j;
			int i = maincfg.CFG_TELNET1ST;
			j = maincfg.CFG_TELNETMAX;

			while (j) {
				if (isnode(i, &nin)) {
					if (l == 1)
						bigolm(i, l, ms);
					else if (l == 2)
						miniolm(i, ms);
					else
						lineolm(i, ms);
				}
				j--;
				i++;
			}
		} else if (cn->MULTI_NODE == 253) {
			int j;
			int i = maincfg.CFG_LOCAL1ST;
			j = maincfg.CFG_LOCALMAX;

			while (j) {
				if (isnode(i, &nin)) {
					if (l == 1)
						bigolm(i, l, ms);
					else if (l == 2)
						miniolm(i, ms);
					else
						lineolm(i, ms);
				}
				j--;
				i++;
			}
		} else if (node != cn->MULTI_NODE && isnode(cn->MULTI_NODE, &nin)) {
			if (l == 1)
				bigolm(cn->MULTI_NODE, l, ms);
			else if (l == 2)
				miniolm(cn->MULTI_NODE, ms);
			else
				lineolm(cn->MULTI_NODE, ms);
		}
		cn++;
	}
	return 1;
}

static int bigolm(int dnode, int l, const char *ms)
{
	struct dd_nodemessage ddn;
	struct timeval tv;
	char bbuf[1024];
	int olfd;
	struct DayDream_NodeInfo nin;

	isnode(dnode, &nin);
	if ((nin.ddn_flags & (1L << 0)) == 0)
		return 0;

	memset(&ddn, 0, sizeof(struct dd_nodemessage));
	ddn.dn_command = 2;
	snprintf(ddn.dn_string, sizeof ddn.dn_string,
	    "[36m\nMessage from %s (%s) on node %d:\n[0m", user.user_handle,
	    user.user_realname, node);

	gettimeofday(&tv, 0);
	ddn.dn_data1 = tv.tv_usec;
	snprintf(bbuf, sizeof bbuf, "%s/olm%d.%d", DDTMP, dnode, 
		(int) tv.tv_usec);

	olfd = open(bbuf, O_WRONLY | O_CREAT | O_TRUNC, 0666);
	if (olfd != -1) {
		const char *s = ms;
		fsetperm(olfd, 0666);
		while (l) {
			snprintf(bbuf, sizeof bbuf, "%s\n", s);
			safe_write(olfd, bbuf, strlen(bbuf));
			l--;
			s = &s[80];
		}
		close(olfd);
	}
	sendtosock(dnode, &ddn);
	return 1;
}

static int lineolm(int dnode, const char *ms)
{
	struct dd_nodemessage ddn;

	struct DayDream_NodeInfo nin;

	isnode(dnode, &nin);
	if ((nin.ddn_flags & (1L << 0)) == 0)
		return 0;

	memset(&ddn, 0, sizeof(struct dd_nodemessage));
	ddn.dn_command = 1;
	snprintf(ddn.dn_string, sizeof ddn.dn_string,
	     "[36m\nMessage from %s (%s) on node %d:\n[0m%-80.80s\n\n", 
	     user.user_handle, user.user_realname, node, ms);
	sendtosock(dnode, &ddn);
	return 1;
}

static int miniolm(int dnode, const char *ms)
{
	struct dd_nodemessage ddn;

	struct DayDream_NodeInfo nin;

	isnode(dnode, &nin);
	if ((nin.ddn_flags & (1L << 0)) == 0)
		return 0;

	if (dnode == node)
		return 0;
	memset(&ddn, 0, sizeof(struct dd_nodemessage));
	ddn.dn_command = 1;
	strlcpy(ddn.dn_string, ms, sizeof ddn.dn_string);

	sendtosock(dnode, &ddn);
	return 1;
}
