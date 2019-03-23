/******************************************************************************
 *                                                                            *
 *                      AUTHORS: Annie Chen, Pulkit Jain                      *
 *                      PURPOSE: Header for common structures and             *
 *                               functions used by both Client and            *
 *                               Server                                       *
 *                   REFERENCES: https://bit.ly/2UYu0xm                       *
 *                                                                            *
 *****************************************************************************/


//
// Includes and Definitions
//
#include <time.h>
#include <netdb.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define BUFFER_SIZE 2048


//
// Data Structures
//
typedef enum HTTPMethod {
    /* HTTP Methods we support */
    GET,
    // CONNECT,
    UNSUPPORTED
} HTTPMethod;

typedef struct HTTPHeader {
    /* HTTP Headers will be represented as a linked list, this is a node */
    char *name;
    char *value;
    struct HTTPHeader *next;
} HTTPHeader;

typedef struct HTTPHeaders_LL {
    /* HTTP Headers linked list will be managed using this structure */
    HTTPHeader *head;
} HTTPHeaders_LL;

typedef struct HTTPCommunication {
    /* HTTP Request will be the parsed form of the raw request,
     * HTTP Response will be the message we send to the client,
     * - Handling responses thus allows us to add/modify headers easily */
    HTTPMethod method;
    char *url;
    char *version;
    HTTPHeaders_LL *ll;
    char *body;
} HTTPCommunication;


//
// Forward Declarations
//
void error_out(const char *msg);
void error_declare(const char *msg);

int connect_to_server(char *hostname, int port_num);  // TODO
char *read_all(int sockfd);  // TODO

void free_hdr(HTTPHeader *hdr);  // TODO
void free_comm(HTTPCommunication *comm);  // TODO
void display_comm(HTTPCommunication *comm);  // TODO
HTTPCommunication *parse_request(char *raw);  // TODO


