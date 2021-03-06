#include <string.h>
#include "cspairs.h"
#include "servertable.h"
#include "cppairs.h"
#include "filetable.h"

void error(const char *msg) {
    perror(msg);
    exit(0);
}

#define TEST_CLIENT "charles"
#define TEST_PASS "qwerty"

void cppairs_test_suite(db_t db);
void cspairs_test_suite(db_t db);
void filetable_test_suite(db_t db);
void servertable_test_suite(db_t db);
void personal_server_test_suite(db_t db);

int main(int argc, char* argv[]) {
    db_t db = connect_to_db("nathan", "fileshare");
    if (db == NULL)
        error("ERROR connecting to database");

    cppairs_test_suite(db);
    cspairs_test_suite(db);
    filetable_test_suite(db);
    servertable_test_suite(db);
    personal_server_test_suite(db);

    close_db_connection(db);
    return 0;
}

void cppairs_test_suite(db_t db) {
    enum DB_STATUS dbs;

    dbs = create_cppairs_table(db, 1);
    if (dbs != SUCCESS && dbs != ELEMENT_ALREADY_EXISTS)
        error("ERROR creating cppairs table");

    if (add_cppair(db, TEST_CLIENT, TEST_PASS))
        error("ERROR adding cppair");

    if (!valid_authentication(db, TEST_CLIENT, TEST_PASS))
        error("ERROR verifying client authentication");

    return;
}

void cspairs_test_suite(db_t db) {
    enum DB_STATUS dbs;

    dbs = create_cspairs_table(db, 1);
    if (dbs != SUCCESS && dbs != ELEMENT_ALREADY_EXISTS)
        error("ERROR creating cspairs table");

    struct Server addr = {
        .name = "test",
        .port = 9010,
        .ip_address = "nathan@allenhub.com"
    };
    if (add_cspair(db, TEST_CLIENT, &addr, 0))
        error("ERROR adding cspair");

    struct db_return result = get_server_from_client(db, TEST_CLIENT);
    struct Server *stored_addr = (struct Server *) result.result;
    if (addr.port != stored_addr->port ||
        strcmp(addr.ip_address, stored_addr->ip_address)) {
        error("ERROR retrieving stored server info");
    }
    free(stored_addr);

    if (!client_exists(db, TEST_CLIENT))
        error("ERROR verifying client existence");

    char *list;
    struct db_return dbr = get_user_list(db, &list);
    free(list);
    if (dbr.status)
        error("ERROR getting user list");

    return;
}

void filetable_test_suite(db_t db) {
    enum DB_STATUS dbs;
    struct db_return dbr;

    dbs = create_file_table(db, 1);
    if (dbs != SUCCESS && dbs != ELEMENT_ALREADY_EXISTS)
        error("ERROR creating file table");

    struct file_info file = {
        .owner = TEST_CLIENT,
        .name = "asdf.txt",
        .file = "asdf",
        .len = 4,
        .checked_out_by = ""
    };
    // printf("%d\n", add_file(db, TEST_CLIENT, TEST_PASS, file));
    if (add_file(db, TEST_CLIENT, TEST_PASS, file))
        error("ERROR adding file");

    file.file = "asdfg";
    file.len = 5;
    if (update_file(db, TEST_CLIENT, TEST_PASS, file))
        error("ERROR updating file");

    char file_name[40];
    sprintf(file_name, "%s/%s", TEST_CLIENT, file.name);
    if (checkout_file(db, file_name, TEST_CLIENT))
        error("ERROR checking out file");

    sprintf(file_name, "%s/%s", TEST_CLIENT, file.name);
    dbr = is_file_editor(db, file_name, TEST_CLIENT);
    if (dbr.status || !dbr.result)
        error("ERROR checking file editor");

    sprintf(file_name, "%s/%s", TEST_CLIENT, file.name);
    dbr = ready_for_checkout(db, file_name);
    if (dbr.status || dbr.result)
        error("ERROR validating file existance");

    sprintf(file_name, "%s/%s", TEST_CLIENT, file.name);
    if (de_checkout_file(db, file_name))
        error("ERROR de-checking out file");

    sprintf(file_name, "%s/%s", TEST_CLIENT, file.name);
    dbr = file_exists(db, file_name);
    if (dbr.status || !dbr.result)
        error("ERROR validating file existance");

    sprintf(file_name, "%s/%s", TEST_CLIENT, file.name);
    dbr = ready_for_checkout(db, file_name);
    if (dbr.status || !dbr.result)
        error("ERROR validating file existance");

    char *list;
    if ((long) get_files(db, TEST_CLIENT, &list).result !=
        strlen(file.name) + 1)
        error("ERROR getting files");
    free(list);

    return;
}

void servertable_test_suite(db_t db) {
    enum DB_STATUS dbs;

    dbs = create_server_table(db, 1);
    if (dbs != SUCCESS && dbs != ELEMENT_ALREADY_EXISTS)
        error("ERROR creating server table");

    struct Server addr = {
        .name = "test",
        .port = 9010,
        .ip_address = "nathan@allenhub.com"
    };
    if (add_server(db, &addr, 0))
        error("ERROR adding server");

    if (increment_clients(db, &addr) ||
        (long) clients_served_by(db, &addr).result != 1)
        error("ERROR incrementing or reading server's clients");

    // struct db_return result = least_populated_server(db);
    // if (result.status ||
    //     strcmp(((struct Server *) result.result)->name, "test"))
    //     error("ERROR retrieving least populated server");
    // free(result.result);

    return;
}

void personal_server_test_suite(db_t db) {
    enum DB_STATUS dbs;

    dbs = create_server_table(db, 0);
    if (dbs != SUCCESS && dbs != ELEMENT_ALREADY_EXISTS)
        error("ERROR creating server table");

    struct Server addr = {
        .name = "nathan",
        .port = 9010,
        .ip_address = "nathan@allenhub.com"
    };
    if (add_server(db, &addr, 1))
        error("ERROR adding personal server");

    if (add_cspair(db, "nathan", &addr, 1))
        error("ERROR adding cspair to personal server");

    char *list;
    struct db_return dbr = get_user_list(db, &list);
    free(list);
    if (dbr.status)
        error("ERROR getting user list");

    return;
}
