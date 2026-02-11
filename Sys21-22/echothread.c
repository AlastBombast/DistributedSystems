/*
EPI GIJÓN - University of Oviedo
DEGREE IN COMPUTER SCIENCE/ENGINEERING
DISTRIBUTED SYSTEMS - 3rd YEAR
COURSE MATERIAL
-------------------------
MECHANISM: THREADS
FILE: echo-threads-on-demand.c
DESCRIPTION: Implementation of a server that implements the ECHO service RCF 0862. Only the service is implemented under TCP. Attendance is achieved creating a thread every time a client arrives. The thread provide the echo service and the parent waits for another client. When the thread finishes giving the service, it ends.
*/

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

#define PORT 48666 // Default port (remember port 7 is a reserved port)
#define MAX_LINE 80

/**** Data type definitions ****/
// Connection info types
struct conn
{
    char *client_ip;
    int client_port;
    int data_socket;
    unsigned int n_thread;
};
typedef struct conn connection_data;
/*************************************/

// Listening and data socket
int listening_socket, data_socket;

// SigInt handler (Ctrl-C). When we press Ctrl-C, the listening socket will close
void ExitAsAPro()
{
    printf("INT signal received. Cleaning up and exiting.\n");
    close(listening_socket);
    exit(0);
}

// ********************************************************
// Socket API helper functions

int ListeningSocket(int port)
{
    int listening_socket;
    struct sockaddr_in address;

    // Create listening/passive socket (TCP)
    listening_socket = socket(PF_INET, SOCK_STREAM, 0);
    if (listening_socket == -1)
    {
        perror("Error when creating the passive socket");
        exit(-1);
    }

    // Asign IP and port to the socket
    address.sin_family = AF_INET;                // Address type
    address.sin_port = htons(port);              // Local IP
    address.sin_addr.s_addr = htonl(INADDR_ANY); // Port
    if (bind(listening_socket, (struct sockaddr *)&address, sizeof(address)) == -1)
    {
        perror("Error when assigning the address to the ps");
        close(listening_socket);
        exit(-1);
    }

    // Socket in listening/passive mode
    if (listen(listening_socket, SOMAXCONN) == -1)
    {
        perror("Error when setting listening mode");
        close(listening_socket);
        exit(-1);
    }

    return listening_socket;
}

int Accept(int listening_socket, char **ip, int *port)
{
    int data_socket;
    struct sockaddr_in address;
    static socklen_t len = sizeof(address);

    data_socket = accept(listening_socket, (struct sockaddr *)&address, &len);
    if (data_socket == -1)
    {
        perror("Error when accepting a client");
        exit(-1);
    }

    *ip = inet_ntoa(address.sin_addr);
    *port = ntohs(address.sin_port);

    return data_socket;
}

int Send(int socket, char *buff, int len)
{
    int bytes_sent;
    bytes_sent = send(socket, buff, len, 0);
    if (bytes_sent == -1)
    {
        perror("When sending to client");
        exit(-1);
    }

    return bytes_sent;
}

int Receive(int data_socket, char *buff, int len)
{
    int bytes_received;
    bytes_received = recv(data_socket, buff, len, 0);
    if (bytes_received == -1)
    {
        perror("Error when receiving from client");
        exit(-1);
    }

    return bytes_received;
}

void Close(int socket)
{
    if (close(socket) == -1)
    {
        perror("Error when closing the socket");
        exit(-1);
    }
}
// ********************************************************

// Worker thread
void *EchoService(connection_data *conn)
{
    int bytes_received;
    char buffer[MAX_LINE];
    printf("Thread %d: Received connection from %s:%d\n", conn->n_thread, conn->client_ip, conn->client_port);

    // Read socket while client connection is up
    do
    {
        bytes_received = Receive(conn->data_socket, buffer, MAX_LINE);
        printf("Thread %d: Got a message of length %d\n", conn->n_thread, bytes_received);
        buffer[bytes_received] = 0; // Add end of string byte
        printf("Thread %d: Msg: %s\n", conn->n_thread, buffer);
        Send(conn->data_socket, buffer, strlen(buffer));
    } while (bytes_received != 0);

    printf("Thread %d: The client close the connection. Bye bye cursed world.\n", conn->n_thread);
    Close(conn->data_socket);

    // Resources deallocation
    free(conn->client_ip);
    free(conn);

    return NULL; // A thread function must return a pointer. In this case NULL, because we don't care about the return status
}

int main(int argc, char *argv[])
{
    int server_port, client_port, thread_counter = 0;
    char *client_ip;
    pthread_t tid;
    connection_data *conn;

    if (argc < 2)
    {
        printf("Usage: %s <server_port>\n", argv[0]);
        printf("Default server port: %d\n", PORT);
    }

    if (argc > 1)
        server_port = atoi(argv[1]);
    else
        server_port = PORT;

    // Capturing SIGINT Ctrl-C to free allocated resources before exiting
    signal(SIGINT, ExitAsAPro);

    // Create a passive socket (global variable)
    listening_socket = ListeningSocket(server_port);

    while (1)
    {
        // Waiting for new client connections (global variablee)
        data_socket = Accept(listening_socket, &client_ip, &client_port);

        // Memory allocation to store connection info
        conn = (connection_data *)malloc(sizeof(connection_data));
        conn->client_ip = (char *)malloc(strlen(client_ip) + 1);
        strcpy(conn->client_ip, client_ip);
        conn->client_port = client_port;
        conn->data_socket = data_socket; // The data socket (global) is saved for each client (thread)
        conn->n_thread = thread_counter;

        // Create a worker thread
        pthread_create(&tid, NULL, EchoService, conn);

        // We make the thread independent
        pthread_detach(tid);

        printf("Main server: Thread %d up and running\n", thread_counter);

        // Increase thread counter
        thread_counter++;
    }

    return 0; // This code is not reachable but prevents the compiler from throwing warnings
}