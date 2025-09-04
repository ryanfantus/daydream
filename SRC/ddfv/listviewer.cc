#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>

#include <common.h>
#include <libddc++.h>
#include <listviewer.h>
#include <ddcommon.h>

/****************************************************************************/

static int fallthru;
static char *titlestring="DayDream FileViewer v2.0";

void titlebar()
{
	dprintf("\e[0m\e[2J\e[H\e[0;44;37;1m\e[K\e[1;%dH%s",
		(80-strlen(titlestring))>>1, titlestring);
}

void help()
{	
	titlebar();	
	if (!helpfile||access(helpfile, R_OK))
		dprintf("\n\n\e[0m"
			"\e[32marrow down, arrow up\e[0m browse\n"
			"\e[32mpage down, page up\e[0m    -||-\n"
			"\e[32mplus, minus\e[0m          change area\n"
			"\e[32menter\e[0m                view file or packet's contents\n"
			"\e[32mdouble-esc or \"q\"\e[0m    back\n"
			"\e[32m\"t\"\e[0m                  tag files\n"
			"\e[32m\"1\"-\"9\"\e[0m              change max. length"
			" of descs\n"
			"\e[32m\"0\"\e[0m                  reload from the "
			"default value\n"
			"\n\e[31;1mpress any key to continue.");
	else {
		dprintf("\n\e[0m");
		dd_typefile(d, helpfile, TYPE_WARN);
	}
}

int ListViewer::Handler(void)
{
	int go=1;
	fallthru=0;
		
	while (go && !fallthru) {
		int key=HotKey(HOT_CURSOR);
		switch (key) {			
		case 'q':
		case 'Q':
		case 0x1b:
			go=0;
			break;
		case '-':
			if (current_area>1) {
				current_area--;
			        fallthru++;
			}
			break;
		case '+':
			if (current_area<GetConf()->CONF_FILEAREAS) {
				current_area++;
				fallthru++;
			}
			break;
		case 's':
		case 'S':
			sort_query();
			Print(P_REFRESH);
			break;
		case 'h':
		case 'H':
		case '?':
			help();
			HotKey();
			Print(P_REFRESH);
			break;
		default:
			if (!entries)
				break;
			
			if (HandleKeyboard(key)) {
//				GetEntry(selection, Flags(selection))->shown=E_HIDDEN;
				Print(P_UPDATE);
			}

			break;
		};
	}	       
	
	return fallthru;
}

void ListViewer::sort_query()
{
	int ch;
	dprintf("\e[0;36m\e[2;1H\e[Ksort by (nNdD)? ");
	ch=HotKey();
	switch (ch) {
	case 'n':
		sort(SORTKEY_NAME);
		break;
	case 'N':
		sort(SORTKEY_NAME|SORTKEY_ASCENDING);
		break;
	case 'd':
		sort(SORTKEY_DATE);
		break;
	case 'D':
		sort(SORTKEY_DATE|SORTKEY_ASCENDING);
		break;
	}
}
	
int ListViewer::HandleKeyboard(int ch)
{
	int handled=1;
	switch (ch) {
	case 13:
		if ((!SpawnNewViewer()) && !fallthru)
			Print(P_REFRESH);
		break;
	case 250:
		Up();
		break;
	case 251:
		Down();
		break;
	case 248:
		PageUp();
		break;
	case 249:
		PageDown();
		break;
	default:
		handled=0;
		break;
	}
	
	if (!handled) {
		if (ch>='0' && ch<='9') {
			int new_len=ch-'0';
			if (!new_len)
				new_len=GetIntVal(USER_FLINES);
			
			if (new_len == max_desc_len)
				return 0;

			max_desc_len=new_len;
			Print(P_REFRESH);
		}
	}
	
	return 0;
}

/****************************************************************************/

char *good_exts[]={
	"*.txt",
	"*.nfo",
	"*.diz",
	"read*.*",
	"*.ans",
	"*.asc",
	"*.gfx",
	NULL
};

int probably_text_file(char *fname)
{
	for (char **p=good_exts; *p!=NULL; p++) {
		if (wildcmp(fname, *p))
			return 1;
	}
	return 0;
}
		       
int ListViewer::SpawnNewViewer(void)
{
	int st;
	char *tmppath=new char[512];
	char *filename;
	
	FVEntryPack *entry=(FVEntryPack *)GetEntry(selection, Flags(selection));
	filename=entry->filename;
	
	if (entry->dirflag) {
		char *newtitle=new char[512];
		char *p=filename+strlen(filename);
		
		while (p>filename && p[-1]!='/')
			p--;
		
		sprintf(newtitle, "%s/%s", Title, p);
		
		DirectoryList dl(ScreenLength, entry->filename, newtitle);
		dl.Print(P_REFRESH);
		dl.Handler();
		
		delete [] newtitle;
		return 0;
	}
					
	if (probably_text_file(filename)) {
		dprintf("\e[0m\e[2J\e[H");
		dd_typefile(d, filename, TYPE_WARN);
		dprintf("\n\e[31;1mpress any key to continue\e[0m");
		if (dd_hotkey(d, 0)==-1) /* carrier lost */
			exit(1);
	} else if (!getarchiver(filename)) {
		delete [] tmppath;
		return 1;
	} else if ((st=extract_packet(filename, tmppath))) {
		if (st==EX_SUCCESS) {
			char *newtitle=new char[512];
			char *p=filename+strlen(filename);
			
			while (p>filename && p[-1]!='/')
				p--;
			
			sprintf(newtitle, "%s - %s", Title, p);
			
			DirectoryList dl(ScreenLength, tmppath, newtitle);
			dl.Print(P_REFRESH);
			dl.Handler();
			
			delete [] newtitle;
		}
		deldir(tmppath);
		rmdir(tmppath);
	}
	delete [] tmppath;
	return 0;
}

/****************************************************************************/

char *ListViewer::GetTitle(void)
{
	return NULL;
};

/****************************************************************************/

EntryPack *ListViewer::GetEntry(int a, int b)
{
	return NULL;
}

/****************************************************************************/

ListViewer::~ListViewer(void)
{
	delete [] Title;
}

/****************************************************************************/

int ListViewer::type()
{
	return 0;
}

/****************************************************************************/

int ListViewer::Visible(int new_start)
{
	if (new_start!=-1)
		start=new_start;
	Print(P_EXAMINE);
	return (GetEntry(selection, Flags(selection))->shown==E_FULLY);
}

/****************************************************************************/

void ListViewer::AlignToBottom(void)
{
	int cstart=selection;
	for (; cstart>0; cstart--) 
		if (!Visible(cstart-1))
			break;
	start=cstart;
}

/****************************************************************************/

void ListViewer::AlignToTop(void)
{
	start=selection;
	
	if (Print(P_EXAMINE)>0) {
		int cstart=start;
		int osel=selection;
		selection=entries-1;
		for (; cstart>0; cstart--)
			if (!Visible(cstart-1))
				break;
		start=cstart;
		selection=osel;
	}
	
	/* smarter behaviour would be nice */
}

/****************************************************************************/

void ListViewer::PageUp(void)
{
	int old_start=start;
	
	if (selection>0)
		selection--;
	AlignToBottom();
	selection=start;
	Print(old_start==start?P_UPDATE:P_REFRESH);
}

void ListViewer::PageDown(void)
{
	int old_start=start;
	
	for (; selection<entries-1; selection++) {
		if (!Visible())
			break;
	}
	AlignToTop();
	Print(old_start==start?P_UPDATE:P_REFRESH);
}

/****************************************************************************/

void ListViewer::Down(int count)
{
	selection+=count;
	if (selection>=entries)
		selection=entries-1;
	if (!Visible()) {
		AlignToTop();
		Print(P_REFRESH);
	} else Print(P_UPDATE);
}

/****************************************************************************/

void ListViewer::Up(int count)
{
	selection-=count;
	if (selection<0)
		selection=0;
	if (!Visible()) {
		AlignToBottom();
		Print(P_REFRESH);
	} else Print(P_UPDATE);
}
		
/****************************************************************************/
	
int ListViewer::Print(int mode)
{
	if (mode==P_REFRESH)
		ClearScreen();
	
	if (!entries) {
		if (mode==P_REFRESH)
			dprintf("\e[3;1H\e[0;31;1mThis %s contains no "
				"files.", type()?"area":"directory");
		return 0;
	}
	
        RowsLeft=ScreenLength-2;
	int CurrentRow=2;
	int CurrentItem=start;
	
	if (mode==P_EXAMINE||mode==P_REFRESH)
		ClearVisibilityFlags();
	
	while (RowsLeft) {
		if (CurrentItem>=entries)
			break;
		EntryPack *ep=GetEntry(CurrentItem, Flags(CurrentItem));
		int MaxRows=ep->entries;
		if (RowsLeft<MaxRows)
			MaxRows=RowsLeft;
		
		if (max_desc_len!=0)
			if (MaxRows>max_desc_len)
				MaxRows=max_desc_len;
		
		if (mode!=P_EXAMINE) {
			for (int i=0; i<MaxRows; i++) {
				if ((mode!=P_UPDATE)||ep->modified()) {
					ep->reset_modify_flag();
					EntryPack::Entry *e=(*ep)[i];
					dprintf("\e[%dH", i+CurrentRow+1);
					for (int j=0; j<e->entries; j++) {
						dprintf("%s%s", (*e)[j]->color,
							(*e)[j]->contents);
						if (mode==P_UPDATE) 
							break;
					}
					if (mode==P_UPDATE)
						break;
				}
			}
		}
		CurrentItem++;
		RowsLeft-=MaxRows;
		CurrentRow+=MaxRows;
		
		int tmp=ep->entries;
		if (tmp>max_desc_len)
			tmp=max_desc_len;
		
		ep->shown=tmp>MaxRows?E_PARTIAL:E_FULLY;
	}
	
	return RowsLeft;
}

/****************************************************************************/

int ListViewer::Flags(int n)
{
	return (n==selection)?L_SEL:0;
}

/****************************************************************************/

void ListViewer::ClearVisibilityFlags(void) 
{
	for (int i=0; i<entries; i++)
		GetEntry(i, Flags(i))->shown=E_HIDDEN;
}

/****************************************************************************/

void ListViewer::ClearScreen(void) 
{
	char *p=GetTitle();
	if (strlen(p)>80)
		p+=strlen(p)-80;
	

	titlebar();
//	LightBar Slogan("\e[0;44;37;1m", 0, "DayDream FileViewer v2.0");
	LightBar Title("\e[0;46;30m", 1, "%s", p);
//	dprintf("\e[0m\e[2J\e[H");
//	Slogan.Print();
	Title.Print();
}

/****************************************************************************/

ListViewer::ListViewer(int sl) : ScreenLength(sl), Title(NULL)
{
	start=0;
	selection=0;
	
	max_desc_len=GetIntVal(USER_FLINES);	
}

/****************************************************************************/
