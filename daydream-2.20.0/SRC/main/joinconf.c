#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <daydream.h>
#include <utility.h>
#include <ddcommon.h>

int joinconf(int confn, int flags)
{
	int newconfnum;
	conference_t *mc;
	struct iterator *iterator;

	char jbuffer[100];
	
	newconfnum = confn;

	changenodestatus("Changing conference");

	if ((flags & JC_LIST) && newconfnum == -1)
		TypeFile("joinconference", TYPE_WARN | TYPE_MAKE);


	while (newconfnum == -1) {		
		jbuffer[0] = 0;
		DDPut(sd[jcpromptstr]);
		if (!(Prompt(jbuffer, 3, 0)))
			return 0;
		if ((!strcasecmp(jbuffer, "l")) || jbuffer[0] == '?')
			TypeFile("joinconference", TYPE_WARN | TYPE_MAKE);
		else if (jbuffer[0] == 0)
			return 0;
		else {
			char *endptr;
			newconfnum = strtol(jbuffer, &endptr, 10);
			if (*jbuffer && !*endptr)
				break;
		}
		
		newconfnum = -1;
	}
	
	iterator = conference_iterator();
	while ((mc = (conference_t *) iterator_next(iterator))) {
		if (mc->conf.CONF_NUMBER != newconfnum)
			continue;
		
		if (!checkconfaccess(newconfnum, &user)) {
			if (!(flags & JC_SHUTUP)) {
				DDPut(sd[jcnoaccessstr]);
			}
			iterator_discard(iterator);
			return 0;
		}
				
		if (*mc->conf.CONF_PASSWD && !(flags & JC_SHUTUP) && !(flags & JC_QUICK)) {
			conference_t *oldconf = conference();
			
			set_conference(mc);
			TypeFile("conferencepw", TYPE_CONF | TYPE_MAKE);
			set_conference(oldconf);
			
			DDPut("[36mConference password: [0m");
			*jbuffer = 0;
			if (!(Prompt(jbuffer, 16, PROMPT_SECRET))) {
				iterator_discard(iterator);
				return 0;
			}
			if (strcasecmp(jbuffer, mc->conf.CONF_PASSWD)) {
				iterator_discard(iterator);
				return 0;
			}
		}
		
		if (!(flags & JC_NOUPDATE))
			user.user_joinconference = newconfnum;
		set_conference(mc);
		
		if (conference()->conf.CONF_MSGBASES)
			current_msgbase = conference()->msgbases[0];
		else 
			current_msgbase = NULL;

		if (!(flags & JC_QUICK)) {
			TypeFile("conferencejoined", TYPE_MAKE | TYPE_CONF);
		}
		
		if (current_msgbase) {
			if (flags & JC_QUICK) {
				changemsgbase(current_msgbase->MSGBASE_NUMBER, MC_QUICK | MC_NOSTAT);
			} else {
				changemsgbase(current_msgbase->MSGBASE_NUMBER, MC_QUICK);
			}
		}
		iterator_discard(iterator);
		return 1;
	}
	iterator_discard(iterator);
		
	if (!(flags & JC_SHUTUP)) {
		DDPut(sd[jcnoconfstr]);
	}

	return 0;
}

int nextconf(void)
{
	conference_t *mc;
	
	for (mc = conference()->next; mc; mc = mc->next) 
		if (checkconfaccess(mc->conf.CONF_NUMBER, &user))
			return joinconf(mc->conf.CONF_NUMBER, 0);
	
	return joinconf(-1, JC_LIST);
}

int prevconf(void)
{
	conference_t *mc;
	
	for (mc = conference()->prev; mc->prev; mc = mc->prev)
		if (checkconfaccess(mc->conf.CONF_NUMBER, &user))
			return joinconf(mc->conf.CONF_NUMBER, 0);
	
	return joinconf(-1, JC_LIST);
}

int joinconfmenu(const char *jcparams)
{
	char *endptr;
	int newconf;
	
	if (!jcparams)
		return joinconf(-1, JC_LIST);
	if (jcparams[0] == 'l' || jcparams[0] == 'L' || jcparams[0] == '?')
		return joinconf(-1, JC_LIST);
	
	newconf = strtol(jcparams, &endptr, 10);
	if (!*jcparams || *endptr || newconf < 0)
		return joinconf(-1, JC_LIST);
	
	return joinconf(newconf, 0);
}

int cmbmenu(char *cmbparams)
{
	if (!cmbparams)
		return changemsgbase(0, 0);
	return changemsgbase(atoi(cmbparams), 0);

}

int nextbase(void)
{
	return changemsgbase(current_msgbase->MSGBASE_NUMBER + 1, 0);
}

int prevbase(void)
{
	return changemsgbase(current_msgbase->MSGBASE_NUMBER - 1, 0);
}

int changemsgbase(int newb, int flags)
{
	char cbuffer[500];
	int basen;
	int i;	
	struct DayDream_LRP lrpd;
	int lrpfd;
	struct stat st;
	msgbase_t *cb;

	if (!conference()->conf.CONF_MSGBASES) {
		current_msgbase = NULL;
		return 0;
	}
       			
	basen = newb;
	
	if (basen > conference()->conf.CONF_MSGBASES)
		basen = 0;
	
	/* The MC_QUICK flag is used on non-interactive occasions. */
	if (!basen && (flags & MC_QUICK))
		return 0;
	
	if (!basen) {
		if (conference()->conf.CONF_MSGBASES == 1) {
			DDPut(sd[cmbonlymsgsstr]);
			return 0;
		}
		TypeFile("messagebases", TYPE_WARN | TYPE_CONF | TYPE_MAKE);
	}	      
	
	while (!basen) {
		DDPut(sd[cmbselectstr]);
		cbuffer[0] = 0;
		if (!(Prompt(cbuffer, 3, 0)))
			return 0;
		if (!strcasecmp(cbuffer, "?") || (!strcasecmp(cbuffer, "l"))) {
			TypeFile("messagebases", TYPE_WARN | TYPE_CONF | TYPE_MAKE);
		} else if (cbuffer[0] == 0) {
			return 0;
		} else
			basen = atoi(cbuffer);
	}
	for (i = 0; i < conference()->conf.CONF_MSGBASES; i++) {
		cb = conference()->msgbases[i];		
		
		if (cb->MSGBASE_NUMBER != basen)
			continue;
		
		current_msgbase = cb;
		getmsgptrs();
		if (lrpdatname[0] && (oldlrp != lrp || oldlsp != lsp)) {
			lrpfd = open(lrpdatname, O_WRONLY | O_CREAT, 0666);
			if (lrpfd == -1) {
				DDPut(sd[cmberrlrpstr]);
				return 0;
			}
			fsetperm(lrpfd, 0666);
			fstat(lrpfd, &st);
			lrpd.lrp_read = lrp;
			lrpd.lrp_scan = lsp;
			lseek(lrpfd, sizeof(struct DayDream_LRP) * user.user_account_id, SEEK_SET);
			safe_write(lrpfd, &lrpd, sizeof(struct DayDream_LRP));
			close(lrpfd);
		}
		snprintf(lrpdatname, sizeof lrpdatname,
			"%s/messages/base%3.3d/msgbase.lrp", 
			conference()->conf.CONF_PATH, cb->MSGBASE_NUMBER);
		lrpfd = open(lrpdatname, O_RDONLY);
		
		lrp = 0;
		lsp = 0;
		oldlrp = 0;
		oldlsp = 0;
		
		if (lrpfd != -1) {
			if ((lseek(lrpfd, sizeof(struct DayDream_LRP) * user.user_account_id, SEEK_SET)) != -1) {
				if (read(lrpfd, &lrpd, sizeof(struct DayDream_LRP))) {
					oldlrp = lrp = lrpd.lrp_read;
					oldlsp = lsp = lrpd.lrp_scan;
				}
			}
			close(lrpfd);
		}
		current_msgbase = cb;
		getmsgptrs();
		
		if ((flags & MC_NOSTAT) == 0) {
			ddprintf(sd[cmbstat1str], current_msgbase->MSGBASE_NAME, highest - lowest);
			ddprintf(sd[cmbstat2str], lrp, lsp);
			ddprintf(sd[cmbstat3str], lowest, highest);
		}
		
		return 1;
	}

	DDPut(sd[cmbunkbasestr]);
	return 0;
}
