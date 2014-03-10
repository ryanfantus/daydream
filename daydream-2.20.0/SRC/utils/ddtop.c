#include <dd.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>

struct ddtopwent {
	int64_t dde_ulb;
	int64_t dde_dlb;
	unsigned short dde_ulf;
	unsigned short dde_dlf;
	unsigned short dde_calls;
	unsigned short dde_msgs;
	unsigned short dde_id;
	time_t dde_firstcall;
};

struct ddtopwbase {
	time_t ddw_start;
};
 
static struct userbase *ub;
static struct userbase *ub2;

static int includesysop=1;

static int users;

static int *ulers;
static int *ulfers;
static uint64_t totuploads=0;
static int totfuploads=0;

static int *dlers;
static int *dlfers;
static uint64_t totdnloads=0;
static int totfdnloads=0;

static int *callers;
static int totcalls=0;

static int *msgers;
static int totmsgs=0;

static int *wulers;
static int *wulfers;
static uint64_t wtotuploads=0;
static int wtotfuploads=0;

static int *wdlers;
static int *wdlfers;
static uint64_t wtotdnloads=0;
static int wtotfdnloads=0;

static int *wcallers;
static int wtotcalls=0;

static int *wmsgers;
static int wtotmsgs=0;

static int namemode=0;
static int orgmode=0;

static char *script=0;

static FILE *scriptfd;

static time_t wtstart;
static time_t currdate;
static time_t wtend;

static struct ddtopwent *wtb;
static struct ddtopwent *wtb2;

static int wtusers;

static void stripansi(char *);
static char *strspa(char *src, char *dest);
static char *fgetsnolf(char *, int, FILE *) __attr_bounded__ (__string__, 1, 2);
static char *procc(char *s, char *foo);
static char *StyleNum(char *bufferi, uint64_t num);
static void showhelp(void);
static void makeweekbase(void);
static void readwt(int);
static int enddate(int);
static void makeuplist(int *);
static void makeupflist(int *);
static void makewuplist(int *);
static void makewupflist(int *);
static void makednlist(int *);
static void makednflist(int *);
static void makewdnlist(int *);
static void makewdnflist(int *);
static void makeclist(int *);
static void makemlist(int *);
static void makewclist(int *);
static void makewmlist(int *);
static void mkmsg(char *, char *, char *, char *);

int main(int argc, char *argv[])
{
	struct stat st;
	char fbuf[1024];
	int userbfd;
	char *cp;
	int ji;
	
	while (--argc) {
	cp = *++argv;
		if (*cp == '-') {
			while( *++cp) {
				switch(*cp) {
				case 'r':
					namemode=1; break;
				case 'l':
					orgmode=1;  break;
				case 'e':
					includesysop=0;
					break;
				}
				
			}
		} else {
			script=cp;
		}
	}
	if (!script) {
		showhelp();
		exit(0);
	}
	sprintf(fbuf,"%s/data/userbase.dat",getenv("DAYDREAM"));
	stat(fbuf,&st);

	scriptfd=fopen(script,"r");
	if (!scriptfd) {
		printf("Can't open script!\n");
		exit(0);
	}
	
	userbfd=open(fbuf,O_RDONLY);
	ub=malloc(st.st_size);
	ub2=malloc(st.st_size);
	if (userbfd==-1) {
		perror("Can't open userbase!");
		exit(1);
	}
	read(userbfd,ub,st.st_size);
	users=st.st_size/sizeof(struct userbase);
	close(userbfd);

	sprintf(fbuf,"%s/data/ddtop.data",getenv("DAYDREAM"));
	userbfd=open(fbuf,O_RDONLY);
	if (userbfd==-1) {
		makeweekbase();
		userbfd=open(fbuf,O_RDONLY);
		if (userbfd < 0) exit(1);
		readwt(userbfd);
	} else {
		readwt(userbfd);
	}
	
	currdate=time(0)/(60*60*24);
	wtend=enddate(wtstart);
	
	if (wtusers > users) wtusers=users;
	
	{
		struct userbase *u;
		struct ddtopwent *w;
		int i;
		u=ub;
		w=wtb;
		
		for(ji=users,i=1;ji;ji--,u++,w++,i++)
		{
			if (i>wtusers) {
				w->dde_ulb=u->user_ulbytes;
				w->dde_dlb=u->user_dlbytes;
				w->dde_ulf=u->user_ulfiles;
				w->dde_dlf=u->user_dlfiles;
				w->dde_calls=u->user_connections;
				w->dde_msgs=(u->user_pubmessages+u->user_pvtmessages);
				w->dde_id=u->user_account_id;
				w->dde_firstcall=u->user_firstcall;
			} else if (w->dde_firstcall==u->user_firstcall) {
				w->dde_ulb=u->user_ulbytes-w->dde_ulb;
				if (w->dde_ulb < 0) w->dde_ulb=0;
				w->dde_dlb=u->user_dlbytes-w->dde_dlb;
				if (w->dde_dlb < 0) w->dde_dlb=0;
				w->dde_ulf=u->user_ulfiles-w->dde_ulf;
				w->dde_dlf=u->user_dlfiles-w->dde_dlf;
				w->dde_calls=u->user_connections-w->dde_calls;
				w->dde_msgs=(u->user_pubmessages+u->user_pvtmessages)-w->dde_msgs;
			} else {
				w->dde_ulb=u->user_ulbytes;
				w->dde_dlb=u->user_dlbytes;
				w->dde_ulf=u->user_ulfiles;
				w->dde_dlf=u->user_dlfiles;
				w->dde_calls=u->user_connections;
				w->dde_msgs=(u->user_pubmessages+u->user_pvtmessages);
				w->dde_id=u->user_account_id;
				w->dde_firstcall=u->user_firstcall;
			}
		}
	}	
	ulers=malloc(users*sizeof(int));
	memset(ulers,255,users*sizeof(int));
	makeuplist(ulers);

	ulfers=malloc(users*sizeof(int));
	memset(ulfers,255,users*sizeof(int));
	makeupflist(ulfers);

	dlers=malloc(users*sizeof(int));
	memset(dlers,255,users*sizeof(int));
	makednlist(dlers);

	dlfers=malloc(users*sizeof(int));
	memset(dlfers,255,users*sizeof(int));
	makednflist(dlfers);

	callers=malloc(users*sizeof(int));
	memset(callers,255,users*sizeof(int));
	makeclist(callers);

	msgers=malloc(users*sizeof(int));
	memset(msgers,255,users*sizeof(int));
	makemlist(msgers);

	wulers=malloc(users*sizeof(int));
	memset(wulers,255,users*sizeof(int));
	makewuplist(wulers);

	wulfers=malloc(users*sizeof(int));
	memset(wulfers,255,users*sizeof(int));
	makewupflist(wulfers);
	
	wdlers=malloc(users*sizeof(int));
	memset(wdlers,255,users*sizeof(int));
	makewdnlist(wdlers);

	wdlfers=malloc(users*sizeof(int));
	memset(wdlfers,255,users*sizeof(int));
	makewdnflist(wdlfers);

	wcallers=malloc(users*sizeof(int));
	memset(wcallers,255,users*sizeof(int));
	makewclist(wcallers);

	wmsgers=malloc(users*sizeof(int));
	memset(wmsgers,255,users*sizeof(int));
	makewmlist(wmsgers);

	while(fgetsnolf(fbuf,1024,scriptfd))
	{
		char design[1024];
		char inp[4096];
		char outb[4096];
		char txto[1024];
		char gfxo[1024];
		
		char *outp=0;
		FILE *des;
		FILE *out1, *out2;
		FILE *sout1, *sout2;
		int lin=0;
		char msgcom[1024];
		
		*msgcom=0;
		sout1=sout2=0;
		
		if (*fbuf==';') continue;
		outp=strspa(fbuf,design);
		if (!outp) continue;
		outp++;
		des=fopen(design,"r");
		if (!des) continue;
		outp=strspa(outp,inp);

		sprintf(gfxo,"%s.gfx",inp);
		out1=fopen(gfxo,"w");
		sprintf(txto,"%s.txt",inp);
		out2=fopen(txto,"w");
		if (outp && (wtend < currdate)) {
			outp++;
			sprintf(design,"%s.gfx",outp);
			sout1=fopen(design,"w");
			sprintf(design,"%s.txt",outp);
			sout2=fopen(design,"w");
		}
		
		while(fgets(inp,4096,des))
		{
			char *s, *t;
			if (lin==0 && !strncmp(inp,"@MSG",4)) {
				mkmsg(msgcom,&inp[5],txto,gfxo);
				lin++;
				continue;
			}
			lin++;
			s=inp;
			t=outb;
			while(*s) {
				if (*s!='%') {
					*t++=*s++;
				} else {
					char *u;
					char foob[120];
					
					s=procc(++s,foob);
					u=foob;
					while (*u) {
						*t++=*u++;
					}
				}
			}
			*t=0;
			fputs(outb,out1);
			if (sout1) fputs(outb,sout1);
			stripansi(outb);
			fputs(outb,out2);
			if (sout2) fputs(outb,sout2);
		}
		fclose(out1);
		fclose(out2);
		if (sout1) {
			fclose(sout1);
			fclose(sout2);	
		}
		fclose(des);
		if (*msgcom && (wtend < currdate)) {
			system(msgcom);
		}	
	}
	fclose(scriptfd);
	if (wtend < currdate) 
		makeweekbase();
	return 0;
}

static void mkmsg(char *d, char *s, char *tx, char *gf)
{
	while(*s)
	{
		if (*s==13||*s==10) {
			s++;
		} else if (*s=='%') {
			s++;
			if (*s=='t' || *s=='T') {
				while(*tx) *d++=*tx++;
				s++;
			} else if (*s=='g' || *s=='G') {
				while(*gf) *d++=*gf++;
				s++;
			} else if (*s=='%') *d++=*s++;
		} else {
			*d++=*s++;
		}
	}
	*d=0;
}

static void readwt(int fd)
{
	struct stat st;
	fstat(fd,&st);
	read(fd,&wtstart,sizeof(time_t));
	wtb=malloc(users*sizeof(struct ddtopwent));
	wtb2=malloc(users*sizeof(struct ddtopwent));
	read(fd,wtb,st.st_size-sizeof(time_t));
	close(fd);
	wtusers=(st.st_size-sizeof(time_t))/sizeof(struct ddtopwent);
}

static char *procc(char *s, char *foo)
{
	char intb[80];
	char fom[120];
	char ent[120];
	struct userbase *us=0;
	struct ddtopwent *ws=0;
	int uo;
	int stmode=0;
	
	char *t;
	int pos=0;
	int len=0;
	int align=1;
	
	*ent=0;
	if (*s!='[') {
		*foo++=*s++;
		*foo=0;
		return s;
	}
	s++;

	if (*s!='.') {
		t=intb;
		while (*s!='.') {
			*t++=*s++;
		}
		*t=0;
		pos=atoi(intb);
	}		
	s++;

	if (*s!='.') {
		t=intb;
		if (*s=='-') {
			align=2;
			s++;
		}
		while (*s!='.' && *s!='-') {
			*t++=*s++;
		}
		if (*s=='-') {
			align+=2;
			s++;
		}
		*t=0;
		len=atoi(intb);
	}		
	s++;

	t=intb;
	while(*s!='_') {
		*t++=*s++;
	}
	s++;
	*t=0;
	
	if (!strcmp("SYSSUM",intb)) {
		t=intb;
		while(*s!='_' && *s!='.' && *s!=']') {
			*t++=*s++;
		}
		*t=0;
		if (!strcmp("AUTHOR",intb)) {
			strcpy(ent,"Hydra");
		} else if (!strcmp("PROGNAME",intb)) {
			strcpy(ent,"DD-Top V1.00");
		} else if (!strcmp("BYTESUP",intb)) {
			StyleNum(ent,totuploads);
		} else if (!strcmp("BYTESDOWN",intb)) {
			StyleNum(ent,totdnloads);
		} else if (!strcmp("FILESUP",intb)) {
			StyleNum(ent,totfuploads);
		} else if (!strcmp("FILESDOWN",intb)) {
			StyleNum(ent,totfdnloads);
		} else if (!strcmp("CALLS",intb)) {
			StyleNum(ent,totcalls);
		} else if (!strcmp("MESSAGES",intb)) {
			StyleNum(ent,totmsgs);
		} else if (!strcmp("USERS",intb)) {
			StyleNum(ent,users);
		} else if (!strcmp("DATE",intb)) {
			time_t tim;
			struct tm *tm;
			tim=time(0);
			tm=localtime(&tim);
			sprintf(ent,"%2.2d-%2.2d-%2.2d",tm->tm_mday,tm->tm_mon+1,tm->tm_year % 100);
		} else if (!strcmp("TIME",intb)) {
			time_t tim;
			struct tm *tm;
			tim=time(0);
			tm=localtime(&tim);
			sprintf(ent,"%2.2d:%2.2d:%2.2d",tm->tm_hour,tm->tm_min,tm->tm_sec);
		}
	} else if (!strcmp("WEEKSUM",intb)) {
		t=intb;
		while(*s!='_' && *s!='.' && *s!=']') {
			*t++=*s++;
		}
		*t=0;
		if (!strcmp("BYTESUP",intb)) {
			StyleNum(ent,wtotuploads);
		} else if (!strcmp("BYTESDOWN",intb)) {
			StyleNum(ent,wtotdnloads);
		} else if (!strcmp("FILESUP",intb)) {
			StyleNum(ent,wtotfuploads);
		} else if (!strcmp("FILESDOWN",intb)) {
			StyleNum(ent,wtotfdnloads);
		} else if (!strcmp("CALLS",intb)) {
			StyleNum(ent,wtotcalls);
		} else if (!strcmp("MESSAGES",intb)) {
			StyleNum(ent,wtotmsgs);
		} else if (!strcmp("USERS",intb)) {
			StyleNum(ent,users);
		} else if (!strcmp("DAYSLEFT",intb)) {
			StyleNum(ent,(wtend-currdate)+1);
		} else if (!strcmp("DAYSPASSED",intb)) {
			if (currdate > wtend) {
				strcpy(ent,"7");
			} else {
				StyleNum(ent,(7-(wtend-currdate)));
			}
		}
	} else if (!strcmp("UL",intb) && pos <= users) {
		t=intb;
		while(*s!='_') {
			*t++=*s++;
		}
		s++;
		*t=0;
		if (!strcmp("TOT",intb)) {
			uo=ulers[pos-1];
			if (uo!=-1 && uo <= users) {
				us=ub+uo;
				ws=wtb+uo;
			}
		} else if (!strcmp("WEEK",intb)) {
			stmode=1;
			uo=wulers[pos-1];
			if (uo!=-1 && uo <= users) {
				us=ub+uo;
				ws=wtb+uo;
			}
		}
	} else if (!strcmp("ULS",intb) && pos <= users) {
		t=intb;
		while(*s!='_') {
			*t++=*s++;
		}
		s++;
		*t=0;
		if (!strcmp("TOT",intb)) {
			uo=ulfers[pos-1];
			if (uo!=-1 && uo <= users) {
				us=ub+uo;
			}
		} else if (!strcmp("WEEK",intb)) {
			stmode=1;
			uo=wulfers[pos-1];
			if (uo!=-1 && uo <= users) {
				us=ub+uo;
				ws=wtb+uo;
			}
		}
 	
	} else if (!strcmp("DL",intb) && pos <= users) {
		t=intb;
		while(*s!='_') {
			*t++=*s++;
		}
		s++;
		*t=0;
		if (!strcmp("TOT",intb)) {
			uo=dlers[pos-1];
			if (uo!=-1 && uo <= users) {
				us=ub+uo;
			}
		} else if (!strcmp("WEEK",intb)) {
			stmode=1;
			uo=wdlers[pos-1];
			if (uo!=-1 && uo <= users) {
				us=ub+uo;
				ws=wtb+uo;
			}
		}
	
	} else if (!strcmp("DLS",intb) && pos <= users) {
		t=intb;
		while(*s!='_') {
			*t++=*s++;
		}
		s++;
		*t=0;
		if (!strcmp("TOT",intb)) {
			uo=dlfers[pos-1];
			if (uo!=-1 && uo <= users) {
				us=ub+uo;
			}
		} else if (!strcmp("WEEK",intb)) {
			stmode=1;
			uo=wdlfers[pos-1];
			if (uo!=-1 && uo <= users) {
				us=ub+uo;
				ws=wtb+uo;
			}
		}
	
	} else if (!strcmp("CALL",intb) && pos <= users) {
		t=intb;
		while(*s!='_') {
			*t++=*s++;
		}
		s++;
		*t=0;
		if (!strcmp("TOT",intb)) {
			uo=callers[pos-1];
			if (uo!=-1 && uo <= users) {
				us=ub+uo;
			}
		} else if (!strcmp("WEEK",intb)) {
			stmode=1;
			uo=wcallers[pos-1];
			if (uo!=-1 && uo <= users) {
				us=ub+uo;
				ws=wtb+uo;
			}
		}
	
	} else if (!strcmp("MESS",intb) && pos <= users) {
		t=intb;
		while(*s!='_') {
			*t++=*s++;
		}
		s++;
		*t=0;
		if (!strcmp("TOT",intb)) {
			uo=msgers[pos-1];
			if (uo!=-1 && uo <= users) {
				us=ub+uo;
			}
		} else if (!strcmp("WEEK",intb)) {
			stmode=1;
			uo=wmsgers[pos-1];
			if (uo!=-1 && uo <= users) {
				us=ub+uo;
				ws=wtb+uo;
			}
		}
		
	}

	if (us) {
		t=intb;
		while(*s!='.' && *s!=']') {
			*t++=*s++;
		}
		*t=0;
		if (pos > users) {
			strcpy(ent," ");
		} else if (!strcmp("NAME",intb)) {
			strcpy(ent, namemode ? us->user_realname : us->user_handle);
		} else if (!strcmp("BYTESUP",intb)) {
			if (stmode==0) {
				StyleNum(ent,us->user_ulbytes);
			} else {
				StyleNum(ent,ws->dde_ulb);
			}
		} else if (!strcmp("BYTESDOWN",intb)) {
			if (stmode==0) {
				StyleNum(ent,us->user_dlbytes);
			} else {
				StyleNum(ent,ws->dde_dlb);
			}
		} else if (!strcmp("FILESUP",intb)) {
			if (stmode==0) {
				StyleNum(ent,us->user_ulfiles);
			} else {
				StyleNum(ent,ws->dde_ulf);
			}
		} else if (!strcmp("FILESDOWN",intb)) {
			if (stmode==0) {
				StyleNum(ent,us->user_dlfiles);
			} else {
				StyleNum(ent,ws->dde_dlf);
			}
		} else if (!strcmp("CALLS",intb)) {
			if (stmode==0) {
				StyleNum(ent,us->user_connections);
			} else {
				StyleNum(ent,ws->dde_calls);
			}
		} else if (!strcmp("MESSAGES",intb)) {
			if (stmode==0) {
				StyleNum(ent,us->user_pubmessages+us->user_pvtmessages);
			} else {
				StyleNum(ent,ws->dde_msgs);
			}
		} else if (!strcmp("SLOTNR",intb)) {
			StyleNum(ent,us->user_account_id);
		} else if (!strcmp("LOCATION",intb)) {
			strcpy(ent, orgmode ? us->user_zipcity : us->user_organization);
		} else if (!strcmp("BYTERATIO",intb)) {
			if (stmode==0) {
				if (us->user_ulbytes > us->user_dlbytes) {
					if (us->user_dlbytes && ((us->user_ulbytes / us->user_dlbytes) > 2)) {
						strcpy(ent,"**");
					} else if (us->user_dlbytes && ((us->user_ulbytes / us->user_dlbytes) > 1)) {
						strcpy(ent,"*");
					} else if (!us->user_dlbytes) {
						strcpy(ent,"**");	
					}
				} else {
					if (us->user_ulbytes && ((us->user_dlbytes / us->user_ulbytes) > 2)) {
						strcpy(ent,"!!");
					} else if (us->user_ulbytes && ((us->user_dlbytes / us->user_ulbytes) > 1)) {
						strcpy(ent,"!");
					} else if (!us->user_ulbytes) {
						strcpy(ent,"!!");	
					}
				}
			} else {
				if (ws->dde_ulb > ws->dde_dlb) {
					if (ws->dde_dlb && ((ws->dde_ulb / ws->dde_dlb) > 2)) {
						strcpy(ent,"**");
					} else if (ws->dde_dlb && ((ws->dde_ulb / ws->dde_dlb) > 1)) {
						strcpy(ent,"*");
					} else if (!ws->dde_dlb) {
						strcpy(ent,"**");	
					}
				} else {
					if (ws->dde_ulb && ((ws->dde_dlb / ws->dde_ulb) > 2)) {
						strcpy(ent,"!!");
					} else if (ws->dde_ulb && ((ws->dde_dlb / ws->dde_ulb) > 1)) {
						strcpy(ent,"!");
					} else if (!ws->dde_ulb) {
						strcpy(ent,"!!");	
					}
				}
			}
		} else if (!strcmp("FILERATIO",intb)) {
			if (stmode==0) {
				if (us->user_ulfiles > us->user_dlfiles) {
					if (us->user_dlfiles && ((us->user_ulfiles / us->user_dlfiles) > 2)) {
						strcpy(ent,"**");
					} else if (us->user_dlfiles && ((us->user_ulfiles / us->user_dlfiles) > 1)) {
						strcpy(ent,"*");
					} else if (!us->user_dlfiles) {
						strcpy(ent,"**");	
					}
				} else {
					if (us->user_ulfiles && ((us->user_dlfiles / us->user_ulfiles) > 2)) {
						strcpy(ent,"!!");
					} else if (us->user_ulfiles && ((us->user_dlfiles / us->user_ulfiles) > 1)) {
						strcpy(ent,"!");
					} else if (!us->user_ulfiles) {
						strcpy(ent,"!!");	
					}
				}
			} else {
				if (ws->dde_ulf > ws->dde_dlf) {
					if (ws->dde_dlf && ((ws->dde_ulf / ws->dde_dlf) > 2)) {
						strcpy(ent,"**");
					} else if (ws->dde_dlf && ((ws->dde_ulf / ws->dde_dlf) > 1)) {
						strcpy(ent,"*");
					} else if (!ws->dde_dlf) {
						strcpy(ent,"**");	
					}
				} else {
					if (ws->dde_ulf && ((ws->dde_dlf / ws->dde_ulf) > 2)) {
						strcpy(ent,"!!");
					} else if (ws->dde_ulf && ((ws->dde_dlf / ws->dde_ulf) > 1)) {
						strcpy(ent,"!");
					} else if (!ws->dde_ulf) {
						strcpy(ent,"!!");	
					}
				}
			}
		}
	}

	if (!len) {
		strcpy(fom,"%s");
	} else if (align < 3) {
		sprintf(fom,"%%%d.%ds",len,len);
	} else {
		sprintf(fom,"%%-%d.%ds",len,len);
	}

	sprintf(foo,fom,ent);

	while (*s!=']') s++;
	s++;
	return s;
}

static char *StyleNum(char *bufferi, uint64_t num)
{
	char tempbuf[60];
	char *tp;
	char *p;
	int  buflen;
	int  silmacount;

	sprintf(tempbuf,"%Lu",num);
	tp=&tempbuf[50];
	buflen=strlen(tempbuf);
	p=&tempbuf[buflen-1];
	tp[1]=0;
	silmacount=3;

	while (p!=tempbuf)
	{
		if (!silmacount) {
			*tp='.';
			tp--;			
			silmacount=3;
		}
		*tp--=*p--;
		silmacount--;
	}
	if (!silmacount) {
		*tp='.';
		tp--;			
	}
	*tp=*p;

	strcpy(bufferi,tp);
	return(bufferi);
}

static void stripansi(char *tempmem)
{

	int go=1;
	int s=0;
	int t=0;
	int u;
	
  	while (go)
  	{
   		if (tempmem[s]==27) {
			u=s;
  			s=s+2;
skiploop:
			if (s) goto go1;
			go=0;
			goto gooff;
go1:
			if (tempmem[s] >= 'A') goto go2;
			s++;
			goto skiploop;
go2:
			if (tempmem[s] == 'm') goto go3;
			s=u; tempmem[t]=tempmem[s]; t++;
go3:
			s++;
gooff:;
		} else {
			tempmem[t]=tempmem[s]; t++;s++; 
			if(tempmem[s]==0) go=0; 
    		}
	}
	tempmem[t]=0;
}

static char *strspa(char *src, char *dest)
{
	int go=1;

	if (src==0) return 0;
		
	while (*src==' ') src++;
	if (*src==0) return 0;
	while (go)
	{
		if (*src==' '||*src==0) go=0;
		else *dest++=*src++;
	}
	*dest=0;
	return src;
}

static char *fgetsnolf(char *buf, int n, FILE *fh)
{
	char *hih;
	char *s;
	
	hih=fgets(buf,n,fh);
	if (!hih) return 0;
	s=buf;
	while (*s)
	{
		if (*s==13 || *s==10) {
			*s=0;
			break;
		}
		s++;
	}
	return hih;
}

static void showhelp(void)
{
	printf( "DDTop V1.0\n\n"
		"Usage:\n"
		"DDTop [-r] [-l] [-e] script\n"
		" -r - use realnames\n"
		" -l - use location\n"
		" -e - exclude sysop\n");
}

static int cmpupf(const void *e1, const void *e2)
{
	struct userbase *ent1, *ent2;
	ent1=(struct userbase *)e1;
	ent2=(struct userbase *)e2;
	if (ent1->user_ulfiles < ent2->user_ulfiles) return 1;
	if (ent1->user_ulfiles == ent2->user_ulfiles) return 0;
	return -1;
}

static void makeupflist(int *ul)
{
	struct userbase *u;
	int i;
	int *j;
	memcpy(ub2,ub,users*sizeof(struct userbase));
	qsort(ub2,users,sizeof(struct userbase),cmpupf);
	u=ub2;
	memset(ul,255,users*sizeof(int));
	j=ul;
	
	for (i=users;i;i--,u++)
	{
		if ( (u->user_account_id==0 && !includesysop) || u->user_ulfiles==0 ) continue;
		*j++=u->user_account_id;
		totfuploads+=u->user_ulfiles;
	}
}

static int cmpupb(const void *e1, const void *e2)
{
	struct userbase *ent1, *ent2;
	ent1=(struct userbase *)e1;
	ent2=(struct userbase *)e2;
	if (ent1->user_ulbytes < ent2->user_ulbytes) return 1;
	if (ent1->user_ulbytes == ent2->user_ulbytes) return 0;
	return -1;
}

static void makeuplist(int *ulers)
{
	struct userbase *u;
	int i;
	int *j;
	memcpy(ub2,ub,users*sizeof(struct userbase));
	qsort(ub2,users,sizeof(struct userbase),cmpupb);
	u=ub2;
	memset(ulers,255,users*sizeof(int));
	j=ulers;
	
	for (i=users;i;i--,u++)
	{
		if ( (u->user_account_id==0 && !includesysop) || u->user_ulbytes==0) continue;
		*j++=u->user_account_id;
		totuploads+=u->user_ulbytes;
	}
}


static int wcmpupf(const void *e1, const void *e2)
{
	struct ddtopwent *ent1, *ent2;
	ent1=(struct ddtopwent *)e1;
	ent2=(struct ddtopwent *)e2;
	
	if (ent1->dde_ulf < ent2->dde_ulf) return 1;
	if (ent1->dde_ulf == ent2->dde_ulf) return 0;
	return -1;
}

static void makewupflist(int *ul)
{
	struct ddtopwent *u;
	int i;
	int *j;
	int e=0;
	
	memcpy(wtb2,wtb,users*sizeof(struct ddtopwent));
	qsort(wtb2,users,sizeof(struct ddtopwent),wcmpupf);
	u=wtb2;
	memset(ul,255,users*sizeof(int));
	j=ul;
	
	for (i=users;i;i--,u++,e++)
	{
		if ((u->dde_id==0 && !includesysop) || u->dde_ulf==0) continue;
		*j++=u->dde_id;
		wtotfuploads+=u->dde_ulf;
	}
}

static int wcmpupb(const void *e1, const void *e2)
{
	struct ddtopwent *ent1, *ent2;
	ent1=(struct ddtopwent *)e1;
	ent2=(struct ddtopwent *)e2;
	
	if (ent1->dde_ulb < ent2->dde_ulb) return 1;
	if (ent1->dde_ulb == ent2->dde_ulb) return 0;
	return -1;
}

static void makewuplist(int *ul)
{
	struct ddtopwent *u;
	int i;
	int *j;
	int e=0;
	
	memcpy(wtb2,wtb,users*sizeof(struct ddtopwent));
	qsort(wtb2,users,sizeof(struct ddtopwent),wcmpupb);
	u=wtb2;
	memset(ul,255,users*sizeof(int));
	j=ul;
	
	for (i=users;i;i--,u++,e++)
	{
		if ((u->dde_id==0 && !includesysop) || u->dde_ulb==0) continue;
		*j++=u->dde_id;
		wtotuploads+=u->dde_ulb;
	}
}

static int wcmpdlf(const void *e1, const void *e2)
{
	struct ddtopwent *ent1, *ent2;
	ent1=(struct ddtopwent *)e1;
	ent2=(struct ddtopwent *)e2;
	
	if (ent1->dde_dlf < ent2->dde_dlf) return 1;
	if (ent1->dde_dlf == ent2->dde_dlf) return 0;
	return -1;
}

static void makewdnflist(int *ul)
{
	struct ddtopwent *u;
	int i;
	int *j;
	int e=0;
	
	memcpy(wtb2,wtb,users*sizeof(struct ddtopwent));
	qsort(wtb2,users,sizeof(struct ddtopwent),wcmpdlf);
	u=wtb2;
	memset(ul,255,users*sizeof(int));
	j=ul;
	
	for (i=users;i;i--,u++,e++)
	{
		if ((u->dde_id==0 && !includesysop) || !u->dde_dlf) continue;
		*j++=u->dde_id;
		wtotfdnloads+=u->dde_dlf;
	}
}

static int wcmpdlb(const void *e1, const void *e2)
{
	struct ddtopwent *ent1, *ent2;
	ent1=(struct ddtopwent *)e1;
	ent2=(struct ddtopwent *)e2;
	
	if (ent1->dde_dlb < ent2->dde_dlb) return 1;
	if (ent1->dde_dlb == ent2->dde_dlb) return 0;
	return -1;
}

static void makewdnlist(int *ul)
{
	struct ddtopwent *u;
	int i;
	int *j;
	int e=0;
	
	memcpy(wtb2,wtb,users*sizeof(struct ddtopwent));
	qsort(wtb2,users,sizeof(struct ddtopwent),wcmpdlb);
	u=wtb2;
	memset(ul,255,users*sizeof(int));
	j=ul;
	
	for (i=users;i;i--,u++,e++)
	{
		if ( (u->dde_id==0 && !includesysop) || u->dde_dlb==0) continue;
		*j++=u->dde_id;
		wtotdnloads+=u->dde_dlb;
	}
}

static int wcmpcall(const void *e1, const void *e2)
{
	struct ddtopwent *ent1, *ent2;
	ent1=(struct ddtopwent *)e1;
	ent2=(struct ddtopwent *)e2;
	
	if (ent1->dde_calls < ent2->dde_calls) return 1;
	if (ent1->dde_calls == ent2->dde_calls) return 0;
	return -1;
}

static void makewclist(int *ul)
{
	struct ddtopwent *u;
	int i;
	int *j;
	int e=0;
	
	memcpy(wtb2,wtb,users*sizeof(struct ddtopwent));
	qsort(wtb2,users,sizeof(struct ddtopwent),wcmpcall);
	u=wtb2;
	memset(ul,255,users*sizeof(int));
	j=ul;
	
	for (i=users;i;i--,u++,e++)
	{
		if ( (u->dde_id==0 && !includesysop) || u->dde_calls==0) continue;
		*j++=u->dde_id;
		wtotcalls+=u->dde_calls;
	}
}

static int wcmpmsg(const void *e1, const void *e2)
{
	struct ddtopwent *ent1, *ent2;
	ent1=(struct ddtopwent *)e1;
	ent2=(struct ddtopwent *)e2;
	
	if (ent1->dde_msgs < ent2->dde_msgs) return 1;
	if (ent1->dde_msgs == ent2->dde_msgs) return 0;
	return -1;
}

static void makewmlist(int *ul)
{
	struct ddtopwent *u;
	int i;
	int *j;
	int e=0;
	
	memcpy(wtb2,wtb,users*sizeof(struct ddtopwent));
	qsort(wtb2,users,sizeof(struct ddtopwent),wcmpmsg);
	u=wtb2;
	memset(ul,255,users*sizeof(int));
	j=ul;
	
	for (i=users;i;i--,u++,e++)
	{
		if ( (u->dde_id==0 && !includesysop) || u->dde_msgs==0) continue;
		*j++=u->dde_id;
		wtotmsgs+=u->dde_msgs;
	}
}


static int cmpdnf(const void *e1, const void *e2)
{
	struct userbase *ent1, *ent2;
	ent1=(struct userbase *)e1;
	ent2=(struct userbase *)e2;
	if (ent1->user_dlfiles < ent2->user_dlfiles) return 1;
	if (ent1->user_dlfiles == ent2->user_dlfiles) return 0;
	return -1;
}

static void makednflist(int *ul)
{
	struct userbase *u;
	int i;
	int *j;
	memcpy(ub2,ub,users*sizeof(struct userbase));
	qsort(ub2,users,sizeof(struct userbase),cmpdnf);
	u=ub2;
	memset(ul,255,users*sizeof(int));
	j=ul;
	
	for (i=users;i;i--,u++)
	{
		if ((u->user_account_id==0 && !includesysop) || !u->user_dlfiles) continue;
		*j++=u->user_account_id;
		totfdnloads+=u->user_dlfiles;
	}
}

static int cmpdnb(const void *e1, const void *e2)
{
	struct userbase *ent1, *ent2;
	ent1=(struct userbase *)e1;
	ent2=(struct userbase *)e2;
	if (ent1->user_dlbytes < ent2->user_dlbytes) return 1;
	if (ent1->user_dlbytes == ent2->user_dlbytes) return 0;
	return -1;
}

static void makednlist(int *ul)
{
	struct userbase *u;
	int i;
	int *j;
	memcpy(ub2,ub,users*sizeof(struct userbase));
	qsort(ub2,users,sizeof(struct userbase),cmpdnb);
	u=ub2;
	memset(ul,255,users*sizeof(int));
	j=ul;
	
	for (i=users;i;i--,u++)
	{
		if ((u->user_account_id==0 && !includesysop) || !u->user_dlbytes) continue;
		*j++=u->user_account_id;
		totdnloads+=u->user_dlbytes;
	}
}

static int cmpcall(const void *e1, const void *e2)
{
	struct userbase *ent1, *ent2;
	ent1=(struct userbase *)e1;
	ent2=(struct userbase *)e2;
	if (ent1->user_connections < ent2->user_connections) return 1;
	if (ent1->user_connections == ent2->user_connections) return 0;
	return -1;
}

static void makeclist(int *ul)
{
	struct userbase *u;
	int i;
	int *j;
	memcpy(ub2,ub,users*sizeof(struct userbase));
	qsort(ub2,users,sizeof(struct userbase),cmpcall);
	u=ub2;
	memset(ul,255,users*sizeof(int));
	j=ul;
	
	for (i=users;i;i--,u++)
	{
		if ((u->user_account_id==0 && !includesysop)||!u->user_connections) continue;
		*j++=u->user_account_id;
		totcalls+=u->user_connections;
	}
}

static int cmpm(const void *e1, const void *e2)
{
	struct userbase *ent1, *ent2;
	ent1=(struct userbase *)e1;
	ent2=(struct userbase *)e2;
	if ((ent1->user_pubmessages+ent1->user_pvtmessages) < (ent2->user_pvtmessages+ent2->user_pubmessages)) return 1;
	if ((ent1->user_pubmessages+ent1->user_pvtmessages) == (ent2->user_pubmessages+ent2->user_pvtmessages)) return 0;
	return -1;
}

static void makemlist(int *ul)
{
	struct userbase *u;
	int i;
	int *j;
	memcpy(ub2,ub,users*sizeof(struct userbase));
	qsort(ub2,users,sizeof(struct userbase),cmpm);
	u=ub2;
	memset(ul,255,users*sizeof(int));
	j=ul;
	
	for (i=users;i;i--,u++)
	{
		if ((u->user_account_id==0 && !includesysop) || !(u->user_pubmessages+u->user_pvtmessages)) continue;
		*j++=u->user_account_id;
		totmsgs+=(u->user_pubmessages+u->user_pvtmessages);
	}
}

static void makeweekbase(void)
{
	int wfd;
	time_t t;
	int i;
	struct userbase *u;
	
	char wb[1024];
	
	sprintf(wb,"%s/data/ddtop.data",getenv("DAYDREAM"));
	wfd=open(wb,O_WRONLY|O_CREAT|O_TRUNC,0644);
	if (wfd < 0) return;
	t=time(0)/(60*60*24);
	write(wfd,&t,sizeof(time_t));
	u=ub;
	for(i=users;i;i--,u++)
	{
		struct ddtopwent we;
		we.dde_ulb=u->user_ulbytes;
		we.dde_dlb=u->user_dlbytes;
		we.dde_ulf=u->user_ulfiles;
		we.dde_dlf=u->user_dlfiles;
		we.dde_calls=u->user_connections;
		we.dde_msgs=u->user_pubmessages+u->user_pvtmessages;
		we.dde_firstcall=u->user_firstcall;
		we.dde_id=u->user_account_id;
		write(wfd,&we,sizeof(struct ddtopwent));
	}
	close(wfd);
}

static int enddate(int da)
{
	int i;
	da-=(252460800/(60*60*24));
	i=((6-(da-1)%7)+da);
	return i+(252460800/(60*60*24));
}

