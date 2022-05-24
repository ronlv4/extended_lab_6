#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include "string.h"

#define STDIN 0
#define STDOUT 1

void handler(int signum)
{
	char *signame = strsignal(signum);
	write(STDOUT, "\n",1);
	write(STDOUT, "caught signal ",14);
	write(STDOUT, signame, strlen(signame));
	signal(signum, SIG_DFL);
	raise(signum);
}

int main(int argc, char **argv){ 
	signal(SIGINT, handler);
	signal(SIGTSTP, handler);
	signal(SIGCONT, handler);

	while(1) {
		sleep(2);
	}

	return 0;
}
