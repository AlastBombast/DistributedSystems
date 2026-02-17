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
#include <semaphore.h>

#define PORT 48666
#define MAX_LINE 80
#define QUEUE_SIZE 6
#define WORKER_NUM 3

struct jobQueue
{
    int data[QUEUE_SIZE];
    int head, tail;
    pthread_mutex_t mutex;
    sem_t freeSlot;
    sem_t usedSlot;
};

typedef struct jobQueue circularQueue;

struct conn
{
    char *client_ip;
    int client_port;
    int data_socket;
    unsigned int n_thread;
};

typedef struct conn connection_data;

int passive_socket, data_socket;

int initializeQueue(circularQueue *q, int size)
{
	q->head = 0;
	q->tail = 0;

	if (pthread_mutex_init(&q->mutex, NULL) != 0){
		perror("Error in mutex initialization");
		free(q->data);
		exit(-1);
	}

	if (sem_init(&q->freeSlot, 0, size) != 0){
		perror("Error in free slot initialization");
		pthread_mutex_destroy(&q->mutex);
		free(q->data);
		exit(-1);
	}

	if (sem_init(&q->usedSlot, 0, 0) != 0){
		perror("Error in used slot initialization");
		sem_destroy(&q->freeSlot);
		pthread_mutex_destroy(&q->mutex);
		free(q->data);
		exit(-1);
	}

	return 0;
}

void destroyQueue(circularQueue *q)
{
	sem_destroy(&q->freeSlot);
	sem_destroy(&q->usedSlot);
	pthread_mutex_destroy(&q->mutex);
	free(q->data);
}

void insertQueueData(circularQueue *q, int value)
{
	sem_wait(&q->freeSlot);
	pthread_mutex_lock(&q->mutex);
	
	q->data[q->head] = value;
	q->head = (q->head+1) % QUEUE_SIZE;

	pthread_mutex_unlock(&q->mutex);
	sem_post(&q->usedSlot);
}

int getQueueData(circularQueue *q)
{
	int value;

	sem_wait(&q->usedSlot);
	pthread_mutex_lock(&q->mutex);

	value = q->data[q->tail];
	q->tail = (q->tail+1) % QUEUE_SIZE;

	pthread_mutex_unlock(&q->mutex);
	sem_post(&q->freeSlot);

	return value;
}


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


circularQueue gQ;

void *boss(void *arg)
{
	(void)arg;
	static unsigned int conn_counter = 0;
	
	printf("Boss waiting on port: %d", PORT);

	while(1){
		char *client_ip;
		int client_port;

		int data_socket = acceptCommand(passive_socket, &client_ip, &client_port);

		connection_data *conn = malloc(sizeof(connection_data));
		conn->client_ip = malloc(strlen(client_ip) + 1);
		strcpy(conn->client_ip, client_ip);
		conn->client_port = client_port;
		conn->data_socket = data_socket;
		conn->n_thread = conn_counter;

		printf("Boss accepted connection %u from %s:%d - queuing %d",conn_counter, client_ip, client_port, data_socket);
		conn_counter++;

		insertQueueData(&gQ, conn->data_socket);
	}

	return 0;
}

void *worker(void *arg)
{
	int id = *(int *)arg;
	printf("Worker %d ready; tid:%lu", id, (unsigned long)pthread_self());

	while (1) {
		connection_data *conn = (connection_data *)getQueueData(&gQ);
		EchoService(conn);
	}
	return 0;
}

int main(int argc, char *argv[])
{

    int server_port;
    
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

    if (initializeQueue(&gQ, QUEUE_SIZE) != 0){
    	perror("Error on queue initialization");
	close(passive_socket);
	exit(-1);
    }

    printf("Queue initialized; Queue size: %d", QUEUE_SIZE);

    pthread_t boss_tid;
    pthread_create(&boss_tid, NULL, boss, NULL);
    printf("Boss thread created");

    pthread_t worker_tids[WORKER_NUM];
    int worker_ids[WORKER_NUM];

    for(int i=0; i<WORKER_NUM; i++){
    	worker_id[i] = i;
	pthread_create(&worker_tids[i], NULL, worker, &worker_ids[i]);
	printf("Worker %d  thread created", i);
    }

    pthread_join(boss_tid, NULL);
    
    for(int i=0; i<WORKER_NUM; i++){
    	pthread_join(worker_tids[i], NULL);
    }
	
    DestroyQueue(&gQ);
    closeCommand(passive_socket);

    return 0;

}
