#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>

#include <daydream.h>
#include <ddcommon.h>

int flagerror = 0;

static int flagsinglef(char *, int);
static void secstosentence(int, char *, size_t)
	__attr_bounded__ (__string__, 2, 3);

int taged(const char *params)
{
	char buf[512];
	char parbuf[512];
	const char *srcstrh;

	setprotocol();
	srcstrh = params;

	if (!(strspace(parbuf, srcstrh, sizeof parbuf) <= sizeof parbuf &&
	      *parbuf)) {
		if (filestagged)
			listtags();
	}

	for (;;) {
		if (!(srcstrh = strspa(srcstrh, parbuf, 512))) {
			DDPut(sd[tepromptstr]);
			buf[0] = 0;
			parbuf[0] = 0;
			if (!(Prompt(buf, 100, 0)))
				return 0;
			srcstrh = buf;
			srcstrh = strspa(srcstrh, parbuf, 512);
		}
		if (!strcasecmp(parbuf, "q") || parbuf[0] == 0) {
			recountfiles();
			return 0;
		} else if (!strcasecmp(parbuf, "r")) {
			if (!(srcstrh = strspa(srcstrh, parbuf, 512))) {
				DDPut(sd[teremprstr]);
				buf[0] = 0;
				if (!(Prompt(buf, 100, 0)))
					return 0;
				srcstrh = buf;
				srcstrh = strspa(srcstrh, parbuf, 512);
			}
			do {
				struct FFlag *myf;

				if (parbuf[0] == 0)
					continue;

				myf = (struct FFlag *) flaggedfiles->lh_Head;
				while (myf->fhead.ln_Succ) {
					if (wildcmp(myf->f_filename, parbuf)) {
						struct FFlag *oldf;

						Remove((struct Node *) myf);
						ddprintf(sd[teremovedstr], myf->f_filename);
						oldf = myf;
						myf = (struct FFlag *) myf->fhead.ln_Succ;
						free(oldf);
					} else {
						myf = (struct FFlag *) myf->fhead.ln_Succ;
					}
				}
			} while ((srcstrh = strspa(srcstrh, parbuf, 512)));
			recountfiles();
		} else if (!strcasecmp(parbuf, "l")) {
			listtags();
		} else {
			flagfile(parbuf, 1);
		}
	}
}

int flagres(int res, const char *file, int size)
{
	char buf[512];
	switch (res) {
	case 0:
	case 4:
		snprintf(buf, sizeof buf, sd[tetaggedstr], file, size);
		if (res == 4)
			strlcat(buf, sd[tefreestr], sizeof buf);
		else
			strlcat(buf, "\n", sizeof buf);
		DDPut(buf);
		break;
	case 1:
		ddprintf(sd[teulratstr], file);
		break;
	case 2:
		ddprintf(sd[teulbratstr], file);
		break;
	case 3:
		ddprintf(sd[tealflagstr], file);
		break;
	}
	return res;
}

int isfreedl(char *file)
{
	FILE *freedldat;
	char puskuri[300];
	if (conference()->conf.CONF_ATTRIBUTES & (1L << 0))
		return 2;

	snprintf(puskuri, sizeof puskuri, "%s/data/freedownloads.dat", 
		conference()->conf.CONF_PATH);

	if ((freedldat = fopen(puskuri, "r"))) {
		while (fgetsnolf(puskuri, 250, freedldat)) {
			if (wildcmp(file, puskuri)) {
				fclose(freedldat);
				return 1;
			}
		}
		fclose(freedldat);
	}
	snprintf(puskuri, sizeof puskuri, "%s/data/freedownloads.dat", 
		origdir);

	if ((freedldat = fopen(puskuri, "r"))) {
		while (fgetsnolf(puskuri, 250, freedldat)) {
			if (wildcmp(file, puskuri)) {
				fclose(freedldat);
				return 1;
			}
		}
		fclose(freedldat);
	}
	return 0;
}

int flagfile(char *file, int res)
{
	int r;
	int files = 0;
	int wildmode = 0;
	DIR *dh;
	struct dirent *dent;
	char buf1[600];
	FILE *plist;

	r = flagsingle(file, res);
	if (r == 0 || r == 4)
		return 1;
	if (r != -1)
		return 0;

	if (!(conference()->conf.CONF_ATTRIBUTES & (1L << 6)) && iswilds(file))
		wildmode = 1;

	snprintf(buf1, sizeof buf1, "%s/data/paths.dat", 
		conference()->conf.CONF_PATH);
	if ((plist = fopen(buf1, "r"))) {
		while (fgetsnolf(buf1, 512, plist)) {
			if ((dh = opendir(buf1))) {
				while ((dent = readdir(dh))) {
					if (!strcmp(dent->d_name, ".") || (!strcmp(dent->d_name, "..")))
						continue;
					if (wildmode) {
						if (wildcmp(dent->d_name, file)) {
							r = flagsingle(dent->d_name, res);
							if (r == 0 || r == 4)
								files++;
						}
					} else {
						if (!strcasecmp(dent->d_name, file)) {
							r = flagsingle(dent->d_name, res);
							if (r == 0 || r == 4)
								files++;
							break;
						}
					}
				}
				closedir(dh);
			}
		}
		fclose(plist);
	}
	return files;
}

int flagsingle(char *file, int res)
{
	flagerror = flagsinglef(file, res);
	return flagerror;
}

static int flagsinglef(char *file, int res)
{
	char buf1[512];
	char buf2[512];
	char lonam[256];
	FILE *plist;
	struct stat st;
	char *s;
	int lomod;

	s = file;
	while (*s) {
		if (*s == '/')
			return -1;
		s++;
	}
	if (!strcmp(".", file) || !strcmp("..", file))
		return -1;

	strlcpy(lonam, file, sizeof lonam);
	strlwr(lonam);

	lomod = strcmp(lonam, file);

	snprintf(buf1, sizeof buf1, "%s/data/paths.dat", 
		conference()->conf.CONF_PATH);
	if ((plist = fopen(buf1, "r"))) {
		while (fgetsnolf(buf1, 512, plist)) {
			snprintf(buf2, sizeof buf2, "%s%s", buf1, file);
			if (stat(buf2, &st) == 0) {
				int flags = 0;

				fclose(plist);

				if (isfreedl(file))
					flags |= (FLAG_FREE);
				if (res) {
					return flagres(addtag(buf1, file, conference()->conf.CONF_NUMBER, st.st_size, flags), file, st.st_size);
				} else
					return addtag(buf1, file, conference()->conf.CONF_NUMBER, st.st_size, flags);
			} else if (lomod) {
				snprintf(buf2, sizeof buf2, "%s%s", 
					buf1, lonam);
				if (stat(buf2, &st) == 0) {
					int flags = 0;

					fclose(plist);

					if (isfreedl(file))
						flags |= (FLAG_FREE);
					if (res) {
						return flagres(addtag(buf1, file, conference()->conf.CONF_NUMBER, st.st_size, flags), file, st.st_size);
					} else
						return addtag(buf1, file, conference()->conf.CONF_NUMBER, st.st_size, flags);
				}
			}
		}
		fclose(plist);
	}
	return -1;
}

static void secstosentence(int secs, char *buf, size_t buflen)
{

	char buffa[30];
	int hr = 0;
	int min = 0;
	int sec = 0;

	buf[0] = 0;

	while (secs > 3599) {
		hr++;
		secs -= 3600;
	}
	while (secs > 59) {
		min++;
		secs -= 60;
	}
	sec = secs;

	if (hr > 1) {
		snprintf(buf, buflen, "%d hours", hr);
	} else if (hr == 1) {
		strlcat(buf, "1 hour", buflen);
	}
	if (min) {
		if (hr)
			strlcat(buf, ", ", buflen);
		if (min > 1) {
			snprintf(buffa, sizeof buffa, "%d minutes", min);
			strlcat(buf, buffa, buflen);
		} else
			strlcat(buf, "1 minute", buflen);
	}
	if (sec || (!min && !hr)) {
		if (hr || min)
			strlcat(buf, ", ", buflen);
		if (sec != 1) {
			snprintf(buffa, sizeof buffa, "%d seconds", sec);
			strlcat(buf, buffa, buflen);
		} else
			strlcat(buf, "1 second", buflen);
	}
}

int estimsecs(int bytes)
{
	if (!bpsrate)
		return 0;
	return bytes / (((bpsrate / 9) * protocol->PROTOCOL_EFFICIENCY) / 100);
}

void recountfiles(void)
{
	struct FFlag *myf;

	filestagged = bytestagged = ffilestagged = fbytestagged = 0;
	myf = (struct FFlag *) flaggedfiles->lh_Head;

	while (myf->fhead.ln_Succ) {
		filestagged++;
		bytestagged += myf->f_size;
		if (myf->f_flags & FLAG_FREE) {
			ffilestagged++;
			fbytestagged += myf->f_size;
		}
		myf = (struct FFlag *) myf->fhead.ln_Succ;
	}
}

/* Add tag: returns: 0 == success, 1 == file ratio, 2 == byte ratio
   3 == already tagged, 4 == free leech */

int addtag(const char *path, const char *file, int conf, int size, int flags)
{
	struct FFlag *myf;

	if (!(flags & FLAG_FREE)) {
		if (user.user_fileratio) {
			if (((user.user_fileratio * user.user_ulfiles) + user.user_freedlfiles) < (user.user_dlfiles + 1 + filestagged - ffilestagged))
				return 1;
		}
		if (user.user_byteratio) {
			if (((user.user_byteratio * user.user_ulbytes) + user.user_freedlbytes) < (user.user_dlbytes + size + bytestagged - fbytestagged))
				return 2;
		}
	}
	if (isfiletagged(file))
		return 3;

	myf = (struct FFlag *) xmalloc(sizeof(struct FFlag));

	strlcpy(myf->f_path, path, sizeof myf->f_path);
	strlcpy(myf->f_filename, file, sizeof myf->f_filename);
	filestagged++;
	bytestagged += size;
	if (flags & FLAG_FREE) {
		ffilestagged++;
		fbytestagged += size;
	}
	myf->f_size = size;
	myf->f_conf = conf;
	myf->f_flags = flags;
	AddTail(flaggedfiles, (struct Node *) myf);
	if (flags & FLAG_FREE)
		return 4;
	else
		return 0;
}

int isfiletagged(const char *fn)
{
	struct FFlag *myf;

	myf = (struct FFlag *) flaggedfiles->lh_Head;
	if (myf->fhead.ln_Succ) {
		while (myf->fhead.ln_Succ) {
			if (!strcasecmp(myf->f_filename, fn))
				return 1;
			myf = (struct FFlag *) myf->fhead.ln_Succ;
		}
	}
	return 0;
}

int listtags(void)
{
	char buf[512];
	char tb[50];
	struct FFlag *myf;
	int bcnt = 0;

	myf = (struct FFlag *) flaggedfiles->lh_Head;
	if (myf->fhead.ln_Succ) {
		ddprintf(sd[telisthstr], bpsrate, protocol->PROTOCOL_NAME);
		while (myf->fhead.ln_Succ) {
			secstosentence(estimsecs(myf->f_size), tb, sizeof tb);
			bcnt += myf->f_size;
			snprintf(buf, sizeof buf, 
				"[33m%-16.16s [35m%-13d [36m%s ", 
				myf->f_filename, myf->f_size, tb);
			if (myf->f_flags & FLAG_FREE)
				strlcat(buf, sd[tefreestr], sizeof buf);
			else
				strlcat(buf, "\n", sizeof buf);
			DDPut(buf);
			myf = (struct FFlag *) myf->fhead.ln_Succ;
		}
		DDPut(sd[tetailstr]);
		secstosentence(estimsecs(bcnt), tb, sizeof tb);
		ddprintf(sd[tetotstr], bcnt, tb);
	} else {
		return 0;
	}
	return 1;
}

int unflagfile(const char *file)
{
	struct FFlag *oldf;
	struct FFlag *myf;
	int cnt = 0;
	myf = (struct FFlag *) flaggedfiles->lh_Head;

	while (myf->fhead.ln_Succ) {
		if (wildcmp(myf->f_filename, file)) {
			Remove((struct Node *) myf);
			oldf = myf;
			myf = (struct FFlag *) myf->fhead.ln_Succ;
			free(oldf);
			cnt++;
		} else {
			myf = (struct FFlag *) myf->fhead.ln_Succ;
		}
	}
	recountfiles();
	return cnt;
}

int findfilestolist(char *file, char *list_fname)
{
	int files = 0;
	DIR *dh;
	struct dirent *dent;
	char buf1[1024], *fname;
	FILE *plist;
	FILE *flist;

	if (!(flist = fopen(list_fname, "w")))
		return 0;

	if (conference()->conf.CONF_ATTRIBUTES & (1L << 6) ||
	    iswilds(file)) {
		if (!(fname = find_file(file, NULL))) {
			fclose(flist);
			return 0;
		}
		fprintf(flist, "%s\n", fname);
		fclose(flist);
		return 1;
	}

	snprintf(buf1, sizeof buf1, "%s/data/paths.dat", 
		conference()->conf.CONF_PATH);
	if (!(plist = fopen(buf1, "r"))) {
		fclose(flist);
		return 0;
	}

	while (fgetsnolf(buf1, 512, plist)) {
		if (!(dh = opendir(buf1)))
			continue;
		
		while ((dent = readdir(dh))) {
			if (!strcmp(dent->d_name, ".") ||
			    (!strcmp(dent->d_name, "..")))
				continue;
			if (wildcmp(dent->d_name, file)) {
				if (!(fname = find_file(dent->d_name, NULL)))
					continue;
				fprintf(flist, "%s\n", fname);
				files++;
			}
		}
		closedir(dh);

	}
	
	fclose(plist);
	fclose(flist);
	return files;
}

int dumpfilestofile(const char *fil)
{
	struct FFlag *myf;
	int cnt = 0;
	int fd;

	myf = (struct FFlag *) flaggedfiles->lh_Head;
	if (myf->fhead.ln_Succ) {
		fd = open(fil, O_WRONLY | O_TRUNC | O_CREAT, 0666);
		if (fd == -1)
			return 0;
		fsetperm(fd, 0666);
		while (myf->fhead.ln_Succ) {
			safe_write(fd, myf, sizeof(struct FFlag));
			myf = (struct FFlag *) myf->fhead.ln_Succ;
			cnt++;
		}
		close(fd);
	}
	return cnt;
}
