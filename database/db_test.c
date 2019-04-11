#include <string.h>
#include "cspairs.h"
#include "servertable.h"
#include "cppairs.h"
#include "filetable.h"

void do_exit(PGconn *conn) {
    PQfinish(conn);
    exit(1);
}

void error(char *err) {
    perror(err);
    exit(1);
}

#define TEST_CLIENT "charles"
#define TEST_PASS "qwerty"

void cppairs_test_suite(db_t *db);
void cspairs_test_suite(db_t *db);
void filetable_test_suite(db_t *db);
void servertable_test_suite(db_t *db);

int main(int argc, char* argv[]) {
    db_t *db = connect_to_db("nathan", "fileshare");
    if (db == NULL)
        error("ERROR connecting to database");

    cppairs_test_suite(db);
    cspairs_test_suite(db);
    filetable_test_suite(db);
    servertable_test_suite(db);

    close_db_connection(db);
    return 0;
}

void cppairs_test_suite(db_t *db) {
    if (create_table(db, "cppairs", "Client VARCHAR(20) PRIMARY KEY, \
                                     Password VARCHAR(20)"))
        error("ERROR creating copairs table");

    // if (add_cppair(db, TEST_CLIENT, TEST_PASS))
    //     error("ERROR adding cppair");

    // if (valid_authentication(db, TEST_CLIENT, TEST_PASS))
    //     error("ERROR verifying client authentication");

    return;
}

void cspairs_test_suite(db_t *db) {
    if (create_table(db, "cspairs", "Name VARCHAR(20), Port SMALLINT, \
                                     Domain VARCHAR(255)"))
        error("ERROR creating cspairs table");

    // struct server_addr addr = {
    //     .name = "test",
    //     .port = 9010,
    //     .domain_name = "nathan@allenhub.com"
    // };
    // if (add_cspair(db, TEST_CLIENT, &addr))
    //     error("ERROR adding cspair");

    // struct db_return result = get_server_from_client(db, TEST_CLIENT);
    // struct server_addr *stored_addr = (struct server_addr *) result.result;
    // if (addr.port != stored_addr->port ||
    //     strcmp(addr.domain_name, stored_addr->domain_name)) {
    //     error("ERROR retrieving stored server info");
    // }
    // free(stored_addr);

    // if (!client_exists(db, TEST_CLIENT))
    //     error("ERROR verifying client existence");

    return;
}

void filetable_test_suite(db_t *db) {
    if (create_table(db, "files", "Filename VARCHAR(100), Owner VARCHAR(20), \
                                   Size INT"))
        error("ERROR creating file table");

    // struct file_info file = {
    //     .owner = TEST_CLIENT,
    //     .name = "asdf.txt",
    //     .file = "asdf",
    //     .len = 4
    // };
    // if (add_file(db, TEST_CLIENT, TEST_PASS, file))
    //     error("ERROR adding file");

    // file.file = "asdfg";
    // file.len = 5;
    // if (update_file(db, TEST_CLIENT, TEST_PASS ,file))
    //     error("ERROR updating file");

    // if (delete_file(db, TEST_CLIENT, TEST_PASS, "asdf.txt"))
    //     error("ERROR deleting file");

    return;
}

void servertable_test_suite(db_t *db) {
    if (create_table(db, "servers", "Name VARCHAR(20) PRIMARY KEY, \
                                     PORT SMALLINT, Domain VARCHAR(255), \
                                     Clients INT, Stored_Bytes BIGINT"))
        error("ERROR creating server table");

    // struct server_addr addr = {
    //     .name = "test",
    //     .port = 9010,
    //     .domain_name = "nathan@allenhub.com"
    // };
    // if (add_server(db, &addr))
    //     error("ERROR adding server");

    // if (increment_clients(db, &addr) ||
    //     (long) clients_served_by(db, &addr).result != 1)
    //     error("ERROR incrementing or reading server's clients");

    // struct db_return result = least_populated_server(db);
    // if (result.status || ((struct server_addr *) result.result)->id != 1)
    //     error("ERROR retrieving least populated server");
    // free(result.result);

    return;
}
