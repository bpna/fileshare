/*
 * ServerTable
 * Fileshare (Comp 112 Final Project)
 * Nathan Allen, Jonah Feldman, Patrick Kinsella
 * 1 April 2019
 */

#include "db.h"

enum DB_STATUS add_server(db_t *db, struct server_addr *addr);

struct db_return clients_served_by(db_t *db, struct server_addr *addr);

struct db_return least_populated_server(db_t *db);

enum DB_STATUS increment_clients(db_t *db, struct server_addr *addr);

enum DB_STATUS get_server_id(db_t *db, struct server_addr *addr);
