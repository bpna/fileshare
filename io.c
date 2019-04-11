#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <unistd.h>
#include <strings.h>
#include <netdb.h>

#define FILE_BUFFER_MAX_LEN 10000

void error(const char *msg) {
    perror(msg);
    exit(0);
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

int connect_to_server(char *fqdn, int portno) {
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
    bcopy((char *)server->h_addr_list[0],
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(portno);
    if (connect(sockfd,(struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        error("ERROR connecting");
    }

    return sockfd;
}

int write_message(int csock, char *data, int length) {
    int n = 0;

    while (n < length) {
        n += write(csock, &data[n], length - n);
        if (n < 0) {
            error("ERROR writing to socket");
        }
    }

    return 0;
}

int write_file(int csock, char *filename)
{
    FILE *fp = fopen(filename, "rb");
    char bytes[FILE_BUFFER_MAX_LEN];
    long filelen;
    int to_write, bytes_written = 0;

    fseek(fp, 0, SEEK_END);
    filelen = ftell(fp);
    rewind(fp);

    while (bytes_written < filelen) {
        if (filelen - bytes_written < FILE_BUFFER_MAX_LEN) {
            to_write = filelen - bytes_written;
        } else {
            to_write = FILE_BUFFER_MAX_LEN;
        }

        fread(bytes, 1, to_write, fp);
        write_message(csock, bytes, to_write);
        bytes_written += to_write;
    }
    fclose(fp);

    return 0;
}