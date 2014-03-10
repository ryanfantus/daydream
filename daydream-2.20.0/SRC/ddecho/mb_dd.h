#include <ddmsglib.h>

typedef struct DD_Area_ {
	char area[64];
	char path[128];
	int basenum;
	FILE* fh;
	int fd;
	int low;
	int high;
	int r_pos;
} DD_Area;


