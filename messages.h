/*
 * 03/31/2019
 * Comp 112 Final Project
 * Jonah Feldman, Nathan Allen, and Patrick Kinsella
 *
 * messages.h contains the message type enumerator, header field constants,
 * and header struct definition for our file server protocol.
 *
 * It is intended for shared use by the client file manager, file server,
 * and router to prevent bugs from incorrect implementations of the file
 * sharing protocol.
 */

#define ID_FIELD_LENGTH 1
#define ID_OFFSET 0
#define SOURCE_FIELD_LENGTH 20
#define SOURCE_FIELD_OFFSET 1
#define PASSWORD_FIELD_LENGTH 20
#define PASSWORD_FIELD_OFFSET 21
#define FILENAME_FIELD_LENGTH 20
#define FILENAME_FIELD_OFFSET 41
#define LENGTH_FIELD_LENGTH 4
#define LENGTH_FIELD_OFFSET 61
#define HEADER_LENGTH 65

enum message_type {NEW_CLIENT = 1, NEW_CLIENT_ACK, ERROR_CLIENT_EXISTS,
                   REQUEST_USER, CREATE_CLIENT = 64, CREATE_CLIENT_ACK,
                   NEW_SERVER, NEW_SERVER_ACK, ERROR_SERVER_EXISTS,
                   UPLOAD_FILE = 128, UPLOAD_ACK, ERROR_FILE_EXISTS,
                   ERROR_UPLOAD_FAILURE, REQUEST_FILE, RETURN_FILE, 
                   UPDATE_FILE, UPDATE_ACK, ERROR_FILE_DOES_NOT_EXIST,
                   ERROR_BAD_PERMISSIONS};

struct __attribute__((__packed__)) Header {
    unsigned char id;
    char source[SOURCE_FIELD_LENGTH];
    char password[PASSWORD_FIELD_LENGTH];
    char filename[FILENAME_FIELD_LENGTH];
    uint32_t length;
};