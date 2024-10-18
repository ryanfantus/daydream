#include <stdlib.h>
#include <string.h>
#include "entrypack.h"
#include "common.h"

char *DirColors[2][2][2]={
	{
		{"\e[0;36m", "\e[0;41;33;1m"},
		{"\e[0;37;1m", "\e[0;42;37;1m"}},
	{
		{"\e[44;37;1m", "\e[0;42;33;1m"},
		{"", ""}}
};
		
char *stdcolor="\e[0;37m";

/****************************************************************************/

char *basename(char *buffer, char *path)
{
	char *p;
	if (path==NULL)
		return NULL;
	if ((p=strrchr(path, '/'))==NULL)
		p=path;
	else p++;
	if (buffer==NULL)
		return strdup(p);
	else {
		strcpy(buffer, p);
		return buffer;
	}
}

/****************************************************************************/

int ext_atoi(char *s, int len)
{
	char tmp=s[len];
	s[len]=0;
	int i=atoi(s);
	s[len]=tmp;
	return i;
}

/****************************************************************************/

FVEntryPack::FVEntryPack(void) : dirflag(0), tagged(0)
{
};

/****************************************************************************/

void FVEntryPack::SetTagStatus(int t) 
{
	tagged=t;
};

/****************************************************************************/

void FVEntryPack::ToggleTag(void) 
{
	tagged=tagged?0:1;
};
