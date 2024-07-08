
#ifndef STRUCTS_AND_MACROS_HEADER
#define STRUCTS_AND_MACROS_HEADER

#include <sys/socket.h>
#include <openssl/ssl.h>

// Toggles
#define PRINT_RECEIVED   true
#define PRINT_FILE_TYPE  true
#define PRINT_400        true
#define PRINT_POST_PARSE true
#define PRINT_HASH       true
#define PROFANITY_PRINT  true
#define PRINT_HASH       true


#define MAX_REQUEST_SIZE 2047

// struct size turns out to be about 2204 (double check), may want to pad?
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