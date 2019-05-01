/*
 * CPPairs
 * Fileshare (Comp 112 Final Project)
 * Nathan Allen, Jonah Feldman, Patrick Kinsella
 * 1 April 2019
 */

#include <string.h>
#include "cppairs.h"

enum DB_STATUS create_cppairs_table(db_t db, char drop_existing) {
    return create_table(db, "cppairs", "Client VARCHAR(20) PRIMARY KEY, \
                                        Password VARCHAR(20)", drop_existing);
}

enum DB_STATUS add_cppair(db_t db, char *client, char *pass) {
    if (check_connection(db))
        return CORRUPTED;

    char *stm = calloc(100, sizeof (char));
    sprintf(stm, "INSERT INTO cppairs VALUES('%s', '%s')",
            client, pass);

    PGresult *res = PQexec(db, stm);
    free(stm);
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        PQclear(res);
        return COMMAND_FAILED;
    }

    PQclear(res);

    return SUCCESS;
}

char valid_authentication(db_t db, char *client, char *pass) {
    char *stm = calloc(70, sizeof (char));
    sprintf(stm, "SELECT password FROM cppairs WHERE client='%s'", client);

    PGresult *res = PQexec(db, stm);
    free(stm);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        PQclear(res);
        return 1;
    }

    char result = !strcmp(pass, PQgetvalue(res, 0, 0));
    PQclear(res);

    return result;
}
