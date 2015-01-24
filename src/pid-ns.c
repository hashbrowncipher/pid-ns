#define _GNU_SOURCE

#include <errno.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>

int usage(char * name) {
	printf("Usage: %s [-d] <command> <arg1> ... <argn>\n", name);
	return 252;
}


int main(int argc, char * argv[]) {
	int arg = 0;
	int detach = 0;

	while((arg = getopt(argc, argv, "d")) != -1) {
		switch(arg) {
		case 'd':
			detach = 1;
			break;
		default:
			return usage(argv[0]);
			break;
		}
	}


	if (argc < optind + 1) {
		return usage(argv[0]);
	}

	int ret;
	ret = unshare(CLONE_NEWPID);
	if (ret < 0) {
		return 255;
	}

	/*
	 * unshare(2) requires root privileges, so the expectation is for this script
	 * to execute setuid root.  Once we've done unshare(), we drop root privs
	 * like they're hot.
	 */
	ret = getuid();
	ret = setuid(ret);
	if (ret < 0) {
		return 255;
	}

	ret = fork();
	if (ret < 0) {
		return 255;
	} else if (ret == 0) {
		execvp(argv[optind], argv + optind);

		/* If we get here, execvp has failed.  Set the exit code in accordance
		 * with POSIX */
		if(errno == ENOENT) {
			return 127;
		} else {
			return 126;
		}
	}

	if(detach) {
		return 0;
	}

	int status;
	ret = wait(&status);
	if (ret < 0) {
		return 255;
	}

	if (WIFSIGNALED(status)) {
		return 128 + WTERMSIG(status);
	}
	return WEXITSTATUS(status);
}
