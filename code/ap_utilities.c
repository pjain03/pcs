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
        error_out("Couldn't connect to the server!");
    }
    
    return sockfd;
}


char *read_all(int sockfd) {
    /* Read until we fail/communication ends abruptly, returns NULL */

    // setup
    int readlen = 0,            // length of raw
        last_read = 0,          // length of the last read into buffer
        buf_size = BUFFER_SIZE; // buffer size
    char *buffer, *raw = NULL;
    if ((buffer = (char *) malloc(buf_size)) == NULL) {
        error_out("Couldn't malloc!");
    }

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

    // cleanup
    free(buffer);

    return raw;
}


char *read_hdr(int sockfd) {
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

    // read until fail/communication interrupted/header complete
    // - header complete when last chars read are CRLF2 or CRCR or LFLF
    //   (CRLF2, CRCR, LFLF are defined in the header file)
    // - we check the above by string comparing the last chars to them. this
    //   means, if the last chars are CRLF2 then strcmp == 0, hdr_togo = 0
    //          if the last chars are CRCR then strcmp == 0, hdr_togo = 0
    //          if the last chars are LFLF then strcmp == 0, hdr_togo = 0
    //  if any of the above cases are hit, hdr_togo must be 0. therefore, we
    //  && the cases together.
    do {
        last_read = read(sockfd, buffer, buf_size);
        if (last_read > 0) {
            raw = realloc(raw, readlen + last_read);
            memcpy(raw + readlen, buffer, last_read);
            readlen += last_read;
        }
        if (readlen >= sizeof(CRLF2)) {
            hdr_togo = strncmp(raw + readlen - strlen(CRLF2), CRLF2, strlen(CRLF2)) &&
                       strncmp(raw + readlen - strlen(CRCR), CRCR, strlen(CRCR)) &&
                       strncmp(raw + readlen - strlen(LFLF), LFLF, strlen(LFLF));
        }
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

    // cleanup
    free(buffer);

    return raw;
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


void free_comm(HTTPCommunication *comm) {
    /* Frees the HTTPCommunication structure */

    if (comm) {
        if (comm->url) {
            free(comm->url);
            comm->url = NULL;
        }
        if (comm->version) {
            free(comm->version);
            comm->version = NULL;
        }
        if (comm->hdrs) {
            free_hdr(comm->hdrs);
            comm->version = NULL;
        }
        if (comm->body) {
            free(comm->body);
            comm->version = NULL;
        }
        free(comm);
        comm = NULL;
    }
}


void display_comm(HTTPCommunication *comm) {
    /* Displays the HTTPCommunication structure */

    if (comm) {
        printf("\nHTTPCommunication:\n-----------------\n");
        printf("Method: %d\n", comm->method);
        printf("Requested URI: %s\n", comm->url);
        printf("HTTP Version: %s\n", comm->version);
        printf("Headers:\n");
        HTTPHeader *hdr;
        for (hdr = comm->hdrs; hdr; hdr = hdr->next) {
            printf("%32s: %s\n", hdr->name, hdr->value);
        }
        printf("Message Body:");
        if (strlen(comm->body)) {
            printf("\n%s\n\n", comm->body);
        } else {
            printf(" EMPTY\n\n");
        }
    }
}


HTTPCommunication *parse_request(char *raw) {
    /* Parses and returns the raw data as a HTTPCommunication structure */

    HTTPCommunication *request;
    if ((request = (HTTPCommunication *) malloc(sizeof(HTTPCommunication)))
            == NULL) {
        error_out("Couldn't malloc!");
    }

    // set the method
    // NOTE: add HTTP methods here! Eg. CONNECT
    size_t method_length = strcspn(raw, " ");
    if (strncmp(raw, GET_RQ, strlen(GET_RQ)) == 0) {
        request->method = GET;
    } else {
        request->method = UNSUPPORTED;
    }
    raw += method_length + 1;

    // set the URI
    size_t uri_length = strcspn(raw, " ");
    if ((request->url = (char *) malloc(uri_length + 1)) == NULL) {
        free_comm(request);
        error_out("Couldn't malloc!");
    }
    memcpy(request->url, raw, uri_length);
    request->url[uri_length] = '\0';
    raw += uri_length + 1;

    // set the HTTP-Version
    size_t ver_length = strcspn(raw, CRLF);
    if ((request->version = (char *) malloc(ver_length + 1)) == NULL) {
        free_comm(request);
        error_out("Couldn't malloc!");
    }
    memcpy(request->version, raw, ver_length);
    request->version[ver_length] = '\0';
    raw += ver_length;
    if (strncmp(raw, CR, strlen(CR)) == 0) {
        raw += strlen(CR);
    }
    if (strncmp(raw, LF, strlen(LF)) == 0) {
        raw += strlen(LF);
    }

    // set the hdrs
    HTTPHeader *hdr = NULL, *lst = NULL;
    while (strncmp(raw, CRLF, strlen(CRLF)) != 0) {
        lst = hdr;
        if ((hdr = (HTTPHeader *) malloc(sizeof(HTTPHeader))) == NULL) {
            free_comm(request);
            error_out("Couldn't malloc!");
        }

        // header field name
        size_t name_length = strcspn(raw, ":");
        if ((hdr->name = (char *) malloc(name_length + 1)) == NULL) {
            free_comm(request);
            error_out("Couldn't malloc!");
        }
        memcpy(hdr->name, raw, name_length);
        hdr->name[name_length] = '\0';
        raw += name_length;
        while (*raw == ' ') {
            raw++;
        }

        // header field value
        size_t value_length = strcspn(raw, CRLF);
        if ((hdr->value = (char *) malloc(value_length + 1)) == NULL) {
            free_comm(request);
            error_out("Couldn't malloc!");
        }
        memcpy(hdr->value, raw, value_length);
        hdr->value[value_length] = '\0';
        raw += value_length;
        if (strncmp(raw, CR, strlen(CR)) == 0) {
            raw += strlen(CR);
        }
        if (strncmp(raw, LF, strlen(LF)) == 0) {
            raw += strlen(LF);
        }

        hdr->next = lst;
    }
    request->hdrs = hdr;
    raw += strlen(CRLF);

    // set the body
    size_t body_length = strlen(raw);
    if ((request->body = (char *) malloc(body_length + 1)) == NULL) {
        free_comm(request);
        error_out("Couldn't malloc!");
    }
    memcpy(request->body, raw, body_length);
    request->body[body_length] = '\0';

    return request;
}


