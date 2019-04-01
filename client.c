#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include "db.h"

#define DISCONNECTED 0
#define ID_FIELD_LENGTH 1
#define ID_OFFSET 0
#define SOURCE_FIELD_LENGTH 20
#define SOURCE_FIELD_OFFSET 1
#define PASSWORD_FIELD_LENGTH 20
#define PASSWORD_FIELD_OFFSET 21
#define FILENAME_FIELD_LENGTH 20
#define FILENAME_FIELD_OFFSET 41
#define LENGTH_FIELD_LENGTH 4
#define LENGTH_FIELD_OFFSET 61
#define HEADER_LENGTH 65
#define MAX_MSG_SIZE 450

enum message_type {NEW_CLIENT = 1, NEW_CLIENT_ACK, ERROR_CLIENT_EXISTS,
                   REQUEST_USER, CREATE_CLIENT = 64, CREATE_CLIENT_ACK,
                   NEW_SERVER, NEW_SERVER_ACK, ERROR_SERVER_EXISTS,
                   UPLOAD_FILE = 128, UPLOAD_ACK, ERROR_FILE_EXISTS,
                   ERROR_UPLOAD_FAILURE, REQUEST_FILE, RETURN_FILE, 
                   UPDATE_FILE, UPDATE_ACK, ERROR_FILE_DOES_NOT_EXIST,
                   ERROR_BAD_PERMISSIONS};

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

struct __attribute__((__packed__)) Header {
    unsigned char id;
    char source[SOURCE_FIELD_LENGTH];
    char password[PASSWORD_FIELD_LENGTH];
    char filename[FILENAME_FIELD_LENGTH];
    uint32_t length;
};

int parse_request(int argc, char **argv);
int connect_to_server(char *fqdn, int portno);
int write_message(int sockfd, char *data, int length);
int write_file(int csock, char *filename);

int main(int argc, char **argv)
{
    int sockfd, portno, n;
    char header[450];
    enum message_type message_id;
    struct Header message_header;

    message_id = parse_request(argc, argv);
    switch (message_id) {
        case UPLOAD_FILE:
            sockfd = connect_to_server(argv[2], atoi(argv[3]));
            bzero(&message_header, sizeof(message_header));
            message_header.id = UPLOAD_FILE;
            printf("argv[4] = %s\n", argv[4]);
            strcpy(message_header.source, argv[4]);
            printf("argv[5] = %s\n", argv[5]);
            strcpy(message_header.password, argv[5]);
            printf("argv[6] = %s\n", argv[6]);
            strcpy(message_header.filename, argv[6]);
            write_message(sockfd, ((char *)&message_header), HEADER_LENGTH);
            write_file(sockfd, message_header.filename);
            break;

        default:
            fprintf(stderr, "bad message type >:O\n");
    }

    close(sockfd);
    return 0;
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
    }

    if (strcmp(argv[1], "upload_file") == 0) {
        if (argc != 7) {
            fprintf(stderr, "usage: %s upload_file [server-FQDN] \
                             [server-portno] [username] [password] \
                             [filename]\n", argv[0]);          
        }

        return UPLOAD_FILE;
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
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) {
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


