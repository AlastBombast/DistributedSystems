#include "protocol.h"
#include "utils.h"
#include <arpa/inet.h>
#include <ctype.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
//ADDED
#include <time.h>

#define MAX_LINE_SIZE 500
#define MAX_MSG_SIZE 500
#define MAX_NICK_SIZE 10

char *nick; // To send it in each message
char target[MAX_NICK_SIZE];
int tcp_socket;

// These functions are implemented after main()
void read_and_process_keyboard(int udp_socket, char *ip, int port);
void receive_and_show_message(int udp_socket);
int get_max(int a, int b);
int connect_to_server(char *ip, int port); // ADDED to shrink up code

int main(int argc, char *argv[]) {
    int udp_socket;
    int port, peer_port; // For the socket between peers
    struct sockaddr_in udp_addr, server_addr;
    int keyboard = 0; // Descriptor of the standard input
    char *ip;
	//ADDED
	struct timeval pong_interval;
    // For the select
    fd_set listen;
    int max;
    int result;

    // Checking arguments
    //ADDED
    if (argc != 6) {
        printf("Usage: %s <peer_port> <nick> <broker_ip> <broker_port> <pong_interval>\n", argv[0]);
        exit(0);
    }

    nick = argv[2];
    
    // Truncate nick if necessary
    if (strlen(nick) >= MAX_NICK_SIZE - 1)
        nick[MAX_NICK_SIZE] = 0;

    ip = strdup(argv[3]);

    port = atoi(argv[4]);
    peer_port = atoi(argv[1]);

    // Socket initialization
    // We create the UDP socket in the port that is passed to it by command line
    udp_socket = socket(PF_INET, SOCK_DGRAM, 0);
    if (udp_socket == -1) {
        perror("When creating the UDP socket");
        exit(1);
    }
    udp_addr.sin_family = AF_INET;
    udp_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    udp_addr.sin_port = htons(peer_port);
    result = bind(udp_socket, (struct sockaddr *)&udp_addr,
                  sizeof(struct sockaddr_in));
    if (result == -1) {
        perror("When doing bind");
        exit(1);
    }

    printf("Use the /CHAT <ip> <port> command to indicate the destination of "
           "your messages\n");
    printf("The text you write later will be sent to that contact\n\n");
    printf("At any time you can put /CHAT <ip> <port> again\n");
    printf("to switch to a new contact.\n\n");

    // Connect to Server

    result = connect_to_server(ip, port);

    if (result == -1) {
        perror("When connecting to server");
        exit(1);
    }

    // Register to server
    SendByte(tcp_socket, JOIN_CMD);
    SendShort(tcp_socket, strlen(nick));
    SendString(tcp_socket, nick);
    SendEndpoint(tcp_socket, &udp_addr);

    // Read JOIN respone
    short int res = ReadByte(tcp_socket);

    if (res == JOIN_ERR) {
        perror("On registering in server");
        exit(1);
    } else if (res == CMD_OK) {
        printf("Registered succesfully to server\n");
        Close(tcp_socket);
    }

    // And we enter the waiting loop
    while (1) {
        // Print a prompt to invite to write
        printf("%s>", nick);
        fflush(stdout); // To get it out now, since it doesn't end with \n

        // TODO
        // Select initialization

        max = get_max(max, udp_socket);

        FD_ZERO(&listen);
        FD_SET(keyboard, &listen);
        FD_SET(udp_socket, &listen);

        // TODO
        // Wait in select for the user to write something, or for something to
        // arrive over the UDP socket
        
		//ADDED
   	 	pong_interval.tv_sec = atoi(argv[5]);
    	pong_interval.tv_usec = 0;
        select(max + 1, &listen, NULL, NULL, &pong_interval);
		
		//ADDED
		if (!FD_ISSET(udp_socket, &listen) && !FD_ISSET(keyboard, &listen)) {
			int res = connect_to_server(ip, port);
			if (res == -1){
				perror("Error when connecting to server.");
			}
			else {
				SendByte(tcp_socket, PONG_CMD);
				log_pong_sent(nick);
				Close(tcp_socket);
			}
		}
		
        // When exiting, something has happened
        if (FD_ISSET(udp_socket, &listen)) {
            receive_and_show_message(udp_socket);
        }

        if (FD_ISSET(keyboard, &listen)) {
            read_and_process_keyboard(udp_socket, ip, port);
        }
    }
}

int connect_to_server(char *ip, int port) {
    tcp_socket = CreateTCPDataSocket();
    int result = Connect(tcp_socket, ip, port);

    if (result == -1) {
        perror("When connecting to server");
    }
    return result;
}

void receive_and_show_message(int udp_socket) {
    char buff[MAX_MSG_SIZE];
    int received;

    received = recvfrom(udp_socket, buff, MAX_MSG_SIZE, 0, NULL, NULL);
    if (received == -1)
        return; // We silently ignore errors
    buff[received] = 0;
    printf("\n\t\t%s\n", buff);
    printf("\n%s> ", nick);
}

void read_and_process_keyboard(int udp_socket, char *ip, int port) {
    char line[MAX_LINE_SIZE];
    char cmd[MAX_LINE_SIZE];
    static char destination_ip[25] = "Unassigned";
    static int destination_port = 0;
    static struct sockaddr_in target_dir;
    char *message_to_send;
    int i;
    int result;
    char res;
	char* closed;
	
    // Read the line
    //ADDED
    closed = fgets(line, MAX_LINE_SIZE - MAX_NICK_SIZE, stdin);
    
    if (closed == NULL) {
            printf("Unregistering...\n");

            result = connect_to_server(ip, port);

            if (result == -1) {
                perror("Could not connect to server");
                return;
            }

            SendByte(tcp_socket, LEAVE_CMD);
            SendShort(tcp_socket, strlen(nick));
            SendString(tcp_socket, nick);
			Close(tcp_socket);
            exit(0);
        }
        
    // Remove the carriage return or spaces at the end
    for (i = strlen(line) - 1; i >= 0; i--) {
        if (isspace(line[i]))
            line[i] = 0;
        else
            break;
    }
    // Get a new prompt
    printf("%s>", nick);

    // See if it's a command
    if (line[0] == '/') {
        // If begins with /, it is a command
        // The easiest thing is to read its contents with sscanf
        sscanf(line, "%s", cmd);

        // TODO
        // Depending on the value found in cmd, we perform the appropriate
        // action

        if (strcmp(cmd, "/CHAT") == 0) {

            sscanf(line, "%s %s", cmd, target);

            result = connect_to_server(ip, port);

            if (result == -1) {
                perror("When connecting to server");
                return;
            }

            SendByte(tcp_socket, QUERY_CMD);
            SendShort(tcp_socket, strlen(target));
            SendString(tcp_socket, target);

            res = ReadByte(tcp_socket);

            if (res == (char)QUERY_ERR) {
                printf("Could not find user '%s'\n", target);
                return;
            }

            ReadEndpoint(tcp_socket, &target_dir);

            strcpy(destination_ip, inet_ntoa(target_dir.sin_addr));
            destination_port = ntohs(target_dir.sin_port);

        } else if (strcmp(cmd, "/QUIT") == 0) {
            printf("Unregistering...\n");

            result = connect_to_server(ip, port);

            if (result == -1) {
                perror("Could not connect to server");
                return;
            }

            SendByte(tcp_socket, LEAVE_CMD);
            SendShort(tcp_socket, strlen(nick));
            SendString(tcp_socket, nick);

            res = ReadByte(tcp_socket);

            if (res == (char)LEAVE_ERR) {
                perror("You are not registered to this server. Quitting...");
            }

            Close(udp_socket);
            exit(0);
        } else {
            printf("Incorrect command. Try /CHAT or /QUIT\n");
            return;
        }

    }

    else // If the line does not start with /, it is a message to send to the
         // peer
    {
        // You have to send it to the ip and ports previously assigned,
        // but first we check if they were actually assigned
        // before these values
        if (strcmp(destination_ip, "Unassigned") == 0) {
            printf("ERROR: before sending message you must use the command\n");
            return;
        }
        // If everything goes well, we send the message,
        // preceded by the user's nick

        // TODO
        // Create a buffer with the message to send and send it

        message_to_send = (char *)malloc(MAX_MSG_SIZE);
        sprintf(message_to_send, "%s> %s", nick, line);
        sendto(udp_socket, message_to_send, strlen(message_to_send), 0,
               (struct sockaddr *)&target_dir, sizeof(target_dir));
        free(message_to_send);
    }

    Close(tcp_socket);
}

int get_max(int a, int b) {
    if (a > b) {
        return a;
    } else {
        return b;
    }
}
