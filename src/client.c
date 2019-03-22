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
    if (argc < 2) {
        error_out("Incorrect number of arguments!\n"
                  "Usage: a.out <port number>");
    }

    // cleanup and exit
    exit(EXIT_SUCCESS);
}


