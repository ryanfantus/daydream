#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include "listviewer.h"
#include "entrypack.h"
#include "common.h"

DirEntryPack::DirEntryPack(dirent const &de, char *path) : FVEntryPack(), oldflags(-1), Path(path)
{
	Entry e;
	struct stat st;
	char buffer[256];
	
	// Use snprintf for safe buffer handling
	int result = snprintf(buffer, sizeof(buffer), "%s/%s", path, de.d_name);
	if (result >= (int)sizeof(buffer)) {
		// Path too long, handle error gracefully
		filename = NULL;
		dirflag = 0;
		return;
	}
	
	filename=strdup(buffer);
	if (stat(buffer, &st) != 0) {
		// stat failed, handle gracefully
		dirflag = 0;
		return;
	}
	dirflag=S_ISDIR(st.st_mode);
	
	e.insert(Entry::SubEntry((char *)NULL, 0, 0, "%s", (char *)de.d_name));
	
	if (dirflag) 
		e.insert(Entry::SubEntry(stdcolor, 0, 36, "%12s", "<directory>"));
	else e.insert(Entry::SubEntry(stdcolor, 0, 36, "%12d", st.st_size));
	insert(e);
	
};

/****************************************************************************/

int DirEntryPack::Update(int flags) 
{
	(*(*this)[0])[0]->color=DirColors[0][dirflag?1:0][flags&L_SEL?1:0];	
	if (flags==oldflags) 
		return 0;       
	oldflags=flags;
	modifyflag=1;
	return 1;
}
