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
#include "messages.h"
#include "partial_message_handler.h"

#define DISCONNECTED 0

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

int open_and_bind_socket(int portno);
int add_partial_data(char *data, int length);

int main(int argc, char *argv[])
{
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

int open_and_bind_socket(int portno) {
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

int read_handler(int sockfd, struct PartialMessageHandler *handler) {
    int header_bytes_read;
    char buffer[HEADER_LENGTH];
    int bytes_read = read(sockfd, buffer, MAX_MSG_SIZE);

    if (bytes_read == 0) {
        close(sockfd);
        return DISCONNECTED;
    }

    header_bytes_read = get_partial_header(handler, sockfd, buffer);
    if (bytes_read < HEADER_LENGTH) {
        n = read(sockfd, buffer, HEADER_LENGTH - header_bytes_read);
        if (n < HEADER_LENGTH - header_bytes_read) {
            add_partial(handler, buffer, sockfd, n, 0);

        }
    }
}
