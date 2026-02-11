#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <ctype.h>

int main(int argc, char *argv[])
{

    int passive_socket, data_socket;
    struct sockaddr_in local_addr;
    char buffer[100];

    passive_socket = atoi(argv[0]);
    passive_socket = socket(PF_INET,SOCK_STREAM,0);
    
    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    local_addr.sin_port = htons(7890);
    
    bind(passive_socket, (struct sockaddr*)&local_addr, sizeof(local_addr));
    listen(passive_socket, SOMAXCONN);

    while(1){

        data_socket = accept(passive_socket, 0 , 0);
        recv(data_socket,buffer,100,0);
        for (int i=0; i<strlen(buffer);i++){
            buffer[i] = toupper(buffer[i]);
        }
        printf("%s", buffer);
        send(data_socket, buffer, strlen(buffer),0);
        close(data_socket);

    }
   
}