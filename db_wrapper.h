#include <stdint.h>
#include "database/db.h"

#define DB_OWNER "jfeldz"
#define DB_NAME "fileshare"
#define USE_DB 1
#define CSPAIRS_FNAME "client_cspairs.txt"
#define CSPAIRS_FILE_MAX_LENGTH 10000

struct Server {
    uint16_t port;
    char domain_name[64];
};

void add_cspair_wrapper(db_t *db, char *client, char *fqdn, unsigned port,
                        char *loc);
struct Server * get_server_from_client_wrapper(db_t *db, char *client,
                                               char *loc);

