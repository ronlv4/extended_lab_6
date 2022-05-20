#include <stdio.h>
#include <stdlib.h>
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

#define TERMINATED -1
#define RUNNING 1
#define SUSPENDED 0

typedef struct process {
    cmdLine *cmd; /* the parsed command line*/
    pid_t pid; /* the process id that is
		  running the command*/
    int status; /* status of the process:
		   RUNNING/SUSPENDED/TERMINATED */
    struct process *next; /* next process in chain */
} process;


void addProcess(process **process_list, cmdLine *cmd, pid_t pid) {
    process *ps = (process *) malloc(sizeof(process));
    ps->cmd = cmd;
    ps->pid = pid;
    ps->status = RUNNING;
    if (process_list == NULL)
    {
        process_list = malloc(sizeof(process*));
        process_list[0] = ps;
        return;
    }
    ps->next = process_list[0];
}

void printProcessList(process **process_list) {
    if (process_list == NULL) {
        printf("process list is empty\n");
        return;
    }

    printf("process_id\tcommand\tprocess_status\n");
    process *current_process = process_list[0];

    do {
        printf("%d\t%s\t%d", current_process->pid, current_process->cmd->arguments[0], current_process->status);
        current_process = current_process->next;
    } while (current_process != NULL);
}

void freeProcessList(process **process_list) {
    process *current;
    process *next;
    if (process_list == NULL)
        return;
    current = next = process_list[0];
    while (current != NULL) {
        next = next->next;
        free(current);
        current = next;
    }
}

void updateProcessList(process **process_list){

}

void simulate_chdir(char *cwd, char *path) {
    strcat(cwd, "/");
    strcat(cwd, path);
    if (chdir(cwd) == -1)
        perror(NULL);
}


int execute(cmdLine *pCmdLine) {
    int status;
    pid_t ch_pid = fork();
    if (ch_pid == -1) {
        perror(NULL);
        _exit(1);
    }

    if (ch_pid > 0) {
        if (pCmdLine->blocking)
            wait(&status);
        return status;

    } else {
        execvp(pCmdLine->arguments[0], pCmdLine->arguments);
        perror(NULL);
        _exit(1);
    }
    return 0;
}

int main(int argc, char **argv) {
    char cwd[MAX_PATH];
    process **process_list;
    cmdLine *current_cmd;
    char user_input[MAX_LENGTH];
    while (1) {
        getcwd(cwd, MAX_PATH);
        write(STDOUT, cwd, strlen(cwd));
        write(STDOUT, "$ ", 2);
        fgets(user_input, MAX_INPUT, stdin);
        user_input[strlen(user_input) - 1] = '\0';
        if (strcmp(user_input, "quit") == 0)
            _exit(0);
        current_cmd = parseCmdLines(user_input);
        if (strcmp(current_cmd->arguments[0], "cd") == 0) {
            simulate_chdir(cwd, current_cmd->arguments[1]);
            continue;
        }
        if (strcmp(current_cmd->arguments[0], "showprocs") == 0) {
            printProcessList(process_list);
            continue;
        }
        addProcess(process_list, current_cmd, 5);
        printProcessList(process_list);
        execute(current_cmd);
    }
    return 0;
}
