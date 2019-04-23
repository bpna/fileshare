/*
 * CSPairs
 * Fileshare (Comp 112 Final Project)
 * Nathan Allen, Jonah Feldman, Patrick Kinsella
 * 1 April 2019
 */

#include "db.h"

enum DB_STATUS create_cspairs_table(db_t db, char drop_existing);

struct db_return get_server_from_client(db_t db, char *client);

enum DB_STATUS add_cspair(db_t db, char *client, struct Server *server,
                          int increment_client);

enum DB_STATUS add_backup_cspair(db_t db, char *client,
                                 struct Server *server);

int client_exists(db_t db, char *client);

struct db_return get_user_list(db_t db, char **list);
