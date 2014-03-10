#include <stdio.h>

#include <daydream.h>

void userstats(void)
{
	char extrabuf[100];

	ddprintf(sd[stats1str], user.user_realname, user.user_screenlength);
	ddprintf(sd[stats2str], user.user_handle, user.user_securitylevel);

	ddprintf(sd[stats3str], user.user_zipcity, timeleft / 60, pages);
	ddprintf(sd[stats4str], user.user_organization);
	ddprintf(sd[stats5str], user.user_pubmessages, user.user_pvtmessages);

	snprintf(extrabuf, sizeof extrabuf, "%d", user.user_fileratio);
	ddprintf(sd[stats6str], user.user_ulbytes, user.user_ulfiles,
		       user.user_fileratio ? extrabuf : sd[disastr]);

	snprintf(extrabuf, sizeof extrabuf, "%d", user.user_byteratio);
	ddprintf(sd[stats7str], user.user_dlbytes, user.user_dlfiles, 
			user.user_byteratio ? extrabuf : sd[disastr]);

	snprintf(extrabuf, sizeof extrabuf, "%-24.24s", 
		ctime(&user.user_firstcall));
	snprintf(extrabuf + 30, sizeof extrabuf - 30,
		"%-24.24s", ctime(&user.user_lastcall));

	ddprintf(sd[stats8str], &extrabuf[4], &extrabuf[34], 
			user.user_connections);

	ddprintf(sd[stats9str], user.user_voicephone, protocol->PROTOCOL_NAME);

	ddprintf(sd[stats10str], user.user_computermodel);
	ddprintf(sd[stats11str], user.user_signature);
}
