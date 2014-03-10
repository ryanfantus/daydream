#include <stdlib.h>
#include <stdio.h>

#include "ddlib.h"

static struct dif *d;
static int node_number;

static void show_log();
static void die();

int main(int argc, char *argv[])
{
	if (argc != 2) {
		fprintf(stderr, "Run me from DayDream.\n");
		return 1;
	}
	
	if (!getenv("DAYDREAM")) {
		fprintf(stderr, "DAYDREAM environment variable not set.\n");
		return 1;
	}
	
	d = dd_initdoor(argv[1]);
	if (!d) {
		fprintf(stderr, "Can't open socket.\n");
		return 1;
	}
	
	atexit(die);
	
	node_number = atoi(argv[1]);
	show_log();
	return 0;
}

static void die()
{
	dd_close(d);
}

static void usage()
{
	dd_sendstring(d, "\e[0mUsage: log <node number>\n");
	exit(1);
}

static int log_number()
{
	int num;
	char buffer[1024], *p;
	dd_getstrval(d, buffer, DOOR_PARAMS);
	num = strtol(buffer, &p, 10);
	if (*p) 
		usage();
	if (!*buffer)
		return node_number;
	return num;
}

static void show_log()
{
	char buffer[1024];       
	snprintf(buffer, sizeof(buffer), "%s/logfiles/daydream%d.log", 
		getenv("DAYDREAM"), log_number());
	dd_typefile(d, buffer, TYPE_WARN | TYPE_NOCODES);
}
