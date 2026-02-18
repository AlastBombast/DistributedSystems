#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/select.h>
#include <unistd.h>

int passiveSocket(int port)
{
	int passive_socket;
	struct sockaddr_in address;

	passive_socket = socket(PF_INET, SOCK_STREAM, 0);
	if (passive_socket == -1){
		perror("Error when creating passive socket");
		exit(-1);
	}

	address.sin_family = AF_INET;
	address.sin_port = htons(port);
	address.sin_addr.s_addr = htonl(INADDR_ANY);

	if(bind(passive_socket, (struct sockaddr *)&address, sizeof(address)) == -1){
		perror("Error when binding passive socket");
		close(passive_socket);
		exit(-1);
	}

	if (listen(passive_socket, SOMAXCONN) == -1){
		perror("Error when setting listening mode");
		close(passive_socket);
		exit(-1);
	}

	return passive_socket;
}

int udpSocket(int server_port)
{
	int s;
	struct sockaddr_in addr;
	
	s = socket(PF_INET, SOCK_DGRAM, 0);
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(server_port);
	bind(s, (struct sockaddr *)&addr, sizeof(addr));

	return s;
}

void udpService(int s)
{
	struct sockaddr_in client_addr;
	socklen_t len = sizeof(client_addr);
	char buffer[100];
	int n_read;

	n_read = recvfrom(s, buffer, 100, 0, (struct sockaddr *)&client_addr, &len);
	sendto(s, buffer, n_read, 0, (struct sockaddr *)&client_addr, len);
}

int tcpService(int data_socket)
{
	char buffer[100];
	int n_read;

	n_read = recv(data_socket, buffer, 100, 0);
	if(n_read > 0){
		send(data_socket, buffer, n_read, 0);
	}
	
	return n_read;
}

int max(int a, int b)
{
	if (a > b){
		return a;
	}
	else{
		return b;
	}
}

int getMax(int tcp, int udp, int data)
{
	int res;

	if(data == 0){
		res = max(tcp, udp);
	}
	else{
		res = max(data, udp);
	}

	return res;
}

int main(int argc, char *argv[])
{
	int server_port;
	int passive_socket, tcp_data_socket, udp_socket;
	int max_fds;
	fd_set select_set;

	if(argc < 2){
		printf("Usage: %s server_port \n", argv[0]);
		exit(0);
	}

	server_port = atoi(argv[1]);
	passive_socket = passiveSocket(server_port);
	udp_socket = udpSocket(server_port);
	tcp_data_socket = 0;

	while (1){
		
		FD_ZERO(&select_set);

		FD_SET(udp_socket, &select_set);
	
		if (tcp_data_socket != 0){
			
			FD_SET(tcp_data_socket, &select_set);

		}

		else {
			
			FD_SET(passive_socket, &select_set);
		
		}

		max_fds = getMax(passive_socket, udp_socket, tcp_data_socket);

		select(max_fds +1, &select_set, NULL, NULL, NULL);
		printf("There was some activity on one of the sockets\n");

		if (FD_ISSET(udp_socket, &select_set)){
			printf("UDP socket had activity\n");
			udpService(udp_socket);
		}
		
		if(FD_ISSET(tcp_data_socket, &select_set)){
			int n;
			printf("Client sent some bytes by TCP\n");
			n = tcpService(tcp_data_socket);

			if(n == 0){
				printf("Client closed connection, closing...");
				close(tcp_data_socket);
			}
		}
	}
	
	return 0;
}
