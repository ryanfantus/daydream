/* 
 * DD-Echo
 * Fido address handling routines
 * Author: Bo Simonsen <bo@geekworld.dk>
 */

#include "headers.h"

FidoAddr* text_to_fidoaddr(char* text, FidoAddr* aka) {
	int ret;
	ret = sscanf(text, "%u:%u/%u.%u", &aka->zone, &aka->net, &aka->node, &aka->point);
	switch(ret) {
	case 4:
		/* perfect */
		break;
	case 3:
		/* no point */
		aka->point = 0;
		break;
	default:
		/* format NN/NN */
		aka->zone = 0;
		sscanf(text, "%u/%u", &aka->net, &aka->node);
		break;
	}
	
	return aka;
}

char* fidoaddr_to_text(FidoAddr* aka, char* text) {
	static char tmp[64];

	if(text == NULL) {
		text = tmp;
	}

	if (aka->zone == 0) {
		sprintf(text, "%u/%u", aka->net, aka->node);
	}
	else if (aka->point == 0) {
		sprintf(text, "%u:%u/%u", aka->zone, aka->net, aka->node);
	}
	else {
		sprintf(text, "%u:%u/%u.%u", aka->zone, aka->net, aka->node, aka->point);
	}

	return text;
}

int fidoaddr_compare(FidoAddr* aka1, FidoAddr* aka2) {
	if(aka1->zone == aka2->zone && 
		 aka1->net == aka2->net &&
		 aka1->node == aka2->node &&
		 aka1->point == aka2->point) {
		return 1;
	}

	return 0;
}

int fidoaddr_empty(FidoAddr* aka) {
	if(aka->net == 0 && aka->node == 0 && aka->zone == 0 && aka->point == 0) {
		return 1;
	}
	
	return 0;
}

