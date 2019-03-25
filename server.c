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

#define HEADER_LENGTH 50
#define DISCONNECT_CODE -69
#define MAX_NUM_CLIENTS 200
#define MINUTE 60000

void error(const char *msg)
{
    perror(msg);
    exit(1);
} 



int main(int argc, char *argv[])
{
   // int headerLength = (sizeof(int) * 2) + 42;

    if (argc < 2) {
        fprintf(stderr,"ERROR, no port provided\n");
        exit(1);
    }

    int lSock = intializeLSock(atoi(argv[1]));


    


    struct sockaddr_in cli_addr;
    socklen_t clilen;
    clilen = sizeof(cli_addr);


    int maxSock, rv, newSock = -1;
    struct timeval tv;
    tv.tv_sec=1;
    tv.tv_usec=1000;
    fd_set masterFDSet, copyFDSet;
    FD_ZERO(&masterFDSet);
    FD_ZERO(&copyFDSet);
    FD_SET(lSock, &masterFDSet);

    maxSock = lSock;
    struct ClientList *cList = ClientListConstructor();
    struct partialMessage **partialList = calloc(MAX_NUM_CLIENTS, sizeof(struct partialMessage *));

    while (1){
        FD_ZERO(&copyFDSet);
        memcpy(&copyFDSet, &masterFDSet, sizeof(masterFDSet));
        rv = select (maxSock + 1, &copyFDSet, NULL, NULL, &tv);
   
        if (rv == -1)
            perror("Select");
        else if (rv ==0)
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
                        int status = readMessage(sockfd, cList, partialList);

                        if (status == DISCONNECT_CODE){
                            close(sockfd);
                            //write a disconnect function
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