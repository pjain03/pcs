/******************************************************************************
 *                                                                            *
 *                      AUTHORS: Annie Chen, Pulkit Jain                      *
 *                      PURPOSE: Server module (Our Proxy)                    *
 *                                                                            *
 *****************************************************************************/


//
// Includes and Definitions
//
#include "search_engine.h"

#define NUM_QUEUED_CONNECTIONS 5


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
int handle_get_request(int sockfd, int proxy, int last_read, Connection *connection,
                       Connection **connection_list, int *max_fd, fd_set *master);
int handle_connect_request(int sockfd, int proxy, int last_read, Connection *connection,
                           Connection **connection_list, int *max_fd, fd_set *master);
int handle_options_request(int sockfd, int proxy, int last_read, Connection *connection,
                           Connection **connection_list, int *max_fd, fd_set *master);
int handle_cache_request(int sockfd, int proxy, int last_read, Connection *connection,
                         Connection **connection_list, int *max_fd, fd_set *master);
int handle_cache_query(int sockfd, int proxy, int last_read, Connection *connection,
                       Connection **connection_list, int *max_fd, fd_set *master);
int handle_cache_get(int sockfd, int proxy, int last_read, Connection *connection,
                     Connection **connection_list, int *max_fd, fd_set *master);
int handle_get_response(int last_read, Connection *connection);
int handle_connect_response(int last_read, Connection *connection);
int serialize_results(URLResults *results, char **raw_ptr);
void add_select(int sockfd, int *max_fd, fd_set *master);
void setup_get_server(int server, Connection *client_connection,
                      Connection **connection_list);
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

    // start waiting for clients to connect
    while (1) {
        FD_ZERO(&readfds);
        memcpy(&readfds, &master, sizeof(master));
        if ((n = select(max_fd + 1, &readfds, NULL, NULL, NULL)) == -1) {
            error_out("Select errored out!");
        } else if (n == 0) {
            error_declare("TODO: Handle Timeout!");
            tv.tv_sec = TIMEOUT_INTERVAL;
            tv.tv_usec = TIMEOUT_INTERVAL;
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
                    error_declare("Client wasn't accepted??\n");
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

    // parse read from the sockfd
    if (last_read > 0) {
        if (connection->target_sockfd < 0) {

            // communication hasn't started between client and server
            if (!header_not_completed(connection->raw, connection->read_len)) {

                connection->request = parse_request(connection->read_len, connection->raw);
                // display_request(connection->request);

                if (connection->request->method == GET) {

                    // if we are the host then it is a query for the cache
                    if (strcmp(get_hdr_value(connection->request->hdrs, "Host"),
                            Proxy_URL) == 0) {
                        last_read = handle_cache_request(sockfd, proxy, last_read,
                                                         connection, connection_list,
                                                         max_fd, master);
                    } else {
                        last_read = handle_get_request(sockfd, proxy, last_read,
                                                       connection, connection_list,
                                                       max_fd, master);
                    }
                } else if (connection->request->method == CONNECT) {
                    last_read = handle_connect_request(sockfd, proxy, last_read,
                                                       connection, connection_list,
                                                       max_fd, master);
                } else if (connection->request->method == OPTIONS) {
                    last_read = handle_options_request(sockfd, proxy, last_read,
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

int handle_options_request(int sockfd, int proxy, int last_read, Connection *connection,
                           Connection **connection_list, int *max_fd, fd_set *master) {
    /* Handle the OPTIONS request */

    int server = add_server(proxy, sockfd, connection->request, connection_list);
    if (server > 0) {
        add_select(server, max_fd, master);

        // respond to the preflight request with an Access-Control-Allow-Methods response header
        char *response = NULL;
        int response_length = 0;
        if ((connection->response = (HTTPResponse *) malloc(sizeof(HTTPResponse)))
                == NULL) {
            error_out("Couldn't malloc!");
        }
        if ((connection->response->version =
                (char *) malloc(strlen(connection->request->version))) == NULL) {
            error_out("Couldn't malloc!");
        }

        // setup response
        memcpy(connection->response->version, connection->request->version,
               strlen(connection->request->version));
        connection->response->status_desc = "No Content";
        connection->response->status = "204";
        connection->response->hdrs = NULL;

        add_hdr(&(connection->response->hdrs), "Connection", "Keep-Alive");
        add_hdr(&(connection->response->hdrs), "Access-Control-Allow-Origin", "*");
        add_hdr(&(connection->response->hdrs), "Access-Control-Allow-Methods", "GET, CONNECT, OPTIONS");
        add_hdr(&(connection->response->hdrs), "Access-Control-Allow-Headers", "*");
        add_hdr(&(connection->response->hdrs), "Access-Control-Max-Age", "86400");

        connection->response->body_length = 0;
        connection->response->time_fetched = time(NULL);
        response_length = construct_response(connection->response, &response);
        last_read = write_to_socket(sockfd, response, response_length);
        // display_response(connection->response);
    }

    free_request(connection->request);

    return last_read;
}


int handle_cache_request(int sockfd, int proxy, int last_read, Connection *connection,
                         Connection **connection_list, int *max_fd, fd_set *master) {
    /* Handles the different types of cache requests */

    int is_query = strstr(connection->request->url, QUERY) != NULL;
    int is_get = strstr(connection->request->url, GET_CACHE) != NULL;
    int processed = 0;

    if (is_query) {
        // cache_query
        last_read = handle_cache_query(sockfd, proxy, last_read, connection,
                                       connection_list, max_fd, master);
    }
    if (is_get) {
        // cache get
        last_read = handle_cache_get(sockfd, proxy, last_read, connection,
                                     connection_list, max_fd, master);
    }
    if (!(is_query || is_get)) {
        // unsupported argument - drop requester
        return 0;
    }

    return last_read;
}


int handle_cache_query(int sockfd, int proxy, int last_read, Connection *connection,
                       Connection **connection_list, int *max_fd, fd_set *master) {
    /* Handle query to the cache */

    CURL *curl = curl_easy_init();
    char *response = NULL, *query = NULL, *tmp_query_start = NULL,
         *tmp_query_end = NULL;
    int response_length = 0, query_length = 0, tmp_query_length = 0;
    if ((connection->response = (HTTPResponse *) malloc(sizeof(HTTPResponse)))
            == NULL) {
        error_out("Couldn't malloc!");
    }
    if ((connection->response->version =
            (char *) malloc(strlen(connection->request->version) + 1)) == NULL) {
        error_out("Couldn't malloc!");
    }

    // setup response
    memcpy(connection->response->version, connection->request->version,
           strlen(connection->request->version));
    connection->response->version[strlen(connection->request->version)] = '\0';
    connection->response->status_desc = "OK";
    connection->response->status = "200";
    connection->response->time_fetched = time(NULL);
    connection->response->hdrs = NULL;

    // extract query
    if ((tmp_query_start = strstr(connection->request->url, QUERY))
            == NULL) {
        // no query string found
        
        error_declare("No query made!");
        return -1;
    }
    if ((tmp_query_end = strstr(tmp_query_start, AMPERSAND)) != NULL) {
        // ignore multiple query string arguments

        tmp_query_length = tmp_query_end - tmp_query_start;
        if ((query = (char *) malloc(tmp_query_length + 1)) == NULL) {
            error_out("Couldn't malloc!");
        }
        memcpy(query, tmp_query_start, tmp_query_length);
        query[tmp_query_length] = '\0';
    } else {
        // extract only query string
        query = tmp_query_start;
        query_length = strlen(tmp_query_start);
    }

    // clean the query
    query = curl_easy_unescape(curl, query, tmp_query_length, &query_length);
    curl_easy_cleanup(curl);
    query += strlen(QUERY);  // remove leading "query="
    
    // TODO:
    URLResults *results = NULL;
    results = find_relevant_urls(query);

    if (results != NULL) {

        connection->response->body = NULL;
        connection->response->body_length = connection->response->total_body_length
            = serialize_results(results, &(connection->response->body));
        add_hdr(&(connection->response->hdrs), CONTENT_LENGTH,
                itoa_ap(connection->response->body_length));
    } else {
        return 0;
    }

    // set appropriate headers
    add_hdr(&(connection->response->hdrs), "Access-Control-Allow-Origin", "*");

    // create and send response
    response_length = construct_response(connection->response, &response);
    // display_response(connection->response);
    last_read = write_to_socket(sockfd, response, response_length);

    return last_read;
}


int handle_cache_get(int sockfd, int proxy, int last_read, Connection *connection,
                     Connection **connection_list, int *max_fd, fd_set *master) {
    /* Handle get to the cache from the search engine */

    CURL *curl = curl_easy_init();
    char *response = NULL, *get = NULL, *tmp_get_start = NULL,
         *tmp_get_end = NULL;
    int response_length = 0, get_length = 0, tmp_get_length = 0;

    // extract query
    if ((tmp_get_start = strstr(connection->request->url, GET_CACHE))
            == NULL) {
        // no query string found
        
        error_declare("No get made!");
        return -1;
    }
    if ((tmp_get_end = strstr(tmp_get_start, AMPERSAND)) != NULL) {
        // ignore multiple query string arguments
        tmp_get_length = tmp_get_end - tmp_get_start;
        if ((get = (char *) malloc(tmp_get_length + 1)) == NULL) {
            error_out("Couldn't malloc!");
        }
        memcpy(get, tmp_get_start, tmp_get_length);
        get[tmp_get_length] = '\0';
    } else {
        // extract only get string
        get = tmp_get_start;
        get_length = strlen(tmp_get_start);
    }

    // clean the query
    get = curl_easy_unescape(curl, get, tmp_get_length, &get_length);
    curl_easy_cleanup(curl);
    get += strlen(GET_CACHE);  // remove leading "get_cache="
    
    connection->response = get_data_from_cache(get);

    // set appropriate headers
    if (get_hdr_value(connection->response->hdrs, "Access-Control-Allow-Origin") == NULL) {
        add_hdr(&(connection->response->hdrs), "Access-Control-Allow-Origin", "*");
    }

    // create and send response
    response_length = construct_response(connection->response, &response);
    // display_response(connection->response);
    last_read = write_to_socket(sockfd, response, response_length);

    return last_read;
}


int handle_get_response(int last_read, Connection *connection) {
    /* Handle the GET response */

    write_to_socket(connection->target_sockfd,
                    connection->raw, last_read);
    if (!connection->response) {
        if (!header_not_completed(connection->raw, connection->read_len)) {
            connection->response = parse_response(connection->read_len,
                                                  connection->raw);
            free(connection->raw);
            connection->raw = NULL;
            connection->read_len = 0;
        }
    } else {
        memcpy(connection->response->body + connection->response->body_length,
                connection->raw, last_read);
        connection->response->body_length += last_read;
        free(connection->raw);
        connection->raw = NULL;
        connection->read_len = 0;
    }

    if (connection->response->body_length == connection->response->total_body_length) {
        // display_response(connection->response);
        HTTPResponse *evicted_response = check_cache_capacity();
        if (evicted_response != NULL) {
            // An item was evicted - keywords need to be cleared out too
            remove_keywords_from_keywords_table(evicted_response);
        }

        CacheObject *cache_entry = add_data_to_cache(connection->request->url, connection->response);
        // set the keywords
        extract_keywords(&(connection->response), cache_entry);
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


int serialize_results(URLResults *results, char **raw_ptr) {
    /* Serializes results and places it in the buffer provided, returns the
     * length of the serialized output */

    int result_length = 0, i = 0;
    char *raw = NULL;

    while (i < NUM_TOP_RESULTS && results->urls[i] != NULL) {
        if ((raw = (char *) realloc(raw, result_length + strlen(results->urls[i]) + 1))
                == NULL) {
            error_out("Couldn't malloc!");
        }
        memcpy(raw + result_length, results->urls[i], strlen(results->urls[i]));
        result_length += strlen(results->urls[i]) + 1;
        raw[result_length - 1] = '\0';
        i++;
    }

    *raw_ptr = raw;

    return result_length;
}


