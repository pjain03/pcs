/******************************************************************************
 *                                                                            *
 *                      AUTHORS: Annie Chen, Pulkit Jain                      *
 *                      PURPOSE: Client module to test our Server             *
 *                                                                            *
 *****************************************************************************/


//
// Includes and Definitions
//
#include <netdb.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "ap_utilities.c"

#define BUFFER_SIZE 1024


//
// Data Structures
//


//
// Forward Declarations
//
int connect_to_server(char *hostname, int port_num);


//
// Implementation
//
int main(int argc, char **argv) {
    /* Runs the full program */

    // we require a port to listen on
    if (argc < 3) {
        error_out("Incorrect number of arguments!\n"
                  "Usage: ./client <host name> <port number>");
    }

    // important variables
    int port_num, client;
    char buffer[BUFFER_SIZE], *hostname;

    // setup client
    hostname = argv[1];
    port_num = atoi(argv[2]);
    client = connect_to_server(hostname, port_num);

    // cleanup and exit
    exit(EXIT_SUCCESS);
}


int connect_to_server(char *hostname, int port_num) {
    /* Setup the client and return its information */

    int client;
    struct sockaddr_in serveraddr;
    struct hostent *server;

    // socket: create the socket
    if ((client = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        error_out("Couldn't open socket!");
    }

    // gethostbyname: get the server's DNS entry
    if ((server = gethostbyname(hostname)) == NULL) {
        error_out("Couldn't get host!");
    }

    // build the server's Internet address
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
	  (char *)&serveraddr.sin_addr.s_addr, server->h_length);
    serveraddr.sin_port = htons(port_num);

    // connect: create a connection with the server
    if (connect(client, (const struct sockaddr*) &serveraddr,
                sizeof(serveraddr)) < 0) {
        error_out("Couldn't connect to the server!");
    }
    
    return client;
}


