/******************************************************************************
 *                                                                            *
 *                      AUTHORS: Annie Chen, Pulkit Jain                      *
 *                      PURPOSE: Header for common structures and functions   *
 *                               used by both Client and Server               *
 *                                                                            *
 *****************************************************************************/


//
// Includes and Definitions
//
#include <time.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>


//
// Data Structures
//


//
// Forward Declarations
//
void error_out(const char *msg);
void error_declare(const char *msg);


