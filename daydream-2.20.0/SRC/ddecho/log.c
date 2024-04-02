/* 
 * DD-Echo 
 * Logging routines
 * Author: Bo Simonsen <bo@geekworld.dk>
 */

#include "headers.h"

static FILE* flog;

void BeginLog(char* file)
{
	flog = fopen(file, "a");
	if(flog == NULL) {
		printf("Can't open logfile! %s\n", file);
		exit(0);
	}
	logit(FALSE, "!! %s started !!", version);
}

void EndLog(void)
{
	logit(FALSE, "!! %s ended !!\n", version);
	if(flog != NULL)
		fclose(flog);
}

void logit(bool print, char * format, ...)
{
	time_t tt;
	struct tm * st = NULL;
	va_list var_args;
	char string[2048];
	char string2[4096];
   
	if(flog == NULL)
		return;

	va_start(var_args, format);
	vsprintf(string, format, var_args);

	tt = time(NULL);
	st = localtime(&tt);
    
	StripCrLf(string);

	sprintf(string2, "%02d:%02d:%02d %02d/%02d-%02d  %s\n", st->tm_hour, 
																											 st->tm_min,
																											 st->tm_sec,
																											 st->tm_mday, 
																											 st->tm_mon+1,
																											 st->tm_year-100, 
																											 string);		      

 	if(print) {
		fputs(string, stdout);
		fputc('\n', stdout);
	}
  
	fputs(string2, flog);
	va_end(var_args);
}


