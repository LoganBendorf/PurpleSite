
#include "send_status.h"
#include "connection_helpers.h"

// For serving resources, don't drop client
void send_200(struct client_info* client, unsigned long content_length, const char* content_type) {
    char buffer[1024] = {0};
    sprintf(buffer, "HTTP/1.1 200 OK\r\n");
    SSL_write(client->ssl, buffer, strlen(buffer));

    sprintf(buffer, "Connection: close\r\n");
    SSL_write(client->ssl, buffer, strlen(buffer));

    sprintf(buffer, "Content-Length: %lu\r\n", content_length);
    SSL_write(client->ssl, buffer, strlen(buffer));

    sprintf(buffer, "Content-Type: %s\r\n", content_type);
    SSL_write(client->ssl, buffer, strlen(buffer));

    sprintf(buffer, "\r\n");
    SSL_write(client->ssl, buffer, strlen(buffer));
}

void send_201(struct client_info* client, struct client_info** clients_ptr) {
    if (client == NULL) {
        return;}
    const char* c201 = "HTTP/1.1 201 Created\r\n"
                       "Access-Control-Expose-Headers: Location\r\n"
                       "Location: /emails.txt\r\n"
                       "\r\n";
    SSL_write(client->ssl, c201, strlen(c201));
    drop_client(client, clients_ptr);
}

void send_301(struct client_info* client, struct client_info** clients_ptr) {
    if (client == NULL) {
        return;}
    const char* c301 = "HTTP/1.1 301 Moved Permanently\r\n"
                       "Location: https://www.purplesite.skin\r\n\r\n";
    printf("Sent default 301:\n%s\n", c301);
    // Send is intentional, redirecting from http to https
    send(client->socket, c301, strlen(c301), 0);
    drop_client(client, clients_ptr);
}

#if PRINT_400 == false
#define printf(...)
#endif
// Bad request, msg buffer should be 256 bytes
void send_400(struct client_info* client, struct client_info** clients_ptr, char* msg) {
    if (client == NULL) {
        return;}
    if (msg == NULL || strlen(msg) == 0) {
        const char* c400 = "HTTP/1.1 400 Bad Request\r\n"
                           "Connection: close\r\n"
                           "content-Length: 0\r\n\r\n";
        printf("Sent default 400:\n%s\n", c400);
        SSL_write(client->ssl, c400, strlen(c400));
        drop_client(client, clients_ptr);
        return;
    }
    const char* beg = "HTTP/1.1 400 ";
    const char* end = "Connection: close\r\n"
                      "Content-Length: 0\r\n\r\n";
    if (strlen(msg) + strlen(beg) + strlen(end) > 253) {
        printf("error msg was to large, truncated\n");
        msg[253 - strlen(end) - strlen(end)] = 0;}
    char c400[256] = {0};
    strcpy(c400, beg);

    char* pointer = c400 + strlen(beg);
    strcat(c400, msg);
    strcat(c400, "\r\n");

    strcat(c400, end);

    SSL_write(client->ssl, c400, 256);
    drop_client(client, clients_ptr);
    printf("Sent custom 400:\n%s\n", c400);
}
#if PRINT_400 == false
#undef printf
#endif

void send_404(struct client_info* client, struct client_info** clients_ptr) {
    if (client == NULL) {
        return;}
    const char* c404 = "HTTP/1.1 404 Not Found\r\n"
                       "Connection: close\r\n"
                       "content-Length: 9\r\n\r\nNot Found";
    SSL_write(client->ssl, c404, strlen(c404));
    drop_client(client, clients_ptr);
}

// Big error, msg buffer size should be 256
void send_500(struct client_info* client, struct client_info** clients_ptr, char* msg) {
    if (client == NULL) {
        return;}
    if (msg == NULL || strlen(msg) == 0) {
        const char* c500 = "HTTP/1.1 507 Insufficient Storage\r\n"
                           "Connection: close\r\n"
                           "content-Length: 0\r\n\r\n";
        printf("Sent default 500:\n%s\n", c500);
        SSL_write(client->ssl, c500, strlen(c500));
        drop_client(client, clients_ptr);
        return;
    }
    const char* beg = "HTTP/1.1 500 ";
    const char* end = "Connection: close\r\n"
                      "Content-Length: 0\r\n\r\n";
    if (strlen(msg) + strlen(beg) + strlen(end) > 253) {
        printf("error msg was to large, truncated\n");
        msg[253 - strlen(end) - strlen(end)] = 0;}
    char c500[256] = {0};
    strcpy(c500, beg);

    char* pointer = c500 + strlen(beg);
    strcat(c500, msg);
    strcat(c500, "\r\n");

    strcat(c500, end);

    SSL_write(client->ssl, c500, 256);
    drop_client(client, clients_ptr);
    printf("Sent custom 500:\n%s\n", c500);
}

// Out of memory
void send_507(struct client_info* client, struct client_info** clients_ptr) {
    if (client == NULL) {
        return;}
    const char* c507 = "HTTP/1.1 507 Insufficient Storage\r\n"
                       "Connection: close\r\n"
                       "content-Length: 0\r\n\r\n";
    SSL_write(client->ssl, c507, strlen(c507));
    drop_client(client, clients_ptr);
}