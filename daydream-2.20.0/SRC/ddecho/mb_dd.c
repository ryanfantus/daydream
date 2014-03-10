/* 
 * DD-Echo
 * Daydream message base handling routines
 * Author: Bo Simonsen <bo@geekworld.dk>
 */

#include "headers.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define MSG_PERM S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP

DD_Area* dd_OpenArea(char* area, char* path, int basenum) {
	DD_Area* a;
	struct DayDream_MsgPointers ptrs;

	a = NEW(DD_Area);

	memset(a, '\0', sizeof(DD_Area));

	a->fd = ddmsg_open_base(path, basenum, O_RDWR | O_CREAT, 0666);

	if(a->fd < 0) {
		free(a);
		return NULL;
	}

	a->fh = fdopen(a->fd, "r+");

	strcpy(a->area, area);
	strcpy(a->path, path);
	a->basenum = basenum;

	ddmsg_getptrs(path, basenum, &ptrs);

	a->low = ptrs.msp_low;
	a->high = ptrs.msp_high;

	return a;
}

void dd_CloseArea(DD_Area* a) {
	struct DayDream_MsgPointers ptrs;

	if(a->low > 0 && a->high > 0) {
		ptrs.msp_low = a->low;
		ptrs.msp_high = a->high;
		ddmsg_setptrs(a->path, a->basenum, &ptrs);
	}

	if(a->fh != NULL) {
		fclose(a->fh);
	}
	if(a->fd != -1) {
		ddmsg_close_base(a->fd);
	}

	free(a);
}

int dd_SeekMsg(DD_Area* a, int num) {
	struct DayDream_Message dd_msg;

	fseek(a->fh, 0, SEEK_SET);

	while(fread(&dd_msg, sizeof(struct DayDream_Message), 1, a->fh)) {
		if(dd_msg.MSG_NUMBER == num) {
			break;
		}
	}

	if(feof(a->fh)) {
		return -1;
	}

	fseek(a->fh, -sizeof(struct DayDream_Message), SEEK_CUR);

	return 0;
}

int dd_RealReadMsg(DD_Area* a, int num, MEM_MSG* mem) {
	FILE* fh;
	int fd;
	struct DayDream_Message dd_msg;
	struct tm* tmp;
	char buf[4096];
	int first_line = 0;

	if(num > 0) {
		dd_SeekMsg(a, num);
	}
	else {
		if(ftell(a->fh) != a->r_pos) {
			fseek(a->fh, a->r_pos, SEEK_SET);
		}
	}

	memset(mem, '\0', sizeof(MEM_MSG));

	if(fread(&dd_msg, sizeof(struct DayDream_Message), 1, a->fh) != 1) {
		return -1;
	}

	mem->MsgNum = dd_msg.MSG_NUMBER;

	strncpy(mem->MsgFrom, dd_msg.MSG_AUTHOR, 26);

	if (dd_msg.MSG_RECEIVER[0] == 0) {
		strcpy(mem->MsgTo, "All");
	} else {
		strncpy(mem->MsgTo, dd_msg.MSG_RECEIVER, 26);
	}

	strncpy(mem->Subject, dd_msg.MSG_SUBJECT, 68);
	tmp = gmtime(&dd_msg.MSG_CREATION);
	memcpy(&mem->MsgDate, tmp, sizeof(struct tm));
	
	mem->MsgOrig.zone = dd_msg.MSG_FN_ORIG_ZONE;
	mem->MsgOrig.net = dd_msg.MSG_FN_ORIG_NET;
	mem->MsgOrig.node = dd_msg.MSG_FN_ORIG_NODE;
	mem->MsgOrig.point = dd_msg.MSG_FN_ORIG_POINT;
	mem->MsgDest.zone = dd_msg.MSG_FN_DEST_ZONE;
	mem->MsgDest.net = dd_msg.MSG_FN_DEST_NET;
	mem->MsgDest.node = dd_msg.MSG_FN_DEST_NODE;
	mem->MsgDest.point = dd_msg.MSG_FN_DEST_POINT;
	mem->Flags = (int) dd_msg.MSG_FLAGS;

	fd = ddmsg_open_msg(a->path, a->basenum, dd_msg.MSG_NUMBER, O_RDONLY, 0666);

	if (fd > 0) {
		fh = fdopen(fd, "r");

		while(fgets(buf, 256, fh)) {
			StripCrLf(buf);
			if (!strncmp("AREA:", buf, 5) && !first_line) {                    
				strncpy(mem->Area, buf+5, 64);                                              
			}    
			bsList_add(&mem->Text, strdup(buf));

			if(!first_line) {
				first_line = 1;
			}
		}

		fclose(fh);
		ddmsg_close_msg(fd);

	} else {
		mem->Flags |= MSG_FLAGS_DELETED;
	}

	a->r_pos = ftell(a->fh);
	
	return 0;
}

int dd_RealWriteMsg(DD_Area* a, int num, MEM_MSG* mem, int write_msg) {
	FILE* fh;
	struct DayDream_Message dd_msg;
	bsListItem* item;
	int fd;


	memset(&dd_msg, '\0', sizeof(dd_msg));

	if(num > 0) {
		dd_SeekMsg(a, num);
		dd_msg.MSG_NUMBER = num;
	}
	else {
		if(a->low > 0) {
			fseek(a->fh, 0, SEEK_END);
		} 
		else {
			a->low = 1;
		}

		a->high += 1;
		dd_msg.MSG_NUMBER = a->high;
	}

	strncpy(dd_msg.MSG_AUTHOR, mem->MsgFrom, 26);
	strncpy(dd_msg.MSG_RECEIVER, mem->MsgTo, 26);
	strncpy(dd_msg.MSG_SUBJECT, mem->Subject, 68);
	dd_msg.MSG_CREATION = mktime(&mem->MsgDate);
	dd_msg.MSG_RECEIVED = time(NULL);
	dd_msg.MSG_FN_ORIG_ZONE = mem->MsgOrig.zone;
	dd_msg.MSG_FN_ORIG_NET = mem->MsgOrig.net;
	dd_msg.MSG_FN_ORIG_NODE = mem->MsgOrig.node;
	dd_msg.MSG_FN_ORIG_POINT = mem->MsgOrig.point;
	dd_msg.MSG_FN_DEST_ZONE = mem->MsgDest.zone;
	dd_msg.MSG_FN_DEST_NET = mem->MsgDest.net;
	dd_msg.MSG_FN_DEST_NODE = mem->MsgDest.node;
	dd_msg.MSG_FN_DEST_POINT = mem->MsgDest.point;
	dd_msg.MSG_FLAGS = (int) mem->Flags;

	if(fwrite(&dd_msg, sizeof(dd_msg), 1, a->fh) != 1) {
		return -1;
	}

	if(dd_msg.MSG_FLAGS & MSG_FLAGS_DELETED) {
		ddmsg_delete_msg(a->path, a->basenum, dd_msg.MSG_NUMBER);
		return 0;
	}
	else if(write_msg) {
		fd = ddmsg_open_msg(a->path, a->basenum, dd_msg.MSG_NUMBER, O_WRONLY | O_CREAT, 0666);

		if(fd < 0) {
			return -1;
		}

		fh = fdopen(fd, "w");

		for(item=mem->Text.first; item; item=item->next) {
			fputs(item->val, fh);
			fputs("\n", fh);
		}

		fclose(fh);

		ddmsg_close_msg(fd);
	}

	return 0;

}


