/*
 * DD-Echo
 * Export related routines
 * Author: Bo Simonsen <bo@geekworld.dk>
 */

#include "headers.h"

int init = 0;

void RandomFilename(char* buf, char* ext) {

	if (init == 0) {
		init = time(NULL);
		srand(init);
	}
	snprintf(buf, 64, "%08x.%s", rand(), ext);
	
}

void WritePkt(MEM_MSG* mem, FidoAddr* dest, Flav Flavour) {
	char pktname[128];
	FILE* fh;

	GetPktName(pktname, dest, Flavour, (mem->Area[0] == 0 ? 1 : 0));

	fh = fopen(pktname, "r+");

	if(fh) {
		fseek(fh, -2, SEEK_END);
	} else {
		fh = fopen(pktname, "w");
		WritePktHeader(fh, mem);
	}

	WritePktMsg(fh, mem);

	fclose(fh);
}

void AddVia(MEM_MSG* mem) {
	char buf[512];  /* Increased buffer size */
	time_t t;
	struct tm* tt;
	FidoAddr tmp;

	FindMyAka(&tmp, &mem->MsgDest);

	t = time(NULL);
	tt = gmtime(&t);

	snprintf(buf, sizeof(buf), "\1Via %s @%04d%02d%02d.%02d%02d%02d.UTC %s", fidoaddr_to_text(&tmp, NULL), tt->tm_year+1900, tt->tm_mon+1, tt->tm_mday, tt->tm_hour, tt->tm_min, tt->tm_sec, version);	
	bsList_add(&mem->Text, strdup(buf));
}

void AddTid(MEM_MSG* mem) {
	bsListItem* item;
	char tidstr[64];
	char* ptr;

	for(item=mem->Text.first; item; item=item->next) {
		ptr = (char*) item->val;

		if(ptr[0] != '\1' && strncasecmp(ptr, "AREA:", 5)) {
			break;
		}
	}

	snprintf(tidstr, sizeof(tidstr), "\1TID: %s", version);

	bsList_insertBefore(&mem->Text, item, strdup(tidstr));
}

int RouteMail(MEM_MSG* mem) {
	Route* route;
	Password* pw;
	bsListItem* item;
	char buf[256];
	int found = 0;
	time_t tmptime;
	struct tm* tmptm;
	Flav Flavour;
	FidoAddr Dest;
	int i;
	FILE* fh;

	if(mem->Area[0] == 0 && main_cfg.StarMsgNetmail[0] != 0) {
		i = msg_HighMsgNum(main_cfg.StarMsgNetmail) + 1;
		snprintf(buf, sizeof(buf), "%s/%d.msg", main_cfg.StarMsgNetmail, i);

		/* Not nessersary but for debugging it's nice to know weather the message
		 * passed by dd-echo */

		AddVia(mem);

		fh = fopen(buf, "w");
		msg_WriteMsg(fh, mem);
		fclose(fh);
	}
	else {

		if(mem->Area[0] == 0 && mem->Flags & MSG_FLAGS_CRASH) {
			memcpy(&Dest, &mem->MsgDest, sizeof(FidoAddr));
			Flavour = CRASH;
			logit(TRUE, "Crashing mail to %s\n", fidoaddr_to_text(&mem->MsgDest, buf));
		}
		else {
			for(item=main_cfg.Routes.first; item; item=item->next) {
				route = item->val;
				fidoaddr_to_text(&mem->MsgDest, buf);

				if(patmat(buf, route->Pattern)) {
					found = 1;
					break;
				}
			}

			if(!found) {
				logit(TRUE, "No route! (To: %s)\n", fidoaddr_to_text(&mem->MsgDest, NULL));
				return -1;
			}

			if(route->Type == ROUTE) {
				fidoaddr_to_text(&route->Dest, buf);
				logit(TRUE, "Routing via %s\n", buf);
				memcpy(&Dest, &route->Dest, sizeof(FidoAddr));
			} else {
				logit(TRUE, "Sending direct to %s\n", fidoaddr_to_text(&mem->MsgDest, buf));
				memcpy(&Dest, &mem->MsgDest, sizeof(FidoAddr));
			}

			Flavour = route->Flavour;
		}

		memcpy(&mem->PktDest, &Dest, sizeof(FidoAddr));
		FindMyAka(&mem->PktOrig, &Dest);

		pw = GetPassword(&Dest);

		if (pw != NULL) {
			strncpy(mem->PktPw, pw->Password, sizeof(mem->PktPw)-1);
			mem->PktPw[sizeof(mem->PktPw)-1] = '\0';
		} else {
			mem->PktPw[0] = '\0';
		}

		tmptime = time(NULL);
		tmptm = gmtime(&tmptime);
		memcpy(&mem->PktDate, tmptm, sizeof(struct tm));

		if(mem->Area[0] == 0) {
			AddVia(mem);
		}

		WritePkt(mem, &Dest, Flavour);
	}

	return 0;
}

int ExportMsg(MEM_MSG* mem) {
	bsListItem* item;
	Area* area;
	FidoAddr orig;
	char buf1[64], buf2[64];
	Export* export;
	MEM_MSG mem2;
	int retval;
	bsList seenbys;	

	if (!DestIsHere(&mem->MsgDest) && !fidoaddr_empty(&mem->MsgDest)) {
		logit(TRUE, "Msg #%d : Routing netmail %s (%s) --> %s (%s)", mem->MsgNum, mem->MsgFrom, fidoaddr_to_text(&mem->MsgOrig, buf1), mem->MsgTo, fidoaddr_to_text(&mem->MsgDest, buf2));
		
		CopyMemMsg(mem, &mem2);
		retval = RouteMail(&mem2);
		FreeMemMsg(&mem2);
		
		if(retval < 0) {
			return -1;
		}
	}
	else if(!strcasecmp(mem->Area, "")) {
		// Areafix stuff
	}
	else {
		area = GetArea(mem->Area);

		if(area == NULL) {
			logit(TRUE, "Can't find area (%s)\n", mem->Area);
			return -1;
		}

		orig = mem->MsgOrig;

		bsList_init(&seenbys);

		if(main_cfg.CheckSeenbys) {
			GetSeenbyList(mem, &seenbys);
		}

		for(item=area->SendTo.first; item; item=item->next) {
			export = item->val;

			if(!fidoaddr_compare(&export->Aka, &orig) && export->Send) {
				if(main_cfg.CheckSeenbys && 
				   export->Aka.point == 0 &&
				   Node2dExists(&seenbys, &export->Aka)) {
					logit(TRUE, "Receiver is in seenby");
					continue;
				}				

				logit(TRUE, "Msg #%d Area %s : Exporting --> %s", mem->MsgNum, mem->Area, fidoaddr_to_text((FidoAddr*) item->val, NULL));

				memset(&mem2, '\0', sizeof(MEM_MSG));

				CopyMemMsg(mem, &mem2);

				if(!fidoaddr_empty(&area->ourAka)) {
					memcpy(&mem2.MsgOrig, &area->ourAka, sizeof(FidoAddr));
				} else {
					FindMyAka(&mem2.MsgOrig, &mem->MsgDest);
				}

				memcpy(&mem2.MsgDest, item->val, sizeof(FidoAddr));

				AddSeenbyPath(&mem2, &area->SendTo);

				if(mem2.Flags & MSG_FLAGS_LOCAL && main_cfg.AddTid) {
					AddTid(&mem2);
				}

				retval = RouteMail(&mem2);
				FreeMemMsg(&mem2);

				if(retval < 0) {
					return -1;
				}

			}
		}

		bsList_free(&seenbys);
	}

	return 0;
}


