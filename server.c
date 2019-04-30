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
#include <sys/stat.h>
#include <time.h>
#include <sys/types.h>
#include <errno.h>
#include "partial_message_handler.h"
#include "database/cppairs.h"
#include "database/cspairs.h"
#include "database/filetable.h"
#include "io.h"
#include "db_wrapper.h"

#define HEADER_LENGTH 85
#define DISCONNECT -69
#define BAD_FILENAME -76

//functions to write

//reads in a message from the operator, adds a client, sends ack, creates folder for that
//does partial message handling


//linked list of partial messages

void sendHeader(int msgType, char *user, char *pwd,
                char *fname, int len, int sockfd);
char valid_fname(char *fname);
char upload_file(int sockfd, struct Header *msgHeader,
                 struct PartialMessageHandler *handler);
char server_update_file(int sockfd, struct Header *msgHeader,
                 struct PartialMessageHandler* handler);
int handle_file_request(int sockfd, struct Header *msgHeader, char is_checkout_request);
int handle_request(int sockfd, struct PartialMessageHandler *handler,
                   int personal);
void connect_to_operator(char *domainName, int operator_portno,
                         int server_portno, char* servername, int personal);
int create_client(int sockfd, struct Header *msgHeader,
                  struct PartialMessageHandler *handler);
int create_client_err(int sockfd, struct Header *msgHeader,
                      struct PartialMessageHandler *handler);
int file_list(int sockfd, struct Header *msgHeader);
char has_permissions(enum message_type message_id, struct Header *h);
char server_delete_file(int sockfd, struct Header *msgHeader);
void sync_with_backup(char *client_name);
void sync_file(char *client_name, char *file_name);
char handle_sync_file(int sockfd, struct Header *msgHeader,
                      struct PartialMessageHandler* handler);

//ARGV arguments
//      port number to run on
//      name of server (or server owner, if personal server)
//      FQDN of operator
//      port number of operator
//      bool of personal server
int main(int argc, char *argv[]) {
    if (argc < 6) {
        fprintf(stderr,"ERROR, not enough arguments\n");
        exit(1);
    }

    int lSock = open_and_bind_socket(atoi(argv[1]));
    int personal = atoi(argv[5]);
    struct sockaddr_in cli_addr;
    time_t curr_time, last_updated;
    socklen_t clilen;
    clilen = sizeof(cli_addr);

    db_t db = connect_to_db_wrapper();
    enum DB_STATUS dbs = create_file_table(db, 1);
    close_db_connection(db);
    if (dbs != SUCCESS && dbs != ELEMENT_ALREADY_EXISTS)
        error("ERROR creating file table");

    if (personal)
        time(&last_updated);

    /* : Talk with the operator to say that you've come online */
    connect_to_operator(argv[3], atoi(argv[4]),
                        atoi(argv[1]), argv[2], personal);


    create_file_table_wrapper();
    int maxSock, rv, newSock = -1;
    struct timeval tv;
    tv.tv_sec=1;
    tv.tv_usec=1000;
    fd_set masterFDSet, copyFDSet;
    FD_ZERO(&masterFDSet);
    FD_ZERO(&copyFDSet);
    listen(lSock, BACKLOG_QUEUE_SIZE);
    FD_SET(lSock, &masterFDSet);

    struct PartialMessageHandler *handler = init_partials();

    maxSock = lSock;

    while (1) {
        FD_ZERO(&copyFDSet);
        memcpy(&copyFDSet, &masterFDSet, sizeof(masterFDSet));
        rv = select (maxSock + 1, &copyFDSet, NULL, NULL, &tv);

        if (personal) {
            time(&curr_time);
            if (curr_time - last_updated >= 5) {
                time(&last_updated);
                sync_with_backup(argv[2]);
            }
        }

        if (rv == -1)
            perror("Select");
        else if (rv ==0)
            timeout_sweep(handler, &masterFDSet);
        else {
            timeout_sweep(handler, &masterFDSet);
            for (int sockfd = 0; sockfd < maxSock + 1; sockfd++) {
                //if there is any connection or new data to be read
                if (FD_ISSET(sockfd, &copyFDSet)) {
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
                        int status = handle_request(sockfd, handler, personal);

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
    write_message(sockfd, (void *) &myHeader, HEADER_LENGTH);
    return;
}

void sync_with_backup(char *client_name) {
    db_t db;
    struct db_return dbr;
    char *file_list;
    int len, i = 0;

    // db = connect_to_db(DB_OWNER, DB_NAME);
    // dbr = get_file_list(db, client_name, &file_list);
    // if (dbr.status) {
    //     fprintf(stderr, "ERROR getting file list for sync");
    // }

    // len = dbr.result;
    // while (i < len) {
    //     sync_file(client_name, file_list[i]);
    //     i += strlen(file_list) + 1;
    //     file_list += i;
    // }
    return;
}

void sync_file(char *client_name, char *file_name) {
    int sockfd;
    struct Server *server;
    db_t db;
    struct db_return dbr;
    struct stat sb;

    if (stat(file_name, &sb) == -1)
        return;

    db = connect_to_db(DB_OWNER, DB_NAME);
    dbr = get_backup_server_from_client(db, client_name);
    if (dbr.status)
        return;
    server = dbr.result;

    sockfd = connect_to_server(server->domain_name, server->port);
    free(server);
    char *fname = make_full_fname(client_name, file_name);

    sendHeader(SYNC_FILE, client_name, NULL, fname, sb.st_size, sockfd);
    free(fname);
    write_file(sockfd, file_name);
    return;
}

//returns 1 if all characters in the fname are alphnumric, ".", or "-". returns 0 otherwise
char valid_fname(char *fname) {
    for (int i = 0; i < FILENAME_FIELD_LENGTH; i++) {
        //if NULL character
        if (fname[i] == '\0' && i != 0)
            return 1;
        if (fname[i] != '-' && fname[i] != '_' && fname[i] != '.' &&
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
  
    //TODO: on every unsuccesful disconnect, delete file from table

    uint32_t bytesToRead = FILE_BUFFER_MAX_LEN;
    char buffer[FILE_BUFFER_MAX_LEN];
    bzero(buffer, FILE_BUFFER_MAX_LEN);
    int n = 0;

    int bytesRead = get_bytes_read(handler, sockfd);


    //if file already exists, send ERROR_CODE and disconnect
    if (access(msgHeader->filename, F_OK) != -1) {
        fprintf(stderr, "tried to upload file %s that already exists\n", msgHeader->filename);
        sendHeader(ERROR_FILE_EXISTS, NULL, NULL,
                   msgHeader->filename, 0, sockfd);
        return DISCONNECT;
    }

    if (valid_fname(msgHeader->filename) == 0) {
        fprintf(stderr, "invalid fname led to upload failure\n" );
        sendHeader(ERROR_INVALID_FNAME, NULL, NULL,
                   msgHeader->filename, 0, sockfd);
        return DISCONNECT;
    }

    if (!has_permissions(UPLOAD_FILE, msgHeader)){
        fprintf(stderr, "tried to uplaod file with bad permissions \n");
        sendHeader(ERROR_BAD_PERMISSIONS, NULL, NULL,
                   msgHeader->filename, 0, sockfd);
        return DISCONNECT;
    }


    char owns_file = is_file_editor_wrapper(msgHeader->source, msgHeader->filename);
    // if not the file ownder
    if (owns_file == 0){
        fprintf(stderr, "person does not own file %s, dropping connection\n", msgHeader->filename);
        return DISCONNECT;
    }
    else {
        //if file does not exist
        if (owns_file == -1){
            fprintf(stderr, "adding file %s to db\n", msgHeader->filename);
            add_file_wrapper(msgHeader->filename);
        }
        checkout_file_db_wrapper(msgHeader->source, msgHeader->filename);
    }

    if (msgHeader->length > 0){
        //if number of bytes left to read < 100000
        if (msgHeader->length - bytesRead < bytesToRead)
            bytesToRead = msgHeader->length  - bytesRead;

        n = read(sockfd, buffer, bytesToRead);
        fprintf(stderr, "%d bytes read from socket\n", n);

        if (n == 0){
            //TODO: should be delete file
            delete_file_from_table_wrapper(msgHeader->filename);
            return DISCONNECT;

        }
    }

    n = add_partial(handler, buffer, sockfd, n, 1);

    //if file completely read in
    if (n > 0) {
        sendHeader(UPLOAD_ACK, NULL, NULL, msgHeader->filename, 0, sockfd);
        de_checkout_file_wrapper(msgHeader->filename);
        return DISCONNECT;
    }

    else if (n < 0){
        fprintf(stderr, "problem in add_partial led to upload_failure\n" );
        sendHeader(ERROR_UPLOAD_FAILURE, NULL, NULL,msgHeader->filename, 0, sockfd );
        delete_file_from_table_wrapper(msgHeader->filename);
        return DISCONNECT;
    }

    return 0;
}

// vefifies a client has write access to file, reads in a file from the socket, and on completiong
// overwrites current file and sends an ACK
char server_update_file(int sockfd, struct Header *msgHeader,
                 struct PartialMessageHandler* handler) {
    uint32_t bytesToRead = FILE_BUFFER_MAX_LEN;
    char buffer[FILE_BUFFER_MAX_LEN];
    int bytesRead = get_bytes_read(handler, sockfd);
    int n = 0;


    //if file does not exist, send ERROR_CODE and disconnect
    if (access( msgHeader->filename, F_OK ) == -1) {
        fprintf(stderr, "tried to update file that does not existed\n");
        sendHeader(ERROR_FILE_DOES_NOT_EXIST, NULL, NULL,
                   msgHeader->filename, 0, sockfd);
        return DISCONNECT;
    }
  
    if (!is_file_editor_wrapper(msgHeader->source, msgHeader->filename)){
        fprintf(stderr, "tried to update file without checking it out \n");
        sendHeader(ERROR_BAD_PERMISSIONS, NULL, NULL,
                   msgHeader->filename, 0, sockfd);
        return DISCONNECT;
    }

    if (!has_permissions(UPDATE_FILE, msgHeader)){
        fprintf(stderr, "tried to update file with bad permissions \n");
        sendHeader(ERROR_BAD_PERMISSIONS, NULL, NULL,
                   msgHeader->filename, 0, sockfd);
        return DISCONNECT;
    }

    if (msgHeader->length > 0){
        //if number of bytes left to read < 100000
        if (msgHeader->length - bytesRead < bytesToRead)
            bytesToRead = msgHeader->length  - bytesRead;

        n = read(sockfd, buffer, bytesToRead);
        fprintf(stderr, "%d bytes read from socket\n", n);

        if (n == 0){
            return DISCONNECT;
        }
    }

    n = add_partial(handler, buffer, sockfd, n, 1);

    //if file completely read in
    if (n > 0) {
        de_checkout_file_wrapper(msgHeader->filename);
        sendHeader(UPDATE_ACK, NULL, NULL, msgHeader->filename, 0, sockfd);
        return DISCONNECT;
    }
    return 0;
}

char handle_sync_file(int sockfd, struct Header *msgHeader,
                      struct PartialMessageHandler* handler) {
    uint32_t bytesToRead = FILE_BUFFER_MAX_LEN;
    char buffer[FILE_BUFFER_MAX_LEN];
    int bytesRead = get_bytes_read(handler, sockfd);

    //if number of bytes left to read < 100000
    if (msgHeader->length - bytesRead < bytesToRead)
        bytesToRead = msgHeader->length  - bytesRead;

    //if file does not exist, send ERROR_CODE and disconnect
    if (access(msgHeader->filename, F_OK ) == -1) {
        fprintf(stderr, "tried to update file that does not existed\n");
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
        // TODO: update file
        return DISCONNECT;
    }

    return 0;
}

/*
 * deletes a given file
 */
char server_delete_file(int sockfd, struct Header *msgHeader) {
    //if file does not exist, send ERROR_CODE and disconnect
    if (access( msgHeader->filename, F_OK ) == -1) {
        fprintf(stderr, "tried to delete file that does not existed\n");
        sendHeader(ERROR_FILE_DOES_NOT_EXIST, NULL, NULL,
                   msgHeader->filename, 0, sockfd);
        return DISCONNECT;
    }

    if (!has_permissions(DELETE_FILE, msgHeader)){
        fprintf(stderr, "tried to update file with bad permissions \n");
        sendHeader(ERROR_BAD_PERMISSIONS, NULL, NULL,
                   msgHeader->filename, 0, sockfd);
        return DISCONNECT;
    }

    //if file is not checked out
    if (checkout_file_db_wrapper(msgHeader->source, msgHeader->filename) == -1){

        fprintf(stderr, "tried to delete a file that is checked out\n");
        sendHeader(ERROR_FILE_DOES_NOT_EXIST, NULL, NULL,
                   msgHeader->filename, 0, sockfd);
        return DISCONNECT;
    }
    remove(msgHeader->filename);
    delete_file_from_table_wrapper(msgHeader->filename);
    sendHeader(DELETE_FILE_ACK, NULL, NULL,
                   msgHeader->filename, 0, sockfd);

    return DISCONNECT;
}

/*
 * purpose: handles a request for a file from a client. If user has proper permissions
 *  and the request is valid, sends back the RETURN_READ_ONLY_FILE header and file
 * Returns DISCONNECT in every case
 */
int handle_file_request(int sockfd, struct Header *msgHeader,
                        char is_checkout_request) {
    struct stat sb;
    char *token;
    char buffer[FILENAME_FIELD_LENGTH * 2];
    bzero(buffer, FILENAME_FIELD_LENGTH * 2);
    enum message_type message_id = RETURN_READ_ONLY_FILE;

    if (stat(msgHeader->filename, &sb) == -1) {
        fprintf(stderr, "client requested file that does not exist\n" );
        sendHeader(ERROR_FILE_DOES_NOT_EXIST, NULL, NULL,
                   msgHeader->filename, 0, sockfd);
        return DISCONNECT;
    }



    if (is_checkout_request){
        if (!has_permissions(CHECKOUT_FILE, msgHeader)){
            fprintf(stderr, "tried to update file with bad permissions \n");
            sendHeader(ERROR_BAD_PERMISSIONS, NULL, NULL,
                       msgHeader->filename, 0, sockfd);
            return DISCONNECT;
        }


        if (checkout_file_db_wrapper(msgHeader->source, msgHeader->filename) != -1){
            fprintf(stderr, "in server.c, he got permission to check out file\n" );
            message_id = RETURN_CHECKEDOUT_FILE;
        }
    }

    memcpy(buffer, msgHeader->filename, FILENAME_FIELD_LENGTH);

    token = strtok(buffer, "/");
    if (token == NULL){
        fprintf(stderr, "malformed file name\n" );
        de_checkout_file_wrapper(msgHeader->filename);
        return DISCONNECT;
    }
    token = strtok(NULL, "");

    sendHeader(message_id, NULL, NULL, token, sb.st_size, sockfd);
    if (write_file(sockfd, msgHeader->filename))
        error("ERROR sending file");

    return DISCONNECT;
}

//reads in a request
//in case of either ERROR or SUCCESS, return DISCONNECT CODE. only returns 0 in case of partial read
int handle_request(int sockfd, struct PartialMessageHandler *handler,
                   int personal) {
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

    fprintf(stderr, "the type of message incoming is %d\n", msgHeader->id);
    fprintf(stderr, "the username incoming is %s\n", msgHeader->source);
    fprintf(stderr, "the password incoming is %s\n", msgHeader->password);
    fprintf(stderr, "the fname incoming is %s\n", msgHeader->filename);
    fprintf(stderr, "the length incoming is %d\n", msgHeader->length);


    message_id = msgHeader->id;
    switch(message_id){
        case CREATE_CLIENT:
            if (personal) {
                fprintf(stderr, "trying to add client to personal server\n");
                return create_client_err(sockfd, msgHeader, handler);
            } else {
                fprintf(stderr, "creating clinet\n");
                return create_client(sockfd, msgHeader, handler);
            }
        case UPLOAD_FILE:
            fprintf(stderr, "uploading file\n");
            return upload_file(sockfd, msgHeader, handler);
                //verify creds from SQL database, verify file doesn't exist, read, write file to Disk, send ACK
        case REQUEST_FILE:
            fprintf(stderr, "requesting file\n");
            return handle_file_request(sockfd, msgHeader, 0);
            //verify creds, verify file exists, send back file
        case UPDATE_FILE:
            fprintf(stderr, "updating file\n");
            return server_update_file(sockfd, msgHeader, handler);
            //verify creds from SQL database, verify file doesn't exist, read, write file to Disk, send ACK
            //TODO in future: add in permision cases
        case FILE_LIST:
            return file_list(sockfd, msgHeader);
        case CHECKOUT_FILE:
                fprintf(stderr, "checking out file\n" );
                return handle_file_request(sockfd, msgHeader, 1);
        case DELETE_FILE:
            return server_delete_file(sockfd, msgHeader);
        case SYNC_FILE:
            return handle_sync_file(sockfd, msgHeader, handler);
        default:
            return DISCONNECT;
    }
    return DISCONNECT;
}

int create_client(int sockfd, struct Header *msgHeader,
                  struct PartialMessageHandler* handler) {
    db_t db;
    //enum DB_STATUS dbs;
    char buffer[512];
    char username[SOURCE_FIELD_LENGTH];
    char password[PASSWORD_FIELD_LENGTH];
    char *token;
    int n;
    bzero(buffer, 512);
    bzero(username, SOURCE_FIELD_LENGTH);
    bzero(password, PASSWORD_FIELD_LENGTH);

    

    n = read(sockfd, buffer, msgHeader->length);
    if (n < 1)
        return DISCONNECT;

    if (add_partial(handler, buffer, sockfd, n, 0) == 0){
        return 1;
    }

    db = connect_to_db(DB_OWNER, DB_NAME);
    token = strtok(buffer, ":");
    strcpy(username, token);
    token = strtok(NULL, "");

    strcpy(password, token);


    // dbs = add_cppair(db, username, password);
    close_db_connection(db);
    // if (dbs)
    //     return DISCONNECT;

    mkdir(username, S_IRWXU);

    sendHeader(CREATE_CLIENT_ACK, NULL, NULL, NULL, 0, sockfd);
    return DISCONNECT;
}

int file_list(int sockfd, struct Header *msgHeader) {
    struct dirent *ent;
    DIR *dir;
    char files[10000]; // TODO: create array resize
    int loc = 0;

    bzero(files, 10000);
    dir = opendir(msgHeader->filename);
    ent = readdir(dir);
    ent = readdir(dir); /* get rid of . and .. files */
    ent = readdir(dir); /* first actual file */
    while (ent != NULL) {
        memcpy(&files[loc], ent->d_name, strlen(ent->d_name));
        loc += strlen(ent->d_name) + 1;
    }

    sendHeader(FILE_LIST_ACK, NULL, NULL, NULL, loc, sockfd);
    write_message(sockfd, files, loc);

    return DISCONNECT;
}

int create_client_err(int sockfd, struct Header *msgHeader,
                      struct PartialMessageHandler *handler) {
    char buffer[512];
    char username[SOURCE_FIELD_LENGTH];
    char password[PASSWORD_FIELD_LENGTH];
    char *token;
    int n;
    bzero(buffer, 512);
    bzero(username, SOURCE_FIELD_LENGTH);
    bzero(password, PASSWORD_FIELD_LENGTH);

    n = read(sockfd, buffer, msgHeader->length);
    if (n < 1)
        return DISCONNECT;

    if (add_partial(handler, buffer, sockfd, n, 0) == 0){
        return 1;
    }

    token = strtok(buffer, ":");
    strcpy(username, token);

    sendHeader(CREATE_CLIENT_ERROR, NULL, NULL, username, 0, sockfd);
    return DISCONNECT;

}

//connects to operator, recieves ack, and returns socked number of connection with operator
//TODO: put the first half of this in a "connectToServer" function, possibly in a different file
void connect_to_operator(char *domainName, int operator_portno,
                         int server_portno, char *servername, int personal) {
    char buffer[512];
    bzero(buffer, 512);
    int operator_sock = connect_to_server(domainName, operator_portno);
    int n;
    enum message_type type;

    if (personal)
        type = NEW_PERSONAL_SERVER;
    else
        type = NEW_SERVER;
    /* send message*/
    sprintf(buffer, "%s:%d", "localhost", server_portno);
    sendHeader(type, servername, NULL, NULL, strlen(buffer), operator_sock);
    write_message(operator_sock, buffer, strlen(buffer));

    n = read(operator_sock, buffer, HEADER_LENGTH);
    if (n <= 0)
        error("unable to establish connection with operator");
    close(operator_sock);

    struct Header *headerBuf = (void *) buffer;
    enum message_type message_id = headerBuf->id;
    if (message_id == NEW_SERVER_ACK || message_id == NEW_PERSONAL_SERVER_ACK) {
        fprintf(stderr, "successfully connected to operator\n" );
        return;
    } else if (message_id == ERROR_SERVER_EXISTS)
        error("inputted servername already exists");
    else
        error("unknown error connecting operator, please try again");
    return;
}

/*
 * A function that determines if a client has proper read/write access to the
 * file they are reqesting to download, upload, or modify.
 *
 * Arguments:
 *      message_id: the action the client is performing
 *      h: the request header
 *
 * Returns 1 if client has correct permissions, 0 elsewhere
 */
char has_permissions(enum message_type message_id, struct Header *h){
    char *file_owner;
    char fname[FILENAME_FIELD_LENGTH];

    memcpy(fname, h->filename, FILENAME_FIELD_LENGTH);
    file_owner = strtok(fname, "/");

    /*
     if (personal_server){
        if message_id == REQUEST_FILE
            return 1
        else
            return (strcmp(file_owner, h->source) == 0)?  1 : 0;
     }
     */

    if (message_id != UPLOAD_FILE )
        return 1;

    //if the file owner the person uploading the file?
    return (strcmp(file_owner, h->source) == 0)?  1 : 0;
}

int file_list(int sockfd, struct Header *msgHeader) {
    db_t db = connect_to_db_wrapper();
    char **list = NULL;
    int length = get_file_list_wrapper(db, msgHeader->filename, list);
    sendHeader(FILE_LIST_ACK, NULL, NULL, NULL, length, sockfd);
    write_message(sockfd, *list, length);

    return DISCONNECT;
}

