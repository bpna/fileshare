/*
 * FileTable
 * Fileshare (Comp 112 Final Project)
 * Nathan Allen, Jonah Feldman, Patrick Kinsella
 * 1 April 2019
 */

#include <string.h>
#include "filetable.h"
#include "cppairs.h"

enum DB_STATUS add_file(db_t *db, char *client, char *pass,
                        struct file_info file) {
    if (check_connection(db))
        return CORRUPTED;
    if (valid_authentication(db, client, pass))
        return INVALID_AUTHENTICATION;

    // if (get_file(db, client, file.name).status == ELEMENT_ALREADY_EXISTS)
    //     return ELEMENT_ALREADY_EXISTS;

    char *stm = calloc(100, sizeof (char));
    sprintf(stm, "INSERT INTO files VALUES('%s', '%s', 0)",
            file.name, client, file.len);

    // TODO: actually save the file

    PGresult *res = PQexec(db, stm);
    free(stm);
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        on_db_error(db, res);
        return COMMAND_FAILED;
    }

    PQclear(res);
    return SUCCESS;
}

// struct db_return owned_by(db_t *db, char *filename) {
//     if (check_connection(db))
//         return generate_dbr(CORRUPTED, NULL);

//     char *stm = calloc(150, sizeof (char));
//     sprintf(stm, "SELECT owner FROM files WHERE filename='%s'", filename);

//     PGresult *res = PQexec(db, stm);
//     free(stm);
//     if (PQresultStatus(res) != PGRES_TUPLES_OK) {
//         on_db_error(db, res);
//         return generate_dbr(COMMAND_FAILED, NULL);
//     }

//     char *owner = calloc(20, sizeof (char));
//     strcpy(owner, PQgetvalue(res, 0, 0));

//     PQclear(res);
//     return generate_dbr(SUCCESS, owner);
// }

enum DB_STATUS delete_file(db_t *db, char *client,
                           char *pass, char *filename) {
    if (check_connection(db))
        return CORRUPTED;
    else if (valid_authentication(db, client, pass))
        return INVALID_AUTHENTICATION;

    char *stm = calloc(150, sizeof (char));
    sprintf(stm, "DELETE FROM files WHERE filename='%s'", filename);
    return exec_command(db, stm);
}

enum DB_STATUS update_file(db_t *db, char *client, char *pass,
                           struct file_info file) {
    if (check_connection(db))
        return CORRUPTED;
    else if (valid_authentication(db, client, pass))
        return INVALID_AUTHENTICATION;

    char *stm = calloc(100, sizeof (char));
    sprintf(stm, "UPDATE files SET size=%d WHERE filename='%s'",
            file.len, file.name);

    // TODO: actually update the file

    return exec_command(db, stm);
}
