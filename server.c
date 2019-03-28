#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>

#include <sys/time.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>

#define HEADER_LENGTH 70
#define DISCONNECT_CODE -69
#define MAX_NUM_CLIENTS 200
#define MINUTE 60000
#define UPLOAD_FILE_CODE 256
#define UPLOAD_ACK_CODE 257
#define ERROR_FILE_EXISTS_CODE 258
#define ERROR_UPLOAD_FAILURE_CODE 259
#define REQUEST_FILE_CODE 260
#define RETURN_FILE_CODE 261
#define UPDATE_FILE_CODE 262
#define UPDATE_ACK_CODE 263
#define ERROR_FILE_DOES_NOT_EXIST_CODE 264


//TODO: if you eliminate a timed-out partial for upload, make sure to delete file that was being written 
//linked list of partial messages
struct PartialNode{
    int bytesRead;
    char header[70];
    char body[256];
    int sockfd;
    unsigned int lastUpdated;
    struct PartialNode *next;

};


struct __attribute__((__packed__)) header {
    unsigned short msgType;
    char user[20];
    char pwd[20];
    char fname[20];
    unsigned long len;
};

void error(const char *msg)
{
    perror(msg);
    exit(1);
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

//creates a parital message node, and attaches it to head of linked list
struct PartialNode * createPartial(int sockfd, struct PartialNode ** head, char *buffer, int n){

    struct PartialNode *newHead = malloc(sizeof(struct PartialNode));
    newHead->sockfd = sockfd;
    newHead->bytesRead = n;
    newHead->lastUpdated = time(NULL);
    bzero(newHead->body, 256);
    bzero(newHead->header, HEADER_LENGTH);
    memcpy(newHead->header, buffer, n);
    newHead->next = *head;

    //sets the head pointer to now point to new head
    *head = newHead;
    return newHead;

}

//given a sockfd to search by, searches the linked list of partial messages
//returns the Node is found, returns NULL if node does not exist

struct PartialNode *findPartial(int sockfd, struct PartialNode * head){

    struct PartialNode * temp = head;
    while (temp != NULL){
        if (temp->sockfd == sockfd){
            return temp;
        }
        temp = temp->next;
    }
    return NULL;


}

//reads in a request
//in case of either ERROR or SUCCESS, return DISCONNECT CODE. only returns 0 in case of partial read

int handleRequest(int sockfd, struct PartialNode ** head){

    char buffer[HEADER_LENGTH];
    bzero(buffer, HEADER_LENGTH);
    struct PartialNode *node = findPartial(sockfd, *head);
    char did_read = 0;
    if (node == NULL){
        did_read = 1;
        int n = read(sockfd, buffer, HEADER_LENGTH);
        node = createPartial(sockfd, head, buffer, n);
    }
    else if (node->bytesRead < HEADER_LENGTH){
        did_read = 1;
        int n = read(sockfd, buffer, HEADER_LENGTH - node->bytesRead);
        memcpy(&node->header[node->bytesRead], buffer, n);        
        node->lastUpdated = time(NULL);
        node->bytesRead += n;

    }

    if (node->bytesRead < 50){
        return 0;
    }
    struct header* msgHeader = (void *) node->header;

    /* TODO: Deal with endianess */
    

    switch(msgHeader->msgType){
        case UPLOAD_FILE_CODE:
            if (did_read != 1){
                //verify creds from SQL database, verify file doesn't exist, read, write file to Disk, send ACK
            }
            
            break;
        case REQUEST_FILE_CODE:
            //verify creds, verify file exists, send back file
            break;
        case UPDATE_FILE_CODE:
            if (did_read != 1){

             //verify creds from SQL database, verify file doesn't exist, read, write file to Disk, send ACK
            }
            break;
            //TODO in future: add in permision cases
    }




}


//ARGV arguments
//      port number to run on 
//      name of server
//      FQDN of router
//      port number of router
//      
int main(int argc, char *argv[])
{

    if (argc < 2) {
        fprintf(stderr,"ERROR, no port provided\n");
        exit(1);
    }

    int lSock = intializeLSock(atoi(argv[1]));

    struct sockaddr_in cli_addr;
    socklen_t clilen;
    clilen = sizeof(cli_addr);

    /* TODO: Talk with the router to say that you've come online */

    int maxSock, rv, newSock = -1;
    struct timeval tv;
    tv.tv_sec=1;
    tv.tv_usec=1000;
    fd_set masterFDSet, copyFDSet;
    FD_ZERO(&masterFDSet);
    FD_ZERO(&copyFDSet);
    FD_SET(lSock, &masterFDSet);

    maxSock = lSock;
    struct PartialNode *head = NULL;

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
                        newSock = accept(sockfd, (struct sockaddr *) &cli_addr,  &clilen);
                        if (newSock > 0){
                            FD_SET(newSock, &masterFDSet);
                            maxSock = (newSock > maxSock) ? newSock: maxSock;
                        }
                        
                    }
                    else{
                        int status = handleRequest(sockfd, &head);

                        if (status == DISCONNECT_CODE){

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