#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/select.h>
#include <ctype.h>

#include <sys/time.h>
#include <time.h>
#include <sys/types.h>
#include "partial_message_handler.h"
#include "database/cppairs.h"

#define HEADER_LENGTH 85
#define DISCONNECT -69
#define BAD_FILENAME -76
#define DB_OWNER "nathan"
#define DB_NAME "fileshare"

//functions to write

//reads in a message from the operator, adds a client, sends ack, creates folder for that
//does partial message handling

//TODO: set timeout value on read

//TODO: if you eliminate a timed-out partial for upload, make sure to delete file that was being written
//linked list of partial messages

void error(const char *msg) {
    perror(msg);
    exit(1);
}

void sendHeader(int msgType, char *user, char * pwd,
                char* fname, int len, int sockfd);
int intializeLSock(int portno);
char valid_fname(char *fname);
char upload_file(int sockfd, struct Header *msgHeader,
                 struct PartialMessageHandler* handler);
char update_file(int sockfd, struct Header *msgHeader,
                 struct PartialMessageHandler* handler);
int handle_file_request(int sockfd, struct Header *msgHeader,
                        struct PartialMessageHandler* handler);
int handle_request(int sockfd, struct PartialMessageHandler *handler);
int connect_to_operator(char *domainName, int portno, char* servername);
int create_client(int sockfd, struct Header *msgHeader,
                  struct PartialMessageHandler* handler);

//ARGV arguments
//      port number to run on
//      name of server
//      FQDN of operator
//      port number of operator
//
int main(int argc, char *argv[]) {
    if (argc < 5) {
        fprintf(stderr,"ERROR, not enough arguments\n");
        exit(1);
    }

    int lSock = intializeLSock(atoi(argv[1]));
    struct sockaddr_in cli_addr;
    socklen_t clilen;
    clilen = sizeof(cli_addr);

    /* TODO: Talk with the operator to say that you've come online */
    int operatorSock = connect_to_operator(argv[2], atoi(argv[3]), argv[1]);

    int maxSock, rv, newSock = -1;
    struct timeval tv;
    tv.tv_sec=1;
    tv.tv_usec=1000;
    fd_set masterFDSet, copyFDSet;
    FD_ZERO(&masterFDSet);
    FD_ZERO(&copyFDSet);
    FD_SET(lSock, &masterFDSet);

    struct PartialMessageHandler *handler = init_partials();

    maxSock = lSock;

    while (1) {
        FD_ZERO(&copyFDSet);
        memcpy(&copyFDSet, &masterFDSet, sizeof(masterFDSet));
        rv = select (maxSock + 1, &copyFDSet, NULL, NULL, &tv);

        if (rv == -1)
            perror("Select");
        else if (rv ==0)
            timeout_sweep(handler, &masterFDSet);
        else {
            timeout_sweep(handler, &masterFDSet);
            for (int sockfd = 0; sockfd < maxSock + 1; sockfd++) {
                //if there is any connection or new data to be read
                if ( FD_ISSET (sockfd, &copyFDSet) ) {
                    if (sockfd == lSock) {
                        fprintf(stderr, "there's a new connection in town\n");
                        newSock = accept(sockfd, (struct sockaddr *) &cli_addr,
                                         &clilen);
                        if (newSock > 0) {
                            FD_SET(newSock, &masterFDSet);
                            maxSock = (newSock > maxSock) ? newSock: maxSock;
                        }
                    } else {
                        fprintf(stderr, "new info incoming\n" );
                        //if sockfd = operatorSock
                            // addClient
                        int status = handle_request(sockfd, handler);

                        if (status == DISCONNECT){
                            fprintf(stderr, "disconnecting client\n" );
                            delete_partial(handler, sockfd);
                            close(sockfd);
                            FD_CLR(sockfd, &masterFDSet);
                        }
                    }
                }
            }
        }
    }
    close(lSock);
    return 0;
}

//returns an allocated header
void sendHeader(int msgType, char *user, char * pwd,
                char* fname, int len, int sockfd) {
    struct Header myHeader;
    bzero(&myHeader, HEADER_LENGTH);
    myHeader.id = msgType;
    if (user != NULL)
            memcpy(myHeader.source, user, SOURCE_FIELD_LENGTH);
    if (pwd != NULL)
            memcpy(myHeader.password, pwd, PASSWORD_FIELD_LENGTH);
    if (fname != NULL)
            memcpy(myHeader.filename, fname, FILENAME_FIELD_LENGTH);
    myHeader.length = htonl(len);
    int n = write(sockfd, (void *) &myHeader, HEADER_LENGTH);
    return;
}

//sets up the server sockets and binds it to the port
int intializeLSock(int portno) {
    int sockfd;
    struct sockaddr_in serv_addr;
    int n;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
       error("ERROR opening socket");
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
             error("ERROR on binding");
    listen(sockfd,5);
    return sockfd;
}

//returns 1 if all characters in the fname are alphnumric, ".", or "-". returns 0 otherwise
char valid_fname(char *fname) {
    for (int i = 0; i < FILENAME_FIELD_LENGTH; i++) {
        //if NULL character
        if (fname[i] == '\0' && i != 0)
            return 1;
        if (fname[i] != '-' && fname[i] != '.' &&
            fname[i] != '/' && isalnum(fname[i]) == 0)
            return 0;
    }
    return 1;
}

//reads in file
//writes ack to file
//writes ack if successfull, error if not
//returns DISCONNECT on succesfull read OR Error, returns 0 on partial read
char upload_file(int sockfd, struct Header *msgHeader,
                 struct PartialMessageHandler* handler) {
    int bytesToRead = FILE_BUFFER_MAX_LEN;
    char buffer[FILE_BUFFER_MAX_LEN];

    int bytesRead = get_bytes_read(handler, sockfd);

    //if file already exists, send ERROR_CODE and disconnect
    if (access(msgHeader->filename, F_OK) != -1) {
        fprintf(stderr, "tried to upload file that existed\n");
        sendHeader(ERROR_FILE_EXISTS, NULL, NULL,
                   msgHeader->filename, 0, sockfd);
        return DISCONNECT;
    }

    //TODO: make an error code for "bad filename"
    if (valid_fname(msgHeader->filename) == 0) {
        sendHeader(ERROR_UPLOAD_FAILURE, NULL, NULL,
                   msgHeader->filename, 0, sockfd);
        return DISCONNECT;
    }

    //if number of bytes left to read < 100000
    if (msgHeader->length  - bytesRead < bytesToRead)
        bytesToRead = msgHeader->length  - bytesRead;

    int n = read(sockfd, buffer, bytesToRead);
    fprintf(stderr, "%d bytes read from socket\n", n);

    if (n == 0)
        return DISCONNECT;

    n = add_partial(handler, buffer, sockfd, n, 1);

    //if file completely read in
    if (n > 0) {
        sendHeader(UPLOAD_ACK, NULL, NULL, msgHeader->filename, 0, sockfd);
        return DISCONNECT;
    }

    return 0;
}

// vefifies a client has write access to file, reads in a file from the socket, and on completiong
// overwrites current file and sends an ACK
char update_file(int sockfd, struct Header *msgHeader,
                 struct PartialMessageHandler* handler) {
    int bytesToRead = FILE_BUFFER_MAX_LEN;
    char buffer[FILE_BUFFER_MAX_LEN];
    int bytesRead = get_bytes_read(handler, sockfd);

    //if number of bytes left to read < 100000
    if (msgHeader->length  - bytesRead < bytesToRead)
        bytesToRead = msgHeader->length  - bytesRead;

    //if file does not exist, send ERROR_CODE and disconnect
    if (access( msgHeader->filename, F_OK ) == -1) {
        fprintf(stderr, "tried to upload file that existed\n");
        sendHeader(ERROR_FILE_DOES_NOT_EXIST, NULL, NULL,
                   msgHeader->filename, 0, sockfd);
        return DISCONNECT;
    }

    int n = read(sockfd, buffer, bytesToRead);
    fprintf(stderr, "%d bytes read from socket\n", n);

    if (n == 0)
        return DISCONNECT;

    n = add_partial(handler, buffer, sockfd, n, 1);

    //if file completely read in
    if (n > 0) {
        sendHeader(UPDATE_ACK, NULL, NULL, msgHeader->filename, 0, sockfd);
        return DISCONNECT;
    }

    return 0;
}

/*
 * purpose: handles a request for a file from a client. If user has proper permissions
 *  and the request is valid, sends back the RETURN_FILE header and file
 * Returns DISCONNECT in every case
 */
int handle_file_request(int sockfd, struct Header *msgHeader,
                        struct PartialMessageHandler* handler) {
    //if file does not exist, send ERROR_CODE and disconnect
    FILE *fp = fopen(msgHeader->filename, "rb");
    int length, n, m;
    char buffer[FILE_BUFFER_MAX_LEN];

    if (fp == NULL) {
        sendHeader(ERROR_FILE_DOES_NOT_EXIST, NULL, NULL, msgHeader->filename,
                   0, sockfd);
        return DISCONNECT;
    }

    //TODO: check permissions

    fseek(fp, 0, SEEK_END);
    length = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    sendHeader(RETURN_FILE, NULL, NULL, msgHeader->filename, length, sockfd);
    do {
        n = fread(buffer, 1, FILE_BUFFER_MAX_LEN, fp);
        m = write(sockfd, buffer, n);
        if (m < 0)
            error("error writing to socket");
    } while (n == FILE_BUFFER_MAX_LEN);
    fclose(fp);
    return DISCONNECT;
}

//reads in a request
//in case of either ERROR or SUCCESS, return DISCONNECT CODE. only returns 0 in case of partial read
int handle_request(int sockfd, struct PartialMessageHandler *handler) {
    int n = 0, header_bytes_read;
    char buffer[HEADER_LENGTH];
    bzero(buffer, HEADER_LENGTH);
    struct Header *msgHeader;
    header_bytes_read = get_partial_header(handler, sockfd, buffer);
    msgHeader = (void *) buffer;
    enum message_type message_id;

    //TODO: put functions inside of these, this is paralell code
    if (header_bytes_read < HEADER_LENGTH){
        n = read(sockfd, buffer, HEADER_LENGTH - header_bytes_read);
        if (n < HEADER_LENGTH - header_bytes_read){
            add_partial(handler, buffer, sockfd, n, 0);
            return 0;
        }
        else{
            memcpy(&msgHeader[header_bytes_read], buffer,
                   HEADER_LENGTH - header_bytes_read);
            msgHeader->length = ntohl(msgHeader->length);
            if (msgHeader->length > 0){
                add_partial(handler, buffer, sockfd, n, 0);
                return 0;
            }
        }
    }

    fprintf(stderr, "the number of bytes read in was %d\n", n);
    fprintf(stderr, "the type of message incoming is %d\n", msgHeader->id);
    fprintf(stderr, "the username incoming is %s\n", msgHeader->source);
    fprintf(stderr, "the password incoming is %s\n", msgHeader->password);
    fprintf(stderr, "the fname incoming is %s\n", msgHeader->filename);
    fprintf(stderr, "the length incoming is %d\n", msgHeader->length);

    /* TODO: Deal with endianess */

    message_id = msgHeader->id;
    switch(message_id){
        case CREATE_CLIENT:
            fprintf(stderr, "creating clinet\n");
            // return create_client(sockfd, msgHeader, handler);
        case UPLOAD_FILE:
            fprintf(stderr, "uploading file\n");
            return upload_file(sockfd, msgHeader, handler);
                //verify creds from SQL database, verify file doesn't exist, read, write file to Disk, send ACK
        case REQUEST_FILE:
            fprintf(stderr, "requesting file\n");
            return handle_file_request(sockfd, msgHeader, handler);
            //verify creds, verify file exists, send back file
        case UPDATE_FILE:
            fprintf(stderr, "updating file\n");
            return update_file(sockfd, msgHeader, handler);
            //verify creds from SQL database, verify file doesn't exist, read, write file to Disk, send ACK
            //TODO in future: add in permision cases
            //TODO: Delete file
            //TODO:
        case NEW_SERVER_ACK:
            fprintf(stderr, "new server ack\n");
            return DISCONNECT;
        case ERROR_SERVER_EXISTS:
            fprintf(stderr, "server exists\n");
            error("ERROR server exists in operator");
        default:
            return 1;
    }
    return DISCONNECT;
}

int create_client(int sockfd, struct Header *msgHeader,
                  struct PartialMessageHandler* handler) {
    db_t *db;
    enum DB_STATUS dbs;
    char *username, *password, *servername;
    struct Header h;

    db = connect_to_db(DB_OWNER, DB_NAME);
    // TODO: get user info

    dbs = add_cppair(db, username, password);
    close_db_connection(db);
    if (dbs)
        return 1;

    sendHeader(CREATE_CLIENT_ACK, servername, NULL, NULL, 0, sockfd);
    return DISCONNECT;
}

//connects to operator, recieves ack, and returns socked number of connection with operator
//TODO: put the first half of this in a "connectToServer" function, possibly in a different file
int connect_to_operator(char *domainName, int portno, char* servername) {
    char buffer[HEADER_LENGTH];
    bzero(buffer, HEADER_LENGTH);

    struct sockaddr_in serv_addr;
    struct hostent *server;

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");

    server = gethostbyname(domainName);
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

    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0)
        error("ERROR connecting");

    /* send message*/
    sendHeader(NEW_SERVER, NULL, NULL, servername, 0, sockfd);

    int n = read(sockfd, buffer, HEADER_LENGTH);
    if (n == 0)
        error("unable to establish connection with operator");

    struct Header *headerBuf = (void *) buffer;
    enum message_type message_id = headerBuf->id;
    if (message_id == NEW_SERVER_ACK)
        return sockfd;
    else if (message_id == ERROR_SERVER_EXISTS)
        error("inputted servername already exists");
    else
        error("unknown error connecting operator, please try again");
    return sockfd;
}
