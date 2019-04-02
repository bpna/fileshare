#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include "db.h"
#include "messages.h"

#define DISCONNECTED 0
#define MAX_MSG_SIZE 450

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

void read_new_client_ack_payload(struct Header *message_header, PGconn *db);
void read_error_client_exists_payload(struct Header *message_header);
int parse_request(int argc, char **argv);
int connect_to_server(char *fqdn, int portno);
int write_message(int sockfd, char *data, int length);
int write_file(int csock, char *filename);

int main(int argc, char **argv)
{
    int sockfd, portno, n;
    char header_buffer[HEADER_LENGTH];
    char *clientbuf;
    enum message_type message_id;
    enum DB_STATUS db_status;
    struct Header message_header;
    struct db_return db_return;
    struct server_addr server;

    /* create database */
    PGconn *db = connect_to_db();

    /* send the message request to the router or file server */
    message_id = parse_request(argc, argv);
    switch (message_id) {
        case NEW_CLIENT:
            sockfd = connect_to_server(argv[2], atoi(argv[3]));
            bzero(&message_header, sizeof(message_header));
            message_header.id = NEW_CLIENT;
            strcpy(message_header.source, argv[4]);
            strcpy(message_header.password, argv[5]);
            write_message(sockfd, (char *) &message_header, HEADER_LENGTH);
            break;
        case UPLOAD_FILE:
            sockfd = connect_to_server(argv[2], atoi(argv[3]));
            bzero(&message_header, sizeof(message_header));
            message_header.id = UPLOAD_FILE;
            strcpy(message_header.source, argv[4]);
            strcpy(message_header.password, argv[5]);
            strcpy(message_header.filename, argv[6]);
            write_message(sockfd, (char *) &message_header, HEADER_LENGTH);
            write_file(sockfd, message_header.filename);
            break;
        case REQUEST_FILE:
            db_return = get_server_from_client(db, argv[6]);
            check_db_status(db_return->status, "main() - REQUEST_FILE");
            server = db_return->result;
            if (server = NULL) {
                sockfd = connect_to_server(argv[2], argv[3]);
                send_user_request(sockfd);

            

        default:
            fprintf(stderr, "bad message type %d >:O\n", message_id);
            return 1;
    }

    /* read reply from the server */
    switch (message_id) {
        case NEW_CLIENT:
            n = 0;
            while (n < HEADER_LENGTH) {
                n += read(sockfd, &header_buffer[n], HEADER_LENGTH - n);
            }
            memcpy(message_header, header_buffer, HEADER_LENGTH);
            message_header->length = htonl(message_header->length);
            if (message_header->id == NEW_CLIENT_ACK) {
                read_new_client_ack_payload(&message_header, char *client, db);
                printf("Client %s successfully added\n", argv[4]);
            } else if (message_header->id == ERROR_CLIENT_EXISTS) {
                clientbuf = read_error_client_exists_payload(&message_header);
                printf("Client %s already exists\n", clientbuf);
                free(clientbuf);
            } else {
                fprintf(stderr, "Bad response type %d received from router",
                        message_header->id);
            }
            break;
        case UPLOAD_FILE:
            n = 0;
            while (n < HEADER_LENGTH) {
                n += read(sockfd, &header_buffer[n], HEADER_LENGTH - n);
            }
            memcpy(message_header, header_buffer, HEADER_LENGTH);
            message_header->length = htonl(message_header->length);
            if (message_header->id == UPLOAD_ACK) {
                printf("File %s successfully uploaded\n", 
                       message_header->filename);
            } else if (message_header->id == ERROR_FILE_EXISTS) {
                printf("File %s already exists\n", message_header->filename);
            } else if (message_header->id == ERROR_UPLOAD_FAILURE) {
                printf("File %s failed to upload\n", message_header->filename);
            }
            break;
    }

    close(sockfd);
    return 0;
}

static int freshvar()
{
    static int x = 0;
    x++;

    return x;
}

void check_db_status(enum db_status, char *func)
{
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
    }
}

void send_user_request(int sockfd)
{
    struct Header h;
}

/* void read_returned_file(int sockfd, struct Header *message_header)
{


} */

void read_new_client_ack_payload(int sockfd, struct Header *message_header, 
                                 char *client, PGconn *db)
{
    int length = message_header->length;
    int n = 0;
    int portno;
    enum DB_STATUS db_status;
    struct server_addr server;
    char *fqdn, portchar;
    char *buffer = malloc(length + 1);
    if (buffer == NULL) {
        error("Allocation failure");
    }

    while (n < length) {
        n += read(sockfd, &buffer[n], length - n);
    }
    buffer[length] = '\0';
    
    bzero((char *) &serv_addr, sizeof(serv_addr));
    fqdn = strtok(buffer, ":");
    portchar = strtok(NULL, ":");
    portno = atoi(portchar);
    strcpy(server->domain_name, fqdn);
    server->port = portno;
    server->id = freshvar();

    db_status = add_cspair(db, client, &server);
    check_db_status(db_status, "read_new_client_ack_payload()");
    free(buffer);
}


char * read_error_client_exists_payload(struct Header *message_header)
{
    int length = message_header->length;
    char *client_name = malloc(length + 1);
    if (client_name == NULL) {
        error("Allocation failure");
    }

    while (n < length) {
        n += read(sockfd, &client_name[n], length - n);
    }
    client_name[length] = '\0';
    
    return client_name;
}


int parse_request(int argc, char **argv)
{
    if (argc < 2) {
       fprintf(stderr, "usage: %s [request-type] [request-params...]\n", argv[0]);
       exit(0);
    }

    if (strcmp(argv[1], "new_client") == 0) {
        if (argc != 5) {
            fprintf(stderr, "usage: %s new_client [router-FQDN] [router-portno] \
                             [username] [password]\n", argv[0]);
            exit(0);
        }
    } else if (strcmp(argv[1], "upload_file") == 0) {
        if (argc != 7) {
            fprintf(stderr, "usage: %s upload_file [server-FQDN] \
                             [username] [password] \
                             [filename]\n", argv[0]);          
        }

        return UPLOAD_FILE;
    } else if (strcmp(argv[1], "request_file") == 0) {
        if (argc != 8) {
            fprintf(stderr, "usage: %s request_file [router-FQDN] [router-portno] \
                             [username] [password] [owner-username] [filename]\n",
                             argv[0]);
        }

        return REQUEST_FILE;
    }

    return 0;
}

int connect_to_server(char *fqdn, int portno)
{
    struct hostent *server;
    struct sockaddr_in serv_addr;

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        error("ERROR opening socket");
    }

    printf("server arg is %s\n", fqdn);
    server = gethostbyname(fqdn);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(portno);
    if (connect(sockfd,(struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        error("ERROR connecting");
    }
 
    return sockfd;
}

int write_file(int csock, char *filename)
{
    FILE *fp = fopen(filename, "rb");
    char bytes[10000];
    long filelen;
    int to_write, bytes_written = 0;

    fseek(fp, 0, SEEK_END);
    filelen = ftell(fp);
    rewind(fp);

    while (bytes_written < filelen) {
        if (filelen - bytes_written < 10000) {
            to_write = filelen - bytes_written;
        } else {
            to_write = 10000;
        }

        fread(bytes, to_write, 1, fp);
        write_message(csock, bytes, to_write);
        bytes_written += to_write;
    }

    return 0;
}

int write_message(int csock, char *data, int length)
{
    int n = 0;

    while (n < length) {
        n += write(csock, &data[n], length - n);
    }

    return 0;
}


