/* 
 * DD-Echo
 * Linked-list routines
 * Author: Bo Simonsen <bo@geekworld.dk>
 */

#include "headers.h"

void bsList_add(bsList* list, void* item) {
	bsListItem* tmp;

	tmp = NEW(bsListItem);
	tmp->val = item;

	if (list->first == NULL) {
		list->first = tmp;
		list->last = tmp;
		tmp->next = NULL;
		tmp->prev = NULL;
	}
	else {
		tmp->next = NULL;
		tmp->prev = list->last;
		list->last->next = tmp;
		list->last = tmp;
	}
}

void bsList_init(bsList* list) {
	memset(list, 0, sizeof(bsList));
}

void bsList_free(bsList* list) {
	bsListItem *var, *tvar;
	
	var=list->first;

	while(var) {
		tvar = var->next;
		FREE(var->val);
		FREE(var);
		var = tvar;
	}

	list->first = NULL;
	list->last = NULL;
}

void bsList_freeFromOffset(bsList* list, bsListItem* item) {
	bsListItem *var, *tvar;
	
	var=item->next;

	while(var) {
		tvar = var->next;
		FREE(var->val);
		FREE(var);
		var = tvar;
	}

	item->next = NULL;
	list->last = item;
}

void bsList_delete(bsList* list, bsListItem* item) {
	if(!item || !list) return;
	
	if(item->prev && item->next) {
		item->prev->next = item->next;
		item->next->prev = item->prev;
	}
	else {
		if(!item->prev)
		{
			list->first = item->next;
			if(list->first) list->first->prev = NULL;
		}
		if(!item->next) {
			list->last = item->prev;
			if(list->last) list->last->next = NULL;
		}
	}
}

void bsList_insertAfter(bsList* list, bsListItem* item, void* val) {
	bsListItem* newitem;

	newitem = NEW(bsListItem);

	newitem->val = val;

	if(item->next) {

		newitem->next = item->next;
		newitem->prev = item;

		item->next->prev = newitem;
		item->next = newitem;
	}
	else {
		bsList_add(list, val);
	}
}

void bsList_insertBefore(bsList* list, bsListItem* item, void* val) {
	bsListItem* newitem;

	newitem = NEW(bsListItem);

	newitem->val = val;

	if(item->prev) {
		newitem->next = item;
		newitem->prev = item->prev;

		item->prev->next = newitem;
		item->prev = newitem;
	} else {
		newitem->next = item;
		newitem->prev = NULL;
		item->prev = newitem;
		list->first = newitem;
	}
}

#if 0
int main(int argc, char* argv[]) {
	bsList a;
	bsListItem* item;
	memset(&a, '\0', sizeof(a));
	bsList_add(&a, strdup("a"));
	bsList_add(&a, strdup("c"));
	bsList_insertBefore(&a, a.last, strdup("b"));
	for(item=a.first; item; item=item->next) {
		printf("%s\n", item->val);
	}
	for(item=a.last; item; item=item->prev) {
		printf("%s\n", item->val);
	}

}
#endif
