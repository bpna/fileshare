/*
 * CSPairs
 * Fileshare (Comp 112 Final Project)
 * Nathan Allen, Jonah Feldman, Patrick Kinsella
 * 1 April 2019
 */

#include <string.h>
#include "cspairs.h"
#include "servertable.h"

struct server_addr *find_server(db_t *db, char *client);

struct db_return get_server_from_client(db_t *db, char *client) {
    if (check_connection(db))
        return generate_dbr(CORRUPTED, NULL);

    char *stm = calloc(65, sizeof (char));
    sprintf(stm,
        "SELECT port, domain FROM cspairs WHERE name='%s'", client);

    PGresult *res = PQexec(db, stm);
    free(stm);

    if (PQntuples(res) == 0)
        return generate_dbr(SUCCESS, find_server(db, client));

    struct server_addr *addr =
        (struct server_addr *) malloc(sizeof (struct server_addr));

    // addr->id = get_id(addr);
    addr->port = atoi(PQgetvalue(res, 0, 0));
    strcpy(addr->domain_name, PQgetvalue(res, 0, 1));

    PQclear(res);

    return generate_dbr(SUCCESS, addr);
}

enum DB_STATUS add_cspair(db_t *db, char *client, struct server_addr *server) {
    if (check_connection(db))
        return CORRUPTED;

    char *stm = calloc(100, sizeof (char));
    sprintf(stm, "INSERT INTO cspairs VALUES('%s', %d, '%s')",
            client, server->port, server->domain_name);

    enum DB_STATUS status = exec_command(db, stm);
    return status;
}

int client_exists(db_t *db, char *client) {
    char *stm = calloc(65, sizeof (char));
    sprintf(stm, "SELECT * FROM cspairs WHERE name='%s'", client);

    PGresult *res = PQexec(db, stm);
    int result = PQntuples(res);

    free(stm);
    PQclear(res);

    return result;
}

struct server_addr *find_server(db_t *db, char *client) {
    return NULL;
}