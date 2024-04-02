/* 
 * DD-Echo 
 * Main routines scan, toss, pack
 * Author: Bo Simonsen <bo@geekworld.dk>
 */

#include "headers.h"

const char* version = "DD-Echo/" UNAME " 1.0";

MainCfg main_cfg;
bsList arc_list;
bsList stat_list;

const char* unknown_product="Unknown productcode";

char* FindProduct(unsigned int prod) {
	int i;
	for(i=0; products[i].name != 0 ; i++) {
		if(products[i].adr == prod)
			return products[i].name;
	}
	return (char*) unknown_product;
}

int CheckPktHeader(MEM_MSG* mem) {
	Password* pw;

	if(main_cfg.InsecurePkt) {
		return 0;
	}

	if(!DestIsHere(&mem->PktDest)) {
		logit(TRUE, "Destination is not here! (%s)\n", fidoaddr_to_text(&mem->PktDest, NULL));
		return -1;
	}

	if (strcasecmp(mem->PktPw, "")) {
		pw = GetPassword(&mem->PktOrig);
		if(pw == NULL) {
			logit(TRUE, "Link isn't known (%s)\n", fidoaddr_to_text(&mem->PktOrig, NULL));
			return -1;
		}
		else if(strcasecmp(pw->Password, mem->PktPw) > 0) {
			logit(TRUE, "Password doesn't match! (pkt: %s, loc: %s)\n", mem->PktPw, pw->Password);
			return -1;
		}
	}

	return 0;
}

Area_Type CheckPktMsg(MEM_MSG* mem, int type) {

	Area* area;
	bsListItem* item;
	ForwardList* fl;
	Export* export;
	bool found;
	char buf[64];


	// Now checks for Strict Inbound Checking for protected only mail
	if(main_cfg.StrictInboundChecking && type == IN_NP && mem->Area[0]) {
		logit(TRUE, "Echomail in non proceted inbound!");
		return BAD;
	}


	if(!DestIsHere(&mem->MsgDest) &&
	   (main_cfg.ForwardFrom.first != NULL || main_cfg.ForwardTo.first != NULL)) {
		found = FALSE;

		for(item=main_cfg.ForwardFrom.first; item; item=item->next) {
			fidoaddr_to_text(&mem->MsgOrig, buf);

			if(patmat(buf, (char*) item->val)) {
				found = TRUE;
				break;
			}
		}

		for(item=main_cfg.ForwardTo.first; item; item=item->next) {
			fidoaddr_to_text(&mem->MsgDest, buf);

			if(patmat(buf, (char*) item->val)) {
				found = TRUE;
				break;
			}
		}

		if(!found) {
			logit(TRUE, "Sender/Reciever doesn't match forward rules!");
			return BAD;
		}
	}

	if(mem->Area[0] && DestIsHere(&mem->MsgDest)) {
		area = GetArea(mem->Area);

		if(area == NULL) {
			/* Lets see if the area is in the forwardlist, then we will add it! */
			for(item = main_cfg.ForwardLists.first; item; item = item->next) {
				fl = item->val;
				
				/* We will only autocreate if we are recieving
				   from the link defined in ForwardList */
				if(!fidoaddr_compare(&fl->Aka, &mem->MsgOrig)) {
					continue;
				}
				
				if(FindInForwardList(mem->Area, fl)) {
					createArea(fl, mem->Area);
					area = GetArea(mem->Area);
					logit(TRUE, "Created area %s in group %c from %s", mem->Area, fl->Group, fidoaddr_to_text(&fl->Aka, NULL));
					break;
				}
			}
			/* It wasn't let us put it in bad */
			if(area == NULL) {
				logit(TRUE, "Area is not known (%s)\n", mem->Area);
				return BAD;
			}
		}
		if(!main_cfg.InsecureLink) {
			export = GetExport(area, &mem->MsgOrig);

			if(!export) {
				logit(TRUE, "Sender is not in export list %s (%s)\n", fidoaddr_to_text(&mem->MsgOrig, NULL), mem->Area);
				return BAD;
			}

			if(!export->Recv) {
				logit(TRUE, "We'll not recieve from link!");
				return BAD;
			}
		}

		if(main_cfg.DupeLog[0] != 0 && CheckDupe(mem)) {
			logit(TRUE, "Dupe message from %s in %s with subject \"%s\"", mem->MsgFrom, mem->Area, mem->Subject);
			return main_cfg.KillDupes ? KILL : DUPE;
		}
	}

	if(!DestIsHere(&mem->MsgDest)) {
		return NETMAIL;
	}

	return mem->Area[0] ? ECHOMAIL : NETMAIL;

}

void WriteNewHeaders(MEM_MSG* mem) {
	bsListItem* item;
	char buf1[64];
	char buf2[64];

	for(item=mem->Text.first; item; item=item->next) {
		if(!strncmp(item->val, "\1INTL", 5)) {
			sprintf(item->val, "\1INTL %s %s", fidoaddr_to_text(&mem->MsgDest, buf1), fidoaddr_to_text(&mem->MsgOrig, buf2));
		}
		else if(!strncmp(item->val, "\1TOPT", 5)) {
			sprintf(item->val, "\1TOPT %d", mem->MsgDest.point);
		}
	}
}

void PrepareMsg(MEM_MSG* mem) {

	/* Strip crash and stuff */

	if(mem->Flags & MSG_FLAGS_CRASH) {
		mem->Flags &= ~MSG_FLAGS_CRASH;
	}
	if(mem->Flags & MSG_FLAGS_KILL_SENT) {
		mem->Flags &= ~MSG_FLAGS_KILL_SENT;
	}

	/* If we should kill intransit mail lets do */
	if(main_cfg.KillTransit && !DestIsHere(&mem->MsgDest)) {
		mem->Flags |= MSG_FLAGS_KILL_SENT;
	}

	/* Add to dupe */
	if(main_cfg.DupeLog[0] != 0 && mem->Area[0]) {
		AddDupe(mem);
	}
}

int toss_file(char* fn, int type) {
	MEM_MSG mem;
	FILE *fh;
	Area *area = NULL;
	DD_Area* a;
	char buf1[64], buf2[64];
	Area_Type retval;
	char* pkt_fn;

	pkt_fn = strrchr(fn, '/');
	
	if(pkt_fn == 0) {
	    logit(TRUE, "System error");
	    exit(1);
	}
	
	++pkt_fn;

	fh = fopen(fn, "r");

	if(fh == NULL) {
		logit(TRUE, "Can't open file! (%s)\n", fn);
		return -1;
	}

	memset(&mem, '\0', sizeof(MEM_MSG));

	ReadPktHeader(fh, &mem);

	if (type != IN_L) {
		if (CheckPktHeader(&mem) < 0) {
			return -1;
		}
	}
	
	logit(TRUE, "* Packet=%s, Date=%s, Pw=%s\n", pkt_fn, TmToStr(&mem.PktDate, NULL), mem.PktPw[0] ? "Yes" : "No");
	logit(TRUE, "* Orig=%s, Dest=%s, Type=%d%s, By=%s %d.%d\n", fidoaddr_to_text(&mem.PktOrig, buf1), fidoaddr_to_text(&mem.PktDest, buf2), mem.PktType, mem.Stoneage ? " (Stoneage)" : "+", FindProduct(mem.Product), mem.Major_ver, mem.Minor_ver);
	
	while(ReadPktMsg(fh, &mem) >= 0) {

		switch((retval = CheckPktMsg(&mem, type))) {

			case BAD:
			case DUPE:
				area = FindAreaByType(retval, NULL);
				a = dd_OpenArea(area->Area, main_cfg.Conferences[area->Conference], area->Base);
				dd_RealWriteMsg(a, 0, &mem, 1);
				dd_CloseArea(a);
				AddToStat(area->Area);
				break;
			case NETMAIL:
				logit(TRUE, "Importing netmail %s (%s) --> %s (%s)", mem.MsgFrom, fidoaddr_to_text(&mem.MsgOrig, buf1), mem.MsgTo, fidoaddr_to_text(&mem.MsgDest, buf2));
				area = FindAreaByType(NETMAIL, &mem.MsgDest);
				PrepareMsg(&mem);
				a = dd_OpenArea(area->Area, main_cfg.Conferences[area->Conference], area->Base);
				dd_RealWriteMsg(a, 0, &mem, 1);
				dd_CloseArea(a);
				break;
			case ECHOMAIL:
				area = GetArea(mem.Area);
				PrepareMsg(&mem);
				a = dd_OpenArea(area->Area, main_cfg.Conferences[area->Conference], area->Base);
				if(a == NULL) {
					logit(TRUE, "Cannot open area (%s, %d:%d)", mem.Area, area->Conference, area->Base);
					exit(1);
				}
				dd_RealWriteMsg(a, 0, &mem, 1);
				dd_CloseArea(a);
				AddToStat(mem.Area);
				break;
			case KILL:
				logit(TRUE, "Message is killed");
				break;
		}

		FreeMemMsg(&mem);

	}

	fclose(fh);

	return 0;
}

int afix_scan() {
	MEM_MSG mem;
	Area* area;
	bsListItem* item, *titem;
	DD_Area* a;
	MEM_MSG result_mem;
	
	printf("Areafix Scanning\n");

	for(item=main_cfg.Areas.first; item; item=item->next) {
		area = item->val;

		if (area->Type != NETMAIL) {
			continue;
		}

		printf("Scanning %s\n", area->Area);

		a = dd_OpenArea(area->Area, main_cfg.Conferences[area->Conference], area->Base);

		while(dd_RealReadMsg(a, 0, &mem) >= 0) {

			if (mem.Flags & MSG_FLAGS_DELETED || !DestIsHere(&mem.MsgDest) || mem.Flags & MSG_FLAGS_RECV) {
				continue;
			}


			for(titem=main_cfg.AreafixNames.first; titem; titem=titem->next) {
				if(!strcasecmp(titem->val, mem.MsgTo)) {
					logit(TRUE, "Processing #%d: Areafix Msg from: %s (%s)", mem.MsgNum, mem.MsgFrom, fidoaddr_to_text(&mem.MsgOrig, NULL));
					if(!ProcessAreafix(&mem, &result_mem)) {
						if(main_cfg.KillAreafixResp) 
							result_mem.Flags |= MSG_FLAGS_KILL_SENT;

						dd_RealWriteMsg(a, 0, &result_mem, 1);

						if(main_cfg.KillAreafixReq)
							mem.Flags |= MSG_FLAGS_DELETED;

						mem.Flags |= MSG_FLAGS_RECV;
						dd_RealWriteMsg(a, mem.MsgNum, &mem, 0);
						break;
					}
				}
			}

			FreeMemMsg(&mem);
		}

		dd_CloseArea(a);
	}

	return 0;

}

int scan(int echo) {
	MEM_MSG mem;
	Area* area;
	bsListItem* item;
	DD_Area* a;
	
	if(echo)
		logit(TRUE, "Scanning");
	else
		logit(TRUE, "Packing");
	
	for(item=main_cfg.Areas.first; item; item=item->next) {
		area = item->val;

		if (echo) {
			if(area->Type != ECHOMAIL) {
				continue;
			}
		}
		else {
			if (area->Type != NETMAIL) {
				continue;
			}
		}

		printf("Scanning %s (%d:%d)\n", area->Area, area->Conference, area->Base);
		a = dd_OpenArea(area->Area, main_cfg.Conferences[area->Conference], area->Base);

		if(!a) {
			logit(TRUE, "Can't open area\n");
			continue;
		}

		while(dd_RealReadMsg(a, 0, &mem) >= 0) {

			if ((mem.Flags & MSG_FLAGS_EXPORTED) || mem.Flags & MSG_FLAGS_DELETED) {
				continue;
			}

			/*printf("Processing #%d\n", mem.MsgNum);*/

			if(ExportMsg(&mem) >= 0) {
				if(mem.Flags & MSG_FLAGS_KILL_SENT) {
					mem.Flags |= MSG_FLAGS_DELETED;
				}
				mem.Flags |= MSG_FLAGS_EXPORTED;
				dd_RealWriteMsg(a, mem.MsgNum, &mem, 0);
			}

			FreeMemMsg(&mem);
		}

		dd_CloseArea(a);
	}

	return 0;
}

int toss_from_bad() {
	MEM_MSG mem;
	Area* area, *area2;
	bsListItem* item;
	DD_Area* a, *a2;

	logit(TRUE, "Tossing from bad");

	for(item=main_cfg.Areas.first; item; item=item->next) {
		area = item->val;

		if (area->Type != BAD) {
			continue;
		}


		a = dd_OpenArea(area->Area, main_cfg.Conferences[area->Conference], area->Base);

		while(dd_RealReadMsg(a, 0, &mem) >= 0) {

			if (mem.Flags & MSG_FLAGS_DELETED) {
				continue;
			}

			logit(TRUE, "Processing msg #%d\n", mem.MsgNum);

			switch(CheckPktMsg(&mem, IN_P)) {
				case NETMAIL:
				case ECHOMAIL:
					area2 = GetArea(mem.Area);
					a2 = dd_OpenArea(area2->Area, main_cfg.Conferences[area2->Conference], area2->Base);

					if(dd_RealWriteMsg(a2, 0, &mem, 1) >= 0) {
						/* Delete */
						mem.Flags |= MSG_FLAGS_DELETED;
						dd_RealWriteMsg(a, mem.MsgNum, &mem, 0);
						AddToStat(mem.Area);
					}
					else {
						logit(TRUE, "Error writing message\n");
					}

					dd_CloseArea(a2);
					break;
				default:
					break;
			}

			FreeMemMsg(&mem);
		}

		dd_CloseArea(a);
	}

	return 0;
}

void SetEnv(char* indir) {
	setenv("INBOUND", indir, 1);
	setenv("OUTBOUND", main_cfg.Outbound, 1);
}


int toss_dir(char* indir, int type) {
	DIR* dir;
	struct dirent* de;
	char fn[128], ffn[512];
	char buf[512];
	char* file, *ext;

	logit(TRUE, "Tossing from %s", indir);

	if(main_cfg.BeforeToss[0]) {
		SetEnv(indir);
		if(system(main_cfg.BeforeToss) != 0) {
			logit(TRUE, "BeforeToss exited with non-zero status!");
			exit(1);
		}
	}

	dir = opendir(indir);

	if(dir == NULL) {
		logit(TRUE, "Can't open inbound!\n");
		return -1;
	}

	while((de = readdir(dir))) {
		
		strcpy(fn, de->d_name);

		file = strtok(fn, ".");
		ext = strtok(NULL, "");

		if (file == NULL || ext == NULL) {
			continue;
		}

		if (!strcasecmp(ext, "pkt")) {
			sprintf(ffn, "%s/%s", indir, de->d_name);

			if(toss_file(ffn, type) >= 0) {
				unlink(ffn);
			}
			else {
				logit(TRUE, "Moving to Bad pkt dir");
				sprintf(buf, "%s/%s", main_cfg.BadPktDir, de->d_name);
				rename(ffn, buf);
			}
		}
	}

	if(main_cfg.AfterToss[0]) {
		SetEnv(indir);
		if(system(main_cfg.AfterToss) != 0) {
			logit(TRUE, "BeforeToss exited with non-zero status!");
			exit(1);
		}
	}

	return 0;
}

int toss() {
	toss_dir(main_cfg.Inbound, IN_NP);
	toss_dir(main_cfg.SecInbound, IN_P);
	toss_dir(main_cfg.LocInbound, IN_L);

	return 0;
}

#define COMMAND_SCAN 1
#define COMMAND_TOSS 2
#define COMMAND_TOSSBAD 4
#define COMMAND_PACK 8
#define COMMAND_AFIXSCAN 16

const char* help[] = {"TOSS - Searches inbound for pkt files",
               "TOSSBAD - Toss from bad echos",
               "SCAN - Exporting echomail",
               "PACK - Routing and packing netmail",
               "AFIX - Scan for Areafix requests"};
int help_len = 5;
         

int main(int argc, char* argv[]) {
	int i;
	int cmd = 0;
	char wd[128];

	if(!getcwd(wd, 128)) {
		printf("getcwd() == NULL\n");
		exit(1);
	}

	printf("%s\n", version);
        printf("The net- and echomail processor for Daydream BBS\n");
	printf("Copyright Bo Simonsen, 2008,2009,2010\n\n");

	bsList_init(&arc_list);
	bsList_init(&stat_list);

	for(i=0; i < argc; i++) {
		if (!strcasecmp(argv[i], "toss")) {
			cmd |= COMMAND_TOSS;
		}
		else if (!strcasecmp(argv[i], "scan")) {
			cmd |= COMMAND_SCAN;
		}
		else if (!strcasecmp(argv[i], "tossbad")) {
			cmd |= COMMAND_TOSSBAD;
		}
		else if (!strcasecmp(argv[i], "pack")) {
			cmd |= COMMAND_PACK;
		}
		else if (!strcasecmp(argv[i], "afix")) {
			cmd |= COMMAND_AFIXSCAN;
		}
	}
	if(cmd == 0) {
                printf("Possible commands are:\n\n");
                for(i=0; i < help_len; ++i) {
                	printf("  %s\n", help[i]);
                }
		printf("\nSet the config file using environment variable DDECHO\n\n");
		exit(1);
        }

	if(ReadCfg(&main_cfg, getenv("DDECHO")) == NULL) {
		printf("Can't find configfile\n");
		return -1;
	}

	BeginLog(main_cfg.LogFile);

	if (cmd & COMMAND_TOSSBAD) {
		toss_from_bad();
		PrintStats();
	}
	if (cmd & COMMAND_TOSS) {
		toss();
		PrintStats();
	}
	if (cmd & COMMAND_SCAN) {
		scan(1);
	}
	if (cmd & COMMAND_AFIXSCAN) {
		afix_scan();
	}
	if (cmd & COMMAND_PACK) {
		scan(0);
	}

	Arc();

	bsList_free(&stat_list);
	FreeCfg(&main_cfg);

	if(chdir(wd) != 0) {
		logit(TRUE, "chdir error!");
	}
	
	EndLog();

	return 0;
}
