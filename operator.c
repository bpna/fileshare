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

#define DISCONNECTED 0
#define MAX_MSG_READ 450
#define DB_OWNER "nathan"
#define DB_NAME "fileshare"

void error(const char *msg) {
    perror(msg);
    exit(1);
}

int open_and_bind_socket(int portno);
int add_partial_data(char *data, int length);
int read_handler(int sockfd);
int handle_header(struct Header *h, int sockfd,
                  struct PartialMessageHandler *pm);
int new_client(struct Header *h, int sockfd);
int send_new_client_ack(int sockfd, struct server_addr *server);
int request_user(struct Header *h, int sockfd);
int new_server(struct Header *h, int sockfd);

int main(int argc, char *argv[]) {
    int master_socket, csock, max_fd, rv, sockfd, status;
    socklen_t clilen;
    fd_set master_fd_set, copy_fd_set;
    struct sockaddr_in serv_addr, cli_addr;
    struct timeval timeout;
    struct PartialMessageHandler *p = init_partials();

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
                        status = read_handler(sockfd);
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

int open_and_bind_socket(int portno) {
    struct sockaddr_in serv_addr;
    int master_socket;

    master_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (master_socket < 0) {
        error("ERROR opening socket");
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    if (bind(master_socket, (struct sockaddr *) &serv_addr,
             sizeof(serv_addr)) < 0){
        error("ERROR on binding");
    }

    return master_socket;
}

int read_handler(int sockfd) {
    char buffer[512];
    int bytes_read = read(sockfd, buffer, 512);

    if (bytes_read == 0) {
        close(sockfd);
        return DISCONNECTED;
    }

    // if (bytes_read < HEADER_LENGTH) {

    return 0;
}

int handle_header(struct Header *h, int sockfd,
                  struct PartialMessageHandler *pm) {
    switch (h->id) {
        case NEW_CLIENT:
            return new_client(h, sockfd);
        case REQUEST_USER:
            return request_user(h, sockfd);
        case CREATE_CLIENT_ACK:
            return 0;
        case NEW_SERVER:
            return new_server(h, sockfd);
        default:
            return 1;
    }
}

int new_client(struct Header *h, int sockfd) {
    db_t *db;
    struct db_return dbr;
    struct server_addr *server;
    int server_sock, n, len;
    struct Header outgoing_message;

    db = connect_to_db(DB_OWNER, DB_NAME);
    dbr = least_populated_server(db);
    close_db_connection(db);
    server = (struct server_addr *) dbr.result;
    if (dbr.status != SUCCESS) {
        free(server);
        return 1;
    }

    // server_sock = connect_to_server(server->domain_name, server->port);

    outgoing_message.id = CREATE_CLIENT;
    strcpy(outgoing_message.source, OPERATOR_SOURCE);
    len = strlen(h->source);
    outgoing_message.length = htonl(len);

    n = send(server_sock, (char *) &outgoing_message, HEADER_LENGTH, 0);
    if (n < HEADER_LENGTH) {
        free(server);
        close(server_sock);
        return 1;
    }

    n = send(server_sock, h->source, len, 0);
    close(server_sock);
    if (n < len)
        return 1;

    return send_new_client_ack(sockfd, server);
}

int send_new_client_ack(int sockfd, struct server_addr *server) {
    struct Header outgoing_message;
    int n, len;
    char *payload = calloc(275, sizeof (char));

    sprintf(payload, "%s:%d", server->domain_name, server->port);
    free(server);

    outgoing_message.id = 2;
    strcpy(outgoing_message.source, OPERATOR_SOURCE);
    len = strlen(payload);
    outgoing_message.length = htonl(len);

    n = send(sockfd, (char *) &outgoing_message, HEADER_LENGTH, 0);
    if (n < HEADER_LENGTH)
        return 1;

    n = send(sockfd, payload, len, 0);
    if (n < len)
        return 1;

    return DISCONNECTED;
}

int request_user(struct Header *h, int sockfd) {
    db_t *db;
    struct db_return dbr;
    char *desired_user;
    struct server_addr *server;
    int server_sock, n, len;
    struct Header outgoing_message;
    char *payload = calloc(275, sizeof (char));

    //desired_user = TODO: get desired user

    db = connect_to_db(DB_OWNER, DB_NAME);
    dbr = get_server_from_client(db, desired_user);
    free(desired_user);
    close_db_connection(db);
    outgoing_message.id = 5;
    strcpy(outgoing_message.source, OPERATOR_SOURCE);
    if (dbr.status == ELEMENT_NOT_FOUND) {
        len = 0;
    } else if (dbr.status == SUCCESS) {
        server = (struct server_addr *) dbr.result;
        sprintf(payload, "%s:%d", server->domain_name, server->port);
        len = strlen(payload);
        free(server);
    } else {
        return 1;
    }

    outgoing_message.length = htonl(len);
    n = send(sockfd, (char *) &outgoing_message, HEADER_LENGTH, 0);
    if (n < HEADER_LENGTH) {
        free(payload);
        return 1;
    }

    if (len > 0) {
        n = send(sockfd, payload, len, 0);
        free(payload);
        if (n < len) {
            return 1;
        }
    }

    return DISCONNECTED;
}

int new_server(struct Header *h, int sockfd) {
    return DISCONNECTED;
}
