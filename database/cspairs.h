/*
 * CSPairs
 * Fileshare (Comp 112 Final Project)
 * Nathan Allen, Jonah Feldman, Patrick Kinsella
 * 1 April 2019
 */

#include "db.h"

/*
 * create_cspairs_table
 *
 * Arguments:
 *     db_t db: database connection variable
 *     char drop_existing: bool representing whether or not to drop the table if
 *                         it already exists
 * Returns:
 *     command status: SUCCESS, CORRUPTED, COMMAND_FAILED,
 *                     ELEMENT_ALREADY_EXISTS
 */
enum DB_STATUS create_cspairs_table(db_t db, char drop_existing);

/*
 * get_server_from_client
 *
 * Arguments:
 *     db_t db: database connection variable
 *     char *client: client to get information from
 * Returns:
 *     db_return struct with status and Server struct containing server info
 *     status is ELEMENT_NOT_FOUND if client's info is not in the local database
 *
 *     SUCCESS, CORRUPTED, ELEMENT_NOT_FOUND
 */
struct db_return get_server_from_client(db_t db, char *client);

/*
 * add_cspair
 *
 * Arguments:
 *     db_t db: database connection variable
 *     char *client: client to add
 *     struct Server *server: struct containing server info
 *     char increment_client: bool representing whether or not to increment
 *                            clients in server table
 * Returns:
 *     command status: SUCCESS, CORRUPTED, COMMAND_FAILED,
 *                     ELEMENT_NOT_FOUND
 */
enum DB_STATUS add_cspair(db_t db, char *client, struct Server *server,
                          char increment_client);

/*
 * add_backup_cspair
 *
 * Arguments:
 *     db_t db: database connection variable
 *     char *client: client to add
 *     struct Server *server: struct containing backup server info
 * Returns:
 *     command status: SUCCESS, CORRUPTED, COMMAND_FAILED
 */
enum DB_STATUS add_backup_cspair(db_t db, char *client,
                                 struct Server *server);

/*
 * client_exists
 *
 * Arguments:
 *     db_t db: database connection variable
 *     char *client: client in question
 * Returns:
 *     int representing whether or not client exists
 */
int client_exists(db_t db, char *client);

/*
 * get_user_list
 *
 * Arguments:
 *     db_t db: database connection variable
 *     char **list: pointer to char * that will point to allocated client list
 * Returns:
 *     db_return struct with status and long representing the length of the
 *     string created
 *
 *     SUCCESS, CORRUPTED
 */
struct db_return get_user_list(db_t db, char **list);
