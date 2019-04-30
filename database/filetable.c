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
    return create_table(db, "files", "Filename VARCHAR(20), Owner VARCHAR(20), "
                                     "Checked_Out_By VARCHAR(20)",
                                      drop_existing);
}

enum DB_STATUS add_file(db_t db,
                        char* filename) {
    if (check_connection(db))
        return CORRUPTED;
    // if (!valid_authentication(db, client, pass))
    //     return INVALID_AUTHENTICATION;
    char safe_filename[40];
    strcpy(safe_filename, filename);
    char *owner = strtok(safe_filename, "/");
    char *real_fname = strtok(NULL, "");



    char *stm = calloc(100, sizeof (char));
    sprintf(stm, "INSERT INTO files VALUES('%s', '%s')",
            real_fname, owner);

    PGresult *res = PQexec(db, stm);
    free(stm);
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        PQclear(res);
        return COMMAND_FAILED;
    }

    PQclear(res);
    return SUCCESS;
}

enum DB_STATUS delete_file_from_table(db_t db, char *filename) {
    if (check_connection(db))
        return CORRUPTED;
   // else if (valid_authentication(db, client, pass))
   //     return INVALID_AUTHENTICATION;
    char safe_filename[40];
    strcpy(safe_filename, filename);
    char *token = strtok(safe_filename, "/");
    token = strtok(NULL, "");


    char *stm = calloc(150, sizeof (char));
    sprintf(stm, "DELETE FROM files WHERE filename='%s'", token);
    return exec_command(db, stm);
}

// enum DB_STATUS update_file_on_table(db_t db, char *client, char *pass,
//                            struct file_info file) {
//     if (check_connection(db))
//         return CORRUPTED;
//     // else if (!valid_authentication(db, client, pass))
//     //     return INVALID_AUTHENTICATION;

//     char *stm = calloc(100, sizeof (char));
//     sprintf(stm, "UPDATE files SET size=%d WHERE filename='%s'",
//             file.len, file.name);

//     return exec_command(db, stm);
// }

enum DB_STATUS checkout_file_from_table(db_t db, char *filename, char *requester) {
    if (check_connection(db))
        return CORRUPTED;

    char safe_filename[40];
    strcpy(safe_filename, filename);


    char file[20], owner[20];
    strcpy(owner, strtok(safe_filename, "/"));
    strcpy(file, strtok(NULL, ""));
    fprintf(stderr, "requester is %s\n", requester);;

    char *stm = calloc(150, sizeof (char));
    sprintf(stm, "UPDATE files SET checked_out_by='%s' WHERE filename='%s' AND "
                 "owner='%s'",
            requester, file, owner);

    return exec_command(db, stm);
}

struct db_return is_file_editor(db_t db, char *filename, char *editor) {
    if (check_connection(db))
        return generate_dbr(CORRUPTED, NULL);
    char safe_filename[40];
    strcpy(safe_filename, filename);

    char file[20], owner[20];
    strcpy(owner, strtok(safe_filename, "/"));
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
    char safe_filename[40];
    strcpy(safe_filename, filename);


    char file[20], owner[20];
    strcpy(owner, strtok(safe_filename, "/"));
    strcpy(file, strtok(NULL, ""));

    char *stm = calloc(100, sizeof (char));
    sprintf(stm, "UPDATE files SET Checked_Out_By='' WHERE filename='%s' AND "
                 "Owner='%s'", file, owner);

    return exec_command(db, stm);
}

struct db_return file_exists(db_t db, char *filename) {
    if (check_connection(db))
        return generate_dbr(CORRUPTED, 0);

    char safe_filename[40];
    strcpy(safe_filename, filename);


    char file[20], owner[20];
    strcpy(owner, strtok(safe_filename, "/"));
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

struct db_return get_files(db_t db, char *client, char **list) {
    if (check_connection(db))
        return generate_dbr(CORRUPTED, NULL);

    char *stm = calloc(70, sizeof (char));
    sprintf(stm, "SELECT filename FROM files WHERE owner='%s'", client);
    PGresult *res = PQexec(db, stm);
    free(stm);

    int tuples = PQntuples(res);
    if (tuples == 0) {
        PQclear(res);
        return generate_dbr(SUCCESS, NULL);
    } else {
        long list_len = 0, name_len;
        char *name;
        *list = calloc(1, sizeof (char));
        for (int i = 0; i < tuples; i++) {
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
