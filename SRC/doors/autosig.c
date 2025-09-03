/*******************************************************************

DayDream BBS autosig editor v1.0
by esc of demonic productions 2011

This is a down and dirty autosig editor.  It either creates or
overwrites the file autosig.dat in the user's personal directory.

It will parse pipe codes, and includes some defaults.  Enjoy!

********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "ddlib.h"

struct dif *d;

void show_sig() {
  char buf1[1024];
  char s[256], format[256];
  FILE *data_file;
  int i;

  *s = '\0';

  dd_typefile(d, "autosigtop", TYPE_MAKE|TYPE_WARN);

  sprintf(buf1, "%s/users/%i/autosig.dat", getenv("DAYDREAM"),
	  dd_getintval(d, USER_ACCOUNT_ID));

  if ((data_file = fopen(buf1, "r"))) {
	while(fgets(s, 256, data_file)) {
		*format = '\0';
		sprintf(format, "\e[0m%s\e[0m", s);
		dd_parsepipes(format);
		dd_sendstring(d, format);
	}
  fclose(data_file);
  } else {
	dd_sendstring(d, "No autosig created yet!\n");
  }
}

void create_sig() {
  char buf1[80], line1[256], line2[256], line3[256], format[256];
  char *s;
  FILE *data_file;
  int what=0;

  *line1 = '\0';
  *line2 = '\0';
  *line3 = '\0';

  dd_sendstring(d, "\n\e[33mWrite your sig on the three lines below, pipe color codes as follows:\e[0m\n");
  dd_sendstring_noparse(d, "\e[33mForeground: \e[0;30m|00 \e[0;34m|01 \e[0;32m|02 \e[0;36m|03 \e[0;31m|04 \e[0;35m|05 \e[0;33m|06 \e[0;37m|07 \e[1;30m|08 \e[1;34m|09 \e[1;32m|10 \e[1;36m|11 \e[1;31m|12 \e[1;35m|13 \e[1;33m|14 \e[1;37m|15\e[0m\n");
  dd_sendstring_noparse(d, "\e[33mBackground: \e[0m\e[40m|16\e[0m \e[44m|17\e[0m \e[42m|18\e[0m \e[46m|19\e[0m \e[41m|20\e[0m \e[45m|21\e[0m \e[43m|22\e[0m \e[47m|23\e[0m\n");
  dd_sendstring(d, "\e[36m> ");
  dd_prompt(d, line1, 76, PROMPT_NOCRLF);
  dd_sendstring(d, "\n\e[36m> ");
  dd_prompt(d, line2, 76, PROMPT_NOCRLF);
  dd_sendstring(d, "\n\e[36m> ");
  dd_prompt(d, line3, 76, PROMPT_NOCRLF);

  dd_sendstring(d, "\e[0m\n");

  *format = '\0';
  dd_sendstring(d, "\e[33mCurrent sig is as follows: \n");
  sprintf(format, "\e[0m%s\e[0m\n", line1);
  dd_parsepipes(format);
  dd_sendstring(d, format);
  sprintf(format, "\e[0m%s\e[0m\n", line2);
  dd_parsepipes(format);
  dd_sendstring(d, format);
  sprintf(format, "\e[0m%s\e[0m\n", line3);
  dd_parsepipes(format);
  dd_sendstring(d, format);

  dd_sendstring(d, "\n\e[33mDo you want to save your new autosig? \e[0m(\e[32mYes\e[0m/\e[32mno\e[0m) \e[36m:\e[0m ");
  what = dd_hotkey(d, HOT_YESNO);

  switch (what) {
    case 1:
      *buf1 = '\0';
      sprintf(buf1, "%s/users/%i/autosig.dat", getenv("DAYDREAM"),
              dd_getintval(d, USER_ACCOUNT_ID));

      if (!(data_file = fopen(buf1, "w+"))) {
              dd_sendstring(d, "\nUnable to create autosig.dat, inform op!\n");
              dd_pause(d);
              return;
      }
      if (strlen(line1) >= 1)
        fprintf(data_file, "%s\n", line1);
      if (strlen(line2) >= 1)
        fprintf(data_file, "%s\n", line2);
      if (strlen(line3) >= 1)
        fprintf(data_file, "%s\n", line3);
      fflush(data_file);
      fclose(data_file);
    default:
      break;
  }
}

void main(int argc, char *argv[]) {
  char buf[256];
  int what=0;
  int done=0;

  if (argc==1) {
	printf("This is a DayDream door, foo!\n");
	exit(1);
  }

  d = dd_initdoor(argv[1]);

  if (d == 0) {
	printf("Couldn't find socket!\n");
	exit(1);
  }

  dd_changestatus(d, "Changing AutoSig.");
  while(!done) {
    dd_clrscr(d);
    show_sig();

    *buf='\0';
    sprintf(buf, "\n\e[33mDo you wish to create/modify your sig? \e[0m(\e[32myes\e[0m/\e[32mNo\e[0m) \e[36m:\e[0m ");
    dd_sendstring(d, buf);
    what = dd_hotkey(d, HOT_NOYES);

    switch (what) {
      case 1:
	create_sig();
	break;
      case 2:
	done=1;
	break;
      default:
	break;
    }
  }

  dd_close(d);

}

