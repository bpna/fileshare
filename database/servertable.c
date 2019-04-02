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
    sprintf(stm, "INSERT INTO servers VALUES(%d, %d, '%s', 0, 0)",
            addr->id, addr->port, addr->domain_name);

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

    struct server_addr *addr =
        (struct server_addr *) malloc(sizeof (struct server_addr));

    addr->port = atoi(PQgetvalue(res, 0, 0));
    strcpy(addr->domain_name, PQgetvalue(res, 0, 1));

    free(stm);
    PQclear(res);

    return get_server_id(db, addr);
}

enum DB_STATUS increment_clients(db_t *db, struct server_addr *addr) {
    char *stm = calloc(100, sizeof (char));
    sprintf(stm, "SELECT clients FROM servers WHERE port=%d AND domain='%s'",
            addr->port, addr->domain_name);

    PGresult *res = PQexec(db, stm);

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

struct db_return get_server_id(db_t *db, struct server_addr *addr) {
    if (check_connection(db))
        return generate_dbr(CORRUPTED, NULL);
    addr->id = 1; //TODO: actually write this

    return generate_dbr(SUCCESS, addr);
}
