#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>

#include <ddlib.h>
#include <ddcommon.h>

/*
	DayDream Door Library - written by Antti Häyrynen
 */

void writedm(struct dif *);

struct DayDream_Conference *__confdatas = 0;

struct dif *dd_initdoor(char *node)
{
	struct dif *mydif;
	struct sockaddr_un socknfo;
	char portname[80];

	mydif = (struct dif *) malloc(sizeof(struct dif));
	snprintf(portname, sizeof portname, "%s/dd_door%s", DDTMP, node);
	strlcpy(socknfo.sun_path, portname, sizeof(socknfo.sun_path));
	socknfo.sun_family = AF_UNIX;

	mydif->dif_sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (mydif->dif_sockfd == -1) {
		free(mydif);
		return 0;
	}
	if (connect (mydif->dif_sockfd, (struct sockaddr *) &socknfo,
		sizeof(socknfo)) == -1) {
		close(mydif->dif_sockfd);
		free(mydif);
		return 0;
	}
	signal(SIGHUP, SIG_IGN);

	return mydif;
}

void dd_close(struct dif *d)
{
	if (__confdatas) 
		free(__confdatas);
	d->dif_dm.ddm_command = 1;
	writedm(d);
}

void freecdatas(void)
{
	free(__confdatas);
}

struct DayDream_Conference *dd_getconfdata(void)
{
	struct stat fib;
	int datafd;
	char *s;
	if (!__confdatas) {
		char tbuf[1024];
		char *home;

		if ((home = getenv("DAYDREAM")) == NULL)
			return NULL;
		snprintf(tbuf, sizeof tbuf, "%s/data/conferences.dat",
			 home);
		if ((datafd = open(tbuf, O_RDONLY)) == -1)
			return NULL;

		fstat(datafd, &fib);

		__confdatas = (struct DayDream_Conference *) 
			malloc(fib.st_size + 2);
		read(datafd, __confdatas, fib.st_size);
		close(datafd);
		s = (char *) __confdatas;
		s[fib.st_size] = 255;
	}
	return __confdatas;
}

struct DayDream_Conference *dd_getconf(int cnum)
{
	struct DayDream_Conference *tconf;
	struct DayDream_MsgBase *tbase;
	int bcnt;

	if (!(tconf = dd_getconfdata()))
		return 0;

	for (;;) {
		if (tconf->CONF_NUMBER == 255) 
			return NULL;

		if (tconf->CONF_NUMBER == cnum) 
			return tconf;
		
		tbase = (struct DayDream_MsgBase *) tconf + 1;
		bcnt = tconf->CONF_MSGBASES;
		while (bcnt) {
			tbase++;
			bcnt--;
		}
		tconf = (struct DayDream_Conference *) tbase;
	}

}

struct DayDream_MsgBase *dd_getbase(int cnum, int bnum)
{
	struct DayDream_Conference *tconf;
	int bcnt;
	struct DayDream_MsgBase *tbase;

	tconf = dd_getconf(cnum);
	if (!tconf)
		return NULL;

	tbase = (struct DayDream_MsgBase *) tconf + 1;
	bcnt = tconf->CONF_MSGBASES;
	while (bcnt) {
		if (tbase->MSGBASE_NUMBER == bnum)
			return tbase;
		tbase++;
		bcnt--;
	}
	return NULL;
}

void dd_sendstring(struct dif *d, const char *str)
{
	while (strlen(str) > sizeof(d->dif_dm.ddm_string) - 1) {
		d->dif_dm.ddm_command = 2;
		strlcpy(d->dif_dm.ddm_string, str,
			sizeof(d->dif_dm.ddm_string));
		writedm(d);
		str = &str[sizeof(d->dif_dm.ddm_string) - 1];
	}
	d->dif_dm.ddm_command = 2;
	strlcpy(d->dif_dm.ddm_string, str, sizeof(d->dif_dm.ddm_string));
	writedm(d);
}

int dd_sendfmt(struct dif *d, const char *fmt, ...)
{
	char *buf;
	va_list args;
	int len;

	va_start(args, fmt);
	len = vasprintf(&buf, fmt, args);
	va_end(args);

	if (len == -1)
		return -1;

	dd_sendstring(d, buf);
	free(buf);

	return len;
}

int dd_prompt(struct dif *d, char *buffer, int len, int flags)
{
	strlcpy(d->dif_dm.ddm_string, buffer, sizeof d->dif_dm.ddm_string);
	d->dif_dm.ddm_data1 = len;
	d->dif_dm.ddm_data2 = flags;
	d->dif_dm.ddm_command = 3;
	writedm(d);
	strlcpy(buffer, d->dif_dm.ddm_string, len);
	return d->dif_dm.ddm_data1;
}

int dd_hotkey(struct dif *d, int flags)
{
	d->dif_dm.ddm_command = 4;
	d->dif_dm.ddm_data1 = flags;
	writedm(d);
	return d->dif_dm.ddm_data1;
}

int dd_typefile(struct dif *d, const char *text, int flags)
{
	d->dif_dm.ddm_command = 5;
	d->dif_dm.ddm_data1 = flags;
	strlcpy(d->dif_dm.ddm_string, text, sizeof d->dif_dm.ddm_string);
	writedm(d);
	return d->dif_dm.ddm_data3;
}

int dd_flagsingle(struct dif *d, char *file, int flags)
{
	d->dif_dm.ddm_command = 6;
	d->dif_dm.ddm_data1 = flags;
	strlcpy(d->dif_dm.ddm_string, file, sizeof d->dif_dm.ddm_string);
	writedm(d);
	return d->dif_dm.ddm_data1;
}

int dd_findusername(struct dif *d, char *user)
{
	d->dif_dm.ddm_command = 7;
	strlcpy(d->dif_dm.ddm_string, user, sizeof d->dif_dm.ddm_string);
	writedm(d);
	return d->dif_dm.ddm_data1;
}

int dd_system(struct dif *d, char *user, int mode)
{
	d->dif_dm.ddm_command = 8;
	d->dif_dm.ddm_data1 = mode;
	strlcpy(d->dif_dm.ddm_string, user, sizeof d->dif_dm.ddm_string);
	writedm(d);
	return 1;
}

int dd_docmd(struct dif *d, char *cmd)
{
	d->dif_dm.ddm_command = 9;
	strlcpy(d->dif_dm.ddm_string, cmd, sizeof d->dif_dm.ddm_string);
	writedm(d);
	return 1;
}

int dd_writelog(struct dif *d, char *cmd)
{
	d->dif_dm.ddm_command = 10;
	strlcpy(d->dif_dm.ddm_string, cmd, sizeof d->dif_dm.ddm_string);
	writedm(d);
	return 1;
}

int dd_changestatus(struct dif *d, char *stat)
{
	d->dif_dm.ddm_command = 11;
	strlcpy(d->dif_dm.ddm_string, stat, sizeof d->dif_dm.ddm_string);
	writedm(d);
	return 1;
}

void dd_pause(struct dif *d)
{
	d->dif_dm.ddm_command = 12;
	writedm(d);
}

int dd_joinconf(struct dif *d, int dc, int fl)
{
	d->dif_dm.ddm_command = 13;
	d->dif_dm.ddm_data1 = dc;
	d->dif_dm.ddm_data2 = fl;
	writedm(d);
	return d->dif_dm.ddm_data1;
}

int dd_isfreedl(struct dif *d, char *s)
{
	d->dif_dm.ddm_command = 14;
	strlcpy(d->dif_dm.ddm_string, s, sizeof d->dif_dm.ddm_string);
	writedm(d);
	return d->dif_dm.ddm_data1;
}

int dd_flagfile(struct dif *d, char *s, int i)
{
	d->dif_dm.ddm_command = 15;
	d->dif_dm.ddm_data1 = i;
	strlcpy(d->dif_dm.ddm_string, s, sizeof d->dif_dm.ddm_string);
	writedm(d);
	return d->dif_dm.ddm_data1;
}

void dd_getstrval(struct dif *d, char *buf, int val)
{
	d->dif_dm.ddm_command = val;
	d->dif_dm.ddm_data1 = 0;
	writedm(d);
	strcpy(buf, d->dif_dm.ddm_string);
}

void dd_getstrlval(struct dif *d, char *buf, size_t len, int val)
{
	d->dif_dm.ddm_command = val;
	d->dif_dm.ddm_data1 = 0;
	writedm(d);
	strlcpy(buf, d->dif_dm.ddm_string, len);
}

void dd_setstrval(struct dif *d, char *buf, int val)
{
	d->dif_dm.ddm_command = val;
	d->dif_dm.ddm_data1 = 1;
	strlcpy(d->dif_dm.ddm_string, buf, sizeof d->dif_dm.ddm_string);
	writedm(d);
}

int dd_getintval(struct dif *d, int val)
{
	d->dif_dm.ddm_command = val;
	d->dif_dm.ddm_data1 = 0;
	writedm(d);
	return d->dif_dm.ddm_data1;
}

void dd_setintval(struct dif *d, int val, int nw)
{
	d->dif_dm.ddm_command = val;
	d->dif_dm.ddm_data1 = 1;
	d->dif_dm.ddm_data2 = nw;
	writedm(d);
}

uint64_t dd_getlintval(struct dif *d, int val)
{
	d->dif_dm.ddm_command = val;
	d->dif_dm.ddm_data1 = 0;
	writedm(d);
	return d->dif_dm.ddm_ldata;
}

void dd_setlintval(struct dif *d, int val, uint64_t nw)
{
	d->dif_dm.ddm_command = val;
	d->dif_dm.ddm_data1 = 1;
	d->dif_dm.ddm_ldata = nw;
	writedm(d);
}

void dd_getlprs(struct dif *d, struct DayDream_LRP *lp)
{
	d->dif_dm.ddm_command = 16;
	writedm(d);
	lp->lrp_read = d->dif_dm.ddm_data1;
	lp->lrp_scan = d->dif_dm.ddm_data2;
}

void dd_setlprs(struct dif *d, struct DayDream_LRP *lp)
{
	d->dif_dm.ddm_command = 17;
	d->dif_dm.ddm_data1 = lp->lrp_read;
	d->dif_dm.ddm_data2 = lp->lrp_scan;
	writedm(d);
}

void writedm(struct dif *d)
{
	write(d->dif_sockfd, &d->dif_dm, sizeof(struct DayDream_DoorMsg));
	read(d->dif_sockfd, &d->dif_dm, sizeof(struct DayDream_DoorMsg));
}

int dd_isconfaccess(struct dif *d, int confn)
{
	d->dif_dm.ddm_command = 18;
	d->dif_dm.ddm_data1 = confn;
	writedm(d);
	return d->dif_dm.ddm_data1;
}

int dd_isanybasestagged(struct dif *d, int confa)
{
	d->dif_dm.ddm_command = 19;
	d->dif_dm.ddm_data1 = confa;
	writedm(d);
	return d->dif_dm.ddm_data1;
}

int dd_isconftagged(struct dif *d, int n)
{
	d->dif_dm.ddm_command = 20;
	d->dif_dm.ddm_data1 = n;
	writedm(d);
	return d->dif_dm.ddm_data1;
}

int dd_isbasetagged(struct dif *d, int n, int b)
{
	d->dif_dm.ddm_command = 21;
	d->dif_dm.ddm_data1 = n;
	d->dif_dm.ddm_data2 = b;
	writedm(d);
	return d->dif_dm.ddm_data1;
}

void dd_getmprs(struct dif *d, struct DayDream_MsgPointers *lp)
{
	d->dif_dm.ddm_command = 22;
	writedm(d);
	lp->msp_high = d->dif_dm.ddm_data1;
	lp->msp_low = d->dif_dm.ddm_data2;
}

void dd_setmprs(struct dif *d, struct DayDream_MsgPointers *lp)
{
	d->dif_dm.ddm_command = 23;
	d->dif_dm.ddm_data1 = lp->msp_high;
	d->dif_dm.ddm_data2 = lp->msp_low;
	writedm(d);
}

int dd_changemsgbase(struct dif *d, int base, int flags)
{
	d->dif_dm.ddm_command = 24;
	d->dif_dm.ddm_data1 = base;
	d->dif_dm.ddm_data2 = flags;
	writedm(d);
	return d->dif_dm.ddm_data1;
}

void dd_sendfiles(struct dif *d, char *flist)
{
	d->dif_dm.ddm_command = 25;
	strlcpy(d->dif_dm.ddm_string, flist, sizeof d->dif_dm.ddm_string);
	writedm(d);
	return;
}

void dd_getfiles(struct dif *d, char *path)
{
	d->dif_dm.ddm_command = 26;
	strlcpy(d->dif_dm.ddm_string, path, sizeof d->dif_dm.ddm_string);
	writedm(d);
}

/* dd_fileattach is a private command :) */

int dd_fileattach(struct dif *d)
{
	d->dif_dm.ddm_command = 27;
	writedm(d);
	return 1;
}

int dd_unflagfile(struct dif *d, char *s)
{
	d->dif_dm.ddm_command = 28;
	strlcpy(d->dif_dm.ddm_string, s, sizeof d->dif_dm.ddm_string);
	writedm(d);
	return d->dif_dm.ddm_data1;
}

int dd_findfilestolist(struct dif *d, char *file, char *list)
{
	if (strlen(file) > 48)
		return 0;

	d->dif_dm.ddm_command = 29;
	strlcpy(d->dif_dm.ddm_string, file, 48);
	strlcpy(&d->dif_dm.ddm_string[50], list,
		sizeof(d->dif_dm.ddm_string) - 50);
	writedm(d);
	return d->dif_dm.ddm_data1;
}

int dd_getfidounique(void)
{
	char *home;

	if ((home = getenv("DAYDREAM")) == NULL)
		return 0;

	return ddmsg_getfidounique(home);
}

int dd_isfiletagged(struct dif *d, char *str)
{
	d->dif_dm.ddm_command = 30;
	strlcpy(d->dif_dm.ddm_string, str, sizeof d->dif_dm.ddm_string);
	writedm(d);
	return d->dif_dm.ddm_data1;
}

int dd_dumpfilestofile(struct dif *d, char *str)
{
	d->dif_dm.ddm_command = 31;
	strlcpy(d->dif_dm.ddm_string, str, sizeof d->dif_dm.ddm_string);
	writedm(d);
	return d->dif_dm.ddm_data1;
}

// Ansi Forground Escape Sequences
// thanks to mercyful fate
void dd_ansi_fg(char *data, int fg) {

    switch (fg) {
        case 0:
            strcat(data, "\e[0;30m");
            break;
        case 1:
            strcat(data, "\e[0;34m");
            break;
        case 2:
            strcat(data, "\e[0;32m");
            break;
        case 3:
            strcat(data, "\e[0;36m");
            break;
        case 4:
            strcat(data, "\e[0;31m");
            break;
        case 5:
            strcat(data, "\e[0;35m");
            break;
        case 6:
            strcat(data, "\e[0;33m");
            break;
        case 7:
            strcat(data, "\e[0;37m");
            break;
        case 8:
            strcat(data, "\e[1;30m");
            break;
        case 9:
            strcat(data, "\e[1;34m");
            break;
        case 10:
            strcat(data, "\e[1;32m");
            break;
        case 11:
            strcat(data, "\e[1;36m");
            break;
        case 12:
            strcat(data, "\e[1;31m");
            break;
        case 13:
            strcat(data, "\e[1;35m");
            break;
        case 14:
            strcat(data, "\e[1;33m");
            break;
        case 15:
            strcat(data, "\e[1;37m");
            break;
        default :
            break;
    }
}

// Ansi Background Escape Sequences
// Thanks to mercyful fate
void dd_ansi_bg(char *data, int bg) {

    switch (bg) {
        case 16:
            strcat(data, "\e[40m");
            break;
        case 17:
            strcat(data, "\e[44m");
            break;
        case 18:
            strcat(data, "\e[42m");
            break;
        case 19:
            strcat(data, "\e[46m");
            break;
        case 20:
            strcat(data, "\e[41m");
            break;
        case 21:
            strcat(data, "\e[45m");
            break;
        case 22:
            strcat(data, "\e[43m");
            break;
        case 23:
            strcat(data, "\e[47m");
            break;
        // Default to none.
        case 24:
            strcat(data, "\e[0m");
            break;
        default : break;
    }
}

// dd_parsepipes, parses all |01 - |23 codes
// thanks to mercyful fate.  supposedly can use for whole files, not just a
// line of txt.
void dd_parsepipes(char* szString) {

    char szReplace[10]    = {0};  // Holds Ansi Sequence
    char *szStrBuilder;		  // Holds new String being built


    int size  = strlen(szString);   // get size of string we are parsing,
    int index = 0;	            // Index of Pipe Char in String
    char* pch;                      // Pointer to Pipe Char in String if found

    // Generic Counters
    int i = 0;
    int j = 0;

    // Search Initial String for Pipe Char, if not found exit, else continue
    pch = (char*) memchr (szString, '|', size);
  	if (pch != NULL)
  	{
 		// Calculate Index Position of | char found in string.
		index = pch-szString;
		//printf("Pipe Code found!: %d, size %d \r\n", index, size)
	}
	else
	{
	 	// No Pipe Code in String
		// no need to test further, so return
    	return;
	}

    // Setup Loop through String to test and replace pipe codes with
    // Ansi ESC Sequences.
    for (  ; index < size; index++)
	{
        // Make sure pipe can't possibly extend past the end of the string.
        // End of String reached, no possiable pipe code.
        if (index +2 >= size)
           return;
        // Test if Current Index is a Pipe Code.
        if (szString[index] == '|')
		{
            memset(&szReplace,0,sizeof(szReplace));
            //memset(&szStrBuilder,0,sizeof(szStrBuilder));
	    // Make Sure Both Chars after Pipe Code are Digits or Numbers.
            // Else Pass through and Ignore.
            if ( isdigit(szString[index+1]) && isdigit(szString[index+2]) )
			{
                switch (szString[index+1])
				{
                    case '0' : // Parse First Digit 0
                        switch (szString[index+2])
						{ // Parse Second Digit 0 - 9
                            case '0' : dd_ansi_fg(szReplace, 0); break;
                            case '1' : dd_ansi_fg(szReplace, 1); break;
                            case '2' : dd_ansi_fg(szReplace, 2); break;
                            case '3' : dd_ansi_fg(szReplace, 3); break;
                            case '4' : dd_ansi_fg(szReplace, 4); break;
                            case '5' : dd_ansi_fg(szReplace, 5); break;
                            case '6' : dd_ansi_fg(szReplace, 6); break;
                            case '7' : dd_ansi_fg(szReplace, 7); break;
                            case '8' : dd_ansi_fg(szReplace, 8); break;
                            case '9' : dd_ansi_fg(szReplace, 9); break;
                            default :
				break;
                        }
                        break;
                    case '1' : // Parse First Digit 1
                        switch (szString[index+2])
						{ // Parse Second Digit 10 - 19
                            case '0' : dd_ansi_fg(szReplace, 10); break;
                            case '1' : dd_ansi_fg(szReplace, 11); break;
                            case '2' : dd_ansi_fg(szReplace, 12); break;
                            case '3' : dd_ansi_fg(szReplace, 13); break;
                            case '4' : dd_ansi_fg(szReplace, 14); break;
                            case '5' : dd_ansi_fg(szReplace, 15); break;
                            case '6' : dd_ansi_bg(szReplace, 16); break;
                            case '7' : dd_ansi_bg(szReplace, 17); break;
                            case '8' : dd_ansi_bg(szReplace, 18); break;
                            case '9' : dd_ansi_bg(szReplace, 19); break;
                            default :
				break;
                        }
                        break;

                    case '2' : // Parse First Digit 2
                        switch (szString[index+2])
						{ // Parse Second Digit 20 - 23
                            case '0' : dd_ansi_bg(szReplace, 20); break;
                            case '1' : dd_ansi_bg(szReplace, 21); break;
                            case '2' : dd_ansi_bg(szReplace, 22); break;
                            case '3' : dd_ansi_bg(szReplace, 23); break;
                            default :
								break;
                        }
                    break;

                } // End Switch

                // Check if we matched an Ansi Sequence
                // If we did, Copy string up to, not including pipe,
                // Then con cat new ansi sequence to replace pipe in
		// our new string And copy remainder of string back
		// to run through Search for next pipe sequence.

                if (strcmp(szReplace,"") != 0)
                {
			szStrBuilder = (char *) calloc((size + ( strlen(szReplace) + 2 )), sizeof(char) );
			if (szStrBuilder == NULL)
			{
				return;
			}
              		// Sequence found.
			// 1. Copy String up to pipe into new string.
			for (i = 0; i < index; i++)
			szStrBuilder[i] = szString[i];

			// ConCat New Ansi Sequence into String
			strcat(szStrBuilder,szReplace);

			// Skip Past 2 Digits after pipe, and copy over remainder of 
			// String into new string so we have a complete string again.
			// used to be szStrBuilder)-1
			for (i = strlen(szStrBuilder), j = index+3; j < size; i++, j++)					
				szStrBuilder[i] = szString[j];

			// Not reaassign new string back to Original String.
			sprintf(szString,"%s",szStrBuilder);

			// Reset new size of string since ansi sequence is longer
			// Then a pipe code.

			free (szStrBuilder);
			szStrBuilder = NULL;
			size = strlen(szString);
			// then test again
			// if anymore pipe codes exists,  if they do repeat process.
		        pch = (char*) memchr (szString, '|', size);  
  			if (pch != NULL)
		  	{
		 		// Calculate Index Position of | char found in string.
				index = pch-szString;   
				--index; // Loop will incriment this so we need to set it down 1.
				//printf("Found second sequence ! index %d, size %d\r\n",index, size);	
			} else {
			 	//printf("Not Found second sequence ! \r\n");	
			 	// No Pipe Code in String 
				// no need to test further, so return
			    	return;
			}

                }
            }
        }
    } // End of For Loop, Finished.
}

// Thanks to rezine for this one
void dd_strippipes(char *tempmem)
{

	int go = 1;
	int s = 0;
	int t = 0;
	int u;

	while (tempmem[s] && go) {
		if (tempmem[s] == 124) {
			u = s;
			s = s + 1;
		     skiploop:
			if (s)
				goto go1;
			go = 0;
			goto gooff;
		     go1:
			if ((tempmem[s] >= '0') && (tempmem[s] <= '9'))
				goto go2;
			s++;
			goto skiploop;
		     go2:
			s++;
			if ((tempmem[s] >= '0') && (tempmem[s] <= '9'))
				goto go3;
			s = u;
			tempmem[t] = tempmem[s];
			t++;
		     go3:
			s++;
		     gooff:;
		} else {
			tempmem[t] = tempmem[s];
			t++;
			s++;
			if (tempmem[s] == 0)
				go = 0;
		}
	}
	tempmem[t] = 0;
}

// Thanks to rezine once again
void dd_stripansi(char *tempmem)
{

	int go = 1;
	int s = 0;
	int t = 0;
	int u;

	while (tempmem[s] && go) {
		if (tempmem[s] == 27) {
			u = s;
			s = s + 2;
		      skiploop:
			if (s)
				goto go1;
			go = 0;
			goto gooff;
		      go1:
			if (tempmem[s] >= 'A')
				goto go2;
			s++;
			goto skiploop;
		      go2:
			if (tempmem[s] == 'm')
				goto go3;
			s = u;
			tempmem[t] = tempmem[s];
			t++;
		      go3:
			s++;
		      gooff:;
		} else {
			tempmem[t] = tempmem[s];
			t++;
			s++;
			if (tempmem[s] == 0)
				go = 0;
		}
	}
	tempmem[t] = 0;
}


// Another from rezine
void dd_ansipos(struct dif *d,int x, int y)
{
	char tmpstring[10];
	sprintf(tmpstring,"\e[%i;%iH",y,x);
	dd_sendstring(d,tmpstring);
}


// you guessed it, rezine
void dd_clrscr(struct dif *d)
{
	dd_sendstring(d,"\e[2J");
}

// rezine strikes again
void dd_center(struct dif *d, char *string)
{
	int i;
	int len;

	len = dd_strlenansi(string);
	for(i=1;i<=40-len/2;i++)
	{
		dd_sendstring(d," ");
	}
	dd_sendstring(d,string);
	dd_sendstring(d,"\n");
}

// rezine again
int dd_strlenansi(char *string)
{
	int i,len,striplen=0;

	len=strlen(string);

	for(i=0;i<=len;i++)
	{
		if (string[i]==27)
		{
			striplen++;
			while (string[i]!='m')
			{
				striplen++;
				i++;
			}
		}
	}
	return(len-striplen);
}

// another hit from rezine
char *dd_stripcrlf(char *string)
{
	int i, len;

	len=strlen(string);
	for(i=len-3;i<=len;i++)
	{
		if(string[i]==10)
		{
			string[i]=0;
			break;
		}
	}
	return(string);
}

// Turn cursor off (good for litebars)
void dd_cursoroff(struct dif *d)
{
        dd_sendstring(d,"\e[?25l");
}

// Turn cursor on (good for litebars)
void dd_cursoron(struct dif *d)
{
        dd_sendstring(d,"\e[25h");
}
