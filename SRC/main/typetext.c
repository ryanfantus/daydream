#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <syslog.h>

#include <daydream.h>
#include <ddcommon.h>
#include <symtab.h>
#include <console.h>

enum {
	fname_sec = 0x01,
	fname_raw = 0x02,
	fname_ext_gfx = 0x04,
	fname_ext_txt = 0x08,
	fname_ext = fname_ext_gfx | fname_ext_txt,
	fname_dpath = 0x10,
	fname_cpath = 0x20,
	fname_opath = 0x40,
	fname_cnum = 0x80,
	fname_mpath = 0x100
};

struct name_component {
	int type;
	char *ptr;
};

static int dotype(char *file, int flags);
static void dump_buffer(const char *, size_t, int *)
	__attr_bounded__ (__string__, 1, 2);
static int type_random_file(const char *, size_t, const char *, size_t)
	__attr_bounded__ (__string__, 1, 2);

static int doconv = 1;

static char ddprintf_buffer[4096];

int ddprintf(const char *fmt,...)
{
	int chars;

	va_list args;
	va_start(args, fmt);
	chars = vsnprintf(ddprintf_buffer, 4096, fmt, args);
	va_end(args);

	DDPut(ddprintf_buffer);
	return chars;
}

void ddput(const char *typeme, size_t len)
{
	unsigned char *tempmem;
	size_t i = 0;
	int k = 0;
	int s = 0;
	int u;
	int go = 1;

	if (bgmode)
		return;

	if (typeme == NULL)
		return;

	if (bgmode)
		return;

	if (*typeme == 1) {
		typeme++;
		TypeFile(typeme, TYPE_MAKE | TYPE_WARN | TYPE_CONF);
		return;
	}
	       
	tempmem = (unsigned char *) xmalloc(len * 2 + 1);

	for (i = 0; i < len; i++) {
		if (typeme[i] == 10) {
			tempmem[k] = 13;
			tempmem[k + 1] = 10;
			k = k + 2;
		} else {
			tempmem[k] = typeme[i];
			k++;
		}
	}

	tempmem[k] = 0;

	if (!ansi) {
		k = 0;
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
				tempmem[k] = tempmem[s];
				k++;
			      go3:
				s++;
			      gooff:;
			} else {
				tempmem[k] = tempmem[s];
				k++;
				s++;
				if (tempmem[s] == 0)
					go = 0;
			}
		}
		tempmem[k] = 0;
	}

	if (console_active())
		console_putsn(tempmem, k);

	if (doconv && display && display->DISPLAY_ATTRIBUTES & (1L << 2)) {
		unsigned char *s;
		s = tempmem;
		while (*s) {
			*s = outconvtab[(unsigned char) *s];
			s++;
		}
	}

	if (userinput && checkcarrier() && !lmode)
		safe_write(serhandle, tempmem, k);

	free(tempmem);
}

void DDPut(const char *s)
{
	ddput(s, strlen(s));
}


// Ansi Forground Escape Sequences
void ansi_fg(char *data, int fg) {

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
void ansi_bg(char *data, int bg) {

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

// Parsepipes, parses all |01 - |23 codes, pcboard codes, wildcat, wwiv
void parsepipes(char* szString) {

    char szReplace[20]    = {0};  // Holds Ansi Sequence
    char *szStrBuilder;		  // Holds new String being built


    int size  = strlen(szString);   // get size of string we are parsing,
    int index = 0;	            // Index of Pipe Char in String
    char* pch;                      // Pointer to Pipe Char in String if found

    // Generic Counters
    int i = 0;
    int j = 0;

    // Search Initial String for color codes (|, @X, and WWIV), if not found exit, else continue
    char *pch_pipe = (char*) memchr (szString, '|', size);
    char *pch_at = (char*) memchr (szString, '@', size);
    char *pch_wwiv = (char*) memchr (szString, 3, size);  // ASCII 3 is Control-C
    
    if (pch_pipe && pch_at && pch_wwiv) {
        pch = (pch_pipe < pch_at) ? pch_pipe : pch_at;
        pch = (pch < pch_wwiv) ? pch : pch_wwiv;
    } else if (pch_pipe && pch_at) {
        pch = (pch_pipe < pch_at) ? pch_pipe : pch_at;
    } else if (pch_pipe && pch_wwiv) {
        pch = (pch_pipe < pch_wwiv) ? pch_pipe : pch_wwiv;
    } else if (pch_at && pch_wwiv) {
        pch = (pch_at < pch_wwiv) ? pch_at : pch_wwiv;
    } else if (pch_pipe) {
        pch = pch_pipe;
    } else if (pch_at) {
        pch = pch_at;
    } else if (pch_wwiv) {
        pch = pch_wwiv;
    } else {
        pch = NULL;
    }
    
  	if (pch != NULL)
  	{
 		// Calculate Index Position of color code char found in string.
		index = pch-szString;
		//printf("Color Code found!: %d, size %d \r\n", index, size)
	}
	else
	{
	 	// No color codes in String
		// no need to test further, so return
    	return;
	}

    // Setup Loop through String to test and replace pipe codes with
    // Ansi ESC Sequences.
    for (  ; index < size; index++)
	{
        // Make sure color codes can't possibly extend past the end of the string.
        // Check for minimum length needed for pipe codes (3 chars), @X codes (4 chars), or WWIV codes (2 chars)
        if ((szString[index] == '|' && index + 2 >= size) ||
            (szString[index] == '@' && index + 3 >= size) ||
            (szString[index] == 3 && index + 2 >= size))
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
                            case '0' : ansi_fg(szReplace, 0); break;
                            case '1' : ansi_fg(szReplace, 1); break;
                            case '2' : ansi_fg(szReplace, 2); break;
                            case '3' : ansi_fg(szReplace, 3); break;
                            case '4' : ansi_fg(szReplace, 4); break;
                            case '5' : ansi_fg(szReplace, 5); break;
                            case '6' : ansi_fg(szReplace, 6); break;
                            case '7' : ansi_fg(szReplace, 7); break;
                            case '8' : ansi_fg(szReplace, 8); break;
                            case '9' : ansi_fg(szReplace, 9); break;
                            default :
				break;
                        }
                        break;
                    case '1' : // Parse First Digit 1
                        switch (szString[index+2])
						{ // Parse Second Digit 10 - 19
                            case '0' : ansi_fg(szReplace, 10); break;
                            case '1' : ansi_fg(szReplace, 11); break;
                            case '2' : ansi_fg(szReplace, 12); break;
                            case '3' : ansi_fg(szReplace, 13); break;
                            case '4' : ansi_fg(szReplace, 14); break;
                            case '5' : ansi_fg(szReplace, 15); break;
                            case '6' : ansi_bg(szReplace, 16); break;
                            case '7' : ansi_bg(szReplace, 17); break;
                            case '8' : ansi_bg(szReplace, 18); break;
                            case '9' : ansi_bg(szReplace, 19); break;
                            default :
				break;
                        }
                        break;

                    case '2' : // Parse First Digit 2
                        switch (szString[index+2])
						{ // Parse Second Digit 20 - 23
                            case '0' : ansi_bg(szReplace, 20); break;
                            case '1' : ansi_bg(szReplace, 21); break;
                            case '2' : ansi_bg(szReplace, 22); break;
                            case '3' : ansi_bg(szReplace, 23); break;
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
			szStrBuilder = (char *) calloc((size + strlen(szReplace) + 50), sizeof(char) );
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
			
			// Ensure null termination
			szStrBuilder[i] = '\0';

			// Reassign new string back to Original String.
			strcpy(szString, szStrBuilder);

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
        if (szString[index] == '@' && szString[index + 1] == 'X')
		{
            memset(&szReplace,0,sizeof(szReplace));
            //memset(&szStrBuilder,0,sizeof(szStrBuilder));
	    // Make Sure Both Chars after Pipe Code are Digits or Numbers.
            // Else Pass through and Ignore.
            if ( isxdigit(szString[index+2]) && isxdigit(szString[index+3]) )
			{
                // Parse PCBoard @X hex color codes: @XBF where B=background, F=foreground
                int bg_hex, fg_hex;
                char bg_char = tolower(szString[index+2]);
                char fg_char = tolower(szString[index+3]);
                
                // Convert hex characters to values
                if (bg_char >= '0' && bg_char <= '9') bg_hex = bg_char - '0';
                else if (bg_char >= 'a' && bg_char <= 'f') bg_hex = bg_char - 'a' + 10;
                else break;
                
                if (fg_char >= '0' && fg_char <= '9') fg_hex = fg_char - '0';
                else if (fg_char >= 'a' && fg_char <= 'f') fg_hex = fg_char - 'a' + 10;
                else break;
                
                // Generate ANSI sequence for PCBoard colors
                // Validate color values are in range (0-15 for fg, 0-7 for bg)
                if (fg_hex > 15 || bg_hex > 7) break;
                
                if (fg_hex > 7) {
                    snprintf(szReplace, sizeof(szReplace), "\e[1;%d;%dm", 30 + (fg_hex - 8), 40 + bg_hex);
                } else {
                    snprintf(szReplace, sizeof(szReplace), "\e[0;%d;%dm", 30 + fg_hex, 40 + bg_hex);
                }

                // Check if we matched an Ansi Sequence
                // If we did, Copy string up to, not including pipe,
                // Then con cat new ansi sequence to replace pipe in
		// our new string And copy remainder of string back
		// to run through Search for next pipe sequence.

                if (strcmp(szReplace,"") != 0)
                {
			szStrBuilder = (char *) calloc((size + strlen(szReplace) + 50), sizeof(char) );
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

			// Skip Past 3 chars after @X, and copy over remainder of 
			// String into new string so we have a complete string again.
			// used to be szStrBuilder)-1
			for (i = strlen(szStrBuilder), j = index+4; j < size; i++, j++)					
				szStrBuilder[i] = szString[j];
			
			// Ensure null termination
			szStrBuilder[i] = '\0';

			// Reassign new string back to Original String.
			strcpy(szString, szStrBuilder);

			// Reset new size of string since ansi sequence is longer
			// Then a pipe code.

			free (szStrBuilder);
			szStrBuilder = NULL;
			size = strlen(szString);
			// then test again
			// if anymore pipe codes exists,  if they do repeat process.
		        // Look for both pipe codes and @X codes
		        char *pch_pipe = (char*) memchr (szString, '|', size);
		        char *pch_at = (char*) memchr (szString, '@', size);
		        
		        if (pch_pipe && pch_at) {
		            pch = (pch_pipe < pch_at) ? pch_pipe : pch_at;
		        } else if (pch_pipe) {
		            pch = pch_pipe;
		        } else if (pch_at) {
		            pch = pch_at;
		        } else {
		            pch = NULL;
		        }
		        
  			if (pch != NULL)
		  	{
		 		// Calculate Index Position of color code char found in string.
				index = pch-szString;   
				--index; // Loop will incriment this so we need to set it down 1.
				//printf("Found second sequence ! index %d, size %d\r\n",index, size);	
			} else {
			 	//printf("Not Found second sequence ! \r\n");	
			 	// No color codes in String 
				// no need to test further, so return
			    	return;
			}

                }
            }
        }
        // Test if Current Index is a WWIV Color Code (Control-C + digit)
        if (szString[index] == 3)  // ASCII 3 is Control-C
		{
            memset(&szReplace,0,sizeof(szReplace));
            
            // Make sure the next character is a digit
            if (isdigit(szString[index+1]))
			{
                // Parse WWIV color codes
                switch (szString[index+1])
				{
                    case '0': // Normal
                        strcpy(szReplace, "\e[0m");
                        break;
                    case '1': // High Intensity Cyan
                        strcpy(szReplace, "\e[1;36m");
                        break;
                    case '2': // High Intensity Yellow
                        strcpy(szReplace, "\e[1;33m");
                        break;
                    case '3': // Normal Magenta
                        strcpy(szReplace, "\e[0;35m");
                        break;
                    case '4': // High Intensity White with Blue Background
                        strcpy(szReplace, "\e[1;37;44m");
                        break;
                    case '5': // Normal Green
                        strcpy(szReplace, "\e[0;32m");
                        break;
                    case '6': // High Intensity Blinking Red
                        strcpy(szReplace, "\e[1;5;31m");
                        break;
                    case '7': // High Intensity Blue
                        strcpy(szReplace, "\e[1;34m");
                        break;
                    case '8': // Low Intensity Blue
                        strcpy(szReplace, "\e[0;34m");
                        break;
                    case '9': // Low Intensity Cyan
                        strcpy(szReplace, "\e[0;36m");
                        break;
                    default:
                        break;
                }

                // Check if we matched an ANSI Sequence
                if (strcmp(szReplace,"") != 0)
                {
			szStrBuilder = (char *) calloc((size + strlen(szReplace) + 50), sizeof(char) );
			if (szStrBuilder == NULL)
			{
				return;
			}
              		// Sequence found.
			// 1. Copy String up to Control-C into new string.
			for (i = 0; i < index; i++)
			szStrBuilder[i] = szString[i];

			// ConCat New Ansi Sequence into String
			strcat(szStrBuilder,szReplace);

			// Skip Past 1 digit after Control-C, and copy over remainder of 
			// String into new string so we have a complete string again.
			for (i = strlen(szStrBuilder), j = index+2; j < size; i++, j++)					
				szStrBuilder[i] = szString[j];
			
			// Ensure null termination
			szStrBuilder[i] = '\0';

			// Reassign new string back to Original String.
			strcpy(szString, szStrBuilder);

			// Reset new size of string since ansi sequence is longer
			// Then a Control-C code.

			free (szStrBuilder);
			szStrBuilder = NULL;
			size = strlen(szString);
			// then test again
			// if anymore color codes exist, if they do repeat process.
		        // Look for pipe codes, @X codes, and WWIV codes
		        char *pch_pipe = (char*) memchr (szString, '|', size);
		        char *pch_at = (char*) memchr (szString, '@', size);
		        char *pch_wwiv = (char*) memchr (szString, 3, size);
		        
		        if (pch_pipe && pch_at && pch_wwiv) {
		            pch = (pch_pipe < pch_at) ? pch_pipe : pch_at;
		            pch = (pch < pch_wwiv) ? pch : pch_wwiv;
		        } else if (pch_pipe && pch_at) {
		            pch = (pch_pipe < pch_at) ? pch_pipe : pch_at;
		        } else if (pch_pipe && pch_wwiv) {
		            pch = (pch_pipe < pch_wwiv) ? pch_pipe : pch_wwiv;
		        } else if (pch_at && pch_wwiv) {
		            pch = (pch_at < pch_wwiv) ? pch_at : pch_wwiv;
		        } else if (pch_pipe) {
		            pch = pch_pipe;
		        } else if (pch_at) {
		            pch = pch_at;
		        } else if (pch_wwiv) {
		            pch = pch_wwiv;
		        } else {
		            pch = NULL;
		        }
		        
  			if (pch != NULL)
		  	{
		 		// Calculate Index Position of color code char found in string.
				index = pch-szString;   
				--index; // Loop will incriment this so we need to set it down 1.
				//printf("Found second sequence ! index %d, size %d\r\n",index, size);	
			} else {
			 	//printf("Not Found second sequence ! \r\n");	
			 	// No color codes in String 
				// no need to test further, so return
			    	return;
			}

                }
            }
        }
    } // End of For Loop, Finished.
}

void strippipes(char *tempmem)
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

void stripansi(char *tempmem)
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



static char *compose_name(list_t *name_comps, int omit)
{
	char tmpstr[64];
	string_t *name = strnew();

	while (name_comps) {
		struct name_component *comp =
			car(struct name_component *, name_comps);
		name_comps = cdr(name_comps);
		switch (comp->type & ~omit) {
		case 0:
			break;
		case fname_sec:
			snprintf(tmpstr, sizeof tmpstr, 
				".%d", user.user_securitylevel);
			name = strappend(name, tmpstr);
			break;
		case fname_raw:
			name = strappend(name, comp->ptr);
			break;
		case fname_ext_gfx:
			name = strappend(name, ".gfx");
			break;
		case fname_ext_txt:
			name = strappend(name, ".txt");
			break;		     
		case fname_dpath:
			name = strappend(name, display->DISPLAY_PATH);
			name = strappend(name, "/");
			break;
		case fname_cpath:
			name = strappend(name,
					 conference()->conf.CONF_PATH);
			name = strappend(name, "/");
			break;
		case fname_opath:
			name = strappend(name, origdir);
			break;
		case fname_mpath:
			name = strappend(name, origdir);
			name = strappend(name, "/menu/");
			break;
		case fname_cnum:
			snprintf(tmpstr, sizeof tmpstr, ".%d", 
				conference()->conf.CONF_NUMBER);
			break;
		default:
			strfree(name, 1);
			return NULL;
		}
	}
	
	return strfree(name, 0);
}

static int hascomp(list_t *name_comps, int type)
{
	while (name_comps) {
		if (car(struct name_component *, name_comps)->type & type)
			break;
		name_comps = cdr(name_comps);
	}
	return (int) name_comps;
}
	
static int find_and_type_file(list_t *name_comps, int flags)
{
	int i, trythese[][2] = {
		{ fname_sec | fname_ext, fname_ext_txt },
		{ fname_sec | fname_ext, fname_ext_gfx },
		{ fname_ext, fname_ext_txt | fname_sec },
		{ fname_ext, fname_ext_gfx | fname_sec },
		{ fname_sec, 0 },
		{ 0, fname_sec }
	};
	
	for (i = 0; i < 6; i++) {
		char *filename;
		
		int flag = trythese[i][0];
		
		if ((flag & fname_ext) && !hascomp(name_comps, fname_ext))
			continue;
		    
		if ((flag & fname_sec) && !hascomp(name_comps, fname_sec))
			continue;
		
		if (!ansi && (flag & fname_ext_gfx & trythese[i][1]))
			continue;
		
		if (display && display->DISPLAY_ATTRIBUTES & (1L << 5) &&
		    ansi && (flag & fname_ext_txt)) 
			continue;
		
		filename = compose_name(name_comps, trythese[i][1]);
		if (!filename)
			continue;
		
		if (dotype(filename, flags)) {
			free(filename);
			return 1;
		}
				
		free(filename);
	}
		
	return 0;
}
				
static int typefile(const char *filename, int flags)
{
	int retcode = 0;
	list_t *name_comps = NULL;
	
	while (*filename) {
		struct name_component *comp = (struct name_component *) 
			xmalloc(sizeof(struct name_component));
		const char *ptr = filename;
		
		if (*filename == '%' && filename[1] != '%') {
			switch (*++filename) {
			case 'c':
				comp->type = fname_cpath;
				break;
			case 's':
				comp->type = fname_sec;
				break;
			case 'e':
				comp->type = fname_ext;
				break;
			case 'd':
				comp->type = fname_dpath;
				break;
			case 'o':
				comp->type = fname_opath;
				break;
			case 'n':
				comp->type = fname_cnum;
				break;
			case 'm':
				comp->type = fname_mpath;
				break;
			default:
				goto bailout;
			}
			filename++;
		} else {
			while (*ptr) {
				if (*ptr == '%' && ptr[1] != '%')
					break;
				else
					ptr++;
			}
			comp->type = fname_raw;
			comp->ptr = (char *) xmalloc(ptr - filename + 1);
			comp->ptr[ptr - filename] = 0;
			strncpy(comp->ptr, filename, ptr - filename);
			filename = ptr;
		}
		cons(name_comps, comp);
	}
	
	retcode = find_and_type_file(name_comps, flags);
	
 bailout:
	while (name_comps) {
		struct name_component *comp =
			shift(struct name_component *, name_comps);
		if (comp->type == fname_raw)
			free(comp->ptr);
		free(comp);
	}
	return retcode;
}		

int TypeFile(const char *typethis, int flags)
{	
	char buffer[PATH_MAX];
	const char *sec = "";
	
	if (flags & TYPE_SEC)
		sec = "%s";
	
	if (flags & TYPE_CONF) {
		if (ssnprintf(buffer, "%%cdisplay/%%d%s%s%%e", typethis, sec))
			return 0;
		if (typefile(buffer, flags))
			return 1;
	}
		
	if (flags & TYPE_MAKE) {
		if (ssnprintf(buffer, "%%o/display/%%d%s%s%%e", typethis, sec))
			return 0;
		if (typefile(buffer, flags))
			return 1;
	}

	if (!(flags & (TYPE_CONF | TYPE_MAKE)))
		if (typefile(typethis, flags))
			return 1;
	
	if (flags & TYPE_WARN) 
		ddprintf(sd[missingtextstr], typethis);

	syslog(LOG_ERR, "cannot open %.200s", typethis);
	return 0;
}

static int dotype_more_prompt(int *sl)
{
	int hot;
	
	if (--*sl)
		return 0;
	
	DDPut(sd[morepromptstr]);
	hot = HotKey(0);
	DDPut("\r                                                         \r");
	if (hot == 'N' || hot == 'n' || hot == 'q' || hot == 'Q')
		return -1;
	if (hot == 'C' || hot == 'c') 
		*sl = -1;
	else 
		*sl = user.user_screenlength - 1;
	
	return 0;
}

static int istoken(const char **ptr, const char *token)
{
	if (toupper((*ptr)[0]) == token[0] && toupper((*ptr)[1]) == token[1]) {
		*ptr = *ptr + 2;
		return 1;
	} else
		return 0;
}

static void dump_buffer(const char *s, size_t length, int *screen_length)
{
	while (length) {
		const char *linefeed;

		if ((linefeed = memchr(s, 0x0a, length)) == NULL) {
			ddput(s, length);
			return;
		}
		
		ddput(s, linefeed - s + 1);
		length -= linefeed - s + 1;
		s = linefeed + 1;

		if (dotype_more_prompt(screen_length))
			return;
	}
}

static const char *strsep2(const char *start, const char *end, char ch)
{
	for (; start < end && *start != ch; start++);
	return start;
}

static int type_random_file(const char *prefix, size_t prefixlen,
	const char *range, size_t rangelen)
{
	char tmp[16];
	char filename[PATH_MAX];
	int suffix;

	if (rangelen >= sizeof tmp) 
		return -1;
	if (prefixlen >= sizeof filename)
		return -1;

	memmove(tmp, range, rangelen);
	tmp[rangelen] = '\0';

	if ((suffix = str2uint(tmp, 1, 1UL << 24)) == -1)
		return -1;
	suffix = (rand() % suffix) + 1;
	if (ssnprintf(tmp, "%d", suffix))
		return -1;
	memmove(filename, prefix, prefixlen);
	filename[prefixlen] = '\0';
	if (strlcat(filename, tmp, sizeof filename) >= sizeof(filename))
		return -1;

	return TypeFile(filename, TYPE_WARN) ? 0 : -1;
}

int formatted_print(const char *buffer, size_t length, int flags)
{
	int olda;
	int nocodes = 0;
	int sl, bleft;
	char tempbuf[512];
	const char *ptr, *cmdchar;
		
	if (flags & TYPE_NOCODES)
		nocodes = 1;
	
	if (onlinestat)
		sl = user.user_screenlength - 1;
	else
		sl = -1;
	
		olda = ansi;
	if (flags & TYPE_NOSTRIP) {
		ansi = 1;
	}
	       
	for (ptr = buffer; ptr - buffer < length; doconv = 1) {
		if (nocodes)
			cmdchar = buffer + length;
		else
			cmdchar = strsep2(ptr, buffer + length, '~');

		if (display && !(display->DISPLAY_ATTRIBUTES & (1L << 3))) 
			doconv = 0;	

		dump_buffer(ptr, cmdchar - ptr, &sl);
		if (cmdchar == buffer + length) 
			break;

		bleft = buffer + length - cmdchar;
		/* no control codes fit in 3 bytes or less */
		if (bleft <= 3) {
			ddput(cmdchar, buffer + length - cmdchar);
			break;
		}
	
		if (cmdchar[1] != '#') {
			ddput("~", 1);
			ptr = cmdchar + 1;
			continue;
		}

		bleft -= 2;
		cmdchar += 2;		
		
		if (istoken(&cmdchar, "PA")) {
			HotKey(0);
		} else if (istoken(&cmdchar, "RE")) {
			DDPut(sd[pausestr]);
			HotKey(0);
		} else if (istoken(&cmdchar, "RN")) {
			DDPut(user.user_realname);
		} else if (istoken(&cmdchar, "HA")) {
			DDPut(user.user_handle);
		} else if (istoken(&cmdchar, "OR")) {
			DDPut(user.user_organization);
		} else if (istoken(&cmdchar, "LO")) {
			DDPut(user.user_zipcity);			
		} else if (istoken(&cmdchar, "PH")) {
			DDPut(user.user_voicephone);
		} else if (istoken(&cmdchar, "SL")) {
			ddprintf("%d", user.user_screenlength);
		} else if (istoken(&cmdchar, "PR")) {
			if (protocol)
				DDPut(protocol->PROTOCOL_NAME);
		} else if (istoken(&cmdchar, "SI")) {
			DDPut(user.user_signature);
		} else if (istoken(&cmdchar, "CM")) {
			DDPut(user.user_computermodel);
		} else if (istoken(&cmdchar, "DA")) {
			time_t aika;
			char *aikas;
			
			aika = time(0);
			aikas = ctime(&aika);
			aikas[24] = 0;
			DDPut(aikas);
		} else if (istoken(&cmdchar, "ID")) {
			ddprintf("%d", user.user_account_id);
		} else if (istoken(&cmdchar, "DT")) {
			ddprintf("%d", user.user_dailytimelimit);
		} else if (istoken(&cmdchar, "FC")) {
			ddprintf("%24.24s", ctime(&user.user_firstcall));
		} else if (istoken(&cmdchar, "LC")) {
			ddprintf("%24.24s", ctime(&user.user_lastcall));
		} else if (istoken(&cmdchar, "OL")) {
			ddprintf("%d", bpsrate);
		} else if (istoken(&cmdchar, "SE")) {
			ddprintf("%d", user.user_securitylevel);
		} else if (istoken(&cmdchar, "UB")) {
			ddprintf("%Lu", user.user_ulbytes);
		} else if (istoken(&cmdchar, "UF")) {
			ddprintf("%d", user.user_ulfiles);
		} else if (istoken(&cmdchar, "ML")) {
			if (user.user_flines)
				ddprintf("%d", user.user_flines);
			else
				DDPut("Unlimited");
		} else if (istoken(&cmdchar, "DB")) {
			ddprintf("%Lu", user.user_dlbytes);
		} else if (istoken(&cmdchar, "DF")) {
			ddprintf("%d", user.user_dlfiles);
		} else if (istoken(&cmdchar, "BA")) {
			freebstr(tempbuf, sizeof tempbuf);
			DDPut(tempbuf);
		} else if (istoken(&cmdchar, "FA")) {
			freefstr(tempbuf, sizeof tempbuf);
			DDPut(tempbuf);
		} else if (istoken(&cmdchar, "MP")) {
			ddprintf("%d", user.user_pubmessages + user.user_pvtmessages);
		} else if (istoken(&cmdchar, "TC")) {
			ddprintf("%d", user.user_connections);
		} else if (istoken(&cmdchar, "PA")) {
			ddprintf("%d", pages);
		} else if (istoken(&cmdchar, "CN")) {
			DDPut(conference()->conf.CONF_NAME);
		} else if (istoken(&cmdchar, "CU")) {
			ddprintf("%d", conference()->conf.CONF_NUMBER);
		} else if (istoken(&cmdchar, "MC")) {			
			const char *sr;
			/* probably better errno could exist */
			bleft -= 2;
		        if ((sr	= memchr(cmdchar, '|', bleft)) == NULL) {
				errno = EINVAL;
				return -1;
			}
			docmd(cmdchar, sr - cmdchar, 0);
			cmdchar = sr + 1;	
		} else if (istoken(&cmdchar, "TF")) {
			const char *sr;
			char *codebuf;

			bleft -= 2;

			fflush(stdout);
			if ((sr = memchr(cmdchar, '|', bleft)) == NULL) {
				errno = EINVAL;
				return -1;
			}	
			codebuf = (char *) xmalloc(sr - cmdchar + 1);
			strlcpy(codebuf, cmdchar, sr - cmdchar + 1);
			/* FIXME: consider checking return value */
			TypeFile(codebuf, 0);
			free(codebuf);
			cmdchar = sr + 1;
		} else if (istoken(&cmdchar, "NM")) {
			sl = -1;
		} else if (istoken(&cmdchar, "NC")) {
			nocodes = 1;
		} else if (istoken(&cmdchar, "RA")) {
			const char *sr, *rr;
			
			bleft -= 2;

			if ((sr = memchr(cmdchar, '|', bleft)) == NULL) {
				errno = EINVAL;
				return -1;
			}
			if ((rr = memchr(sr + 1, '|', 
				bleft - (sr - cmdchar + 1))) == NULL) {
				errno = EINVAL;
				return -1;
			}

			if (type_random_file(cmdchar, sr - cmdchar,
				sr + 1, rr - sr - 1)) 
				syslog(LOG_ERR, "cannot type random file");

			cmdchar = rr + 1;
		} else if (istoken(&cmdchar, "MB")) {
			/* FIXME: this should not rely on the NUL-terminator.
			 * Anyhow, there's no way for the input string not to
			 * be NUL-terminated, so this is not a vulnerability
			 * yet.
			 */
			parse_menu_command(&cmdchar);
		} else if (istoken(&cmdchar, "LF")) {
			char *sr;

			bleft -= 2;

			if ((sr = memchr(cmdchar, 0x0a, bleft)) == NULL)
				break;
			cmdchar = sr + 1;
		}

		ptr = cmdchar;
	}

	ansi = olda;
	return 1;
}
	
/* dotype() return value:
 * 
 *  0 - error
 *  1 - successful
 */
static int dotype(char *filename, int flags)
{
	int fd, retcode;
	char *buffer;
	struct stat st;

	if ((fd = open(filename, O_RDONLY)) == -1)
		return 0;
	fstat(fd, &st);		
		
	// Allocate buffer with extra space for color code expansion
	// Color codes can expand significantly (e.g., @X08 -> \e[0;30;40m)
	buffer = (char *) xmalloc(st.st_size * 3 + 1000);
	read(fd, buffer, st.st_size);
	buffer[st.st_size] = '\0';  // Ensure null termination
	close(fd);

    parsepipes(buffer);
	retcode = formatted_print(buffer, strlen(buffer), flags);
	free(buffer);
	
	return retcode;
}

int processmsg(struct dd_nodemessage *ddm)
{
	struct olm *myolm;
	char buf[1024];

	switch (ddm->dn_command) {
	case 1:
		myolm = (struct olm *) xmalloc(sizeof(struct olm));
		strlcpy(myolm->olm_msg, ddm->dn_string, sizeof myolm->olm_msg);
		myolm->olm_number = 0;
		AddTail(olms, (struct Node *) myolm);
		break;
	case 2:
		myolm = (struct olm *) xmalloc(sizeof(struct olm));
		strlcpy(myolm->olm_msg, ddm->dn_string, sizeof myolm->olm_msg);
		myolm->olm_number = ddm->dn_data1;
		AddTail(olms, (struct Node *) myolm);
		break;
	case 3:
		LineChat();
		break;
	case 4:
		dropcarrier();

		snprintf(buf, sizeof buf, 
			"User kicked out at %s\n", currt());
		writelog(buf);
		break;
	case 5:
		usered();
		break;
	case 10:
		user.user_ulfiles += ddm->dn_data1;
		break;
	case 11:
		user.user_dlfiles += ddm->dn_data1;
		break;

	case 12:
		user.user_ulbytes += ddm->dn_data1;
		break;
	case 13:
		user.user_dlbytes += ddm->dn_data1;
		break;
	}
	return 1;
}
