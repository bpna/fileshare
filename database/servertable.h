/*
 * ServerTable
 * Fileshare (Comp 112 Final Project)
 * Nathan Allen, Jonah Feldman, Patrick Kinsella
 * 1 April 2019
 */

#include "db.h"

enum DB_STATUS create_server_table(db_t *db, char drop_existing);

enum DB_STATUS add_server(db_t *db, struct Server *addr, int personal);

struct db_return clients_served_by(db_t *db, struct Server *addr);

struct db_return least_populated_server(db_t *db);

enum DB_STATUS increment_clients(db_t *db, struct Server *addr);

enum DB_STATUS get_server_name(db_t *db, struct Server *addr);

struct db_return get_server_from_name(db_t *db, char *server);
