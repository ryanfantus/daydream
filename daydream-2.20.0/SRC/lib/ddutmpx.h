#ifndef _DDUTMPX_H_INCLUDED
#define _DDUTMPX_H_INCLUDED

#include <config.h>

#ifdef HAVE_UTMPX_H
#include <utmpx.h>
#endif
#ifdef HAVE_UTMP_H
#include <utmp.h>
#endif

#ifndef HAVE_UTMPX_H
typedef struct utmp UTMPX;
#else
typedef struct utmpx UTMPX;
#endif

/* some sanity checks */
#if defined(HAVE_SETUTXENT) || defined(HAVE_GETUTXENT) || defined(HAVE_ENDUTXENT)
#if !defined(HAVE_SETUTXENT) || !defined(HAVE_GETUTXENT) || !defined(HAVE_ENDUTXENT)
#error Some of {set,get,end}utxent() defined, but not all!
#endif
#endif

#if defined(HAVE_SETUTENT) || defined(HAVE_GETUTENT) || defined(HAVE_ENDUTENT)
#if !defined(HAVE_SETUTENT) || !defined(HAVE_GETUTENT) || !defined(HAVE_ENDUTENT)
#error Some of {set,get,end}utent() defined, but not all!
#endif
#endif

#ifndef HAVE_SETUTXENT
void setutxent(void);
UTMPX *getutxent(void);
void endutxent(void);
#endif

#endif
