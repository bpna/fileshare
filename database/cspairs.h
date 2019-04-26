/*
 * CSPairs
 * Fileshare (Comp 112 Final Project)
 * Nathan Allen, Jonah Feldman, Patrick Kinsella
 * 1 April 2019
 */

#include "db.h"

struct db_return get_server_from_client(db_t *db, char *client);

enum DB_STATUS add_cspair(db_t *db, char *client, struct server_addr *server);

int client_exists(db_t *db, char *client);
