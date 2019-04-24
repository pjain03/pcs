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
                } else {
                    printf("Client wasn't accepted??\n");
                }
            } else {
                if ((n = handle_client(i, proxy, buffer, connection_list,
                                       max_fd, master)) <= 0) {

                    // We either errored out or finished our conversation.
                    // This is cleanup.
                    remove_connection(i, master, connection_list);
                }
            }
        }
    }
    printf("--\n");
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
        if (connection->target_sockfd < 0) {

            // communication hasn't started between client and server
            if (!header_not_completed(connection->raw, connection->read_len)) {

                connection->request = parse_request(connection->read_len, connection->raw);
                display_request(connection->request);

                

                int server = add_server(proxy, sockfd, connection->request, connection_list);

                if (server > 0) {
                    // TODO: handle CONNECT
                    add_select(server, max_fd, master);
                    if (connection->request->method == GET) {
                        last_read = write_to_socket(server, connection->raw, connection->read_len);
                    } else {
                        error_declare("Unsupported request!");
                        last_read = -1;
                    }
                } else {
                    // removes client in case of error
                    printf("Couldn't add server??\n");
                    last_read = -1;
                }

                // clear out buffer
                free(connection->raw);
                connection->raw = NULL;
                connection->read_len = 0;
            }
        } else {

            // communication is underway (target_fd being set means that a
            // valid request was received, and communication can now start)
            if (connection->request->method == GET) {

                int offset = connection->read_len - last_read;
                write_to_socket(connection->target_sockfd,
                                connection->raw + offset, last_read);
                if (!connection->response) {
                    if (!header_not_completed(connection->raw, connection->read_len)) {
                        connection->response = parse_response(connection->read_len,
                                                              connection->raw);
                    }
                } else {
                    memcpy(connection->response->body + connection->response->body_length,
                           connection->raw + offset, last_read);
                    connection->response->body_length += last_read;
                }

                if (connection->response->body_length == connection->response->total_body_length) {
                    display_response(connection->response);
                }
            } else {
                error_declare("Unsupported response!");
                last_read = -1;
            }
        }
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


