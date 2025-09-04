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
	char buffer[256];  // Increase buffer size for safety
	va_list args;
	va_start(args, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, args);
	va_end(args);
	Set(bckgnd, row, "%s", buffer);
	BackgroundColor=strdup(bckgnd);
	Row=row;
}

void LightBar::Set(char *bckgnd, int row, const char *fmt, ...) 
{
	char buffer[256];  // Increase buffer size for safety
	va_list args;
	va_start(args, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, args);
	va_end(args);
	Contents=strdup(buffer);
	BackgroundColor=strdup(bckgnd);
	Row=row;
}

void LightBar::Print(void) 
{
	if (!Contents || !BackgroundColor) {
		return;  // Prevent NULL pointer dereference
	}
	
	size_t content_len = strlen(Contents);
	int center_pos = content_len < 80 ? (80 - content_len) / 2 : 0;
	
	dprintf("\e[%dH%s\e[K\e[%dC%s", Row+1, BackgroundColor, 
		center_pos, Contents);
}

LightBar::~LightBar(void) 
{ 
	delete [] BackgroundColor;
	delete [] Contents;
}
