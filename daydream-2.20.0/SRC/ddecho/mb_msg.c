/*
 * DD-Echo
 * Star-MSG (*.MSG) message base routines
 * Author: Bo Simonsen <bo@geekworld.dk>
 */

#include "headers.h"

void strtolower(char* str) {
	do
	{
		*str = (char) tolower((int) *str);
	} while(*str++);
}

int msg_HighMsgNum(char* netdir) {
	int high = 0;
	int i;
	DIR* dir;
	struct dirent* di;

	dir = opendir(netdir);

	while((di = readdir(dir))) {

		strtolower(di->d_name);

		if(sscanf(di->d_name, "%d.msg", &i) == 1) {
			if(i > high) {
				high = i;
			}
		}
	}

	return high;

}

void msg_WriteMsg(FILE* fh, MEM_MSG* mem) {
	StoredMsg s;
	bsListItem* item;

	memset(&s, '\0', sizeof(StoredMsg));

	strncpy((char*) s.From, mem->MsgFrom, 36);
	strncpy((char*) s.To, mem->MsgTo, 36);
	strncpy((char*) s.Subject, mem->Subject, 72);

	TmToStr(&mem->MsgDate, (char*) s.DateTime);

	s.DestZone = mem->MsgDest.zone;
	s.DestNet = mem->MsgDest.net;
	s.DestNode = mem->MsgDest.node;
	s.DestPoint = mem->MsgDest.point;
	s.OrigZone = mem->MsgOrig.zone;
	s.OrigNet = mem->MsgOrig.net;
	s.OrigNode = mem->MsgOrig.node;
	s.OrigPoint = mem->MsgOrig.point;

	s.Attr = FlagsToAttr(mem->Flags);

	fwrite(&s, sizeof(StoredMsg), 1, fh);

	for(item=mem->Text.first; item; item=item->next) {
		fputs(item->val, fh);
		fputc('\r', fh);
	}

	fputc('\0', fh);

}
