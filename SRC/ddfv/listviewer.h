#ifndef _LISTVIEWER_H_INCLUDED
#define _LISTVIEWER_H_INCLUDED

#include "entrypack.h"
#include "lightbar.h"

class ListViewer {
 private:
	/* RowsLeft contains the number of unused rows after "blitting" */
	int RowsLeft;
	int ScreenLength;
	int start; 

	int max_desc_len;

	void Down(int=1);
	void Up(int=1);
	int Visible(int=-1);
	int SpawnNewViewer(void);
	
	void PageDown();
	void PageUp();
	
	void AlignToTop(void);
	void AlignToBottom(void);
	void ClearVisibilityFlags(void);
	void ClearScreen(void);
			
 protected:
	char *Title;
	int entries;
	int selection;	
	int *updated;
	
	int Flags(int);
	virtual int type();
	virtual int HandleKeyboard(int);
	
 public:
	int Print(int);

	int Handler(void);
	
	ListViewer(int);
	virtual char *GetTitle(void);

	virtual void sort(int)=0;
	void sort_query();
	
	virtual EntryPack *GetEntry(int, int);
	virtual ~ListViewer(void);
};

class DirectoryList : public ListViewer {
 private:
	char *Path;
	DirEntryPack **contents;
       		
	virtual int HandleKeyboard(int);
	
 public:
	DirectoryList(int, char *, char *);
	EntryPack *GetEntry(int, int);
	void Load(void);
	char *GetTitle(void);
	virtual ~DirectoryList(void);
	
	virtual void sort(int);
};

class FileList : public ListViewer {
 private:
	ListEntryPack **contents;       
	int type();
		
	virtual int HandleKeyboard(int);
	
 public:
	FileList(int, int);
	EntryPack *GetEntry(int, int);
	char *GetTitle(void);
	void insert(ListEntryPack const &);
	virtual ~FileList(void);
	
	virtual void sort(int);
};

enum {L_NORM=0, L_TAG=1L<<0, L_SEL=1L<<1};
enum {E_HIDDEN=0, E_PARTIAL=1, E_FULLY=2};
enum {P_EXAMINE=0, P_UPDATE=1, P_REFRESH=2};

enum {KEY_AREACHG=1};

enum {
	SORTKEY_NAME=0x01,
	SORTKEY_DATE=0x02,
	SORTKEY_ASCENDING=0x04
};

extern int current_area;

#endif /* _LISTVIEWER_H_INCLUDED */
