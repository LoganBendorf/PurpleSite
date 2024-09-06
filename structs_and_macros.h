
#ifndef STRUCTS_AND_MACROS_HEADER
#define STRUCTS_AND_MACROS_HEADER

#include <sys/socket.h>
#include <openssl/ssl.h>
#include <stdbool.h>

// Toggles
#define DEBUG_INIT_USERS true
#define PRINT_CLIENTS_AFTER_FUNC    false
#define PRINT_FUNC_START false
#define PRINT_RECEIVED   true
#define PRINT_FILE_TYPE  true
#define PRINT_400        true
#define PRINT_POST_PARSE true
#define PRINT_HASH       true
#define PROFANITY_PRINT  true
#define PRINT_HASH       true

// Info
#define MAX_NAME_CHARACTERS     15
#define MAX_NAME_BYTES          MAX_NAME_CHARACTERS * 4
#define MAX_TITLE_CHARACTERS    24
#define MAX_TITLE_BYTES         MAX_TITLE_CHARACTERS * 4
#define MAX_COMPOSE_CHARACTERS  200
#define MAX_COMPOSE_BYTES       MAX_COMPOSE_CHARACTERS * 4

#define MAX_REQUEST_SIZE 2047

// struct size turns out to be about 2204 (double check)
// maybe a pointer to char request would be better
struct client_info {
    socklen_t address_length;
    struct sockaddr_storage address;
    int socket;
    SSL* ssl;
    char request[MAX_REQUEST_SIZE + 1];
    int received;
    struct client_info* next;
    int parseFailures;
};

#endif