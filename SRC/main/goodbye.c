#include <ctype.h>

#include <daydream.h>

int logoff(const char *params)
{
	const char *srcstrh;
	char logoffb[400];
	
	srcstrh = params;
	recountfiles();
	for (;;) {
		if (strtoken(logoffb, &srcstrh, 
			     sizeof logoffb) > sizeof logoffb)
			continue;
		if (!*logoffb) {
			TypeFile("beforegoodbye", TYPE_MAKE);
			if (!filestagged) {
				DDPut(sd[gbprstr]);
				switch (HotKey(HOT_YESNO)) {
				case 1:
					logoffb[0] = 'Y';
					break;
				case 2:
					logoffb[0] = 'N';
					DDPut("\n");
					break;
				case 0:
					return 0;
				}
			} else {
				DDPut(sd[gbpr2str]);
				switch (toupper(HotKey(0))) {
				case 'C':
					DDPut(sd[clearfstr]);
					logoffb[0] = 'C';
					break;
				case 'Y':
				case 10:
				case 13:
					logoffb[0] = 'Y';
					DDPut(sd[yesstr]);
					break;
				case 'N':
					logoffb[0] = 'N';
					DDPut(sd[nostr]);
					DDPut("\n");
					break;
				case 0:
					return 0;
				}
			}
		}
		if ((toupper(logoffb[0]) == 'Y') || (toupper(logoffb[0]) == 'C')) {
			if (toupper(logoffb[0]) == 'C') {
				clearlist(flaggedfiles);
			}
			snprintf(logoffb, sizeof logoffb,
				"Disconnection completed at %s", currt());
			writelog(logoffb);
			TypeFile("goodbye", TYPE_WARN | TYPE_MAKE);
			rundoorbatch("data/logoffdoors.dat", 0);
			return 0;
		} else if (logoffb[0] == 'n' || logoffb[0] == 'N') {
			return 1;
		}
	}
}
