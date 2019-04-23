#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netdb.h>
#include "io.h"
#include "db_wrapper.h"
#include "database/db.h"
#include "database/cspairs.h"
#include "partial_message_handler.h"

#define RW_LENGTH 10000
#define NEW_CLIENT_ARG_COUNT 4
#define UPLOAD_FILE_ARG_COUNT 5
#define REQUEST_FILE_ARG_COUNT 6
#define UPDATE_FILE_ARG_COUNT 6
#define REQUEST_TYPE_ARG 1
#define OPERATOR_FQDN_ARG 2
#define OPERATOR_PORT_ARG 3
#define USERNAME_ARG 2
#define PASSWORD_ARG 3
#define UPLOAD_FNAME_ARG 4
#define OWNER_ARG 4
#define REQUEST_FNAME_ARG 5
#define UPDATE_FNAME_ARG 5

struct Server *get_operator_address(char **argv, db_t *db);
void read_new_client_ack_payload(int sockfd, struct Header *message_header,
                                 char *client, db_t *db);
char *read_error_client_exists_payload(int sockfd,
                                       struct Header *message_header);
int check_input_get_msg_id(int argc, char **argv);
int parse_and_send_request(const enum message_type message_id, char **argv,
                           struct Server *operator, db_t *db);
int send_new_client_request(char **argv, struct Server *operator);
int send_upload_file_request(char **argv, struct Server *operator, db_t *db);
int send_request_file_request(char **argv, struct Server *operator, db_t *db);
int send_checkout_file_request(char **argv, struct Server *operator, db_t *db);
int send_update_file_request(char **argv, struct Server *operator, db_t *db);
int process_reply(int sockfd, const enum message_type message_id, char **argv,
                  db_t *db);
struct Server *send_recv_user_req(int sockfd, char *user, char *password,
                                  char *file_owner);
char *make_full_fname(char* owner, char *fname);

int main(int argc, char **argv) {
    int sockfd;
    enum message_type message_id;
    struct Server *operator;
    db_t *db = connect_to_db_wrapper();

    if (argc < 2) {
       fprintf(stderr, "usage: %s [request-type] [request-params...]\n", argv[0]);
       exit(0);
    }

    if (strcmp(argv[REQUEST_TYPE_ARG], "init") == 0) {
        if (argc != 4) {
            fprintf(stderr, "usage: %s init [router-FQDN] [router-portno]", argv[0]);
            exit(0);
        }
        add_cspair_wrapper(db, OPERATOR_SOURCE, argv[OPERATOR_FQDN_ARG],
                           atoi(argv[OPERATOR_PORT_ARG]), "main()", 0);
    } else {
        operator = get_operator_address(argv, db);
        message_id = check_input_get_msg_id(argc, argv);
        sockfd = parse_and_send_request(message_id, argv, operator, db);
        free(operator);
        if (sockfd == -1) {
            return 1;
        }
        process_reply(sockfd, message_id, argv, db);

        close(sockfd);
    }
    return 0;
}

struct Server *get_operator_address(char **argv, db_t *db) {
    struct Server *operator;
    operator = get_server_from_client_wrapper(db, OPERATOR_SOURCE,
                                              "get_operator_address()");
    if (operator == NULL) {
        fprintf(stderr, "operator not in the client/server pairs \
                table, please run\n\t%s init [OPERATOR-FQDN] \
                [OPERATOR-PORT]\n", argv[0]);
        exit(0);
    }

    return operator;
}

int check_input_get_msg_id(int argc, char **argv) {
    if (strcmp(argv[REQUEST_TYPE_ARG], "new_client") == 0) {
        if (argc != NEW_CLIENT_ARG_COUNT) {
            fprintf(stderr, "usage: %s new_client [username] [password]\n",
                    argv[0]);
            exit(0);
        }

        return NEW_CLIENT;
    } else if (strcmp(argv[REQUEST_TYPE_ARG], "upload_file") == 0) {
        if (argc != UPLOAD_FILE_ARG_COUNT) {
            fprintf(stderr, "usage: %s upload_file [username] [password] \
                    [filename]\n", argv[0]);
            exit(0);
        }

        return UPLOAD_FILE;
    } else if (strcmp(argv[REQUEST_TYPE_ARG], "request_file") == 0) {
        if (argc != REQUEST_FILE_ARG_COUNT) {
            fprintf(stderr, "usage: %s request_file [username] [password] \
                    [owner-username] [filename]\n", argv[0]);
            exit(0);
        }

        return REQUEST_FILE;


    }  else if (strcmp(argv[REQUEST_TYPE_ARG], "checkout_file") == 0) {
        if (argc != REQUEST_FILE_ARG_COUNT) {
            fprintf(stderr, "usage: %s checkout_file [username] [password] \
                    [owner-username] [filename]\n", argv[0]);
            exit(0);
        }

        return CHECKOUT_FILE;

    } else if (strcmp(argv[REQUEST_TYPE_ARG], "update_file") == 0) {
        if (argc != UPDATE_FILE_ARG_COUNT) {
            fprintf(stderr, "usage: %s update_file [username] [password] \
                    [owner-username] [filename]\n", argv[0]);
            exit(0);
        }

        return UPDATE_FILE;
    // } else if (strcmp(argv[REQUEST_TYPE_ARG], "request_user_list") == 0) {
    //     return REQUEST_USER_LIST;
    } else {
        fprintf(stderr, "unknown message type received: %s\n",
                argv[REQUEST_TYPE_ARG]);
        fprintf(stderr, "accepted message types are:\ninit\nnew_client\n\
                upload_file\nrequest_file\nupdate_file\n");
        exit(0);
    }
}

/*
 * parses input params from client using check_input_get_msg_id()
 * crafts requests for the operator and target server and writes the request
 * header and payload to the socket.
 *
 * It is a failure condition for the named file to not exist for the
 * UPLOAD_FILE case.
 *
 * returns the socket file descriptor for the opened socket. Depending on
 * the request type, this socket will be to EITHER the operator or server
 * so it's critical that process request also check the message_id before
 * using the socket.
 */
int parse_and_send_request(const enum message_type message_id, char **argv,
                           struct Server *operator, db_t *db) {
    int sockfd;

    switch (message_id) {
        case NEW_CLIENT:
            sockfd = send_new_client_request(argv, operator);
            break;
        case UPLOAD_FILE:
            sockfd = send_upload_file_request(argv, operator, db);
            break;
        case REQUEST_FILE:
            sockfd = send_request_file_request(argv, operator, db);
            break;
        case CHECKOUT_FILE:
            sockfd = send_checkout_file_request(argv, operator, db);
            break;


        case UPDATE_FILE:
            sockfd = send_update_file_request(argv, operator, db);
            break;
        default:
            fprintf(stderr, "bad message type %d >:O\n", message_id);
            exit(1);
    }
    return sockfd;
}

int send_new_client_request(char **argv, struct Server *operator) {
    int sockfd;
    struct Header message_header;

    sockfd = connect_to_server(operator->domain_name, operator->port);

    bzero(&message_header, sizeof(message_header));
    message_header.id = NEW_CLIENT;
    strcpy(message_header.source, argv[USERNAME_ARG]);
    strcpy(message_header.password, argv[PASSWORD_ARG]);

    write_message(sockfd, (char *) &message_header, HEADER_LENGTH);
    return sockfd;
}

int send_upload_file_request(char **argv, struct Server *operator, db_t *db) {
    int sockfd;
    struct Header message_header;
    struct Server *server;
    struct stat sb;
    char *full_filename = NULL;

    if (stat(argv[UPLOAD_FNAME_ARG], &sb) == -1) {
        fprintf(stderr, "Named file does not exist, exiting\n");
        exit(1);
    }

    server = get_server_from_client_wrapper(db, argv[USERNAME_ARG],
                                            "send_upload_file_request()");
    if (server == NULL) { /* no client/server pairing on client */
        sockfd = connect_to_server(operator->domain_name, operator->port);
        /* get server for self from operator */
        server = send_recv_user_req(sockfd, argv[USERNAME_ARG],
                                            argv[PASSWORD_ARG],
                                            argv[USERNAME_ARG]);
        if (server == NULL) { /* no client/server pairing on operator */
            fprintf(stderr, "Server for user %s could not be resolved, \
                    exiting\n", argv[USERNAME_ARG]);
            exit(1);
        }
        add_cspair_wrapper(db, argv[USERNAME_ARG], server->domain_name,
                           server->port, "send_upload_file_request()", 0);
        close(sockfd);
    }

    /* upload the file */
    sockfd = connect_to_server(server->domain_name, server->port);
    bzero(&message_header, sizeof(message_header));
    message_header.id = UPLOAD_FILE;
    strcpy(message_header.source, argv[USERNAME_ARG]);
    strcpy(message_header.password, argv[PASSWORD_ARG]);
    full_filename = make_full_fname(argv[USERNAME_ARG], argv[UPLOAD_FNAME_ARG]);
    strcpy(message_header.filename, full_filename);
    message_header.length = htonl(sb.st_size);
    write_message(sockfd, (char *) &message_header, HEADER_LENGTH);
    write_file(sockfd, argv[UPLOAD_FNAME_ARG]);

    free(server);
    free(full_filename);
    return sockfd;
}

int send_checkout_file_request(char **argv, struct Server *operator, db_t *db) {
    int sockfd;
    struct Header message_header;
    struct Server *server;
    char *full_filename = NULL;

    server = get_server_from_client_wrapper(db, argv[OWNER_ARG],
                                            "send_checkout_file_request()");
    if (server == NULL) { /* no client/server pairing on client */
        sockfd = connect_to_server(operator->domain_name, operator->port);
        /* get server for file owner from operator */
        server = send_recv_user_req(sockfd, argv[USERNAME_ARG],
                                            argv[PASSWORD_ARG],
                                            argv[OWNER_ARG]);
        if (server == NULL) { /* no client/server pairing on operator */
            fprintf(stderr, "Server for user %s could not be resolved, \
                    exiting\n", argv[OWNER_ARG]);
            exit(1);
        }
        add_cspair_wrapper(db, argv[OWNER_ARG], server->domain_name,
                           server->port,
                           "send_checkout_file_request()", 0);
        close(sockfd);
    }

    /* request the file */
    sockfd = connect_to_server(server->domain_name, server->port);
    bzero(&message_header, sizeof(message_header));
    message_header.id = CHECKOUT_FILE;
    strcpy(message_header.source, argv[USERNAME_ARG]);
    strcpy(message_header.password, argv[PASSWORD_ARG]);
    full_filename = make_full_fname(argv[OWNER_ARG],
                                    argv[REQUEST_FNAME_ARG]);
    strcpy(message_header.filename, full_filename);
    write_message(sockfd, (char *) &message_header, HEADER_LENGTH);

    free(server);
    free(full_filename);
    return sockfd;
}


int send_request_file_request(char **argv, struct Server *operator, db_t *db) {
    int sockfd;
    struct Header message_header;
    struct Server *server;
    char *full_filename = NULL;

    server = get_server_from_client_wrapper(db, argv[OWNER_ARG],
                                            "send_request_file_request()");
    if (server == NULL) { /* no client/server pairing on client */
        sockfd = connect_to_server(operator->domain_name, operator->port);
        /* get server for file owner from operator */
        server = send_recv_user_req(sockfd, argv[USERNAME_ARG],
                                            argv[PASSWORD_ARG],
                                            argv[OWNER_ARG]);
        if (server == NULL) { /* no client/server pairing on operator */
            fprintf(stderr, "Server for user %s could not be resolved, \
                    exiting\n", argv[OWNER_ARG]);
            exit(1);
        }
        add_cspair_wrapper(db, argv[OWNER_ARG], server->domain_name,
                           server->port,
                           "send_request_file_request()", 0);
        close(sockfd);
    }

    /* request the file */
    sockfd = connect_to_server(server->domain_name, server->port);
    bzero(&message_header, sizeof(message_header));
    message_header.id = REQUEST_FILE;
    strcpy(message_header.source, argv[USERNAME_ARG]);
    strcpy(message_header.password, argv[PASSWORD_ARG]);
    full_filename = make_full_fname(argv[OWNER_ARG],
                                    argv[REQUEST_FNAME_ARG]);
    strcpy(message_header.filename, full_filename);
    write_message(sockfd, (char *) &message_header, HEADER_LENGTH);

    free(server);
    free(full_filename);
    return sockfd;
}

int send_update_file_request(char **argv, struct Server *operator, db_t *db) {
    int sockfd;
    struct Header message_header;
    struct Server *server;
    struct stat sb;
    char *full_filename = NULL;

    if (stat(argv[UPDATE_FNAME_ARG], &sb) == -1) {
        fprintf(stderr, "Named file does not exist, exiting\n");
        exit(1);
    }
    server = get_server_from_client_wrapper(db, argv[OWNER_ARG],
                                            "send_update_file_request()");
    if (server == NULL) { /* no client/server pairing on client */
        sockfd = connect_to_server(operator->domain_name, operator->port);
        /* get server for owner from operator */
        server = send_recv_user_req(sockfd, argv[USERNAME_ARG],
                                            argv[PASSWORD_ARG],
                                            argv[OWNER_ARG]);
        if (server == NULL) { /* no client/server pairing on operator */
            fprintf(stderr, "Server for user %s could not be resolved, \
                    exiting\n", argv[USERNAME_ARG]);
            exit(1);
        }
        add_cspair_wrapper(db, argv[OWNER_ARG], server->domain_name,
                           server->port, "send_update_file_request()", 0);
        close(sockfd);
    }

    /* upload the file */
    sockfd = connect_to_server(server->domain_name, server->port);
    bzero(&message_header, sizeof(message_header));
    message_header.id = UPDATE_FILE;
    strcpy(message_header.source, argv[USERNAME_ARG]);
    strcpy(message_header.password, argv[PASSWORD_ARG]);
    full_filename = make_full_fname(argv[OWNER_ARG],
                                    argv[UPDATE_FNAME_ARG]);
    strcpy(message_header.filename, full_filename);
    message_header.length = htonl(sb.st_size);
    write_message(sockfd, (char *) &message_header, HEADER_LENGTH);
    write_file(sockfd, argv[UPDATE_FNAME_ARG]);

    free(server);
    free(full_filename);
    return sockfd;
}

int process_reply(int sockfd, const enum message_type message_id, char **argv,
        db_t *db)
{
    int n, m;
    char *clientbuf;
    char header_buffer[HEADER_LENGTH];
    struct Header message_header;
    char file_buffer[RW_LENGTH];

    n = 0;
    while (n < HEADER_LENGTH) {
        m = read(sockfd, &header_buffer[n], HEADER_LENGTH - n);
        if (m < 0) {
            error("ERROR reading from socket");
        }
        n += m;
    }
    memcpy(&message_header, header_buffer, HEADER_LENGTH);
    message_header.length = ntohl(message_header.length);

    switch (message_id) {
        case NEW_CLIENT:
            if (message_header.id == NEW_CLIENT_ACK) {
                read_new_client_ack_payload(sockfd, &message_header, argv[USERNAME_ARG],
                                            db);
                fprintf(stderr, "Client %s successfully added\n", argv[USERNAME_ARG]);
            } else if (message_header.id == ERROR_CLIENT_EXISTS) {
                clientbuf = read_error_client_exists_payload(sockfd,
                                                             &message_header);
                fprintf(stderr, "Client %s already exists\n", clientbuf);
                free(clientbuf);
            } else {
                fprintf(stderr, "Bad response type %d received from router",
                        message_header.id);
            }

            break;
        case UPLOAD_FILE:
            if (message_header.id == UPLOAD_ACK) {
                fprintf(stderr,"File %s successfully uploaded\n",
                       message_header.filename);
            } else if (message_header.id == ERROR_FILE_EXISTS) {
                fprintf(stderr,"File %s already exists\n", message_header.filename);
            } else if (message_header.id == ERROR_UPLOAD_FAILURE) {
                fprintf(stderr,"File %s failed to upload\n", message_header.filename);
            }

            break;
        case UPDATE_FILE:
            if (message_header.id == UPDATE_ACK)
                fprintf(stderr,"File %s successfully updated\n",
                       message_header.filename);
            else if (message_header.id == ERROR_FILE_DOES_NOT_EXIST)
                fprintf(stderr,"File %s already exists\n", message_header.filename);
            else if (message_header.id == ERROR_BAD_PERMISSIONS)
                fprintf(stderr,"Invalid permissions for %s\n",
                       message_header.filename);
            break;

        case CHECKOUT_FILE:
                if (message_header.id == RETURN_CHECKEDOUT_FILE){
                    do{
                        m = read(sockfd, file_buffer, RW_LENGTH);
                        if (m <= 0) {
                            error("ERROR reading from socket");
                        }
                    } while(save_buffer(message_header.filename, file_buffer, m,
                                        message_header.length) == 0);
                    fprintf(stderr, "successfully checked out file\n" );
                    break;
                } else if (message_header.id == RETURN_READ_ONLY_FILE)
                    fprintf(stderr, "someone has already checked out this \
                                     file, you are downloading a read-only \
                                     copy\n");

                //NATHAN ALLEN DO NOT PUT A BREAK STATEMENT HERE
        case REQUEST_FILE:
            if (message_header.id == ERROR_FILE_DOES_NOT_EXIST)
               fprintf(stderr,"File %s does not exist on server\n", message_header.filename);
            else if (message_header.id == RETURN_READ_ONLY_FILE){
                do{
                    m = read(sockfd, file_buffer, RW_LENGTH);
                    if (m <= 0) {
                        error("ERROR reading from socket");
                    }
                } while(save_buffer(message_header.filename, file_buffer, m,
                                    message_header.length) == 0);
                fprintf(stderr, "Read only file successfully retrieved\n" );
            }
            else
                fprintf(stderr,"Unknown error when requesting file \n");

            break;
        default:
            break;
    }
    return 0;
}

/*
 * send_recv_user_req() sends a request for the server associated with
 * user file_owner to the operator, which is represented by open socket sockfd.
 *
 * returns a malloc'd Server struct containing the fqdn and port of the
 * file owner that the caller is responsible for freeing
 */
struct Server *send_recv_user_req(int sockfd, char *user, char *password,
                                  char *file_owner) {
    int n, m, length;
    struct Header header;
    char buffer[FQDN_PORT_MAX_LENGTH];
    char *token;
    struct Server *server = malloc(sizeof(struct Server));
    if (server == NULL) {
        error("ERROR allocation failure");
    }

    bzero(&header, sizeof(header));
    header.id = REQUEST_USER;
    strcpy(header.source, user);
    strcpy(header.password, password);
    strcpy(header.filename, file_owner);

    write_message(sockfd, (char *) &header, HEADER_LENGTH);

    bzero(buffer, FQDN_PORT_MAX_LENGTH);
    n = 0;
    while (n < HEADER_LENGTH) {
        m = read(sockfd, &buffer[n], HEADER_LENGTH - n);
        if (m < 0) {
            error("ERROR reading from socket");
        }
        n += m;
    }

    memcpy(&header, buffer, HEADER_LENGTH);
    if (header.id == REQUEST_USER_ACK) {
        length = ntohl(header.length);
        n = 0;
        if (length > FQDN_PORT_MAX_LENGTH){
            fprintf(stderr, "payload %d was too large\n", length );
            error("ERROR payload too large");
        }
        bzero(buffer, FQDN_PORT_MAX_LENGTH);
        while (n < length) {
            m = read(sockfd, &buffer[n], length - n);
            if (m < 0) {
                error("ERROR reading from socket");
            }
            n += m;
        }

        token = strtok(buffer, ":");
        if (token == NULL) {
            fprintf(stderr, "malformed payload in REQUEST_USER_ACK from \
                             operator, exiting\n");
            exit(1);
        }
        strcpy(server->domain_name, token);
        token = strtok(NULL, "");
        if (token == NULL) {
            fprintf(stderr, "malformed payload in REQUEST_USER_ACK from \
                             operator, exiting\n");
            exit(1);
        }
        server->port = atoi(token);

        return server;
    } else if (header.id != ERROR_USER_DOES_NOT_EXIST) {
        fprintf(stderr, "bad response %d received from operator when \
                         requesting user, exiting\n", header.id);
        exit(1);
    } else {
        return NULL;
    }
}

void read_new_client_ack_payload(int sockfd, struct Header *message_header,
                                 char *client, db_t *db) {
    int length = message_header->length;
    int n, m;
    char *buffer = malloc(length + 1);
    char *fqdn, *port;
    if (buffer == NULL) {
        error("Allocation failure");
    }

    n = 0;
    while (n < length) {
        m = read(sockfd, &buffer[n], length - n);
        if (m < 0) {
            error("ERROR reading from socket");
        }
        n += m;
    }
    buffer[length] = '\0';

    fqdn = strtok(buffer, ":");
    if (fqdn == NULL) {
        fprintf(stderr, "new_client_ack_payload empty, exiting\n");
        exit(1);
    }
    if (strlen(fqdn) > SERVER_NAME_MAX_LENGTH) {
        fprintf(stderr, "server name %s too long, exiting\n", fqdn);
        exit(1);
    }
    port = strtok(NULL, "");
    if (port == NULL) {
        fprintf(stderr, "new_client_ack_payload, no port number in payload\n");
        exit(1);
    }
    add_cspair_wrapper(db, client, fqdn, atoi(port),
                       "read_new_client_ack_payload()", 0);
    free(buffer);
}


char *read_error_client_exists_payload(int sockfd,
                                       struct Header *message_header) {
    int n, m;
    int length = message_header->length;
    char *client_name = malloc(length + 1);
    if (client_name == NULL) {
        error("Allocation failure");
    }

    n = 0;
    while (n < length) {
        m = read(sockfd, &client_name[n], length - n);
        if (m < 0) {
            error("ERROR reading from socket");
        }
        n += m;
    }
    client_name[length] = '\0';

    return client_name;
}

/* Purpose: creates a filename in format "file-owner/filename" so as to be
 * interpretable by the server
 * Takes the owner name and fname as cstrings as arguments
 * Returns the malloced full filename
 */
char *make_full_fname(char* owner, char *fname) {
    int len_owner = strlen(owner);
    int len_fname = strlen(fname);
    //2 extra bytes for the "/" and the "\0"
    char *full_fname = calloc(len_owner + len_fname + 2, 1);
    memcpy(full_fname, owner, len_owner);
    full_fname[len_owner] = '/';
    memcpy((&full_fname[len_owner + 1]), fname, len_fname);
    return full_fname;
}
