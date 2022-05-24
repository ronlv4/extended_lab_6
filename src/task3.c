#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/wait.h>

#include "limits.h"
#include "string.h"
#include "LineParser.h"
#include "pipes.c"

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
process *deleteProcess();
void addProcess();
void printProcessList();
void freeProcessList();
void simulate_chdir();
int execute();

void nap(process **process_list, cmdLine *pCmdLine) {
    int status, ch_pid;
    int time = atoi(pCmdLine->arguments[1]);
    int ps_pid = atoi(pCmdLine->arguments[2]);
    process *cur = *process_list;
    while (cur) {
        if (cur->pid != ps_pid){
            cur = cur->next;
            continue;
        }
        if (cur->status != RUNNING) {
            printf("process %d is not RUNNING\n", ps_pid);
            return;
        }
        ch_pid = fork();
        if (ch_pid == -1) {
            perror(NULL);
            _exit(errno);
        }
        if (ch_pid) {
            if (pCmdLine->blocking)
                waitpid(ch_pid, &status, 0);
        } else {
            kill(ps_pid, SIGTSTP);
            sleep(time);
            kill(ps_pid, SIGCONT);
        }
        return;
    }
    printf("process %d does not exist\n", ps_pid);
}

void stop(process **process_list, pid_t ps_pid) {
    int status, ch_pid;
    process *cur = *process_list;
    while (cur) {
        if (cur->pid != ps_pid){
            cur = cur->next;
            continue;
        }
        if (cur->status != RUNNING && cur->status != SUSPENDED) {
            printf("process %d is not RUNNING or SUSPENDED\n", ps_pid);
            return;
        }
        ch_pid = fork();
        if (ch_pid == -1) {
            perror(NULL);
            _exit(errno);
        }
        if (ch_pid) {
                waitpid(ch_pid, &status, 0);
        } else {
            kill(ps_pid, SIGINT);
        }
        return;
    }
    printf("process %d does not exist\n", ps_pid);
}

void updateProcessList(process **process_list) {
    int status, ans;
    process *cur = *process_list;
    while (cur) {
        pid_t ret = waitpid(cur->pid, &ans, WNOHANG | WCONTINUED | WUNTRACED);
        if (ret != 0) {
            status = (WIFSIGNALED(ans) || WIFEXITED(ans)) ? TERMINATED :
                     WIFSTOPPED(ans) ? SUSPENDED :
                     WIFCONTINUED(ans) ? RUNNING :
                     2;
            if (status != 2)
                updateProcessStatus(cur, cur->pid, status);
        }
        cur = cur->next;
    }
}

void updateProcessStatus(process *process_list, int pid, int status) {
    process *current_process = process_list;

    while (current_process) {
        if (current_process->pid != pid) {
            current_process = current_process->next;
            continue;
        }
        current_process->status = status;
        return;
    }
    printf("no such process - %d\n", pid);
}

process *deleteProcess(process **process_list, process *to_delete) {
    process *temp = *process_list;
    process *prev = NULL;
    if (temp != NULL && temp == to_delete) {
        *process_list = temp->next;
        free(temp);
        return *process_list;
    }

    while (temp != NULL && temp != to_delete) {
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
    if (!*process_list) {
        ps->next = NULL;
        *process_list = ps;
        return;
    }
    ps->next = *process_list;
    *process_list = ps;
}

void printProcessList(process **process_list) {
    if (*process_list == NULL) {
        printf("process list is empty\n");
        return;
    }
    updateProcessList(process_list);

    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

    unsigned int spaces_num = (w.ws_col - strlen("process_idcommandprocess_status")) / 2;
    printf("%s %*s %*s\n", "process_id", spaces_num, "command", spaces_num, "process_status");
    process *current_process = *process_list;

    while (current_process) {
        char *string_status = current_process->status == -1 ? "TERMINATED" :
                              current_process->status == 1 ? "RUNNING" :
                              "SUSPENDED";
        printf("%d %*s %*s\n", current_process->pid, spaces_num, current_process->cmd->arguments[0], spaces_num,
               string_status);
        if (current_process->status == TERMINATED) {
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
    int nPipes = count_pipes(pCmdLine);
    int **pipes = createPipes(nPipes);
    int *left_pipe, *right_pipe;
    cmdLine *cur = pCmdLine;
    pid_t ch_pid;
    int wstatus[nPipes + 1];
    int ch_pids[nPipes + 1];
    int input_fd, output_fd;
    int is_input_redirect;
    int is_output_redirect;

    while (cur) {
        input_fd = STDIN_FILENO;
        output_fd = STDOUT_FILENO;
        is_input_redirect = cur->inputRedirect != NULL;
        is_output_redirect = cur->outputRedirect != NULL;
        left_pipe = leftPipe(pipes, cur);
        right_pipe = rightPipe(pipes, cur);
        ch_pid = fork();
        if (ch_pid) {
            if (left_pipe) {
                close(left_pipe[0]);
            }
            if (right_pipe) {
                close(right_pipe[1]);
            }
            ch_pids[cur->idx] = ch_pid;
            addProcess(process_list, cur, ch_pid);
        } else {
            if (is_input_redirect) {
                input_fd = open(cur->inputRedirect, O_RDONLY);
            }
            if (is_output_redirect) {
                output_fd = open(cur->outputRedirect, O_WRONLY);
            }
            if (left_pipe) {
                input_fd = left_pipe[0];
            }
            if (right_pipe) {
                output_fd = right_pipe[1];
            }
            if (output_fd != STDOUT_FILENO) {
                close(STDOUT_FILENO);
                dup(output_fd);
                close(output_fd);
            }
            if (input_fd != STDIN_FILENO) {
                close(STDIN_FILENO);
                dup(input_fd);
                close(input_fd);
            }
            execvp(cur->arguments[0], cur->arguments);
        }
        cur = cur->next;
    }

    cur = pCmdLine;
    while (cur){
        if (cur->blocking)
            waitpid(ch_pids[cur->idx], wstatus + cur->idx, 0);
        cur = cur->next;
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
        write(STDOUT_FILENO, cwd, strlen(cwd));
        write(STDOUT_FILENO, "$ ", 2);
        fflush(STDIN_FILENO);
        fgets(user_input, MAX_INPUT, stdin);
        user_input[strlen(user_input) - 1] = '\0';
        if (strcmp(user_input, "quit") == 0) {
            freeProcessList(process_list);
            _exit(0);
        }
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
        if (strcmp(current_cmd->arguments[0], "nap") == 0) {
            nap(&process_list, current_cmd);
            continue;
        }
        if (strcmp(current_cmd->arguments[0], "stop") == 0) {
            stop(&process_list, atoi(current_cmd->arguments[1]));
            continue;
        }
        execute(current_cmd, &process_list);
    }
    return 0;
}
