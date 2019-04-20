/*
 * 03/31/2019
 * COMP 112 Final Project
 * Jonah Feldman, Nathan Allen, Patrick Kinsella
 *
 * The partial message handler interface allows any node reading from a
 * socket to store partial reads of arbitrary length <= 10000 bytes.
 * INIT_BUFFER_LENGTH and RW_LENGTH constants are also defined in this file
 * so clients of the partial message handler interface may initialize and read
 * into arrays the correct number of bytes passed to this interface.
 *
 * The partial message handler must be initialized with init_partials(), which
 * returns a pointer to struct PartialMessageHandler that must be passed
 * to other functions in the interface. It is a checked runtime error to pass
 * a NULL PartialMessageHandler * to any function in the interface.
 */

#include "file_saving_handler.h"
#include <sys/select.h>
#include <strings.h>
#define INIT_BUFFER_LENGTH 512
#define MINUTE 60000

/*
 * Call this function before calling other functions in the interface.
 * Pass it's result as the first argument to add_partial() and free_partials()
 */
struct PartialMessageHandler * init_partials();

/*
 * buffer is a pointer to bytes read by sockfd, length is the number of bytes
 * in buffer.
 *
 * returns -1 if an error was encountered.
 * returns 1 when partial message has been read in completely and TCP connection
 * can be closed, else if message not completely read in returns 0.  if is_file_input != 0,
 * a return value of 1 indicates the file is fully written to disk. if is_file_input == 0,
 * a return value of 1 indicates that the buffer has been modified in place to contain the entire
 * payload.
 *
 * Requirements: buffer >= length of entire message if  is_file_input == 0
 *
 * char is_file_input indicates if the buffer contains file data that needs to be written to disk
 * if is_file_input != 0, the buffer is written to disk.
 */
int add_partial(struct PartialMessageHandler *p, char *buffer, int sockfd,
                int length, char is_file_inpnut);

int get_partial_header(struct PartialMessageHandler *p, int sockfd,
                     char *headerBuf);

void free_partials(struct PartialMessageHandler *p);

int get_bytes_read(struct PartialMessageHandler *p, int sockfd);

void delete_partial(struct PartialMessageHandler *p, int sockfd);
void timeout_sweep(struct PartialMessageHandler *p, fd_set *masterFDSet);
