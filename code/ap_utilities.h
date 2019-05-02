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

#ifndef AP_H
#define AP_H


#include <time.h>
#include <netdb.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <curl/curl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "uthash/src/uthash.h"

#define DEFAULT_HTTP_PORT 80
#define MAX_CONNECTIONS 10
#define CONTENT_LENGTH "Content-Length"
#define BUFFER_SIZE 2048
#define CONNECT_RQ "CONNECT"
#define OPTIONS_RQ "OPTIONS"
#define AMPERSAND "&"
#define GET_RQ "GET"
#define COLON ":"
#define EMPTY "\0"
#define CRLF2 "\r\n\r\n"
#define QUERY "query="
#define CRLF "\r\n"
#define CRCR "\r\r"
#define LFLF "\n\n"
#define HOST "Host"
#define AGE "Age"
#define OK " 200 Connection established"
#define CR "\r"
#define LF "\n"
#define NUM_KEYWORDS 10

//
// Data Structures
//
typedef enum HTTPMethod {
    /* HTTP Methods we support */
    GET,
    CONNECT,
    OPTIONS, 
    UNSUPPORTED
} HTTPMethod;

typedef struct HTTPHeader {
    /* HTTP Headers will be represented as a linked list, this is a node */
    char *name;
    char *value;
    struct HTTPHeader *next;
} HTTPHeader;

typedef struct HTTPRequest {
    /* HTTP Request will be the parsed form of the raw request */
    HTTPMethod method;
    char *url;
    char *version;
    int port;
    char *host;
    HTTPHeader *hdrs;
    int body_length; // For post requests
    char *body;
} HTTPRequest;

typedef struct HTTPResponse {
    /* HTTP Request will be the parsed form of the raw request */
    char *version;
    char *status;
    char *status_desc;
    HTTPHeader *hdrs;
    int body_length;
    int total_body_length;
    char *body;
    char *keywords[NUM_KEYWORDS]; 
    time_t time_fetched;
} HTTPResponse;

typedef struct Connection {
    /* We will map sockfd to this other data */
    int requesting_sockfd; // key
    int target_sockfd;
    char *raw;
    int read_len;
    // int got_header;
    HTTPRequest *request;
    HTTPResponse *response;
	UT_hash_handle hh;
} Connection;



//
// Forward Declarations
//
void error_out(const char *msg);
void error_declare(const char *msg);

int add_client(int proxy, Connection **connection_list);
int add_server(int proxy, int client, HTTPRequest *request,
               Connection **connection_list);
int add_client_connection(int requesting_sockfd, Connection **connection_list);
int add_server_connection(int requesting_sockfd, int target_sockfd,
                          HTTPRequest *request, Connection **connection_list);
void clear_connection(Connection *connection);
void remove_connection(int sockfd, fd_set *master, Connection **connection_list);
Connection *search_connection(int sockfd, Connection **connection_list);

int accept_client(int proxy);
int write_to_socket(int sockfd, char *buffer, int buffer_length);
int connect_to_server(char *hostname, int port_num);
int read_all(int sockfd, char **raw);
int read_hdr(int sockfd, char **raw);
int read_sockfd(int sockfd, char *buffer, Connection *connection);
int header_not_completed(char *raw, int raw_len);
void add_hdr(HTTPHeader **hdr, char *key, char *value);
char *get_hdr_value(HTTPHeader *hdrs, const char *name);
char *itoa_ap(int x);

void free_hdr(HTTPHeader *hdr);
void free_request(HTTPRequest *request);
void free_response(HTTPResponse *response);
void display_request(HTTPRequest *request);
void display_response(HTTPResponse *response);
HTTPHeader *parse_headers(int *offset, char **raw_ptr);
HTTPRequest *parse_request(int length, char *raw);
HTTPResponse *parse_response(int length, char *raw);
int construct_response(HTTPResponse *response, char **raw);


#endif /* AP_H */

