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

//linked list of partial messages
struct PartialNode{
    int bytesRead;
    char header[70];
    char *body;
    int sockfd;
    unsigned int lastUpdated;

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
//reads in a request
//in case of either ERROR or SUCCESS, return DISCONNECT CODE. only returns 0 in case of partial read

int handleRequest(int sockfd, struct PartialNode * head){
    //TODO: Handle partial messages

    /*if partial message 
        if need to read more
            read
            return 0
        elif complete
            continue to switch
    else:
        do a read, return
        return 0
    */
    char buffer[HEADER_LENGTH];
    int n = read(sockfd, buffer, HEADER_LENGTH);
    struct header* msgHeader = (void *)buffer;

    switch(msgHeader->msgType){
        case 1:
            //add to SQL database, possibly verify should exist from router
            break;
        case 2:
            //verify creds from SQL database, read, write file to Disk, send ACK
            break;
        case 3:
            //verify creds, verify file exists, send back file
            break;
        case 4:
            //verify creds, verify file exists, overwrite file, send ACK
            break;
        case 5:
            //Verify creds, verify they have permission to do this, update permission via SQL, return ACK
            break;
        case 6:
            //Verify creds, verify they have permission to do this, update permission via SQL, return ACK
            break;

    }


}

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
            //do a timeoutSearch
            ;
        else{
            for (int sockfd = 0; sockfd < maxSock + 1; sockfd++){
                //if there is any connection or new data to be read
                if ( FD_ISSET (sockfd, &copyFDSet) ) {
                    if (sockfd == lSock){
                        newSock = accept(sockfd, (struct sockaddr *) &cli_addr,  &clilen);
                        if (newSock > 0){
                            FD_SET(newSock, &masterFDSet);
                            maxSock = (newSock > maxSock) ? newSock: maxSock;
                            //add to sockArray
                        }
                        
                    }
                    else{
                        int status = handleRequest(sockfd, head);

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