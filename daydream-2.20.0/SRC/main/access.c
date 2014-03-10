#include <fcntl.h>
#include <unistd.h>

#include <daydream.h>

static int get_preset_security(struct userbase *ub, struct DD_Seclevel *sc,
			       int *max_pages, int *acc1, int *acc2)
{
	struct DD_Seclevel *currentsec = secs;
	while (currentsec->SEC_SECLEVEL) {
		if (currentsec->SEC_SECLEVEL == ub->user_securitylevel) {
			ub->user_fileratio = currentsec->SEC_FILERATIO;
			ub->user_byteratio = currentsec->SEC_BYTERATIO;
			ub->user_dailytimelimit = currentsec->SEC_DAILYTIME;
			ub->user_conferenceacc1 = currentsec->SEC_CONFERENCEACC1;
			ub->user_conferenceacc2 = currentsec->SEC_CONFERENCEACC2;
			if (max_pages)
				*max_pages = currentsec->SEC_PAGESPERCALL;
			if (acc1)
				*acc1 = currentsec->SEC_ACCESSBITS1;
			if (acc2)
				*acc2 = currentsec->SEC_ACCESSBITS2;
			if (sc)
				memcpy(sc, currentsec, sizeof(struct DD_Seclevel));
			return 1;
		} else {
			currentsec++;
		}
	}
	return 0;
}

static int get_security(struct userbase *ub, struct DD_Seclevel *sc,
			int *max_pages, int *acc1, int *acc2)
{
	struct DD_Seclevel security;
	char getsecbuf[300];
	int secfd;

	snprintf(getsecbuf, sizeof getsecbuf, "%s/users/%d/security.dat", 
			origdir, ub->user_account_id);
	
	secfd = open(getsecbuf, O_RDONLY);
	if (secfd == -1)
		return get_preset_security(ub, sc, max_pages, acc1, acc2);
	if (read(secfd, &security, sizeof(struct DD_Seclevel)) !=
		sizeof(struct DD_Seclevel)) {
		close(secfd);
		return get_preset_security(ub, sc, max_pages, acc1, acc2);
	}	
	close(secfd);
	
	ub->user_fileratio = security.SEC_FILERATIO;
	ub->user_byteratio = security.SEC_BYTERATIO;
	ub->user_dailytimelimit = security.SEC_DAILYTIME;
	ub->user_conferenceacc1 = security.SEC_CONFERENCEACC1;
	ub->user_conferenceacc2 = security.SEC_CONFERENCEACC2;
	if (max_pages)
		*max_pages = security.SEC_PAGESPERCALL;
	if (acc1)
		*acc1 = security.SEC_ACCESSBITS1;
	if (acc2)
		*acc2 = security.SEC_ACCESSBITS2;
	if (sc)
		memcpy(sc, &security, sizeof(struct DD_Seclevel));

	return 1;
}

int getsec(void)
{
	return get_security(&user, NULL, &pages, &access1, &access2);
}
	
int checkconfaccess(int confn, struct userbase *ub)
{
	int newcn = confn - 1;
	
	if (!get_security(ub, NULL, NULL, NULL, NULL))
		return 0;
	
	if (newcn < 32) {
		if (ub->user_conferenceacc1 & (1L << newcn))
			return 1;
	} else {
		newcn -= 32;
		if (ub->user_conferenceacc2 & (1L << newcn))
			return 1;
	}
	return 0;
}

int isaccess(int flag, int accint)
{
	if (!getsec())
		return 0;
	
	if (accint & (1L << flag))
		return 1;
	else {
		DDPut(sd[accden2str]);
		return 0;
	}
}
