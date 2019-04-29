/*
 * DB
 * Fileshare (Comp 112 Final Project)
 * Nathan Allen, Jonah Feldman, Patrick Kinsella
 * 1 April 2019
 */

#include <stdio.h>
#include <stdlib.h>
#include <postgresql/libpq-fe.h>
#include "../io.h"

#define db_t PGconn *
#define DB_OWNER "postgres"
#define DB_NAME "comp112-fileshare:us-east1:myinstance"
#define DB_ADDR "35.231.141.209"

#ifndef DB_STRUCTS
#define DB_STRUCTS
struct Server {
    char name[20];
    short port;
    char domain_name[64];
};

enum DB_STATUS {
    SUCCESS,
    CORRUPTED,
    COMMAND_FAILED,
    INVALID_AUTHENTICATION,
    ELEMENT_NOT_FOUND,
    ELEMENT_ALREADY_EXISTS
};

struct db_return {
    enum DB_STATUS status;
    void *result;
};
#endif

/*
 * connect_to_db
 *
 * Arguments:
 *     char *owner: name of owner
 *     char *database: name of psql database on local machine
 * Returns:
 *     db_t representing database connection variable
 *     NULL on error
 */
db_t connect_to_db(char *owner, char *database, char *hostaddr);

/*
 * check_connection
 *
 * Arguments:
 *     db_t db: database connection variable
 * Returns:
 *     0 on good connection, 1 on bad connection
 */
int check_connection(db_t db);

/*
 * exec_command
 *
 * Arguments:
 *     db_t db: database connection variable
 *     char *stm: psql command to be executed
 * Returns:
 *     command status: SUCCESS, CORRUPTED, COMMAND_FAILED
 * Notes:
 *     passed command cannot return any values
 */
enum DB_STATUS exec_command(db_t db, char *stm);

/*
 * create_table
 *
 * Arguments:
 *     db_t db: database connection variable
 *     char *tablename: name of table to create
 *     char *columns: string of table columns
 *     char drop_existing: bool representing whether or not to drop the table if
 *                         it already exists
 * Returns:
 *     command status: SUCCESS, CORRUPTED, COMMAND_FAILED,
 *                     ELEMENT_ALREADY_EXISTS
 */
enum DB_STATUS create_table(db_t db, char *tablename, char *columns,
                            char drop_existing);

/*
 * drop_table
 *
 * Arguments:
 *     db_t db: database connection variable
 *     char *tablename: name of table to drop
 * Returns:
 *     command status: SUCCESS, CORRUPTED, COMMAND_FAILED
 */
enum DB_STATUS drop_table(db_t db, char *tablename);

/*
 * close_db_connection
 *
 * Arguments:
 *     db_t db: database connection variable
 */
void close_db_connection(db_t db);

/*
 * generate_dbr
 *
 * Arguments:
 *     enum DB_STATUS status: command status to put in struct
 *     void *result: result to put in struct
 * Returns:
 *     db_return struct with status and result specified
 *
 */
struct db_return generate_dbr(enum DB_STATUS status, void *result);
