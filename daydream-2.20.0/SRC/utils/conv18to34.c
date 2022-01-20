/* Convert Amiga-DD 18 char filelists to new 34 char long format */

#include <time.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[])
{
	char inp[400];
	char outp[400];

	FILE *infile;
	char *s;
	char *t;

	if (argc != 2) {
		printf
		    ("Convert 18 char filelists to 34 char filelists by Antti Häyrynen.\nUsage: %s [oldlist]\n\nOutput is stdio!\n",
		     argv[0]);
		exit(0);
	}
	if (!(infile = fopen(argv[1], "r"))) {
		printf("Can't open infile!\n");
		exit(0);
	}

	while (fgets(inp, sizeof inp, infile)) {
		if (*inp == ' ') {
			write(STDOUT_FILENO, inp, strlen(inp));
		} else {
			int i;
			struct tm tm;

			t = inp;
			s = outp;

			for (i = 0; i < 19; i++)
				outp[i] = inp[i];
			for (i = 19; i < 35; i++)
				outp[i] = ' ';

			outp[35] = inp[19];
			outp[36] = inp[20];
			outp[37] = '-';
			outp[38] = '-';
			outp[39] = ' ';
			outp[40] = inp[22];
			outp[41] = inp[23];
			outp[42] = inp[24];
			outp[43] = inp[25];
			outp[44] = '0';
			outp[45] = '0';
			outp[46] = '0';
			outp[47] = ' ';
			inp[0] = inp[28];
			inp[1] = inp[29];
			inp[2] = 0;
			inp[3] = inp[30];
			inp[4] = inp[31];
			inp[5] = 0;
			inp[6] = inp[32];
			inp[7] = inp[33];
			inp[8] = 0;

			tm.tm_sec = 0;
			tm.tm_min = 0;
			tm.tm_hour = 0;
			tm.tm_mday = atoi(inp);
			tm.tm_mon = atoi(&inp[3]) - 1;
			tm.tm_year = atoi(&inp[6]);
			/* Y2K hack */
			if (tm.tm_year < 70)
				tm.tm_year += 100;

			strcpy(&outp[48], asctime(&tm));
			write(STDOUT_FILENO, outp, strlen(outp));
			write(STDOUT_FILENO,
			      "                                   ", 35);
			write(STDOUT_FILENO, &inp[35], strlen(&inp[35]));
		}
	}
	fclose(infile);

	return 0;
}
