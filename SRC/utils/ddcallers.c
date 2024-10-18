#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <dd.h>

struct storday {
	int yday;
	uint64_t ulbytes;
	uint64_t dlbytes;
	int calls;
	int newusers;
	int hacks;
	int carrierlosses;
	int pages;
	int ulfiles;
	int dlfiles;
	int mins;
};

struct storage {
	char storlines[78 * 40];
	struct storday days[7];
};

static char *header = 0;
static char *output = 0;
static char *weekrep = 0;

static int namemode = 0;
static int orgmode = 0;
static int update = 1;
static int lines = 10;
static int node = 0;
static int incsysop = 1;
static FILE *outfd1;
static FILE *outfd2;
static char fbuf[1024];

static struct storage stor;
static struct tm myt;

static void showhelp(void);
static int createstorage(void);
static int genweekrep(char *);
static int dooutput(void);

int main(int argc, char *argv[])
{
	int storfd;
	char buf1[1024];
	int logfd, go;
	time_t ti;

	struct callerslog clog;

	memset(&stor, 0, sizeof(struct storage));
	
	time(&ti);
	memcpy(&myt, localtime(&ti), sizeof(struct tm));

	go = 1;
	while (go) {
		switch (getopt(argc, argv, "rluh:w:o:a:n:es")) {
		case EOF:
			go = 0;
			break;
		case '?':
		case ':':
			showhelp();
			exit(1);
		case 'r':
			namemode = 1;
			break;
		case 'l':
			orgmode = 1;
			break;
		case 'u':
			update = 0;
			break;
		case 'h':
			header = optarg;
			break;
		case 'w':
			weekrep = optarg;
			break;
		case 'o':
			output = optarg;
			break;
		case 'a':
			lines = atoi(optarg);
			break;
		case 'n':
			node = atoi(optarg);
			break;
		case 'e':
			incsysop = 0;
			break;
		case 's':
			createstorage();
			exit(0);
			break;
		}
	}
	
	if (!output || !node || !lines || lines > 40) {
		showhelp();
		exit(1);
	}
	sprintf(fbuf, "%s/data/ddcallers.dat", getenv("DAYDREAM"));
	storfd = open(fbuf, O_RDWR);
	if (storfd == -1) {
		createstorage();
		perror("Can't open storage!");
		exit(1);
	}
	read(storfd, &stor, sizeof(struct storage));

	sprintf(buf1, "%s.txt", output);
	if (!(outfd1 = fopen(buf1, "w"))) {
		perror("Can't open output");
		exit(1);
	}
	sprintf(buf1, "%s.gfx", output);
	if (!(outfd2 = fopen(buf1, "w"))) {
		perror("Can't open output");
		exit(1);
	}
	if (update) {
		struct stat st;
		int userbfd;
		struct userbase user;
		struct tm tm;
		struct tm tem1;
		char upb[10];
		char dnb[10];
		time_t tk;
		struct storday *sd;

		sprintf(buf1, "%s/logfiles/callerslog%d.dat", getenv("DAYDREAM"), node);
		logfd = open(buf1, O_RDONLY);
		if (logfd == -1) {
			perror("Can't open callerslog!");
			exit(1);
		}

		if (fstat(logfd, &st) == -1) {
			perror("cannot stat callerslog");
			exit(1);
		}
		if (st.st_size % sizeof(struct callerslog)) {
			fputs("log file size not integer multiple", stderr);
			exit(1);
		}
		lseek(logfd, st.st_size - sizeof(struct callerslog), SEEK_SET);
		read(logfd, &clog, sizeof(struct callerslog));
		close(logfd);
		sprintf(fbuf, "%s/data/userbase.dat", getenv("DAYDREAM"));
		userbfd = open(fbuf, O_RDONLY);
		if (userbfd == -1) {
			perror("Can't open userbase!");
			exit(1);
		}
		lseek(userbfd, clog.cl_userid * sizeof(struct userbase), SEEK_SET);
		read(userbfd, &user, sizeof(struct userbase));
		close(userbfd);

		if (user.user_account_id || incsysop) {
			char bpsr[128];
			memcpy(&tem1, localtime(&clog.cl_logon), sizeof(struct tm));
			tk = clog.cl_logoff - clog.cl_logon;
			memcpy(&tm, gmtime(&tk), sizeof(struct tm));

			if ((clog.cl_ulbytes / 1048576) > 9999) 
				sprintf(upb, "%3.3dG", clog.cl_ulbytes / (1024 * 1048576));
			else if ((clog.cl_ulbytes / 1024) > 9999) {
				sprintf(upb, "%3.3dM", clog.cl_ulbytes / (1024 * 1024));
			} else if (clog.cl_ulbytes) {
				sprintf(upb, "%4.4d", clog.cl_ulbytes / 1024);
			} else {
				strcpy(upb, "----");
			}

			if ((clog.cl_dlbytes / 1048576) > 9999) 
				sprintf(dnb, "%3.3dG", clog.cl_dlbytes / (1048576 * 1024));
			if ((clog.cl_dlbytes / 1024) > 9999) {
				sprintf(dnb, "%3.3dM", clog.cl_dlbytes / (1024 * 1024));
			} else if (clog.cl_dlbytes) {
				sprintf(dnb, "%4.4d", clog.cl_dlbytes / 1024);
			} else {
				strcpy(dnb, "----");
			}

			sprintf(&bpsr[100], "%-5d", clog.cl_bpsrate);
			if (strlen(&bpsr[100]) > 5) {
				bpsr[0] = bpsr[100];
				bpsr[1] = bpsr[101];
				bpsr[2] = bpsr[102];
				bpsr[3] = 'k';
				bpsr[4] = bpsr[103];
				bpsr[5] = 0;
			} else
				strcpy(bpsr, &bpsr[100]);

			sprintf(buf1, "%2.2d %s %-16.16s %-22.22s %2.2d:%2.2d %2.2d:%2.2d %s:%s %c%c%c%c%c%c\n",
				node, bpsr,
			namemode ? user.user_realname : user.user_handle,
				orgmode ? user.user_zipcity : user.user_organization,
				tem1.tm_hour, tem1.tm_min, tm.tm_hour, tm.tm_min,
				upb, dnb,
				clog.cl_ulbytes ? 'U' : '-',
				clog.cl_dlbytes ? 'D' : '-',
			    (clog.cl_flags & CL_CARRIERLOST) ? 'C' : '-',
				(clog.cl_flags & CL_NEWUSER) ? 'N' : '-',
			     (clog.cl_flags & CL_PAGEDSYSOP) ? 'P' : '-',
			    (clog.cl_flags & CL_PASSWDFAIL) ? 'H' : '-');
			memcpy(&stor.storlines[0], &stor.storlines[78], 78 * 39);
			memcpy(&stor.storlines[78 * 39], buf1, 78);

			sd = &stor.days[myt.tm_wday];
			if (sd->yday != myt.tm_yday) {
				sd->yday = myt.tm_yday;
				sd->ulbytes = sd->dlbytes = sd->calls = sd->newusers = sd->hacks = sd->carrierlosses = sd->pages = sd->ulfiles = sd->dlfiles = 0;
			}
			sd->ulbytes += clog.cl_ulbytes;
			sd->dlbytes += clog.cl_dlbytes;
			sd->ulfiles += clog.cl_ulfiles;
			sd->dlfiles += clog.cl_dlfiles;
			sd->calls++;
			if (clog.cl_flags & CL_NEWUSER)
				sd->newusers++;
			if (clog.cl_flags & CL_PASSWDFAIL)
				sd->hacks++;
			if (clog.cl_flags & CL_CARRIERLOST)
				sd->carrierlosses++;
			if (clog.cl_flags & CL_PAGEDSYSOP)
				sd->pages++;
			sd->mins += (clog.cl_logoff - clog.cl_logon) / 60;
			lseek(storfd, 0, SEEK_SET);
			write(storfd, &stor, sizeof(struct storage));
		}
	}
	if (header) {
		FILE *hfd;
		char hbuf1[1024];

		sprintf(hbuf1, "%s.txt", header);
		if ((hfd = fopen(hbuf1, "r"))) {
			while (fgets(hbuf1, 1024, hfd))
				fputs(hbuf1, outfd1);
			fclose(hfd);
		}
		sprintf(hbuf1, "%s.gfx", header);
		if ((hfd = fopen(hbuf1, "r"))) {
			while (fgets(hbuf1, 1024, hfd))
				fputs(hbuf1, outfd2);
			fclose(hfd);
		}
	}
	dooutput();
	if (weekrep)
		genweekrep(weekrep);
	close(storfd);
	return 0;
}

static int genweekrep(char *rep)
{
	char wb[1024];
	FILE *o1;
	FILE *o2;
	int i;
	int j = myt.tm_wday;
	const char *dayns[] =
	{"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"};
	struct storday totd;

	struct storday *td;
	memset(&totd, 0, sizeof(struct storday));
	for (i = 0; i < 7; i++) {
		td = &stor.days[i];
		totd.ulbytes += td->ulbytes;
		totd.dlbytes += td->dlbytes;
		totd.dlfiles += td->dlfiles;
		totd.ulfiles += td->ulfiles;
		totd.calls += td->calls;
		totd.newusers += td->newusers;
		totd.hacks += td->hacks;
		totd.carrierlosses += td->carrierlosses;
		totd.pages += td->pages;
		totd.mins += td->mins;
	}
	sprintf(wb, "%s.gfx", rep);
	o1 = fopen(wb, "w");
	if (!o1)
		return 0;
	sprintf(wb, "%s.txt", rep);
	o2 = fopen(wb, "w");
	if (!o2) {
		fclose(o1);
		return 0;
	}
	fputs("[2J[H[36m                               Weekly Stats \n"
	      "\n"
	      "[33mDay        Calls  News Hacks Drops Pages Hours    UpKb Uplds    DnKb Dnlds\n"
	      "[34m--------------------------------------------------------------------------\n", o1);

	fputs("[2J[H                               Weekly Stats \n"
	      "\n"
	      "Day        Calls  News Hacks Drops Pages Hours    UpKb Uplds    DnKb Dnlds\n"
	      "--------------------------------------------------------------------------\n", o2);

	for (i = 0; i < 7; i++) {
		struct storday *td;

		td = &stor.days[j];
		fprintf(o2, "%-9.9s %6d %5d %5d %5d %5d %5d %7qu %5d %7qu %5d\n", dayns[j], td->calls, td->newusers, td->hacks, td->carrierlosses, td->pages, td->mins / 60, td->ulbytes / 1024, td->ulfiles, td->dlbytes / 1024, td->dlfiles);
		fprintf(o1, "[36m%-9.9s [32m%6d [33m%5d [34m%5d [35m%5d [36m%5d [37m%5d [32m%7qu [33m%5d [34m%7qu [35m%5d\n", dayns[j], td->calls, td->newusers, td->hacks, td->carrierlosses, td->pages, td->mins / 60, td->ulbytes / 1024, td->ulfiles, td->dlbytes / 1024, td->dlfiles);
		j--;
		if (j < 0)
			j = 6;
	}
	fprintf(o1, "[34m--------------------------------------------------------------------------\n"
		"[36mTOTAL     [32m%6d [33m%5d [34m%5d [35m%5d [36m%5d [37m%5d [32m%7qu [33m%5d [34m%7qu [35m%5d\n",
		totd.calls, totd.newusers, totd.hacks, totd.carrierlosses, totd.pages, (totd.mins / 60), (totd.ulbytes / 1024), totd.ulfiles, (totd.dlbytes / 1024), totd.dlfiles);
	fprintf(o1, "[36mAVERAGE   [32m%6d [33m%5d [34m%5d [35m%5d [36m%5d [37m%5d [32m%7qu [33m%5d [34m%7qu [35m%5d\n",
		totd.calls / 7, totd.newusers / 7, totd.hacks / 7, totd.carrierlosses / 7, totd.pages / 7, (totd.mins / 60) / 7, (totd.ulbytes / 1024) / 7, totd.ulfiles / 7, (totd.dlbytes / 1024) / 7, totd.dlfiles / 7);
	fputs("[34m--------------------------[ [36mDDCallers V2.0 - Coded by Antti Häyrynen [34m]----\n[0m", o1);

	fprintf(o2, "--------------------------------------------------------------------------\n"
		"TOTAL     %6d %5d %5d %5d %5d %5d %7qu %5d %7qu %5d\n",
		totd.calls, totd.newusers, totd.hacks, totd.carrierlosses, totd.pages, (totd.mins / 60), (totd.ulbytes / 1024), totd.ulfiles, (totd.dlbytes / 1024), totd.dlfiles);
	fprintf(o2, "AVERAGE   %6d %5d %5d %5d %5d %5d %7qu %5d %7qu %5d\n",
		totd.calls / 7, totd.newusers / 7, totd.hacks / 7, totd.carrierlosses / 7, totd.pages / 7, (totd.mins / 60) / 7, (totd.ulbytes / 1024) / 7, totd.ulfiles / 7, (totd.dlbytes / 1024) / 7, totd.dlfiles / 7);
	fputs("--------------------------[ DDCallers V2.0 - Coded by Antti Häyrynen ]----\n", o2);
	return 1;
}

static int dooutput(void)
{
	char buf[80];
	char cbuf[300];
	char *s;
	int i;
	struct storday *td;
	struct storday *yd;

	td = &stor.days[myt.tm_wday];
	if (myt.tm_wday == 0) {
		yd = &stor.days[6];
	} else {
		yd = &stor.days[myt.tm_wday - 1];
	}
	s = &stor.storlines[78 * 39];

	for (i = lines; i; i--) {
		strncpy(buf, s, 78);
		s = &s[-78];
		buf[79] = 0;
		fputs(buf, outfd1);

		strcpy(cbuf, "[35m");
		strncat(cbuf, buf, 3);
		strcat(cbuf, "[36m");
		strncat(cbuf, &buf[3], 6);
		strcat(cbuf, "[33m");
		strncat(cbuf, &buf[9], 17);
		strcat(cbuf, "[32m");
		strncat(cbuf, &buf[26], 23);
		strcat(cbuf, "[0m");
		strncat(cbuf, &buf[49], 2);
		strcat(cbuf, "[34m");
		strncat(cbuf, &buf[51], 1);
		strcat(cbuf, "[0m");
		strncat(cbuf, &buf[52], 3);

		strcat(cbuf, "[36m");
		strncat(cbuf, &buf[55], 2);
		strcat(cbuf, "[34m");
		strncat(cbuf, &buf[57], 1);
		strcat(cbuf, "[36m");
		strncat(cbuf, &buf[58], 3);

		strcat(cbuf, "[35m");
		strncat(cbuf, &buf[61], 4);
		strcat(cbuf, "[34m");
		strncat(cbuf, &buf[65], 1);
		strcat(cbuf, "[35m");
		strncat(cbuf, &buf[66], 5);

		strcat(cbuf, "[33m");
		strncat(cbuf, &buf[71], 7);

		fputs(cbuf, outfd2);
	}
	fputs("-----------------------------------------------------------------------------\n", outfd1);
	fputs("[ D ] - Download             [ U ] - Upload              [ C ] - Carrier lost\n", outfd1);
	fputs("[ H ] - Password Failure     [ P ] - Paged Sysop         [ N ] - New user\n", outfd1);
	fputs("-----------------------------------------------------------------------------\n[0m", outfd1);
	fprintf(outfd1, " Today     : Ul's ( %3d / %6qukB )  Dl's ( %3d / %6qukB )  Calls ( %3d )\n", td->ulfiles, td->ulbytes / 1024, td->dlfiles, td->dlbytes / 1024, td->calls);
	fprintf(outfd1, " Yesterday : Ul's ( %3d / %6qukB )  Dl's ( %3d / %6qukB )  Calls ( %3d )\n", yd->ulfiles, yd->ulbytes / 1024, yd->dlfiles, yd->dlbytes / 1024, yd->calls);
	fputs("-----------------------------[ DDCallers V2.0 - Coded by Antti Häyrynen ]----\n", outfd1);

	fputs("[34m-----------------------------------------------------------------------------\n[0m", outfd2);
	fputs("[34m[ [36mD[34m ] - [35mDownload          [34m   [ [36mU [34m] - [35mUpload              [34m[ [36mC [34m] - [35mCarrier lost\n", outfd2);
	fputs("[34m[ [36mH[34m ] - [35mPassword Failure  [34m   [ [36mP [34m] - [35mPaged Sysop         [34m[ [36mN [34m] - [35mNew user\n", outfd2);
	fputs("[34m-----------------------------------------------------------------------------\n[0m", outfd2);
	fprintf(outfd2, "[33m Today     [0m: [33mUl's [0m([32m %3d [33m/[32m %6qukB[0m )[33m  Dl's [0m([32m %3d[33m /[32m %6qukB[0m ) [33m Calls [0m([32m %3d[0m )\n", td->ulfiles, td->ulbytes / 1024, td->dlfiles, td->dlbytes / 1024, td->calls);
	fprintf(outfd2, "[33m Yesterday [0m: [33mUl's [0m([32m %3d [33m/[32m %6qukB[0m )[33m  Dl's [0m([32m %3d[33m /[32m %6qukB[0m ) [33m Calls [0m([32m %3d[0m )\n", yd->ulfiles, yd->ulbytes / 1024, yd->dlfiles, yd->dlbytes / 1024, yd->calls);
	fputs("[34m-----------------------------[ [36mDDCallers V2.0 - Coded by Antti Häyrynen [34m]----\n", outfd2);

	fclose(outfd1);
	fclose(outfd2);
	return 1;
}

static void showhelp(void)
{
	printf("DDCallers v1.0 - Code by Antti Häyrynen\n");
	printf("Usage: ddcallers -n [node] -h [header] -o [output] -a [lines] \n"
	       "                 -w [weekly report] -r -l -u \n");
	printf("\nOptions:\n  -r -> use realnames, -l -> use location, -u - no update\n");
	printf("\nddcallers -s to clear storagefile!\n");
}

static int createstorage(void)
{
	FILE *storagef;
	int i;

	sprintf(fbuf, "%s/data/ddcallers.dat", getenv("DAYDREAM"));

	if ((storagef = fopen(fbuf, "w"))) {
		for (i = 0; i != 40; i++) 
			fputs("01 57600 -----            --------------         00:00 00:10 ----:---- UDCNPH\n", storagef);
		
		fclose(storagef);
		return 1;
	}
	return 0;
}
