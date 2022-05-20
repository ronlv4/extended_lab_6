#include <stdio.h>
#include <unistd.h>
#include <string.h>

void rd(int fd, int child)
{
    char c;
    int first = 1;

    do {
        if (!read(fd, &c, 1))
            break;
        if (first) {
            printf("%s got:", (child) ? "Child" : "Parent");
            first = 0;
        }
        putchar(c);
    } while (c != '\n');
}

void wr(int fd, const char * msg, int child)
{
    write(fd, msg, strlen(msg));
    printf("%s sent:%s", (child) ? "Child" : "Parent", msg);
}

int main() {
    int child_to_parent[2];
    int parent_to_child[2];

    pipe(child_to_parent);
    pipe(parent_to_child);

    pid_t id = fork()