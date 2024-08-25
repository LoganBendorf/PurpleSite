
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>

#include <unistd.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include <sys/time.h>


#define SERVER "127.0.0.1"
#define PORT 443

int server_fd;
SSL_CTX *ctx;
SSL *ssl;

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

    struct timeval tv;
    tv.tv_sec = 5; // 5 seconds
    tv.tv_usec = 0;
    setsockopt(server_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
        
    // bind socket to ssl
    SSL_set_fd(ssl, server_fd);

    // ssl handshake
    if (SSL_connect(ssl) <= 0) {
        ERR_print_errors_fp(stderr);
        exit(1);
    } else {
        printf("Connected to server with %s encryption\n", SSL_get_cipher(ssl));
    }
}

void normal_connect() {
    // Create a socket and connect to the server
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Unable to create socket");
        exit(1);
    }
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, SERVER, &addr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        close(server_fd);
        exit(1);
    }
    if (connect(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Connection failed");
        close(server_fd);
        exit(1);
    }

    struct timeval tv;
    tv.tv_sec = 5; // 5 seconds
    tv.tv_usec = 0;
    setsockopt(server_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
}

void ssl_disconnect() {
    close(server_fd);
    SSL_shutdown(ssl);
    SSL_free(ssl);
}

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

char* find_id(char* id, char* pointer, char* filepath) {

    pointer = strstr(pointer, id);
    if (pointer == 0) {
        printf("Failed to find id (%s). [%s]\n", id, filepath); exit(1);
    }
    pointer += strlen(id);
    while (*pointer == ' ') {
        pointer++;
    }
    char* quinter = strstr(pointer, "\n");
    if (quinter == 0) {
        printf("Failed to find ssl data. [%s]\n", filepath); exit(1);
    }
    if (quinter - pointer < 0) {
        printf("Bad id (%s) data. [%s]\n", id, filepath); exit(1);
    }
    return pointer;
}

void run_test(char* filepath) {

    printf("\n    |Test: %s|\n", filepath);
    printf("    |Locating data|\n");

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

    char* pointer = test_data;
    char* quinter = 0;
    // Find SSL
    //      - Must be true or false
    //      - Can random
    typedef enum  {
        ssl_branch_true, ssl_branch_false, ssl_branch_ENUM_END
    } SSL_branch;
    SSL_branch ssl_branch = -1;

    char* ssl_id = "ssl =";
    bool ssl_val;

    pointer = find_id(ssl_id, pointer, filepath);

    if (strncmp(pointer, "random", 6) == 0) {
        ssl_branch = rand() % ssl_branch_ENUM_END;
    } else if (strncmp(pointer, "false", 5) == 0) {
        ssl_branch = ssl_branch_false;
    } else if (strncmp(pointer, "true", 4) == 0) {
        ssl_branch = ssl_branch_true;
    } else{
        printf("Bad ssl data. [%s]\n", filepath); exit(1);
    }

    switch (ssl_branch) {
        case ssl_branch_true: ssl_val = true; break;
        case ssl_branch_false: ssl_val = false; break;
        default: printf("Bad ssl data. [%s]\n", filepath); exit(1);
    }

    printf("ssl = %s\n", ssl_val ? "true" : "false");

    // Find http
    //      - Must be http or https
    //      - Can be random
    typedef enum  {
        http_branch_http, http_branch_https, http_branch_ENUM_END
    } HTTP_branch;
    HTTP_branch http_branch = -1;

    char* http_id = "http =";
    char http_val[32] = {0};

    pointer = find_id(http_id, pointer, filepath);

    if (strncmp(pointer, "random", 6) == 0) {
        http_branch = rand() % http_branch_ENUM_END;
    } else if (strncmp(pointer, "https", 5) == 0) {
        http_branch = http_branch_https;
    } else if (strncmp(pointer, "http", 4) == 0) {
        http_branch = http_branch_http;
    } else {
        printf("Bad http data. [%s]\n", filepath); exit(1);
    }

    switch (http_branch) {
        case http_branch_http: strcpy(http_val, "http"); break;
        case http_branch_https: strcpy(http_val, "https"); break;
        default: printf("Bad http data. [%s]\n", filepath); exit(1);
    }

    printf("http = %s\n", http_val);

    // Find data_blob
    //      - Can be empty
    char* data_blob_id = "data_blob =";
    char data_blob_val[1024] = {0};

    pointer = strstr(pointer, data_blob_id);
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
    //      - "get" can have second value, which can be "garbage"
    //      - Can be random
    typedef enum  {
        header_branch_empty, header_branch_garbage, header_branch_post, header_branch_get, header_branch_ENUM_END
    } HEADER_branch;
    HEADER_branch header_branch = -1;

    char* header_id = "header =";
    char header_val[1024] = {0};

    pointer = find_id(header_id, pointer, filepath);

    if (strncmp(pointer, "random", 6) == 0) {
        header_branch = rand() % header_branch_ENUM_END;
    } else if (strncmp(pointer, "garbage", 7) == 0) {
        header_branch = header_branch_garbage;
    } else if (strncmp(pointer, "post", 4) == 0) {
        header_branch = header_branch_post;
    } else if (strncmp(pointer, "get", 3) == 0) {
        header_branch = header_branch_get;
    } 

    switch (header_branch) {
        case header_branch_garbage: fill_with_garbage(header_val, 1024); break;
        case header_branch_post: strcpy(header_val, "POST / HTTP/1.1\r\n"); break;
        case header_branch_get: 
            // Have to calculate quinter first or it might skip to the next line
            quinter = strstr(pointer, "\n");
            while ( !(*pointer == ' ' || *pointer == '\n') ) {
                pointer++;
            }
            while (*pointer == ' ') {
                pointer++;
            }
            if (pointer < quinter) {
                char file_to_get[1024] = {0};
                strcpy(header_val, "GET /");
                if (strncmp(pointer, "garbage", 7) == 0) {
                    // Can't fill with too much garbage or overflow when strcatting
                    fill_with_garbage(file_to_get, 900);
                } else {
                    strncpy(file_to_get, pointer, quinter - pointer);
                }
                strcat(header_val, file_to_get);
                strcat(header_val, " HTTP/1.1\r\n");
            } else {
                strcpy(header_val, "GET / HTTP/1.1\r\n");
            }
        default: break;
    }

    printf("header = %s\n", header_val);

    // Find content_length
    //      - Can be empty
    //      - Can be auto (not now)
    char* content_length_id = "content_length =";
    char content_length_val[1024] = {0};

    pointer = find_id(content_length_id, pointer, filepath);
    
    if (strncmp(pointer, "auto", 4) == 0) {
        printf("Auto not implemented yet smile\n"); exit(1);
    } else {
        strcpy(content_length_val, "Content-Length: ");
        char length_numerical_val[128] = {0};
        quinter = strstr(pointer, "\n");
        strncpy(length_numerical_val, pointer, quinter - pointer);
        strcat(content_length_val, length_numerical_val);
        strcat(content_length_val, "\r\n");
    }

    printf("content_length = %s\n", content_length_val);

    // Find content_type
    //      - Can be empty
    //      - Can be garbage
    //      - Can be auto
    //      - Can be random
    typedef enum  {
        content_type_branch_empty, content_type_branch_garbage, content_type_branch_auto, content_type_branch_ENUM_END
    } CONTENT_TYPE_branch;
    CONTENT_TYPE_branch content_type_branch = -1;
    
    char* content_type_id = "content_type =";
    char content_type_val[1024] = {0};

    pointer = find_id(content_type_id, pointer, filepath);

    if (strncmp(pointer, "random", 6) == 0) {
        content_type_branch = rand() % content_type_branch_ENUM_END;
    } else if (strncmp(pointer, "garbage", 7) == 0) {
        content_type_branch = content_type_branch_garbage;
    } else if (strncmp(pointer, "auto", 4) == 0) {
        content_type_branch = content_type_branch_auto;
    }

    switch (content_type_branch) {
        case content_type_branch_empty: break;
        case content_type_branch_garbage: fill_with_garbage(content_type_val, 1024); break;
        case content_type_branch_auto: strcpy(content_type_val, "Content-Type: multipart/form-data; boundary="); break;
        default: 
            quinter = strstr(pointer, "\n");
            strncpy(content_type_val, pointer, quinter - pointer);
    }

    printf("content_type = %s\n", content_type_val);

    // Find boundary
    //      - Can be empty
    //      - Can be garbage
    //      - Can be auto
    //      - Can be random
    typedef enum  {
        boundary_branch_empty, boundary_branch_garbage, boundary_branch_auto, boundary_branch_ENUM_END
    } BOUNDARY_branch;
    BOUNDARY_branch boundary_branch = -1;

    typedef enum {
        CUSTOM, AUTO
    } BOUNDARY_TYPYE;
    char* boundary_id = "boundary =";
    char boundary_val[1024] = {0};
    BOUNDARY_TYPYE boundary_type = CUSTOM;

    pointer = find_id(boundary_id, pointer, filepath);

    if (strncmp(pointer, "random", 6) == 0) {
        boundary_branch = rand() % boundary_branch_ENUM_END;
    } else if (strncmp(pointer, "garbage", 7) == 0) {
        boundary_branch = boundary_branch_garbage;
    } else if (strncmp(pointer, "auto", 4) == 0) {
        boundary_branch = boundary_branch_auto;
    }

    switch (boundary_branch) {
        case boundary_branch_empty: break;
        case boundary_branch_garbage: fill_with_garbage(boundary_val, 1024); break;
        case boundary_branch_auto: 
            strcpy(boundary_val, "------WebKitFormBoundary9NJ1haiCuCNLrSLJ\r\n");
            boundary_type = AUTO;
            break;
        default: 
            quinter = strstr(pointer, "\n");
            strncpy(boundary_val, pointer, quinter - pointer);
    }

    printf("boundary = %s\n", boundary_val);

    // Find username
    //      - Can be empty
    //      - Can be garbage
    char* username_id = "username =";
    char username_val[1024] = {0};

    pointer = find_id(username_id, pointer, filepath);

    if (strncmp(pointer, "garbage", 7) == 0) {
        fill_with_garbage(username_val, 1024);
    } else {
        strcpy(username_val, "Content-Disposition: form-data; name=\"username\"\r\n");
        char username_buffer[1024] = {0};
        quinter = strstr(pointer, "\n");
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

    pointer = find_id(recipient_id, pointer, filepath);

    if (strncmp(pointer, "garbage", 7) == 0) {
        fill_with_garbage(recipient_val, 1024);
    } else {
        strcpy(recipient_val, "Content-Disposition: form-data; name=\"recipient\"\r\n");
        char recipient_buffer[1024] = {0};
        quinter = strstr(pointer, "\n");
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

    pointer = find_id(title_id, pointer, filepath);

    if (strncmp(pointer, "garbage", 7) == 0) {
        fill_with_garbage(title_val, 1024);
    } else {
        strcpy(title_val, "Content-Disposition: form-data; name=\"title\"\r\n");
        char title_buffer[1024] = {0};
        quinter = strstr(pointer, "\n");
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

    pointer = find_id(compose_text_id, pointer, filepath);

    if (strncmp(pointer, "garbage", 7) == 0) {
        fill_with_garbage(compose_text_val, 1024);
    } else {
        strcpy(compose_text_val, "Content-Disposition: form-data; name=\"composeText\"\r\n");
        char compose_text_buffer[1024] = {0};
        quinter = strstr(pointer, "\n");
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

    printf("    |Formating data|\n");

    typedef enum  {
        format_option_bsr_bsn,
        format_option_header, 
        format_option_content_length,
        format_option_content_type,
        format_option_boundary,
        format_option_username,
        format_option_recipient,
        format_option_title,
        format_option_compose_text,
        format_option_end,
        format_option_data_blob,
        format_option_delay,
        format_option_ENUM_END
    } FORMAT_options;
    FORMAT_options format_option = -1;

    int line = 1;
    char* start = pointer;
    char write_buffer[10240] = {0};

    while (pointer < end_of_format) {

        if (strncmp(pointer, "random", 6) == 0) {
            format_option = rand() % format_option_ENUM_END;
        } else if (strncmp(pointer, "\\r\\n", 4) == 0) {
            format_option = format_option_bsr_bsn;
        } else if (strncmp(pointer, "header", 6) == 0) {
            format_option = format_option_header;            
        } else if (strncmp(pointer, "content_length", 14) == 0) {
            format_option = format_option_content_length; 
        } else if (strncmp(pointer, "content_type", 12) == 0) {
            format_option = format_option_content_type; 
        } else if (strncmp(pointer, "boundary", 8) == 0) {
            format_option = format_option_boundary; 
        } else if (strncmp(pointer, "username", 8) == 0) {
            format_option = format_option_username; 
        } else if (strncmp(pointer, "recipient", 9) == 0) {
            format_option = format_option_recipient; 
        } else if (strncmp(pointer, "title", 5) == 0) {
            format_option = format_option_title; 
        } else if (strncmp(pointer, "compose_text", 12) == 0) {
            format_option = format_option_compose_text; 
        } else if (strncmp(pointer, "end", 3) == 0) {
            format_option = format_option_end; 
        } else if (strncmp(pointer, "data_blob", 9) == 0) {
            format_option = format_option_data_blob; 
        } else if (strncmp(pointer, "delay", 5) == 0) {
            format_option = format_option_delay; 
        }

        printf("option = %d\n", format_option);

        switch (format_option) {
            case format_option_bsr_bsn:
                strcat(write_buffer, "\r\n");
                pointer += 4;
                break;
            case format_option_header:
                strcat(write_buffer, header_val);
                pointer += 6;
                break;
            case format_option_content_length:
                strcat(write_buffer, content_length_val);
                pointer += 14;
                break;
            case format_option_content_type:
                strcat(write_buffer, content_type_val); 
                strcat(write_buffer, boundary_val + 2);
                pointer += 12;
                break;
            case format_option_boundary:
                strcat(write_buffer, boundary_val); 
                pointer += 8;
                break;
            case format_option_username:
                strcat(write_buffer, username_val); 
                pointer += 8;
                break;
            case format_option_recipient:
                strcat(write_buffer, recipient_val); 
                pointer += 9;
                break;
            case format_option_title:
                strcat(write_buffer, title_val);
                pointer += 5;
                break;
            case format_option_compose_text:
                strcat(write_buffer, compose_text_val); 
                pointer += 12;
                break;
            case format_option_end:
                strncpy(write_buffer + strlen(write_buffer) - 2, end_val, strlen(end_val));
                pointer += 100000;
                break;
            case format_option_data_blob:
                strcat(write_buffer, data_blob_val);
                pointer += 9;
                break;
            case format_option_delay:
                pointer += 5;
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
                
                break;
            default: 
                printf("Unknown value in format data. Line %d. Column %ld.\nValue = \"", line, pointer - start + 1);
                while ( pointer < end_of_format && !(*pointer == ' ' || *pointer == '\n' || *pointer == '\t') ) {
                    printf("%c", *pointer);
                    pointer++;
                }
                printf("\"\n");
                exit(1);
        }

        while ( (pointer < end_of_format) && (*pointer == ' ' || *pointer == '\n' || *pointer == '\t') ) {
            if (*pointer == '\n') {
                line++;
                start = pointer;
            }
            pointer++;
        }
    }

    printf("\n    |Printing write buffer|\n");
    printf("%s", write_buffer);
    if (ssl_val == true) {
        ssl_connect();
    } else {
        normal_connect();
    }
    if (strncmp(http_val, "https", 5) == 0) {
        SSL_write(ssl, write_buffer, strlen(write_buffer));
    } else {
        write(server_fd, write_buffer, strlen(write_buffer));
    }

    // Receive a message from the server
    char buffer[10240] = {0};
    int bytes = 0;
    if (ssl_val == true) {
        bytes = SSL_read(ssl, buffer, 10240);
    } else {
        bytes = read(server_fd, buffer, 10240);
    }
    if (bytes > 0) {
        buffer[bytes] = 0;
        printf("Received: \"%s\"\n", buffer);
    } else {
        ERR_print_errors_fp(stderr); exit(1);
    }

    if (ssl_val == true) {
        ssl_disconnect();
    } else {
        close(server_fd);
    }
}

int main() {

    // INIT SSL
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();

    // Init random
    srand(time(NULL));


    // Run Tests
    //run_test("post");
    //run_test("random_format");
    run_test("full_random\n");


    // Clean up
    SSL_CTX_free(ctx);
    EVP_cleanup();
}