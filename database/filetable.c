/*
 * FileTable
 * Fileshare (Comp 112 Final Project)
 * Nathan Allen, Jonah Feldman, Patrick Kinsella
 * 1 April 2019
 */

#include <string.h>
#include "filetable.h"
#include "cppairs.h"

enum DB_STATUS create_file_table(db_t db, char drop_existing) {
    return create_table(db, "files", "Filename VARCHAR(100), Owner VARCHAR(20), "
                                     "Size INT, Checked_Out_By VARCHAR(20)",
                                      drop_existing);
}

enum DB_STATUS add_file(db_t db, char *client, char *pass,
                        struct file_info file) {
    if (check_connection(db))
        return CORRUPTED;
    if (!valid_authentication(db, client, pass))
        return INVALID_AUTHENTICATION;

    char *stm = calloc(100, sizeof (char));
    sprintf(stm, "INSERT INTO files VALUES('%s', '%s', %d)",
            file.name, client, file.len);

    PGresult *res = PQexec(db, stm);
    free(stm);
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        PQclear(res);
        return COMMAND_FAILED;
    }

    PQclear(res);
    return SUCCESS;
}

enum DB_STATUS delete_file(db_t db, char *client,
                           char *pass, char *filename) {
    if (check_connection(db))
        return CORRUPTED;
    else if (!valid_authentication(db, client, pass))
        return INVALID_AUTHENTICATION;

    char *stm = calloc(150, sizeof (char));
    sprintf(stm, "DELETE FROM files WHERE filename='%s'", filename);
    return exec_command(db, stm);
}

enum DB_STATUS update_file(db_t db, char *client, char *pass,
                           struct file_info file) {
    if (check_connection(db))
        return CORRUPTED;
    else if (!valid_authentication(db, client, pass))
        return INVALID_AUTHENTICATION;

    char *stm = calloc(100, sizeof (char));
    sprintf(stm, "UPDATE files SET size=%d WHERE filename='%s'",
            file.len, file.name);

    return exec_command(db, stm);
}

enum DB_STATUS checkout_file(db_t db, char *filename, char *requester) {
    if (check_connection(db))
        return CORRUPTED;

    char file[20], owner[20];
    strcpy(owner, strtok(filename, "/"));
    strcpy(file, strtok(NULL, ""));

    char *stm = calloc(150, sizeof (char));
    sprintf(stm, "UPDATE files SET checked_out_by='%s' WHERE filename='%s' AND "
                 "owner='%s'",
            requester, file, owner);

    return exec_command(db, stm);
}

struct db_return is_file_editor(db_t db, char *filename, char *editor) {
    if (check_connection(db))
        return generate_dbr(CORRUPTED, NULL);

    char file[20], owner[20];
    strcpy(owner, strtok(filename, "/"));
    strcpy(file, strtok(NULL, ""));

    char *stm = calloc(100, sizeof (char));
    sprintf(stm, "SELECT checked_out_by FROM files WHERE filename='%s' AND "
                 "owner='%s'", file, owner);

    PGresult *res = PQexec(db, stm);
    free(stm);

    if (PQntuples(res) == 0) {
        PQclear(res);
        return generate_dbr(ELEMENT_NOT_FOUND, NULL);
    }

    long diff = !strcmp(PQgetvalue(res, 0, 0), editor);

    PQclear(res);

    return generate_dbr(SUCCESS, (void *) diff);
}

enum DB_STATUS de_checkout_file(db_t db, char *filename) {
    if (check_connection(db))
        return CORRUPTED;

    char file[20], owner[20];
    strcpy(owner, strtok(filename, "/"));
    strcpy(file, strtok(NULL, ""));

    char *stm = calloc(100, sizeof (char));
    sprintf(stm, "UPDATE files SET Checked_Out_By='' WHERE filename='%s' AND "
                 "Owner='%s'", file, owner);

    return exec_command(db, stm);
}

struct db_return file_exists(db_t db, char *filename) {
    if (check_connection(db))
        return generate_dbr(CORRUPTED, 0);

    char file[20], owner[20];
    strcpy(owner, strtok(filename, "/"));
    strcpy(file, strtok(NULL, ""));

    char *stm = calloc(100, sizeof (char));
    sprintf(stm, "SELECT COUNT(*) FROM files WHERE filename='%s' AND "
                 "owner='%s'", file, owner);

    PGresult *res = PQexec(db, stm);
    free(stm);

    if (PQntuples(res) == 0) {
        PQclear(res);
        return generate_dbr(COMMAND_FAILED, 0);
    }

    long exists = atoi(PQgetvalue(res, 0, 0));

    PQclear(res);

    return generate_dbr(SUCCESS, (void *) exists);
}

struct db_return ready_for_checkout(db_t db, char *filename) {
    return is_file_editor(db, filename, "");
}
