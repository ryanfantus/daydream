#include <fcntl.h>
#include <stdio.h>

#include <daydream.h>
#include <symtab.h>

void userlist(char *cmppat)
{
	struct userbase tmpuser;
	int lcount;
	int account_id = 0;

	char userbuf[500];
	int usercnt = 0, frozcnt = 0, sysopcnt = 0, cosysopcnt = 0;

	changenodestatus("Listing users");

	lcount = user.user_screenlength - 3;

	DDPut(sd[userlheadstr]);

	for (;; account_id++) {
		if (getubentbyid(account_id, &tmpuser) == -1)
			break;
		if ((tmpuser.user_toggles & UBENT_STAT_MASK) == 
			UBENT_STAT_DELETED)
			continue;
		
		if (cmppat &&
		    !wildcmp(tmpuser.user_realname, cmppat) && 
		    !wildcmp(tmpuser.user_handle, cmppat))
			continue;
		
		snprintf(userbuf, sizeof userbuf, sd[userllinestr], 
			tmpuser.user_realname, tmpuser.user_handle, 
			tmpuser.user_organization, tmpuser.user_securitylevel, 
			tmpuser.user_connections);
				
		usercnt++;
		if ((tmpuser.user_toggles & (1L << 31)) &&
			((tmpuser.user_toggles & (1L << 31)) == 0)) {
			DDPut("[34m");
			frozcnt++;
		} else if (tmpuser.user_securitylevel == 255) {
			DDPut("[32m");
			sysopcnt++;
		} else if (tmpuser.user_securitylevel == 
			maincfg.CFG_COSYSOPLEVEL) {
			DDPut("[33m");
			cosysopcnt++;
		} else {
			DDPut("[0m");
		}
		DDPut(userbuf);
		lcount--;
		
		if (lcount == 0) {
			int hot;
			
			DDPut(sd[morepromptstr]);
			hot = HotKey(0);
			DDPut("\r                                                         \r");
			if (hot == 'N' || hot == 'n') 			      
				break;
			if (hot == 'C' || hot == 'c') {
				lcount = -1;
			} else {
				lcount = user.user_screenlength;
			}
		}

	}	
	
	ddprintf(sd[userltailstr], sysopcnt, usercnt, cosysopcnt, frozcnt);
}
