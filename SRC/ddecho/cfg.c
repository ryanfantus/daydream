/*
 * DD-Echo
 * Configuration parser
 * Author: Bo Simonsen <bo@geekworld.dk>
 */

#include "headers.h"

void StripCrLf(char* text) {
	do {
		if (*text == '\n' || *text == '\r' ) {
			*text = 0;
		}
	} while(*text++);
}

void FreeCfg(MainCfg* cfg) {
	bsList_free(&cfg->Akas);
	bsList_free(&cfg->Passwords);
	bsList_free(&cfg->AreafixPws);
	bsList_free(&cfg->Groups);
	bsList_free(&cfg->Routes);
	bsList_free(&cfg->Packers);
	bsList_free(&cfg->PackRules);
	bsList_free(&cfg->Areas);
	bsList_free(&cfg->ForwardFrom);
	bsList_free(&cfg->ForwardTo);
	bsList_free(&cfg->TinySeenbys);
	bsList_free(&cfg->AreafixNames);
	bsList_free(&cfg->ForwardLists);
}

FidoAddr* FindMyAka(FidoAddr* my_aka, FidoAddr* dest) {
	bsListItem* item;
	FidoAddr* aka;

	for(item=main_cfg.Akas.first; item; item=item->next) {
		aka = (FidoAddr*) item->val;
		if (aka->zone == dest->zone) {
			memcpy(my_aka, aka, sizeof(FidoAddr));
			return my_aka;
		}
	}

	memcpy(my_aka, main_cfg.Akas.first->val, sizeof(FidoAddr));
	return my_aka;

}


Export* GetExport(Area* area, FidoAddr* aka) {
	bsListItem* item;
	Export* export;

	for(item=area->SendTo.first; item; item=item->next) {
		export = item->val;

		if(fidoaddr_compare(&export->Aka, aka)) {
			return export;
		}
	}
	
	return NULL;
}

Area* GetArea(char* area) {
	bsListItem* item;
	Area* a = NULL;

	for(item=main_cfg.Areas.first; item; item=item->next) {
		a = item->val;
		if (!strcasecmp(a->Area, area)) {
			return a;
		}
	}

	return NULL;

}

Area* FindAreaByType(Area_Type Type, FidoAddr *aka) {
	Area* area = NULL;
	Area* tmp = NULL;
	bsListItem* item;

	for(item=main_cfg.Areas.first; item; item=item->next) {
		area = item->val;

		if(area->Type == Type) {
			if(!aka ||
                           (aka && 
                            fidoaddr_compare(&area->ourAka, aka))) {
				return area;			
			} 
			else {
				tmp = area;
			}
		}

	}

	if(aka) {
		return tmp;
	}

	return NULL;
}


Password* GetPassword(FidoAddr* aka) {
	Password* pw;
	bsListItem* item;

	for(item=main_cfg.Passwords.first; item; item=item->next) {
		pw = item->val;
		if (fidoaddr_compare(aka, &pw->Aka)) {
			return pw;
		}
	}

	return NULL;
}

Packer* GetPacker(FidoAddr* dest) {
	PackRule* rule;
	bsListItem* item;
	char buf[128];

	for(item=main_cfg.PackRules.first; item; item=item->next) {
		rule = item->val;
		fidoaddr_to_text(dest, buf);
		if (patmat(buf, rule->Pattern)) {
			return rule->PackWith;
		}
	}

	return NULL;
}

int DestIsHere(FidoAddr* aka) {
	int found = 0;
	bsListItem* item;

	for(item=main_cfg.Akas.first; item; item=item->next) {
		if (fidoaddr_compare(aka, (FidoAddr*) item->val)) {
			found = 1;
			break;
		}
	}

	return found;
}

int ParseCfgLine(MainCfg* cfg, char* buf) {
	FidoAddr* tmpaddr;
	Password* tmppw;
	Route* tmproute, *tmproute2;
	Packer* tmppack;
	PackRule* tmppackrule, *tmppackrule2;
	Export* tmpexport;
	Area* tmparea;
	Group* tmpgroup;
	ForwardList* tmpfl;
	char *pos=NULL, *pos1=NULL, *pos2=NULL, *pos3=NULL, *pos4=NULL, *pos5=NULL;
	bsListItem* item;
	int i;

	pos = strtok(buf, " ");
	pos1 = strtok(NULL, " ");

	if (pos == NULL) {
		return 0;
	}
	if (pos[0] == ';' || pos[0] == '#') {
		return 0;
	}

	if(!strcasecmp(pos, "Aka")) {
		tmpaddr = NEW(FidoAddr);
		text_to_fidoaddr(pos1, tmpaddr);
		bsList_add(&cfg->Akas, tmpaddr);
	}
	else if(!strcasecmp(pos, "AreafixHelpFile")) {
		strncpy(cfg->AreafixHelpfile, pos1, 128);
	}
	else if(!strcasecmp(pos, "Inbound")) {
		strncpy(cfg->Inbound, pos1, 128);
	}
	else if(!strcasecmp(pos, "SecInbound")) {
		strncpy(cfg->SecInbound, pos1, 128);
	}
	else if(!strcasecmp(pos, "LocInbound")) {
		strncpy(cfg->LocInbound, pos1, 128);
	}
	else if(!strcasecmp(pos, "BadPktDir")) {
		strncpy(cfg->BadPktDir, pos1, 128);
	}		
	else if(!strcasecmp(pos, "Outbound")) {
		strncpy(cfg->Outbound, pos1, 128);
	} 
	else if(!strcasecmp(pos, "StrictInboundChecking")) {
		cfg->StrictInboundChecking = TRUE;
	}
	else if(!strcasecmp(pos, "DupeLog")) {
		strncpy(cfg->DupeLog, pos1, 128);
	}
	else if(!strcasecmp(pos, "LogFile")) {
		strncpy(cfg->LogFile, pos1, 128);
	} 
	else if(!strcasecmp(pos, "BeforeToss")) {
		strncpy(cfg->BeforeToss, pos1, 128);
	}
	else if(!strcasecmp(pos, "AfterToss")) {
		strncpy(cfg->AfterToss, pos1, 128);
	} 
	else if(!strcasecmp(pos, "StarMsgNetmail")) {
		strncpy(cfg->StarMsgNetmail, pos1, 128);
	} 
	else if(!strcasecmp(pos, "KillTransit")) {
		cfg->KillTransit = TRUE;
	}
	else if(!strcasecmp(pos, "KillDupes")) {
		cfg->KillDupes = TRUE;
	}
	else if(!strcasecmp(pos, "KillAreafixReq")) {
		cfg->KillAreafixReq = TRUE;
	}
	else if(!strcasecmp(pos, "KillAreafixResp")) {
		cfg->KillAreafixResp = TRUE;
	}
	else if(!strcasecmp(pos, "PackNetmail")) {
		cfg->PackNetmail = TRUE;
	}
	else if(!strcasecmp(pos, "AddTid")) {
		cfg->AddTid = TRUE;
	}
	else if(!strcasecmp(pos, "InsecurePkt")) {
		cfg->InsecurePkt = TRUE;
	}
	else if(!strcasecmp(pos, "InsecureLink")) {
		cfg->InsecureLink = TRUE;
	}
	else if(!strcasecmp(pos, "CheckSeenbys")) {
		cfg->CheckSeenbys = TRUE;
	}
	else if(!strcasecmp(pos, "DefZone")) {
		cfg->DefZone = atoi(pos1);
	}
	else if(!strcasecmp(pos, "MaxArcSize")) {
		cfg->MaxArcSize = atoi(pos1);
	}
	else if(!strcasecmp(pos, "MaxPktSize")) {
		cfg->MaxPktSize = atoi(pos1);
	}
	else if(!strcasecmp(pos, "Conference")) {
		pos2 = strtok(NULL, " ");
		strcpy(cfg->Conferences[atoi(pos1)], pos2);
	}
	else if(!strcasecmp(pos, "Password")) {
		pos2 = strtok(NULL, " ");
		tmppw = NEW(Password);
		text_to_fidoaddr(pos1, &tmppw->Aka);
		strncpy(tmppw->Password, pos2, 8);
		tmppw->Password[8] = '\0';  /* Ensure null termination */
		bsList_add(&cfg->Passwords, tmppw);
	}
	else if(!strcasecmp(pos, "AreafixPw")) {
		pos2 = strtok(NULL, " ");
		tmppw = NEW(Password);
		text_to_fidoaddr(pos1, &tmppw->Aka);
		strncpy(tmppw->Password, pos2, 8);
		tmppw->Password[8] = '\0';  /* Ensure null termination */
		bsList_add(&cfg->AreafixPws, tmppw);
	}
	else if(!strcasecmp(pos, "Group")) {
		pos2 = strtok(NULL, " ");
		tmpgroup = NEW(Group);
		text_to_fidoaddr(pos1, &tmpgroup->Aka);
		tmpgroup->Group = pos2[0];
		pos3 = strtok(NULL, "");
		if(pos3) {
			if(tolower(pos3[0]) == 'S' && tolower(pos3[1]) == 'R') {
				tmpgroup->Send = TRUE;
				tmpgroup->Recv = TRUE;
			}
			else if(tolower(pos[0]) == 'S') {
				tmpgroup->Send = TRUE;
				tmpgroup->Recv = FALSE;
			}
			else if(tolower(pos[0]) == 'R') {
				tmpgroup->Send = FALSE;
				tmpgroup->Recv = TRUE;
			}
			else {
				tmpgroup->Send = FALSE;
				tmpgroup->Recv = FALSE;
			}
		}

		bsList_add(&cfg->Groups, tmpgroup);
	}
	else if(!strcasecmp(pos, "ForwardList")) {
		pos2 = strtok(NULL, " ");
		pos3 = strtok(NULL, " ");
		pos4 = strtok(NULL, " ");
		pos5 = strtok(NULL, " ");
		tmpfl = NEW(ForwardList);
		strncpy(tmpfl->List, pos1, 64);
		text_to_fidoaddr(pos2, &tmpfl->ourAka);
		text_to_fidoaddr(pos3, &tmpfl->Aka);
		tmpfl->Group = pos4[0];
		tmpfl->Conference = atoi(pos5);
		bsList_add(&cfg->ForwardLists, tmpfl);
	}
	else if(!strcasecmp(pos, "AreafixName")) {
		bsList_add(&cfg->AreafixNames, strdup(pos1));
		for(pos2 = strtok(NULL, " "); pos2; pos2 = strtok(NULL, " ")) {
			bsList_add(&cfg->AreafixNames, strdup(pos2));
		}
	}
	else if(!strcasecmp(pos, "Route") || !strcasecmp(pos, "Send")) {
		tmproute = NEW(Route);
		if (!strcasecmp(pos1, "Hold")) {
			tmproute->Flavour = HOLD;
		}
		else if (!strcasecmp(pos1, "Crash")) {
			tmproute->Flavour = CRASH;
		}
		else {
			tmproute->Flavour = NORMAL;
		}

		if(!strcasecmp(pos, "Send")) {
			tmproute->Type = SEND;
		}
		else {
			tmproute->Type = ROUTE;
			if (tmproute->Flavour == NORMAL) {
				text_to_fidoaddr(pos1, &tmproute->Dest);
			}
			else {
				pos2 = strtok(NULL, " ");
				text_to_fidoaddr(pos2, &tmproute->Dest);
			}
		}

		for(pos3=strtok(NULL, " "); pos3; pos3=strtok(NULL, " ")) {
			tmproute2 = COPY(tmproute);
			strncpy(tmproute2->Pattern, pos3, 128);
			bsList_add(&cfg->Routes, tmproute2);
		}
	}
	else if(!strcasecmp(pos, "ForwardFrom")) {
		bsList_add(&cfg->ForwardFrom, strdup(pos1));
		for(pos2=strtok(NULL, " "); pos2; pos2=strtok(NULL, " ")) {
			bsList_add(&cfg->ForwardFrom, strdup(pos2));
		}
	}
	else if(!strcasecmp(pos, "ForwardTo")) {
		bsList_add(&cfg->ForwardTo, strdup(pos1));
		for(pos2=strtok(NULL, " "); pos2; pos2=strtok(NULL, " ")) {
			bsList_add(&cfg->ForwardTo, strdup(pos2));
		}
	}
	else if(!strcasecmp(pos, "TinySeenby")) {
		bsList_add(&cfg->TinySeenbys, strdup(pos1));

		for(pos2=strtok(NULL, " "); pos2; pos2=strtok(NULL, " ")) {
			bsList_add(&cfg->TinySeenbys, strdup(pos2));
		}
	}
	else if(!strcasecmp(pos, "Packer")) {
		pos2 = strtok(NULL, "");
		tmppack = NEW(Packer);
		strncpy(tmppack->Name, pos1, 16);
		strncpy(tmppack->Compress, pos2, sizeof(tmppack->Compress) - 1);
		tmppack->Compress[sizeof(tmppack->Compress) - 1] = '\0';

		bsList_add(&cfg->Packers, tmppack);
	}
	else if(!strcasecmp(pos, "Pack")) {

		tmppackrule = NEW(PackRule);
		tmppackrule->PackWith = NULL;

		if(strcasecmp(pos1, "None")) {

			for(item=cfg->Packers.first; item; item=item->next) {
				tmppack = item->val;
	
				if (!strcasecmp(tmppack->Name, pos1)) {
					tmppackrule->PackWith = tmppack;
					break;
				}
			}

			if (!tmppackrule->PackWith) {
				printf("Can't find packer! (%s)\n", pos2);
				return -1;
			}

		}

		for(pos2=strtok(NULL, " "); pos2; pos2=strtok(NULL, " ")) {
			tmppackrule2 = COPY(tmppackrule);
			strncpy(tmppackrule2->Pattern, pos2, 64);
			bsList_add(&cfg->PackRules, tmppackrule2);
		}
	}
	else if(!strcasecmp(pos, "Area")) {
		char area_name[65] = {0};
		char *conf_base_token = NULL;
		
		/* Build area name by collecting tokens until we find conference:base pattern */
		if (pos1) {
			strncpy(area_name, pos1, sizeof(area_name) - 1);
		}
		
		/* Get next token and check if it's conference:base or part of area name */
		pos2 = strtok(NULL, " ");
		while (pos2) {
			/* Check if this token looks like conference:base (digit:digit) */
			int is_conf_base = 0;
			if (isdigit(pos2[0])) {
				char *colon_pos = strchr(pos2, ':');
				if (colon_pos && isdigit(*(colon_pos + 1))) {
					is_conf_base = 1;
				}
			}
			
			if (is_conf_base) {
				conf_base_token = pos2;
				break;
			} else {
				/* This token is part of the area name */
				if (strlen(area_name) > 0) {
					strncat(area_name, " ", sizeof(area_name) - strlen(area_name) - 1);
				}
				strncat(area_name, pos2, sizeof(area_name) - strlen(area_name) - 1);
				pos2 = strtok(NULL, " ");
			}
		}
		
		if (!conf_base_token) {
			printf("Error: Missing conference:base in Area configuration\n");
			return -1;
		}
			
		tmparea = NEW(Area);
		memset(tmparea, '\0', sizeof(Area));

		strncpy(tmparea->Area, area_name, 64);
		sscanf(conf_base_token, "%d:%d", &tmparea->Conference, &tmparea->Base);

		tmparea->Type = ECHOMAIL;

		for(pos3 = strtok(NULL, " ");pos3; pos3 = strtok(NULL, " ")) {
			if(!strcasecmp(pos3, "-a")) {
				pos3 = strtok(NULL, " ");
				text_to_fidoaddr(pos3, &tmparea->ourAka);
			}
			else if(!strcasecmp(pos3, "-g")) {
				pos3 = strtok(NULL, " ");
				tmparea->Group = pos3[0];
			}
			else {
				tmpexport = NEW(Export);
				memset(tmpexport, '\0', sizeof(Export));
				
				tmpexport->Send = TRUE;
				tmpexport->Recv=  TRUE;
				tmpexport->NoRemove = FALSE;

				i = 0;

				if(pos[i] == '!') {
					tmpexport->NoRemove = TRUE;
					i += 1;
				}

				if(pos3[i] == '%') {
					tmpexport->Send = FALSE;
					i += 1;
				} 
				else if(pos3[i] == '&') {
					tmpexport->Recv = FALSE;
					i += 1;
				}
				text_to_fidoaddr(pos3+i, &tmpexport->Aka);
				bsList_add(&tmparea->SendTo, tmpexport);
			}
		}
		bsList_add(&cfg->Areas, tmparea);
	}
	else if(!strcasecmp(pos, "BadArea") || 
					!strcasecmp(pos, "NetmailArea") || 
					!strcasecmp(pos, "DupeArea")) {

		tmparea = NEW(Area);

		pos2 = strtok(NULL, " ");
	
		strncpy(tmparea->Area, pos1, 64);
		sscanf(pos2, "%d:%d", &tmparea->Conference, &tmparea->Base);

		if (!strcasecmp(pos, "BadArea")) {
			tmparea->Type = BAD;
			pos3 = strtok(NULL, " ");
		}
		else if(!strcasecmp(pos, "DupeArea")) {
			tmparea->Type = DUPE;
			pos3 = strtok(NULL, " ");
		}
		else {
			tmparea->Type = NETMAIL;
			for(pos3 = strtok(NULL, " ");pos3; pos3 = strtok(NULL, " ")) {
				if(!strcasecmp(pos3, "-a")) {
					pos3 = strtok(NULL, " ");
					text_to_fidoaddr(pos3, &tmparea->ourAka);
				}
			}
		}

		bsList_add(&cfg->Areas, tmparea);

	}
	else {
		printf("Unknown cfg entry! (%s)\n", pos);
		return -1;
	}
	
	return 0;
}


MainCfg* ReadCfg(MainCfg* cfg, char* file) {
	FILE* fh;
	char buf[256];

	memset(cfg, '\0', sizeof(cfg));
	
	fh = fopen(file, "r");
	if(fh == NULL) {
		return NULL;
	}

	while(fgets(buf, 256, fh)) {
		StripCrLf(buf);
		if(ParseCfgLine(cfg, buf) < 0)
			return NULL;
	}

	return cfg;
}

#if 0
void DumpCfg(MainCfg* cfg) {
	bsListItem* item, *item2;
	char buf[64];

	for(item=cfg->Akas.first; item; item=item->next) {
		fidoaddr_to_text(item->val, buf);
		printf("Aka: %s\n", buf);
	}
	printf("Inbound: %s\n", cfg->Inbound);
	printf("SecInbound: %s\n", cfg->SecInbound);
	printf("LocInbound: %s\n", cfg->LocInbound);
	printf("Outbound: %s\n", cfg->Outbound);
	for(item=cfg->Passwords.first; item; item=item->next) {
		fidoaddr_to_text(&((Password*)item->val)->Aka, buf);
		printf("Password Aka %s Pw %s\n", buf, ((Password*)item->val)->Password);
	}
	for(item=cfg->Routes.first; item; item=item->next) {
		fidoaddr_to_text(&((Route*)item->val)->Dest, buf);
		printf("Route Dest %s Pattern %s Flav %d\n", buf, ((Route*)item->val)->Pattern, ((Route*)item->val)->Flavour);
	}
	for(item=cfg->Packers.first; item; item=item->next) {
		printf("Packer name %s\n", ((Packer*)item->val)->Name);
		printf("Packer compress %s\n", ((Packer*)item->val)->Compress);
	}
	for(item=cfg->PackRules.first; item; item=item->next) {
		printf("PackFor %s with %s\n", ((PackRule*)item->val)->Pattern, ((PackRule*)item->val)->PackWith->Name);
	}

	for(item=cfg->Areas.first; item; item=item->next) {
		fidoaddr_to_text(&((Export*)item->val)->ourAka, buf);
		printf("Export %s Path %s Ouraka %s Type %d\n", ((Export*) item->val)->Area,((Export*) item->val)->Path, buf, ((Export*)item->val)->Type);

		for(item2 = ((Export*)item->val)->SendTo.first; item2; item2=item2->next) {
			fidoaddr_to_text(((FidoAddr*)item2->val), buf);
			printf("Export to: %s\n", buf);
		}
	}
}
#endif
