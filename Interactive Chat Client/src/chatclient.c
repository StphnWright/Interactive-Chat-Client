#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "util.h"

int client_socket = -1;
char username[MAX_NAME_LEN + 1];
char inbuf[BUFLEN + 1];
char outbuf[MAX_MSG_LEN + 1];

/* handle_stdin processes user input from the terminal, validates the input,
and sends the message to the server. It returns 0 on success, -1 on error, 
and -2 if the user input is NULL.*/

// Handle user input from stdin
int handle_stdin() {
    // Read user input into the outbuf
    memset(outbuf, 0, sizeof(outbuf));
    outbuf[sizeof(outbuf) - 1] = 'x';
        
    errno = 0;
    // Read input from stdin into outbuf up to MAX_MSG_LEN characters
    char* result = fgets(outbuf, MAX_MSG_LEN, stdin);
    if (errno != 0) {
        perror("fgets failed to read user input");
        return -1;
    }
       
    // Indicate EOF or error if fgets is NULL
    if (result == NULL) {
        return -2;
    }

    // Empty string or newline character
    if (strcmp(outbuf, "") == 0 || strcmp(outbuf, "\n") == 0) {
        return 0;
    }

    // Find the location of the first newline
    char *newline_index = strchr(outbuf, '\n');
    if (newline_index == NULL) {
        // If no newline, indicate that the message is too long
        fprintf(stderr, "Sorry, limit your message to 1 line of at most %d characters.\n", MAX_MSG_LEN);     
    
        // Consume extra characters inside the buffer
        int c;
        while ((c = fgetc(stdin)) != '\n' && c != EOF) {}
        
        // Return an empty string
        outbuf[0] = '\0';
        return 0;
    }
    
    // Terminate the string where the newline is
    newline_index[0] = '\0';
    
    // Send the message to the server
    if (send(client_socket, outbuf, strlen(outbuf) + 1, 0) == -1) {
        perror("send failed");
        return -1;
    }
    
    // Goodbye and terminate if bye
    if (strcmp(outbuf, "bye") == 0) {
        printf("Goodbye.\n");
        close(client_socket);
        exit(EXIT_SUCCESS);
    }
    
    // Finished
    return 0;
}

/* handle_client_socket receives messages from the server, processes the messages, 
and outputs them to the terminal. It returns 0 on success and -1 on error or if 
the connection to the server is closed.*/

// Handle messages received from the server
int handle_client_socket() {
    // Receive message from server into inbuf
    int nbytes = recv(client_socket, inbuf, BUFLEN, 0);

    if (nbytes == -1 && errno != EINTR) {
        printf("Warning: Failed to receive incoming message.\n");
        return -1;
    }
    
    // If no bytes are received, return -1 (error or closed connection)
    if (nbytes == 0) {
        fprintf(stderr, "\nConnection to server has been lost.\n");
        close(client_socket);
        exit(EXIT_FAILURE);
    }
    
    // Null-terminate the received message
    inbuf[nbytes] = '\0';

    if (strcmp(inbuf, "bye") == 0) {
        printf("\nServer initiated shutdown.\n");
        close(client_socket);
        exit(EXIT_SUCCESS);     
    }
    
    // Print the received message to the console
    printf("\n%s\n", inbuf);
    return 0;
}

int main(int argc, char **argv) {
    //Check for correct usage
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <server IP> <port>\n", argv[0]);
        return EXIT_FAILURE;
    }
    
    // Validate and convert the address string to binary form
    unsigned char ip_dst[sizeof(struct in6_addr)]; 
    if (! inet_pton(AF_INET, argv[1], ip_dst)) {
        fprintf(stderr, "Error: Invalid IP address '%s'.\n", argv[1]);
        return EXIT_FAILURE;
    }

    // Convert server port string to int
    int server_port;
    if (! parse_int(argv[2], &server_port, "server port number")) {
        return EXIT_FAILURE;
    }

    // Check if server port is within valid range 1024-65535, we are 17365
    if (server_port < 1024 || server_port > 65535) {
        fprintf(stderr, "Error: Port must be in range [1024, 65535].\n");
        return EXIT_FAILURE;
    }
    
    int valid_username = 0;
    // Ensure the user enters a valid username
    while (! valid_username) {
        // Prompt for username if stdin is a terminal
        if (isatty(STDIN_FILENO)) {
            printf("Enter Username: ");
        }
        memset(username, 0, sizeof(username));
        username[sizeof(username) - 1] = 'x';

        // Read username from stdin        
        if (fgets(username, sizeof(username), stdin) == NULL) {
            fprintf(stderr, "Error: fgets failed to read username.\n");
            return EXIT_FAILURE;
        }
        
        int found_extra = 0; 

        // Read and validate username length
        if (username[sizeof(username) - 1] == '\0' && username[sizeof(username) - 2] != '\n') {
            int c;
            while ((c = fgetc(stdin)) != '\n' && c != EOF) {
                found_extra = 1;
            }

            // Discard extra characters if username is too long
            if (found_extra) {
                fprintf(stderr, "Sorry, limit your username to %d characters.\n", MAX_NAME_LEN);
                continue;
            }
        }

        // Mark username as valid if it has a non-zero length
        if (strlen(username) > 1) {
            valid_username = 1;
            username[strlen(username) - 1] = '\0';
        }
    }

    printf("Hello, %s. Let's try to connect to the server.", username);   
 
    // Set up server address struct
    struct sockaddr_in server_addr;
    
    // Create client socket
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        perror("socket failed");
        return EXIT_FAILURE;
    }

    // Set server address properties
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    server_addr.sin_addr.s_addr = inet_addr(argv[1]);

    // Connect to the server
    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        fprintf(stderr, "Error: Failed to connect to server. %s.\n", strerror(errno));
        close(client_socket);
        return EXIT_FAILURE;
    }

    // Set client socket and stdin to non-blocking
    if (fcntl(client_socket, F_SETFL, O_NONBLOCK) == -1) {
        perror("fcntl: client socket failed");
        close(client_socket);
        return EXIT_FAILURE;
    }

    // Send the username to the server and handle any errors
    if (send(client_socket, username, strlen(username) + 1, 0) == -1) {
        perror("send username failed");
        close(client_socket);
        return EXIT_FAILURE;
    }

    // Set up file descriptor set for select()
    fd_set read_fds;
    int fdmax = client_socket;
    int running = 3;
    printf("\n");

    // Main event loop
    while (running) {
        // Clear and set read_fds for stdin and client_socket
        FD_ZERO(&read_fds);
        FD_SET(STDIN_FILENO, &read_fds);
        FD_SET(client_socket, &read_fds);

        // Use select() to wait for input from either stdin or client_socket
        if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select");
            running = 0;
        }

        // If client_socket has input, handle it
        if (FD_ISSET(client_socket, &read_fds)) {
            if (running == 3) {
                running = 2;
            }
            if (handle_client_socket() == -1) {
                perror("handle_client_socket");
                running = 0;
            }
        }
		
        // Only handle other stuff once the server has given a first response
   		if (running < 3) {
   			// Print newline on first prompt
   			if (running == 2) {
   				printf("\n");
   				running = 1;
   			}
   			
			// If stdin has input, handle it
			if (FD_ISSET(STDIN_FILENO, &read_fds)) {
				int status = handle_stdin();
				if (status == -1) {
					fprintf(stderr, "handle_stdin failed\n");
					running = 0;
				} else if (status == -2) {
					running = 0;
					printf("\n");
					break;
				}
			}
			
			// Display the username prompt and flush stdout if stdin is a terminal
			if (isatty(STDIN_FILENO)) {
				printf("[%s]: ", username);
			}
			fflush(stdout);
		}
    }
    
    // Close client socket and exit the program
    close(client_socket);
    return EXIT_SUCCESS;
}
