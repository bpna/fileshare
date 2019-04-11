/*
 * COMP 112 Final Project
 * 2/18/2019
 *
 * Patrick Kinsella, Jonah Feldman, Nathan Allen
 *
 * operator.c facilitates connection between clients and file servers
 */

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include "database/servertable.h"
#include "messages.h"
#include "partial_message_handler.h"
#include "database/cspairs.h"
#include "io.h"

#define DISCONNECTED -69
#define MAX_MSG_READ 450
#define DB_OWNER "nathan"
#define DB_NAME "fileshare"

void error(const char *msg) {
    perror(msg);
    exit(1);
}

int open_and_bind_socket(int portno);
int add_partial_data(char *data, int length);
int read_handler(int sockfd, struct PartialMessageHandler *handler);
int handle_header(struct Header *h, int sockfd,
                  struct PartialMessageHandler *pm);
int new_client(struct Header *h, int sockfd);
int send_client_exists_ack(int sockfd, char *username);
int send_new_client_ack(int sockfd, struct server_addr *server);
int request_user(struct Header *h, int sockfd);
int new_server(struct Header *h, int sockfd);

int main(int argc, char *argv[]) {
    int master_socket, csock, max_fd, rv, sockfd, status;
    socklen_t clilen;
    fd_set master_fd_set, copy_fd_set;
    struct sockaddr_in serv_addr, cli_addr;
    struct timeval timeout;
    struct PartialMessageHandler *handler = init_partials();

    if (argc < 2) {
        fprintf(stderr,"ERROR, no port provided\n");
        exit(1);
    }

    master_socket = open_and_bind_socket(atoi(argv[1]));

    listen(master_socket, BACKLOG_QUEUE_SIZE);
    FD_SET(master_socket, &master_fd_set);
    max_fd = master_socket;

    clilen = sizeof(cli_addr);
    timeout.tv_sec = 3600;
    timeout.tv_usec = 0;

    while (1) {
        FD_ZERO(&copy_fd_set);
        memcpy(&copy_fd_set, &master_fd_set, sizeof(master_fd_set));

        rv = select(max_fd + 1, &copy_fd_set, NULL, NULL, &timeout);

        if (rv == -1) {
            error("ERROR on select");
        } else if (rv == 0) {
            ;
        } else {
            for (sockfd = 0; sockfd < max_fd + 1; sockfd++) {
                if (FD_ISSET(sockfd, &copy_fd_set)) {
                    if (sockfd == master_socket) {
                        csock = accept(master_socket, (struct sockaddr *) &cli_addr,
                                &clilen);
                        if (csock > 0) {
                            FD_SET(csock, &master_fd_set);
                            if (csock > max_fd) {
                                max_fd = csock;
                            }
                        } else if (csock < 0) {
                            error("ERROR on accept");
                        }
                    } else {
                        status = read_handler(sockfd, handler);
                        if (status == DISCONNECTED) {
                            FD_CLR(sockfd, &master_fd_set);
                        }
                    }
                }
            }
        }
    }

    close(master_socket);
    return 0;
}

static int freshvar() {
    static int x = 0;
    x++;

    return x;
}



int read_handler(int sockfd, struct PartialMessageHandler *handler) {

    int n, header_bytes_read;
    char buffer[HEADER_LENGTH];
    bzero(buffer, HEADER_LENGTH);
    struct Header *msg_header;

    header_bytes_read = get_partial_header(handler, sockfd, buffer);
    msg_header = (void *) buffer;

    if (header_bytes_read < HEADER_LENGTH){
        n = read(sockfd, buffer, HEADER_LENGTH - header_bytes_read);
        if (n < HEADER_LENGTH - header_bytes_read){
            add_partial(handler, buffer, sockfd, n, 0);
            return 0;
        }
        else{
            memcpy(&msg_header[header_bytes_read], buffer, HEADER_LENGTH - header_bytes_read);
            msg_header->length = ntohl(msg_header->length);
            if (msg_header->length > 0){
                add_partial(handler, buffer, sockfd, n, 0);
                return 0;
            }
        }
    }

    return handle_header(msg_header, sockfd,
                         struct PartialMessageHandler *handler);
}

int handle_header(struct Header *h, int sockfd,
                  struct PartialMessageHandler *pm) {
    switch (h->id) {
        case NEW_CLIENT:
            return new_client(h, sockfd);
        case REQUEST_USER:
            return request_user(h, sockfd);
        case CREATE_CLIENT_ACK:
            return DISCONNECTED;
        case NEW_SERVER:
            return new_server(h, sockfd);
        default:
            return 1;
    }
}

int new_client(struct Header *h, int sockfd) {
    db_t *db;
    struct db_return dbr;
    enum DB_STATUS dbs;
    struct server_addr *server;
    int server_sock, n, len;
    struct Header outgoing_message;

    db = connect_to_db(DB_OWNER, DB_NAME);
    dbr = least_populated_server(db);
    if (dbr.status != SUCCESS) {
        close_db_connection(db);
        return 1;
    }

    server = (struct server_addr *) dbr.result;

    dbr = get_server_from_client(db, h->source);
    if (dbr.status == SUCCESS) {
        close_db_connection(db);
        free(server);
        return send_client_exists_ack(sockfd, h->source);
    }

    // server_sock = connect_to_server(server->domain_name, server->port);

    outgoing_message.id = CREATE_CLIENT;
    strcpy(outgoing_message.source, OPERATOR_SOURCE);
    len = strlen(h->source);
    outgoing_message.length = htonl(len);

    n = write(server_sock, (char *) &outgoing_message, HEADER_LENGTH);
    if (n < HEADER_LENGTH) {
        close_db_connection(db);
        free(server);
        close(server_sock);
        return 1;
    }

    n = write(server_sock, h->source, len);
    close(server_sock);
    if (n < len) {
        close_db_connection(db);
        free(server);
        return 1;
    }

    dbs = add_cspair(db, h->source, server);
    close_db_connection(db);
    if (dbs != SUCCESS) {
        free(server);
        return 1;
    }

    return send_new_client_ack(sockfd, server);
}

int send_client_exists_ack(int sockfd, char *username) {
    int n, len;
    struct Header outgoing_message;

    outgoing_message.id = ERROR_CLIENT_EXISTS;
    strcpy(outgoing_message.source, OPERATOR_SOURCE);
    len = strlen(username);
    outgoing_message.length = htonl(len);
    n = write(sockfd, (char *) &outgoing_message, HEADER_LENGTH);
    if (n < HEADER_LENGTH)
        return 1;

    n = write(sockfd, username, len);
    if (n < len)
        return 1;
    return DISCONNECTED;
}

int send_new_client_ack(int sockfd, struct server_addr *server) {
    struct Header outgoing_message;
    int n, len;
    char *payload = calloc(275, sizeof (char));

    sprintf(payload, "%s:%d", server->domain_name, server->port);
    free(server);

    outgoing_message.id = NEW_CLIENT_ACK;
    strcpy(outgoing_message.source, OPERATOR_SOURCE);
    len = strlen(payload);
    outgoing_message.length = htonl(len);

    n = write(sockfd, (char *) &outgoing_message, HEADER_LENGTH);
    if (n < HEADER_LENGTH)
        return 1;

    n = write(sockfd, payload, len);
    if (n < len)
        return 1;

    return DISCONNECTED;
}

int request_user(struct Header *h, int sockfd) {
    db_t *db;
    struct db_return dbr;
    char *desired_user;
    struct server_addr *server;
    int n, len;
    struct Header outgoing_message;
    char *payload = calloc(275, sizeof (char));

    // desired_user = TODO: get desired user

    db = connect_to_db(DB_OWNER, DB_NAME);
    dbr = get_server_from_client(db, desired_user);
    free(desired_user);
    close_db_connection(db);
    strcpy(outgoing_message.source, OPERATOR_SOURCE);
    if (dbr.status == ELEMENT_NOT_FOUND) {
        outgoing_message.id = ERROR_USER_DOES_NOT_EXIST;
    } else if (dbr.status == SUCCESS) {
        outgoing_message.id = REQUEST_USER_ACK;
        server = (struct server_addr *) dbr.result;
        sprintf(payload, "%s:%d", server->domain_name, server->port);
        len = strlen(payload);
        free(server);
        outgoing_message.length = htonl(len);
    } else {
        return 1;
    }

    n = write(sockfd, (char *) &outgoing_message, HEADER_LENGTH);
    if (n < HEADER_LENGTH) {
        free(payload);
        return 1;
    }

    if (outgoing_message.id == REQUEST_USER_ACK) {
        n = write(sockfd, payload, len);
        free(payload);
        if (n < len)
            return 1;
    }

    return DISCONNECTED;
}

int new_server(struct Header *h, int sockfd) {
    db_t *db;
    enum DB_STATUS dbs;
    struct server_addr *server = calloc(1, sizeof (struct server_addr));
    int n, len;
    struct Header outgoing_message;
    char *payload;

    db = connect_to_db(DB_OWNER, DB_NAME);
    server->id = freshvar();
    // server->port = h->length TODO: decide how to send portno
    // server->domain_name = h->source TODO: decide how to send hostname

    dbs = add_server(db, server);
    close_db_connection(db);
    if (dbs == ELEMENT_ALREADY_EXISTS) {
        outgoing_message.id = ERROR_SERVER_EXISTS;
        payload = calloc(275, sizeof (char));
        sprintf(payload, "%s:%d", server->domain_name, server->port);
        len = strlen(payload);
        outgoing_message.length = htonl(len);
    } else if (dbs == SUCCESS) {
        outgoing_message.id = NEW_SERVER_ACK;
    } else {
        return 1;
    }
    free(server);

    strcpy(outgoing_message.source, OPERATOR_SOURCE);
    n = read(sockfd, (char *) &outgoing_message, HEADER_LENGTH);
    if (n < HEADER_LENGTH)
        return 1;

    if (outgoing_message.id == ERROR_SERVER_EXISTS) {
        n = write(sockfd, payload, len);
        free(payload);
        if (n < len)
            return 1;
    }

    return DISCONNECTED;
}
