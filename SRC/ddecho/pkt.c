/*
 * DD-Echo
 * Routines for handling Fidonet packet files
 * Author: Bo Simonsen <bo@geekworld.dk>
 */

#include "headers.h"

int ReadPktHeader(FILE* fh, MEM_MSG* mem) {
	PKT pkt;
	char tbuf[64];

	if (fread(&pkt, sizeof(PKT), 1, fh) != 1) {
		printf("Error in reading pkt header!");
		return -1;
	}

	mem->PktOrig.zone = pkt.origZone ? pkt.origZone : pkt.origZoneq;

	/* WHY? */
	if (pkt.origNet == -1) {
		mem->PktOrig.net = pkt.AuxNet;
	} else {
		mem->PktOrig.net = pkt.origNet;
	}

	mem->PktOrig.node = pkt.origNode;
	mem->PktOrig.point = pkt.origPoint;

	mem->PktDest.zone = pkt.destZone ? pkt.destZone : pkt.destZoneq;
	mem->PktDest.net = pkt.destNet;
	mem->PktDest.node = pkt.destNode;
	mem->PktDest.point = pkt.destPoint;

	mem->PktDate.tm_year = (int) (pkt.year - 1900);
	mem->PktDate.tm_mon = (int) pkt.month;
	mem->PktDate.tm_mday = (int) pkt.day;
	mem->PktDate.tm_hour = (int) pkt.hour;
	mem->PktDate.tm_min = (int) pkt.minute;
	mem->PktDate.tm_sec = (int) pkt.second;

	strncpy(mem->PktPw, pkt.password, 8);
	mem->PktPw[8] = '\0';  /* Ensure null termination */

	sprintf(tbuf, "%02x%02x", pkt.prodcode2, pkt.prodcode);
	sscanf(tbuf, "%04x", (unsigned int*) &mem->Product);

	mem->Major_ver = pkt.rev;
	mem->Minor_ver = pkt.rev2;

	mem->PktType = pkt.type;

	if(pkt.cw & 0x0001)
		mem->Stoneage = 0;
	else
		mem->Stoneage = 1;

	if(mem->Stoneage) {
		/* We assume that our link is in the same primary zone
		   as our self, thereby the default zone given in the
		   config. 
		*/
	
		mem->PktOrig.zone = main_cfg.DefZone;
		mem->PktDest.zone = main_cfg.DefZone;
		
		/* Stoneage packets cannot operate with points */
		
		mem->PktOrig.point = 0;
		mem->PktDest.point = 0;
	}

	return 0;

}

int WritePktHeader(FILE* fh, MEM_MSG* mem) {
	PKT pkt;

	memset(&pkt, '\0', sizeof(PKT));

	pkt.type = 2;
	pkt.cw = 0x0001;
	pkt.cw2 = 0x0100;

	// No product code yet.
	pkt.prodcode2 = 0x00;
	pkt.prodcode = 0xFF;

	// My software is version 0.2.
	pkt.rev = 0;
	pkt.rev2 = 2;

	pkt.origZone = mem->PktOrig.zone;
	pkt.origZoneq = mem->PktOrig.zone;

	if(mem->PktOrig.point == 0) {
		pkt.origNet = mem->PktOrig.net;
	} else {
		pkt.origNet = -1;
	}

	pkt.AuxNet = mem->PktOrig.net;

	pkt.origNode = mem->PktOrig.node;
	pkt.origPoint = mem->PktOrig.point;

	pkt.destZone = mem->PktDest.zone;
	pkt.destZoneq = mem->PktDest.zone;
	pkt.destNet = mem->PktDest.net;
	pkt.destNode = mem->PktDest.node;
	pkt.destPoint = mem->PktDest.point;

	pkt.year = mem->PktDate.tm_year + 1900;
	pkt.month = mem->PktDate.tm_mon;
	pkt.day = mem->PktDate.tm_mday;
	pkt.hour = mem->PktDate.tm_hour;
	pkt.minute = mem->PktDate.tm_min;
	pkt.second = mem->PktDate.tm_sec;
	
	strncpy(pkt.password, mem->PktPw, 8);

	return fwrite(&pkt, sizeof(PKT), 1, fh);
}

void DumpMemMsg(MEM_MSG* mem) {
	char buf[64];
	bsListItem* tmp;

	printf("------ Dump of Mem Message -------\n");
	printf("Pkt From: %s\n", fidoaddr_to_text(&mem->PktOrig, buf));
	printf("Pkt To: %s\n", fidoaddr_to_text(&mem->PktDest, buf));
	printf("Pkt pw: %s\n", mem->PktPw);
	printf("Pkt Time: %s\n", TmToStr(&mem->PktDate, buf));
	printf("Product code: %d\n", mem->Product);
	printf("Version: %d.%d\n", mem->Major_ver, mem->Minor_ver);
	printf("Msg From: %s\n", fidoaddr_to_text(&mem->MsgOrig, buf));
	printf("Msg To: %s\n", fidoaddr_to_text(&mem->MsgDest, buf));
	printf("Msg From Name: %s\n", mem->MsgFrom);
	printf("Msg To Name: %s\n", mem->MsgTo);
	printf("Msg Date: %s\n", TmToStr(&mem->MsgDate, buf));
	printf("Msg Area: %s\n", mem->Area);
	printf("Subject: %s\n", mem->Subject);
	printf("Text:");

	for(tmp=mem->Text.first; tmp; tmp=tmp->next) {
		printf("%s\n", (char*) tmp->val);
	}

}

void CopyMemMsg(MEM_MSG* source, MEM_MSG* dest) {
	bsListItem* item;

	memcpy(dest, source, sizeof(MEM_MSG));

	memset(&dest->Text, '\0', sizeof(bsList));

	for(item=source->Text.first; item; item=item->next) {
		bsList_add(&dest->Text, strdup(item->val));
	}

}

void FreeMemMsg(MEM_MSG* mem) {
	bsList_free(&mem->Text);
	memset(&mem->Text, '\0', sizeof(bsList));
}

int NullRead(FILE* fh, char* buf, int size) {
	char c;
	int read = 0;

	while(fread(&c, 1, 1, fh)) {

		if (c == 0 || read >= size) {
			break;
		}

		*buf = c;
		buf++;
		read++;

	}

	*buf = '\0';

	return read;

}

int CRRead(FILE* fh, char* buf, int size) {
	char c;
	int read = 0;

	while(fread(&c, 1, 1, fh)) {

		if (c == 0) {
			return -1;
		}

		else if (c == '\r' || read >= size) {
			break;
		}
		*buf = c;
		buf++;
		
		read++;

	}

	*buf = '\0';

	return read;

}

#define FTN_ATTR_PVT (1L << 0)
#define FTN_ATTR_CRASH (1L << 1)
#define FTN_ATTR_RECV (1L << 2)
#define FTN_ATTR_SENT (1L << 3)
#define FTN_ATTR_ATTACH (1L << 4)
#define FTN_ATTR_INTRANS (1L << 5)
#define FTN_ATTR_ORP (1L << 6)
#define FTN_ATTR_KILL_SENT (1L << 7)
#define FTN_ATTR_LOC (1L << 8)



int AttrToFlags(int ftn_attr) {
	int flags = 0;

	if(ftn_attr & FTN_ATTR_PVT) {
		flags |= MSG_FLAGS_PRIVATE;
	}
	if(ftn_attr & FTN_ATTR_CRASH) {
		flags |= MSG_FLAGS_CRASH;
	}
	if(ftn_attr & FTN_ATTR_ATTACH) {
		flags |= MSG_FLAGS_ATTACH;
	}
	if(ftn_attr & FTN_ATTR_KILL_SENT) {
		flags |= MSG_FLAGS_KILL_SENT;
	}
	if(ftn_attr & FTN_ATTR_SENT) {
		flags |= MSG_FLAGS_SENT;
	}
	if(ftn_attr & FTN_ATTR_RECV) {
		flags |= MSG_FLAGS_RECV;
	}

	return flags;
}

int FlagsToAttr(int flags) {
	int attr = 0;
	
	if(flags & MSG_FLAGS_PRIVATE) {
		attr |= FTN_ATTR_PVT;
	}
	if(flags & MSG_FLAGS_CRASH) {
		attr |= FTN_ATTR_CRASH;
	}
	if(flags & MSG_FLAGS_ATTACH) {
		attr |= FTN_ATTR_ATTACH;
	}
	if(flags & MSG_FLAGS_KILL_SENT) {
		attr |= FTN_ATTR_KILL_SENT;
	}
	if(flags & MSG_FLAGS_SENT) {
		attr |= FTN_ATTR_SENT;
	}
	if(flags & MSG_FLAGS_RECV) {
		attr |= FTN_ATTR_RECV;
	}

	return attr;
}

int ReadPktMsg(FILE* fh, MEM_MSG* mem) {

	PKT_MSG pktmsg;
	char buf[4096];
	char tmpbuf[128];
	int first_line = 0;
	char* ptr1, *ptr2;
	FidoAddr tmp1, tmp2;

	if(fread(&pktmsg, sizeof(PKT_MSG), 1, fh) != 1) {
		return -1;
	}

	mem->MsgOrig.zone = 0;
	mem->MsgOrig.node = (int) pktmsg.origNode;
	mem->MsgOrig.net = (int) pktmsg.origNet;
	mem->MsgOrig.point = 0;

	mem->MsgDest.zone = 0;
	mem->MsgDest.node = (int) pktmsg.destNode;
	mem->MsgDest.net = (int) pktmsg.destNet;
	mem->MsgDest.point = 0;
	mem->Flags = AttrToFlags((int) pktmsg.Attribute);

	NullRead(fh, buf, 64);
	StrToTm(buf, &mem->MsgDate);
	NullRead(fh, buf, 64);
	strncpy(mem->MsgTo, buf, 64);
	NullRead(fh, buf, 64);
	strncpy(mem->MsgFrom, buf, 64);
	NullRead(fh, buf, 72);
	strncpy(mem->Subject, buf, 72);
	
	while (CRRead(fh, buf, 4096) >= 0) {
		if (!strncmp("AREA:", buf, 5) && !first_line) {
			strncpy(mem->Area, buf+5, 64);
		}
		else if (!strncmp("\1FMPT", buf, 5) && mem->Area[0] == 0) {
			mem->MsgOrig.point = atoi(buf+6);
		}
		else if (!strncmp("\1TOPT", buf, 5) && mem->Area[0] == 0) {
			mem->MsgDest.point = atoi(buf+6);
		}
		else if(!strncmp("\1INTL", buf, 5) && mem->Area[0] == 0) {
			strcpy(tmpbuf, buf);

			ptr1 = strtok(tmpbuf+6, " ");
			ptr2 = strtok(NULL, " ");

			if(ptr1 && ptr2) {
				text_to_fidoaddr(ptr1, &tmp1);
				text_to_fidoaddr(ptr2, &tmp2);
				
				mem->MsgDest.zone = tmp1.zone;
				mem->MsgDest.net = tmp1.net;
				mem->MsgDest.node = tmp1.node;
				mem->MsgOrig.zone = tmp2.zone;
				mem->MsgOrig.net = tmp2.net;
				mem->MsgOrig.node = tmp2.node;
			}
		}

		bsList_add(&mem->Text, strdup(buf));

		if (!first_line) {
			first_line = 1;
		}
	}

	/* We hope we guess correctly */
	if(mem->MsgDest.zone == 0) {
		mem->MsgDest.zone = mem->PktDest.zone;
	}
	if(mem->MsgDest.point == 0) {
		mem->MsgDest.point = mem->PktDest.point;
	}
	if(mem->MsgOrig.zone == 0) {
		mem->MsgOrig.zone = mem->PktOrig.zone;
	}
	if(mem->MsgOrig.point == 0) {
		mem->MsgOrig.point = mem->PktOrig.point;
	}

	return 0;
}

int WritePktMsg(FILE* fh, MEM_MSG* mem) {
	PKT_MSG pktmsg;
	char buf[1024];
	bsListItem* item;

	memset(&pktmsg, '\0', sizeof(PKT_MSG));

	pktmsg.version = 2;

	pktmsg.origNode = mem->MsgOrig.node;
	pktmsg.origNet = mem->MsgOrig.net;
	pktmsg.destNode = mem->MsgDest.node;
	pktmsg.destNet = mem->MsgDest.net;
	pktmsg.Attribute = FlagsToAttr(mem->Flags);

	fwrite(&pktmsg, sizeof(PKT_MSG), 1, fh);

	TmToStr(&mem->MsgDate, buf);

	fputs(buf, fh);
	fputc('\0', fh);
	fputs(mem->MsgTo, fh);
	fputc('\0', fh);
	fputs(mem->MsgFrom, fh);
	fputc('\0', fh);
	fputs(mem->Subject, fh);
	fputc('\0', fh);

	for(item=mem->Text.first; item; item=item->next) {
		StripCrLf(item->val);
		fputs(item->val, fh);
		fputc('\r', fh);
	}

	fputc(0, fh);
	fputc(0, fh);
	fputc(0, fh);

	return 0;
}
