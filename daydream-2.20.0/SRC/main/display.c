#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include <daydream.h>
#include <hdr.h>

static int changemode(int);
static int largestmode(void);
static int loadstrings(int);
static int lscpy(char *, char *);

int getdisplaymode(const char *strh, int mode)
{
	char buf[70];
	int newmode;
	int lmodeh;
	char helpfile[80];

	if (!(maincfg.CFG_FLAGS & (1L << 16)))
		return changemode(1);

	if (strh && strtoken(buf, &strh, sizeof buf) <= sizeof buf) {
		if ((newmode = atoi(buf))) {
			newmode = changemode(newmode);
			if (newmode)
				return 1;
		}
	}
	lmodeh = largestmode();
	if (lmodeh == 1) {
		return changemode(1);
	}
	if (!mode) {
		snprintf(helpfile, sizeof helpfile, 
			"%s/display/displaymodelist.txt", origdir);
		TypeFile(helpfile, TYPE_NOSTRIP);
	}
	for (;;) {
		if (mode) {
			snprintf(buf, sizeof buf,
				"Display Mode 1-%d or H)elp: ", lmodeh);
		} else {
			snprintf(buf, sizeof buf,
				"Display Mode [1]-%d or H)elp: ", lmodeh);
		}
		DDPut(buf);
		*buf = 0;
		if (!(Prompt(buf, 3, 0)))
			return -1;
		removespaces(buf);
		if (*buf == 'h' || *buf == 'H' || *buf == '?') {
			snprintf(helpfile, sizeof helpfile,
				"%s/display/displaymodehelp.txt", origdir);
			TypeFile(helpfile, TYPE_NOSTRIP);
		} else if (!*buf) {
			if (mode)
				return 1;
			return changemode(1);
		} else if ((newmode = atoi(buf))) {
			if (changemode(newmode))
				return 1;
		}
	}
}

static int largestmode(void)
{
	int highestd = 0;

	struct DayDream_DisplayMode *d;
	d = displays;
	while (d->DISPLAY_ID) {
		highestd = d->DISPLAY_ID;
		d++;
	}
	return highestd;
}

static int changemode(int newmode)
{
	struct DayDream_DisplayMode *d;
	int tabfd;
	char buffer[80];

	d = displays;

	while (d->DISPLAY_ID) {
		if (newmode == d->DISPLAY_ID) {
			if (d->DISPLAY_ATTRIBUTES & (1L << 0))
				ansi = 1;
			else
				ansi = 0;
			if (d->DISPLAY_INCOMING_TABLEID) {
				snprintf(buffer, sizeof buffer, 
					"%s/data/conversiontable%2.2d.dat",
					 origdir, 	
					d->DISPLAY_INCOMING_TABLEID);
				tabfd = open(buffer, O_RDONLY);
				if (tabfd < 0) {
					DDPut("Can't load incoming table!\n");
					return 0;
				}
				read(tabfd, &inconvtab, 256);
				close(tabfd);
			}
			if (d->DISPLAY_OUTGOING_TABLEID) {
				snprintf(buffer, sizeof buffer,
					"%s/data/conversiontable%2.2d.dat",
					 origdir, 
					d->DISPLAY_OUTGOING_TABLEID);
				tabfd = open(buffer, O_RDONLY);
				if (tabfd < 0) {
					DDPut("Can't load outgoing table!\n");
					return 0;
				}
				read(tabfd, &outconvtab, 256);
				close(tabfd);
			}
			if (!loadstrings(d->DISPLAY_STRINGS))
				return 0;
			display = d;
			return 1;
		}
		d++;
	}
	return 0;
}

void initstringspace(void)
{
	int i;
	for (i = 0; i < MAXSTRS; i++)
		sd[i] = NULL;
}

static void freestringspace(void)
{
	int i;
	for (i = 0; i < MAXSTRS; i++)
		if (sd[i] != NULL) {
			free(sd[i]);
			sd[i] = NULL;
		}
}

static int loadstrings(int mode)
{
	char buffer[4096];
	struct stat st;
	int rowcntr = 0;
	int strsread = 0;
	int i;
	char *exists;
	FILE *fp;

	/* discard previous strings */
	freestringspace();

	snprintf(buffer, sizeof buffer, "%s/display/strings.%3.3d", 
		origdir, mode);
	if (stat(buffer, &st)) {
		ddprintf("Missing %s, cannot use this mode.\n", buffer);
		return 0;
	}
	/* exist-flag per string */
	exists = (char *) xmalloc(MAXSTRS);
	memset(exists, 0, MAXSTRS);

	if ((fp = fopen(buffer, "rt")) == NULL) {
		ddprintf("Unable to open %s, cannot use this mode.\n", buffer);
		free(exists);
		return 0;
	}
	for (; (fgets(buffer, 4096, fp)) != NULL; rowcntr++) {
		char *ptr, *str, *tmp;

		for (tmp = buffer, str = ptr = NULL; *tmp; tmp++) {
			if (*tmp == 0x0a || *tmp == 0x0d) {
				*tmp = 0;
				break;
			}
			if (ptr == NULL) {
				/* comment character */
				if (*tmp == '#')
					break;
				if (!(*tmp == ' ' || *tmp == '\t'))
					ptr = tmp;
			} else if (*tmp == ':' && str == NULL) {
				*tmp++ = 0;
				str = tmp;
			}
		}

		/* empty row */
		if (ptr == NULL)
			continue;

		/* do search */
		for (i = 0; i < MAXSTRS; i++)
			if (!strcasecmp(display_string_table[i].str, ptr))
				break;

		if (i == MAXSTRS) {
			ddprintf("Invalid keyword '%s' in %s/display/"
			       "strings.%3.3d,\ncannot use this mode.\n",
				 ptr, origdir, mode);
			fclose(fp);
			free(exists);
			return 0;
		}
		if (exists[i]++) {
			ddprintf("Duplicate keyword '%s' in %s/display/"
			     "strings.%3.3d at\nrow %d, cannot use this "
				 "mode.\n", ptr, origdir, mode, rowcntr);
			fclose(fp);
			free(exists);
			return 0;
		}
		if (str == NULL || *str == 0) {
			ddprintf("No value for keyword '%s' in %s/display/"
			       "strings.%3.3d,\ncannot use this mode.\n",
				 ptr, origdir, mode);
			fclose(fp);
			free(exists);
			return 0;
		}
		lscpy(str, buffer);

		if ((sd[display_string_table[i].num] = 
			strdup(buffer)) == NULL) {
			log_error(__FILE__, __LINE__, "out of memory");
			exit(1);
		}
		strsread++;
	}
	fclose(fp);

	if (strsread < MAXSTRS) {
		ddprintf("Missing keyword(s) in %s/display/strings.%3.3d:\n\n",
			 origdir, mode);
		for (i = 0; i < MAXSTRS; i++)
			if (!exists[i])
				ddprintf(" %s\n", display_string_table[i].str);
		ddprintf("\nCannot use this mode.\n");
		free(exists);
		return 0;
	}
	
	free(exists);
	return 1;
}

static int lscpy(char *s, char *t)
{
	char *u = t;
	while (*s) {
		if (*s == '\\') {
			s++;
			if (*s == '\\')
				*t++ = '\\';
			else if (*s == 'n')
				*t++ = 10;
			else if (*s == 'r')
				*t++ = '\r';
			else if (*s == 'e')
				*t++ = 27;
			else if (*s == 'a')
				*t++ = 1;
			else if (*s == 's')
				break;
			s++;
		} else {
			*t++ = *s++;
		}
	}
	*t = 0;
	return strlen(u) + 1;
}
