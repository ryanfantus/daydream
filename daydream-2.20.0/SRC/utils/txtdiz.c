/*
 Extracts file_id.diz from text files .. f00!
 */
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <config.h>

static FILE *inf;
static FILE *outf;

static int gou=1;

static char *fgetsnolf(char *, int, FILE *) __attr_bounded__ (__string__, 1, 2);
static void showhelp(void);
static void wrline(char *);

int main(int argc, char *argv[])
{
	char buf[4096];
	char *s;
	
	if (argc < 2) {
		showhelp();
		exit(1);
	}
	if (!(inf=fopen(argv[1],"r"))) {
		printf("Can't open textfile!\n");
		exit(1);
	}
	if (argc == 3) {
		outf=fopen(argv[2],"w");
		if (outf==0) {
			printf("Can't open output!\n");
			exit(1);
		}
	} else {
		outf=stdout;
	}

	while(fgetsnolf(buf,4096,inf))
	{
		s=strstr(buf,"@BEGIN_FILE_ID.DIZ");
		if (s) {
			while(*s!='Z') s++;
			s++;
			if (*s) {
				wrline(s);
			}
			while(gou) {
				if (!fgetsnolf(buf,4096,inf)) break; 
				wrline(buf);
			}

		}
	
	}
	fclose(inf);
	if (outf!=stdout) fclose(outf);
	return 0;
}

static void wrline(char *s)
{
	char *t;
	if (!*s) return;
	
	t=strstr(s,"@END_FILE_ID.DIZ");
	if (t) {
		*t=0;
		gou=0;
	}
	fprintf(outf,"%-44.44s\n",s);
}

static void showhelp(void)
{
	printf("TxtDiz v1.00 - written by Hydra! Usage:\n\n");
	printf("txtdiz [file] <output>\n");
}

static char *fgetsnolf(char *buf, int n, FILE *fh)
{
	char *hih;
	char *s;
	
	hih=fgets(buf,n,fh);
	if (!hih) return 0;
	s=buf;
	while (*s)
	{
		if (*s==13 || *s==10) {
			*s=0;
			break;
		}
		s++;
	}
	return hih;
}
