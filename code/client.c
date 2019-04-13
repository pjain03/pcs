/******************************************************************************
 *                                                                            *
 *                      AUTHORS: Annie Chen, Pulkit Jain                      *
 *                      PURPOSE: Client module to test our Server             *
 *                                                                            *
 *****************************************************************************/


//
// Includes and Definitions
//
#include "ap_utilities.h"


//
// Data Structures
//
char *con = "CONNECT www.google.com:443 HTTP/1.1\r\nHost: www.google.com:443\r\nProxy-Connection: Keep-Alive\r\n\r\n";
char *get = "GET www.google.com:443 HTTP/1.1\r\nHost: www.google.com:443\r\n\r\n";


//
// Forward Declarations
//


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
    int port_num, server;
    char buffer[BUFFER_SIZE], *hostname;
    bzero(buffer, BUFFER_SIZE);

    // setup client
    hostname = argv[1];
    port_num = atoi(argv[2]);
    server = connect_to_server(hostname, port_num);

    // handle CONNECT
    write_to_socket(server, con, strlen(con));
    read(server, buffer, BUFFER_SIZE);
    printf("%s", buffer);
    bzero(buffer, BUFFER_SIZE);

    // start communication
    write_to_socket(server, get, strlen(get));
    read(server, buffer, BUFFER_SIZE);
    printf("READ %s\n", buffer);
    bzero(buffer, BUFFER_SIZE);
    while(1){}

    // cleanup and exit
    exit(EXIT_SUCCESS);
}


