#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include "database/db.h"
#include "db_wrapper.h"
#include "database/cspairs.h"
#include "io.h"

void check_db_status(enum DB_STATUS db_status, char *func) {
    switch (db_status) {
        case CORRUPTED:
            fprintf(stderr, "in %s, db corrupted\n", func);
            exit(1);
        case COMMAND_FAILED:
            fprintf(stderr, "in %s, db command failed\n", func);
            exit(1);
        case INVALID_AUTHENTICATION:
            fprintf(stderr, "in %s, invalid db auth\n", func);
            exit(1);
        default:
            return;
    }
}

void add_cspair_wrapper(db_t *db, char *client, char *fqdn, unsigned port,
                        char *loc) {
    enum DB_STATUS db_status;
    struct server_addr server;
    char portno[6];
    int portno_length;
    FILE *fp;

    if (port > 65536) {
        error("ERROR port no out-of-range\n");
    }

    if (USE_DB) {
        bzero((char *) &server, sizeof(server));
        strcpy(server.domain_name, fqdn);
        server.port = port;

        db_status = add_cspair(db, client, &server);
        check_db_status(db_status, loc);
    } else {
        portno_length = sprintf(portno, "%u", port);
        fp = fopen(CSPAIRS_FNAME, "a+");
        fwrite(client, 1, strlen(client), fp);
        fwrite(":", 1, 1, fp);
        fwrite(fqdn, 1, strlen(fqdn), fp);
        fwrite(":", 1, 1, fp);
        fwrite(portno, 1, portno_length, fp);
        fwrite("\n", 1, 1, fp);
        fclose(fp);
    }
}

/*
 * This function calls get_server_from_client(), or reads from a client/server
 * pairs file, depending on whether USE_DB is set.
 *
 * If the client is not in the pairs list, get_server_from_client_wrapper()
 * returns NULL. Otherwise, it malloc's and returns a Server struct which must
 * be free'd by the caller.
 */
struct Server *get_server_from_client_wrapper(db_t *db, char *client,
                                              char *loc) {
    struct db_return db_return;
    struct server_addr *server;
    struct Server *retval = malloc(sizeof(*retval));
    FILE *fp = NULL;
    char buffer[CSPAIRS_FILE_MAX_LENGTH];
    char *token = NULL;
    int length;

    if (USE_DB) {
        db_return = get_server_from_client(db, client);
        check_db_status(db_return.status, loc);
        if (db_return.result == NULL) {
            free(retval);
            return NULL;
        }

        server = (struct server_addr *) db_return.result;
        retval->port = server->port;
        strcpy(retval->domain_name, server->domain_name);
    } else {
        /* read contents of CSPAIRS file into buffer */
        fp = fopen(CSPAIRS_FNAME, "rb");
        if (fp == NULL) {
            fprintf(stderr, "ERROR opening file %s for reading\n",
                    CSPAIRS_FNAME);
            exit(1);
        }
        fseek(fp, 0, SEEK_END);
        length = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        fread(buffer, 1, length, fp);
        fclose(fp);
        if (length == 0) {
            free(retval);
            return NULL;
        }

        /* parse buffer for desired client */
        token = strtok(buffer, ":\n");
        while (token != NULL && strcmp(token, client) != 0) {
            token = strtok(NULL, ":\n");
        }
        if (token == NULL) {
            return NULL;
        } else {
            token = strtok(NULL, ":\n");
            if (token == NULL) {
                fprintf(stderr, "CSPAIRS file %s badly formed\n",
                        CSPAIRS_FNAME);
                exit(1);
            }
            strcpy(retval->domain_name, token);
            token = strtok(NULL, ":\n");
            if (token == NULL) {
                fprintf(stderr, "CSPAIRS file %s badly formed\n",
                        CSPAIRS_FNAME);
                exit(1);
            }
            retval->port = atoi(token);
        }
    }

    return retval;
}