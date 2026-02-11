#include <unistd.h>
#include <stdio.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#define SUCCESS_CODE 0
#define ERROR_CODE -1

int main(int argc, char **argv)
{
    int fds[2];
    
    // Create a pipe
    // TODO
    pipe(fds);
    
    // This process should send the file to the other process through the pipe
    if (fork() == 0)
    {
        dup2(fds[0], 0);
        close(fds[1]);
        execlp("sort", "sort", NULL);
    }

    // This other process is in charge of obtaining the pipe file and displaying it as standard output
    else if (fork() == 0)
    {
        dup2(fds[1], 1);
        close(fds[0]);
        execlp("who", "who", "-a", NULL);
    }

    // The parent closes both descriptors of the pipe and waits for his children.
    else
    {
        close(fds[0]);
        close(fds[1]);
        wait(0);
        wait(0);
    }

    return 0;
}