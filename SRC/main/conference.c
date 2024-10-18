#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <daydream.h>
#include <utility.h>

static conference_t *current_conference;
static conference_t *conferences;

conference_t *conference(void)
{
	return current_conference;
}

void set_conference(conference_t *conf)
{
	current_conference = conf;
}

conference_t *findconf(int conf_num)
{
	conference_t *mc;
	struct iterator *iterator = conference_iterator();
	while ((mc = (conference_t *) iterator_next(iterator))) {
		if (mc->conf.CONF_NUMBER == conf_num) 
			break;
		mc = mc->next;
	}
	iterator_discard(iterator);
	return mc;
}

void read_conference_data(void)
{
	int fd, bread;
	struct DayDream_Conference cf;
	conference_t *tail;
	
	if ((fd = open("data/conferences.dat", O_RDONLY)) < 0) {
		printf("Can't open \"data/conferences.dat\", exiting.");
		exit(1);
	}
	
	conferences = (conference_t *) xmalloc(sizeof(conference_t));
	conferences->next = NULL;
	conferences->prev = NULL;

	tail = conferences;
	while ((bread = read(fd, &cf, sizeof(struct DayDream_Conference)))) {
		conference_t *cp;
		int i;

		if (bread != sizeof(struct DayDream_Conference)) {
			printf("bad \"data/conferences.dat\".");
			exit(1);
		}
		
		cp = (conference_t *) xmalloc(sizeof(conference_t));
		tail->next = cp;
		cp->prev = tail;
		cp->next = NULL;
		tail = cp;		

		memcpy(&cp->conf, &cf, sizeof(struct DayDream_Conference));

		cp->msgbases = (msgbase_t **) 
			xmalloc(cp->conf.CONF_MSGBASES * sizeof(msgbase_t *));
		for (i = 0; i < cp->conf.CONF_MSGBASES; i++) {
			cp->msgbases[i] = (msgbase_t *) 
				xmalloc(sizeof(msgbase_t));
			if (read(fd, cp->msgbases[i], sizeof(msgbase_t)) !=
			    sizeof(msgbase_t)) {
				printf("bad \"data/conferences.dat\".");
				exit(1);
			}
		}		
	}
	
	close(fd);
	current_conference = conferences->next;
	if (current_conference->conf.CONF_MSGBASES)
		current_msgbase = current_conference->msgbases[0];
}

static void conference_iterator_discard(struct iterator *);
static void *conference_iterator_next(struct iterator *);

static struct iterator_impl conference_iterator_impl = {
	conference_iterator_discard,
	conference_iterator_next
};

struct iterator *conference_iterator(void)
{
	struct iterator *iterator = g_new(struct iterator, 1);
	iterator->impl = &conference_iterator_impl;
	iterator->data = conferences->next;
	return iterator;
}

static void conference_iterator_discard(struct iterator *iterator)
{
	g_free(iterator);
}

static void *conference_iterator_next(struct iterator *iterator)
{
	conference_t *conf = (conference_t *) iterator->data;
	if (conf)
		iterator->data = conf->next;
	return conf;
}
	
	
