#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libddc++.h"

char *fgetsnolf2(char *buffer, int maxlen, FILE *fp)
{
	for (;;) {
		char *p;
		if (fgetsnolf(buffer, maxlen, fp)==NULL)
			return NULL;
		for (p=buffer; *p; p++) 
			if (!isspace(*p))
				return p;
	}
}

int compile_quote(char *dest, char *src)
{
	int quote_on=0;
	char *dest_start = dest;
	const size_t max_dest_len = 256;  // Assume reasonable max length
	
	if (!src || !dest) {
		if (dest) *dest = 0;
		return -1;
	}
	
	if (!*src) {
		*dest=0;
		return 0;
	}

	if (*src=='\"') {
		quote_on=1;
		src++;
	}
	
	while (*src) {
		// Check if we're approaching buffer limit
		if ((size_t)(dest - dest_start) >= max_dest_len - 1) {
			*dest = 0;
			return -1;
		}
		
		if (*src=='\\') {
			if (src[1]) {
				*dest++=src[1];
				src+=2;
				continue;
			} else return -1;
		} else if (*src=='~') {
			char *home=getenv("DAYDREAM");
			if (home) {
				size_t home_len = strlen(home);
				size_t remaining_space = max_dest_len - (dest - dest_start) - 1;
				if (home_len >= remaining_space) {
					// Not enough space for home directory path
					*dest = 0;
					return -1;
				}
				strcpy(dest, home);
				dest += home_len;
			}
			src++;
			continue;
		} else if (*src=='\"') {
			if (!quote_on)
				return -1;
			if (src[1])
				return -1;
			quote_on=0;
			src++;
			break;
		} else *dest++=*src++;
	}
	
	if (quote_on)
		return -1;
	*dest=0;
	return 0;
}

void tokenize(char *line, char **tk, char **val)
{	
	while (*line&&!isgraph(*line))
		line++;
	*tk=line;
	while (*line&&isgraph(*line))
		line++;
	if (*line) 
		*line++=0;
	while (*line&&!isgraph(*line))
		line++;
	*val=line;
	while (*line&&isgraph(*line))
		line++;
	*line=0;
}

typedef struct {
	char *word;
	int type;
	void *param;
	int read;
} keyword_t;

enum {
	KW_CPYSTRING	= 0x01
};
	
char *helpfile=NULL;

keyword_t keywords[]={
	{"helpfile",	KW_CPYSTRING,	&helpfile},
	{NULL,		0,		NULL}
};

void init_parser()
{
	keyword_t *t;
	for (t=keywords; t->word; t++)
		t->read=0;
}

int parse_file(FILE *fp)
{	
	char buffer[256], tmp[256];
	init_parser();
	
	while ((fgetsnolf2(buffer, 256, fp))) {
		keyword_t *t;
		char *kw, *param;
		tokenize(buffer, &kw, &param);				
		for (t=keywords; t->word; t++)
			if (!strcasecmp(t->word, kw))
				break;
		if (!t->word) 
			return -1;
		if (t->read)
			return -1;
		
		switch (t->type) {
		case KW_CPYSTRING:
			if (compile_quote(tmp, param))
				return -1;
			*((char **)t->param)=strdup(tmp);
			break;
		default:
			return -1;
		}
	}	
	
	return 0;
}

/*
int main(int argc, char *argv[])
{
	FILE *fp;
	if (argc<2)
		return 1;
	if ((fp=fopen(argv[1], "r"))==NULL)
		return 0;

	if (parse_file(fp))
		printf("syntax error\n");
	else printf("%s\n", helpfile);
	
	fclose(fp);

      
	
	return 0;
}*/
