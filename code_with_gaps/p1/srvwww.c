#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <signal.h>
#include <pthread.h>
#include <errno.h>
#include <unistd.h>
#include <sched.h>
#include <semaphore.h>
#include <fcntl.h>

// Header files for socket manipulation
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "queue.h"
#include "util.h"
#include "protocol.h"

// ====================================================================
// FUNCTION PROTOTYPES
// ====================================================================
static void handler(int signum);

// ====================================================================
// GLOBAL VARIABLES
// ====================================================================

// Queue where incoming www server requests are inserted
Queue request_queue;

// Port on which we wait for messages
int port;

// Variable storing the number of worker threads
int num_worker_threads;

// Pointer to the beginning of the worker thread identifier array
pthread_t *worker_threads;

// Size of the circular queue
int queue_size;

// Global variable storing the name of the base folder
// where served files are located
char *documentroot = NULL;

// Counting semaphore for the number of used descriptors
sem_t semdesc;

// ====================================================================
// SIGNAL handler function for signals received by the process
// ====================================================================
static void handler(int signum)
{
    register int i;

    switch (signum)
    {
    case SIGINT:
        // Termination. Destroy semaphore, queue, and free memory
        free_queue(&request_queue);
        free(worker_threads);
        sem_destroy(&semdesc);
        exit(0);
    default:
        pthread_exit(NULL);
    }
}

// Function that returns TRUE if the folder name passed
// as argument exists and is a directory
unsigned char folder_exists(const char *name)
{
    struct stat buffer;
    return (stat(name, &buffer) == 0) && S_ISDIR(buffer.st_mode);
}

// Function that checks whether a file exists
unsigned char file_exists(const char *filename)
{
    struct stat buffer;
    return stat(filename, &buffer) == 0 ? TRUE : FALSE;
}


void process_arguments(int argc, char *argv[])
{
    if (argc < 5)
    {
        fprintf(stderr, "Usage: %s port queue_size num_worker_threads absolute_document_root_path\n", argv[0]);
        exit(1);
    }
    if (!validate_number(argv[1]))
    {
        fprintf(stderr, "Error: Port is not a number.\n");
        exit(3);
    }
    port = atoi(argv[1]);
    if ((port < 1024) || (port > 65535))
    {
        fprintf(stderr, "Error: Port must be greater or equal to 1024 and less or equal to 65535\n");
        exit(4);
    }

    // Validate and store the queue size parameter
    // TODO: TO BE COMPLETED BY STUDENT
    ...
    ...

    // Validate and store the number of worker threads
    // TODO: TO BE COMPLETED BY STUDENT
    ...
    ...

    documentroot = strdup(argv[4]);
    if (documentroot == NULL)
    {
        fprintf(stderr, "Error: Could not allocate memory to store document root folder name\n");
        exit(12);
    }
    if (documentroot[strlen(documentroot) - 1] == '/')
        documentroot[strlen(documentroot) - 1] = 0;

    if (!folder_exists(documentroot))
    {
        fprintf(stderr, "Error: Folder %s does not exist or is not a directory\n", documentroot);
        exit(13);
    }
}

// Function that returns the size of a file in bytes
long get_file_size(const char *filename)
{
    struct stat st;

    st.st_size = -1; // A file cannot have a negative size
    if (stat(filename, &st) == -1)
    {
        fprintf(stderr, "Error obtaining size of file %s: %s\n", filename, strerror(errno));
        return -1;
    }
    return st.st_size;
}

char *get_file_chunk(const char *filename, int offset, int size)
{
    int fd;
    static char *buffer = NULL;

    sem_wait(&semdesc); // Wait until a descriptor is available
    if ((fd = open(filename, O_RDONLY)) < 0)
    {
        fprintf(stderr, "Error opening file %s: %s\n", filename, strerror(errno));
        return NULL;
    }
    if (lseek(fd, offset, SEEK_SET) < 0)
    {
        fprintf(stderr, "Error positioning cursor in file %s: %s\n", filename, strerror(errno));
        close(fd);
        return NULL;
    }
    buffer = (char *)malloc(size * sizeof(char)); // Allocate memory for the file chunk
    if (buffer == NULL)
    {
        fprintf(stderr, "Error allocating memory for file chunk %s\n", filename);
        close(fd);
        return NULL;
    }
    if (read(fd, buffer, size) < 0)
    {
        fprintf(stderr, "Error reading file %s: %s\n", filename, strerror(errno));
        free(buffer);
        close(fd);
        return NULL;
    }
    close(fd);
    sem_post(&semdesc); // Release descriptor
    return buffer;
}

void send_file(const char *filename, int sock)
{
    int fd;
    char buffer[MSG_SIZE];
    long size;
    int bytes_read;

    sem_wait(&semdesc); // Wait until a descriptor is available
    if ((fd = open(filename, O_RDONLY)) < 0)
    {
        fprintf(stderr, "Error opening file %s: %s\n", filename, strerror(errno));
        close(sock);
        sem_post(&semdesc); // Release descriptor
        return;
    }
    while ((bytes_read = read(fd, buffer, MSG_SIZE)) > 0)
    {
        if (sendStringTCP(sock, buffer, bytes_read) < 0)
        {
            fprintf(stderr, "Error sending data from file %s: %s\n", filename, strerror(errno));
            break;
        }
    }
    close(fd);
    sem_post(&semdesc); // Release descriptor
}

// ====================================================================
// Worker thread implementation
// ====================================================================

void *Worker(int *id)
{
    int id_worker;               // Worker thread identifier
    queue_item *req;             // Item obtained from the queue
    char *token;                 // For tokenizing the received message
    char *loc;                   // Part of tokenization
    char screen[LINE_SIZE];      // Buffer for debug messages
    char msg[LINE_SIZE];         // Buffer for messages to send
    char *cmd;                   // Command extracted from the received message
    char *argument;              // Argument extracted from the received message
    char *aux;                   // Auxiliary pointer to the beginning of the chunk to send
    char filepath[DIR_SIZE];     // To store the file path, including documentroot
    int num;                     // To store integer values
    int offset;                  // To store file offset
    int size;                    // To store file size

    id_worker = *id;

    // Once the id is stored, free the memory reserved for the worker thread identifier
    free(id);

    // Debug information
    sprintf(screen, "Worker %d starts\n", id_worker);
    log_debug(screen);

    cmd = NULL;
    argument = NULL;

    // Worker code: infinite loop
    while (1)
    {
        req = dequeue(&request_queue);
        sprintf(screen, "Worker %d: Extracted request: %s\n", id_worker, req->msg);
        log_debug(screen);

        // Start tokenizing the received message
        loc = NULL;
        // TODO: TO BE COMPLETED BY STUDENT
        ...
        ...

        if (token == NULL)
        {
            fprintf(stderr, "Error: Malformed message\n");
            if (req->is_stream)
            {
                CloseSocket(req->s);
                sem_post(&semdesc); // Release descriptor semaphore
            }
            free(req);
            continue;
        }

        // First token: the command
        cmd = strdup(token);

        // Next token: the first argument
        token = strtok_r(NULL, " \n\r", &loc);
        argument = strdup(token);

        bzero(filepath, DIR_SIZE);
        // Build file path (documentroot/argument)
        sprintf(filepath, "%s/%s", documentroot, argument);

        // Take different actions depending on the received command

        if (strcmp(cmd, "EXISTFILE") == 0)
        {
            // Check if file exists and prepare response message
            // TODO: TO BE COMPLETED BY STUDENT
            ...
            ...
        }
        else if (strcmp(cmd, "GETSIZE") == 0)
        {
            // Obtain file size and prepare response message
            // TODO: TO BE COMPLETED BY STUDENT
            ...
            ...
        }
        else if (strcmp(cmd, "GETFILE") == 0)
        {
            // TODO: TO BE COMPLETED BY STUDENT
            ...
            ...
        }
        else if (strcmp(cmd, "GETCHUNK") == 0)
        {
            // TODO: TO BE COMPLETED BY STUDENT
            ...
            ...
        }

        // Send the prepared message through the appropriate socket,
        // using the appropriate functions, depending on req->is_stream
        if (req->is_stream)
        {
            // Send via TCP socket and then close it,
            // since we are done with this client
            // (which frees a descriptor for the semaphore)
            // TODO: TO BE COMPLETED BY STUDENT
            ...
            ...
        }
        else // request via UDP
        {
            // Send via UDP socket, which is not closed,
            // because it will be reused for other UDP clients
            // TODO: TO BE COMPLETED BY STUDENT
            ...
            ...
        }

        // Free memory to avoid leaks
        free(cmd);
        free(argument);
        free(req);
    }
}
void ReceiveRequests(int sockTCP, int sockUDP)
{
    int received;                          // Number of bytes received
    struct sockaddr_in client_addr;        // Client address
    socklen_t addr_len = sizeof(client_addr); // Length of the address
    char screen[MSG_SIZE];                 // Buffer for debug messages
    char bufferTCP[MSG_SIZE];              // Buffer for messages received via TCP
    char bufferUDP[MSG_SIZE];              // Buffer for messages received via UDP
    queue_item *p;                         // Pointer to queue data structure
    int s;                                 // Descriptor of socket obtained in accept
    fd_set fds;                             // Descriptor set for select
    int maximum;                            // Maximum descriptor number for select

    // Debug information
    sprintf(screen, "Request Reception Thread starts\n");
    log_debug(screen);

    maximum = max(sockTCP, sockUDP);

    while (1) // Infinite loop for handling messages
    {
        // Prepare fds and call select
        // TODO: TO BE COMPLETED BY STUDENT
        ...
        ...

        // Check if there are clients on the TCP socket
        if (FD_ISSET(sockTCP, &fds))
        {
            bzero(bufferTCP, MSG_SIZE);

            // Accepting the request consumes a descriptor,
            // so we must account for it in the appropriate semaphore
            // TODO: TO BE COMPLETED BY STUDENT
            ...
            ...

            // Receive the message
            received = recvStringTCP(s, bufferTCP, sizeof(bufferTCP));

            // Create a queue_item structure
            p = (queue_item *)malloc(sizeof(queue_item));
            if (p == NULL)
            {
                fprintf(stderr, "Could not allocate memory for a new request\n");
                exit(15);
            }
            else
            {
                // Fill the structure according to the received request
                // and insert it into the queue
                // TODO: TO BE COMPLETED BY STUDENT
                ...
                ...
            }
        }

        // Check if there is data on the UDP socket
        if (FD_ISSET(sockUDP, &fds))
        {
            bzero(&client_addr, sizeof(client_addr));
            bzero(bufferUDP, MSG_SIZE);

            // Receive data via UDP (this does NOT consume a new descriptor,
            // since the UDP socket already existed and is always the same)
            received = recvStringUDP(sockUDP, bufferUDP, sizeof(bufferUDP), &client_addr);

            // Create a queue_item structure
            p = (queue_item *)malloc(sizeof(queue_item));
            if (p == NULL)
            {
                fprintf(stderr, "Could not allocate memory for a new request\n");
                exit(15);
            }
            else
            {
                // Fill the structure according to the received request
                // and insert it into the queue
                // TODO: TO BE COMPLETED BY STUDENT
                ...
                ...
            }
        }
    }
}

// ====================================================================
// MAIN PROGRAM
// ====================================================================

// Its mission is to create and initialize global synchronization resources,
// and launch all threads

int main(int argc, char *argv[])
{
    register int i;         // Loop index
    int *id;                // Dynamic variable to pass the thread id
    int sock_passive_TCP;   // Passive TCP socket
    int sock_UDP;           // UDP socket
    long max_num_desc;      // Maximum number of descriptors

    process_arguments(argc, argv);
    setbuf(stdout, NULL); // Disable buffering of standard output
    signal(SIGINT, handler); // Set behavior for asynchronous signal arrival

    // Initialize TCP and UDP sockets
    // TODO: TO BE COMPLETED BY STUDENT
    ...
    ...

    // Initialize the queue
    init_queue(&request_queue, queue_size);

    // Obtain the maximum number of descriptors that can be open per user
    max_num_desc = sysconf(_SC_OPEN_MAX);

    // Initialize the semaphore that keeps track of used descriptors.
    // Since the user may have other files open in other processes,
    // we divide the value obtained from sysconf by 2 and subtract
    // standard input, standard output, standard error,
    // the passive TCP socket, and the UDP socket.
    // TODO: TO BE COMPLETED BY STUDENT
    ...
    ...

    // Reserve memory for the array of worker thread handles (global variable)
    worker_threads = (pthread_t *)malloc(sizeof(pthread_t) * num_worker_threads);
    if (worker_threads == NULL)
    {
        fprintf(stderr, "ERROR: Could not allocate memory for worker thread data objects\n");
        exit(14);
    }

    // Create all worker threads, passing each one a different identifier
    // through the dynamic variable id
    for (i = 0; i < num_worker_threads; i++)
    {
        // TODO: TO BE COMPLETED BY STUDENT
        ...
        ...
    }

    // Invoke the request reception loop
    ReceiveRequests(sock_passive_TCP, sock_UDP);

    // The previous loop never actually returns, so the code below
    // will not be executed. It would otherwise wait for all threads to finish.
    for (i = 0; i < num_worker_threads; i++)
    {
        pthread_join(worker_threads[i], NULL);
    }
}

