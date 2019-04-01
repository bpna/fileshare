/* 
 * COMP 112 Final Project
 * 2/18/2019
 *
 * Patrick Kinsella, Jonah Feldman, Nathan Allen
 *
 * router.c provides routing between clients and file servers
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
#include "db.h"
#include "partial_message_handler.h"

#define DISCONNECTED 0
#define SOURCE_LENGTH 20
#define PASSWORD_LENGTH 20
#define FILENAME_LENGTH 20
#define LENGTH_FIELD_LENGTH 4
#define MAX_MSG_READ 450

enum client_message {NEW_CLIENT = 1, NEW_CLIENT_ACK, ERROR_CLIENT_EXISTS, \
                     REQUEST_USER};
enum server_message {CREATE_CLIENT = 64, CREATE_CLIENT_ACK, NEW_SERVER, \
                     NEW_SERVER_ACK, ERROR_SERVER_EXISTS};

struct __attribute__((__packed__)) Header {
    unsigned char id;
    char source[SOURCE_LENGTH];
    char password[PASSWORD_LENGTH];
    char filename[FILENAME_LENGTH];
    uint32_t length;
};

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

int open_and_bind_socket(int portno);

int main(int argc, char *argv[])
{
    int master_socket, csock, max_fd, rv, sockfd, status;
    socklen_t clilen;
    fd_set master_fd_set, copy_fd_set;
    struct sockaddr_in serv_addr, cli_addr;
    struct timeval timeout;

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

// int get_message_type();

int open_and_bind_socket(int portno)
{
    int master_socket = socket(AF_INET, SOCK_STREAM, 0);
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

int read_handler(int sockfd)
{
    char buffer[MAX_MSG_SIZE];
    int bytes_read = read(sockfd, buffer, MAX_MSG_SIZE);
}
