#include <time.h>

#include <daydream.h>

static int inchat = 0;

void LineChat(void)
{
	char currentchar;
	int chaton = 1;
	int prevcolor = 0;
	char linebuffer[86];
	int i = 0;
	char delbuf[107];
	int kala;
	int starttimeleft;
	time_t timenow;
	int autorz = 0;

	if (inchat)
		return;

	pageflag = 1;

	starttimeleft = timeleft;

	clog.cl_flags |= CL_CHAT;

	changenodestatus("Chatting with SysOp");

	TypeFile("linechaton", TYPE_MAKE | TYPE_WARN);

	inchat = 1;

	while (chaton) {
		currentchar = HotKey(0);

		if (timeleft > starttimeleft)
			starttimeleft = timeleft;
		if (!checkcarrier())
			chaton = 0;

		if (prevcolor != keysrc) {
			if (keysrc == 1)
				DDPut(maincfg.CFG_COLORUSER);
			if (keysrc == 2)
				DDPut(maincfg.CFG_COLORSYSOP);
			prevcolor = keysrc;
		}
		if (currentchar == 'r' && keysrc == 1 && autorz == 0) {
			autorz++;
		} else if (currentchar == 'z' && keysrc == 1 && autorz == 1) {
			autorz++;
		} else if ((currentchar == 13 || currentchar == 10) && keysrc == 1 && autorz == 2) {
			recfiles(maincfg.CFG_CHATDLPATH, 0);
			autorz = 0;
		} else
			autorz = 0;

		if (currentchar == 27 && keysrc == 2)
			chaton = 0;
		else {
			if (currentchar == 7) {
				delbuf[0] = 7;
				delbuf[1] = 0;
				DDPut(delbuf);
			}
			if (currentchar < 0 || currentchar == 8 || currentchar > 31 || currentchar == 10 || currentchar == 13) {
				if (currentchar == 127 || currentchar == 8) {
					if (i) {
						DDPut("\033[D \033[D");
						i--;
						linebuffer[i] = 0;
					}
				} else {
					if (i == 77) {
						kala = 1;
						while (kala) {
							i--;
							if (linebuffer[i] == ' ') {
								ddprintf("\033[%dD\033[K\n%s", 77 - i, &linebuffer[i + 1]);
								i = 77 - i;
								kala = 0;
							}
							if (!i) {
								kala = 0;
								DDPut("\n");
							}
						}
					}
					if (currentchar == 13)
						currentchar = 10;

					ddprintf("%c", currentchar);
					linebuffer[i] = currentchar;
					i++;
					linebuffer[i] = 0;
					if (currentchar == 10)
						i = 0;
				}
			}
		}
	}
	TypeFile("linechatoff", TYPE_MAKE | TYPE_WARN);
	timenow = time(0);
	timeleft = starttimeleft;
	endtime = timenow + timeleft;
	inchat = 0;
}
