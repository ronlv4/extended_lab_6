#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ioctl.h>

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

void updateProcessList();
void nap();
void stop();
void updateProcessStatus();
process* deleteProcess();
void addProcess();
void printProcessList();
void freeProcessList();
void simulate_chdir();
int execute();

void nap(cmdLine *pCmdLine)
{
    int status;
    int time = atoi(pCmdLine->arguments[1]);
    int ps_pid = atoi(pCmdLine->arguments[2]);
    int pid = fork();
    if (pid == -1)
    {
        perror(NULL);
        _exit(pid);
    }
    if (pid)
    {
        if (pCmdLine->blocking)
            waitpid(pid, &status, WNOHANG | WCONTINUED | WUNTRACED);
    }
    else
    {
        kill(ps_pid, SIGTSTP);
        sleep(time);
        kill(ps_pid, SIGCONT);
    }
}

void stop(int pid)
{
    kill(pid, SIGINT);
}

void updateProcessList(process **process_list)
{
    int status, ans;
    process *cur = *process_list;
    while (cur)
    {
        pid_t ret = waitpid(cur->pid, &ans, WNOHANG | WCONTINUED | WUNTRACED);
        if(ret != 0){
            status = (WIFSIGNALED(ans)|| WIFEXITED(ans)) ? TERMINATED :
                     WIFSTOPPED(ans) ? SUSPENDED :
                     WIFCONTINUED(ans) ? RUNNING:
                     2;
            if (status != 2)
                updateProcessStatus(cur, cur->pid, status);
        }
        cur = cur->next;
    }
}

void updateProcessStatus(process *process_list, int pid, int status)
{
    process *current_process = process_list;
    
    while (current_process)
    {
        if (current_process->pid != pid)
        {
            current_process = current_process->next;
            continue;
        }
	printf("updating process %s to status %d\n", current_process->cmd->arguments[0], status);
        current_process->status = status;
        return;
    }
    printf("no such process - %d\n", pid);
}

process* deleteProcess(process **process_list, process *to_delete){
    process* temp = *process_list;
    process* prev = NULL;
    if (temp != NULL && temp == to_delete)
    {
        *process_list = temp->next; // Changed head
        free(temp);            // free old head
        return *process_list;
    }
 
    while (temp != NULL && temp != to_delete)
    {
        prev = temp;
        temp = temp->next;
    }
 
    if (temp == NULL)
        return NULL;
 
    prev->next = temp->next;
 
    free(temp);

    return prev->next;
}

void addProcess(process **process_list, cmdLine *cmd, pid_t pid) {
    process *ps = (process *) malloc(sizeof(process));
    ps->cmd = cmd;
    ps->pid = pid;
    ps->status = RUNNING;
    ps->next = NULL;
    printf("before assignment:\n\n");
    printf("process list pointer is %p\n", process_list);
    printf("process list value is %p\n", *process_list);
    if (!*process_list)
    {
        /* process_list = malloc(sizeof(process*)); */
        *process_list = ps;
        return;
    }
    ps->next = *process_list;
    *process_list = ps;
    printf("after assignment:\n\n");
    printf("process list pointer is %p\n", process_list);
    printf("process list value is %p\n", *process_list);
}

void printProcessList(process **process_list) {
    if (*process_list == NULL) {
        printf("process list is empty\n");
        return;
    }
    updateProcessList(process_list);
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

    int spaces_num = (w.ws_col - strlen("process_idcommandprocess_status")) / 2;
    printf("%s %*s %*s\n", "process_id", spaces_num, "command", spaces_num, "process_status");
    process *current_process = *process_list;

    while (current_process)
    {
        char *string_status = current_process->status == -1 ? "TERMINATED":
                                current_process->status == 1 ? "RUNNING" :
                                "SUSPENDED";
        printf("%d %*s %*s\n", current_process->pid,spaces_num, current_process->cmd->arguments[0], spaces_num, string_status);
        if (current_process->status == TERMINATED)
        {
	    current_process = deleteProcess(process_list, current_process);
	    continue;
        }
        current_process = current_process->next;
    }
}

void freeProcessList(process *process_list) {
    process *current;
    process *next;
    if (process_list == NULL)
        return;
    current = next = process_list;
    while (current != NULL) {
        next = next->next;
        free(current);
        current = next;
    }
}


void simulate_chdir(char *cwd, char *path) {
    strcat(cwd, "/");
    strcat(cwd, path);
    if (chdir(cwd) == -1)
        perror(NULL);
}


int execute(cmdLine *pCmdLine, process **process_list) {
    int status;
    pid_t ch_pid = fork();
    if (ch_pid == -1) {
        perror(NULL);
        _exit(1);
    }

    if (ch_pid > 0) {
        if (pCmdLine->blocking)
            wait(&status);
        addProcess(process_list,pCmdLine,ch_pid);
    } else {
        if (execvp(pCmdLine->arguments[0], pCmdLine->arguments) == -1)
        {
            perror(NULL);
            _exit(errno);
        }      
    }
    return 0;
}

int main(int argc, char **argv) {
    char cwd[MAX_PATH];
    process *process_list = NULL;
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
        if (strcmp(user_input, "") == 0)
            continue;
        current_cmd = parseCmdLines(user_input);
        if (strcmp(current_cmd->arguments[0], "cd") == 0) {
            simulate_chdir(cwd, current_cmd->arguments[1]);
            continue;
        }
        if (strcmp(current_cmd->arguments[0], "showprocs") == 0) {
            printProcessList(&process_list);
            continue;
        }
        if (strcmp(current_cmd->arguments[0], "nap") == 0)
        {
            nap(current_cmd);
            continue;
        }
        if (strcmp(current_cmd->arguments[0], "stop") == 0)
        {
            stop(atoi(current_cmd->arguments[1]));
            continue;
        }
        execute(current_cmd, &process_list);
    }
    return 0;
}
