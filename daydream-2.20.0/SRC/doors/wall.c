#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <dd.h>
#include <signal.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/un.h>

static int sockfd;

struct wall {
	uint16_t wall_type;
	uint16_t wall_sysop;
	char wall_string[70];
	char wall_from[25];
};

static void sends(const char *);
static void killdoor(void);
static void writedm(struct DayDream_DoorMsg *);

int main(int argc, char *argv[])
{
	char buffer[300];
	struct sockaddr_un socknfo;
	struct DayDream_DoorMsg dmsg;
	struct wall walldata[60];
	char *s;
	int i;
	int prlines;
	int datafd;
	int level;

	struct wall *cd;

	s = (char *) walldata;

	for (i = 0; i < sizeof(struct wall) * 60; i++)
		*s++ = 0;

	if (argc < 2) {
		printf("This program must be launched from DayDream BBS!");
		exit(1);
	}
	signal(SIGHUP, SIG_IGN);

	sprintf(buffer, "%s/dd_door%s", DDTMP, argv[1]);

	strcpy(socknfo.sun_path, buffer);
	socknfo.sun_family = AF_UNIX;

	sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sockfd == -1) {
		printf("Couldn't create socket!\n\n");
		exit(2);
	}
	if (connect(sockfd, (struct sockaddr *) &socknfo, sizeof(socknfo)) == -1) {
		printf("Can't find door socket!\n\n");
		exit(3);
	}
	atexit(killdoor);

	dmsg.ddm_command = 109;
	dmsg.ddm_data1 = 0;
	writedm(&dmsg);
	prlines = dmsg.ddm_data1 - 3;

	dmsg.ddm_command = 122;
	dmsg.ddm_data1 = 0;
	writedm(&dmsg);
	level = dmsg.ddm_data1;


	datafd = open("data/ddwall.dat", O_RDWR | O_CREAT, 0666);
	if (datafd == -1) {
		sends("[35m\nUnable to open datafile!\n\n");
	}
	read(datafd, &walldata, sizeof(struct wall) * 60);

	sends("[2J[H[0;34m---[36m[ [0mDD-Wall/Linux V1.00 [36m][34m-----------------------------[36m[ [0mby Hydra/sELLERi [36m][34m---\n");

	i = 60 - prlines;
	if (i < 0)
		i = 0;
	while (i != 60) {
		const char *col1;
		const char *col2;
		const char *writer;

		cd = &walldata[i];

		if (!cd->wall_sysop)
			col1 = "[32m";
		else
			col1 = "[0m";
		if (cd->wall_type && level == 255) {
			col2 = "[33m";
			writer = cd->wall_from;
		} else if (cd->wall_type) {
			col2 = "[31m";
			writer = "Anonymous";
		} else {
			col2 = "[36m";
			writer = cd->wall_from;
		}

		sprintf(buffer, " %s%-61.61s %s%-14.14s \n", col1, cd->wall_string, col2, writer);
		sends(buffer);
		i++;
	}
	sends("[34m------------------------------------------------------------------------------\n");
	sends(" [35mWanna add a line? ([36myes/No[35m) : [0m");

	dmsg.ddm_command = 4;
	dmsg.ddm_data1 = HOT_NOYES;
	writedm(&dmsg);
	if (dmsg.ddm_data1 == 1) {
		int ano = 0;
		int imp = 0;
		char hinkbuf[70];
		sends("[A                                         \r[34m[[61C[34m][0m[62D");
		dmsg.ddm_command = 3;
		dmsg.ddm_data1 = 61;
		dmsg.ddm_data2 = 0;
		dmsg.ddm_string[0] = 0;
		writedm(&dmsg);
		if (dmsg.ddm_data1 == 0)
			exit(0);
		strcpy(hinkbuf, dmsg.ddm_string);
		sends("[A                                                                         \r");
		sends(" [35mWanna be anonymous? ([36myes/No[35m) : [0m");
		dmsg.ddm_command = 4;
		dmsg.ddm_data1 = HOT_NOYES;
		writedm(&dmsg);

		if (dmsg.ddm_data1 == 1)
			ano = 1;

		if (level == 255) {
			sends("[A                                                                         \r");
			sends(" [35mIs this important (SysOp only)? ([36myes/No[35m) : [0m");
			dmsg.ddm_command = 4;
			dmsg.ddm_data1 = HOT_NOYES;
			writedm(&dmsg);

			if (dmsg.ddm_data1 == 1)
				imp = 1;
		}
		for (i = 0; i < 59; i++) {
			memcpy(&walldata[i], &walldata[i + 1], sizeof(struct wall));
		}
		cd = &walldata[59];
		strcpy(cd->wall_string, hinkbuf);
		cd->wall_type = ano;
		cd->wall_sysop = imp;
		dmsg.ddm_command = 103;
		dmsg.ddm_data1 = 0;
		writedm(&dmsg);
		strcpy(cd->wall_from, dmsg.ddm_string);

		lseek(datafd, 0, SEEK_SET);
		write(datafd, &walldata, sizeof(struct wall) * 60);
	}
	close(datafd);
	return 0;
}

static void killdoor(void)
{
	struct DayDream_DoorMsg dm;

	dm.ddm_command = 1;
	writedm(&dm);
	close(sockfd);
}

static void sends(const char *s)
{
	struct DayDream_DoorMsg dm;

	dm.ddm_command = 2;
	strcpy(dm.ddm_string, s);
	writedm(&dm);
}

static void writedm(struct DayDream_DoorMsg *dm)
{
	write(sockfd, dm, sizeof(struct DayDream_DoorMsg));
	read(sockfd, dm, sizeof(struct DayDream_DoorMsg));
}
