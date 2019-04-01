#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <sys/select.h>

#include <sys/time.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>

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


struct __attribute__((__packed__)) header {
    unsigned char msgType;
    char user[20];
    char pwd[20];
    char fname[20];
    int len;
};

void error(const char *msg)
{
    perror(msg);
    exit(1);
} 

//returns an allocated header
struct header *makeHeader(int msgType, char *user, char * pwd, char* fname, int len){
    struct header *myHeader = malloc(sizeof(struct header));
    myHeader->msgType = msgType;
    memcpy(myHeader->user, user, 20);
    memcpy(myHeader->pwd, pwd, 20);
    memcpy(myHeader->fname, fname, 20);
    myHeader->len = len;
    return myHeader;

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
char uploadFile(int sockfd, struct *header msgHeader){
    //TODO: if file exists, send ERROR message return DISCONNECT CODE
    
    //TODO: verify filename has no special characteres except "."
    int bytesToRead = 10000
    char buffer[bytesRead];
    int bytesRead = getBytesRead(sockfd);

    if(msgHeader->len  - bytesRead < bytesToRead){
        bytesToRead = msgHeader->len  - bytesRead
    }

    //TODO: prepend the file with a special character so we know it isn't "ready"
    FILE *fp = fopen(msgHeader->fname, "a");

    int n = read(sockfd, buffer, bytesToRead);

    if (n == 0){
        //TODO:delete partial, right here
        return DISCONNECT_CODE;
    }


    buffer = addPartial(buffer, sockfd, n);
    if (n + bytesRead == msgHeader->len){
        fwrite(buffer)
    }
    if (n != 0){
        fwrite(buffer, 1, n, fp);
    }
    if (n == 10000){
        return 0;
    }

    return DISCONNECT_CODE;


}

//reads in a request
//in case of either ERROR or SUCCESS, return DISCONNECT CODE. only returns 0 in case of partial read

int handleRequest(int sockfd){
    int n, headerBytesRead;
    char buffer[HEADER_LENGTH];
    bzero(buffer, HEADER_LENGTH);
    struct header *msgHeader;
    char did_read;
    headerBytesRead = getPartialHeader(msgHeader, sockfd);

    //TODO: put functions inside of these, this is paralell code
    if (headerBytesRead == 0){
        n = read(sockfd, buffer, HEADER_LENGTH);
        if (n < HEADER_LENGTH){
            addPartial(buffer, sockfd, n);
            return 0;
        }
        else{
            memcpy(msgHeader, buffer, HEADER_LENGTH);
            if (msgHeader->len > 0){
                addPartial(buffer, sockfd, n);
                return 0;
            }
        }

    }
    else if (headerBytesRead < HEADER_LENGTH){
        n = read(sockfd, buffer, HEADER_LENGTH - headerBytesRead);
        if (n < HEADER_LENGTH - headerBytesRead){
            addPartial(buffer, sockfd, n);
            return 0;
        }
        else{
            memcpy(&msgHeader[headerBytesRead], buffer, HEADER_LENGTH - headerBytesRead);
            if (msgHeader->len > 0){
                addPartial(buffer, sockfd, n);
                return 0;
            }
        }
    }


    fprintf(stderr, "the number of bytes read in was %d\n", n);
    fprintf(stderr, "the type of message incoming is %d\n", msgHeader->msgType);
    fprintf(stderr, "the username incoming is %s\n", msgHeader->user);
    fprintf(stderr, "the password incoming is %s\n", msgHeader->pwd);
    fprintf(stderr, "the fname incoming is %s\n", msgHeader->fname);
    fprintf(stderr, "the length incoming is %d\n", msgHeader->len);


    /* TODO: Deal with endianess */
    

    switch(msgHeader->msgType){
        case UPLOAD_FILE_CODE:
            return uploadFile(sockfd, msgHeader);
              //  return uploadFile(sockfd, node, head);
                //verify creds from SQL database, verify file doesn't exist, read, write file to Disk, send ACK
            
            break;
        case REQUEST_FILE_CODE:
            //verify creds, verify file exists, send back file
            break;
        case UPDATE_FILE_CODE:

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
int connectToRouter(char *domainName, int portno, char* serverName){
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
    char empty[20];
    bzero(empty, 20);

    struct header *headerBuf = makeHeader(NEW_SERVER_CODE, empty, empty, serverName, 0);


    int n = write(sockfd, (void *)headerBuf, HEADER_LENGTH);
    free(headerBuf);
    bzero(headerBuf, HEADER_LENGTH);
    n = read(sockfd, buffer, HEADER_LENGTH);
    //POSSIBE TODO: this will fail if only one byte is read on a partial read. Extremely unlikely, but could happen
    headerBuf = (void *) buffer;
    if (headerBuf->msgType == NEW_SERVER_ACK_CODE){
        return sockfd;
    }
    else if (headerBuf->msgType == ERROR_SERVER_EXISTS_CODE){
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
                        int status = handleRequest(sockfd, &head);

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