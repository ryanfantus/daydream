#ifndef _LIGHTBAR_H_INCLUDED
#define _LIGHTBAR_H_INCLUDED

class LightBar {
 private:
	char *BackgroundColor;
	char *Contents;
	int Row;
	
 public:
	LightBar(void);
	LightBar(LightBar const &);
	LightBar(char *, int, const char *, ...);
	void Set(char *, int, const char *, ...);
	void Print(void);
	~LightBar(void);
};

#endif /* _LIGHTBAR_H_INCLUDED */
