#include <stdint.h>
#include "database/db.h"


struct Server {
    uint16_t port;
    char domain_name[256];
};

db_t *connect_to_db_wrapper();

void add_cspair_wrapper(db_t *db, char *client, char *fqdn, unsigned port,
                        char *loc);
struct Server * get_server_from_client_wrapper(db_t *db, char *client,
                                               char *loc);


/* checks out a given file to the given username. If the file does not
 * exist or the file is already checkout out, returns -1. Else. returns 0
 *
 */
char checkout_file_db_wrapper(char *username, char * filename);

/*
 * adds a filename to the filename databse, setting the checkout owner
 * to a NULL value
 */
void add_file_wrapper(char *filename);