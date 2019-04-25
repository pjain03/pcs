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
char *Proxy_URL = NULL;


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
int handle_get_request(int sockfd, int proxy, int last_read, Connection *connection,
                       Connection **connection_list, int *max_fd, fd_set *master);
int handle_connect_request(int sockfd, int proxy, int last_read, Connection *connection,
                           Connection **connection_list, int *max_fd, fd_set *master);
int handle_get_response(int last_read, Connection *connection);
int handle_connect_response(int last_read, Connection *connection);
void handle_connect(int client, int server, HTTPRequest *request);
void handle_activity(fd_set *master, fd_set *readfds, int *max_fd, int proxy,
                     char *buffer, Connection **connection_list);


//
// Implementation
//
int main(int argc, char **argv) {
    /* Runs the full program */

    // we require a port to listen on
    if (argc < 3) {
        error_out("Incorrect number of arguments!\n"
                  "Usage: ./proxy <host name> <port number> <OPTIONAL: eviction policy>");
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
    if (argc == 3) {
        init_cache(NULL);
    } else {
        init_cache(argv[3]);
    }

    // setup server
    if ((Proxy_URL = (char *) malloc(strlen(argv[1]) + 1 + strlen(argv[2]) + 1))
            == NULL) {
        // in case of malloc failing, we set the Proxy_URL to EMPTY because
        // that will never match the host of a GET
        Proxy_URL = EMPTY;
    } else {
        bzero(Proxy_URL, strlen(argv[1] + 1 + strlen(argv[2]) + 1));
        Proxy_URL[strlen(argv[1]) + 1 + strlen(argv[2])] = '\0';
        memcpy(Proxy_URL, argv[1], strlen(argv[1]));
        memcpy(Proxy_URL + strlen(argv[1]), COLON, 1);
        memcpy(Proxy_URL + strlen(argv[1]) + 1, argv[2], strlen(argv[2]));
    }
    port_num = atoi(argv[2]);
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
    destroy_cache();
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

                if (connection->request->method == GET) {

                    // if we are the host then it is a query for the cache
                    if (strcmp(get_hdr_value(connection->request->hdrs, "Host"),
                            Proxy_URL) == 0) {
                        printf("Query: %s\n", connection->request->body);
                        last_read = -1;
                    } else {
                        last_read = handle_get_request(sockfd, proxy, last_read,
                                                    connection, connection_list,
                                                    max_fd, master);
                    }
                } else if (connection->request->method == CONNECT) {
                    last_read = handle_connect_request(sockfd, proxy, last_read,
                                                       connection, connection_list,
                                                       max_fd, master);
                } else {
                    error_declare("Unsupported request!");
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
                last_read = handle_get_response(last_read, connection);
            } else if (connection->request->method == CONNECT) {
                last_read = handle_connect_response(last_read, connection);
            } else {
                error_declare("Unsupported response!");
                last_read = -1;
            }
        }
    }

    return last_read;
}


int handle_get_request(int sockfd, int proxy, int last_read, Connection *connection,
                       Connection **connection_list, int *max_fd, fd_set *master) {
    /* Handles the GET request */

    if ((connection->response = get_data_from_cache(connection->request->url)) != NULL) {
                    
        // Data was found in the cache
        char *raw_data;
        int raw_data_len = 0;
        raw_data_len = construct_response(connection->response, &raw_data);
        write_to_socket(sockfd, raw_data, raw_data_len);
        last_read = 0;
    } else {

        // Data wasn't found in the cache
        int server = add_server(proxy, sockfd, connection->request, connection_list);
        if (server > 0) {
            add_select(server, max_fd, master);
            last_read = write_to_socket(server, connection->raw, connection->read_len);
        } else {
            // removes client in case of error
            error_declare("Couldn't add server??\n");
            last_read = -1;
        }
    }

    return last_read;
}


int handle_connect_request(int sockfd, int proxy, int last_read, Connection *connection,
                           Connection **connection_list, int *max_fd, fd_set *master) {
    /* Handle the CONNECT request */

    int server = add_server(proxy, sockfd, connection->request, connection_list);
    if (server > 0) {
        add_select(server, max_fd, master);

        // send 200 to client indicating we have successfully opened a tunnel to
        // the destination server
        char *resp = NULL;;
        int resp_len = strlen(connection->request->version) + strlen(OK) + strlen(CRLF2);
        if ((resp = (char *) malloc(resp_len)) == NULL) {
            error_out("Couldn't malloc!");
        }
        bzero(resp, resp_len);
        memcpy(resp, connection->request->version, strlen(connection->request->version));
        memcpy(resp + strlen(connection->request->version), OK, strlen(OK));
        memcpy(resp + strlen(connection->request->version) + strlen(OK), CRLF2, strlen(CRLF2));
        last_read = write_to_socket(sockfd, resp, resp_len);
        free(resp);
        resp = NULL;
    } else {
        // removes client in case of error
        error_declare("Couldn't add server??\n");
        last_read = -1;
    }

    return last_read;
}


int handle_get_response(int last_read, Connection *connection) {
    /* Handle the GET response */

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
        add_data_to_cache(connection->request->url, connection->response);
    }

    return last_read;
}


int handle_connect_response(int last_read, Connection *connection) {
    /* Handle the CONNECT response */

    last_read = write_to_socket(connection->target_sockfd, connection->raw,
                                connection->read_len);
    
    if (last_read != connection->read_len) {
        error_out("Last read did not match connection read len!");
    }

    // clear out buffer
    free(connection->raw);
    connection->raw = NULL;
    connection->read_len = 0;

    return last_read;
}


void add_select(int sockfd, int *max_fd, fd_set *master) {
    /* Adds socket to the select functionality */

    FD_SET(sockfd, master);
    if (*max_fd < sockfd) {
        *max_fd = sockfd;
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
                        if (last_read >= 0) {
                            write_to_socket(server, buffer, last_read);
                        }
                    }
                    // server said something
                    else if (i == server) {
                        last_read = read(server, buffer, BUFFER_SIZE);
                        printf("READ %d bytes from server!\n", last_read);
                        if (last_read >= 0) {
                            write_to_socket(client, buffer, last_read);
                        }
                    }
                }
                if (last_read <= 0) {
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


