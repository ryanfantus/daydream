#include <dirent.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <daydream.h>
#include <ddcommon.h>

static int quote(char *, size_t) __attr_bounded__ (__buffer__, 1, 2);
static int quote_interactive(int, char *, size_t);
static int delete_lines(int, char *);

char wrapbuf[80];

/* Returns the number of lines entered. */
/* FIXME: rethink */
int lineed(char *buffer, size_t bufsize, int mode, struct DayDream_Message *msg)
{
	int i, j;
	char lbuf[300];
	char *s;
	int row;

	*wrapbuf = 0;
	row = 1;

	DDPut(sd[leheadstr]);

	if (mode) 
		row = quote(buffer, bufsize);
	
	for (;;) {
		snprintf(lbuf, sizeof lbuf, "%c", maincfg.CFG_LINEEDCHAR);

		while (row != 495) {
			ddprintf(sd[lelinestr], row);

			s = buffer + (row - 1) * 80;
			*s = 0;
			if (!(Prompt(s, 75, PROMPT_WRAP)))
				return 0;

			if (!*s || !strcasecmp(lbuf, s))
				break;
			row++;
		}

		for (;;) {
			DDPut(sd[lepromptstr]);
			lbuf[0] = 0;
			if (!(Prompt(lbuf, 3, 0)))
				return 0;

			if (!strcasecmp(lbuf, "a")) {
				DDPut(sd[lesureabortstr]);
				i = HotKey(HOT_NOYES);
				if (!i || i == 1)
					return 0;
			} else if (!strcasecmp(lbuf, "c")) {
				TypeFile("lineedcommands", TYPE_WARN | TYPE_MAKE);
			} else if (!strcasecmp(lbuf, "d")) {
				if ((row = delete_lines(row, buffer)) == -1)
					return 0;
			} else if (!strcasecmp(lbuf, "e")) {
				DDPut(sd[leedlinstr]);
				lbuf[0] = 0;
				if (!(Prompt(lbuf, 3, 0)))
					return 0;
				i = atoi(lbuf);
				if (!(i < 1 || i > (row - 1))) {
					ddprintf(sd[leedpstr], i);
					s = buffer + (i - 1) * 80;
					if (!(Prompt(s, 75, 0)))
						return 0;
				}
			} else if (!strcasecmp(lbuf, "r")) {
				break;
			} else if (!strcasecmp(lbuf, "f")) {
				if (msg && *msg->MSG_ATTACH) {
					fileattach();
				} else {
					DDPut(sd[noattachstr]);
				}
			} else if ((!strcasecmp(lbuf, "i")) && row < 495) {
				DDPut(sd[insertstr]);
				lbuf[0] = 0;
				if (!(Prompt(lbuf, 3, 0)))
					return 0;
				DDPut("\nInsert does not work yet, sorry...\n\n");
			} else if (!strcasecmp(lbuf, "l")) {
				int lcount = user.user_screenlength;
				i = row - 1;
				j = 1;
				while (i) {
					s = buffer + (j - 1) * 80;
					ddprintf(sd[lellinestr], j, s);
					lcount--;

					if (lcount == 0) {
						int hot;

						DDPut(sd[morepromptstr]);
						hot = HotKey(0);
						DDPut("\r                                                         \r");
						if (hot == 'N' || hot == 'n')
							break;
						if (hot == 'C' || hot == 'c') {
							lcount = -1;
						} else {
							lcount = user.user_screenlength;
						}
					}
					j++;
					i--;
				}

			} else if (!strcasecmp(lbuf, "s")) {
				return row - 1;
		} else if (!strcasecmp(lbuf, "q")) {
			if (!mode) {
				DDPut(sd[lereperrorstr]);
			} else {
				// Use interactive quote flow with askqlines()
				int new_row = quote_interactive(row, buffer, bufsize);
				if (new_row > 0) {
					row = new_row;
				}
			}
		}
		}
	}
}

static int delete_lines(int nrows, char *rows)
{
	char input[4];
	int first, last;

	DDPut(sd[ledelstr]);
	input[0] = 0;
	if (!(Prompt(input, 3, PROMPT_NOCRLF)))
		return -1;
	if ((first = str2uint(input, 1, nrows)) == -1)
		return nrows;	

	DDPut(sd[ledeltostr]);
	input[0] = 0;
	if (!(Prompt(input, 3, 0)))
		return -1;
	if ((last = str2uint(input, 1, nrows)) == -1)
		return nrows;

	if (last < first) {
		int tmp;
		tmp = last;
		last = first;
		first = tmp;
	}

	memmove(rows + (first - 1) * 80, rows + last * 80, (nrows - last) * 80);
	return nrows - (last - first + 1);
}

int fileattach(void)
{
	char olddir[1024];
	char fabuf[1024];
	FILE *falist;
	int cnt = 0;
	struct dirent *dent;
	DIR *dh;

	if (cleantemp() == -1) {
		DDPut(sd[tempcleanerrstr]);
		return 0;
	}
	recfiles(currnode->MULTI_TEMPORARY, 0);

	snprintf(fabuf, sizeof fabuf, "%s/attachs.%d", DDTMP, node);
	/* FIXME: check falist == NULL */
	falist = fopen(fabuf, "w");

	if ((dh = opendir(currnode->MULTI_TEMPORARY))) {
		getcwd(olddir, 1024);
		chdir(currnode->MULTI_TEMPORARY);
		while ((dent = readdir(dh))) {
			if (dent->d_name[0] == '.' && (dent->d_name[1] == '\0' || (dent->d_name[1] == '.' && dent->d_name[2] == '\0')))
				continue;
			if (!strcmp(".packtmp", dent->d_name))
				continue;

			deldir(".packtmp");
			ddprintf(sd[afqstr], dent->d_name);
			if (HotKey(HOT_YESNO) == 1) {
				fprintf(falist, "%s\n", dent->d_name);
				cnt++;
			} else {
				unlink(dent->d_name);
			}
			if (!checkcarrier())
				break;
		}
		chdir(olddir);
		closedir(dh);
	}
	fclose(falist);
	if (!cnt) {
		snprintf(fabuf, sizeof fabuf, "%s/attachs.%d", DDTMP, node);
		unlink(fabuf);
	}
	return 1;
}

static int quote(char *buffer, size_t bufsize)
{
	char filename[PATH_MAX];
	char input[80];
	int rows;
	FILE *fp;

	if (bufsize % 80)
		return -1;
	if (ssnprintf(filename, "%s/daydream%d.msg", DDTMP, node))
		return -1;
	if ((fp = fopen(filename, "r")) == NULL)
		return -1;
	
	rows = 0;
	while (fgetsnolf(input, 78, fp)) {
		if (!bufsize ||
			strlcpy(buffer, input, bufsize) >= bufsize) {
			fclose(fp);
			return -1;
		}
		buffer += 80;
		bufsize -= 80;
		rows++;
	}
	fclose(fp);

	return rows;
}

/* Interactive quote function that uses askqlines() for better UX */
static int quote_interactive(int current_row, char *buffer, size_t bufsize)
{
	char filename[PATH_MAX];
	char input[80];
	int rows;
	FILE *fp;
	char *insertion_point;
	size_t remaining_bufsize;
	
	// Calculate where to insert the quoted text (after current lines)
	insertion_point = buffer + (current_row - 1) * 80;
	remaining_bufsize = bufsize - ((current_row - 1) * 80);
	
	// Call askqlines() to let user interactively select lines from daydream%d.mtm
	// This will write the selected lines to daydream%d.msg
	if (!askqlines()) {
		// User aborted or error occurred
		return current_row;
	}
	
	// Now read the selected lines from daydream%d.msg and insert into buffer
	if (ssnprintf(filename, "%s/daydream%d.msg", DDTMP, node))
		return current_row;
	
	if ((fp = fopen(filename, "r")) == NULL)
		return current_row;
	
	rows = current_row - 1;  // Start from current position
	while (fgetsnolf(input, 78, fp)) {
		if (remaining_bufsize < 80) {
			// Buffer full
			fclose(fp);
			break;
		}
		if (strlcpy(insertion_point, input, remaining_bufsize) >= remaining_bufsize) {
			fclose(fp);
			break;
		}
		insertion_point += 80;
		remaining_bufsize -= 80;
		rows++;
	}
	fclose(fp);
	
	return rows + 1;  // Return next available row
}

