/******************************************************************************
 *                                                                            *
 *                      AUTHORS: Annie Chen, Pulkit Jain                      *
 *                      PURPOSE: Server module (Our Proxy)                    *
 *                                                                            *
 *****************************************************************************/


//
// Includes and Definitions
//
#include "ap_utilities.h"
#include "cache.h"

#define NUM_QUEUED_CONNECTIONS 5
#define TIMEOUT_INTERVAL 60


//
// Data Structures
//


//
// Forward Declarations
//
int setup_server(int port_num);
int handle_client(int client, int proxy, char *buffer, Connection **connection_list,
                  int *max_fd, fd_set *master);
int handle_new_connection(int proxy);
int send_get_to_server(Connection *connection);
void add_select(int sockfd, int *max_fd, fd_set *master);
void setup_get_server(int server, Connection *client_connection,
                      Connection **connection_list);
void handle_connect(int client, int server, HTTPRequest *request);
void handle_activity(fd_set *master, fd_set *readfds, int *max_fd, int proxy,
                     char *buffer, Connection **connection_list);


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
    Connection *connection_list = NULL;  /* IMPORTANT: initialize this to NULL
                                          *            - UTHash requirement */

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
    printf("PROXY: %d\n", proxy);

    // start waiting for clients to connect
    while (1) {
        FD_ZERO(&readfds);
        memcpy(&readfds, &master, sizeof(master));
        if ((n = select(max_fd + 1, &readfds, NULL, NULL, NULL)) == -1) {
            error_out("Select errored out!");
        } else if (n == 0) {
            error_declare("TODO: Handle Timeout!");
        } else {
            handle_activity(&master, &readfds, &max_fd, proxy, buffer, &connection_list);
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
                     char *buffer, Connection **connection_list) {
    /* Handles client requests */

    int n = 0;

    // if there are multiple such proxies being used in the user's environment,
    // there are no guarantees as to which file descriptor a new connection
    // will receive. therefore, we check all possible file descriptors since a
    // new connection will have a file descriptor larger than our 'max_fd', but
    // we cannot say with certainty which one.
    for (int i = 0; i < *max_fd + 1; i++) {
        if (FD_ISSET(i, readfds)) {
            if (i == proxy) {
                if ((n = add_client(proxy, connection_list)) > 0) {

                    // Client knows if they weren't able to contact us so no
                    // one is left waiting. We do need to handle it if we
                    // cannot accept clients so we log it in accept_client.
                    add_select(n, max_fd, master);
                }
            } else {
                if ((n = handle_client(i, proxy, buffer, connection_list,
                                       max_fd, master)) <= 0) {
                    printf("Removing %d\n", i);

                    // We either errored out or finished our conversation.
                    // This is cleanup.
                    FD_CLR(i, master);
                    remove_connection(i, connection_list);
                    close(i);
                }
            }
        }
    }
}


int handle_client(int sockfd, int proxy, char *buffer, Connection **connection_list,
                  int *max_fd, fd_set *master) {
    /* Handles client */
    // TODO: Adapt this to handle POST at some point (requires more thought)
    //       for now we are assuming that all requests we handle will be
    //       terminated with two CRLFs (i.e. only GET and CONNECT)

    int last_read = -1;
    Connection *connection = search_connection(sockfd, connection_list);

    // read from the sockfd
    last_read = read_sockfd(sockfd, buffer, connection);
    printf("Reading %d bytes from %d\n", last_read, sockfd);

    // parse read from the sockfd
    if (last_read > 0) {

        // if we have method stored, handle appropriately
        if (connection->request) {
            if (!connection->got_header) {
                if (!header_not_completed(connection->raw, connection->read_len)) {
                    connection->got_header = 1;
                    connection->response = parse_response(connection->read_len, connection->raw);
                    // display_response(connection->response);
                    write_to_socket(connection->resp_sockfd, connection->raw, connection->read_len);
                    free(connection->raw);
                    connection->raw = NULL;
                    connection->read_len = 0;
                }
            } else if (connection->request->method == GET) {
                printf("GET response!\n");
                if (connection->response->body_length < connection->response->total_body_length) {
                    connection->response->body = (char *) realloc(connection->response->body, connection->response->body_length + last_read);
                    memcpy(connection->response->body + connection->response->body_length, connection->raw, last_read);
                    connection->response->body_length += last_read;
                    write_to_socket(connection->resp_sockfd, connection->raw, connection->read_len);
                    free(connection->raw);
                    connection->raw = NULL;
                    connection->read_len = 0;
                    printf("%d pending\n", connection->response->total_body_length - connection->response->body_length);
                }
                if (connection->response->body_length >= connection->response->total_body_length) {
                    printf("Closing %d\n", connection->req_sockfd);
                    last_read = 0;
                }
            } else if (connection->request->method == CONNECT) {
                printf("CONNECT response!\n");
            } else {
                printf("UNSUPPORTED response!\n");
            }
        } else if (!connection->got_header) {
            // otherwise if we haven't got header and we now have a header, store header
            // parse details out and act on them

            if (!header_not_completed(connection->raw, connection->read_len)) {
                connection->got_header = 1;
                connection->request = parse_request(connection->read_len, connection->raw);
                // display_request(connection->request);
                if (connection->request->method == GET) {
                    int server = send_get_to_server(connection);
                    add_connection(server, connection_list);
                    add_select(server, max_fd, master);
                    setup_get_server(server, connection, connection_list);
                }
            }
        }

        // otherwise continue
    }

    return last_read;
}


void add_select(int sockfd, int *max_fd, fd_set *master) {
    /* Adds socket to the select functionality */

    FD_SET(sockfd, master);
    if (*max_fd < sockfd) {
        *max_fd = sockfd;
    }
}


void setup_get_server(int server, Connection *client_connection,
                      Connection **connection_list) {
    /* Setup server so we can respond as to GET */

    Connection *connection = search_connection(server, connection_list);

    if (connection != NULL) {
        connection->resp_sockfd = client_connection->req_sockfd;
        connection->request = client_connection->request;
    }
}


int send_get_to_server(Connection *connection) {
    /* Handles the GET pipeline
     * Return -1 if failure */

    int server, write_len;

    // connect to server
    if ((server = connect_to_server(connection->request->host,
                               connection->request->port)) > 0) {
        // write request to server
        if ((write_len = write_to_socket(server, connection->raw, connection->read_len))
                 < 0) {
            return write_len;
        }
        connection->resp_sockfd = server;
        printf("Wrote %d bytes %d->%d\n", write_len, connection->resp_sockfd, connection->req_sockfd);
    }
    
    return server;

    // write response to client

    // int response_length = 0, write_length = 0;
    // char *raw_response = NULL;
    // HTTPResponse *response = response;
    // write_length = write_to_socket(server, raw_request, request_length);

    // if ((response_length = read_all(server, &raw_response)) != -1) {
    //     response = parse_response(response_length, raw_response);
    //     display_response(response);
    //     add_data_to_cache(request->url, response);
    //     write_length = write_to_socket(client, raw_response,
    //                                     response_length);
    // }
}


int handle_new_connection(int proxy) {
    /* Handle new clients */
    // TODO: change the entire structure, no more read_hdr, no more read_all
    //       handling concurrent clients will require us to do things a lot
    //       differently

    int client, server, request_length = 0, response_length = 0,
        write_length = 0;
    char *raw_request = NULL, *raw_response = NULL;
    HTTPRequest *request;
    HTTPResponse *response;

    // connect to the client and receive its request
    if ((client = accept_client(proxy)) >= 0) {
        if ((request_length = read_hdr(client, &raw_request)) != -1) {

            request = parse_request(request_length, raw_request);
            display_request(request);
            response = get_data_from_cache(request->url);

            if (response != NULL) {

                // Received response from the cache, write back to client
                response_length = construct_response(response, &raw_response);
                write_to_socket(client, raw_response, response_length);  // TODO: see if we can refactor
            } else {

                // Get the response from the server, cache it, and send it back to the client
                server = connect_to_server(request->host, request->port);
                if (request->method == GET) {
                    printf("GET!\n");
                    // handle_get(client, server, request, raw_request,
                    //            request_length);
                } else if (request->method == CONNECT) {
                    printf("CONNECT!\n");
                    handle_connect(client, server, request);
                }
            }

            // cleanup
            close(server);
            free(raw_request);
            free_request(request);
            free(raw_response);
            /* Don't clean up just yet - want to keep the malloc'ed data in cache.
               Free when evict from cache.
            free_response(response); */
        }
    }

    return client;
}


void handle_connect(int client, int server, HTTPRequest *request) {
    /* Handle the CONNECT pipeline */

    // setup
    int read_len = 0, last_read = 1, readlen = 0, req_len = 0, max_fd = 0, n = 0,
        th_len = strlen(request->version) + strlen(OK) + strlen(CRLF2);
    fd_set master, readfds;
    struct timeval tv;
    char *buffer, *raw = NULL, *th_resp = NULL;
    if ((buffer = (char *) malloc(BUFFER_SIZE)) == NULL) {
        error_out("Couldn't malloc!");
    }
    if ((th_resp = (char *) malloc(th_len)) == NULL) {
        error_out("Couldn't malloc!");
    }
    bzero(buffer, BUFFER_SIZE);
    bzero(th_resp, th_len);

    // send 200 to client and begin communication
    memcpy(th_resp, request->version, strlen(request->version));
    memcpy(th_resp + strlen(request->version), OK, strlen(OK));
    memcpy(th_resp + strlen(request->version) + strlen(OK), CRLF2, strlen(CRLF2));
    write_to_socket(client, th_resp, th_len);

    // Tunnel connection between client and server
    // TODO: multiple connections should fix this (SO BAD OMFG)
    //       this can then be treated as ANY other connection
    FD_ZERO(&master);
    FD_SET(client, &master);
    FD_SET(server, &master);
    max_fd = server;
    tv.tv_sec = TIMEOUT_INTERVAL;
    tv.tv_usec = TIMEOUT_INTERVAL;
    while (1) {
        FD_ZERO(&readfds);
        memcpy(&readfds, &master, sizeof(master));
        if ((n = select(max_fd + 1, &readfds, NULL, NULL, &tv)) == -1) {
            error_out("Select errored out!");
        } else if (n == 0) {
            error_declare("TODO: Handle Timeout!");
            break;
        } else {
            for (int i = client; i < max_fd + 1; i++) {
                if (FD_ISSET(i, &readfds)) {
                    printf("Data is available now! %d, %d, %d\n", i, client, server);
                    // client said something
                    if (i == client) {
                        last_read = read(client, buffer, BUFFER_SIZE);
                        printf("READ %d bytes from client!\n", last_read);
                        write_to_socket(server, buffer, last_read);
                    }
                    // server said something
                    else if (i == server) {
                        last_read = read(server, buffer, BUFFER_SIZE);
                        printf("READ %d bytes from server!\n", last_read);
                        write_to_socket(client, buffer, last_read);
                    }
                }
                if (last_read < 0) {
                    error_declare("Select CONNECT error!");
                    break;
                } else if (last_read == 0) {
                    break;
                }
                bzero(buffer, BUFFER_SIZE);
            }
        }
        if (last_read <= 0) {
            break;
        }
    }

    // cleanup
    free(buffer);
}


