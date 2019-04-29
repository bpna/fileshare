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

/*
 * create_file_table
 *
 * Arguments:
 *     db_t db: database connection variable
 *     char drop_existing: bool representing whether or not to drop the table if
 *                         it already exists
 * Returns:
 *     command status: SUCCESS, CORRUPTED, COMMAND_FAILED
 */
enum DB_STATUS create_file_table(db_t db, char drop_existing);

/*
 * add_file
 *
 * Arguments:
 *     db_t db: database connection variable
 *     char *client: file owner
 *     char *pass: client password
 *     struct file_info file: struct containing file info
 * Returns:
 *     command status: SUCCESS, CORRUPTED, COMMAND_FAILED,
 *                     INVALID_AUTHENTICATION
 */
enum DB_STATUS add_file(db_t db, char *client, char *pass,
                        struct file_info file);

/*
 * delete_file
 *
 * Arguments:
 *     db_t db: database connection variable
 *     char *client: file owner
 *     char *pass: client password
 *     char *filename: name of file to delete
 * Returns:
 *     command status: SUCCESS, CORRUPTED, COMMAND_FAILED,
 *                     INVALID_AUTHENTICATION
 */
enum DB_STATUS delete_file(db_t db, char *client,
                           char *pass, char *filename);

/*
 * update_file
 *
 * Arguments:
 *     db_t db: database connection variable
 *     char *client: file owner
 *     char *pass: client password
 *     struct file_info file: struct containing file info
 * Returns:
 *     command status: SUCCESS, CORRUPTED, COMMAND_FAILED,
 *                     INVALID_AUTHENTICATION
 */
enum DB_STATUS update_file(db_t db, char *client, char *pass,
                           struct file_info file);

/*
 * checkout_file
 *
 * Arguments:
 *     db_t db: database connection variable
 *     char *filename: name of file in "owner/name" format
 *     char *requester: name of client requesting checkout
 * Returns:
 *     command status: SUCCESS, CORRUPTED, COMMAND_FAILED
 */
enum DB_STATUS checkout_file(db_t db, char *filename, char *requester);

/*
 * is_file_editor
 *
 * Arguments:
 *     db_t db: database connection variable
 *     char *filename: name of file in "owner/name" format
 *     char *editor: name of potential editing client
 * Returns:
 *     db_return struct with status and bool representing whether or not the
 *     named client has checked the file out
 *
 *     SUCCESS, CORRUPTED, ELEMENT_NOT_FOUND
 */
struct db_return is_file_editor(db_t db, char *filename, char *editor);

/*
 * de_checkout_file
 *
 * Arguments:
 *     db_t db: database connection variable
 *     char *filename: name of file in "owner/name" format
 * Returns:
 *     command status: SUCCESS, CORRUPTED, COMMAND_FAILED
 */
enum DB_STATUS de_checkout_file(db_t db, char *filename);

/*
 *
 * file_exists
 *
 * Arguments:
 *     db_t db: database connection variable
 *     char *filename: name of file in "owner/name" format
 * Returns:
 *     db_return struct with status and bool representing whether or not the
 *     file exists
 *
 *     SUCCESS, CORRUPTED, ELEMENT_NOT_FOUND
 */
struct db_return file_exists(db_t db, char *filename);

/*
 *
 * ready_for_checkout
 *
 * Arguments:
 *     db_t db: database connection variable
 *     char *filename: name of file in "owner/name" format
 * Returns:
 *     db_return struct with status and bool representing whether or not the
 *     file is ready for checkout
 *
 *     SUCCESS, CORRUPTED, ELEMENT_NOT_FOUND
 */
struct db_return ready_for_checkout(db_t db, char *filename);
