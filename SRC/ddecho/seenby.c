/*
 * DD-Echo
 * Seenby and \1Path routes
 * Author: Bo Simonsen <bo@geekworld.dk>
 */

#include "headers.h"

void Node2dList(bsList* list, char* text) {
	char* ptr, *ptr2;
	char buf[256];
	int node = 0;
	int net = 0;
	int last = 0;
	FidoAddr* tmp;

	strcpy(buf, text);

	for(ptr=strtok(buf, " "); ptr; ptr=strtok(NULL, " ")) {
		ptr2 = strchr(ptr, '/');

		if(ptr2) {
			*ptr2 = '\0';
			ptr2++;

			net = atoi(ptr);
			node = atoi(ptr2);
			last = net;
		}
		else {
			node = atoi(ptr);
			net = last;
		}

		tmp = NEW(FidoAddr);

		tmp->net = net;
		tmp->node = node;

		bsList_add(list, tmp);
	}

}

void Node2dText(bsList* text, bsList* list, char* prefix) {
	char lbuf[256];
	char tbuf[64];
	bsListItem* item;
	int last = 0;
	FidoAddr* tmp;

	strcpy(lbuf, prefix);

	for(item=list->first; item; item=item->next) {

		tmp = item->val;

		if(last == tmp->net) {
			sprintf(tbuf, "%d", tmp->node);
		} else {
			sprintf(tbuf, "%d/%d", tmp->net, tmp->node);
		}

		if(strlen(lbuf) + 1 + strlen(tbuf) > 79) {
			bsList_add(text, strdup(lbuf));
			
			/* We will force it to "full address" mode
			 * for 2d. */
			sprintf(tbuf, "%d/%d", tmp->net, tmp->node);
			
			strcpy(lbuf, prefix);
			strcat(lbuf, " ");
			strcat(lbuf, tbuf);
		}
		else {
			strcat(lbuf, " ");
			strcat(lbuf, tbuf);
		}
		
		last = tmp->net;
	}

	bsList_add(text, strdup(lbuf));

}

int Node2dExists(bsList* list, FidoAddr* addr) {
	FidoAddr* tmp;
	bsListItem* item;

	for(item=list->first; item != NULL; item=item->next) {
		tmp = item->val;

		if (tmp->net == addr->net && 
		    tmp->node == addr->node) {
			return 1;
		}

	}
	return 0;
}

void GetSeenbyList(MEM_MSG* mem, bsList* seenbys) {
	bsListItem* item;
	char* text;

	for(item=mem->Text.last; item; item=item->prev) {
		text = item->val;
		
		if(!strcmp(text, "")) {
			continue;
		}
		else if(!strncmp(text, "\1PATH:", 6)) {	
			continue;
		}
		else if(!strncmp(text, "SEEN-BY:", 8)) {
			Node2dList(seenbys, text+9);
                }
		else {
			break;
		}
	}
}

void SortSeenbyList(bsList* seenbys) {
	bsListItem* item;
	bsListItem* item2;
	FidoAddr* aka;
	FidoAddr* aka2;
	
	/*******************************************************
	 * We use bubblesort here O(n^2), we could do O(n lg n),
	 * however it may be more expensive when doing copying
	 * to array and stuff. The seenby lists should not be
	 * so long so this should be a performance issue
	 * (we also save memory allocations etc, don't under-
	 * estimate the cost of these!)
	 *******************************************************/

	for(item = seenbys->first; item; item=item->next) {
		aka = item->val;
		for(item2 = item->next; item2; item2=item2->next) {
			aka2 = item2->val;
			
			if(aka->net > aka2->net || 
			   (aka->net == aka2->net && aka->node > aka2->node)) {
				item->val = aka2;
				item2->val = aka;
				aka = item->val;
				aka2 = item2->val;
			}
		}
	}
}

void AddSeenbyPath(MEM_MSG* mem, bsList* links) {
	bsList Seenby_List;
	bsList Path_List;
	char* text;
	bsListItem* item;
	FidoAddr* tmp;
	bool TinySeenby = FALSE;
	char buf[64];

	memset(&Seenby_List, '\0', sizeof(bsList));
	memset(&Path_List, '\0', sizeof(bsList));

	for(item=main_cfg.TinySeenbys.first; item; item=item->next) {
		fidoaddr_to_text(&mem->MsgDest, buf);

		if(patmat(buf, item->val)) {
			TinySeenby = TRUE;
			break;
		}
	}

	for(item=mem->Text.last; item; item=item->prev) {
		text = item->val;
		
		if(!strcmp(text, "")) {
			continue;
		}
		if(!strncmp(text, "SEEN-BY:", 8) && 
                   !TinySeenby && 
                   mem->MsgDest.zone == mem->MsgOrig.zone) {
			Node2dList(&Seenby_List, text+9);
		}
		else if(!strncmp(text, "\1PATH:", 6)) {	
			Node2dList(&Path_List, text+7);
		}
		else {
			break;
		}
	}

	if(item) {
		bsList_freeFromOffset(&mem->Text, item);
	}

	
	for(item=links->first; item; item=item->next) {
		tmp = item->val;

		if(!Node2dExists(&Seenby_List, tmp) && tmp->zone == mem->MsgDest.zone && tmp->point == 0) {
			bsList_add(&Seenby_List, COPY((FidoAddr*) item->val));
		}
	}

	for(item=main_cfg.Akas.first; item; item=item->next) {
		tmp = item->val;

		if(tmp->zone != mem->MsgDest.zone || tmp->point != 0) {
			continue;
		}

		if(!Node2dExists(&Seenby_List, tmp)) {
			bsList_add(&Seenby_List, COPY(tmp));
		}
	}
	
	if(!Node2dExists(&Path_List, &mem->MsgOrig)) {
		bsList_add(&Path_List, COPY(&mem->MsgOrig));
	}

	SortSeenbyList(&Seenby_List);

	Node2dText(&mem->Text, &Seenby_List, "SEEN-BY:");
	Node2dText(&mem->Text, &Path_List, "\1PATH:");

	bsList_free(&Seenby_List);
	bsList_free(&Path_List);

}


