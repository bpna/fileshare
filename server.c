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
#include <unistd.h>
#include "partial_message_handler.h"

#define HEADER_LENGTH 65
#define DISCONNECT_CODE -69
#define MINUTE 60000
#define NEW_SERVER_CODE 66
#define NEW_SERVER_ACK_CODE 67
#define ERROR_SERVER_EXISTS_CODE 68
#define UPLOAD_FILE_CODE 128
#define UPLOAD_ACK_CODE 129
#define ERROR_FILE_EXISTS_CODE 130
#define ERROR_UPLOAD_FAILURE_CODE 131
#define REQUEST_FILE_CODE 132
#define RETURN_FILE_CODE 133
#define UPDATE_FILE_CODE 134
#define UPDATE_ACK_CODE 135
#define ERROR_FILE_DOES_NOT_EXIST_CODE 136

//functions to write

//reads in a message from the router, adds a client, sends ack, creates folder for that 
//does partial message handling

//TODO: set timeout value on read

//TODO: if you eliminate a timed-out partial for upload, make sure to delete file that was being written 
//linked list of partial messages



void error(const char *msg)
{
    perror(msg);
    exit(1);
} 

//returns an allocated header
//
void sendHeader(int msgType, char *user, char * pwd, char* fname, int len, int sockfd){
    struct Header myHeader;
    bzero(myHeader, HEADER_LENGTH);
    myHeader.id = msgType;
    if (user != NULL)
            memcpy(myHeader.source, user, SOURCE_FIELD_LENGTH);
    if (pwd != NULL)
            memcpy(myHeader.password, pwd, PASSWORD_FIELD_LENGTH);
    if (fname != NULL)
            memcpy(myHeader.filename, fname, FILENAME_FIELD_LENGTH);
    myHeader.length = len;
    int n = write(sockfd, (void *) &myHeader, HEADER_LENGTH);

}


//sets up the server sockets and binds it to the port
int intializeLSock(int portno){

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
char validFname(char *fname){
    for (int i = 0; i < HEADER_LENGTH; i++){
        if (fname[i] == '\0')
            return 1;
        if (fname[i] != '-' && fname[i] != '.' && isalnum(fname[i] == 0)){
            return 0;

        }
    }
    return 1;
}

//reads in file
//writes ack to file
//writes ack if successfull, error if not
//returns DISCONNECT_CODE on succesfull read OR Error, returns 0 on partial read
//PROBLEMS:
//      how do you know when its returned to you the "final" chunk of cod
//          how do you know how many bytes that is? critical to know if your'e writing to disk
//      Scenario: you have a 13k bytes file. You do an initial read of 7000 bytes, then a real of 6000 bytes. WHen the partialMessage does the second read, it only returns 10k/13k bytes. When/how do the remainig 3k bytes get returned. If it gets returned as one 13k byte chunk, then how will it fit in the buffer
//      need a function for deletePartial
//      need a function for 
char uploadFile(int sockfd, struct Header *msgHeader, struct PartialMessageHandler* handler){
    
    int bytesToRead = 10000;
    char buffer[20000];

    int bytesRead = get_bytes_read(handler, sockfd);

    //if file already exists, send ERROR_CODe and disconnect
    if( access( msgHeader->filename, F_OK ) != -1 && bytesRead == 0 ){
        fprintf(stderr, "tried to upload file that existed\n");
        sendHeader(ERROR_FILE_EXISTS_CODE, NULL, NULL, msgHeader->filename, 0, sockfd);
        return DISCONNECT_CODE;
    }
    
    
    //TODO: verify filename has no special characteres except "."
    

    if(msgHeader->length  - bytesRead < bytesToRead){
        bytesToRead = msgHeader->length  - bytesRead;
    }

    //TODO: prepend the file with a special character so we know it isn't "ready"
    

    int n = read(sockfd, buffer, bytesToRead);
    fprintf(stderr, "%d bytes read from socket\n", n);

    if (n == 0){
        //TODO:delete partial, right here
        return DISCONNECT_CODE;
    }

    n = add_partial(handler, buffer, sockfd, n);
    fprintf(stderr, "%d bytes returned from add_partial function\n", n);
    FILE *fp = fopen(msgHeader->filename, "a");
    
    fwrite(buffer, 1, n, fp);
    fclose(fp);
    if (n + bytesRead == msgHeader->length){
       
        
        sendHeader(UPLOAD_ACK_CODE, NULL, NULL, msgHeader->filename, 0, sockfd);
        return DISCONNECT_CODE;
    }
    
    return 0;


}

//reads in a request
//in case of either ERROR or SUCCESS, return DISCONNECT CODE. only returns 0 in case of partial read

int handleRequest(int sockfd, struct PartialMessageHandler *handler){
    int n, headerBytesRead;
    char buffer[HEADER_LENGTH];
    bzero(buffer, HEADER_LENGTH);
    struct Header *msgHeader;
    char did_read;
    headerBytesRead = getPartialHeader(handler, sockfd, buffer);
    msgHeader = (void *) buffer;
    enum message_type message_id;

    //TODO: put functions inside of these, this is paralell code
    if (headerBytesRead == 0){
        n = read(sockfd, buffer, HEADER_LENGTH);
        if (n < HEADER_LENGTH){
            add_partial(handler, buffer, sockfd, n);
            return 0;
        }
        else{
            memcpy(msgHeader, buffer, HEADER_LENGTH);
            if (msgHeader->length > 0){
                add_partial(handler, buffer, sockfd, n);
                return 0;
            }
        }

    }
    else if (headerBytesRead < HEADER_LENGTH){
        n = read(sockfd, buffer, HEADER_LENGTH - headerBytesRead);
        if (n < HEADER_LENGTH - headerBytesRead){
            add_partial(handler, buffer, sockfd, n);
            return 0;
        }
        else{
            memcpy(&msgHeader[headerBytesRead], buffer, HEADER_LENGTH - headerBytesRead);
            if (msgHeader->length > 0){
                add_partial(handler, buffer, sockfd, n);
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
        case UPLOAD_FILE:
            fprintf(stderr, "uploading file\n" );
            return uploadFile(sockfd, msgHeader, handler);
                //verify creds from SQL database, verify file doesn't exist, read, write file to Disk, send ACK
            
            break;
        case REQUEST_FILE:
            fprintf(stderr, " requesting file\n" );
            return handleFileRequest(sockfd, msgHeader, handler);
            //verify creds, verify file exists, send back file
            break;
        case UPDATE_FILE:
            fprintf(stderr, " updating file\n" );
            return updateFile(sockfd, msgHeader, handler);

             //verify creds from SQL database, verify file doesn't exist, read, write file to Disk, send ACK
            break;
            //TODO in future: add in permision cases
            //TODO: Delete file
            //TODO: 
    }
    return DISCONNECT_CODE;

}


//connects to router, recieves ack, and returns socked number of connection with router
//TODO: put the first half of this in a "connectToServer" function, possibly in a different file
int connectToRouter(char *domainName, int portno, char* servername){
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
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(portno);
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
        error("ERROR connecting");


    /* send message*/
    sendHeader(NEW_SERVER_CODE, NULL, NULL, servername, 0, sockfd);

    int n = read(sockfd, buffer, HEADER_LENGTH);

    //POSSIBE TODO: this will fail if only one byte is read on a partial read. Extremely unlikely, but could happen
    struct Header *headerBuf = (void *) buffer;
    enum message_type message_id = headerBuf->id;
    if (message_id == NEW_SERVER_ACK){
        return sockfd;
    }
    else if (message_id == ERROR_SERVER_EXISTS){
        error("inputted servername already exists");
    }
    else{
        error("unknown error connecting router, please try again");
    }
    return sockfd;


}


//ARGV arguments
//      port number to run on 
//      name of server
//      FQDN of router
//      port number of router
//      
int main(int argc, char *argv[])
{

    if (argc < 5) {
        fprintf(stderr,"ERROR, not enough arguments\n");
        exit(1);
    }

    int lSock = intializeLSock(atoi(argv[1]));

    struct sockaddr_in cli_addr;
    socklen_t clilen;
    clilen = sizeof(cli_addr);

    /* TODO: Talk with the router to say that you've come online */
    //int routerSock = connectToRouter(argv[2], atoi(argv[3]), argv[1])

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

    while (1){
        FD_ZERO(&copyFDSet);
        memcpy(&copyFDSet, &masterFDSet, sizeof(masterFDSet));
        rv = select (maxSock + 1, &copyFDSet, NULL, NULL, &tv);
   
        if (rv == -1)
            perror("Select");
        else if (rv ==0)
            //TODO: do a timeoutSearch
            ;
        else{
            //TODO: do a timeoutSearch
            for (int sockfd = 0; sockfd < maxSock + 1; sockfd++){
                //if there is any connection or new data to be read
                if ( FD_ISSET (sockfd, &copyFDSet) ) {
                    if (sockfd == lSock){
                        fprintf(stderr, "there's a new connection in town\n");
                        newSock = accept(sockfd, (struct sockaddr *) &cli_addr,  &clilen);
                        if (newSock > 0){
                            FD_SET(newSock, &masterFDSet);
                            maxSock = (newSock > maxSock) ? newSock: maxSock;
                        }
                        
                    }
                    else{
                        fprintf(stderr, "new info incoming\n" );
                        //if sockfd = routerSock
                            // addClient
                        int status = handleRequest(sockfd, handler);

                        if (status == DISCONNECT_CODE){
                            fprintf(stderr, "disconnecting client\n" );
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