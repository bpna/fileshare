#include "messages.h"
#define FILE_BUFFER_MAX_LEN 10000

/*
 * 03/31/2019
 * COMP 112 Final Project
 * Jonah Feldman, Nathan Allen, Patrick Kinsella
 *
 * This module is meant to handle writing files to disk that are coming in over
 * a TCP connction. Because the file might be transferred at seperated intervals, this
 * module abstracts away needing to open and close a file multiple times.
 *
 * This module is a sub-module of the partial_message_handler module
 */


/*
* purpose: right a file to disk of filename "fname", appended by a ~ to 
*   indicate it is a temporary file. If the function writes the last buffer for that
*   file, it removes the ~ from the filename and returns 1. Else, it returns 0
* Arguments:
*   fname: The filename to save (includes the directories)
*   buffer: buffer containing file contents
*   buf_len: lenght of the buffer. MUST be <= FILE_BUFFER_MAX_LEN
*/
int save_buffer(char *fname, char* buffer, unsigned int buf_len, unsigned long filelen);

/*
* Purpose: deletes a partially written file (this is usually done when a partial message times out)
* Returns 0 on success, 1 on failure (usually fails if partial file does not exist)
*
*/
int delete_temp_file(char *fname);