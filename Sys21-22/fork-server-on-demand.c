#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
	int passive_socket, data_socket;
	pid_t child;
	struct sockaddr_in local_addr;
	char buffer[100];
	buffer[0] = '\0';
	local_addr.sin_family = AF_INET;
	local_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	passive_socket = socket(PF_INET, SOCK_STREAM, 0);
	int bind_val = bind(passive_socket, (struct sockaddr*)&local_addr, sizeof(local_addr));
	listen(passive_socket, SOMAXCONN);

	for(;;) {
		socklen_t laddr_size = sizeof(local_addr);
		data_socket = accept(passive_socket, (struct sockaddr*)&local_addr, &laddr_size);

		child = fork();

		if (child == 0) {
			printf("Child with %d is serving client\n", getpid());
			close(passive_socket);

			if (recv(data_socket,buffer,sizeof(buffer), 0)) {
				printf("%s\n", buffer);
			}
			
			printf("Exiting");
			exit(0);
		}

		else {
			close(data_socket);
		}
	}
	
	return 0;
}

