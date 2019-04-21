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
