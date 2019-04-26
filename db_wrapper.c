#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include "db_wrapper.h"
#include "database/db.h"
#include "database/cspairs.h"
#include "database/servertable.h"
#include "io.h"

#define USE_DB 1
#define CSPAIRS_FNAME "client_cspairs.txt"
#define SERVER_LOAD_FNAME "server_nums.txt"
#define CHECKOUT_FILE "checkouts.txt"
#define TEMP_CHECKOUT_FILE "checkouts.txt~"
#define SERVER_FILE_MAX_LENGTH 10000
#define CSPAIRS_FILE_MAX_LENGTH 10000

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

db_t connect_to_db_wrapper() {
    db_t db;
    if (USE_DB) {
        db = connect_to_db(DB_OWNER, DB_NAME);
    } else {
        db = NULL;
    }

    return db;
}

void add_cspair_wrapper(db_t db, char *client, char *fqdn, unsigned port,
                        char *loc, int increment_client) {
    enum DB_STATUS db_status;
    struct Server server;
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

        db_status = add_cspair(db, client, &server, increment_client);
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
struct Server *get_server_from_client_wrapper(db_t db, char *client,
                                              char *loc) {
    struct db_return db_return;
    struct Server *server;
    FILE *fp = NULL;
    char buffer[CSPAIRS_FILE_MAX_LENGTH];
    char *token = NULL;
    int length;

    if (USE_DB) {
        db_return = get_server_from_client(db, client);
        check_db_status(db_return.status, loc);
        if (db_return.result == NULL)
            return NULL;

        server = (struct Server *) db_return.result;
    } else {
        /* read contents of CSPAIRS file into buffer */
        server = malloc(sizeof (struct Server));
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
            free(server);
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
            strcpy(server->domain_name, token);
            token = strtok(NULL, ":\n");
            if (token == NULL) {
                fprintf(stderr, "CSPAIRS file %s badly formed\n",
                        CSPAIRS_FNAME);
                exit(1);
            }
            server->port = atoi(token);
        }
    }

    return server;
}



char checkout_file_db_wrapper(char *requester, char * desired_filename){
    int buflen = FILENAME_FIELD_LENGTH + SOURCE_FIELD_LENGTH + 5;
    char buffer[buflen];
    bzero(buffer, buflen);
    char *file;
    char *file_editor;

    // if (USE_DB){
    //     //TODO: database stuff

    // }
    //{
        FILE *fp = fopen(CHECKOUT_FILE, "r+");
        if (fp == NULL){
                fprintf(stderr, "error in checkout_file_db_wrapper\n" );
                return -1;
        }

        fseek(fp, 0, SEEK_SET);

        //loops through file line by line
        while(fgets(buffer, buflen, fp) != NULL){
            file = strtok(buffer, " ");
            file_editor = strtok(NULL, "\n");

            if (strcmp(desired_filename, file) == 0){

                //if someone has already checked out file
                if (file_editor[0] != '~'){
                    fclose(fp);
                    file_editor = strtok(file_editor, "~");
                    if (strcmp(file_editor, requester) == 0)
                        return 1;
                    return -1;
                }
                else{

                    //rest File pointer to before string of tildas
                    fseek(fp, (SOURCE_FIELD_LENGTH + 1) * -1, SEEK_CUR);
                    fwrite(requester,1, strlen(requester), fp);
                    memset(buffer, '~', buflen);
                    fwrite(buffer, 1, SOURCE_FIELD_LENGTH - strlen(requester), fp);
                    fwrite("\n", 1, 1, fp);
                    fclose(fp);
                    return 0;
                }

            }

        }
        fclose(fp);
        return -1;




   // }
}


char is_file_editor(char *requester, char *desired_filename){

    int buflen = FILENAME_FIELD_LENGTH + SOURCE_FIELD_LENGTH + 5;
    char buffer[buflen];
    bzero(buffer, buflen);
    char *file;
    char *file_editor;

    // if (USE_DB){
    //     //TODO: database stuff

    // }
    //{
        FILE *fp = fopen(CHECKOUT_FILE, "r+");
        if (fp == NULL){
                fprintf(stderr, "error in is_file_editor\n" );
                return -1;
        }

        fseek(fp, 0, SEEK_SET);

        //loops through file line by line
        while(fgets(buffer, buflen, fp) != NULL){
            file = strtok(buffer, " ");
            file_editor = strtok(NULL, "\n");

            if (strcmp(desired_filename, file) == 0){

                //if someone has already checked out file
                if (file_editor[0] != '~'){
                    file_editor = strtok(file_editor, "~");
                    fclose(fp);
                    if (strcmp(file_editor, requester) == 0)
                        return 1;
                    return 0;
                }
                else{
                    fclose(fp);
                    return 0;
                }

            }

        }
        fclose(fp);
        return 0;


}


void add_file_wrapper(char *filename){
    char buffer[SOURCE_FIELD_LENGTH];
    memset(buffer, '~', SOURCE_FIELD_LENGTH);

    FILE *fp = fopen(CHECKOUT_FILE, "a");
    if (fp == NULL){
        fprintf(stderr, "error in add_file_wrapper\n" );
        return;
    }
    fwrite(filename, 1, strlen(filename), fp);
    fwrite(" ", 1, 1, fp);
    fwrite(buffer, 1, SOURCE_FIELD_LENGTH, fp);
    fwrite("\n", 1, 1, fp);
    fclose(fp);
}

void de_checkout_file(char *desired_filename){

    int buflen = FILENAME_FIELD_LENGTH + SOURCE_FIELD_LENGTH + 5;
    char buffer[buflen];
    bzero(buffer, buflen);
    char *file;

    // if (USE_DB){
    //     //TODO: database stuff

    // }
    //{
        FILE *fp = fopen(CHECKOUT_FILE, "r+");
        if (fp == NULL){
                fprintf(stderr, "error in checkout_file_db_wrapper\n" );
                return;
        }

        fseek(fp, 0, SEEK_SET);

        //loops through file line by line
        while(fgets(buffer, buflen, fp) != NULL){
            file = strtok(buffer, " ");

            if (strcmp(desired_filename, file) == 0){
                fseek(fp, (SOURCE_FIELD_LENGTH + 1) * -1, SEEK_CUR);
                memset(buffer, '~', buflen);
                fwrite(buffer, 1, SOURCE_FIELD_LENGTH, fp);
                fwrite("\n", 1, 1, fp);
                break;
            }

        }
        fclose(fp);



}





/*
struct Server *least_populated_server_wrapper(db_t db, char *loc)
{
    struct db_return dbr;
    struct Server *server = malloc(sizeof(*server));

    if (USE_DB) {
        dbr = least_populated_server(db);
        check_db_status(dbr.status, loc);
        if (dbr.result == NULL) {
            free
    } else {

    }

} */
