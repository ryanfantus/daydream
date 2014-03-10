/* 
 * DD-Echo
 * Areafix processing routines
 * Author: Bo Simonsen <bo@geekworld.dk>
 */

#include "headers.h"

struct UpdateAction {
	enum {ADD, DELETE} Action;
	union {
		struct EX_ {
			Area* Area;
			FidoAddr* Aka;
			int Send;
			int Recv;
		} EX;
	};
};

bsList update_list;

int AreafixHelp(bsList* text) {
	char buf[128];
	FILE* fh;
	
	fh = fopen(main_cfg.AreafixHelpfile, "r");
	
	if(!fh) {
		logit(TRUE, "(AREAFIX) Error: Can't open helpfile");
		return -1;
	}
	
	while(fgets(buf, 128, fh)) {
		StripCrLf(buf);
		bsList_add(text, strdup(buf));
	}
	
	return 0;
}

int AreafixRescan(FidoAddr* addr, bsList* text, char* area) {
	Area* a;
	MEM_MSG mem;
	DD_Area* ah;
	int msgs = 0;
	char buf[128];
	
	a = GetArea(area);
	
	if(area == NULL) {
		return -1;
	}

	ah = dd_OpenArea(a->Area, main_cfg.Conferences[a->Conference], a->Base);

	if(!ah) {
		printf("Can't open area\n");
		return -1;
	}

	while(dd_RealReadMsg(ah, 0, &mem) >= 0) {

		memcpy(&mem.MsgDest, addr, sizeof(FidoAddr));
		
		if(!fidoaddr_empty(&a->ourAka)) {
			memcpy(&mem.MsgOrig, &a->ourAka, sizeof(FidoAddr));
		} else {
			FindMyAka(&mem.MsgOrig, &mem.MsgDest);
		}

		AddSeenbyPath(&mem, &a->SendTo);

		if(mem.Flags & MSG_FLAGS_LOCAL && main_cfg.AddTid) {
			AddTid(&mem);
		}

		RouteMail(&mem);
		FreeMemMsg(&mem);
		
		++msgs;
	}
	
	dd_CloseArea(ah);
	
	sprintf (buf, "%-30.30s Rescanned %d msgs", area, msgs);
	bsList_add(text, strdup(buf));
	
	return 0;
}

int AreafixList(FidoAddr* addr, bsList* text) {

	bsListItem* item, *titem;
	Group* group;
	Area* area;
	char buf[128];

	bsList_add(text, strdup("Results of LIST command:"));
	bsList_add(text, strdup(""));

	for(item=main_cfg.Groups.first; item; item=item->next) {
		group = item->val;

		if(fidoaddr_compare(&group->Aka, addr)) {
			sprintf(buf, "Group '%c':", group->Group);
			bsList_add(text, strdup(buf));
			bsList_add(text, strdup(""));

			for(titem = main_cfg.Areas.first; titem; titem = titem->next) {
				area = titem->val;

				if(area->Group == group->Group) {
					sprintf(buf, " %s", area->Area);
					bsList_add(text, strdup(buf));
				}
			}

			bsList_add(text, strdup(""));
		}
	}	
	logit(TRUE, "(AREAFIX) List");
	return 0;
}

int AreafixUnlinked(FidoAddr* addr, bsList* text) {

	bsListItem* item, *titem, *ttitem;
	Group* group;
	Area* area;
	Export* export;
	char buf[128];

	bsList_add(text, strdup("Results of UNLINKED command:"));
	bsList_add(text, strdup(""));

	for(item=main_cfg.Groups.first; item; item=item->next) {
		group = item->val;

		if(fidoaddr_compare(&group->Aka, addr)) {
			sprintf(buf, "Group '%c':", group->Group);
			bsList_add(text, strdup(buf));
			bsList_add(text, strdup(""));

			for(titem = main_cfg.Areas.first; titem; titem = titem->next) {
				area = titem->val;
				
				for(ttitem = area->SendTo.first; ttitem; ttitem = ttitem->next) {
					export = ttitem->val;
					
					if(fidoaddr_compare(&export->Aka, addr)) {
						break;
					}
				}

				if(ttitem == NULL && area->Group == group->Group) {
					sprintf(buf, " %s", area->Area);
					bsList_add(text, strdup(buf));
				}
			}

			bsList_add(text, strdup(""));
		}
	}	
	logit(TRUE, "(AREAFIX) Unlinked");
	return 0;
}


int AreafixQuery(FidoAddr* addr, bsList* text) {
	bsListItem* item, *titem;
	Area* area;
	Export* export;
	char buf[128];

	bsList_add(text, strdup("Results of QUERY command:"));
	bsList_add(text, strdup(""));
	sprintf(buf, " %-30.30s S R NoRemove", "Area");
	bsList_add(text, strdup(buf));

	for(item = main_cfg.Areas.first; item; item = item->next) {
		area = item->val;

		for(titem=area->SendTo.first; titem; titem=titem->next) {
			export = titem->val;

			if(fidoaddr_compare(&export->Aka, addr)) {

				sprintf(buf, " %-30.30s %s %s %s", area->Area, export->Send ? "X" : " ",  export->Recv ? "X" : " ",export->NoRemove ? "X" : " ");
				bsList_add(text, strdup(buf));
				break;
			}
		}
	}	
	bsList_add(text, strdup(""));
	logit(TRUE, "(AREAFIX) Query");
	return 0;
}

int AreafixUnsub(FidoAddr* addr, char* sarea, bsList* text) {

	Area *area = NULL;
	Export* export = NULL;
	bsListItem* item;
	struct UpdateAction* action;
	char buf[128];

	for(item = main_cfg.Areas.first; item; item = item->next) {
		area = item->val;
	
		if(patmat(area->Area, sarea)) {
			export = GetExport(area, addr);

			if(export && !export->NoRemove) {
				action = NEW(struct UpdateAction);
				action->Action = DELETE;
				action->EX.Area = area;
				action->EX.Aka = addr;
				bsList_add(&update_list, action);
				sprintf(buf, "%-50.50s unsubscribed", area->Area);
				bsList_add(text, strdup(buf));
				logit(TRUE, "(AREAFIX) Unsubscribed from area %s", sarea);
			}
		}
	}
	
	return 0;
}

int createArea(ForwardList* fl, char* sarea) {
	char* cfgfn;
	FILE* fh;
	Area *area = NULL;
	bsListItem* item;
	char buf[128];
	char buf1[128];
	char buf2[128];
	char buf3[128];
	
	int max = 0;
	
	for(item = main_cfg.Areas.first; item; item = item->next) {
		area = item->val;
		
		if(area->Base > max && area->Conference == fl->Conference) {
			max = area->Base;
		}
	}
	
	++max;
	
	fidoaddr_to_text(&fl->ourAka, buf1);
	fidoaddr_to_text(&fl->Aka, buf2);
	sprintf(buf3, "Area %s %d:%d -g %c -a %s %s", sarea, fl->Conference, max, fl->Group, buf1, buf2);

	cfgfn = getenv("DDECHO");
	fh = fopen(cfgfn, "a");
	
	if(!fh) {
		logit(TRUE, "Can't open config file %s", cfgfn);
		return -1;
	}
	
	fputs(buf3, fh);
	fputc('\n', fh);
	fclose(fh);
	
	/* Let this function do the work for us to register the area */
	ParseCfgLine(&main_cfg, buf3);
	
	/* Create directory */
	sprintf(buf, "%s/messages/base%03d", main_cfg.Conferences[fl->Conference], max);
	mkdir(buf, MKDIR_DEFS);
	
	return 0;
}

int FindInForwardList(char* area, ForwardList* fl) {
	FILE* fh;
	char* ptr;
	char buf[128];

	fh = fopen(fl->List, "r");
	
	if(!fh) {
		logit(TRUE, "Can't open forward list %s", fl->List);
		return 0;
	}
			
	while(fgets(buf, 128, fh) != NULL) {
		ptr = strtok(buf, " ");
		
		if(!ptr) {
			continue;
		}
				
		if(!strcasecmp(ptr, area)) {
			fclose(fh);
			return TRUE;
		}
	}
	
	fclose(fh);
	
	return FALSE;

}

int AreafixSub(FidoAddr* addr, char* sarea, bsList* text) {
	
	bsListItem* item, *titem = NULL;
	Area* area;
	Group* group;
	bool found = FALSE;
	struct UpdateAction* action;
	char buf[128];
	char buf1[128];
	ForwardList* fl;
	int results = 0;

	for(item = main_cfg.Areas.first; item; item = item->next) {
		area = item->val;
	
		if(patmat(area->Area, sarea)) {

			if(GetExport(area, addr)) {
				sprintf(buf, "%-30.30s allready subscribed", area->Area);
				bsList_add(text, strdup(buf));
				continue;
			}

			found = FALSE;
			for(titem=main_cfg.Groups.first; titem; titem=titem->next) {
				group = titem->val;

				if(fidoaddr_compare(&group->Aka, addr) && area->Group == group->Group) {
					found = TRUE;
					break;
				}
			}

			if(!found) {
				continue;
			}

			action = NEW(struct UpdateAction);
			action->Action = ADD;
			action->EX.Area = area;
			action->EX.Aka = addr;

			action->EX.Send = group->Send;
			action->EX.Recv = group->Recv;

			bsList_add(&update_list, action);

			sprintf(buf, "%-30.30s successfully subscribed", area->Area);
			logit(TRUE, "(AREAFIX) Subscribed to area %s", sarea);
			bsList_add(text, strdup(buf));
			results++;
		}
	}
	
	/* Lets try to find the requested area in the arealist */
	if(results == 0) {
		for(item = main_cfg.ForwardLists.first; !found && item != NULL; item = item->next) {
			fl = item->val;

			for(titem=main_cfg.Groups.first; titem; titem=titem->next) {
				group = titem->val;

				if(fidoaddr_compare(&group->Aka, addr) && fl->Group == group->Group) {
					break;
				}
			}
			
			/* User is not in this group */
			if(!titem) {
				continue;
			}

			if(FindInForwardList(sarea, fl)) {
				logit(TRUE, "(AREAFIX) Creating area %s", sarea);
				if(createArea(fl, sarea) < 0) {
					logit(TRUE, "ERROR: Couldnt create area %s", sarea);
				}
				/* It's ok to call recursively, this would fall into the first case */
				AreafixSub(addr, sarea, text);
					
				logit(TRUE, "(AREAFIX) Forwarding request to %s for %s", fidoaddr_to_text(&fl->Aka, NULL), sarea);
				forwardRequest(&fl->Aka, &fl->ourAka, sarea);
					
				sprintf(buf1, "%-30.30s request forwarded", sarea);
				bsList_add(text, strdup(buf1));
					
				break;
			}
			
			
		}
	
	}

	return 0;
}

int ProcessAreafixText(MEM_MSG* mem, bsList* text) {

	bsListItem* item;
	char* ptr;
	int state = 0;

	for(item=mem->Text.first; item; item=item->next) {
		
		ptr = item->val;

		if(!strcasecmp(ptr, "---")) {
			break;
		}

		if(ptr[0] == '\1') {
			if(state)
				break;
			else
				continue;
		}
		else {
			state = 1;
		}

		if(ptr[0] == '%') {
			ptr++;

			if(!strncasecmp(ptr, "HELP", 4)) {
				AreafixHelp(text);
			}
			else if(!strncasecmp(ptr, "LIST", 4)) {
				AreafixList(&mem->MsgOrig, text);
			}
			else if(!strncasecmp(ptr, "QUERY", 5)) {
				AreafixQuery(&mem->MsgOrig, text);
			}
			else if(!strncasecmp(ptr, "LINKED", 6)) {
				AreafixQuery(&mem->MsgOrig, text);
			}
			else if(!strncasecmp(ptr, "UNLINKED", 8)) {
				AreafixUnlinked(&mem->MsgOrig, text);
			}			
			else if(!strncasecmp(ptr, "RESCAN", 6)) {
				AreafixRescan(&mem->MsgOrig, text, ptr+7);
			}
		}
		else if(ptr[0] == '-') {
			ptr++;
			AreafixUnsub(&mem->MsgOrig, ptr, text);
		}
		else {
			if(ptr[0] == '+') {
				ptr++;
			}

			AreafixSub(&mem->MsgOrig, ptr, text);
		}
	}

	return 0;
}

void UpdateCfg() {

	FILE* fh_r, *fh_w;
	char bakfn[128];
	char buf[512], tbuf[512], ttbuf[512], addrbuf[64];
	char* ptr, *ptr_;
	bsListItem* item;
	struct UpdateAction* a;
	char* cfgfn;

	cfgfn = getenv("DDECHO");

	sprintf(bakfn, "%s.bak", cfgfn);

	rename(cfgfn, bakfn);

	fh_r = fopen(bakfn, "r");
	fh_w = fopen(cfgfn, "w");

	while(fgets(buf, sizeof(buf), fh_r)) {
		StripCrLf(buf);
		strcpy(tbuf, buf);
		ptr = strtok(tbuf, " ");
		ptr_ = strtok(NULL, " ");

		if(ptr && ptr_ && !strcasecmp(ptr, "Area")) {
			for(item=update_list.first; item; item=item->next) {
				a = item->val;
				if(a->Action == ADD) {
					if(!strcasecmp(ptr_, a->EX.Area->Area)) {
						strcat(buf, " ");
						if(!a->EX.Send) {
							strcat(buf, "%");
							strcat(buf, fidoaddr_to_text(a->EX.Aka, NULL));
						}
						else if(!a->EX.Recv) {
							strcat(buf, "&");
							strcat(buf, fidoaddr_to_text(a->EX.Aka, NULL));
						}
						else {
							strcat(buf, fidoaddr_to_text(a->EX.Aka, NULL));
						}
						break;
					}
				}
				else if(a->Action == DELETE) {
					if(!strcasecmp(ptr_, a->EX.Area->Area)) {

						strcpy(ttbuf, ptr);
						strcat(ttbuf, " ");
						strcat(ttbuf, ptr_);
						strcat(ttbuf, " ");
						fidoaddr_to_text(a->EX.Aka, addrbuf);

						while((ptr_ = strtok(NULL, " "))) {
							if(strcasecmp(addrbuf, ptr_) && strcasecmp(addrbuf, ptr_+1)) {
								strcat(ttbuf, ptr_);
								strcat(ttbuf, " ");
							}
						}
						strcpy(buf, ttbuf);
						break;
					}
				}
			}
		}

		fputs(buf, fh_w);
		fputc('\n', fh_w);
	}

	fclose(fh_r);
	fclose(fh_w);
}

void addHeaders(MEM_MSG* mem, bsList* text) {
	char buf[128];

	sprintf(buf, "\1INTL %d:%d/%d %d:%d/%d", mem->MsgDest.zone, mem->MsgDest.net, mem->MsgDest.node, mem->MsgOrig.zone, mem->MsgOrig.net, mem->MsgOrig.node);
	bsList_insertBefore(text, text->first, strdup(buf)); 

	sprintf(buf, "\1PID: %s", version);
	bsList_insertAfter(text, text->first, strdup(buf));

	if(mem->MsgOrig.point != 0) {
		sprintf(buf, "\1FMPT %d", mem->MsgOrig.point);
		bsList_insertAfter(text, text->first, strdup(buf));
	}
	if(mem->MsgDest.point != 0) {
		sprintf(buf, "\1TOPT %d", mem->MsgDest.point);
		bsList_insertAfter(text, text->first, strdup(buf));
	}


	sprintf(buf, "\1MSGID: %s %08x", fidoaddr_to_text(&mem->MsgOrig, NULL), (int) time(NULL));
	bsList_insertAfter(text, text->first, strdup(buf)); 


	sprintf(buf, "--- %s", version);
	bsList_add(text, strdup(buf));
}


void SendAreafixResponse(MEM_MSG* mem, MEM_MSG* result_mem, bsList* text) {
	struct tm* tt;
	time_t t;

	memset(result_mem, '\0', sizeof(MEM_MSG));

	strcpy(result_mem->MsgFrom, "Areafix");
	strcpy(result_mem->MsgTo, mem->MsgFrom);
	strcpy(result_mem->Subject, "Areafix response");

	result_mem->Flags = MSG_FLAGS_LOCAL;

	memcpy(&result_mem->MsgDest, &mem->MsgOrig, sizeof(FidoAddr));
	memcpy(&result_mem->MsgOrig, &mem->MsgDest, sizeof(FidoAddr));

	t = time(NULL);
	tt = localtime(&t);

	memcpy(&result_mem->MsgDate, tt, sizeof(struct tm));
	memcpy(&result_mem->Text, text, sizeof(bsList));

	addHeaders(result_mem, &result_mem->Text);

}

int forwardRequest(FidoAddr* Aka, FidoAddr* ourAka, char* area) {
	struct tm* tt;
	time_t t;
	bsListItem* item;
	Password* pw = NULL;
	MEM_MSG mem;

	memset(&mem, '\0', sizeof(MEM_MSG));

	for(item=main_cfg.AreafixPws.first; item; item=item->next) {
		pw = item->val;

		if(fidoaddr_compare(Aka, &pw->Aka)) {
			break;
		}
	}

	if(!pw) {
		return -1;
	}

	strcpy(mem.MsgFrom, version);
	strcpy(mem.MsgTo, "Areafix");
	strcpy(mem.Subject, pw->Password);

	mem.Flags = MSG_FLAGS_LOCAL;

	memcpy(&mem.MsgDest, Aka, sizeof(FidoAddr));
	memcpy(&mem.MsgOrig, ourAka, sizeof(FidoAddr));

	t = time(NULL);
	tt = localtime(&t);

	memcpy(&mem.MsgDate, tt, sizeof(struct tm));
	
	bsList_add(&mem.Text, strdup(area));
	
	addHeaders(&mem, &mem.Text);
	ExportMsg(&mem);
	FreeMemMsg(&mem);

	return 0;
}


int ProcessAreafix(MEM_MSG* mem, MEM_MSG* result_mem) {
	bsList text;
	bool found = FALSE;
	bsListItem* item;
	Password* pw;

	memset(&text, '\0', sizeof(bsList));
	memset(&update_list, '\0', sizeof(bsList));

	for(item=main_cfg.AreafixPws.first; item; item=item->next) {
		pw = item->val;

		if(fidoaddr_compare(&mem->MsgOrig, &pw->Aka) && !strcmp(mem->Subject, pw->Password)) {
			found = TRUE;
		}
	}

	if(found) {

		ProcessAreafixText(mem, &text);

		if(update_list.first) {
			UpdateCfg();
		}
	}
	else {
		bsList_add(&text, strdup("Security violation!"));
		logit(TRUE, "(AREAFIX) Security violation");
	}

	if(text.first) {
		SendAreafixResponse(mem, result_mem, &text);
	}

	return 0;
}
