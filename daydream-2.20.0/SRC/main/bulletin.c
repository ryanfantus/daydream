#include <fcntl.h>
#include <stdlib.h>

#include <daydream.h>

int bulletins(char *params)
{
	char bdir[200];
	char menunam[240];
	char bullb[500];
	char tbuf[400];
	char nbu[240];
	int first = 0;

	int menfd;
	const char *srcstrh;

	changenodestatus("Viewing bulletins");

	snprintf(bdir, sizeof bdir, "%s/bulletins/",
			conference()->conf.CONF_PATH);
	snprintf(menunam, sizeof menunam, 
			"%sbulletinmenu.%s", bdir, ansi ? "gfx" : "txt");

	menfd = open(menunam, O_RDONLY);
	if (menfd == -1) {
		snprintf(bdir, sizeof bdir, "bulletins/");
		snprintf(menunam, sizeof menunam, "bulletins/bulletinmenu.%s",
				ansi ? "gfx" : "txt");
		menfd = open(menunam, O_RDONLY);
		if (menfd == -1) {
			DDPut(sd[bunobullsstr]);
			return 0;
		}
	}
	close(menfd);
	srcstrh = params;

	for (;;) {
		int bulnum;
		
		if (strtoken(bullb, &srcstrh, sizeof bullb) > sizeof bullb)
			continue;
		
		if (!*bullb) {
			if (!first)
				TypeFile(menunam, TYPE_WARN);
			DDPut(sd[bumenustr]);
			*tbuf = 0;
			if (!(Prompt(tbuf, 60, 0)))
				return 0;
			srcstrh = tbuf;
			
			if (strtoken(bullb, &srcstrh, 
				     sizeof bullb) > sizeof bullb)
				return 0;
			if (!*bullb)
				return 0;
		}
		first = 1;
		if (*bullb == 'q' || *bullb == 'Q')
			return 0;
		if (*bullb == 'l' || *bullb == 'L' || *bullb == '?') {
			TypeFile(menunam, TYPE_WARN);
		} else if ((bulnum = atoi(bullb))) {
			snprintf(nbu, sizeof nbu, "%sbulletin.%d.%s", bdir, 
					bulnum, ansi ? "gfx" : "txt");
			if (TypeFile(nbu, TYPE_WARN)) {
				if (!(user.user_toggles & (1L << 4))) {
					DDPut(sd[pause2str]);
					HotKey(0);
				}
			}
		}
	}
}
