#include <stdio.h>
#include <ctype.h>
#include <string.h>

int bgtab[] = { 40,44,42,46,41,45,43,47 };

int fgtab[] = { 30,34,32,36,31,45,43,47 };

int main(int argc, char *argv[])
{
	FILE *fi;
	char ibuf[4096];
	char *s, *t;
	char cbuf[256];
	char b[64];
	int fg, bg;
	
	if (argc < 2) {
		printf("%s v0.001 [pcb textfile]\n",argv[0]);
		exit(0);
	}
	fi=fopen(argv[1],"r");
	if (!fi) {
		perror("Can't open source\n");
		exit(1);
	}
	b[1]=0;

	while(fgets(ibuf,4096,fi)) {
		s=ibuf;
		while(*s) {
			if (*s=='@' && s[1]=='X') {
				t=cbuf;
				*t++=*s++;
				*t++=*s++;
				*t++=*s++;
				*t++=*s++;

				if (*s=='@') *t++=*s++;
				*t++=0;
				if (!strncmp(cbuf,"@X",2)) {
					*b=tolower(cbuf[2]);
					sscanf(b,"%x",&bg);
					*b=tolower(cbuf[3]);
					sscanf(b,"%x",&fg);
					if (fg>7) {
						fputs("[1;",stdout);
						fg-=8;
					} else {
						fputs("[0;",stdout);
					}
					fprintf(stdout,"%d;%dm",fgtab[fg],bgtab[bg]);
				}
			} else {
				fputc(*s++,stdout);
			}
		}
	}
	fclose(fi);
	return 0;
}
