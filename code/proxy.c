/******************************************************************************
 *                                                                            *
 *                      AUTHORS: Annie Chen, Pulkit Jain                      *
 *                      PURPOSE: Server module (Our Proxy)                    *
 *                                                                            *
 *****************************************************************************/


//
// Includes and Definitions
//
#include "ap_utilities.c"

#define NUM_QUEUED_CONNECTIONS 5
#define TIMEOUT_INTERVAL 60


//
// Data Structures
//


//
// Forward Declarations
//
int setup_server(int port_num);
int accept_client(int proxy);
int handle_new_connection(int proxy);
void handle_activity(fd_set *master, fd_set *readfds, int *max_fd, int proxy,
                     char *buffer);


//
// Implementation
//
int main(int argc, char **argv) {
    /* Runs the full program */

    // we require a port to listen on
    if (argc < 2) {
        error_out("Incorrect number of arguments!\n"
                  "Usage: ./proxy <port number>");
    }

    // important variables
    // NOTE: the variable 'proxy' (defined below) refers to the proxy server we
    //       use to serve client requests. the variable 'server' defined later
    //       refers to the connections we make to the servers as requested by
    //       clients.
    int port_num, proxy, max_fd, n;
    char buffer[BUFFER_SIZE];
    fd_set master, readfds;
    struct timeval tv;

    // setup server
    port_num = atoi(argv[1]);
    proxy = setup_server(port_num);
    printf("Listening on port %d...\n", port_num);

    // setup select
    FD_ZERO(&master);
    FD_SET(proxy, &master);
    max_fd = proxy;
    tv.tv_sec = TIMEOUT_INTERVAL;
    tv.tv_usec = TIMEOUT_INTERVAL;

    // start waiting for clients to connect
    while (1) {
        FD_ZERO(&readfds);
        memcpy(&readfds, &master, sizeof(master));
        if ((n = select(max_fd + 1, &readfds, NULL, NULL, &tv)) == -1) {
            error_out("Select errored out!");
        } else if (n == 0) {
            error_declare("TODO: Handle Timeout!");
            break;
        } else {
            handle_activity(&master, &readfds, &max_fd, proxy, buffer);
        }
    }

    // cleanup and exit
    for (int i = 3; i < max_fd; i++) {
        FD_CLR(i, &master);
    }
    FD_ZERO(&readfds);
    FD_ZERO(&master);
    close(proxy);
    exit(EXIT_SUCCESS);
}


int setup_server(int port_num) {
    /* Setup the server and return its connection information */

    int sockfd;
    struct sockaddr_in address;

    // setup server info
    address.sin_family = AF_INET;
    address.sin_port = htons(port_num);
    address.sin_addr.s_addr = htonl(INADDR_ANY);

    // create a socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        error_out("Server socket could not be created!");
    }

    // bind socket to any port available and our specified ip address
    if (bind(sockfd, (struct sockaddr *) &address, sizeof(address)) < 0) {
        error_out("Server socket could not be bound!");
    }

    // allow NUM_QUEUED_CONNECTIONS requests to queue up as we listen
    if (listen(sockfd, NUM_QUEUED_CONNECTIONS) < 0) {
        error_out("Cannot listen on server socket!");
    }

    return sockfd;
}


void handle_activity(fd_set *master, fd_set *readfds, int *max_fd, int proxy,
                     char *buffer) {
    /* Handles client requests */

    // if there are multiple such proxies being used in the user's environment,
    // there are no guarantees as to which file descriptor a new connection
    // will receive. therefore, we check all possible file descriptors since a
    // new connection will have a file descriptor larger than our 'max_fd', but
    // we cannot say with certainty which one.
    for (int i = 0; i < FD_SETSIZE + 1; i++) {
        if (FD_ISSET(i, readfds)) {
            if (i == proxy) {
                int client = handle_new_connection(proxy);
                close(client);
            }
            // TODO: handle activity on an existing connection.
            //    so far, we are not handling persistent connections. we simply
            //    accept a client, handle their request, and close that
            //    connection. when we do make connections persistent, we will
            //    need to handle the case where 'i != proxy'. we will also need
            //    to update our 'master' and 'max_fd' variables then (in the
            //    'i == proxy' part). we might also have to keep track of every
            //    persistent connection we have
        }
    }
}


int handle_new_connection(int proxy) {
    /* Handle new clients */

    int client, server, readlen = 0;
    char *raw_request, *raw_response;
    HTTPRequest *request;
    HTTPResponse *response;

    // connect to the client and receive its request
    if ((client = accept_client(proxy)) >= 0) {
        if ((raw_request = read_hdr(client)) != NULL) {
            request = parse_request(raw_request);
            display_request(request);

            // get the response from the server and send it back to the client
            server = connect_to_server(request->host, request->port);
            if (write_to_socket(server, raw_request) >= 0) {
                if ((raw_response = read_all(server)) != NULL) {
                    response = parse_response(raw_response);
                    display_response(response);
                    write_to_socket(client, raw_response);
                }
            }

            // cleanup
            free(raw_request);
            free_request(request);
            free(raw_response);
            free_response(response);
        }
    }

    return client;
}


int accept_client(int proxy) {
    /* Accepts a new client and returns its sockfd */
    
    int sockfd;
    struct sockaddr_in address;
    socklen_t addr_len = sizeof(address);

    if ((sockfd = accept(proxy, (struct sockaddr*) &address, &addr_len)) < 0) {
        error_declare("Server cannot accept incoming connections!");
    }

    return sockfd;
}


