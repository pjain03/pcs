/******************************************************************************
 *                                                                            *
 *                      AUTHORS: Annie Chen, Pulkit Jain                      *
 *                      PURPOSE: Header for common structures and             *
 *                               functions used by both Client and            *
 *                               Server                                       *
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

// Message Types
unsigned short HELLO = 1;
unsigned short HELLO_ACK = 2;
unsigned short LIST_REQUEST = 3;
unsigned short CLIENT_LIST = 4;
unsigned short CHAT = 5;
unsigned short EXIT = 6;
unsigned short ERR_CAP = 7;
unsigned short ERR_CD = 8;

//
// Data Structures
//


//
// Forward Declarations
//
void error_out(const char *msg);
void error_declare(const char *msg);


