#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include "file_saving_handler.h"

/* purpose: right a file to disk of filename "fname", appended by a ~ to 
*   indicate it is a temporary file. If the function writes the last buffer for that
*   file, it removes the ~ from the filename and returns 1. -1 is returned on failure. Else, it returns 0
* Arguments:
*   fname: The filename to save (includes the directories)
*   buffer: buffer containing file contents
*   buf_len: lenght of the buffer. MUST be <= FILE_BUFFER_MAX_LEN
*/
int save_buffer(char *fname, char* buffer, unsigned int buf_len, unsigned long filelen){

    if (buf_len > FILE_BUFFER_MAX_LEN)
        return -1;
    //creates temporary fname
    char temp_fname[strlen(fname) + 2];
    strcpy(temp_fname, fname);
    temp_fname[strlen(fname)] = '~';
    temp_fname[strlen(fname) + 1] = '\0';

    unsigned long current_filelen;
    FILE *fp = fopen(temp_fname, "ab");
    current_filelen = ftell(fp);
    // fprintf(stderr, "file length is %lu\n", current_filelen);

    fwrite(buffer, 1, buf_len, fp);
    fclose(fp);

    fprintf(stderr, "current_filelen is %d\nbuf_len is %d\n", current_filelen, buf_len);
    fprintf(stderr, "filelen is %lu\n", filelen);
    if (current_filelen + buf_len == filelen){
        rename(temp_fname, fname);
        return 1;
    }
    return 0;


}

/*
* Purpose: deletes a partially written file (this is usually done when a partial message times out)
* Returns 0 on success, -1 on failure (usually fails if partial file does not exist)
*
*/
int delete_temp_file(char *fname){

    char temp_fname[strlen(fname) + 2];
    strcpy(temp_fname, fname);
    temp_fname[strlen(fname)] = '~';
    temp_fname[strlen(fname) + 1] = '\0';

    return remove(temp_fname);
}
