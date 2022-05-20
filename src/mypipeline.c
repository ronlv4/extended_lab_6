#include "stdio.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "errno.h"

int main(int argc, char **argv) {
    int pipe_fd[2]; /* {read_end, write_end} */
    pipe(pipe_fd);
    int err_fd = STDERR_FILENO;
    int duplicated_read, duplicated_write;
    int wstatus1, wstatus2;
    pid_t ch1_pid;
    char *ls_args[] = { "ls", "-l", NULL };
    char *tail_args[] = { "tail", "-n", "2", NULL };
    dprintf(err_fd, "(parent_process>forking...)");
    ch1_pid = fork();
    if (ch1_pid == -1)
    {
        perror(NULL);
        _exit(errno);
    }
    if (ch1_pid){
        dprintf(err_fd, "(parent_process>created process with id: %d)\n", ch1_pid);
        dprintf(err_fd, "(parent_process>closing the write end of the pipe...)");
        close(pipe_fd[1]);
        pid_t ch2_pid = fork();
        if (ch2_pid){
            dprintf(err_fd, "(parent_process>closing the read end of the pipe...)");
            close(pipe_fd[0]);
            dprintf(err_fd, "(parent_process>waiting for the child processes to terminate...)");
            waitpid(ch1_pid, &wstatus1, 0);
            waitpid(ch2_pid, &wstatus2, 0);
            dprintf(err_fd, "(parent_process>exiting...)");
        } else{
            dprintf(err_fd, "(child2>redirecting stdin to the read end of the pipe...)");
            close(STDIN_FILENO);
            duplicated_read = dup(pipe_fd[0]);
            close(pipe_fd[0]);
            dprintf(err_fd, "(child2>going to execute cmd: ...)");
            execvp("tail", tail_args);
        }

    } else{
        dprintf(err_fd, "(child1>redirecting stdout to the write end of the pipe...)");
        close(STDOUT_FILENO);
        duplicated_write = dup(pipe_fd[1]);
        close(pipe_fd[1]);
        dprintf(err_fd, "(child1>going to execute cmd: ...)");
        execvp("ls", ls_args);
    }
}
