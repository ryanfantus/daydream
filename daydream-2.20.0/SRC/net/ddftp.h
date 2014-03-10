#ifndef _DDFTP_H_INCLUDED
#define _DDFTP_H_INCLUDED

#define FTP_DATA_BOTTOM	40000
#define FTP_DATA_TOP	44999

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <config.h>

#ifndef HAVE_SETPROCTITLE
void setproctitle(const char *, ...) __attribute__ ((format (printf, 1, 2)));
#endif

void blkfree(char **);
void cwd(char *);
void _delete(char *);
void dolog(struct sockaddr_in *);
void dologout(int);
void fatal(char *);
void lreply(int, const char *, ...) __attribute__ ((format (printf, 2, 3)));
void makedir(char *);
void nack(const char *);
void pass(char *);
void passive(void);
void perror_reply(int, char *);
void pwd(void);
void removedir(char *);
void renamecmd(char *, char *);
void reply(int, const char *, ...) __attribute__ ((format (printf, 2, 3)));
void retrieve(char *, char *);
void send_file_list(char *);
int setulconf(int);
void statcmd(void);
void statfilecmd(char *);
void _store(char *, const char *, int);
void upper(char *);
void user(char *);
void yyerror(char *);
int yyparse(void);
char *renamefrom(char *);
char *ftpgetline(char *, int, FILE *) __attr_bounded__ (__string__, 1, 2);

#endif /* _DDFTP_H_INCLUDED */
