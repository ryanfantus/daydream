#include <stdlib.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include "listviewer.h"
#include "entrypack.h"
#include "config.h"

DirectoryList::DirectoryList(int sl, char *path, char *title) : ListViewer(sl)
{
	Title=strdup(title);
	Path=strdup(path);
	Load();
};

int DirectoryList::HandleKeyboard(int ch)
{
	if (ch>='0' && ch<='9')
		return 0;
	
	return ListViewer::HandleKeyboard(ch);
}

/****************************************************************************/

void DirectoryList::sort(int key)
{
}

/****************************************************************************/

char *DirectoryList::GetTitle(void)
{
	return Title;
};

/****************************************************************************/

char **__select_path;

int select_func(const dirent *de) 
{
	char buffer[256];
	struct stat st;
	
	// Use snprintf for safe buffer handling
	int result = snprintf(buffer, sizeof(buffer), "%s/%s", *__select_path, de->d_name);
	if (result >= (int)sizeof(buffer)) {
		// Path too long, skip this entry
		return 0;
	}
	
	if (stat(buffer, &st) != 0) {
		// stat failed, skip this entry
		return 0;
	}
	
	if ((!S_ISDIR(st.st_mode))||(strcmp(de->d_name, ".")&&strcmp(de->d_name, "..")))
		return 1;
	else return 0;
};

/****************************************************************************/

#ifndef HAVE_ALPHASORT
int alphasort(const void *a, const void *b)
{
	struct dirent **da, **db;
	da = (struct dirent **) a;
	db = (struct dirent **) b;
	return strcmp((*da)->d_name, (*db)->d_name);
}
#endif 

#ifndef HAVE_SCANDIR
/* FIXME: this needs to be reaudited */
int scandir(const char *dir, struct dirent ***namelist, 
	int (*selector)(const struct dirent *), 
	int (*cmp)(const void *, const void *))
{
	DIR *dirhandle;
	struct dirent **nl;
	struct dirent *dent;
	int alloc = 4, entries = 0;
	*namelist = (struct dirent **) malloc(sizeof(struct dirent *) * alloc);
	if (!*namelist)
		return -1;
	if ((dirhandle = opendir(dir)) == NULL) { 
		free(*namelist);  // Fix: free the allocated memory, not the pointer to pointer
		*namelist = NULL;
		return -1;
	}	
		while ((dent = readdir(dirhandle)) != NULL) {
		if (alloc <= entries + 1) {  // Fix: use <= to ensure we have space
			alloc *= 2;
			nl = (struct dirent **) realloc(*namelist, 
				sizeof(struct dirent *) * alloc);
			if (nl == NULL) {
				// Clean up existing allocations before returning
				for (int i = 0; i < entries; i++) {
					free((*namelist)[i]);
				}
				free(*namelist);
				*namelist = NULL;
				closedir(dirhandle);
				return -1;
			}
			*namelist = nl;
		}
		if (selector && !selector(dent))
			continue;
		(*namelist)[entries] = (struct dirent *) 
			malloc(sizeof(struct dirent));
		if ((*namelist)[entries] == NULL) {
			// Handle malloc failure
			for (int i = 0; i < entries; i++) {
				free((*namelist)[i]);
			}
			free(*namelist);
			*namelist = NULL;
			closedir(dirhandle);
			return -1;
		}
		memcpy((*namelist)[entries++], dent, sizeof(struct dirent));
	}
	closedir(dirhandle);
	if (cmp)
		qsort(*namelist, entries, sizeof(struct dirent **), cmp);
	return entries - 1;
}
#endif 

void DirectoryList::Load(void)
{
	dirent **d;
	__select_path=&Path;
	entries=scandir(Path, &d, select_func, alphasort);
	contents=new DirEntryPack *[entries];
	for (int i=0; i<entries; i++) {
		contents[i]=new DirEntryPack(*d[i], Path);
		delete [] d[i];
	}
	delete [] d;
}

/****************************************************************************/

EntryPack *DirectoryList::GetEntry(int entry, int flags)
{
	DirEntryPack *e=contents[entry];
	e->Update(flags); // maybe the returned value could be used for stg...
	return e;
}
	
/****************************************************************************/
	
DirectoryList::~DirectoryList(void)
{
	for (int i=0; i<entries; i++)
		delete contents[i];
	delete [] contents;
	delete [] Path;
}
