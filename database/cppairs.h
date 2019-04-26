/*
 * CPPairs
 * Fileshare (Comp 112 Final Project)
 * Nathan Allen, Jonah Feldman, Patrick Kinsella
 * 1 April 2019
 */

#include "db.h"

enum DB_STATUS add_cppair(db_t *db, char *client, char *pass);

int valid_authentication(db_t *db, char *client, char *pass);
