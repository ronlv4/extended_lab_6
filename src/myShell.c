#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "limits.h"
#include "string.h"
#include "LineParser.h"

#define STDIN 0
#define STDOUT 1
#define MAX_PATH 4092
#define MAX_LENGTH 2048


void simulate_chdir(char *cwd, char *path)
{
    strcat(cwd, "/");
    strcat(cwd, path);
    if (chdir(cwd) == -1)
	perror(NULL);
}


int execute(cmdLine *pCmdLine)
{
    int status;
    pid_t ch_pid = fork();
    if (ch_pid == -1)
    {
	perror(NULL);
	_exit(1);
    }
    
    if (ch_pid > 0)
    {
	if (pCmdLine->blocking)
		wait(&status);
        return status;

    } else
    {
        execvp(pCmdLine->arguments[0], pCmdLine->arguments);
        perror(NULL);
	_exit(1);
    }
    return 0;
}

int main(int argc, char **argv)
{
    char cwd[MAX_PATH];
    cmdLine *current_cmd;
    char user_input[MAX_LENGTH];
    while (1)
    {
        getcwd(cwd, MAX_PATH);
        write(STDOUT, cwd, strlen(cwd));
        write(STDOUT, "$ ", 2);
        fgets(user_input, MAX_INPUT, stdin);
        user_input[strlen(user_input)-1] = '\0';
	if (strcmp(user_input, "quit") == 0)
	    _exit(0);
        current_cmd = parseCmdLines(user_input);
	if (strcmp(current_cmd->arguments[0], "cd") == 0)
	{
	    simulate_chdir(cwd, current_cmd->arguments[1]);
	    continue;
	}
        execute(current_cmd);
    }
    return 0;
}
