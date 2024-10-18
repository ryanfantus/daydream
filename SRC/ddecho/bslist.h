typedef struct bsListItem_ {
	void* val;
	struct bsListItem_* next;
	struct bsListItem_* prev;
} bsListItem;

typedef struct bsList_ {
	bsListItem* first;
	bsListItem* last;
} bsList;


