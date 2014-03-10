#include <assert.h>
#include <ctype.h>
#include <fcntl.h>
#include "libddc++.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "entrypack.h"
#include "listviewer.h"

#define NewFile(s) (isgraph((s)[0]))

/****************************************************************************/

FileList::FileList(int screen_len, int area) : ListViewer(screen_len), contents(NULL)
{
	DayDream_Conference *conf=GetConf();
	char *buffer=new char[512];
	sprintf(buffer, "Area %d", area);
	Title=strdup(buffer);
	snprintf(buffer, 512, "%s/data/directory.%3.3d", conf->CONF_PATH, area);
		
	entries=0;
	FILE *fp=fopen(buffer, "rt");
	if (fp==NULL) {
		delete [] buffer;
		return;
	}
	if (fgetsnolf(buffer, 512, fp)==NULL) {
		delete [] buffer;
		fclose(fp);
		return;
	}
	for (int go=1; go;) {
		ListEntryPack e(buffer, conf->CONF_ATTRIBUTES&(1L<<3));
		for (;;) {
			if (fgetsnolf(buffer, 512, fp)==NULL) {
				go=0;
				break;
			} else if (NewFile(buffer))
					break;
			else e.AddLine(buffer);
		}
				
		if (e.filename!=NULL)
			insert(e);
	}
	delete [] buffer;
	fclose(fp);
			  			           
	struct FFlag fl;
	char tmp[256];
	tmpnam(tmp);
	if (dd_dumpfilestofile(d, tmp)) {
		int fd=open(tmp, O_RDONLY);
		if (fd<0) 
			return;
		
		while (read(fd, &fl, sizeof(struct FFlag))>0) {
			if (fl.f_conf==GetConf()->CONF_NUMBER) {
				for (int i=0; i<entries; i++)
					if (!strcmp(fl.f_filename, (*(*((EntryPack *)contents[i]))[0])[0]->contents))
						contents[i]->tagged=1;
			}
		}
		close(fd);
		unlink(tmp);
	}
}

/****************************************************************************/

int FileList::HandleKeyboard(int ch)
{
	if (toupper(ch)=='T') {
		if (contents[selection]->tagged)
			dd_unflagfile(d, (*(*((EntryPack *)contents[selection]))[0])[0]->contents);
		else dd_flagfile(d, (*(*((EntryPack *)contents[selection]))[0])[0]->contents, 0);
		
		contents[selection]->tagged^=1;
		contents[selection]->modifyflag=1;
		return 1;
	}
	
	return ListViewer::HandleKeyboard(ch);
}

void FileList::insert(ListEntryPack const &e)
{
	contents=(ListEntryPack **)realloc(contents, sizeof(ListEntryPack *)*++entries);
	contents[entries-1]=new ListEntryPack(e);
}
			
/****************************************************************************/

char *FileList::GetTitle(void)
{
	return Title;
}

/****************************************************************************/

static int inverted_sort=0;

int namesort_func(const void *a, const void *b)
{
	char *sa=(*(ListEntryPack **)a)->filename;
	char *sb=(*(ListEntryPack **)b)->filename;
	return inverted_sort*strcasecmp(sa, sb);
}

int datesort_func(const void *a, const void *b)
{
	int sa=(*(ListEntryPack **)a)->date;
	int sb=(*(ListEntryPack **)b)->date;
	return inverted_sort*(sa-sb);
}

void FileList::sort(int key)
{
	inverted_sort=key&SORTKEY_ASCENDING?-1:1;
	if (key&SORTKEY_NAME) 
		qsort(contents, entries, sizeof(ListEntryPack *), namesort_func);
	else if (key&SORTKEY_DATE)
		qsort(contents, entries, sizeof(ListEntryPack *), datesort_func);
}

/****************************************************************************/

int FileList::type()
{
	return 1;
}

/****************************************************************************/

FileList::~FileList(void)
{
	for (int i=0; i<entries; i++)
		delete contents[i];
	delete [] contents;
}

/****************************************************************************/

EntryPack *FileList::GetEntry(int entry, int flags)
{
	ListEntryPack *e=contents[entry];
	e->Update(flags); // maybe the returned value could be used for stg...
	return e;
}
