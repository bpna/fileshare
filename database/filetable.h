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
    char *owner;
    char *name;
    void *file;
    int len;
};
#endif

enum DB_STATUS create_file_table(db_t *db, char drop_existing);

enum DB_STATUS add_file(db_t *db, char *client, char *pass,
                        struct file_info file);

// struct db_return owned_by(db_t *db, char *filename);

enum DB_STATUS delete_file(db_t *db, char *client,
                           char *pass, char *filename);

enum DB_STATUS update_file(db_t *db, char *client, char *pass,
                           struct file_info file);

struct db_return get_file(db_t *db, char *owner, char *filename);
