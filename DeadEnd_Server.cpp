/*
    Server program for the game "Dead End"

    Moises Uriel Torres A01021323
    Daniel Atilano A01020270
*/

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>
#include <unistd.h>
#include <time.h>
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

using namespace std;

///// Structure definitions

// Data that will be sent to each thread
typedef struct data_struct {
    int connection_fd;

} thread_data_t;

// Global variables for signal handlers
int interrupt_exit = 0;


///// FUNCTION DECLARATIONS
void setupHandlers();
void onInterrupt(int signal);
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

// Handler for SIGINT
void onInterrupt(int signal)
{
    interrupt_exit = 1;
}

// Define signal handlers
void setupHandlers()
{
    //Create list for blocked signals
    sigset_t new_set;
    //The set is filled with all possible signals
    sigfillset(&new_set);
    //All save signal 2; i.e. Ctrl-C
    sigdelset(&new_set,2);
    //The new set blocks all other signals
    sigprocmask(SIG_SETMASK, &new_set, NULL);

    struct sigaction new_action;
    new_action.sa_handler = onInterrupt;
    sigaction(SIGINT, &new_action, NULL);
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

            //The data for the thread is saved into the structure
            connection_data->connection_fd = client_fd;

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
    char buffer[BUFFER_SIZE];
    int status;
    //Counter and limit for the generation of random events(sounds)
    int counter, limit;

    //The random is seeded
    srand(time(NULL));

    //The thread data structure is extracted from the parameters
    thread_data_t * connection_data = (thread_data_t *) arg;

    //The connection_fd is saved
    int connection_fd = connection_data->connection_fd;

    //The information of the client is saved
    struct sockaddr_in client_address;
    socklen_t client_address_size;
    char client_presentation[INET_ADDRSTRLEN];    

    inet_ntop(client_address.sin_family, &client_address.sin_addr, client_presentation, sizeof client_presentation);

    //The server waits for the message that the client is about to begin the game
    recvString(connection_fd, buffer, BUFFER_SIZE);
    printf("The client %s is about to begin the game\n",client_presentation);

    bzero(buffer, BUFFER_SIZE);

    //A random limit between 750 and 1000 is generated
    limit = rand() %15000 + 10000;

    //Loop that continues while the game is opened 
    while(!interrupt_exit){

        //The counter increases
        counter+=1;

        //If the counter reaches the limit
        if(counter>=limit)
        {
            //A message is sent to the client indicating to play a sound
            sprintf(buffer, "1");
            sendString(connection_fd, buffer, BUFFER_SIZE);

            bzero(buffer, BUFFER_SIZE);

            //The server receives the message indicating if the user lost
            recvString(connection_fd, buffer, BUFFER_SIZE);

            //If the status is -1, it means the user lost
            if(strncmp(buffer,"-1", BUFFER_SIZE)==0)
            {
                printf("The client lost\n");

                //The cycle finishes
                break;
            }

            bzero(buffer, BUFFER_SIZE);

            //The counter is resetted and a new limit is created
            counter = 0;
            limit = rand() %15000 + 10000;
        }

    }

    //The thread exits
    pthread_exit(NULL);
}

