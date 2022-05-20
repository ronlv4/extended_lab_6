#include "stdio.h"

int main(int argc, char **argv) {
    FILE* pipe;
    int wstatus;
    int pid = fork();
    if (pid == -1){
        perror(NULL);
        _exit(errno);
    }
    if (pid){
        wait(&wstatus);
        if (!wstatus){
            int fd = fileno(pipe);
            fwrite()
            fprintf_s()
        }


    }
}
