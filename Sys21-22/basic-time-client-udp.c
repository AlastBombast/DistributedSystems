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

    int data_socket, bytes_received;
    struct sockaddr_in local_addr, client_addr;
    socklen_t length_addr = sizeof(client_addr);

    unsigned long int datagram;
    double secondsPerYear = 365.25 * 24 * 60 * 60;
    double years;
    char timeHr[50];
    //char buffer[50];


    data_socket = socket(PF_INET,SOCK_DGRAM,0);
    
    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    local_addr.sin_port = htons(7890);
    
    bind(data_socket, (struct sockaddr*)&local_addr, sizeof(local_addr));

    while(1){

        bytes_received = recvfrom(data_socket, &datagram, sizeof(datagram), 0, (struct sockaddr*)&client_addr, &length_addr);
        
        datagram = time(NULL);
        years = datagram / secondsPerYear;
        snprintf(timeHr,sizeof(timeHr), "%f", 1970.0+years);
         
        printf("Client since %s (%d)\n", inet_ntoa(client_addr.sin_addr),ntohs(client_addr.sin_port));
        sendto(data_socket, timeHr, strlen(timeHr),0,(struct sockaddr*)&client_addr,length_addr);
    }
   
}