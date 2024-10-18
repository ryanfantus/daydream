#include <stdarg.h>
#include <stdio.h>
#include <fcntl.h>
#include <global.h>
#include <syslog.h>
#include <dd.h>
#include <ddcommon.h>
#include <md5.h>
#include <stdlib.h>

#define MD_CTX MD5_CTX
#define MDInit MD5Init
#define MDUpdate MD5Update
#define MDFinal MD5Final

static struct DayDream_MainConfig mcfg;
char* datadir;

static int cmppasswds(char *passwd, unsigned char *thepw)
{
  MD_CTX context;
  unsigned char digest[16];
  char newpw[30];
  int i;

  strcpy(newpw, passwd);
  strupr(newpw);

  MDInit(&context);
  MDUpdate(&context, newpw, strlen(newpw));
  MDFinal(digest, &context);

  for (i = 0; i < 16; i++) {
    if (thepw[i] != digest[i])
      return (0);
  }
  return (1);
}

static struct userbase *ddgetpwnam(char *name)
{
  int fd;
  char buf[PATH_MAX];
  static struct userbase ub;

  if (ssnprintf(buf, "%s/data/userbase.dat", datadir))
    return NULL;

  if ((fd = open(buf, O_RDONLY)) != -1) {
    while (read(fd, &ub, sizeof(struct userbase))) {
      if ((ub.user_toggles & (1L << 30))
          && ((ub.user_toggles & (1L << 31)) == 0))
        continue;
      if (!strcasecmp(ub.user_handle, name)
          || !strcasecmp(ub.user_realname, name)) {
        close(fd);
        return (&ub);
      }
    }
    close(fd);
  } else
    syslog(LOG_ERR, "cannot open userbase");
  return NULL;
}

static int getddconfig(void)
{
        char buf[1024];
        int fd;

        if (ssnprintf(buf, "%s/data/daydream.dat", datadir))
                return -1;
        if ((fd = open(buf, O_RDONLY)) == -1)
                return -1;
        if (read(fd, &mcfg, sizeof(struct DayDream_MainConfig)) !=
            sizeof(struct DayDream_MainConfig))
                return -1;
        close(fd);

	return 0;
}

int main() {
  char acc[128];
  char pw[128];
  char dd[128];
  char ftpdir[128];
  char buf[128];
  char* tmp; 

  openlog("ftpauth", LOG_PID, LOG_AUTH);

  tmp = getenv("DAYDREAM");
  if(!tmp) {
    syslog(0, "Can't get DAYDREAM env");
    exit(1);
  }

  strncpy(dd, tmp, 128);
  datadir = dd;


  tmp = getenv("AUTHD_ACCOUNT");
  if(!tmp) {
    syslog(0, "Can't get account");
    exit(1);
  }

  strncpy(acc, tmp, 128);

  tmp = getenv("AUTHD_PASSWORD");
  if(!tmp) {
    syslog(0, "Can't get pw");
    exit(1);
  }

  strncpy(pw, tmp, 128);

  if(getddconfig() < 0) {
    syslog(0, "Can't read config");
    exit(1);
  }

  struct userbase* u = ddgetpwnam(acc);
  
  if(u) {

    sprintf(ftpdir, "%s/users/%d/ftp", datadir, u->user_account_id);
    if (create_directory(ftpdir, mcfg.CFG_BBSUID, mcfg.CFG_BBSGID, 0770)) {
       syslog(0, "Can't create ftpdir %s", ftpdir);  
       exit(1);
    }

    sprintf(buf, "%s/ul", ftpdir);
    if (create_directory(buf, mcfg.CFG_BBSUID, mcfg.CFG_BBSGID, 0770)) {
       syslog(0, "Can't create ftpdir/ul %s", buf);  
       exit(1);
    }
    sprintf(buf, "%s/dl", ftpdir);
    if (create_directory(buf, mcfg.CFG_BBSUID, mcfg.CFG_BBSGID, 0770)) {
       syslog(0, "Can't create ftpdir/dl %s", buf);  
       exit(1);
    }

    if(cmppasswds(pw, u->user_password)) {
      printf("auth_ok:1\n");
      printf("uid:1001\n");
      printf("gid:1001\n");
      printf("dir:%s\n", ftpdir);
    }
    else {
      printf("auth_ok:0\n");
    }
  }
  else {
    printf("auth_ok:0\n");
  }
  
  printf("end\n");

  closelog();

  return 0;
}
