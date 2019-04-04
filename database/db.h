/*
 * DB
 * Fileshare (Comp 112 Final Project)
 * Nathan Allen, Jonah Feldman, Patrick Kinsella
 * 1 April 2019
 */

#include <stdio.h>
#include <stdlib.h>
#include <libpq-fe.h>

#define db_t PGconn

#ifndef DB_STRUCTS
#define DB_STRUCTS
struct server_addr {
    int id;
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

db_t *connect_to_db(char *owner, char *database);

void on_db_error(db_t *db, PGresult *res);

int check_connection(db_t *db);

enum DB_STATUS exec_command(db_t *db, char *stm);

enum DB_STATUS create_table(db_t *db, char *tablename, char *columns);

enum DB_STATUS drop_table(db_t *db, char *tablename);

void close_db_connection(db_t *db);

struct db_return generate_dbr(enum DB_STATUS status, void *result);