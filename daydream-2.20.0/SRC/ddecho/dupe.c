/* 
 * DD-Echo
 * Dupe registration and checking routines
 * Author: Bo Simonsen <bo@geekworld.dk>
 */

#include "headers.h"

bool CheckDupe(MEM_MSG* mem) {
	char msgid[64];
	bsListItem* item;
	FILE* fh;
	DupeEntry dupe;
	bool retval;

	msgid[0] = 0;
	retval = FALSE;

	for(item=mem->Text.first; item; item=item->next) {
		if(!strncasecmp(item->val, "\1MSGID:", 7)) {
			strncpy(msgid, item->val+8, 64);
			break;
		}
	}

	if(msgid[0] != 0) {
		fh = fopen(main_cfg.DupeLog, "r"); 

		if(fh == NULL) {
			return FALSE;
		}	
		while(fread(&dupe, sizeof(DupeEntry), 1, fh) == 1) {
			if(!strcasecmp(msgid, dupe.MSGID) && !strcasecmp(mem->Area, dupe.Area)) {
				retval = TRUE;
				break;
			}
		}
		fclose(fh);
	}

	return retval;
}

int AddDupe(MEM_MSG* mem) {

	char msgid[64];
	DupeEntry dupe;
	FILE* fh;
	bsListItem* item;

	msgid[0] = 0;

	for(item=mem->Text.first; item; item=item->next) {
		if(!strncasecmp(item->val, "\1MSGID:", 7)) {
			strncpy(msgid, item->val+8, 64);
			break;
		}
	}

	if(msgid[0] != 0) {
		memset(&dupe, '\0', sizeof(DupeEntry));

		strncpy(dupe.Area, mem->Area, 64);
		strncpy(dupe.MSGID, msgid, 64);

		fh = fopen(main_cfg.DupeLog, "a");
		if(fh == NULL) {
			return -1;
		}

		if(fwrite(&dupe, sizeof(DupeEntry), 1, fh) != 1) {
			return -1;
		}
		
		fclose(fh);
	}

	return 0;

}
