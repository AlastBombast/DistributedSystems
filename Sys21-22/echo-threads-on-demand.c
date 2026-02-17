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

struct conn
{
    char *client_ip;
    int client_port;
    int data_socket;
    unsigned int n_thread;
};

typedef struct conn connection_data;

int passive_socket, data_socket;

void exitCommand()
{
	printf("INT signal recieved; Exiting...");
	close(passive_socket);
	exit(0);
};

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

	if (bind(passive_socket, (struct sockaddr *)&address, sizeof(address)) == -1){
		perror("Error when assigning the address to the ps");
		close(passive_socket);
		exit(-1);
	}

	if (listen(passive_socket, SOMAXCONN) == -1){
		perror("Error when setting listening mode");
		close(passive_socket);
		exit(-1);
	}

	return passive_socket;
};

int acceptCommand(int passive_socket, char **ip, int *port)
{
	int data_socket;
	struct sockaddr_in address;
	static socklen_t len = sizeof(address);

	data_socket = accept(passive_socket, (struct sockaddr *)&address, &len);

	if (data_socket == -1){
		perror("Error when accepint client");
		exit(-1);
	}

	*ip = inet_ntoa(address.sin_addr);
	*port = ntohs(address.sin_port);

	return data_socket;
}

int sendCommand(int socket, char *buffer, int len)
{
	int bytes_sent;
	bytes_sent = send(socket, buffer, len, 0);
	
	if (bytes_sent == -1){
		perror("Error when sending to client");
		exit(-1);
	}

	return bytes_sent;
}

int receiveCommand(int data_socket,char *buffer, int len)
{
	int bytes_received;
	bytes_received = recv(data_socket, buffer, len, 0);

	if (bytes_received == -1){
		perror("Error when receiving");
		exit(-1);
	}

	return bytes_received;
}

void closeCommand(int socket)
{
	if (close(socket) == -1){
		perror("Error when closing socket");
		exit(-1);
	}
}

void *EchoService(connection_data *conn)
{
	int bytes_received;
	char buffer[MAX_LINE];

	printf("Thread %d: Received connection from %s:%d\n", conn->n_thread, conn->client_ip, conn->client_port);

	do{
		bytes_received = receiveCommand(conn->data_socket, buffer, MAX_LINE);
		printf("Thread %d: Got a message with length %d\n", conn->n_thread, bytes_received);
		buffer[bytes_received] = 0;
		printf("Thread %d: Msg:%s\n", conn->n_thread, buffer);
		sendCommand(conn->data_socket, buffer, strlen(buffer));
	} while (bytes_received != 0);

	printf("Thread %d: Client closed the connection\n", conn->n_thread);
	closeCommand(conn->data_socket);

	free(conn->client_ip);
	free(conn);

	return NULL;
}



int main(int argc, char *argv[])
{

    int server_port, client_port, thread_counter = 0;
    char *client_ip;
    pthread_t tid;
    connection_data *conn;
    
    if (argc < 2){
    	printf("Usage: %s <server_port>\n", argv[0]);
    	printf("Default server port: %d\n", PORT);
    }

    if (argc > 1){
    	server_port = atoi(argv[1]);
    }

    else{
    	server_port = PORT;
    }

    signal(SIGINT, exitCommand);

    passive_socket = passiveSocket(server_port);


    while(1)
    {
		
        data_socket = acceptCommand(passive_socket, &client_ip , &client_port);

	conn = (connection_data *)malloc(sizeof(connection_data));
	conn->client_ip = (char *)malloc(strlen(client_ip) + 1);
	strcpy(conn->client_ip, client_ip);
	conn->client_port = client_port;
	conn->data_socket = data_socket;
	conn->n_thread = thread_counter;

	pthread_create(&tid, NULL, EchoService, conn);
	
	pthread_detach(tid);

	printf("Main Server: Thread %d running..\n", thread_counter);

	thread_counter++;

    }
   
    return 0;

}
