#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libddc++.h"
#include "lightbar.h"


LightBar::LightBar(void) 
{
	BackgroundColor=NULL;
	Contents=NULL; 
}

LightBar::LightBar(LightBar const &l)
{
	if (Contents)
		Contents=strdup(l.Contents);
	if (BackgroundColor)
		BackgroundColor=strdup(l.BackgroundColor);
}

LightBar::LightBar(char *bckgnd, int row, const char *fmt, ...) 
{
	char buffer[80];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buffer, 80, fmt, args);
	va_end(args);
	Set(bckgnd, row, "%s", buffer);
	BackgroundColor=strdup(bckgnd);
	Row=row;
}

void LightBar::Set(char *bckgnd, int row, const char *fmt, ...) 
{
	char buffer[80];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buffer, 80, fmt, args);
	va_end(args);
	Contents=strdup(buffer);
	BackgroundColor=strdup(bckgnd);
	Row=row;
}

void LightBar::Print(void) 
{
	dprintf("\e[%dH%s\e[K\e[%dC%s", Row+1, BackgroundColor, 
		(80-strlen(Contents))/2, Contents);
}

LightBar::~LightBar(void) 
{ 
	delete [] BackgroundColor;
	delete [] Contents;
}
