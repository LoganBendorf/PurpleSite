
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>

#include <unistd.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

SSL *ssl;

char* skip_whitespace(char* pointer) {
    while (*pointer == ' ' || *pointer == '\n' || *pointer == '\t') {
        pointer++;
    }
    return pointer;
}

char* fill_with_garbage(char* buffer, int max_bytes) {
    int amount = (rand() % max_bytes) / sizeof(int);
    for (int i = 0; i < amount; i++) {
        int garbage_int = rand();
        char garbage_buffer[128];
        sprintf(garbage_buffer, "%d", garbage_int);
        strncat(buffer, garbage_buffer, sizeof(int));
    }
}

void run_test(char* filepath) {
    FILE* fp = fopen(filepath, "r");
    if (fp == NULL) {
        printf("File failed to open. [%s]\n", filepath); exit(1);
    }

    fseek(fp, 0L, SEEK_END);
    size_t file_size = ftell(fp);
    rewind(fp);

    char test_data[10240] = {0};
    int bytes_read = fread(test_data, 1, 10240, fp);
    // Might be able to put this on one line. while (fread...);
    while (bytes_read != 0) {
        bytes_read = fread(test_data, 1, 10240, fp);
    }
    if (strlen(test_data) < file_size) {
        printf("Failed to read entire test file. [%s]\n", filepath); exit(1);
    }

    char* pointer = 0;
    char* quinter = 0;

    // Find SSL
    //      - Must be true or false
    char* ssl_id = "ssl =";
    bool ssl_val;
    pointer = strstr(test_data, ssl_id);
    if (pointer == 0) {
        printf("Failed to find ssl. [%s]\n", filepath); exit(1);
    }
    pointer += strlen(ssl_id);
    while (*pointer == ' ') {
        pointer++;
    }
    quinter = strstr(pointer, "\n");
    if (quinter == 0) {
        printf("Failed to find ssl data. [%s]\n", filepath); exit(1);
    }
    if (quinter - pointer < strlen("true")) {
        printf("Bad ssl data. [%s]\n", filepath); exit(1);
    }
    if (strncmp(pointer, "false", 5) == 0) {
        ssl_val = false;
    } else if (strncmp(pointer, "true", 4) == 0) {
        ssl_val = true;
    } else{
        printf("Bad ssl data. [%s]\n", filepath); exit(1);
    }

    printf("ssl = %s\n", ssl_val ? "true" : "false");

    // Find http
    //      - Must be http or https
    char* http_id = "http =";
    char http_val[32] = {0};
    pointer = strstr(quinter, http_id);
    if (pointer == 0) {
        printf("Failed to find http. [%s]\n", filepath); exit(1);
    }
    pointer += strlen(http_id);
    while (*pointer == ' ') {
        pointer++;
    }
    quinter = strstr(pointer, "\n");
    if (quinter == 0 /*|| quinter - pointer <= strlen("http")*/) {
        printf("Failed to find http data. [%s]\n", filepath); exit(1);
    }
    if (strncmp(pointer, "https", 5) == 0) {
        strcpy(http_val, "https");
    } else if (strncmp(pointer, "http", 4) == 0) {
        strcpy(http_val, "http");
    } else {
        printf("Bad http data. [%s]\n", filepath); exit(1);
    }

    printf("http = %s\n", http_val);

    // Find data_blob
    //      - Can be empty
    char* data_blob_id = "data_blob =";
    char data_blob_val[1024] = {0};
    pointer = strstr(quinter, data_blob_id);
    if (pointer == 0) {
        printf("Failed to find data_blob. [%s]\n", filepath); exit(1);
    }
    pointer += strlen(data_blob_id);
    while (*pointer == ' ') {
        pointer++;
    }
    quinter = strstr(pointer, "\n");
    strncpy(data_blob_val, pointer, quinter - pointer);

    printf("data_blob = %s\n", data_blob_val);

    // Find header
    //      - Can be empty
    //      - Can be garbage
    //      - get can have second value, which can be garbage
    char* header_id = "header =";
    char header_val[1024] = {0};
    pointer = strstr(quinter, header_id);
    if (pointer == 0) {
        printf("Failed to find header. [%s]\n", filepath); exit(1);
    }
    pointer += strlen(header_id);
    while (*pointer == ' ') {
        pointer++;
    }
    quinter = strstr(pointer, "\n");
    if (quinter == 0) {
        printf("Failed to find header data. [%s]\n", filepath); exit(1);
    }
    if (strncmp(pointer, "garbage", 7) == 0) {
        fill_with_garbage(header_val, 1024);
    } else if (strncmp(pointer, "post", 4) == 0) {
        strcpy(header_val, "POST / HTTP/1.1\r\n");
    } else if (strncmp(pointer, "get", 3) == 0) {
        pointer += strlen("get");
        if (pointer < quinter) {
            pointer += 1;  
            char file_to_get[1024] = {0};
            strcpy(header_val, "GET /");
            if (strncmp(pointer, "garbage", 7) == 0) {
                fill_with_garbage(file_to_get, 40);
            } else {
                strncpy(file_to_get, pointer, quinter - pointer);
            }
            strcat(header_val, file_to_get);
            strcat(header_val, " HTTP/1.1\r\n");
        } else {
            strcpy(header_val, "GET / HTTP/1.1\r\n");
        }
    } 

    printf("header = %s\n", header_val);

    // Find content_length
    //      - Can be empty
    //      - Can be auto (not now)
    char* content_length_id = "content_length =";
    char content_length_val[1024] = {0};
    pointer = strstr(quinter, content_length_id);
    if (pointer == 0) {
        printf("Failed to find content_length. [%s]\n", filepath); exit(1);
    }
    pointer += strlen(content_length_id);
    while (*pointer == ' ') {
        pointer++;
    }
    quinter = strstr(pointer, "\n");
    if (quinter == 0) {
        printf("Failed to find content_length data. [%s]\n", filepath); exit(1);
    }
    if (strncmp(pointer, "auto", 4) == 0) {
        printf("Auto not implemented yet smile\n"); exit(1);
    } else {
        strcpy(content_length_val, "Content-Length: ");
        char length_numerical_val[128] = {0};
        strncpy(length_numerical_val, pointer, quinter - pointer);
        strcat(content_length_val, length_numerical_val);
        strcat(content_length_val, "\r\n");
    }

    printf("content_length = %s\n", content_length_val);

    // Find content_type
    //      - Can be empty
    //      - Can be garbage
    //      - Can be auto
    char* content_type_id = "content_type =";
    char content_type_val[1024] = {0};
    pointer = strstr(quinter, content_type_id);
    if (pointer == 0) {
        printf("Failed to find content_type. [%s]\n", filepath); exit(1);
    }
    pointer += strlen(content_type_id);
    while (*pointer == ' ') {
        pointer++;
    }
    quinter = strstr(pointer, "\n");
    if (quinter == 0) {
        printf("Failed to find content_type data. [%s]\n", filepath); exit(1);
    }
    if (strncmp(pointer, "garbage", 7) == 0) {
        fill_with_garbage(content_type_val, 1024);
    } else if (strncmp(pointer, "auto", 4) == 0) {
        strcpy(content_type_val, "Content-Type: multipart/form-data; boundary=");
    } else {
        strncpy(content_type_val, pointer, quinter - pointer);
    }

    printf("content_type = %s\n", content_type_val);

    // Find boundary
    //      - Can be empty
    //      - Can be garbage
    //      - Can be auto
    typedef enum {
        CUSTOM, AUTO
    } BOUNDARY_TYPYE;
    char* boundary_id = "boundary =";
    char boundary_val[1024] = {0};
    BOUNDARY_TYPYE boundary_type = CUSTOM;
    pointer = strstr(quinter, boundary_id);
    if (pointer == 0) {
        printf("Failed to find boundary. [%s]\n", filepath); exit(1);
    }
    pointer += strlen(boundary_id);
    while (*pointer == ' ') {
        pointer++;
    }
    quinter = strstr(pointer, "\n");
    if (quinter == 0) {
        printf("Failed to find boundary data. [%s]\n", filepath); exit(1);
    }
    if (strncmp(pointer, "garbage", 7) == 0) {
        fill_with_garbage(boundary_val, 1024);
    } else if (strncmp(pointer, "auto", 4) == 0) {
        strcpy(boundary_val, "------WebKitFormBoundary9NJ1haiCuCNLrSLJ\r\n");
        boundary_type = AUTO;
    } else {
        strncpy(boundary_val, pointer, quinter - pointer);
    }

    printf("boundary = %s\n", boundary_val);

    // Find username
    //      - Can be empty
    //      - Can be garbage
    char* username_id = "username =";
    char username_val[1024] = {0};
    pointer = strstr(quinter, username_id);
    if (pointer == 0) {
        printf("Failed to find username. [%s]\n", filepath); exit(1);
    }
    pointer += strlen(username_id);
    while (*pointer == ' ') {
        pointer++;
    }
    quinter = strstr(pointer, "\n");
    if (quinter == 0) {
        printf("Failed to find username data. [%s]\n", filepath); exit(1);
    }
    if (strncmp(pointer, "garbage", 7) == 0) {
        fill_with_garbage(username_val, 1024);
    } else {
        strcpy(username_val, "Content-Disposition: form-data; name=\"username\"\r\n");
        char username_buffer[1024] = {0};
        strncpy(username_buffer, pointer, quinter - pointer);
        strcat(username_val, username_buffer);
        strcat(username_val, "\r\n");
    }

    printf("username = %s\n", username_val);

    // Find recipient
    //      - Can be empty
    //      - Can be garbage
    char* recipient_id = "recipient =";
    char recipient_val[1024] = {0};
    pointer = strstr(quinter, recipient_id);
    if (pointer == 0) {
        printf("Failed to find recipient. [%s]\n", filepath); exit(1);
    }
    pointer += strlen(recipient_id);
    while (*pointer == ' ') {
        pointer++;
    }
    quinter = strstr(pointer, "\n");
    if (quinter == 0) {
        printf("Failed to find recipient data. [%s]\n", filepath); exit(1);
    }
    if (strncmp(pointer, "garbage", 7) == 0) {
        fill_with_garbage(recipient_val, 1024);
    } else {
        strcpy(recipient_val, "Content-Disposition: form-data; name=\"recipient\"\r\n");
        char recipient_buffer[1024] = {0};
        strncpy(recipient_buffer, pointer, quinter - pointer);
        strcat(recipient_val, recipient_buffer);
        strcat(recipient_val, "\r\n");
    }
    
    printf("recipient = %s\n", recipient_val);

    // Find title
    //      - Can be empty
    //      - Can be garbage
    char* title_id = "title =";
    char title_val[1024] = {0};
    pointer = strstr(quinter, title_id);
    if (pointer == 0) {
        printf("Failed to find title. [%s]\n", filepath); exit(1);
    }
    pointer += strlen(title_id);
    while (*pointer == ' ') {
        pointer++;
    }
    quinter = strstr(pointer, "\n");
    if (quinter == 0) {
        printf("Failed to find title data. [%s]\n", filepath); exit(1);
    }
    if (strncmp(pointer, "garbage", 7) == 0) {
        fill_with_garbage(title_val, 1024);
    } else {
        strcpy(title_val, "Content-Disposition: form-data; name=\"title\"\r\n");
        char title_buffer[1024] = {0};
        strncpy(title_buffer, pointer, quinter - pointer);
        strcat(title_val, title_buffer);
        strcat(title_val, "\r\n");
    }

    printf("title = %s", title_val);

    // Find compose_text
    //      - Can be empty
    //      - Can be garbage
    char* compose_text_id = "compose_text =";
    char compose_text_val[1024] = {0};
    pointer = strstr(quinter, compose_text_id);
    if (pointer == 0) {
        printf("Failed to find compose_text. [%s]\n", filepath); exit(1);
    }
    pointer += strlen(compose_text_id);
    while (*pointer == ' ') {
        pointer++;
    }
    quinter = strstr(pointer, "\n");
    if (quinter == 0) {
        printf("Failed to find compose_text data. [%s]\n", filepath); exit(1);
    }
    if (strncmp(pointer, "garbage", 7) == 0) {
        fill_with_garbage(compose_text_val, 1024);
    } else {
        strcpy(compose_text_val, "Content-Disposition: form-data; name=\"composeText\"\r\n");
        char compose_text_buffer[1024] = {0};
        strncpy(compose_text_buffer, pointer, quinter - pointer);
        strcat(compose_text_val, compose_text_buffer);
        strcat(compose_text_val, "\r\n");
    }

    printf("compose_text = %s\n", compose_text_val);

    char end_val[8] = "--\r\n\r\n";

    // Find format
    char* format_id = "format {";
    pointer = strstr(quinter, format_id);
    if (pointer == 0) {
        printf("Failed to find format. [%s]\n", filepath); exit(1);
    }
    pointer += strlen(format_id);
    char* end_of_format = strstr(pointer, "}");
    if (end_of_format == 0) {
        printf("Failed to find end of format data. [%s]\n", filepath); exit(1);
    }

    pointer = skip_whitespace(pointer);
    if (pointer >= test_data + strlen(test_data) - 1) {
        printf("Failed to find format data. [%s]\n", filepath); exit(1);
    }

    printf("\nPRINTING WRITE BUFFER\n");

    char write_buffer[10240] = {0};

    while (pointer < end_of_format) {

        if (strncmp(pointer, "\\r\\n", 4) == 0) {
            strcat(write_buffer, "\r\n");
            pointer += 5;
            printf("added \\r\\n\n");
        } else if (strncmp(pointer, "header", 6) == 0) {
            strcat(write_buffer, header_val);
            pointer += 7;
        } else if (strncmp(pointer, "content_length", 14) == 0) {
            strcat(write_buffer, content_length_val);
            pointer += 15;
        } else if (strncmp(pointer, "content_type", 12) == 0) {
            strcat(write_buffer, content_type_val); 
            strcat(write_buffer, boundary_val + 2);
            pointer += 13;
        } else if (strncmp(pointer, "boundary", 8) == 0) {
            strcat(write_buffer, boundary_val); 
            pointer += 9;
        } else if (strncmp(pointer, "username", 8) == 0) {
            strcat(write_buffer, username_val); 
            pointer += 9;
        } else if (strncmp(pointer, "recipient", 9) == 0) {
            strcat(write_buffer, recipient_val); 
            pointer += 10;
        } else if (strncmp(pointer, "title", 5) == 0) {
            strcat(write_buffer, title_val);
            pointer += 6;
        } else if (strncmp(pointer, "compose_text", 12) == 0) {
            strcat(write_buffer, compose_text_val); 
            pointer += 13;
        } else if (strncmp(pointer, "end", 3) == 0) {
            strncpy(write_buffer + strlen(write_buffer) - 2, end_val, strlen(end_val));
            pointer += 10000;
        } else if (strncmp(pointer, "data_blob", 9) == 0) {
            strcat(write_buffer, data_blob_val);
            pointer += 10;
        } else if (strncmp(pointer, "delay", 5) == 0) {
            pointer += 6;
            char delay_as_char[128] = {0};
            char* end_of_int = strstr(pointer, "\n");
            strncpy(delay_as_char, pointer, end_of_int - pointer);
            int delay = 0;
            delay = atoi(delay_as_char);

            // for now cause we're not sending anything
            strcat(write_buffer, "!!!!! delay ");
            strcat(write_buffer, delay_as_char);
            strcat(write_buffer, " !!!!!\n");

            pointer = end_of_int;
        } else {
            printf("unknown value in format data\n"); exit(1);
        }

        pointer = skip_whitespace(pointer);
    }

    printf("%s", write_buffer);
    SSL_write(ssl, write_buffer, strlen(write_buffer));
    // Receive a message from the server
    char buffer[1024] = {0};
    int bytes = SSL_read(ssl, buffer, sizeof(buffer));
    if (bytes > 0) {
        buffer[bytes] = 0;
        printf("Received: \"%s\"\n", buffer);
    } else {
        ERR_print_errors_fp(stderr);
    }

}


#define SERVER "127.0.0.1"
#define PORT 443

int server_fd;
SSL_CTX *ctx;

void ssl_connect() {
    // Create a new SSL context
    const SSL_METHOD *method = SSLv23_client_method();
    ctx = SSL_CTX_new(method);
    if (!ctx) {
        ERR_print_errors_fp(stderr);
        exit(1);
    }
     // Create a new SSL connection state
    ssl = SSL_new(ctx);
    if (!ssl) {
        ERR_print_errors_fp(stderr);
        SSL_CTX_free(ctx);
        exit(EXIT_FAILURE);
    }
    // Create a socket and connect to the server
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Unable to create socket");
        SSL_free(ssl);
        SSL_CTX_free(ctx);
        exit(1);
    }
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, SERVER, &addr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        close(server_fd);
        SSL_free(ssl);
        SSL_CTX_free(ctx);
        exit(1);
    }
    if (connect(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Connection failed");
        close(server_fd);
        SSL_free(ssl);
        SSL_CTX_free(ctx);
        exit(1);
    }
    // Associate the socket with the SSL connection
    SSL_set_fd(ssl, server_fd);
    // Perform the SSL/TLS handshake with the server
    if (SSL_connect(ssl) <= 0) {
        ERR_print_errors_fp(stderr);
        exit(1);
    } else {
        printf("Connected to server with %s encryption\n", SSL_get_cipher(ssl));
    }
}

void ssl_disconnect() {
    close(server_fd);
    SSL_shutdown(ssl);
    SSL_free(ssl);
}

int main() {

    // INIT SSL
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();

    ssl_connect();

    // Init random
    srand(time(NULL));

    // Run Tests
    run_test("test3");
    //ssl_disconnect();
    //ssl_connect();
    //run_test("test2");
    

    // Clean up
    SSL_shutdown(ssl);
    SSL_free(ssl);
    close(server_fd);
    SSL_CTX_free(ctx);
    EVP_cleanup();

}