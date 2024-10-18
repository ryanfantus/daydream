#ifndef _LIBDDCPP_H_INCLUDED
#define _LIBDDCPP_H_INCLUDED

#include <stdio.h>

#include <ddlib.h>

extern struct DayDream_Archiver *arcs;
extern struct DayDream_Archiver *arc;
extern struct dif *d;

extern void loadarchivers();
extern int getarchiver(char *);
extern void dprintf(const char *, ...);
extern int HotKey(int=0);
char *fgetsnolf(char *, int, FILE *) __attr_bounded__ (__string__, 1, 2);
extern DayDream_Conference *GetConf(int=-1);
extern int FindFile(char *, char *);
extern void GetStrVal(char *, size_t, int);
extern int GetIntVal(int);
extern int wildcmp(char *, char *);

extern void DoorCode();

#endif /* _LIBDDCPP_H_INCLUDED */
