/*
 * ServerTable
 * Fileshare (Comp 112 Final Project)
 * Nathan Allen, Jonah Feldman, Patrick Kinsella
 * 1 April 2019
 */

#include <string.h>
#include "servertable.h"

enum DB_STATUS add_server(db_t *db, struct server_addr *addr) {
    if (check_connection(db))
        return CORRUPTED;

    //TODO: check if port-domain pair is in db

    char *stm = calloc(100, sizeof (char));
    sprintf(stm, "INSERT INTO servers VALUES(%s, %d, '%s', 0, 0)",
            addr->name, addr->port, addr->domain_name);

    return exec_command(db, stm);
}

struct db_return clients_served_by(db_t *db, struct server_addr *addr) {
    if (check_connection(db))
        return generate_dbr(CORRUPTED, NULL);

    char *stm = calloc(100, sizeof (char));
    sprintf(stm, "SELECT count(*) FROM cspairs \
                  WHERE port=%d AND domain='%s'",
            addr->port, addr->domain_name);

    PGresult *res = PQexec(db, stm);

    if (PQntuples(res) == 0) {
        free(stm);
        return generate_dbr(ELEMENT_NOT_FOUND, NULL);
    }

    long count = atol(PQgetvalue(res, 0, 0));

    free(stm);
    PQclear(res);

    return generate_dbr(SUCCESS, (void *) count);
}

struct db_return least_populated_server(db_t *db) {
    if (check_connection(db))
        return generate_dbr(CORRUPTED, NULL);

    char *stm = calloc(100, sizeof (char));
    sprintf(stm,
        "SELECT port, domain, COUNT(*) FROM cspairs \
         GROUP BY port, domain ORDER BY 2 ASC;");

    PGresult *res = PQexec(db, stm);

    if (PQntuples(res) == 0) {
        free(stm);
        return generate_dbr(ELEMENT_NOT_FOUND, NULL);
    }

    struct server_addr *addr =
        (struct server_addr *) malloc(sizeof (struct server_addr));

    addr->port = atoi(PQgetvalue(res, 0, 0));
    strcpy(addr->domain_name, PQgetvalue(res, 0, 1));

    free(stm);
    PQclear(res);

    return generate_dbr(get_server_name(db, addr), addr);
}

enum DB_STATUS increment_clients(db_t *db, struct server_addr *addr) {
    char *stm = calloc(350, sizeof (char));
    sprintf(stm, "SELECT clients FROM servers WHERE port=%d AND domain='%s'",
            addr->port, addr->domain_name);

    PGresult *res = PQexec(db, stm);

    if (PQntuples(res) == 0) {
        free(stm);
        return ELEMENT_NOT_FOUND;
    }

    int clients = atoi(PQgetvalue(res, 0, 0));
    PQclear(res);

    clients++;

    sprintf(stm, "UPDATE servers SET clients=%d WHERE port=%d AND domain='%s'",
            clients, addr->port, addr->domain_name);

    res = PQexec(db, stm);
    free(stm);
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        on_db_error(db, res);
        return COMMAND_FAILED;
    }

    PQclear(res);

    return SUCCESS;
}

enum DB_STATUS get_server_name(db_t *db, struct server_addr *addr) {
    char *stm;
    PGresult *res;
    if (check_connection(db))
        return CORRUPTED;

    stm = calloc(350, sizeof (char));
    sprintf(stm, "SELECT id FROM servers WHERE port=%d AND domain='%s'",
            addr->port, addr->domain_name);
    res = PQexec(db, stm);
    free(stm);

    if (PQntuples(res) == 0)
        return ELEMENT_NOT_FOUND;

    strcpy(addr->name, PQgetvalue(res, 0, 0));
    PQclear(res);

    return SUCCESS;
}
