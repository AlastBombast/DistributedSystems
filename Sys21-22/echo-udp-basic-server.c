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

int main(int argc, char *argv[])
{

    int data_socket, bytes_received;
    struct sockaddr_in local_addr, client_addr;
    socklen_t length_addr = sizeof(client_addr);
    char *msg = "Hi";
    char buffer[50];

    data_socket = socket(PF_INET,SOCK_DGRAM,0);
    
    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    local_addr.sin_port = htons(7890);
    
    bind(data_socket, (struct sockaddr*)&local_addr, sizeof(local_addr));

    while(1){

        bytes_received = recvfrom(data_socket,buffer,strlen(buffer),0,(struct sockaddr*)&client_addr,&length_addr);
        printf("Client since %s (%d)\n", inet_ntoa(client_addr.sin_addr),ntohs(client_addr.sin_port));
        sendto(data_socket, msg, strlen(msg),0,(struct sockaddr*)&client_addr,length_addr);

    }
   
}