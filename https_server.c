
#define MAX_NAME_CHARACTERS     15
#define MAX_NAME_BYTES          MAX_NAME_CHARACTERS * 4
#define MAX_TITLE_CHARACTERS    24
#define MAX_TITLE_BYTES         MAX_TITLE_CHARACTERS * 4
#define MAX_COMPOSE_CHARACTERS  200
#define MAX_COMPOSE_BYTES       MAX_COMPOSE_CHARACTERS * 4


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

static struct client_info* clients = 0;

struct email {
    // hash just to make sure no repeated emails
    int hash;
    char sender[16];
    char recipient[16];
    char title[16];
    char body[1024];
    int upvotes;
    time_t creationDate;
};

/*
struct user_data {
    char name[16];
    // make just a giant blob
    struct email* emails;
};
struct user_date* users;
*/


int min_size_profanity = 999;
int max_size_profanity = -1;
// if num of profanity is > 127, crash
char* profanity_list[128] = {0};
int profanity_hash_list[128] = {0};

const char* get_content_type(const char* path);
int create_socket(const char* host, const char* port);
struct client_info* get_client(int socket);
void drop_client(struct client_info* client);
const char* get_client_address(struct client_info* ci);
fd_set wait_on_clients(int server);
void send_301(struct client_info* client);
void send_400(struct client_info* client, char* string);
void send_404(struct client_info* client);
void serve_resource(struct client_info* client, const char* path);
void handle_post(struct client_info* client);
int num_kosher_chars(char* string, int type);
struct email parse_user_file_data(char* username);
int hash_function(char* string);
void hash_profanity_list();
bool contains_profanity(char* string);
void print_client(struct client_info* client, FILE* fd);

int main(int argc, char* argv[]) {
    printf("SIZE OF FD_SETSIZE (MAX NUM CONNECTIONS) = %d\n", FD_SETSIZE);

    if (argc > 2) {
        printf("usage: ./https_server [PORT]\n");
        return 0;
    }

    // OS will send SIGPIPE, killing the program, if a TCP packet is sent (i.e. send()) to a disconeccted socket
    // Ignoring SIGPIPE so program doesn't die
    signal(SIGPIPE, SIG_IGN);

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
    int http = create_socket(0, "80");

    const int webserver_uid = 1001;
    const int webserver_gid = 1002;
    // drop root privileges if root
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

    // MAIN LOOP
    while (true) {
        
        fd_set reads;
        reads = wait_on_clients(server);

        // use poll() instead
        /*fd_set http_reads;
        http_reads = wait_on_clients(http);
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

            struct client_info* client = get_client(-1);

            client->socket = accept(server,
                        (struct sockaddr*) &client->address,
                        &(client->address_length));

            if (client->socket < 0) {
                fprintf(stderr, "accept() failed. (%d, %s)\n", errno, strerror(errno));
                print_client(client, stderr);
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
                print_client(client, stderr);
                return 1;
            }

            SSL_set_fd(client->ssl, client->socket);
            // SSL_accept probably blocks, use a thread to accept?
            int accept_return_code = SSL_accept(client->ssl);
            if (accept_return_code <= 0) {
                fprintf(stderr, "%s: SSL_accept() failed.\n", get_client_address(client));
                ERR_print_errors_fp(stderr);

                int error_code = SSL_get_error(client->ssl, accept_return_code);
                printf("ERRORNUM = %d\n", error_code);
                switch(error_code) {
                    case 5 /*http request*/: send_301(client); break;
                    default: drop_client(client); break;
                }

            } else {
                printf("New connection from %s.\n", get_client_address(client));
                printf("SSL connection using %s\n", SSL_get_cipher(client->ssl));
            }
        }

        struct client_info* client = clients;
        while (client != NULL) {
            
            struct client_info* next = client->next;

            if (FD_ISSET(client->socket, &reads)) {

                // Skips client if getsockopt() returns an error, dies if getsockopt() failed
                // maybe should handle error
                int errorVal = 0;
                socklen_t size = client->address_length;
                if (getsockopt(client->socket, SOL_SOCKET, SO_ERROR, &errorVal, &size) < 0) {
                    fprintf(stderr, "getsockopt() failed. (%d, %s)\n", errno, strerror(errno));
                    print_client(client, stderr);
                    return 1;
                }
                if (errorVal < 0) {
                    printf("getsockopt(), skipped client. (%d, %s)\n", errorVal, strerror(errorVal));
                    client = next;
                    continue;
                }

                if (MAX_REQUEST_SIZE == client->received) {
                    send_400(client, NULL);
                    continue;
                }

                int bytes_read = SSL_read(client->ssl,
                            client->request + client->received,
                            MAX_REQUEST_SIZE - client->received);
                #if PRINT_RECEIVED == true
                printf("\nReceived:\n%s\n", client->request);
                #endif

                if (bytes_read < 1) {
                    printf("Unexpected disconnect from %s.\n", get_client_address(client));
                    drop_client(client);
                    continue;
                }
                client->received += bytes_read;
                client->request[client->received] = 0;

                char* q = strstr(client->request, "\r\n\r\n");
                if (q) {
                    // strncmp returns 0 if strings are equal
                    if (strncmp("GET /", client->request, 5) == 0) {
                        char* path = client->request + 4;
                        char* end_path = strstr(path, " ");
                        if (!end_path) {
                            send_400(client, NULL);
                        } else {
                            *end_path = 0;
                            serve_resource(client, path);
                        }
                    } else if (strncmp("POST /", client->request, 6) == 0) {
                        handle_post(client);
                    } else {
                        send_400(client, NULL);
                    }
                }
            }

            client = next;
        }
    }

    printf("\nClosing server...\n");
    // while might be buggy, untested
    while (clients) drop_client(clients);
    SSL_CTX_free(ctx);
    close(server);
    printf("Finished.\n");
    return 0;
}

#if PRINT_FILE_TYPE == true
#define return type =
#endif
const char* get_content_type(const char* path) {

    const char* last_dot = strrchr(path, '.');
    char* type = "application/octet-stream";
    if (last_dot) {
        if (strcmp(last_dot, ".html") == 0) return "text/html";
        if (strcmp(last_dot, ".css") == 0) return "text/css";
        if (strcmp(last_dot, ".js") == 0) return "application/javascript";
        if (strcmp(last_dot, ".ico") == 0) return "image/x-icon";

        if (strcmp(last_dot, ".csv") == 0) return "text/csv";
        if (strcmp(last_dot, ".gif") == 0) return "image/gif";
        if (strcmp(last_dot, ".htm") == 0) return "text/html";
        if (strcmp(last_dot, ".jpeg") == 0) return "image/jpeg";
        if (strcmp(last_dot, ".jpg") == 0) return "image/jpeg";
        if (strcmp(last_dot, ".json") == 0) return "application/json";
        if (strcmp(last_dot, ".png") == 0) return "image/png";
        if (strcmp(last_dot, ".pdf") == 0) return "application/pdf";
        if (strcmp(last_dot, ".svg") == 0) return "iamge/svg+xml";
        if (strcmp(last_dot, ".txt") == 0) return "text/plain";
    }
    #if PRINT_FILE_TYPE == true
    #undef return 
    printf("Type = %s\n", type);
    printf("Path = %s\n\n", path);
    #endif
    return type;
}

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
        fprintf(stderr, "socket() failed. (%d, %s).\n", errno, strerror(errno));
        exit(1);
    }

    // Lets you reuse a port if it was recently in use (claims it from OS)
    int yes = 1;
    if (setsockopt(socket_listen, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
        perror("setsockopt error\n");
        exit(1);
    }
    
    printf("Binding socket to local address...\n");
    if (bind(socket_listen,
            bind_address->ai_addr, bind_address->ai_addrlen)) {
        fprintf(stderr, "bind() failed. (%d, %s).\n", errno, strerror(errno));
        exit(1);
    }  
    freeaddrinfo(bind_address);

    printf("Listening...\n");
    if (listen(socket_listen, 10) < 0) {
        fprintf(stderr, "listen() failed. (%d, %s).\n", errno, strerror(errno));
        exit(1);
    }

    return socket_listen;
}

struct client_info* get_client(int socket) {

    struct client_info* ci = clients;

    while (ci) {
        if (ci->socket == socket) break;
        ci = ci->next;
    }
    if (ci) return ci;

    struct client_info* new = (struct client_info*) calloc(1, sizeof(struct client_info));

    if (!new) {
        fprintf(stderr, "Out of memory.\n");
        exit(1);
    }

    new->address_length = sizeof(new->address);
    new->next = clients;
    clients = new;
    new->parseFailures = 0;
    return new;
}

void drop_client(struct client_info* client) {

    SSL_shutdown(client->ssl);
    close(client->socket);
    SSL_free(client->ssl);

    struct client_info** pointer = &clients;

    while (*pointer) {
        if (*pointer == client) {
            *pointer = client->next;
            // client = client->next?? equivalent?
            free(client);
            return;
        }
        pointer = &(*pointer)->next;
    }

    fprintf(stderr, "drop_client(). Client not found.\n");
    exit(1);
}

const char* get_client_address(struct client_info* ci) {

    // static might create problems with multuihreading
    static char address_buffer[100];
    getnameinfo((struct sockaddr*) &ci->address,
            ci->address_length,
            address_buffer, sizeof(address_buffer), 0, 0,
            NI_NUMERICHOST);
    return address_buffer;
}

fd_set wait_on_clients(int server) {
    fd_set reads;
    FD_ZERO(&reads);
    FD_SET(server, &reads);
    int max_socket = server;

    struct client_info* ci = clients;

    while (ci) {
        FD_SET(ci->socket, &reads);
        if (ci->socket > max_socket)
            max_socket = ci->socket;
        ci = ci->next;
    }

    struct timeval timeout;
    timeout.tv_sec = 20;
    timeout.tv_usec = 0;
    if (select(max_socket + 1, &reads, 0, 0, &timeout) < 0) {
        fprintf(stderr, "select() failed. (%d, %s)\n", errno, strerror(errno));
        exit(1);
    }

    return reads;
}

void send_301(struct client_info* client) {
    const char* c301 = "HTTP/1.1 301 Moved Permanently\r\n"
            "Location: https://www.purplesite.skin\r\n\r\n";
    printf("Sent default 301:\n%s\n", c301);
    send(client->socket, c301, strlen(c301), 0);
    drop_client(client);
    return;
}

#if PRINT_400 == false
#define printf(...)
#endif
void send_400(struct client_info* client, char* string) {
    if (string == NULL || strlen(string) == 0) {
        const char* c400 = "HTTP/1.1 400 Bad Request\r\n"
            "Connection: close\r\n"
            "Content-Length: 11\r\n\r\nBad Request";
        printf("Sent default 400:\n%s\n", c400);
        SSL_write(client->ssl, c400, strlen(c400));
        drop_client(client);
        return;
    }
    const char* beginningString = "HTTP/1.1 400 ";
    const char* endingString = "Connection: close\r\n"
            "Content-Length: 0\r\n\r\n";
    if (strlen(string) + strlen(beginningString) + strlen(endingString) > 253) {
        printf("string was to large, got chopped\n");
        string[253 - strlen(beginningString) - strlen(endingString)] = 0;
    }
    char c400[256] = {0};
    strcpy(c400, beginningString);

    char* pointer = c400 + strlen(beginningString);
    for (int i = 0; i < strlen(string); i++) {
        *pointer = string[i];
        pointer++;
    }
    strncpy(pointer, "\r\n", 2);
    pointer += 2;

    strcpy(pointer, endingString);

    SSL_write(client->ssl, c400, 256);
    drop_client(client);
    printf("Sent custom 400:\n%s\n", c400);
}
#if PRINT_400 == false
#undef printf
#endif

void send_404(struct client_info* client) {
    const char* c404 = "HTTP/1.1 404 Not Found\r\n"
        "Connection: close\r\n"
        "content-Length: 9\r\n\r\nNot Found";
    //send(client->socket, c404, strlen(c404), 0);
    SSL_write(client->ssl, c404, strlen(c404));
    drop_client(client);
}

void serve_resource(struct client_info* client, const char* path) {

    printf("serve_resource() %s %s\n", get_client_address(client), path);

    if (strcmp(path, "/") == 0) path = "/index.html";

    if (strlen(path) > 100) {
        printf("Path > 100\n");
        send_400(client, NULL);
        return;
    }

    if (strstr(path, "..")) {
        printf("Path contained ..\n");
        send_404(client);
        return;
    }

    if (strstr(path, "public") != 0) {
        printf("sussy request\n");
        send_404(client);
        return;
    }

    char full_path[128];
    sprintf(full_path, "public%s", path);
    
    
    FILE* fp = fopen(full_path, "rb");

    if (fp == NULL) {
        printf("File failed to open, full_path = %s. Errno = %d, errstr = %s\n", full_path, errno, strerror(errno));
        send_404(client);
        return;
    }
    
    fseek(fp, 0L, SEEK_END);
    size_t cl = ftell(fp);
    rewind(fp);

    const char* ct = get_content_type(full_path);
    

    #define BSIZE 1024
    char buffer[BSIZE];

    sprintf(buffer, "HTTP/1.1 200 OK\r\n");
    //send(client->socket, buffer, strlen(buffer), 0);
    SSL_write(client->ssl, buffer, strlen(buffer));
    
    sprintf(buffer, "Connection: close\r\n");
    //send(client->socket, buffer, strlen(buffer), 0);
    SSL_write(client->ssl, buffer, strlen(buffer));
    
    sprintf(buffer, "Content-Length: %lu\r\n", cl);
    //send(client->socket, buffer, strlen(buffer), 0);
    SSL_write(client->ssl, buffer, strlen(buffer));
    
    sprintf(buffer, "Content-Type: %s\r\n", ct);
    //send(client->socket, buffer, strlen(buffer), 0);
    SSL_write(client->ssl, buffer, strlen(buffer));
    
    sprintf(buffer, "\r\n");
    //send(client->socket, buffer, strlen(buffer), 0);
    SSL_write(client->ssl, buffer, strlen(buffer));
    
    int bytes_read = fread(buffer, 1, BSIZE, fp);
    while (bytes_read != 0) {
        //send(client->socket, buffer, bytes_read, 0);
        SSL_write(client->ssl, buffer, bytes_read);
        bytes_read = fread(buffer, 1, BSIZE, fp);
    }

    fclose(fp);
    drop_client(client);
}

#if PRINT_POST_PARSE == false
#define printf(...) 
#endif

void handle_post(struct client_info* client) {
    

    printf("handle_post() \n");
    // Should add user file data into data strcture in main()
    //struct user_data users = parse_user_file_data();

    if (strncmp(client->request, "POST / HTTP/1.1\r\n", 17) != 0) {
        printf("First post line malformed\n");
        send_400(client, NULL);
        return;
    }

    char* end = client->received + client->request;
    char* pointer = client->request;
    char* quinter;
    // FIND CONTENT LENGTH
    #define contentLengthFail(x) printf("Failed to parse content length. Line %d\n", x); \
        send_400(client, NULL); \
        return

    while (strncmp(pointer, "Content-Length: ", 16) != 0) {

        pointer = strstr(pointer, "\n");
        if (pointer == 0) {
            contentLengthFail(__LINE__);
        }
        if (++pointer >= end) {
            contentLengthFail(__LINE__);
        }
    }
    if (pointer + 16 >= end) {
        contentLengthFail(__LINE__);
    }
    pointer += 16;
    quinter = pointer;
    quinter = strstr(quinter, "\r\n");
    if (quinter == 0) {
        contentLengthFail(__LINE__);
    }
    if (quinter - pointer > 4 || quinter - pointer <= 0) {
        contentLengthFail(__LINE__);
    }
    char contentLengthBuffer[5];
    for (int i = 0; i < quinter - pointer; i++) {
        contentLengthBuffer[i] = pointer[i];
    }
    contentLengthBuffer[quinter - pointer + 1] = 0;

    int contentLength = atoi(contentLengthBuffer);

    pointer = quinter;
    pointer += 2;

    #undef contentLengthFail
    // END OF FIND CONTENT LENGTH

    // FIND CONTENT TYPE
    #define contentTypeFail(x) printf("Failed to parse content type. Line %d\n", x); \
        send_400(client, NULL); \
        return
    
    while (strncmp(pointer, "Content-Type: ", 14) != 0) {

        pointer = strstr(pointer, "\n");
        if (pointer == 0) {
            contentTypeFail(__LINE__);
        }
        if (++pointer >= end) {
            contentTypeFail(__LINE__);
        }
    }
    if (pointer + 22 >= end) {
        contentTypeFail(__LINE__);
    }
    pointer += 14;
    quinter = pointer;
    quinter = strstr(quinter, ";");
    if (quinter == 0) {
        contentTypeFail(__LINE__);
    }
    if (quinter - pointer > 31 || quinter - pointer <= 0) {
        printf("Bro content-type is wack\n");
        contentTypeFail(__LINE__);
    }
    char contentTypeBuffer[32];
    for (int i = 0; i < quinter - pointer; i++) {
        contentTypeBuffer[i] = pointer[i];
    }
    contentTypeBuffer[quinter - pointer] = 0;

    pointer = quinter;
    pointer++;

    #undef contentTypeFail
    // END OF FIND CONTENT TYPE

    // FIND BOUNDRY
    #define boundaryFail(x) printf("Failed to parse boundry. Line %d\n", x); \
        send_400(client, NULL); \
        return

    if (pointer >= end) {
        boundaryFail(__LINE__);
    }
    if (strncmp(pointer, " boundary=", 10) != 0) {
        printf("pointer = %.*s\n", 10, pointer);
        boundaryFail(__LINE__);
    }
    pointer += 10;
    quinter = strstr(pointer, "\r\n");
    if (quinter - pointer > 68 || quinter - pointer <= 0) {
        boundaryFail(__LINE__);
    }
    char boundary[69] = "--";
    for (int i = 0; i < quinter - pointer; i++) {
        boundary[i + 2] = pointer[i];
    }
    boundary[quinter - pointer + 2] = 0;
    pointer = quinter;
    pointer += 2;

    #undef boundaryFail
    // END OF FIND BOUNDRY

    // LOCATE CONTENT
    #define locateContentFail(x, y) printf("Failed to locate content. Line %d\n", x); \
        if ((pointer - client->request) < contentLength && client->parseFailures < 2) {\
            printf("\nClient did not send enough data\n\n"); \
            client->parseFailures += 1; \
            return; \
        }\
        send_400(client, y); \
        return

    // GET USERNAME ///////////////////////////////////////////////
    if (pointer >= end) {
        locateContentFail(__LINE__, NULL);
    }
    quinter = strstr(pointer, boundary);
    if (quinter == 0) {
        printf("boundary = %s\n", boundary);
        printf("pointer: %.*s\n", 10, pointer);
        locateContentFail(__LINE__, NULL);
    }  
    pointer = quinter;
    pointer += strlen(boundary);
    if (strncmp(pointer, "\r\n", 2) != 0) {
        fprintf(stderr, "pointer = %s\n", pointer);
        locateContentFail(__LINE__, NULL);
    }
    pointer += 2;

    if (strncmp(pointer, "Content-Disposition: form-data; name=\"username\"", 47) != 0) {
        printf("penis");
        locateContentFail(__LINE__, NULL);
    }
    pointer += 47;
    if (strncmp(pointer, "\r\n", 2) != 0) {
        locateContentFail(__LINE__, NULL);
    }
    pointer += 2;

    quinter = strstr(pointer, boundary);
    if (quinter - pointer <= 2 || quinter - pointer > MAX_NAME_BYTES + 2) {
        printf("pointer = %p\n", pointer);
        printf("quinter = %p\n", quinter);
        locateContentFail(__LINE__, "Failed to locate content, malformed or request size too large\n");
    }
    
    // Should skip through spaces and stuff
    while (*pointer == ' ' || *pointer == '\t' || *pointer == '\n' || *pointer == '\r'){
        pointer++;
        if (pointer >= end) {
            locateContentFail(__LINE__, NULL);
        }
    }
    if (quinter - pointer > MAX_NAME_BYTES) {
        printf("pointer = %p\n", pointer);
        printf("quinter = %p\n", quinter);
        locateContentFail(__LINE__, NULL);
    }
    
    char username[MAX_NAME_BYTES + 1];
    for (int i = 0; i < quinter - pointer; i++) {
        if (*pointer == ' ' || *pointer == '\t' || *pointer == '\n' || *pointer == '\r'){
            continue;
        }
        username[i] = pointer[i];
    }
    // Remove whitespace from end
    char* endOfUsername = quinter - 1;
    int whiteSpaceCount = 0;
    while (*endOfUsername == ' ' || *endOfUsername == '\t' || *endOfUsername == '\n' || *endOfUsername == '\r') {
        //printf("whitepsace removed\n");
        if (endOfUsername <= pointer) {
            break;
        }
        *endOfUsername = 0;
        endOfUsername--;
        whiteSpaceCount++;
    }

    if (quinter - pointer - whiteSpaceCount <= 0) {
        printf("Received empty username\n");
        send_400(client, "Empty username");
        return;
    }
    username[quinter - pointer - whiteSpaceCount] = 0;

    // Check if username string is empty
    if (strlen(username) == 0) {
        printf("Received empty username\n");
        send_400(client, "Empty username");
        return;
    }

    int kosher_chars;
    kosher_chars = num_kosher_chars(username, 0);
    if (kosher_chars <= 0) {
        locateContentFail(__LINE__, "Username is not kosher");
    }
    if (kosher_chars > MAX_NAME_CHARACTERS) {
        locateContentFail(__LINE__, "Username contained too many characterss");
    }

    if (contains_profanity(username)) {
        locateContentFail(__LINE__, "Username has swears );");
    }

    pointer = strstr(quinter, "\r\n");
    if (pointer == 0) {
        locateContentFail(__LINE__, NULL);
    }
    pointer += 2;
    // END USERNAME ///////////////////////////////////////////////

    // GET RECIPIENT ////////////////////////////////////////////////
    if (strncmp(pointer, "Content-Disposition: form-data; name=\"recipient\"", 48) != 0) {
        locateContentFail(__LINE__, NULL);
    }
    pointer += 48;

    if (strncmp(pointer, "\r\n", 2) != 0) {
        locateContentFail(__LINE__, NULL);
    }
    pointer += 2;

    quinter = strstr(pointer, boundary);
    if (quinter - pointer <= 2 || quinter - pointer > MAX_NAME_BYTES + 2) {
        locateContentFail(__LINE__, NULL);
    }

    // Should skip through whitespace
    while (*pointer == ' ' || *pointer == '\t' || *pointer == '\n' || *pointer == '\r'){
        pointer++;
        if (pointer >= end) {
            locateContentFail(__LINE__, NULL);
        }
    }
    if (quinter - pointer > MAX_NAME_BYTES) {
        locateContentFail(__LINE__, NULL);
    }
    char recipient[MAX_NAME_BYTES + 1];
    for (int i = 0; i < quinter - pointer; i++) {
        recipient[i] = pointer[i];
    }
    // Remove whitespace from end
    char* endOfRecipient = quinter - 1;
    whiteSpaceCount = 0;
    while (*endOfRecipient == ' ' || *endOfRecipient == '\t' || *endOfRecipient == '\n' || *endOfRecipient == '\r') {
        //printf("whitepsace removed\n");
        if (endOfRecipient <= pointer) {
            break;
        }
        *endOfRecipient = 0;
        endOfRecipient--;
        whiteSpaceCount++;
    }

    if (quinter - pointer - whiteSpaceCount <= 0) {
        printf("Received empty recipient\n");
        send_400(client, "Empty recipient");
        return;
    }
    recipient[quinter - pointer - whiteSpaceCount] = 0;

    // Check if recipient string is empty
    if (strlen(recipient) == 0) {
        printf("Received empty recipient\n");
        send_400(client, "Empty recipient");
        return;
    }

    // Check if recipient is sender (username)
    bool recip_equals_sender = false;
    if (strlen(recipient) == strlen(username)) {
        recip_equals_sender = true;
        for (int i = 0; i < strlen(recipient); i++) {
            unsigned char byte = username[i]; 
            if (byte >> 7 == 0) {
                // ANSI character if topmost bit is 0
                if (tolower(recipient[i]) != tolower(username[i])) {
                    recip_equals_sender = false;
                    break;
                }
                continue;
            }
            if (recipient[i] != username[i]) {
                recip_equals_sender = false;
                break;
            }
            /* Cool bit printer
            printf("Printing bits of non-ascii char\n");
            int k = 7;
            for (int j = 128; j > 0; j/=2) {
                printf("%u", (j & byte) >> k);
                k--;
            }
            printf("\n");
            */
        }
    }
    if (recip_equals_sender) {
        locateContentFail(__LINE__, "Recipient equals sender");
    }

    kosher_chars = num_kosher_chars(recipient, 0);
    if (kosher_chars <= 0) {
        locateContentFail(__LINE__, "Recipient is not kosher");
    }
    if (kosher_chars > MAX_NAME_CHARACTERS) {
        locateContentFail(__LINE__, "Recipient contained too many characterss");
    }

    if (contains_profanity(recipient)) {
        locateContentFail(__LINE__, "Recipient has swears );");
    }

    pointer = strstr(quinter, "\r\n");
    if (pointer == 0) {
        locateContentFail(__LINE__, NULL);
    }
    pointer += 2;
    // END RECIPIENT ////////////////////////////

    // GET TITLE ////////////////////////////////////////////////
    if (strncmp(pointer, "Content-Disposition: form-data; name=\"title\"", 44) != 0) {
        locateContentFail(__LINE__, NULL);
    }
    pointer += 44;

    if (strncmp(pointer, "\r\n", 2) != 0) {
        locateContentFail(__LINE__, NULL);
    }
    pointer += 2;

    quinter = strstr(pointer, boundary);
    if (quinter - pointer <= 2 || quinter - pointer > MAX_TITLE_BYTES) {
        locateContentFail(__LINE__, NULL);
    }

    // Should skip through whitespace
    while (*pointer == ' ' || *pointer == '\t' || *pointer == '\n' || *pointer == '\r') {
        pointer++;
        if (pointer >= end) {
            locateContentFail(__LINE__, NULL);
        }
    }
    char title[MAX_TITLE_BYTES + 1];
    for (int i = 0; i < quinter - pointer; i++) {
        title[i] = pointer[i];
    }
    // Remove whitespace from end
    char* endOfTitle= quinter - 1;
    whiteSpaceCount = 0;
    while (*endOfTitle == ' ' || *endOfTitle == '\t' || *endOfTitle == '\n' || *endOfTitle == '\r') {
        //printf("whitepsace removed\n");
        if (endOfTitle <= pointer) {
            break;
        }
        *endOfTitle = 0;
        endOfTitle--;
        whiteSpaceCount++;
    }

    if (quinter - pointer - whiteSpaceCount <= 0) {
        printf("Received empty title\n");
        send_400(client, "Empty title");
        return;
    }
    title[quinter - pointer - whiteSpaceCount] = 0;

    // Check if title string is empty
    if (strlen(title) == 0) {
        printf("Received empty title\n");
        send_400(client, "Empty title");
        return;
    }

    kosher_chars = num_kosher_chars(title, 0);
    if (kosher_chars <= 0) {
        locateContentFail(__LINE__, "Title is not kosher");
    }
    if (kosher_chars > MAX_TITLE_CHARACTERS) {
        locateContentFail(__LINE__, "Title contained too many characterss");
    }

    if (contains_profanity(title)) {
        locateContentFail(__LINE__, "Title has swears );");
    }

    pointer = strstr(quinter, "\r\n");
    if (pointer == 0) {
        locateContentFail(__LINE__, NULL);
    }
    pointer += 2;

    // END TITLE ////////////////////////////

    if (strncmp(pointer, "Content-Disposition: form-data; name=\"composeText\"", 50) != 0) {
        locateContentFail(__LINE__, NULL);
    }
    pointer += 50;

    if (strncmp(pointer, "\r\n", 2) != 0) {
        locateContentFail(__LINE__, NULL);
    }
    pointer += 2;

    quinter = strstr(pointer, boundary);
    if (quinter - pointer <= 2 || quinter - pointer > MAX_COMPOSE_BYTES) {
        locateContentFail(__LINE__, NULL);
    }

    // Should skip through whitespace
    while (*pointer == ' ' || *pointer == '\t' || *pointer == '\n' || *pointer == '\r') {
        pointer++;
        if (pointer >= end) {
            locateContentFail(__LINE__, NULL);
        }
    }
    char textAreaBuffer[MAX_COMPOSE_BYTES + 1];
    for (int i = 0; i < quinter - pointer; i++) {
        textAreaBuffer[i] = pointer[i];
    }
    // Remove whitespace from end
    char* endOfTextArea = quinter - 1;
    whiteSpaceCount = 0;
    while (*endOfTextArea == ' ' || *endOfTextArea == '\t' || *endOfTextArea == '\n' || *endOfTextArea == '\r') {
        //printf("whitepsace removed\n");
        if (endOfTextArea <= pointer) {
            break;
        }
        *endOfTextArea = 0;
        endOfTextArea--;
        whiteSpaceCount++;
    }

    if (quinter - pointer - whiteSpaceCount <= 0) {
        printf("Received empty text area\n");
        send_400(client, "Empty text area");
        return;
    }
    textAreaBuffer[quinter - pointer - whiteSpaceCount] = 0;

    if (strlen(textAreaBuffer) == 0) {
        locateContentFail(__LINE__, "Empty text area");
    }

    kosher_chars = num_kosher_chars(textAreaBuffer, 1);
    if (kosher_chars <= 0) {
        locateContentFail(__LINE__, "Text area is not kosher");
    }
    if (kosher_chars > MAX_COMPOSE_CHARACTERS) {
        locateContentFail(__LINE__, "Text area contained too many characters");
    }

    if (contains_profanity(textAreaBuffer)) {
        locateContentFail(__LINE__, "Text area has swears );");
    }


    int charCount = kosher_chars;
    // END OF TRUNCATE
    int text_area_hash = hash_function(textAreaBuffer);

    FILE* file = fopen("public/emails.txt", "a");
    if (file == NULL) {
        fprintf(stderr, "public/emails.txt failed to open\n");
        fprintf(stderr, "errno = %d\n", errno);
        fprintf(stderr, "Error: %s\n", strerror(errno));
        exit(1);
    }

    // UNTESTED WITH LARGE FILE SIZES
    // right now i am assuming 32 bit hash
    printf("\nchecking body hash\n");
    char body_hash[64] = {0};
    char hash_string[64] = {0};
    sprintf(body_hash, "%d", text_area_hash);
    strcat(hash_string, "\nhash: ");
    strcat(hash_string, body_hash);
    strcat(hash_string, "\n");

    // too hard
    /*#define READ_BUFFER_SIZE 1024 * 4
    char read_buffer[READ_BUFFER_SIZE];
    int bytes_read = 0;
    char recovery_bytes[64] = {0};
    bool recover = false;
    char recovered_msg[64] = {0};
    // CURENNTLY WORKING ON RECOVERY MODE
    do {

        bytes_read = fread(read_buffer, READ_BUFFER_SIZE, 1, file);
        int start_value = 0;
        if (recover) {
            char* base = recovery_bytes;
            char* pointer = recovery_bytes;
            char* quinter = hash_string;
            bool found = true;
            while (pointer < recovery_bytes + 64 && pointer != 0) {
                if (*pointer != *quinter) {
                    pointer = strstr(pointer, "\r");
                    if (pointer == 0) {
                        found = false;
                        break;
                    }
                    base = pointer;
                    quinter = hash_string;
                    continue;
                }
                pointer++;
                quinter++;
            }
            if (found) {
                char hash_line[64] = {};
                if (*pointer != '\r') {
                    printf("OOPS\n");
                }
                for (int i = 0; i < strlen(base); i++) {
                    hash_line[i] = base[i];
                }
                if (quinter == 0) {
                    int x = 0;
                    int y = strlen(base);
                    while (read_buffer[x] != '\r') {
                        hash_line[y] = read_buffer[x];
                        x++;
                        y++;
                        if (x == strlen(read_buffer)) {
                            goto END;
                        }
                    }
                }
                // home run
                if (strcmp(hash_line, hash_string) == 0) {
                    locateContentFail(__LINE__, "Message sent already");
                }
                start_value = strlen(hash_string);
            }
        }
        printf("bytes_read = %d\n", bytes_read);
        for (int i = start_value; i < bytes_read; i++) {
            if (read_buffer[i] != '\r') {
                continue;
            }
            if (bytes_read - i < strlen(hash_string)) {
                printf("recovery mode\n");
                for (int j = i; j < bytes_read; j++) {
                    recovery_bytes[j] = read_buffer[i];
                    i++;
                }
                recover = true;
                break;
            }
            if (strcmp(read_buffer + i, hash_string) != 0) {
                continue;
            }

            pretty sure this isn't useful
            char* pointer = strstr(read_buffer + i, hash_string);
            printf("found matching string");
            pointer += 8;
            char* end = strstr(pointer, "\r\n");
            if (end == 0) {
                printf("end equaled 0 during hash parsing, really bad\n");
                continue;
            }
            char hash_read[64] = {0};
            sprintf(hash_read, "%.*s", (int) (end - pointer), pointer);
            printf("body hash = %s: compared hash = %s\n", body_hash, hash_read);
            if (strcmp(hash_read, body_hash) != 0) {
                continue;
            }
            end of not useful
            
            locateContentFail(__LINE__, "Message sent already");
        }
        END:
    } while (bytes_read == READ_BUFFER_SIZE);
    printf("\n");
    #undef READ_BUFFER_SIZE
    */
    // END OF TEXT AREA

    #undef locateContentFail
    //END OF LOCATE CONTENT
    printf("Content-Length = %d\n", contentLength);
    printf("Content-Type = %s\n", contentTypeBuffer);
    printf("username = %s\n", username);
    printf("recipient = %s\n", recipient);
    printf("title = %s\n", title);
    printf("composeText = %s\n", textAreaBuffer);
    printf("\nsize of textAreaBuffer = %ld\n", strlen(textAreaBuffer));
    printf("char count = %d\n", charCount);
    printf("\n");
    printf("Text area hash = %d\n", text_area_hash);
    printf("End of content\n");
    
    struct email new_email = {0};
    new_email.hash = text_area_hash;
    strcpy(new_email.sender, username);
    strcpy(new_email.recipient, recipient);
    strcpy(new_email.title, title);
    strcpy(new_email.body, textAreaBuffer);
    new_email.upvotes = 0;
    time_t rawtime;
    time(&rawtime);
    printf("time created = %ld\n", rawtime);
    new_email.creationDate = rawtime;

    fwrite("\r\nrecipient: ", 13, 1, file);
    fwrite(recipient, strlen(recipient), 1, file);
    
    fwrite("\r\nhash: ", 8, 1, file);
    char hash_as_string[32 + 1];
    int temp = text_area_hash;
    int count = 0;
    while (temp > 0) {
        temp /= 10;
        count++;
    }
    sprintf(hash_as_string, "%d", text_area_hash);
    fwrite(hash_as_string, count, 1, file);

    fwrite("\r\nsender: ", 10, 1, file);
    fwrite(username, strlen(username), 1, file);
    fwrite("\r\ntitle: ", 9, 1, file);
    fwrite(title, strlen(title), 1, file);
    fwrite("\r\nbody: ", 8, 1, file);
    fwrite(textAreaBuffer, strlen(textAreaBuffer), 1, file);
    fwrite("\r\nupvotes: 0", 12, 1, file);
    fwrite("\r\ntime created: ", 16, 1, file);
    char time_as_string[64 + 1];
    long l_temp = rawtime;
    count = 0;
    while (l_temp > 0) {
        l_temp /= 10;
        count++;
    }
    sprintf(time_as_string, "%ld", rawtime);
    fwrite(time_as_string, count, 1, file);
    fwrite("\r\n\r\n", 4, 1, file);


    fclose(file);


    char buffer[BSIZE];
    //sprintf(buffer, "HTTP/1.1 200 OK\r\n");
    sprintf(buffer, "HTTP/1.1 201 Created\r\n");
    SSL_write(client->ssl, buffer, strlen(buffer));

    sprintf(buffer, "Access-Control-Expose-Headers: Location\r\n");
    SSL_write(client->ssl, buffer, strlen(buffer));

    sprintf(buffer, "Location: /emails.txt\r\n");
    SSL_write(client->ssl, buffer, strlen(buffer));
    
    sprintf(buffer, "\r\n");
    SSL_write(client->ssl, buffer, strlen(buffer));

    drop_client(client);
}
#if PRINT_POST_PARSE == false
#undef printf
#endif

typedef union {
    char c[4];
    int i;
} raw_int;

// type 0 is username, recipient, title. type 1 is textarea
int num_kosher_chars(char* string, int type) {

    int char_count = 0;

    // generates the 4 byte value of the allowed emojis
    int emojiArray[9];
    for (int i = 0; i < 9; i++) {
        emojiArray[i] = -2070372368 + (i * 16777216);
    }
    int total_emojis = 9;
    for (int i = 0; i < strlen(string); i++) {
        if (type == 1 && char_count > MAX_COMPOSE_CHARACTERS) {
            string[i] = 0;
            break;
        }
        unsigned char byte = string[i];
        if (byte >> 7 == 0) {
            // ASCII, doesn't matter if using signed or unsigned char here cause top bit is 0
            // should only allow newline when text area;
            if ((byte < 32 || byte > 126) && ((byte != '\n' && byte != '\r') || type == 0)) {
                printf("String had bad ascii character\n");
                int k = 7;
                for (int j = 128; j > 0; j/=2) {
                    printf("%u", (j & byte) >> k);
                    k--;
                }
                printf("\n");
                return -1;
            }
            if (byte == '\r' && (i + 1) < strlen(string) && string[i + 1] == '\n') {
                printf("illegal \\r\\n in string\n");
                return -1;
            }
            char_count++;
            continue;
        }
        // Unicode
        if (strlen(string) - i < 4) {
            printf("String did not have enough room for unicode emoji\n");
            return -1;
        }

        printf("Printing bits of non-ascii char\n");
        for (int iter = 0; iter < 4; iter++) {
            unsigned char byte = string[i + iter];
            int k = 7;
            for (int j = 128; j > 0; j/=2) {
                printf("%u", (j & byte) >> k);
                k--;
            }
            printf("\n");
        }
        raw_int INTEGER;
        INTEGER.c[0] = string[i];
        INTEGER.c[1] = string[i+1];
        INTEGER.c[2] = string[i+2];
        INTEGER.c[3] = string[i+3];


        bool is_in_emoji_array = false;
        for (int j = 0; j < total_emojis; j++) {
            raw_int emoji_int;
            emoji_int.i = emojiArray[j];
            
            printf("emoji_int = %d\n", emoji_int.i);
            printf("INTEGER = %d\n", INTEGER.i);
            if (INTEGER.i == emoji_int.i) {
                is_in_emoji_array = true;
                break;
            }
        }
        if (!is_in_emoji_array) {
            printf("string = %s\n", string);
            printf("String's emoji not in emoji array\n");
            return -1;
        }
        i += 3;
        char_count++;
    }
    return char_count;
}


struct email parse_user_file_data(char* username) {
    /*
    FILE* fp = fopen("userData.txt", "r");

    if (fp == NULL) {
        printf("File failed to open, full_path = %s. Errno = %d, errstr = %s\n", full_path, errno, strerror(errno));
        send_404(client);
        return;
    }
    
    fseek(fp, 0L, SEEK_END);
    size_t cl = ftell(fp);
    rewind(fp);

    const char* ct = get_content_type(full_path);
    

    #define BSIZE 1024
    char buffer[BSIZE];

    sprintf(buffer, "HTTP/1.1 200 OK\r\n");
    //send(client->socket, buffer, strlen(buffer), 0);
    SSL_write(client->ssl, buffer, strlen(buffer));
    
    sprintf(buffer, "Connection: close\r\n");
    //send(client->socket, buffer, strlen(buffer), 0);
    SSL_write(client->ssl, buffer, strlen(buffer));
    
    sprintf(buffer, "Content-Length: %lu\r\n", cl);
    //send(client->socket, buffer, strlen(buffer), 0);
    SSL_write(client->ssl, buffer, strlen(buffer));
    
    sprintf(buffer, "Content-Type: %s\r\n", ct);
    //send(client->socket, buffer, strlen(buffer), 0);
    SSL_write(client->ssl, buffer, strlen(buffer));
    
    sprintf(buffer, "\r\n");
    //send(client->socket, buffer, strlen(buffer), 0);
    SSL_write(client->ssl, buffer, strlen(buffer));
    
    int bytes_read = fread(buffer, 1, BSIZE, fp);
    while (bytes_read != 0) {
        //send(client->socket, buffer, bytes_read, 0);
        SSL_write(client->ssl, buffer, bytes_read);
        bytes_read = fread(buffer, 1, BSIZE, fp);
    }

    fclose(fp);
    drop_client(client);
    */
}

#if PRINT_HASH == false
#define printf(...) 
#endif
// should only put kosher strings into this function
int hash_function(char* string) {
    
    unsigned char copy_string[1024];
    strcpy(copy_string, string);
    for (int i = 0; i < strlen(string); i++) {
        copy_string[i] = tolower(copy_string[i]);
    }
    printf("hash_function() received: %s\n", copy_string);
    int hash = 5381;
    for (int i = 0; i < strlen(string); i++) {

        printf("hash = %d\n", hash);
        printf("profanity_hash[0] = %d\n", profanity_hash_list[0]);
        // if emoji
        if (copy_string[i] >> 7 == 1) {
            printf("hash_function() detected emoji\n");
            hash = ((hash << 5) + hash) + 'o';
            i += 3;
            continue;
        }
        bool shouldSkip = false;
        switch(copy_string[i]) {
            case ' ': shouldSkip = true; break;
            case '_': shouldSkip = true; break;
            case '(': if (i + 1 < strlen(string) && copy_string[i+1] == ')') {
                hash = ((hash << 5) + hash) + 'o';
                i++;
                shouldSkip = true;
                } break;
            case '0': copy_string[i] = 'o'; break;
            case '@': copy_string[i] = 'a'; break;
            case 'i': copy_string[i] = 'l'; break;
            case '|': copy_string[i] = 'l'; break;
            case '!': copy_string[i] = 'l'; break;
            case '3': copy_string[i] = 'e'; break;
            case '$': copy_string[i] = 's'; break;
            default: break;
        }
        if (shouldSkip) {
            continue;
        }
        hash = ((hash << 5) + hash) + copy_string[i];
    }
    if (hash == 0) {
        printf("hash equaled 0, incrementing it by 1\n");
        return 1;
    }
    return hash;
}
#if PRINT_HASH == false
#undef printf
#endif

#if PROFANITY_PRINT == false
#define printf
#endif
void hash_profanity_list() {
    printf("\nPRINTING PROFANITY\n");
    FILE* file = fopen("profanity.txt", "r");
    char line[64];
    int iter = 0;
    while (fgets(line, 64, file)) {
        profanity_list[iter++] = line;
        int size = strlen(line);
        printf("strlen(line) = %d\n", size);
        printf("max_size_profanity = %d\n", max_size_profanity);
        if (strlen(line) < min_size_profanity) {
            min_size_profanity = strlen(line);
        }
        if (size > max_size_profanity) {
            max_size_profanity = size;
            printf("max_size_profanity changed to %d\n", size);
        }
        printf("added %s to profanity list\n", line);
    }
    fclose(file);

    iter = 0;
    while (profanity_list[iter] != 0) {
        profanity_hash_list[iter] = hash_function(profanity_list[iter]);
        printf("hashed %s as %d\n", profanity_list[iter], profanity_hash_list[iter]);
        iter++;
    }
    printf("DONE ADDING PROFANITY\n\n");
}

// should only give string with length 1023, last byte is for 0
// ONCE FOUND HASHES THAT MATCH CHECK TO MAKE SURE IT ACTUALLY IS PROFANITY
bool contains_profanity(char* string) {
    char bad_msg_buffer[1024] = {0};
    strcpy(bad_msg_buffer, string);
    static char temp[1024] = {0};
    int pointer;
    int quinter;
    //printf("min_size_profanity = %d\n", min_size_profanity);
    //printf("max_size_profanity = %d\n", max_size_profanity);
    for (int i = min_size_profanity; i <= max_size_profanity * 2; i++) {
        //printf("in for loop\n");
        pointer = 0;
        quinter = i;
        while (quinter <= strlen(bad_msg_buffer)) {
            //printf("in while loop\n");
            strcpy(temp, bad_msg_buffer);
            temp[quinter + 1] = 0;
            int hash = hash_function(temp + pointer);
            //printf("hash = %d\n", hash);
            int j = 0;
            while (profanity_hash_list[j] != 0) {
                if (hash == profanity_hash_list[j]) {
                    printf("hash = %d: profanity hash = %d\n", hash, profanity_hash_list[j]);
                    printf("profanity found = %s\n", profanity_list[j]);
                    return true;
                }
                j++;
            }
            pointer++;
            quinter++;
        }
    }
    return false;
}

#if PROFANITY_PRINT == false
#undef printf
#endif

void print_client(struct client_info* client, FILE* fd) {

    fprintf(fd, "address_length = %d\n", client->address_length);
    //printf("raw address = %ld\n", client->address);
    fprintf(fd, "address = %s", get_client_address(client));
    fprintf(fd, "socket = %d\n", client->socket);
    fprintf(fd, "parseFailures = %d\n", client->parseFailures);
}