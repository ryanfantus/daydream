/* 
 * DD-Echo 
 * Routines for handling tosser stats
 * Author: Bo Simonsen <bo@geekworld.dk>
 */

#include "headers.h"

typedef struct StatEnt_ {
	char Area[64];
	int Count;
} StatEnt;

void AddToStat(char* area) {
	bsListItem* item;
	StatEnt* s;
	int found = 0;

	for(item=stat_list.first; item; item=item->next) {
		s = item->val;

		if(!strcasecmp(s->Area, area)) {
			s->Count += 1;
			found = 1;
			break;
		}
	}

	if(!found) {
		s = NEW(StatEnt);
		memset(s, '\0', sizeof(StatEnt));

		strcpy(s->Area, area);
		s->Count = 1;

		bsList_add(&stat_list, s);
	}
}

void PrintStats() {
	bsListItem* item;
	StatEnt* s;

	if(stat_list.first != NULL) {
		logit(TRUE, "Area statistics:\n");

		for(item=stat_list.first; item; item=item->next) {
			s = item->val;
			logit(TRUE, "  %-20.20s %d\n", s->Area, s->Count);
		}

	}
}


