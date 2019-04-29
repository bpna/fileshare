/*
 * ServerTable
 * Fileshare (Comp 112 Final Project)
 * Nathan Allen, Jonah Feldman, Patrick Kinsella
 * 1 April 2019
 */

#include "db.h"

/*
 * create_server_table
 *
 * Arguments:
 *     db_t db: database connection variable
 *     char drop_existing: bool representing whether or not to drop the table if
 *                         it already exists
 * Returns:
 *     command status: SUCCESS, CORRUPTED, COMMAND_FAILED,
 *                     ELEMENT_ALREADY_EXISTS
 */
enum DB_STATUS create_server_table(db_t db, char drop_existing);

/*
 * add_server
 *
 * Arguments:
 *     db_t db: database connection variable
 *     struct Server *addr: struct containing server info
 *     char personal: bool representing whether or not the server is a personal
 *                    server
 * Returns:
 *     command status: SUCCESS, CORRUPTED, COMMAND_FAILED
 */
enum DB_STATUS add_server(db_t db, struct Server *addr, char personal);

/*
 * clients_served_by
 *
 * Arguments:
 *     db_t db: database connection variable
 *     struct Server *addr: struct containing server info
 * Returns:
 *     db_return struct with status and long representing the number of clients
 *     served by the specified server
 *
 *     SUCCESS, CORRUPTED, ELEMENT_NOT_FOUND
 */
struct db_return clients_served_by(db_t db, struct Server *addr);

/*
 * least_populated_server
 *
 * Arguments:
 *     db_t db: database connection variable
 * Returns:
 *     db_return struct with status and pointer to allocated Server struct with
 *     server info
 *
 *     SUCCESS, CORRUPTED, ELEMENT_NOT_FOUND
 */
struct db_return least_populated_server(db_t db);

/*
 * increment_clients
 *
 * Arguments:
 *     db_t db: database connection variable
 *     struct Server *addr: struct containing server info
 * Returns:
 *     command status: SUCCESS, CORRUPTED, COMMAND_FAILED,
 *                     ELEMENT_NOT_FOUND
 */
enum DB_STATUS increment_clients(db_t db, struct Server *addr);

/*
 * get_server_name
 *
 * Arguments:
 *     db_t db: database connection variable
 *     struct Server *addr: struct containing server info
 * Returns:
 *     command status: SUCCESS, CORRUPTED, ELEMENT_NOT_FOUND
 * Notes:
 *     fills out server name in addr
 */
enum DB_STATUS get_server_name(db_t db, struct Server *addr);

/*
 * get_server_from_name
 *
 * Arguments:
 *     db_t db: database connection variable
 *     char *server: name of server
 * Returns:
 *     db_return struct with status and pointer to allocated Server struct with
 *     server info
 *
 *     SUCCESS, CORRUPTED, ELEMENT_NOT_FOUND
 */
struct db_return get_server_from_name(db_t db, char *server);
