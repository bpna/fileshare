#!/bin/sh

gcc -g -o server server.c file_saving_handler.c partial_message_handler.c
gcc -g -o client client.c partial_message_handler.c file_saving_handler.c
gcc -g -o operator operator.c partial_message_handler.c file_saving_handler.c database/db.c database/servertable.c database/cspairs.c -I/usr/include/postgresql -lpq -std=c99
