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
#include "db_wrapper.h"

#define DISCONNECTED -69
#define MAX_MSG_READ 450
#define USE_DB 1
#define CSPAIRS_FNAME "client_cspairs.txt"
#define CSPAIRS_FILE_MAX_LENGTH 10000

int open_and_bind_socket(int portno);
int add_partial_data(char *data, int length);
int read_handler(int sockfd, struct PartialMessageHandler *handler);
int handle_header(struct Header *h, int sockfd,
                  struct PartialMessageHandler *pm);
int new_client(struct Header *h, int sockfd);
int send_client_exists_ack(int sockfd, char *username);
int send_new_client_ack(int sockfd, struct Server *server);
int send_new_personal_server_ack(int sockfd);
int request_user(struct Header *h, int sockfd, struct PartialMessageHandler *pm);
int user_list(int sockfd);
int new_server(struct Header *h, int sockfd, int personal);
int new_personal_server(struct Header *h, int sockfd);

int main(int argc, char *argv[]) {
    int master_socket, csock, max_fd, rv, sockfd, status;
    socklen_t clilen;
    fd_set master_fd_set, copy_fd_set;
    struct sockaddr_in cli_addr;
    struct timeval timeout;
    struct PartialMessageHandler *handler = init_partials();
    enum DB_STATUS dbs;
    db_t db = NULL;

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

    //if (USE_DB) {
    db = connect_to_db(DB_OWNER, DB_NAME);
    dbs = create_server_table(db, 1);
    if (dbs != SUCCESS && dbs != ELEMENT_ALREADY_EXISTS)
        error("ERROR creating server table");
    dbs = create_cspairs_table(db, 1);
    if (dbs != SUCCESS && dbs != ELEMENT_ALREADY_EXISTS)
        error("ERROR creating cspairs table");

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
                        fprintf(stderr, "there's a new connection in town\n" );
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
                        fprintf(stderr, "new message incoming\n" );
                        status = read_handler(sockfd, handler);
                        if (status == DISCONNECTED) {
                            delete_partial(handler, sockfd);
                            FD_CLR(sockfd, &master_fd_set);
                            close(sockfd);
                        }
                    }
                }
            }
        }
    }

    close(master_socket);
    return 0;
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
        if (n < 1){
            return DISCONNECTED;
        }
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

    return handle_header(msg_header, sockfd, handler);
}

int handle_header(struct Header *h, int sockfd,
                  struct PartialMessageHandler *pm) {
    fprintf(stderr, "in the header, \nmsgType is %d\nsource is %s\npassword is %s\nlength is %d\n", h->id, h->source, h->password, h->length);
    switch (h->id) {
        case NEW_CLIENT:
        fprintf(stderr, "about to go to return new_client\n" );
            return new_client(h, sockfd);
        case USER:
            return request_user(h, sockfd, pm);
        case CREATE_CLIENT_ACK:
            return DISCONNECTED;
        case NEW_SERVER:
            return new_server(h, sockfd, 0);
        case USER_LIST:
            return user_list(sockfd);
        case NEW_PERSONAL_SERVER:
            return new_personal_server(h, sockfd);
        default:
            return DISCONNECTED;
    }
}

int new_client(struct Header *h, int sockfd) {
    db_t db;
    struct db_return dbr;
    enum DB_STATUS dbs;
    struct Server *server;
    int server_sock, len;
    struct Header outgoing_message;
    bzero(&outgoing_message, HEADER_LENGTH);
    char client_info[512];
    bzero(client_info, 512);

    db = connect_to_db(DB_OWNER, DB_NAME);
    dbr = least_populated_server(db);
    if (dbr.status != SUCCESS) {
        fprintf(stderr, "dbr status was not success\n");
        close_db_connection(db);
        return DISCONNECTED;
    }

    server = (struct Server *) dbr.result;

    dbr = get_server_from_client(db, h->source);
    if (dbr.status == SUCCESS) {
        fprintf(stderr, "ERROR: client already exists\n");
        close_db_connection(db);
        free(server);
        return send_client_exists_ack(sockfd, h->source);
    }

    fprintf(stderr, "about to connect to server in new_client: %s\n", server->ip_address);
    server_sock = connect_to_server(server->ip_address, server->port);

    outgoing_message.id = CREATE_CLIENT;
    strcpy(outgoing_message.source, OPERATOR_SOURCE);
    len = strlen(h->source) + strlen(h->password) + 1;
    outgoing_message.length = htonl(len);

    fprintf(stderr, "about to write CREATE_CLIENT to server\n" );
    write_message(server_sock, (void *)&outgoing_message, HEADER_LENGTH);
    sprintf(client_info, "%s:%s", h->source, h->password);
    write_message(server_sock, client_info, len);
    fprintf(stderr, "just finished writing CREATE_CLIENT to server\n" );

    dbs = add_cspair(db, h->source, server, 1);
    close_db_connection(db);
    if (dbs != SUCCESS) {
        fprintf(stderr, "Error: failed to add to cspair\n" );
        free(server);
        close_db_connection(db);
        return DISCONNECTED;
    }

    return send_new_client_ack(sockfd, server);
}

int send_client_exists_ack(int sockfd, char *username) {
    int len;
    struct Header outgoing_message;

    outgoing_message.id = ERROR_CLIENT_EXISTS;
    strcpy(outgoing_message.source, OPERATOR_SOURCE);
    len = strlen(username);
    outgoing_message.length = htonl(len);
    write_message(sockfd, (char *) &outgoing_message, HEADER_LENGTH);

    write_message(sockfd, username, len);
    return DISCONNECTED;
}

int send_new_client_ack(int sockfd, struct Server *server) {
    struct Header outgoing_message;
    int len;
    char payload[275];
    bzero(payload, 275);

    sprintf(payload, "%s:%d", server->ip_address, server->port);
    free(server);

    outgoing_message.id = NEW_CLIENT_ACK;
    strcpy(outgoing_message.source, OPERATOR_SOURCE);
    len = strlen(payload);
    fprintf(stderr, "in send_new_client_ack, payload is %s and its length is %d\n", payload, len);
    outgoing_message.length = htonl(len);

    write_message(sockfd, (char *) &outgoing_message, HEADER_LENGTH);
    write_message(sockfd, payload, len);

    return DISCONNECTED;
}

int send_new_personal_server_ack(int sockfd) {
    struct Header outgoing_message;

    outgoing_message.id = NEW_PERSONAL_SERVER_ACK;
    strcpy(outgoing_message.source, OPERATOR_SOURCE);
    write_message(sockfd, (char *) &outgoing_message, HEADER_LENGTH);

    return DISCONNECTED;
}

int request_user(struct Header *h, int sockfd, struct PartialMessageHandler *pm) {
    (void) pm;
    db_t db;
    struct db_return dbr;
    struct Server *server;
    int n, len;
    struct Header outgoing_message;
    bzero(&outgoing_message, HEADER_LENGTH);
    char buffer[INIT_BUFFER_LENGTH];
    bzero(buffer, INIT_BUFFER_LENGTH);


    db = connect_to_db(DB_OWNER, DB_NAME);
    dbr = get_server_from_client(db, h->filename);
    close_db_connection(db);
    strcpy(outgoing_message.source, OPERATOR_SOURCE);


    if (dbr.status == ELEMENT_NOT_FOUND) {
        outgoing_message.id = ERROR_USER_DOES_NOT_EXIST;
        outgoing_message.length = 0;
        len = 0;
    } else if (dbr.status == SUCCESS) {
        outgoing_message.id = USER_ACK;
        server = (struct Server *) dbr.result;
        sprintf(buffer, "%s:%d", server->ip_address, server->port);
        fprintf(stderr, "in request_user, buffer is  %s\n", buffer);
        len = strlen(buffer);
        outgoing_message.length = htonl(len);
    } else {
        fprintf(stderr, "Error accessing database in function request_user\n");
        len = 0;
    }

    fprintf(stderr, "in request_user, length is %d\n", len);
    n = write(sockfd, (char *) &outgoing_message, HEADER_LENGTH);
    if (n < HEADER_LENGTH){
        fprintf(stderr, "Error writing to socket in function request_user\n" );
        return DISCONNECTED;
    }

    if (outgoing_message.id == USER_ACK)
        n = write_message(sockfd, buffer, len);

    return DISCONNECTED;
}

int new_server(struct Header *h, int sockfd, int personal) {
    db_t db;
    enum DB_STATUS dbs;
    struct Server server;
    int n;
    struct Header outgoing_message;
    bzero(&outgoing_message, HEADER_LENGTH);
    char  *token;
    char buffer[512];
    bzero(buffer, 512);

    db = connect_to_db(DB_OWNER, DB_NAME);
    strcpy(server.name, h->source);

    n = read(sockfd, buffer, h->length);
    if (n < (int) h->length)
        return 1;
    token = strtok(buffer, ":");
    strcpy(server.ip_address, token);
    token = strtok(NULL, "");
    server.port = atoi(token);

    fprintf(stderr, "fqdn of new_server is %s\nportno of new_server is %d\n",
            server.ip_address, server.port);

    dbs = add_server(db, &server, personal);
    if (dbs == ELEMENT_ALREADY_EXISTS) {
        close_db_connection(db);
        outgoing_message.id = ERROR_SERVER_EXISTS;
        outgoing_message.length = 0;
    } else if (dbs == SUCCESS) {
        close_db_connection(db);
        outgoing_message.id = NEW_SERVER_ACK;
    } else {
        printf("failed: %d\n", dbs);
        return DISCONNECTED;
    }
    printf("sending msg to server\n");

    strcpy(outgoing_message.source, OPERATOR_SOURCE);
    n = write_message(sockfd, (char *) &outgoing_message, HEADER_LENGTH);

    return DISCONNECTED;
}

int user_list(int sockfd) {
    int length;
    struct Header reply_header;
    char *user_list;

    length = get_user_list_wrapper(&user_list);

    bzero(&reply_header, HEADER_LENGTH);
    reply_header.id = USER_LIST_ACK;
    strcpy(reply_header.source, OPERATOR_SOURCE);
    reply_header.length = htonl(length);

    write_message(sockfd, (char *) &reply_header, HEADER_LENGTH);
    printf("user list len: %d\n", length);
    if (length != 0) {
        write_message(sockfd, user_list, length);
        free(user_list);
    }

    return DISCONNECTED;
}

int new_personal_server(struct Header *h, int sockfd) {
    int status;
    db_t db;
    struct db_return dbr;
    enum DB_STATUS dbs;
    struct Server *server;

    status = new_server(h, sockfd, 1);
    if (status != DISCONNECTED)
        return status;

    db = connect_to_db(DB_OWNER, DB_NAME);

    dbr = get_server_from_client(db, h->source);
    if (dbr.status == SUCCESS) {
        fprintf(stderr, "ERROR: client already exists\n");
        free(dbr.result);
        close_db_connection(db);
        return send_client_exists_ack(sockfd, h->source);
    }

    dbr = get_server_from_name(db, h->source);
    if (dbr.status != SUCCESS) {
        fprintf(stderr, "ERROR: server not made");
        close_db_connection(db);
        return DISCONNECTED;
    }
    server = (struct Server *) dbr.result;

    dbs = add_cspair(db, h->source, server, 1);
    free(server);
    if (dbs != SUCCESS) {
        fprintf(stderr, "Error: failed to add to cspair\n" );
        close_db_connection(db);
        return DISCONNECTED;
    }

    dbr = least_populated_server(db);
    if (dbr.status != SUCCESS) {
        fprintf(stderr, "dbr status was not success\n");
        close_db_connection(db);
        return DISCONNECTED;
    }
    server = (struct Server *) dbr.result;

    dbs = add_backup_cspair(db, h->source, server);
    free(server);
    close_db_connection(db);
    if (dbs!= SUCCESS) {
        fprintf(stderr, "Error: failed to add backup server info\n");
        return DISCONNECTED;
    }

    return send_new_personal_server_ack(sockfd);
}
