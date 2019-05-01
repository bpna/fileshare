# Comp 112 Final Project: Fileshare
## Nathan Allen, Jonah Feldman, Patrick Kinsella
### Description
This is a file server to allow simpler file sharing and backup. The base version of our application uses a client/server model, where one operator connects clients to one of a list of servers on which the client can store their files.
### Features
- operator connects new clients to servers
- servers store clients' files
- operator helps clients find other clients' server locations
- personal server can by run by client to serve files from local machine
  - operator sets client up with backup server using traditional server model
- edit locks allow multiple users to work on the same file without overwriting each other
### Compiling
The project can be compiled with `make` in the home directory.
### Usage
#### Operator
The operator needs to be run first. It is run with the following usage:
```
./operator <portno>
```
- `portno`: port number the operator will use

There is only one operator per system.
#### Server
Servers must be run after the operator so they can connect to it. They are run with the following usage:
```
./server <portno> <ip-addr> <name> <operator-ip-addr> <operator-portno> <personal>
```
- `portno`: port number the server will use
- `ip-addr`: IP address of machine server will run on
- `name`: name of server
  - if `personal`, name of owning client. tells operator to create new client
- `operator-ip-addr`: IP address of machine operator is running on
- `operator-portno`: port number the operator is using
- `personal`: bool if server is acting as a personal server or not

Servers will create a directory for each client they are assigned to. They store the client's files in this directory. Therefore, a file's location relative to where the server is run is '*owner*/*filename*'.
#### Client
Clients must be created after the operator and at least one server has been set up. Before interacting with the system, the client must run the following setup case:
```
./client init <operator-ip-addr> <operator-portno>
```
- `operator-ip-addr`: IP address of machine operator is running on
- `operator-portno`: port number the operator is using

After initialization, the client program can run the following commands:
```
./client new_client [username] [password]
./client upload_file [username] [password] [filename]
./client add_file [username] [password] [filename]
./client request_file [username] [password] [owner-username] [filename]
./client checkout_file [username] [password] [owner-username] [filename]
./client update_file [username] [password] [owner-username] [filename]
./client user_list [username] [password]
./client upload_file file_list [username] [password] [owner-username]
./client delete_file [username] [password] [owner-username] [filename]
./client remove_file [username] [password] [filename]
```
- `username`: client's username
- `password`: client's password
- `filename`: name of file
- `owner-username`: username of targeted client
#### Database
Our project uses a PostgreSQL database to store information. All parts of the project require the database. The database's owner and name are set in `database/db.h` as `DB_OWNER` and `DB_NAME`.

As there is overlap between the operator and the client, it is advised that they are run on separate machines. In addition, servers reset their databases when they are run, so servers run on the same machine should be started up at the same time.

### Still in Progress
- We were not able to fully test our personal server code.
- If a server or the operator is disconnected for any reason, there is no way to reconnect to the system.
- We have a `USE_DB` flag in `db_wrapper.c` that, if PostgreSQL is not installed on your machine, will save data in a flat file rather than a database. This works in most cases, but not all.
