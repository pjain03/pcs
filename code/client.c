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
    int port_num, client;
    char buffer[BUFFER_SIZE], *hostname;

    // setup client
    hostname = argv[1];
    port_num = atoi(argv[2]);
    client = connect_to_server(hostname, port_num);

    // cleanup and exit
    exit(EXIT_SUCCESS);
}


