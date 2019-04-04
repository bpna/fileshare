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

#include "file_saving_handler.h"


int main(int argc, char *argv[]){
    // FILE *fp = fopen(argv[1],"rb");

    // fseek( fp, 0, SEEK_END );
    // long filelen = ftell(fp);

    // long bytesLeft = filelen;
    // fseek( fp, 0,  SEEK_SET);
    // char buffer[512];

    // //for testing, meant to see if temp file is created

    // while (bytesLeft > 0){
    //     int n = fread(buffer, 1, 512, fp);
    //     bytesLeft -= n;
    //     save_buffer(argv[1], buffer, n, filelen);
    // }
    // fclose(fp);
    // return 0;

    delete_temp_file(argv[1]);

    
    
}