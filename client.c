#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netdb.h>
#include "io.h"
#include "database/db.h"
#include "database/cspairs.h"
#include "partial_message_handler.h"

#define RW_LENGTH 10000
#define DISCONNECTED 0
#define MAX_MSG_SIZE 450
#define DB_OWNER "nathan"
#define DB_NAME "fileshare"
#define USE_DB 1
#define CSPAIRS_FNAME "client_cspairs.txt"
#define CSPAIRS_FILE_MAX_LENGTH 10000
#define REQUEST_TYPE_ARG 1
#define OPERATOR_FQDN_ARG 2
#define OPERATOR_PORT_ARG 3
#define USERNAME_ARG 4
#define PASSWORD_ARG 5
#define UPLOAD_FNAME_ARG 6
#define OWNER_ARG 6
#define REQUEST_FNAME_ARG 7
#define UPDATE_FNAME_ARG 7

struct Server {
    uint16_t port;
    char domain_name[64];
};

void read_new_client_ack_payload(int sockfd, struct Header *message_header,
                                 char *client, db_t *db);
char *read_error_client_exists_payload(int sockfd,
                                       struct Header *message_header);
int check_input_get_msg_id(int argc, char **argv);
int parse_and_send_request(const enum message_type message_id, char **argv,
                           db_t *db);
void add_cspair_wrapper(db_t *db, char *client, char *fqdn, unsigned port,
                        char *loc);
struct Server * get_server_from_client_wrapper(db_t *db, char *client,
                                               char *loc);
int process_reply(int sockfd, const enum message_type message_id, char **argv,
                  db_t *db);
struct Server *send_recv_user_req(int sockfd, char *user, char *password,
                                  char *file_owner);
char *make_full_fname(char* owner, char *fname);

int main(int argc, char **argv) {
    int sockfd;
    enum message_type message_id;
    db_t *db;

    if (USE_DB) {
        db = connect_to_db(DB_OWNER, DB_NAME);
    } else {
        db = NULL;
    }
    message_id = check_input_get_msg_id(argc, argv);
    sockfd = parse_and_send_request(message_id, argv, db);
    if (sockfd == -1) {
        return 1;
    }
    process_reply(sockfd, message_id, argv, db);

    close(sockfd);
    return 0;
}

int check_input_get_msg_id(int argc, char **argv) {
    if (argc < 2) {
       fprintf(stderr, "usage: %s [request-type] [request-params...]\n", argv[0]);
       exit(0);
    }

    if (strcmp(argv[1], "new_client") == 0) {
        if (argc != 6) {
            fprintf(stderr, "usage: %s new_client [router-FQDN] [router-portno] \
                             [username] [password]\n", argv[0]);
            exit(0);
        }

        return NEW_CLIENT;
    } else if (strcmp(argv[1], "upload_file") == 0) {
        if (argc != 7) {
            fprintf(stderr, "usage: %s upload_file [router-FQDN] [router-portno] \
                             [username] [password] [filename]\n", argv[0]);
        }

        return UPLOAD_FILE;
    } else if (strcmp(argv[1], "request_file") == 0) {
        if (argc != 8) {
            fprintf(stderr, "usage: %s request_file [router-FQDN] [router-portno] \
                             [username] [password] [owner-username] [filename]\n",
                             argv[0]);
        }

        return REQUEST_FILE;
    } else if (strcmp(argv[1], "update_file") == 0) {
        if (argc != 8) {
            fprintf(stderr, "usage: %s update_file [router-FQDN] [router-portno] \
                             [username] [password] [owner-username] [filename]\n",
                             argv[0]);
        }

        return UPDATE_FILE;
    }

    return 0;
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
                           db_t *db) {
    int sockfd;
    struct stat sb;
    struct Header message_header;
    struct Server *server;
    char *full_filename = NULL;

    bzero(&message_header, sizeof(message_header));
    strcpy(message_header.source, argv[USERNAME_ARG]);
    strcpy(message_header.password, argv[PASSWORD_ARG]);

    switch (message_id) {
        case NEW_CLIENT:
            sockfd = connect_to_server(argv[OPERATOR_FQDN_ARG],
                                       atoi(argv[OPERATOR_PORT_ARG]));
            message_header.id = NEW_CLIENT;
            write_message(sockfd, (char *) &message_header, HEADER_LENGTH);
            break;
        case UPLOAD_FILE:
            if (stat(argv[UPLOAD_FNAME_ARG], &sb) == -1) {
                fprintf(stderr, "Named file does not exist, exiting\n");
                exit(1);
            }
            server = get_server_from_client_wrapper(db, argv[USERNAME_ARG],
                                     "parse_and_send_request() - UPLOAD_FILE");
            if (server == NULL) { /* no client/server pairing on client */
                sockfd = connect_to_server(argv[OPERATOR_FQDN_ARG],
                                           atoi(argv[OPERATOR_PORT_ARG]));
                /* get server for self from operator */
                server = send_recv_user_req(sockfd, argv[USERNAME_ARG],
                                            argv[PASSWORD_ARG],
                                            argv[USERNAME_ARG]);
                add_cspair_wrapper(db, argv[USERNAME_ARG], server->domain_name,
                                   server->port,
                                   "parse_and_send_request() - UPLOAD_FILE");
                close(sockfd);
            }

            if (server == NULL) { /* no client/server pairing on operator */
                fprintf(stderr, "Server for user %s could not be resolved, \
                                 exiting\n", argv[USERNAME_ARG]);
                exit(1);
            } else { /* upload the file */
                sockfd = connect_to_server(server->domain_name, server->port);
                message_header.id = UPLOAD_FILE;
                full_filename = make_full_fname(argv[USERNAME_ARG],
                                                argv[UPLOAD_FNAME_ARG]);
                strcpy(message_header.filename, full_filename);
                message_header.length = htonl(sb.st_size);
                write_message(sockfd, (char *) &message_header, HEADER_LENGTH);
                write_file(sockfd, argv[UPLOAD_FNAME_ARG]);
            }

            free(server);
            break;
        case REQUEST_FILE:
            server = get_server_from_client_wrapper(db, argv[OWNER_ARG],
                                    "parse_and_send_request() - REQUEST_FILE");
            if (server == NULL) { /* no client/server pairing on client */
                sockfd = connect_to_server(argv[OPERATOR_FQDN_ARG],
                                           atoi(argv[OPERATOR_PORT_ARG]));
                /* get server for file owner from operator */
                server = send_recv_user_req(sockfd, argv[USERNAME_ARG],
                                            argv[PASSWORD_ARG], argv[OWNER_ARG]);
                if (server == NULL) { /* no client/server pairing on operator */
                    fprintf(stderr, "Server for user %s could not be resolved, \
                                     exiting\n", argv[OWNER_ARG]);
                    exit(1);
                }
                add_cspair_wrapper(db, argv[OWNER_ARG], server->domain_name,
                                   server->port,
                                   "parse_and_send_request() - REQUEST_FILE");
                close(sockfd);
            } else { /* request the file */
                sockfd = connect_to_server(server->domain_name, server->port);
                message_header.id = REQUEST_FILE;
                full_filename = make_full_fname(argv[OWNER_ARG],
                                                argv[REQUEST_FNAME_ARG]);
                strcpy(message_header.filename, full_filename);
                write_message(sockfd, (char *) &message_header, HEADER_LENGTH);
            }

            free(server);
            break;
        case UPDATE_FILE:
            if (stat(argv[UPDATE_FNAME_ARG], &sb) == -1) {
                fprintf(stderr, "Named file does not exist, exiting\n");
                exit(1);
            }
            server = get_server_from_client_wrapper(db, argv[OWNER_ARG],
                                     "parse_and_send_request() - UPLOAD_FILE");
            if (server == NULL) { /* no client/server pairing on client */
                sockfd = connect_to_server(argv[OPERATOR_FQDN_ARG],
                                           atoi(argv[OPERATOR_PORT_ARG]));
                /* get server for owner from operator */
                server = send_recv_user_req(sockfd, argv[USERNAME_ARG],
                                            argv[PASSWORD_ARG],
                                            argv[OWNER_ARG]);
                add_cspair_wrapper(db, argv[OWNER_ARG], server->domain_name,
                                   server->port,
                                   "parse_and_send_request() - UPLOAD_FILE");
                close(sockfd);
            }

            if (server == NULL) { /* no client/server pairing on operator */
                fprintf(stderr, "Server for user %s could not be resolved, \
                                 exiting\n", argv[USERNAME_ARG]);
                exit(1);
            } else { /* upload the file */
                sockfd = connect_to_server(server->domain_name, server->port);
                message_header.id = UPDATE_FILE;
                full_filename = make_full_fname(message_header.source,
                                                argv[UPDATE_FNAME_ARG]);
                strcpy(message_header.filename, full_filename);
                message_header.length = htonl(sb.st_size);
                write_message(sockfd, (char *) &message_header, HEADER_LENGTH);
                write_file(sockfd, argv[UPDATE_FNAME_ARG]);
            }

            free(server);
            break;
        default:
            fprintf(stderr, "bad message type %d >:O\n", message_id);
            exit(1);
    }
    free(full_filename);
    return sockfd;
}

int process_reply(int sockfd, const enum message_type message_id, char **argv,
                  db_t *db) {
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
                read_new_client_ack_payload(sockfd, &message_header, argv[4],
                                            db);
                printf("Client %s successfully added\n", argv[4]);
            } else if (message_header.id == ERROR_CLIENT_EXISTS) {
                clientbuf = read_error_client_exists_payload(sockfd,
                                                             &message_header);
                printf("Client %s already exists\n", clientbuf);
                free(clientbuf);
            } else {
                fprintf(stderr, "Bad response type %d received from router",
                        message_header.id);
            }

            break;
        case UPLOAD_FILE:
            if (message_header.id == UPLOAD_ACK) {
                printf("File %s successfully uploaded\n",
                       message_header.filename);
            } else if (message_header.id == ERROR_FILE_EXISTS) {
                printf("File %s already exists\n", message_header.filename);
            } else if (message_header.id == ERROR_UPLOAD_FAILURE) {
                printf("File %s failed to upload\n", message_header.filename);
            }

            break;
        case UPDATE_FILE:
            if (message_header.id == UPDATE_ACK)
                printf("File %s successfully updated\n",
                       message_header.filename);
            else if (message_header.id == ERROR_FILE_DOES_NOT_EXIST)
                printf("File %s already exists\n", message_header.filename);
            else if (message_header.id == ERROR_BAD_PERMISSIONS)
                printf("Invalid permissions for %s\n",
                       message_header.filename);
            break;
        case REQUEST_FILE:

            if (message_header.id == ERROR_FILE_DOES_NOT_EXIST)
               printf("File %s already exists\n", message_header.filename);
            else if (message_header.id == RETURN_FILE){
                do{
                    m = read(sockfd, file_buffer, RW_LENGTH);
                    if (m <= 0) {
                        error("ERROR reading from socket");
                    }
                } while(save_buffer(message_header.filename, file_buffer, m,
                                    message_header.length) == 0);

            }
            else
                printf("Unknown error when requesting file \n");

            break;
        default:
            break;
    }
    return 0;
}

void check_db_status(enum DB_STATUS db_status, char *func) {
    switch (db_status) {
        case CORRUPTED:
            fprintf(stderr, "in %s, db corrupted\n", func);
            exit(1);
        case COMMAND_FAILED:
            fprintf(stderr, "in %s, db command failed\n", func);
            exit(1);
        case INVALID_AUTHENTICATION:
            fprintf(stderr, "in %s, invalid db auth\n", func);
            exit(1);
        default:
            return;
    }
}

void add_cspair_wrapper(db_t *db, char *client, char *fqdn, unsigned port,
                        char *loc) {
    enum DB_STATUS db_status;
    struct server_addr server;
    char portno[6];
    int portno_length;
    FILE *fp;

    if (port > 65536) {
        error("ERROR port no out-of-range\n");
    }

    if (USE_DB) {
        bzero((char *) &server, sizeof(server));
        strcpy(server.domain_name, fqdn);
        server.port = port;

        db_status = add_cspair(db, client, &server);
        check_db_status(db_status, loc);
    } else {
        portno_length = sprintf(portno, "%u", port);
        fp = fopen(CSPAIRS_FNAME, "a+");
        fwrite(client, 1, strlen(client), fp);
        fwrite(":", 1, 1, fp);
        fwrite(fqdn, 1, strlen(fqdn), fp);
        fwrite(":", 1, 1, fp);
        fwrite(portno, 1, portno_length, fp);
        fwrite("\n", 1, 1, fp);
        fclose(fp);
    }
}

/*
 * This function calls get_server_from_client(), or reads from a client/server
 * pairs file, depending on whether USE_DB is set.
 *
 * If the client is not in the pairs list, get_server_from_client_wrapper()
 * returns NULL. Otherwise, it malloc's and returns a Server struct which must
 * be free'd by the caller.
 */
struct Server *get_server_from_client_wrapper(db_t *db, char *client,
                                              char *loc) {
    struct db_return db_return;
    struct server_addr *server;
    struct Server *retval = malloc(sizeof(*retval));
    FILE *fp = NULL;
    char buffer[CSPAIRS_FILE_MAX_LENGTH];
    char *token = NULL;
    int length;

    if (USE_DB) {
        db_return = get_server_from_client(db, client);
        check_db_status(db_return.status, loc);
        if (db_return.result == NULL) {
            free(retval);
            return NULL;
        }

        server = (struct server_addr *) db_return.result;
        retval->port = server->port;
        strcpy(retval->domain_name, server->domain_name);
    } else {
        /* read contents of CSPAIRS file into buffer */
        fp = fopen(CSPAIRS_FNAME, "rb");
        if (fp == NULL) {
            fprintf(stderr, "ERROR opening file %s for reading\n",
                    CSPAIRS_FNAME);
            exit(1);
        }
        fseek(fp, 0, SEEK_END);
        length = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        fread(buffer, 1, length, fp);
        fclose(fp);
        if (length == 0) {
            free(retval);
            return NULL;
        }

        /* parse buffer for desired client */
        token = strtok(buffer, ":\n");
        while (token != NULL && strcmp(token, client) != 0) {
            token = strtok(NULL, ":\n");
        }
        if (token == NULL) {
            return NULL;
        } else {
            token = strtok(NULL, ":\n");
            if (token == NULL) {
                fprintf(stderr, "CSPAIRS file %s badly formed\n",
                        CSPAIRS_FNAME);
                exit(1);
            }
            strcpy(retval->domain_name, token);
            token = strtok(NULL, ":\n");
            if (token == NULL) {
                fprintf(stderr, "CSPAIRS file %s badly formed\n",
                        CSPAIRS_FNAME);
                exit(1);
            }
            retval->port = atoi(token);
        }
    }

    return retval;
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
    struct Server *server = malloc(sizeof(*server));
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
        if (length < FQDN_PORT_MAX_LENGTH)
            error("ERROR payload too large");
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

/* void read_returned_file(int sockfd, struct Header *message_header)
{


} */

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
                       "read_new_client_ack_payload()");
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
