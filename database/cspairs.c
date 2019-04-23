/*
 * CSPairs
 * Fileshare (Comp 112 Final Project)
 * Nathan Allen, Jonah Feldman, Patrick Kinsella
 * 1 April 2019
 */

#include <string.h>
#include "cspairs.h"
#include "servertable.h"

enum DB_STATUS create_cspairs_table(db_t *db, char drop_existing) {
    return create_table(db, "cspairs", "Name VARCHAR(20) PRIMARY KEY, \
                                        Port SMALLINT, Domain VARCHAR(255), \
                                        Backup_Port SMALLINT, Backup_Domain \
                                        VARCHAR(255)",
                                        drop_existing);
}

struct db_return get_server_from_client(db_t *db, char *client) {
    if (check_connection(db))
        return generate_dbr(CORRUPTED, NULL);

    char *stm = calloc(65, sizeof (char));
    sprintf(stm,
        "SELECT port, domain FROM cspairs WHERE name='%s'", client);

    PGresult *res = PQexec(db, stm);
    free(stm);

    if (PQntuples(res) == 0)
        return generate_dbr(ELEMENT_NOT_FOUND, NULL);

    struct Server *addr =
        (struct Server *) malloc(sizeof (struct Server));

    addr->port = atoi(PQgetvalue(res, 0, 0));
    strcpy(addr->domain_name, PQgetvalue(res, 0, 1));

    PQclear(res);

    return generate_dbr(SUCCESS, addr);
}

enum DB_STATUS add_cspair(db_t *db, char *client, struct Server *server,
                          int increment_client) {
    if (check_connection(db))
        return CORRUPTED;

    char *stm = calloc(100, sizeof (char));
    sprintf(stm, "INSERT INTO cspairs VALUES('%s', %d, '%s')",
            client, server->port, server->domain_name);

    enum DB_STATUS status = exec_command(db, stm);
    if (status || !increment_client)
        return status;
    else
        return increment_clients(db, server);
}

enum DB_STATUS add_backup_cspair(db_t *db, char *client,
                                 struct Server *server)  {
    if (check_connection(db)) {
        return CORRUPTED;
    }

    char *stm = calloc(100, sizeof (char));
    sprintf(stm, "UPDATE cspairs SET backup_port=%d, backup_domain='%s' WHERE \
                  name='%s'", server->port, server->domain_name, client);

    return exec_command(db, stm);
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

struct db_return get_user_list(db_t *db, char **list) {
    if (check_connection(db))
        return generate_dbr(CORRUPTED, NULL);

    PGresult *res = PQexec(db, "SELECT name FROM cspairs");

    int tuples = PQntuples(res);
    if (tuples == 0) {
        PQclear(res);
        return generate_dbr(SUCCESS, NULL);
    } else {
        long list_len = 0, name_len;
        char *name;
        *list = calloc(1, sizeof (char));
        for (size_t i = 0; i < tuples; i++) {
            name = PQgetvalue(res, i, 0);
            name_len = strlen(name) + 1;
            *list = realloc(*list, list_len + name_len);
            strcpy(*list + list_len, name);
            list_len += name_len;
        }
        PQclear(res);

        return generate_dbr(SUCCESS, (void *) list_len);
    }
}
