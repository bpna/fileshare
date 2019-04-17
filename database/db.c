/*
 * DB
 * Fileshare (Comp 112 Final Project)
 * Nathan Allen, Jonah Feldman, Patrick Kinsella
 * 1 April 2019
 */

#include <string.h>
#include "db.h"

db_t *connect_to_db(char *owner, char *database) {
    char *stm = calloc(50, sizeof (char));
    sprintf(stm, "user=%s dbname=%s", owner, database);
    db_t *conn = PQconnectdb(stm);
    free(stm);
    if (check_connection(conn))
        return NULL;
    return conn;
}

int check_connection(db_t *db) {
    if (PQstatus(db) == CONNECTION_BAD) {
        PQfinish(db);
        return 1;
    }
    return 0;
}

enum DB_STATUS exec_command(db_t *db, char *stm) {
    if (check_connection(db))
        return CORRUPTED;

    PGresult *res = PQexec(db, stm);
    free(stm);
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        PQclear(res);
        return COMMAND_FAILED;
    }
    PQclear(res);
    return SUCCESS;
}

enum DB_STATUS create_table(db_t *db, char *tablename, char *columns,
                            char drop_existing) {
    PGresult *res;
    char *stm;
    if (check_connection(db))
        return CORRUPTED;

    if (drop_existing && drop_table(db, tablename))
        error("ERROR dropping table");
    else if (!drop_existing) {
        stm = calloc(6 + strlen(tablename), sizeof (char));
        sprintf(stm, "TABLE %s", tablename);
        res = PQexec(db, stm);
        free(stm);
        if (PQresultStatus(res) == PGRES_TUPLES_OK) {
            PQclear(res);
            return ELEMENT_ALREADY_EXISTS;
        }
        PQclear(res);
    }

    stm = calloc(16 + strlen(tablename) + strlen(columns), sizeof (char));
    sprintf(stm, "CREATE TABLE %s(%s)", tablename, columns);
    res = PQexec(db, stm);
    free(stm);
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        PQclear(res);
        return COMMAND_FAILED;
    }

    PQclear(res);
    return SUCCESS;
}

enum DB_STATUS drop_table(db_t *db, char *tablename) {
    char *stm = calloc(22 + strlen(tablename), sizeof (char));
    sprintf(stm, "DROP TABLE IF EXISTS %s", tablename);

    PGresult *res = PQexec(db, stm);
    free(stm);
    if (PQresultStatus(res) != PGRES_COMMAND_OK &&
        PQresultStatus(res) != PGRES_EMPTY_QUERY) {
        PQclear(res);
        return COMMAND_FAILED;
    }

    PQclear(res);
    return SUCCESS;
}

void close_db_connection(db_t *db) {
    PQfinish(db);
}

struct db_return generate_dbr(enum DB_STATUS status, void *result) {
    struct db_return dbr;
    dbr.status = status;
    dbr.result = result;
    return dbr;
}
