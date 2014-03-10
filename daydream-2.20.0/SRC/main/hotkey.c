#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <syslog.h>

#include <daydream.h>
#include <ddcommon.h>
#include <symtab.h>
#include <console.h>

struct input_t {
	int src;	/* -1 - carrier loss, 0 - timeout,
			 * 1 - user, 2 - sysop, 3 - signal arrived */
	int code;
};

static volatile int kill_timeout;

static void sigalrm_handler(int sig)
{
	kill_timeout = 1;
}

/* returns -1 if the child did not exit */
static int wait_for_process(pid_t pid)
{
	void (*sg)(int);

	kill_timeout = 0;
	sg = signal(SIGALRM, sigalrm_handler);
	alarm(5); /* wait for 5 seconds */
	while (!waitpid(pid, NULL, WNOHANG) && !kill_timeout)
		usleep(5000);
	alarm(0);
	return -kill_timeout;
}

void kill_child(pid_t pid)
{
	kill(pid, SIGTERM);
	if (!wait_for_process(pid))
		return;
	kill(pid, SIGKILL);
	wait_for_process(pid);
}

static void kill_zombies(void)
{
	while (waitpid(-1, NULL, WNOHANG) > 0);
}

/* the latest child created is terminated first, etc. This ensures that
 * it is possible to write doors that write all working information to
 * disk in case of SIGTERM.
 */
static void kill_children(void)
{       
	while (child_pid_list) 
		kill_child(shift(pid_t, child_pid_list));
	kill_zombies();
}

static int timeout(void)
{
	timeleft = endtime - time(NULL);
	if (timeleft < 0 && onlinestat) {
		DDPut(sd[dltstr]);
		return 1;
	}
	return 0;
}

static struct input_t read_input(int params, int idlemode)
{
	fd_set rset;
	struct timeval tv;
	int maxfd;
	struct input_t retval;

	if (!input_queue_empty()) {
		retval.src = 2;
		retval.code = input_queue_get();
		return retval;
	}

	FD_ZERO(&rset);	/* descriptor set for read */
	maxfd = 0;
	
	/* HOT_RE is used in conjunction with handlectrl() in order to
	 * get the second char of ^C-x combination quickly.
	 */
	if (!lmode && userinput && !(params & HOT_RE)) {
		maxfd = serhandle;
		FD_SET(serhandle, &rset);
	}

	maxfd = console_select_input(maxfd, &rset);
	
	if (dsockfd) {
		if (maxfd < dsockfd)
			maxfd = dsockfd;
		FD_SET(dsockfd, &rset);
	}
	
	if (params & HOT_DELAY) {
		tv.tv_sec = delayt;
		tv.tv_usec = 0;
	} else if (params & HOT_QUICK && !(params & HOT_RE)) {
		tv.tv_sec = 0;
		tv.tv_usec = 0;
	} else {
		if (!idlemode) 
			tv.tv_sec = maincfg.CFG_IDLETIMEOUT - 30;
		else
			tv.tv_sec = 30;
		tv.tv_usec = 0;
	}
	
	if (select(maxfd + 1, &rset, NULL, NULL, &tv) == -1) {
		if (errno != EINTR)
			abort();

		retval.src = 3;
		return retval;
	}
	
	if (timeout() || !checkcarrier()) {
		retval.src = -1;
		return retval;
	}
	
	if (!lmode && FD_ISSET(serhandle, &rset)) {
		unsigned char ch;
		if (read(serhandle, &ch, 1) != 1) {
			/* FIXME: not necessarily an error... */
			syslog(LOG_ERR, "read_input(): %m"); 
			exit(1);
		}
		if(ch == '\r' && is_telnet_connection()) {
			unsigned char ch2;
			
			/* A hack :( 
			 
			   Some clinets may send \r\n, and some may send \r\0,
			   however the \0 character gets lost somewhere, or I 
			   suspect that the client never sends it.
			*/
			memset(&tv, '\0', sizeof(struct timeval));
			FD_SET(serhandle, &rset);
			if(select(serhandle+1, &rset, NULL, NULL, &tv) < 0) {
				retval.src = -1;
				return retval;
			}
			if(FD_ISSET(serhandle, &rset)) {
				if(read(serhandle, &ch2, 1) < 0) { 
					// CR or \00
					retval.src = -1;
					return retval;
				}
			}
		}

		retval.src = 1;
		retval.code = ch;
		return retval;
	}
	
	if (FD_ISSET(dsockfd, &rset)) {
		struct dd_nodemessage ddn;
		if (read(dsockfd, &ddn, sizeof(struct dd_nodemessage)) !=
		    sizeof(struct dd_nodemessage)) {
			/* FIXME: not necessarily an error... */
			syslog(LOG_ERR, "read_input(): %m");
			exit(1);
		}
		
		processmsg(&ddn);
		if (params & HOT_MAIN) {
			retval.src = 0;
			return retval;
		} else 
			return read_input(params, idlemode);
	}
	
	if (console_pending_input(&rset)) {
		retval.src = 2;
		retval.code = console_getc();
		return retval;
	}
	
	retval.src = 0;
	return retval;
}

unsigned char HotKey(int params)
{
	struct input_t rv;
	int idlemode = 0;
	
	for (;;) {
		rv = read_input(params, idlemode);
						  
		keysrc = rv.src;
	
		if (rv.src != 0)
			break;
			
		if ((params & HOT_DELAY) || (params & HOT_QUICK))
			return 0;
			
		if (!idleon)
			continue;

		if (!idlemode) {
			if (sd[timeoutstr])
				DDPut(sd[timeoutstr]);
			else
				DDPut("timeout");
			idlemode = 1;
			continue;
		} else {
			char bbuf[1024];
			snprintf(bbuf, 1024, "Connection closed by idle timeout at %s\n", currt());
			writelog(bbuf);
			
			kill_children();
			dropcarrier();
			return 0;
		}

	}
	
	/* signal caught? */
	if (rv.src == 3) 
		return 0;
	
	/* daily time limit? */
	if (rv.src == -1) {
		kill_children();
		dropcarrier();
		return 0;
	}
	
	/* control-c pressed by sysop? */
	if (rv.src == 2 && rv.code == 2)
		return handlectrl(params);
	
				      	
	if (params & HOT_YESNO && !(params & HOT_RE)) {
		if (rv.code == 'Y' || rv.code == 'y' || 
		    rv.code == 13 || rv.code == 10) {
			DDPut(sd[yesstr]);
			return 1;
		} else if (rv.code == 'N' || rv.code == 'n') {
			DDPut(sd[nostr]);
			return 2;
		} else
			return HotKey(params);
	}
	if (params & HOT_NOYES && !(params & HOT_RE)) {
		if (rv.code == 'N' || rv.code == 'n' || 
		    rv.code == 13 || rv.code == 10) {
			DDPut(sd[nostr]);
			return 2;
		} else if (rv.code == 'Y' || rv.code == 'y') {
			DDPut(sd[yesstr]);
			return 1;
		} else
			return HotKey(params);
	}
	if (rv.code == 27 && (params & HOT_CURSOR)) {
		rv.code = HotKey(0);
		if (rv.code == '[') {
			rv.code = HotKey(0);
			if (rv.code == 'A')
				return 250;
			if (rv.code == 'B')
				return 251;
			if (rv.code == 'C')
				return 252;
			if (rv.code == 'D')
				return 253;
			return rv.code;
		}
	}
	if (rv.code == 155 && (params & HOT_CURSOR)) {
		rv.code = HotKey(0);
		if (rv.code == 'A')
			return 250;
		if (rv.code == 'B')
			return 251;
		if (rv.code == 'C')
			return 252;
		if (rv.code == 'D')
			return 253;
		return rv.code;
	}

	return rv.code;
}

