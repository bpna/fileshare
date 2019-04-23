#include <stdint.h>
#include "database/db.h"
#include "messages.h"


db_t *connect_to_db_wrapper();

void add_cspair_wrapper(db_t *db, char *client, char *fqdn, unsigned port,
                        char *loc, int increment_client);
struct Server * get_server_from_client_wrapper(db_t *db, char *client,
                                               char *loc);


/* checks out a given file to the given username. If the file does not
 * exist or the file is already checkout out, returns -1. Else. returns 0
 *
 */
char checkout_file_db_wrapper(char *requester, char *desired_filename);

/*
 * adds a filename to the filename databse, setting the checkout owner
 * to a NULL value
 */
void add_file_wrapper(char *filename);

/* 
 *Returns 1 if the requester has checked out a given file, 0 if not
 * Returns -1 if file does not exist on list
 */
char is_file_editor(char *requester, char *desired_filename);

/*
 * makes a given file availible for checkout
 */
void de_checkout_file(char *desired_filename);
