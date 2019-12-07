/*
    Server program for the game "Dead End"
    Student: Moises Uriel Torres A01021323  Daniel Atilano A01020270
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
// Signals library
#include <errno.h>
#include <signal.h>
// Sockets libraries
#include <netdb.h>
#include <sys/poll.h>
// Posix threads library
#include <pthread.h>

// Custom libraries
#include "sockets.h"
#include "fatal_error.h"

#define MAX_ACCOUNTS 5
#define BUFFER_SIZE 1024
#define MAX_QUEUE 5

///// Structure definitions

// Data that will be sent to each thread
typedef struct data_struct {
    int connection_fd;

} thread_data_t;

// Global variables for signal handlers
int interrupt_exit = 0;


///// FUNCTION DECLARATIONS
void usage(char * program);
void waitForConnections(int server_fd);
void * attentionThread(void * arg);

///// MAIN FUNCTION
int main(int argc, char * argv[])
{
    int server_fd;

    printf("\n=== DEAD END SERVER ===\n");

    // Check the correct arguments
    if (argc != 2)
    {
        usage(argv[0]);
    }

	// Show the IPs assigned to this computer
	printLocalIPs();
    // Start the server
    server_fd = initServer(argv[1], MAX_QUEUE);
	// Listen for connections from the clients
    waitForConnections(server_fd);

    // Close the socket
    close(server_fd);

    // Finish the main thread
    pthread_exit(NULL);

    return 0;
}

///// FUNCTION DEFINITIONS

/*
    Explanation to the user of the parameters required to run the program
*/
void usage(char * program)
{
    printf("Usage:\n");
    printf("\t%s {port_number}\n", program);
    exit(EXIT_FAILURE);
}

/*
    Main loop to wait for incomming connections
*/
void waitForConnections(int server_fd)
{
    struct sockaddr_in client_address;
    socklen_t client_address_size;
    char client_presentation[INET_ADDRSTRLEN];
    int client_fd;
    pthread_t new_tid;
    thread_data_t * connection_data = (thread_data_t *) malloc(sizeof(thread_data_t));
    int poll_response;
	int timeout = 500;		// Time in milliseconds (0.5 seconds)

    // Create a structure array to hold the file descriptors to poll
    struct pollfd test_fds[1];
    // Fill in the structure
    test_fds[0].fd = server_fd;
    test_fds[0].events = POLLIN;    // Check for incomming data

    // Get the size of the structure to store client information
    client_address_size = sizeof client_address;
    
    while (!interrupt_exit)
    {
        //Poll to check for interruptions
        poll_response = poll(test_fds, 1 , timeout);

        if(poll_response == 1){
            // ACCEPT
            // Wait for a client connection
            client_fd = accept(server_fd, (struct sockaddr *)&client_address, &client_address_size);
            if (client_fd == -1)
            {
                fatalError("ERROR: accept");
            }
            
            //Get the data from the client
            inet_ntop(client_address.sin_family, &client_address.sin_addr, client_presentation, sizeof client_presentation);
            printf("Received incomming connection from %s on port %d\n", client_presentation, client_address.sin_port);

            // CREATE A THREAD
            pthread_create(&new_tid, NULL, attentionThread, connection_data);
        }

        //If the server is aborted with CTRL-C
        else if(poll_response == -1){
            printf("\nServer exit\n");
        }
    }

    //Free the structure sent to the thread
    free(connection_data);
}

/*
    Hear the request from the client and send an answer
*/
void * attentionThread(void * arg)
{

    //The thread exits
    pthread_exit(NULL);
}