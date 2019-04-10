CC = gcc

CFLAGS = -g -std=c99 -Wall -Wextra

INCLUDES = $(shell echo *.h)

DB_INCLUDES = -I/usr/include/postgresql -lpq

all: client operator server

# To get *any* .o file, compile its .c file with the following rule.
%.o: %.c $(INCLUDES)
	$(CC) $(CFLAGS) -c $< -o $@

client: client.o partial_message_handler.o file_saving_handler.o database/db.o database/cspairs.o
	$(CC) -g -o $@ $^ $(DB_INCLUDES)

operator: operator.o partial_message_handler.o file_saving_handler.o database/db.o database/servertable.o database/cspairs.o
	$(CC) -g -o $@ $^ $(DB_INCLUDES)

server: server.o file_saving_handler.o partial_message_handler.o
	$(CC) -g -o $@ $^ $(DB_INCLUDES)

clean:
	rm -f client operator server *.o database/*.o
