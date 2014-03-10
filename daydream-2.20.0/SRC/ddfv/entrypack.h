#ifndef _ENTRYPACK_H_INCLUDED
#define _ENTRYPACK_H_INCLUDED

#include <dirent.h>

class EntryPack {
 public:
	class Entry {
	 public:
		class SubEntry {
		 public:
			char *color;
			char *contents;
			SubEntry(char *, int, int, const char *, ...);
			~SubEntry(void);
			SubEntry(SubEntry const &s);
		};
		int entries;
		Entry(void);
		~Entry(void);
		SubEntry *operator [](int) const;
		void insert(SubEntry const &);
		Entry(Entry const &);
		
	 private:
		SubEntry **contents;
	};
	char *filename;
	int firstrow;	
	int shown;
	int entries;
	EntryPack(void);
	Entry *operator [](int) const;
	void insert(Entry const &);
	virtual ~EntryPack(void);
	EntryPack(EntryPack const &);
	virtual int Update(int) { return 0; };
	int modified(void);
	void reset_modify_flag(void);		
	
 private:
	Entry **contents;
	
 public:
	int modifyflag;
};

class FVEntryPack : public EntryPack {
 public:
	int dirflag;
	int tagged;
	FVEntryPack(void);
	void SetTagStatus(int);
	void ToggleTag(void);
};

class DirEntryPack : public FVEntryPack {
public:
	int oldflags;
	char *Path;

	// path is assumed to be valid during the lifespan
	// on an object of DirEntryPack class.
	DirEntryPack(dirent const &, char *);
	int Update(int);
};

class ListEntryPack : public FVEntryPack {
public:
        int date;
	int oldflags;
	

	ListEntryPack(char *, int);
	void AddLine(char *);
	char *GetFileName();
	int Update(int);
};

#endif /* _ENTRYPACK_H_INCLUDED */
