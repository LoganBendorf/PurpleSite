
#include "send_status.h"
#include "connection_helpers.h"

void send_301(struct client_info* client, struct client_info** clientsPtr) {
    if (client == NULL) {
        return;}
    const char* c301 = "HTTP/1.1 301 Moved Permanently\r\n"
            "Location: https://www.purplesite.skin\r\n\r\n";
    printf("Sent default 301:\n%s\n", c301);
    send(client->socket, c301, strlen(c301), 0);
    drop_client(client, clientsPtr);
    return;
}

#if PRINT_400 == false
#define printf(...)
#endif
void send_400(struct client_info* client, struct client_info** clientsPtr, char* string) {
    if (client == NULL) {
        return;}
    if (string == NULL || strlen(string) == 0) {
        const char* c400 = "HTTP/1.1 400 Bad Request\r\n"
            "Connection: close\r\n"
            "Content-Length: 11\r\n\r\nBad Request";
        printf("Sent default 400:\n%s\n", c400);
        SSL_write(client->ssl, c400, strlen(c400));
        drop_client(client, clientsPtr);
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
    drop_client(client, clientsPtr);
    printf("Sent custom 400:\n%s\n", c400);
}
#if PRINT_400 == false
#undef printf
#endif

void send_404(struct client_info* client, struct client_info** clientsPtr) {
    if (client == NULL) {
        return;}
    const char* c404 = "HTTP/1.1 404 Not Found\r\n"
        "Connection: close\r\n"
        "content-Length: 9\r\n\r\nNot Found";
    SSL_write(client->ssl, c404, strlen(c404));
    drop_client(client, clientsPtr);
}