#include <assert.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ctype.h>
#include "libddc++.h"
#include <string.h>
#include "entrypack.h"
#include "listviewer.h"
#include "common.h"

ListEntryPack::ListEntryPack(char *s, int lnames) : oldflags(-1)
{
	if (!s) {
		filename = NULL;
		return;
	}
	
	size_t s_len = strlen(s);
	size_t offset = lnames ? 34 : 12;
	
	if (offset > s_len) {
		filename = NULL;
		return;
	}
	
	char *t = s + offset;
	// Fix the dangerous loop - ensure we don't go before start of string
	while (t > s && isspace(t[-1])) {
		t--;
	}
	
	char *buffer=new char[512];
	char *basename=strdup(s);
	for (char *p=basename; *p; p++) 
		if (isspace(*p)) {
			*p=0;
			break;
		}
	
	if (FindFile(basename, buffer)) {
		struct stat st;
		filename=strdup(buffer);
		assert(!stat(buffer, &st));
		date=st.st_mtime;
	} else filename=NULL;
      	
	delete [] buffer;
	delete [] basename;
	
	if (filename==NULL)
		return;
	
	*t=0;
	Entry e;
	e.insert(Entry::SubEntry((char *)NULL, 0, 0, "%s", s));
		
	for (t++; *t&&isspace(*t); t++);
	if (*t)
		e.insert(Entry::SubEntry(stdcolor, 0, lnames?35:13, "%s", t));
	insert(e);
}

/****************************************************************************/

char *ListEntryPack::GetFileName()
{
	return (*(*this)[0])[0]->contents;
}

/****************************************************************************/

void ListEntryPack::AddLine(char *s)
{
	Entry e;
	e.insert(Entry::SubEntry(stdcolor, 0, 0, "%s", s));
	insert(e);
}

/****************************************************************************/

int ListEntryPack::Update(int flags) 
{
	(*(*this)[0])[0]->color=DirColors[tagged][0][flags&L_SEL?1:0];
	if (flags==oldflags)
		return 0;
      
	oldflags=flags;

	modifyflag=1;
	return 1;
}
