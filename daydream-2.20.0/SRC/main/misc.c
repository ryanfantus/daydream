#include <errno.h>
#include <stdarg.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <ctype.h>
#include <syslog.h>
#include <sys/file.h>

#include <daydream.h>
#include <ddcommon.h>
#include <symtab.h>

int node; /* FIXME! are these really to be declared here? */
int bpsrate;

static int isonck(int, int);

int runlogoffbatch(void)
{
	char buf[512];
	snprintf(buf, sizeof buf, "batch/batch%d.logoff %d", node, node);
	runstdio(buf, -1, 3);
	return 1;
}

void removespaces(char *strh)
{
	char *s;
	s = strh;
	if (!*s)
		return;
	while (*s)
		s++;
	s--;
	while (*s == ' ')
		s--;
	*(s + 1) = 0;
}

void changenodestatus(const char *newstatus)
{
	struct DayDream_NodeInfo ddn;
	char infoname[80];
	int myfd;

	if (bgmode)
		return;

	ddn.ddn_pid = getpid();
	ddn.ddn_flags = 0;
	if (onlinestat) {
		ddn.ddn_userslot = user.user_account_id;
		if ((user.user_toggles & (1L << 9)) == 0) {
			ddn.ddn_flags |= (1L << 0);
		}
	} else
		ddn.ddn_userslot = -1;
	strlcpy(ddn.ddn_activity, newstatus, sizeof ddn.ddn_activity);
	ddn.ddn_bpsrate = bpsrate;
	strncpy(ddn.ddn_pagereason, reason, 79);
	strncpy(ddn.ddn_path, origdir, 79);
	ddn.ddn_timeleft = timeleft;

	snprintf(infoname, sizeof infoname, "%s/nodeinfo%d.data",
		DDTMP, node);
	myfd = open(infoname, O_WRONLY | O_CREAT, 0666);
	if (myfd != -1) {
		fsetperm(myfd, 0666);
		safe_write(myfd, &ddn, sizeof(struct DayDream_NodeInfo));
		close(myfd);
	}
}

char *currt(void)
{
	time_t tt;

	tt = time(0);
	return ctime(&tt);
}

void writelog(const char *strh)
{
	char buffer[80];
	int logfd;


	snprintf(buffer, sizeof buffer, "%s/logfiles/daydream%d.log", 
		origdir, node);
	logfd = open(buffer, O_WRONLY | O_CREAT, 0666);
	if (logfd == -1)
		return;
	fsetperm(logfd, 0666);
	lseek(logfd, 0, SEEK_END);
	safe_write(logfd, strh, strlen(strh));
	close(logfd);
}

int wildcmp(const char *nam, const char *pat)
{
	const char *p;

	for (;;) {
		if (tolower(*nam) == tolower(*pat)) {
			if (*nam++ == '\0')
				return (1);
			pat++;
		} else if (*pat == '?' && *nam != 0) {
			nam++;
			pat++;
		} else
			break;
	}

	if (*pat != '*')
		return (0);

	while (*pat == '*') {
		if (*++pat == '\0')
			return (1);
	}

	for (p = nam + strlen(nam) - 1; p >= nam; p--) {
		if (tolower(*p) == tolower(*pat))
			if (wildcmp(p, pat) == 1)
				return (1);
	}
	return (0);
}

int iswilds(const char *strh)
{
	return strchr(strh, '*') || strchr(strh, '?');
}

int findusername(const char *name)
{
	struct userbase user;
	if (!strcasecmp(name, "sysop"))
		return 0;
	if (getubentbyname(name, &user) == -1)
		return -1;
	return user.user_account_id;
}

static int findusername_wildcard(const char *name)
{
	struct userbase user;
	int account_id = 0, key;

	if (!strcasecmp(name, "sysop"))
		return 0;
	
	if (!iswilds(name))
		return findusername(name);

	for (;; account_id++) {
		if (getubentbyid(account_id, &user) == -1)
			return -1;
		if ((user.user_toggles & UBENT_STAT_MASK) == UBENT_STAT_DELETED)
			continue;
		
		ddprintf(sd[wildverstr], user.user_realname, 
			 user.user_handle);
		
		key = HotKey(HOT_YESNO);
		if (key == 1) {
			account_id = user.user_account_id;
			break;
		} else {
			account_id = -2;
			break;
		}
	}

	return account_id;
}

int checklogon(const char *name)
{
	int userpos;

	userpos = findusername_wildcard(name);
	if (userpos == -1)
		return (0);
	if (userpos == -2)
		return -1;
	if ((maincfg.CFG_FLAGS & (1L << 8)) == 0 && isonline(userpos))
		return 2;
	
	if (getubentbyid(userpos, &user) == -1) {
		DDPut(sd[ubrerrstr]);
		return 0;
	}
	
	clog.cl_userid = user.user_account_id;
	clog.cl_firstcall = user.user_firstcall;
	clog.cl_logon = time(0);
	if (user.user_connections == 0)
		clog.cl_flags |= CL_NEWUSER;
	clog.cl_bpsrate = bpsrate;


	return 1;
}

int cmppasswds(char *passwd, unsigned char *thepw)
{
	MD_CTX context;
	unsigned char digest[16];
	char newpw[30];
	int i;

	for (i = 0; i < 16; i++) {
		if (thepw[i]) {
			i = 100;
			break;
		}
	}
	if (i != 100)
		return 1;

	strlcpy(newpw, passwd, sizeof newpw);
	strupr(newpw);

	MDInit(&context);
	MDUpdate(&context, (unsigned char *) newpw, strlen(newpw));
	MDFinal(digest, &context);

	for (i = 0; i < 16; i++) {
		if (thepw[i] != digest[i])
			return (0);
	}
	return (1);
}


int isonline(int id)
{
	struct DayDream_Multinode *cn;

	cn = nodes;

	while (cn->MULTI_NODE) {
		if (cn->MULTI_NODE == 253) {
			int j;
			int i = maincfg.CFG_TELNET1ST;
			j = maincfg.CFG_TELNETMAX;

			while (j) {
				j--;
				if (isonck(i, id))
					return i + 1;
				i++;
			}
		} else if (cn->MULTI_NODE == 254) {
			int j;
			int i = maincfg.CFG_LOCAL1ST;
			j = maincfg.CFG_LOCALMAX;

			while (j) {
				j--;
				if (isonck(i, id))
					return i + 1;
				i++;
			}
		} else if (cn->MULTI_NODE != 252) {
			if (isonck(cn->MULTI_NODE, id))
				return cn->MULTI_NODE + 1;
		}
		cn++;
	}
	return 0;
}

static int isonck(int num, int id)
{
	struct DayDream_NodeInfo myn;

	if (isnode(num, &myn)) {
		if (myn.ddn_userslot == id)
			return 1;
	}
	return 0;
}

void *xmalloc(size_t size)
{
	void *ptr = malloc(size);
	if (!ptr) {
		syslog(LOG_ERR, "%m");
		abort();
	}
	return ptr;
}

void *xrealloc(void *ptr, size_t size)
{
	ptr = realloc(ptr, size);
	if (!ptr) {
		syslog(LOG_ERR, "%m");
		abort();
	}
	return ptr;
}

size_t strspace(char *dest, const char *src, size_t n)
{
	const char *end;
	size_t length;
	if (src == NULL) {
		*dest = 0;
		return 1;
	}
	while (*src && isspace(*src))
		src++;
	for (end = src; *end && !isspace(*end); end++);
	length = end - src;
	if (length >= n) {
		*dest = 0;
		return length;
	}
	strncpy(dest, src, length);
	dest[length] = 0;
	return length + 1;
}
	
size_t strtoken(char *dest, const char **srcptr, size_t n)
{
	size_t length;
	const char *end;
	const char *src;
	if (srcptr == NULL) {
		*dest = 0;
		return 1;
	}
	src = *srcptr;
	if (src == NULL) {
		*dest = 0;
		return 1;
	}
	while (*src && isspace(*src))
		src++;
	for (end = src; *end && !isspace(*end); end++);
	length = end - src;
	if (length >= n) {
		*dest = 0;
		*srcptr = end;
		return length;
	}
	strncpy(dest, src, length);
	dest[length] = 0;
	*srcptr = end;
	return length + 1;
}	

const char *strspa(const char *src, char *dest, size_t n)
{
	if (strtoken(dest, &src, n) > n)
		return NULL;
	if (!*dest)
		return NULL;
	return src;
}

/* mode = 0 -> blocking, 1 -> non-blocking */
int set_blocking_mode(int fd, int mode)
{
	int fl;
	if ((fl = fcntl(fd, F_GETFL)) == -1)
		return -1;
	fl &= ~O_NONBLOCK;
	return fcntl(fd, F_SETFL, fl | (mode ? O_NONBLOCK : 0));
}

char *fgetsnolf(char *buffer, int n, FILE *fp)
{
	char *ptr;
	
	if (fgets(buffer, n, fp) == NULL)
		return NULL;
	for (ptr = buffer; *ptr && *ptr != 0x0a && *ptr != 0x0d; ptr++);
	*ptr = '\0';
	return buffer;
}

long int str2uint(const char *src, unsigned long int min, unsigned long int max)
{
	long int result;
	char *endptr;

	if (((int) max) < 0 || ((int) min) < 0)
		abort();
	result = strtol(src, &endptr, 10);
	if (!*src || *endptr)
		return -1;
	if (result == LONG_MIN || result == LONG_MAX || result < 0)
		return -1;
	if (((unsigned long int) result) < min || 
		((unsigned long int) result) > max)
		return -1;
	return result;
}

const char *filepart(const char *pathname)
{
	const char *p;

	if ((p = (const char *) strrchr(pathname, '/')))
		return p + 1;
	else
		return pathname;
}
