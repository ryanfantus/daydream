/* $Id: patmat.c,v 1.1 2010-03-24 16:34:49 bo Exp $
 *  Provides functions to pattern matching. Taken from sh sources
 *
 * Copyright (c) 1999-2003
 *      The Husky Developers Team
 *
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Kenneth Almquist.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

/* standard headers */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#define TRUE 1
#define FALSE 0

#ifndef SMAPI
/***  Declarations & defines  ***********************************************/

#define CTLESC '\\'

/***  Implementation  *******************************************************/

/* Returns true if the pattern matches the string.
 */
int str_tolower(char* str)
{
    do
    {
	*str = (char) tolower((int) *str);
    } while(*str++);
    
    return(TRUE);
}

int patmat( const char *string, const char *pattern )
{
	register const char *p, *q;
	register char c;

	p = pattern;
	q = string;
	for (;;) {
		switch (c = *p++) {
		case '\0':
			goto breakloop;
		case CTLESC:
			if (*q++ != *p++)
				return 0;
			break;
		case '?':
			if (*q++ == '\0')
				return 0;
			break;
		case '*':
			c = *p;
			if (c != CTLESC && c != '?' && c != '*' && c != '[') {
				while (*q != c) {
					if (*q == '\0')
						return 0;
					q++;
				}
			}
			do {
				if (patmat(q, p))
					return 1;
			} while (*q++ != '\0');
			return 0;
		case '[': {
			const char *endp;
			int invert, found;
			char chr;

			endp = p;
			if (*endp == '!')
				endp++;
			for (;;) {
				if (*endp == '\0')
					goto dft;	/* no matching ] */
				if (*endp == CTLESC)
					endp++;
				if (*++endp == ']')
					break;
			}
			invert = 0;
			if (*p == '!') {
				invert++;
				p++;
			}
			found = 0;
			chr = *q++;
			c = *p++;
			do {
				if (c == CTLESC)
					c = *p++;
				if (*p == '-' && p[1] != ']') {
					p++;
					if (*p == CTLESC)
						p++;
					if (chr >= c && chr <= *p)
						found = 1;
					p++;
				} else {
					if (chr == c)
						found = 1;
				}
			} while ((c = *p++) != ']');
			if (found == invert)
				return 0;
			break;
		}
dft:	        default:
			if (*q++ != c)
				return 0;
			break;
		}
	}
breakloop:
	if (*q != '\0')
		return 0;
	return 1;
}

int patimat( char *string, char *pattern )
{
    char* tp1, *tp2;
    int retval;
    
    tp1 = strdup(string);
    tp2 = strdup(pattern);
    
    str_tolower(tp1);
    str_tolower(tp2);
    
    retval = patmat(tp1, tp2);
    
    free(tp1);
    free(tp2);
    
    return(retval);
}

#ifdef TEST

#include <stdio.h>

int main(void)
{
    printf("patmat(\"abcdefghi\", \"*ghi\"): %d\n", patmat("abcdefghi", "*ghi"));
    printf("patmat(\"abcdefghi\", \"??c??f*\"): %d\n", patmat("abcdefghi", "??c??f*"));
    printf("patmat(\"abcdefghi\", \"*dh*\"): %d\n", patmat("abcdefghi", "*dh*"));
    printf("patmat(\"abcdefghi\", \"*def\"): %d\n", patmat("abcdefghi", "*def"));
    return 0;
}

#endif
#endif
