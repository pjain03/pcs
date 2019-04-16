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
int add_client(int proxy, Connection **connection_list);
int read_sockfd(int sockfd, char *buffer, Connection *connection);
int setup_server(int port_num);
int accept_client(int proxy);
int handle_client(int client, int proxy, char *buffer, Connection **connection_list);
int add_connection(int sockfd, Connection **connection_list);
int search_connection(int sockfd, Connection **connection_list);
int handle_new_connection(int proxy);
void handle_get(int client, int server, HTTPRequest *request,
                char *raw_request, int request_length);
void handle_connect(int client, int server, HTTPRequest *request);
void handle_activity(fd_set *master, fd_set *readfds, int *max_fd, int proxy,
                     char *buffer, Connection **connection_list);
void clear_connection(Connection *connection);
void remove_connection(int sockfd, Connection **connection_list);


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
    Connection **connection_list;

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

    // setup multiple clients
    connection_list = (Connection **) malloc(sizeof(Connection *) * MAX_CONNECTIONS);
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        connection_list[i] = (Connection *) malloc(sizeof(Connection));
        connection_list[i]->raw = NULL;
        connection_list[i]->sockfd = -1;
        connection_list[i]->read_len = 0;
        connection_list[i]->request = NULL;
        connection_list[i]->response = NULL;
    }

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
            handle_activity(&master, &readfds, &max_fd, proxy, buffer, connection_list);
        }
        for (int i = 0; i < MAX_CONNECTIONS; i++) {
            if (connection_list[i]->sockfd != -1) {
                printf("CONNECTION: %d\n", connection_list[i]->sockfd);
                printf("RAW: %s\n", connection_list[i]->raw);
                printf("READLEN: %d\n", connection_list[i]->read_len);
                printf("\n");
            }
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

    // TODO: handle activity on an existing connection.
    //    so far, we are not handling persistent connections. we simply
    //    accept a client, handle their request, and close that
    //    connection. when we do make connections persistent, we will
    //    need to handle the case where 'i != proxy'. we will also need
    //    to update our 'master' and 'max_fd' variables then (in the
    //    'i == proxy' part). we might also have to keep track of every
    //    persistent connection we have
    //
    //          Replace the following:
    //    - handle_new_connection
    //    - read_all
    //    - read_hdr
    //
    //          Correct the following:
    //    - handle_get
    //    - handle_connect
    //
    //          Edit the following structures:
    //    - Request/Response should be able to handle partial messages

    int n;

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
                    // Watch for reads from this client going forward.
                    FD_SET(n, master);
                    if (*max_fd < n) {
                        *max_fd = n;
                    }
                }
            } else {
                if ((n = handle_client(i, proxy, buffer, connection_list)) <= 0) {
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


int add_client(int proxy, Connection **connection_list) {
    /* Adds client to connection_list */

    int n;

    if ((n = accept_client(proxy)) < 0) {
        error_declare("Cannot accept client!");
    } else {
        n = add_connection(n, connection_list);
    }

    return n;
}


int add_connection(int sockfd, Connection **connection_list) {
    /* Adds connection to our connection list
     * NOTE: -1 means we couldn't add to our connection list */

    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        if (connection_list[i]->sockfd == -1) {
            connection_list[i]->sockfd = sockfd;
            return sockfd;
        }
    }

    return -1;
}


int search_connection(int sockfd, Connection **connection_list) {
    /* Searches for client and returns its ID from connection_list
     * NOTE: returns -1 if we couldn't find the connection in our list */

    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        if (connection_list[i]->sockfd == sockfd) {
            return i;
        }
    }
    return -1;
}


void remove_connection(int sockfd, Connection **connection_list) {
    /* Removes client from the connection_list */

    int connection_id = search_connection(sockfd, connection_list);
    if (connection_id != -1) {
        clear_connection(connection_list[connection_id]);
    }
}


void clear_connection(Connection *connection) {
    /* Clears the information from the connection passed in */

    if (connection->raw != NULL) {
        free(connection->raw);
    }
    connection->raw = NULL;
    connection->sockfd = -1;
    connection->read_len = 0;
    connection->request = NULL;
    connection->response = NULL;
}


int handle_client(int sockfd, int proxy, char *buffer, Connection **connection_list) {
    /* Handles client */

    int last_read = -1;
    Connection *connection = connection_list[search_connection(sockfd, connection_list)];

    // read from the sockfd
    last_read = read_sockfd(sockfd, buffer, connection);

    // parse read from the sockfd
    if (last_read == 0) // response {

    } else if ()

    return last_read;
}


int read_sockfd(int sockfd, char *buffer, Connection *connection) {
    /* Reads from the socket and stores it in the partial buffer of the connection */

    int last_read = 0;
    bzero(buffer, BUFFER_SIZE);

    if ((last_read = read(sockfd, buffer, BUFFER_SIZE)) == -1) {
        error_declare("Couldn't read from client socket!");
        return -1;
    }

    if (last_read > 0) {
        if ((connection->raw = (char *) realloc(connection->raw, connection->read_len + last_read))
                == NULL) {
            error_declare("Couldn't realloc!");
            clear_connection(connection);
            return -1;
        }
        memcpy(connection->raw + connection->read_len, buffer, last_read);
        connection->read_len += last_read;
    }

    return last_read;
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
                write_to_socket(client, raw_response, response_length);  //TODO: see if we can refactor
            } else {

                // Get the response from the server, cache it, and send it back to the client
                server = connect_to_server(request->host, request->port);
                if (request->method == GET) {
                    printf("GET!\n");
                    handle_get(client, server, request, raw_request,
                               request_length);
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


void handle_get(int client, int server, HTTPRequest *request,
                char *raw_request, int request_length) {
    /* Handles the GET pipeline */

    int response_length = 0, write_length = 0;
    char *raw_response = NULL;
    HTTPResponse *response = response;
    write_length = write_to_socket(server, raw_request, request_length);

    if ((response_length = read_all(server, &raw_response)) != -1) {
        response = parse_response(response_length, raw_response);
        display_response(response);
        add_data_to_cache(request->url, response);
        write_length = write_to_socket(client, raw_response,
                                        response_length);
    }
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


