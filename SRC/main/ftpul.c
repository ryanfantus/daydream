#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <daydream.h>
#include <ddcommon.h>
#include <symtab.h>

static int getftpnode(void)
{
	struct DayDream_Multinode *danode;

	danode = nodes;

	while (danode->MULTI_NODE) {
		if (danode->MULTI_NODE == 252) {
			node = (ul_user + 1) * 10000;
			currnode = (struct DayDream_Multinode *)
				xmalloc(sizeof(struct DayDream_Multinode));
			memcpy(currnode, danode, sizeof(struct DayDream_Multinode));
			snprintf(currnode->MULTI_TEMPORARY, 
				sizeof currnode->MULTI_TEMPORARY,
				danode->MULTI_TEMPORARY, ul_user);
			currnode->MULTI_NODE = node;
			bpsrate = danode->MULTI_TTYSPEED;
			return 1;
		}
		danode++;
	}
	return 0;
}

int cmdlineupload(void)
{
	char buf[1024];
	carrier = 0;
	bgmode = 1;

	if (getubentbyid(ul_user, &user) == -1)
		return 0;

	if (!getftpnode())
		return 0;

	if (!getsec())
		return 0;

	getdisplaymode("1", 1);

	if (!joinconf(ul_conf, JC_SHUTUP | JC_QUICK | JC_NOUPDATE))
		return 0;

	mkdir(currnode->MULTI_TEMPORARY, 0770);
	snprintf(buf, sizeof buf, "%s/%s", currnode->MULTI_TEMPORARY, 
		filepart(ul_file));

	newrename(ul_file, buf);
	chdir(currnode->MULTI_TEMPORARY);
	if (!handleupload(filepart(ul_file))) 
		newrename(buf, ul_file);		
	chdir(origdir);
	deldir(currnode->MULTI_TEMPORARY);
	rmdir(currnode->MULTI_TEMPORARY);
	saveuserbase(&user);
	return 1;
}
