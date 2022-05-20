#include "stdio.h"
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char **argv) {
    int pipe_fd[2];
    pipe(pipe_fd);
    int duplicated_read, duplicated_write;
    int wstatus1, wstatus2;
    pid_t ch1_pid;
    ch1_pid = fork();
    if (ch1_pid == -1)
    {
        perror(NULL);
        _exit(errno);
    }
    if (ch1_pid){
        close(pipe_fd[1]);
        pid_t ch2_pid = fork();
        if (ch2_pid){
            close(STDIN_FILENO);
            duplicated_write = dup(pipe_fd[0]);
            close(pipe_fd[0]);
            execvp("tail", {"tail", "-n", "2"});
        } else{
            close(pipe_fd[0]);
            waitpid(ch1_pid, &wstatus1);
            waitpid(ch2_pid, &wstatus2);
        }

    } else{
        close(STDOUT_FILENO);
        duplicated_read = dup(pipe_fd[1]);
        close(pipe_fd[1])
        execevp("ls", {"ls", "-l"});
    }
}
