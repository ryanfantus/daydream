#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "entrypack.h"
#include "libddc++.h"

EntryPack::Entry *EntryPack::operator [](int n) const 
{
	return contents[n];
}

EntryPack::EntryPack(void)
{
	modifyflag=1;
	filename=NULL;
	contents=NULL;
	entries=0; 
}

EntryPack::Entry::SubEntry *EntryPack::Entry::operator [](int n) const 
{
	return contents[n];
}

void EntryPack::reset_modify_flag(void)
{
	modifyflag=0;
}

EntryPack::Entry::~Entry(void) 
{
	if (entries)
		for (int i=0; i<entries; i++) 
			delete contents[i];
	delete [] contents;
}

int EntryPack::modified(void)
{
	return modifyflag;
}

EntryPack::Entry::Entry(void)
{ 
	contents=NULL;
	entries=0;
}

EntryPack::Entry::SubEntry::SubEntry(SubEntry const &s) 
{
	if (s.color!=NULL)
		color=strdup(s.color);
	else color=NULL;
	if (s.contents!=NULL)
		contents=strdup(s.contents);
	else contents=NULL;
}

EntryPack::Entry::SubEntry::~SubEntry(void)
{
	delete [] contents;
}

void EntryPack::insert(Entry const &s) 
{
	contents=(Entry **)realloc(contents, sizeof(Entry *)*++entries);
	contents[entries-1]=new EntryPack::Entry(s);
}

EntryPack::EntryPack(EntryPack const &e) : entries(0), contents(NULL)
{
	if (e.filename!=NULL)
		filename=strdup(e.filename);
	if (e.entries) {
		entries=e.entries;
		contents=new Entry *[entries];
		for (int i=0; i<entries; i++)
			contents[i]=new Entry(*e[i]);
	}
}

EntryPack::~EntryPack(void) 
{
	if (entries)
		for (int i=0; i<entries; i++) 
			delete contents[i];
	delete [] contents;
	delete [] filename;
}

void EntryPack::Entry::insert(SubEntry const &s) 
{
	contents=(SubEntry **)realloc(contents, sizeof(SubEntry *)*++entries);
	contents[entries-1]=new SubEntry(s);
}

EntryPack::Entry::Entry(Entry const &e) : entries(0), contents(NULL)
{
	if (e.entries) {
		entries=e.entries;
		contents=new SubEntry *[entries];
		for (int i=0; i<entries; i++)
			contents[i]=new SubEntry(*e[i]);
	}
}

EntryPack::Entry::SubEntry::SubEntry(char *a, int w, int j, const char *fmt, ...)
{
	color=(a==NULL)?(char *)NULL:a;
	char *buf=new char[256];
	char tmp[256], *tmp2=tmp;
	va_list args;
	va_start(args, fmt);
	vsnprintf(buf, 256, fmt, args);
	va_end(args);
	if (j) 
		tmp2+=sprintf(tmp, "\r\e[%dC", j);
	if (!w)	
		w=strlen(buf);
	sprintf(tmp2, "%-*.*s", w, w, buf);
	contents=strdup(tmp);
	delete [] buf;
}

