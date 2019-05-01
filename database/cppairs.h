/*
 * CPPairs
 * Fileshare (Comp 112 Final Project)
 * Nathan Allen, Jonah Feldman, Patrick Kinsella
 * 1 April 2019
 */

#include "db.h"

/*
 * create_cppairs_table
 *
 * Arguments:
 *     db_t db: database connection variable
 *     char drop_existing: bool representing whether or not to drop the table if
 *                         it already exists
 * Returns:
 *     command status: SUCCESS, CORRUPTED, COMMAND_FAILED,
 *                     ELEMENT_ALREADY_EXISTS
 */
enum DB_STATUS create_cppairs_table(db_t db, char drop_existing);

/*
 * add_cppair
 *
 * Arguments:
 *     db_t db: database connection variable
 *     char *client: client to add
 *     char *pass: client password
 * Returns:
 *     command status: SUCCESS, CORRUPTED, COMMAND_FAILED
 */
enum DB_STATUS add_cppair(db_t db, char *client, char *pass);

/*
 * valid_authentication
 *
 * Arguments:
 *     db_t db: database connection variable
 *     char *client: client in question
 *     char *pass: given password
 * Returns:
 *     int representing validity of given password
 */
char valid_authentication(db_t db, char *client, char *pass);
