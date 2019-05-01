CC = gcc

CFLAGS = -g -std=c99 -Wall -Wextra

INCLUDES = $(shell echo *.h)

DB_INCLUDES = -I/usr/include/postgresql -lpq

DEPS = partial_message_handler.o file_saving_handler.o io.o database/db.o

all: client operator server

# To get *any* .o file, compile its .c file with the following rule.
%.o: %.c $(INCLUDES)
	$(CC) $(CFLAGS) -c $< -o $@

client: client.o $(DEPS) database/cspairs.o db_wrapper.o database/servertable.o database/filetable.c database/cppairs.o
	$(CC) -g -o $@ $^ $(DB_INCLUDES)

operator: operator.o $(DEPS) database/servertable.o database/cspairs.o db_wrapper.o database/filetable.o
	$(CC) -g -o $@ $^ $(DB_INCLUDES)

server: server.o $(DEPS) database/cppairs.o db_wrapper.o database/cspairs.o database/servertable.o database/filetable.o
	$(CC) -g -o $@ $^ $(DB_INCLUDES)

clean:
	rm -f client operator server *.o database/*.o
