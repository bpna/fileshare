#include "messages.h"

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
#include "messages.h"

#define INIT_BUFFER_LENGTH 20000
#define RW_LENGTH 10000

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
 * returns 0 when buffer was added to the handler, but those bytes should not
 * yet be written to disk (< 10000 bytes to write && server has more to read).
 * returns n > 0 when n bytes should be written to disk, buffer is modified
 * in-place.
 */
int add_partial(struct PartialMessageHandler *p, char *buffer, int sockfd,
                int length);

int getPartialHeader(struct PartialMessageHandler *p, int sockfd, 
                     struct Header *headerBuf);

void free_partials(struct PartialMessageHandler *p);
