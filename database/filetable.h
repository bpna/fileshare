/*
 * FileTable
 * Fileshare (Comp 112 Final Project)
 * Nathan Allen, Jonah Feldman, Patrick Kinsella
 * 1 April 2019
 */

#include "db.h"

#define SERVER_ID 1

#ifndef FILE_INFO
#define FILE_INFO
struct file_info {
    char owner[20];
    char name[20];
    void *file;
    int len;
    char checked_out_by[20];
};
#endif

enum DB_STATUS create_file_table(db_t db, char drop_existing);

enum DB_STATUS add_file(db_t db, char *client, char *pass,
                        struct file_info file);

enum DB_STATUS delete_file_from_table(db_t db, char *client,
                           char *pass, char *filename);

enum DB_STATUS update_file(db_t db, char *client, char *pass,
                           struct file_info file);

enum DB_STATUS checkout_file(db_t db, char *filename, char *requester);

struct db_return is_file_editor(db_t db, char *filename, char *editor);

enum DB_STATUS de_checkout_file(db_t db, char *filename);
