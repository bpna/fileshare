/*
 * Created 4/8/2019
 * Jonah Feldman, Nathan Allen, Patrick Kinsella
 *
 * The io module offers generic functions for clients and servers to open and
 * bind to sockets, connect to servers listening on a given port number, and
 * write messages to open sockets
 */

int open_and_bind_socket(int portno);
int connect_to_server(char *fqdn, int portno);
int write_message(int csock, char *data, int length);
int write_file(int csock, char *filename);
int error(const char *msg);
