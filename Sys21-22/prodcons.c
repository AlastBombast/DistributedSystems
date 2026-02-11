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
    int p[2];
    
    // Create a pipe
    pipe(p);
    
    // This process should send the file to the other process through the pipe
    if (fork() == 0)
    {
        dup2(p[0], 0);
        close(p[1]);
        
        FILE* fd;

        if ((fd = fopen("./file1.txt","r")) == NULL){
            execlp("touch", "touch", "./file1.txt", NULL);
            fd = fopen("./file1.txt", "r");
        }
        
        char buffer[10];

        while (fscanf(fd, "%s", buffer) > 0)
        {
            write(1, buffer, 10);
        }

        fclose(fd);
    }

    // This other process is in charge of obtaining the pipe file and displaying it as standard output
    else if (fork() == 0)
    {
        dup2(p[1], 1);
        close(p[0]);

        char buffer[10];
        int count;

        while(count = read(0, buffer, strlen(buffer)) > 0)
        {
            printf("%s", buffer);
        }
    close(0);
    }

    // The parent closes both descriptors of the pipe and waits for his children.
    else
    {
        close(p[0]);
        close(p[1]);
        wait(NULL);
        wait(NULL);
    }

    return 0;
}