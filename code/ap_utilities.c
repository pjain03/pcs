/******************************************************************************
 *                                                                            *
 *                      AUTHORS: Annie Chen, Pulkit Jain                      *
 *                      PURPOSE: Common structures and functions              *
 *                               used by both Client and Server               *
 *                                                                            *
 *****************************************************************************/


//
// Interface
//
#include "ap_utilities.h"


//
// Implementation
//
void error_declare(const char *msg) {
    /* Output error message */

    fprintf(stderr, "\nERROR: %s\n\n", msg);
}


void error_out(const char *msg) {
    /* Output error message and exit */

    error_declare(msg);
    exit(EXIT_FAILURE);
}


int connect_to_server(char *hostname, int port_num) {
    /* Setup the client and return its information */

    int sockfd;
    struct sockaddr_in serveraddr;
    struct hostent *server;

    // socket: create the socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        error_out("Couldn't open socket!");
    }

    // gethostbyname: get the server's DNS entry
    if ((server = gethostbyname(hostname)) == NULL) {
        error_out("Couldn't get host!");
    }

    // build the server's Internet address
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
	  (char *)&serveraddr.sin_addr.s_addr, server->h_length);
    serveraddr.sin_port = htons(port_num);

    // connect: create a connection with the server
    if (connect(sockfd, (const struct sockaddr*) &serveraddr,
                sizeof(serveraddr)) < 0) {
        error_declare("Couldn't connect to the server!");
    }
    
    return sockfd;
}


int header_not_completed(char *raw, int raw_len) {
    /* Returns 0 if request stored in raw was completed and 1 if not */

    // - header complete when last chars read are CRLF2 or CRCR or LFLF
    //   (CRLF2, CRCR, LFLF are defined in the header file)
    // - we check the above by string comparing the last chars to them. this
    //   means, if the last chars are CRLF2 then strcmp == 0, hdr_togo = 0
    //          if the last chars are CRCR then strcmp == 0, hdr_togo = 0
    //          if the last chars are LFLF then strcmp == 0, hdr_togo = 0
    //  if any of the above cases are hit, hdr_togo must be 0. therefore, we
    //  && the cases together.

    int hdr_togo = 1;
    char *tmp;

    if ((tmp = strstr(raw, CRLF2)) != NULL) {
        hdr_togo &= 0;
    } else if ((tmp = strstr(raw, CRCR)) != NULL) {
        hdr_togo &= 0;
    } else if ((tmp = strstr(raw, LFLF)) != NULL) {
        hdr_togo &= 0;
    }

    return hdr_togo;
}


int read_all(int sockfd, char **raw_ptr) {
    /* Read until we fail/communication ends abruptly, returns NULL */

    // setup
    int readlen = 0,            // length of raw
        last_read = 0,          // length of the last read into buffer
        buf_size = BUFFER_SIZE; // buffer size
    char *buffer, *raw = NULL;
    if ((buffer = (char *) malloc(buf_size)) == NULL) {
        error_out("Couldn't malloc!");
    }
    bzero(buffer, BUFFER_SIZE);

    // read until we cannot read anymore/communication is ended
    do {
        last_read = read(sockfd, buffer, buf_size);
        if (last_read > 0) {
            raw = realloc(raw, readlen + last_read);
            memcpy(raw + readlen, buffer, last_read);
            readlen += last_read;
        }
        bzero(buffer, buf_size);
    } while (last_read > 0);

    // if we failed out of the read loop, return NULL
    if (last_read < 0) {
        if (raw) {
            free(raw);
        }
        raw = NULL;
    } else {
        raw = realloc(raw, readlen + 1);
        raw[readlen] = '\0';
    }

    // set the requested pointer to the raw read data
    *raw_ptr = raw;

    // cleanup
    free(buffer);

    return readlen;
}


int read_hdr(int sockfd, char **raw_ptr) {
    /* Read until we fail/communication ends abruptly, returns NULL
     * OR until we get the end of a header, returns the raw header */

    // setup
    int readlen = 0,            // length of raw
        last_read = 0,          // length of the last read into buffer
        buf_size = BUFFER_SIZE, // buffer size
        hdr_togo = 1;           // is more of header to be read still?
    char *buffer, *raw = NULL;
    if ((buffer = (char *) malloc(buf_size)) == NULL) {
        error_out("Couldn't malloc!");
    }
    bzero(buffer, BUFFER_SIZE);

    do {
        last_read = read(sockfd, buffer, buf_size);
        if (last_read > 0) {
            raw = realloc(raw, readlen + last_read);
            memcpy(raw + readlen, buffer, last_read);
            readlen += last_read;
        }
        hdr_togo = header_not_completed(raw, readlen);
        bzero(buffer, buf_size);
    } while (last_read > 0 && hdr_togo);

    // if we failed out of the read loop, return NULL
    if (last_read < 0) {
        if (raw) {
            free(raw);
        }
        raw = NULL;
    } else {
        raw = realloc(raw, readlen + 1);
        raw[readlen] = '\0';
    }

    // set the requested pointer to the raw read data
    *raw_ptr = raw;

    // cleanup
    free(buffer);

    return readlen;
}


void free_hdr(HTTPHeader *hdr) {
    /* Frees the HTTPHeader structure */

    if (hdr) {
        if (hdr->name) {
            free(hdr->name);
            hdr->name = NULL;
        }
        if (hdr->value) {
            free(hdr->value);
            hdr->value = NULL;
        }
        free_hdr(hdr->next);
        free(hdr);
        hdr = NULL;
    }
}


void free_request(HTTPRequest *request) {
    /* Frees the HTTPRequest structure */

    if (request) {
        if (request->host) {
            free(request->host);
            request->host = NULL;
        }
        if (request->url) {
            free(request->url);
            request->url = NULL;
        }
        if (request->version) {
            free(request->version);
            request->version = NULL;
        }
        if (request->hdrs) {
            free_hdr(request->hdrs);
            request->hdrs = NULL;
        }
        if (request->body) {
            free(request->body);
            request->body = NULL;
        }
        free(request);
        request = NULL;
    }
}


void free_response(HTTPResponse *response) {
    /* Frees the HTTPResponse structure */

    if (response) {
        if (response->status) {
            free(response->status);
            response->status = NULL;
        }
        if (response->status_desc) {
            free(response->status_desc);
            response->status_desc = NULL;
        }
        if (response->version) {
            free(response->version);
            response->version = NULL;
        }
        if (response->hdrs) {
            free_hdr(response->hdrs);
            response->hdrs = NULL;
        }
        if (response->body) {
            free(response->body);
            response->body = NULL;
        }
        free(response);
        response = NULL;
    }
}


void display_request(HTTPRequest *request) {
    /* Displays the HTTPRequest structure */

    if (request) {
        printf("\nHTTP Request:\n------------\n");
        printf("Method: %d\n", request->method);
        printf("URL: %s\n", request->url);
        printf("Version: %s\n", request->version);
        printf("Host: %s\n", request->host);
        printf("Port: %d\n", request->port);
        printf("Headers:\n");
        HTTPHeader *hdr;
        for (hdr = request->hdrs; hdr; hdr = hdr->next) {
            printf("%32s: %s\n", hdr->name, hdr->value);
        }
        /*
        printf("Message Body:");
        if (request->body_length) {
            printf("\n%.*s\n\n", request->body_length, request->body);
        } else {
            printf(" EMPTY\n\n");
        } */
    }
}


void display_response(HTTPResponse *response) {
    /* Displays the HTTPResponse structure */

    if (response) {
        printf("\nHTTP Response:\n-------------\n");
        printf("Version: %s\n", response->version);
        printf("Status: %s\n", response->status);
        printf("Status Description: %s\n", response->status_desc);
        printf("Headers:\n");
        HTTPHeader *hdr;
        for (hdr = response->hdrs; hdr; hdr = hdr->next) {
            printf("%32s: %s\n", hdr->name, hdr->value);
        }
        // Not printing out message body, clogging up terminal
        printf("Message Body:");
        if (response->body_length) {
            printf("\n%d\n\n", response->total_body_length);
        } else {
            printf(" EMPTY\n\n"); 
        }
    }
}


char *get_hdr_value(HTTPHeader *hdrs, const char *name) {
    /* Get the value for the name from the hdrs */

    char *value = NULL;

    for (HTTPHeader *hdr = hdrs; hdr; hdr = hdr->next) {
        size_t value_length = strcspn(hdr->value, "\0");
        if (strcmp(hdr->name, name) == 0) {
            int value_length = strcspn(hdr->value, EMPTY);
            if ((value = (char *) malloc(value_length + 1)) == NULL) {
                error_out("Couldn't malloc!");
            }
            memcpy(value, hdr->value, value_length);
            value[value_length] = '\0';
        }
    }

    return value;
}


HTTPHeader *parse_headers(int *offset, char **raw_ptr) {
    /* Parses the headers from the raw data and returns the length of the data
     * parsed from the passed in raw parameter */

    int length = 0;
    char *raw = *raw_ptr;
    HTTPHeader *hdr = NULL, *lst = NULL;

    while (strncmp(raw, CRLF, strlen(CRLF)) != 0) {
        lst = hdr;
        if ((hdr = (HTTPHeader *) malloc(sizeof(HTTPHeader))) == NULL) {
            error_out("Couldn't malloc!");
        }

        // header field name
        size_t name_length = strcspn(raw, ":");
        if ((hdr->name = (char *) malloc(name_length + 1)) == NULL) {
            error_out("Couldn't malloc!");
        }
        memcpy(hdr->name, raw, name_length);
        hdr->name[name_length] = '\0';
        raw += name_length + 1;
        length += name_length + 1;
        while (*raw == ' ') {
            raw++;
            length++;
        }

        // header field value
        size_t value_length = strcspn(raw, CRLF);
        if ((hdr->value = (char *) malloc(value_length + 1)) == NULL) {
            error_out("Couldn't malloc!");
        }
        memcpy(hdr->value, raw, value_length);
        hdr->value[value_length] = '\0';
        raw += value_length;
        length += value_length;
        if (strncmp(raw, CR, strlen(CR)) == 0) {
            raw += strlen(CR);
            length += strlen(CR);
        }
        if (strncmp(raw, LF, strlen(LF)) == 0) {
            raw += strlen(LF);
            length += strlen(LF);
        }

        // add to the header if its not the age header
        if (strcmp(hdr->name, AGE)) {
            hdr->next = lst;
        } else {
            free_hdr(hdr);
        }
    }

    // move the buffer and set the offset
    *raw_ptr = raw;
    *offset += length;

    return hdr;
}


HTTPRequest *parse_request(int length, char *raw) {
    /* Parses and returns the raw data as a HTTPRequest structure */

    int offset = 0;
    char *host;
    HTTPRequest *request;
    if ((request = (HTTPRequest *) malloc(sizeof(HTTPRequest))) == NULL) {
        error_out("Couldn't malloc!");
    }

    // set the method, add new methods here (Eg. CONNECT)
    size_t method_length = strcspn(raw, " ");
    if (strncmp(raw, GET_RQ, strlen(GET_RQ)) == 0) {
        request->method = GET;
    } else if (strncmp(raw, CONNECT_RQ, strlen(CONNECT_RQ)) == 0) {
        request->method = CONNECT;
    } else {
        request->method = UNSUPPORTED;
    }
    raw += method_length + 1;
    offset += method_length + 1;

    // set the URI
    size_t uri_length = strcspn(raw, " ");
    if ((request->url = (char *) malloc(uri_length + 1)) == NULL) {
        free_request(request);
        error_out("Couldn't malloc!");
    }
    memcpy(request->url, raw, uri_length);
    request->url[uri_length] = '\0';
    raw += uri_length + 1;
    offset += uri_length + 1;

    // set the HTTP-Version
    size_t ver_length = strcspn(raw, CRLF);
    if ((request->version = (char *) malloc(ver_length + 1)) == NULL) {
        free_request(request);
        error_out("Couldn't malloc!");
    }
    memcpy(request->version, raw, ver_length);
    request->version[ver_length] = '\0';
    raw += ver_length;
    offset += ver_length;
    if (strncmp(raw, CR, strlen(CR)) == 0) {
        raw += strlen(CR);
        offset += strlen(CR);
    }
    if (strncmp(raw, LF, strlen(LF)) == 0) {
        raw += strlen(LF);
        offset += strlen(LF);
    }

    // set the hdrs
    request->hdrs = parse_headers(&offset, &raw);
    if (strncmp(raw, CR, strlen(CR)) == 0) {
        raw += strlen(CR);
        offset += strlen(CR);
    }
    if (strncmp(raw, LF, strlen(LF)) == 0) {
        raw += strlen(LF);
        offset += strlen(LF);
    }

    // extract host and port if there is "Host" header otherwise keep defaults
    request->host = request->url;
    request->port = DEFAULT_HTTP_PORT;
    if ((host = get_hdr_value(request->hdrs, HOST)) != NULL) {
        size_t value_length = strcspn(host, EMPTY),
               host_length = strcspn(host, COLON);
        if (value_length != host_length) {
            request->port = atoi(host + host_length + 1);
        }
        if ((request->host = (char *) malloc(host_length + 1)) == NULL) {
            free_request(request);
            error_out("Couldn't malloc!");
        }
        memcpy(request->host, host, host_length);
        request->host[host_length] = '\0';
    }

    // set the body
    request->body_length = length - offset;
    if ((request->body = (char *) malloc(request->body_length)) == NULL) {
        free_request(request);
        error_out("Couldn't malloc!");
    }
    memcpy(request->body, raw, request->body_length);

    return request;
}


HTTPResponse *parse_response(int length, char *raw) {
    /* Parses and returns the raw data as an HTTPResponse structure */

    int offset = 0;
    HTTPResponse *response;
    if ((response = (HTTPResponse *) malloc(sizeof(HTTPResponse))) == NULL) {
        error_out("Couldn't malloc!");
    }

    // set the HTTP-Version
    size_t ver_length = strcspn(raw, " ");
    if ((response->version = (char *) malloc(ver_length + 1)) == NULL) {
        free_response(response);
        error_out("Couldn't malloc!");
    }
    memcpy(response->version, raw, ver_length);
    response->version[ver_length] = '\0';
    raw += ver_length + 1;
    offset += ver_length + 1;

    // set the status
    size_t status_length = strcspn(raw, " ");
    if ((response->status = (char *) malloc(status_length + 1)) == NULL) {
        free_response(response);
        error_out("Couldn't malloc!");
    }
    memcpy(response->status, raw, status_length);
    response->status[status_length] = '\0';
    raw += status_length + 1;
    offset += status_length + 1;

    // set the status description
    size_t status_desc_length = strcspn(raw, CRLF);
    if ((response->status_desc = (char *) malloc(status_desc_length + 1))
            == NULL) {
        free_response(response);
        error_out("Couldn't malloc!");
    }
    memcpy(response->status_desc, raw, status_desc_length);
    response->status_desc[status_desc_length] = '\0';
    raw += status_desc_length;
    offset += status_desc_length;
    if (strncmp(raw, CR, strlen(CR)) == 0) {
        raw += strlen(CR);
        offset += strlen(CR);
    }
    if (strncmp(raw, LF, strlen(LF)) == 0) {
        raw += strlen(LF);
        offset += strlen(LF);
    }

    // set the hdrs
    response->hdrs = parse_headers(&offset, &raw);
    if (strncmp(raw, CR, strlen(CR)) == 0) {
        raw += strlen(CR);
        offset += strlen(CR);
    }
    if (strncmp(raw, LF, strlen(LF)) == 0) {
        raw += strlen(LF);
        offset += strlen(LF);
    }

    // set the body
    char *blen = get_hdr_value(response->hdrs, CONTENT_LENGTH);
    response->total_body_length = atoi(blen);
    printf("BODY_LEN: %d\n", response->total_body_length);
    response->body = NULL;
    if (length - offset) {
        response->body = (char *) malloc(response->total_body_length);
        memcpy(response->body, raw + length - offset, length - offset);
    }
    response->body_length = length - offset;

    // set the fetch time
    response->time_fetched = time(NULL);

    return response;
}


int write_to_socket(int sockfd, char *buffer, int buffer_length) {
    /* Write to the given socket and return the length of the written data */

    int writelen = 0;

    if ((writelen = write(sockfd, buffer, buffer_length)) < 0) {
        error_declare("Couldn't write to the socket!");
    }

    return writelen;
}


int construct_response(HTTPResponse *response, char **raw_ptr) {
    /* Reconstructs a response into the provided buffer */
    // TODO: THIS IS VERY INEFFICIENT (many reallocs)
    //       consider calculating total memory we need in the start and then
    //       parsing it as needed

    // setup
    int response_length = 0;
    char *raw = NULL;

    // move the HTTP Version into raw
    int version_length = strlen(response->version);
    if ((raw = (char *) realloc(raw, version_length + 1)) == NULL) {
        free_response(response);
        error_out("Couldn't realloc!");    
    }
    memcpy(raw, response->version, version_length);
    raw[version_length] = ' ';
    response_length = version_length + 1;

    // move the Status into raw
    int status_length = strlen(response->status);
    if ((raw = (char *) realloc(raw, response_length + status_length + 1))
            == NULL) {
        free_response(response);
        error_out("Couldn't realloc!");            
    }
    memcpy(raw + response_length, response->status, status_length);
    raw[response_length + status_length] = ' ';
    response_length += status_length + 1;

    // move the Status Description into raw
    int status_desc_length = strlen(response->status_desc);
    if ((raw = (char *) realloc(raw, response_length + status_desc_length + 1))
            == NULL) {
        free_response(response);
        error_out("Couldn't realloc!");
    }
    memcpy(raw + response_length, response->status_desc, status_desc_length);
    raw[response_length + status_desc_length] = ' ';
    response_length += status_desc_length + 1;

    // CRLF
    int crlf_length = strlen(CRLF);
    if ((raw = (char *) realloc(raw, response_length + crlf_length)) == NULL) {
        free_response(response);
        error_out("Couldn't realloc!");
    }
    memcpy(raw + response_length, CRLF, crlf_length);
    response_length += crlf_length;

    // move header into raw
    for (HTTPHeader *hdr = response->hdrs; hdr; hdr = hdr->next) {
        int name_length = strlen(hdr->name);
        int value_length = strlen(hdr->value);
        if ((raw = (char *) realloc(raw, response_length + name_length + 2 +
                                    value_length + crlf_length)) == NULL) {
            free_response(response);
            error_out("Couldn't realloc!");
        }
        // move name into raw
        memcpy(raw + response_length, hdr->name, name_length);
        response_length += name_length;
        // add separator between name and value
        raw[response_length] = ':';
        raw[response_length + 1] = ' ';
        response_length += 2;
        // move value into raw
        memcpy(raw + response_length, hdr->value, value_length);
        response_length += value_length;
        // add crlf at the end of the header
        memcpy(raw + response_length, CRLF, crlf_length);
        response_length += crlf_length;
    }

    // add age to header
    int age = time(NULL) - response->time_fetched;
    int name_length = strlen(AGE);
    int value_length = snprintf( NULL, 0, "%d", age) + 1;
    char age_string[value_length];
    snprintf(age_string, value_length, "%d", age);
    if ((raw = (char *) realloc(raw, response_length + name_length + 2 +
                                value_length + 2 * crlf_length)) == NULL) {
        free_response(response);
        error_out("Couldn't realloc!");
    }
    memcpy(raw + response_length, AGE, name_length);
    response_length += name_length;
    raw[response_length] = ':';
    raw[response_length + 1] = ' ';
    response_length += 2;
    memcpy(raw + response_length, age_string, value_length);
    response_length += value_length;
    memcpy(raw + response_length, CRLF, crlf_length);
    response_length += crlf_length;

    // CRLF
    memcpy(raw + response_length, CRLF, crlf_length);
    response_length += crlf_length;

    // move the body into raw
    if ((raw = (char *) realloc(raw, response_length + response->body_length))
            == NULL) {
        free_response(response);
        error_out("Couldn't realloc!");
    }
    memcpy(raw + response_length, response->body, response->body_length);
    response_length += response->body_length;

    // set the requested pointer to our data
    *raw_ptr = raw;

    return response_length;
}


int read_sockfd(int sockfd, char *buffer, Connection *connection) {
    /* Reads from the socket and stores it in the partial buffer of the
     * connection */

    int last_read = 0;
    bzero(buffer, BUFFER_SIZE);

    if ((last_read = read(sockfd, buffer, BUFFER_SIZE)) < 0) {
        printf("sockfd: %d ", sockfd);
        error_declare("Couldn't read from client socket!");
        return -1;
    }

    if (last_read > 0) {
        if ((connection->raw = (char *) realloc(connection->raw,
                connection->read_len + last_read)) == NULL) {
            error_declare("Couldn't realloc!");
            clear_connection(connection);
            return -1;
        }
        memcpy(connection->raw + connection->read_len, buffer, last_read);
        connection->read_len += last_read;
    }

    return last_read;
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


int add_client(int proxy, Connection **connection_list) {
    /* Adds client to connection_list */

    int n;

    if ((n = accept_client(proxy)) < 0) {
        error_declare("Cannot accept client!");
    } else {
        n = add_client_connection(n, connection_list);
    }
    
    printf("Accepted client %d\n", n);

    return n;
}


int add_server(int proxy, int client, HTTPRequest *request,
               Connection **connection_list) {
    /* Adds server to connection_list */

    int n;

    if ((n = connect_to_server(request->host, request->port)) < 0) {
        error_declare("Cannot connect to the server!");
    } else {
        n = add_server_connection(n, client, request, connection_list);
    }
    
    printf("Added server %d\n", n);

    return n;
}


int add_client_connection(int requesting_sockfd, Connection **connection_list) {
    /* Adds connection to our connection list
     * NOTE: -1 means we couldn't add to our connection list */

    Connection *connection;

    if ((connection = (Connection *) malloc(sizeof(Connection))) == NULL) {
        error_declare("Couldn't malloc!");
        return -1;
    }
    connection->requesting_sockfd = requesting_sockfd;
    connection->target_sockfd = -1;
    connection->raw = NULL;
    connection->read_len = 0;
    // connection->got_header = 0;
    connection->request = NULL;
    connection->response = NULL;
    HASH_ADD_INT(*connection_list, requesting_sockfd, connection);

    return requesting_sockfd;
}


int add_server_connection(int requesting_sockfd, int target_sockfd,
                          HTTPRequest *request, Connection **connection_list) {
    /* Adds connection to our connection list
     * NOTE: -1 means we couldn't add to our connection list */

    Connection *connection;

    if ((connection = (Connection *) malloc(sizeof(Connection))) == NULL) {
        error_declare("Couldn't malloc!");
        return -1;
    }
    connection->requesting_sockfd = requesting_sockfd;
    connection->target_sockfd = target_sockfd;
    connection->raw = NULL;
    connection->read_len = 0;
    // connection->got_header = 0;
    connection->request = request;
    connection->response = NULL;
    HASH_ADD_INT(*connection_list, requesting_sockfd, connection);

    Connection *client_connection = search_connection(target_sockfd,
                                                      connection_list);
    client_connection->target_sockfd = requesting_sockfd;

    return requesting_sockfd;
}


void remove_connection(int sockfd, fd_set *master, Connection **connection_list) {
    /* Removes client from the connection_list */

    Connection *connection = search_connection(sockfd, connection_list);

    if (connection != NULL) {

        // Remove the target connection to avoid any future 1-sided communication
        Connection *target = search_connection(connection->target_sockfd,
                                               connection_list);
        if (target != NULL) {
            HASH_DEL(*connection_list, target);
            clear_connection(target);
            FD_CLR(connection->target_sockfd, master);
            close(connection->target_sockfd);
            connection->request = NULL;  // so we don't double free this pointer
            printf("Removing %d\n", connection->target_sockfd);
        }

        // Now we can remove the intended connection safely
        HASH_DEL(*connection_list, connection);
        clear_connection(connection);
        FD_CLR(sockfd, master);
        close(sockfd);
        printf("Removing %d\n", sockfd);
    }
}


Connection *search_connection(int sockfd, Connection **connection_list) {
    /* Searches for client and returns its ID from connection_list */

    Connection *connection;
    HASH_FIND_INT(*connection_list, &sockfd, connection);
    return connection;
}


void clear_connection(Connection *connection) {
    /* Closes the socket connection and deletes all data associated with it */

    if (connection) {
        if (connection->raw) {
            free(connection->raw);
            connection->raw = NULL;
        }
        if (connection->request) {
            free_request(connection->request);
            connection->request = NULL;
        }
        // // IMPORTANT: do not free response, handling response is the cache's
        //               business
        // if (connection->response) {
        //     free_response(connection->response);
        //     connection->response = NULL;
        // }
        free(connection);
        connection = NULL;
    }
}


