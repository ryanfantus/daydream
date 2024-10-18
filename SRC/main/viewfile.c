#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>

#include <daydream.h>
#include <ddcommon.h>

int viewfile(const char *params) 
{
	char parbuf[1024];		
	char fname[15] = "";	
	char vbuf[1024];
	int fd;
      	const char *srcstrh;
	
	srcstrh = params;
	
	if (!params || !*params) {
		DDPut(sd[viewstr]);
		*vbuf = 0;
		
		if (!(Prompt(vbuf, 80, PROMPT_FILE)))
			return 0;
				    
		if (!*vbuf)
			return 0;
				    
		srcstrh = vbuf;
	}
	
	for (;;) {
		char *s;

		if (strtoken(parbuf, &srcstrh, sizeof parbuf) > sizeof parbuf)
			continue;
		if (!*parbuf)
			break;
		
		getarchiver(parbuf);
		
		if (!arc) {
			ddprintf(sd[viewunkastr], parbuf);
			continue;
		}
		
		if (!*arc->ARC_VIEW) {
			ddprintf(sd[viewnosupstr], arc->ARC_NAME);
			continue;
		}
		
		if (arc->ARC_VIEWLEVEL > user.user_securitylevel) {
			ddprintf(sd[viewlamerstr], arc->ARC_NAME);
			continue;
		}
		
		s = find_file(parbuf, NULL);
				
		if (!s) {
			ddprintf(sd[viewnostr], parbuf);
			continue;
		}
		
		genstdiocmdline(parbuf, arc->ARC_VIEW, s, 0);

		strlcpy(fname, "/tmp/dd.XXXXXX", sizeof fname);
		if ((fd = mkstemp(fname)) == -1) {
			fprintf(stderr, "%s: %s\n", fname, strerror(errno));
			return 0;
		}
		if (runstdio(parbuf, fd, 3)) {
			DDPut("\033[0m");
			TypeFile(fname, TYPE_NOCODES);
		}
		close(fd);
		unlink(fname);
	}	
	
	return 1;		    
}

static int search_dir(const char *dirname, const char *pattern,
	char *filename, size_t maxlen)
{
	DIR *dir;
	struct dirent *dent;
	int ok = 0;

	if ((dir = opendir(dirname)) == NULL)
		return -1;
	while ((dent = readdir(dir)) != NULL) 
		if (strcasecmp(dent->d_name, pattern) == 0) {
			ok = 1;
			break;
		}
	if (ok && (strlcpy(filename, dirname, maxlen) >= maxlen ||
		strlcat(filename, "/", maxlen) >= maxlen ||
		strlcat(filename, dent->d_name, maxlen) >= maxlen))
		ok = 0;
	closedir(dir);
	return ok ? 0 : -1;
}

char *find_file(char *filename, conference_t *cf)
{
	static char pname[PATH_MAX];
	char dirname[PATH_MAX];
	FILE *fp;
	
	if (!cf)
		cf = conference();
	
	if (strchr(filename, '/'))
		return NULL;

	if (strlcpy(pname, cf->conf.CONF_PATH, 
			sizeof pname) >= sizeof pname ||
		strlcat(pname, "/data/paths.dat", 
			sizeof pname) >= sizeof pname ||
		(fp = fopen(pname, "r")) == NULL)
		return NULL;
	
	while (fgetsnolf(dirname, PATH_MAX, fp)) {
		if (!pname[0])
			continue;

		if (search_dir(dirname, filename, pname, sizeof pname) == 0) {
			fclose(fp);
			return pname;
		}
	}
	
	fclose(fp);
	return NULL;
}
