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
#include <time.h>

int main(int argc, char *argv[])
{

    int data_socket, bytes_received, tcp_socket;
    struct sockaddr_in local_addr, client_addr, time_server_addr;
    socklen_t length_addr = sizeof(client_addr);

    unsigned long int datagram;
    char * timeHr;
    unsigned int ip = atoi(argv[1]);
    char buffer[50];

    tcp_socket = socket(PF_INET, SOCK_STREAM,0);
    data_socket = socket(PF_INET,SOCK_DGRAM,0);
    
    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    local_addr.sin_port = htons(7890);

    time_server_addr.sin_family = AF_INET;
    time_server_addr.sin_addr.s_addr = htonl(ip);
    time_server_addr.sin_port = htons(37);
    
    bind(data_socket, (struct sockaddr*)&local_addr, sizeof(local_addr));
    bind(tcp_socket, (struct sockaddr*)&local_addr, sizeof(local_addr));

    while(1){

        bytes_received = recvfrom(data_socket, &datagram, sizeof(datagram), 0, (struct sockaddr*)&client_addr, &length_addr);
        
        send(tcp_socket, NULL, sizeof(NULL), 0);
        
        timeHr = ctime(&datagram);

         
        printf("Client since %s (%d)\n", inet_ntoa(client_addr.sin_addr),ntohs(client_addr.sin_port));
        sendto(data_socket, timeHr, strlen(timeHr),0,(struct sockaddr*)&client_addr,length_addr);

    }
   
}