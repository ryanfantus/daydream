#ifndef _COMMON_H_INCLUDED
#define _COMMON_H_INCLUDED

#include <stdio.h>

#define EX_FAIL     0
#define EX_INTR     1
#define EX_SUCCESS  2

extern char *DirColors[2][2][2];
extern char *stdcolor;

extern int ext_atoi(char *, int);
extern int extract_packet(char *, char *);
extern char *basename(char *, char *);

extern char *helpfile;

extern int parse_file(FILE *);

#endif  /* _COMMON_H_INCLUDED */
