
#include "connection_helpers.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/select.h> // for pselect()
#include <netdb.h>
#include <unistd.h>
#include <signal.h>


// Should exit on failure
int create_socket(const char* host, const char* port) {

    printf("Configuring local address...\n");
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    struct addrinfo* bind_address;
    getaddrinfo(host, port, &hints, &bind_address);

    printf("Creating socket...\n");
    int socket_listen;
    socket_listen = socket(bind_address->ai_family, bind_address->ai_socktype, bind_address->ai_protocol);
    if (socket_listen < 0) {
        fprintf(stderr, "create_socket(). socket() failed. (%d, %s).\n", errno, strerror(errno));
        exit(1);
    }

    // Lets you reuse a port if it was recently in use (claims it from OS)
    int yes = 1;
    if (setsockopt(socket_listen, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
        perror("create_socket(). setsockopt error\n");
        exit(1);
    }
    
    printf("Binding socket to local address...\n");
    if (bind(socket_listen,
            bind_address->ai_addr, bind_address->ai_addrlen)) {
        fprintf(stderr, "create_socket(). bind() failed. (%d, %s).\n", errno, strerror(errno));
        exit(1);
    }  
    freeaddrinfo(bind_address);

    printf("Listening...\n");
    if (listen(socket_listen, 10) < 0) {
        fprintf(stderr, "create_socket(). listen() failed. (%d, %s).\n", errno, strerror(errno));
        exit(1);
    }

    return socket_listen;
}

struct client_info* get_client(int socket, struct client_info** clientsPtr) {
    #if PRINT_FUNC_START == true
    printf("Getting client\n");
    #endif

    struct client_info* clients = *clientsPtr;

    struct client_info* ci = clients;

    while (ci) {
        if (ci->socket == socket) break;
        ci = ci->next;
    }
    if (ci) return ci;

    struct client_info* new = (struct client_info*) calloc(1, sizeof(struct client_info));

    if (!new) {
        fprintf(stderr, "get_client(). Out of memory?, returning NULL\n");
        return NULL;
    }

    new->address_length = sizeof(new->address);
    new->next = clients;
    clients = new;
    *clientsPtr = clients;
    new->parseFailures = 0;
    printf("New client = %p\n", new);
    return new;
}

void drop_client(struct client_info* client, struct client_info** clientsPtr) {
    #if PRINT_FUNC_START == true
    printf("Dropping client\n");
    #endif

    if (clientsPtr == NULL) {
        return;
    }
    struct client_info** pointer = clientsPtr;

    while (*pointer) {
        if (*pointer == client) {
            *pointer = client->next;
            // client = client->next?? equivalent?
            SSL_shutdown(client->ssl);
            close(client->socket);
            SSL_free(client->ssl);
            free(client);
            return;
        }
        pointer = &(*pointer)->next;
    }

    fprintf(stderr, "drop_client(). Client not found.\n");
}

const char* get_client_address(struct client_info** clientPtr) {

    // static might create problems with multuihreading
    static char address_buffer[100];
    struct client_info* client = *clientPtr;
    getnameinfo((struct sockaddr*) &client->address,
            client->address_length,
            address_buffer, sizeof(address_buffer), 0, 0,
            NI_NUMERICHOST);
    return address_buffer;
}

fd_set wait_on_clients(int server, struct client_info** clientsPtr) {
    #if PRINT_FUNC_START == true
    printf("Waiting\n");
    #endif

    fd_set reads;
    FD_ZERO(&reads);
    FD_SET(server, &reads);
    int max_socket = server;

    struct client_info* clients = *clientsPtr;

    struct client_info* client = clients;

    while (client) {
        FD_SET(client->socket, &reads);
        if (client->socket > max_socket)
            max_socket = client->socket;
        client = client->next;
    }

    sigset_t sigmask;
    sigemptyset(&sigmask);
    sigaddset(&sigmask, SIGINT);

    // pselect() uses timespec insead of timeval
    struct timespec timeout;
    timeout.tv_sec = 20;
    timeout.tv_nsec = 0;
    if (pselect(max_socket  + 1, &reads, 0, 0, &timeout, &sigmask) < 0) {
        fprintf(stderr, "get_client_address(). select() failed. (%d, %s)\n", errno, strerror(errno));
        //exit(1);
    }

    return reads;
}