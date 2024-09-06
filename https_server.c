

// to kill a program with a certain PORT, use
// sudo fuser -k PORT/tcp

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <fcntl.h>
#include <signal.h>
#include <ctype.h>
#include <stddef.h>
#include <locale.h>
#include <time.h>

// openssl library (libssl-dev) needs to be installed
// openssl library requires perl to be installed
#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include "structs_and_macros.h"
#include "connection_helpers.h"
#include "send_status.h"
#include "hash_functions.h"
#include "resource_handlers.h"

// Clients are calloced, dont think static does anything
static struct client_info* clients = NULL;

struct user* users = NULL;

// Things might break if delimiter size is increased
const char* DELIMITER = "\'\'\n";

bool server_should_close = false;

// Profanity items should not change once initialized
int min_size_profanity = 999;
int max_size_profanity = -1;
// if num of profanity is > 127, die
// String pointers are calloced
char* profanity_list[128] = {0};
int profanity_hash_list[128] = {0};


void print_clients(struct client_info* clients, FILE* fd);
void print_client(struct client_info* client, FILE* fd);

void handle_interupt(int signal) {
    server_should_close = true;
}

int main(int argc, char* argv[]) {
    printf("SIZE OF FD_SETSIZE (MAX NUM CONNECTIONS) = %d\n", FD_SETSIZE);

    if (argc > 2) {
        printf("usage: ./https_server [PORT]\n");
        return 0;
    }

    // OS will send SIGPIPE, killing the program, if a TCP packet is sent (i.e. send()) to a disconnected socket
    // Ignoring SIGPIPE so program doesn't die
    signal(SIGPIPE, SIG_IGN);
    sigaction(SIGPIPE, &(struct sigaction){SIG_IGN}, NULL);

    // When SIGINT, close server
    signal(SIGINT, handle_interupt);

    // init profanity list
    hash_profanity_list();

    // INIT OPENSSL
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();

    SSL_CTX* ctx = SSL_CTX_new(TLS_server_method());
    if (!ctx) {
        fprintf(stderr, "SSL_DTX_new() failed.\n");
        return 1;
    }

    // Default timeout is 300 seconds
    SSL_CTX_set_timeout(ctx, 10);

    // Probably should hide these
    if (!SSL_CTX_use_certificate_file(ctx, "/etc/letsencrypt/live/www.purplesite.skin/cert.pem", SSL_FILETYPE_PEM)
        || !SSL_CTX_use_PrivateKey_file(ctx, "/etc/letsencrypt/live/www.purplesite.skin/privkey.pem", SSL_FILETYPE_PEM)) {
        fprintf(stderr, "SSLL_CTX_use_certificate_file() failed.\n");
        ERR_print_errors_fp(stderr);
        return 1;
    }
    
    // CREATE SERVER SOCKET ON PORT argv[1] or default 443
    int server;
    if (argc == 1) {
        server = create_socket(0, "443");
    } else {
        server = create_socket(0, argv[1]);
    }
    // http is just for redirect to https
    int http = create_socket(0, "80");

    // drop root privileges if root
    const int webserver_uid = 1001;
    const int webserver_gid = 1002;
    if (getuid() == 0) {
        printf("sudo, dropping root privileges\n");
        if (setgid(webserver_gid) != 0) {
            fprintf(stderr, "setgid\n");
            exit(1);
        }
        if (setuid(webserver_uid) != 0) {
            fprintf(stderr, "setuid\n");
            exit(1);
        }
    }
    if (setuid(0) != -1) {
        fprintf(stderr, "managed to regain root privileges\n");
        exit(1);
    }

    users = init_users();

    #if PRINT_CLIENTS_AFTER_FUNC == true
    printf("Before main clients = %p\n", clients);
    #endif

    // MAIN LOOP
    while (!server_should_close) {
        
        fd_set reads;
        reads = wait_on_clients(server, &clients);
        #if PRINT_CLIENTS_AFTER_FUNC == true
        printf("After wait_on_clients() clients = %p\n", clients);
        #endif

        // use poll() instead for http
        /*fd_set http_reads;
        http_reads = wait_on_clients(http, &clients);
        struct timeval timeout;
        timeout.tv_sec = 10;
        timeout.tv_usec = 0;
        if (select(http + 1, &http_reads, 0, 0, &timeout) < 0) {
            fprintf(stderr, "select() failed. (%d, %s)\n", errno, strerror(errno));
            exit(1);
        }
        if (FD_ISSET(http, &http_reads)) {
            struct sockaddr address = {0};
            socklen_t address_length = sizeof(address);
            int client = accept(http,
                        &address,
                        &address_length);
            const char* c301 = "HTTP/1.1 301 Moved Permanently\r\n"
                            "Location: https://www.purplesite.skin\r\n\r\n";
            printf("Sent http 301:\n%s\n", c301);
            send(client, c301, strlen(c301), 0);
            close(client);
        }*/

        if (FD_ISSET(server, &reads)) {

            struct client_info* client = get_client(-1, &clients);
            #if PRINT_CLIENTS_AFTER_FUNC == true
            printf("After get_client() clients = %p\n", clients);
            #endif

            client->socket = accept(server,
                        (struct sockaddr*) &client->address,
                        &(client->address_length));

            if (client->socket < 0) {
                fprintf(stderr, "accept() failed. (%d, %s)\n", errno, strerror(errno));
                printf("CLIENT:\n");
                print_client(client, stderr);
                printf("\nCLIENTS");
                print_clients(clients, stderr);
                printf("\n");
                return 1;
            }

            // I think these are to make it non-blocking i dont remember
            /*int flags = fcntl(client->socket, F_GETFL, 0);
            if (flags < 0) {
                fprintf(stderr, "fcntl() failed to get flags. (%d, %s)\n", errno, strerror(errno));
                return 1;
            }

            if (fcntl(client->socket, F_SETFL, flags | O_NONBLOCK) < 0) {
                fprintf(stderr, "fcntl() failed to set flags. (%d, %s)\n", errno, strerror(errno));
                return 1;
            }*/

            // SSL/TLS
            client->ssl = SSL_new(ctx);
            if (!ctx) {
                fprintf(stderr, "SSL_new() failed.\n");
                printf("CLIENT:\n");
                print_client(client, stderr);
                printf("\nCLIENTS");
                print_clients(clients, stderr);
                printf("\n");
                return 1;
            }

            SSL_set_fd(client->ssl, client->socket);
            // SSL_accept probably blocks, use a thread to accept?
            int accept_return_code = SSL_accept(client->ssl);
            if (accept_return_code <= 0) {
                fprintf(stderr, "%s: SSL_accept() failed.\n", get_client_address(&client));
                ERR_print_errors_fp(stderr);

                int error_code = SSL_get_error(client->ssl, accept_return_code);
                printf("ERRORNUM = %d\n", error_code);
                switch(error_code) {
                    case 5 /*http request*/: send_301(client, &clients); break;
                    default: drop_client(client, &clients); break;
                }

            } else {
                printf("New connection from %s.\n", get_client_address(&client));
                printf("SSL connection using %s\n", SSL_get_cipher(client->ssl));
            }
        }

        struct client_info* client = clients;
        #if PRINT_CLIENTS_AFTER_FUNC == true
        printf("Starting client = %p\n", client);
        #endif 
        int MAX_CLIENTS = 1000;
        while (client != NULL) {
            #if PRINT_CLIENTS_AFTER_FUNC == true
            printf("Client is not null\n");
            #endif
            
            struct client_info* next = client->next;

            if (FD_ISSET(client->socket, &reads)) {

                // Skips client if getsockopt() returns an error, no special handling
                int errorVal = 0;
                socklen_t size = client->address_length;
                if (getsockopt(client->socket, SOL_SOCKET, SO_ERROR, &errorVal, &size) < 0) {
                    fprintf(stderr, "getsockopt() failed. (%d, %s)\n", errno, strerror(errno));
                    printf("CLIENT:\n");
                    print_client(client, stderr);
                    printf("\nCLIENTS");
                    print_clients(clients, stderr);
                    printf("\n");
                    drop_client(client, &clients);
                    client = next;
                    continue;
                }

                if (MAX_REQUEST_SIZE == client->received) {
                    send_400(client, &clients, NULL);
                    client = next;
                    continue;
                }

                int bytes_read = SSL_read(client->ssl,
                            client->request + client->received,
                            MAX_REQUEST_SIZE - client->received);
                #if PRINT_RECEIVED == true
                printf("\nReceived:\n%s\n", client->request);
                #endif

                if (bytes_read <= 0) {
                    printf("Unexpected disconnect from %s.\n", get_client_address(&client));
                    drop_client(client, &clients);
                    client = next;
                    continue;
                }
                client->received += bytes_read;
                client->request[client->received] = 0;

                char* http_msg_end = strstr(client->request, "\r\n\r\n");
                if (http_msg_end) {
                    // strncmp returns 0 if strings are equal
                    if (strncmp("GET /", client->request, 5) == 0) {
                        char* path = client->request + 4;
                        char* end_path = strstr(path, " ");
                        if (!end_path) {
                            send_400(client, &clients, NULL);
                        } else {
                            *end_path = 0;
                            serve_resource(client, &clients, users, path);
                            drop_client(client, &clients);
                        }
                    } else if (strncmp("POST /", client->request, 6) == 0) {
                        handle_post(client, &clients, &users);
                    } else {
                        send_400(client, &clients, NULL);
                    }
                }
            }
            if (client == client->next) {
                fprintf(stderr, "Reading clients (line %d). Client equaled itself\n", __LINE__);
                drop_client(client, &clients);
                break;
            }
            if (MAX_CLIENTS-- <= 0) {
                fprintf(stderr, "Reading clients (line %d). Too many clients or loop\n", __LINE__);
                drop_client(client, &clients);
                break;
            }
            client = next;
        }
    }

    printf("\nClosing server...\n");
    // semms to work fine
    while (clients) drop_client(clients, &clients);
    SSL_CTX_free(ctx);
    close(server);
    printf("Finished.\n");
    return 0;
}

void print_clients(struct client_info* clients, FILE* fd) {
    struct client_info* client = clients;
    while (client != NULL) {
        print_client(client, fd);
        if (client == client->next) {
            fprintf(stderr, "prinf_clients(). Client equaled client->next\n");
            return;
        }
        client = client->next;
    }
}

void print_client(struct client_info* client, FILE* fd) {

    fprintf(fd, "address_length = %d\n", client->address_length);
    //printf("raw address = %ld\n", client->address);
    fprintf(fd, "address = %s\n", get_client_address(&client));
    fprintf(fd, "socket = %d\n", client->socket);
    fprintf(fd, "parseFailures = %d\n", client->parseFailures);
}