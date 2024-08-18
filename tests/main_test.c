
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

char* fill_with_garbage(char* buffer, int max_bytes) {
    int ammount = (rand() % max_bytes) / sizeof(int);
    for (int i = 0; i < ammount; i++) {
        int garbage_int = rand();
        char garbage_buffer[sizeof(int)];
        sprintf(garbage_buffer, "%d", garbage_int);
        strcat(buffer, garbage_buffer);
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
    char* ssl_id = "ssl = ";
    bool ssl_val;
    pointer = strstr(test_data, ssl_id);
    if (pointer == 0) {
        printf("Failed to find ssl. [%s]\n", filepath); exit(1);
    }
    pointer += strlen(ssl_id);
    quinter = strstr(pointer, "\n");
    if (quinter == 0 /*|| quinter - pointer <= strlen("true")*/) { // Might have to change the <= to just <
        printf("Failed to find ssl data. [%s]\n", filepath); exit(1);
    }
    if (strncmp(pointer, "false", 5) == 0) {
        ssl_val = false;
    } else if (strncmp(pointer, "true", 4) == 0) {
        ssl_val = true;
    } else{
        printf("Bad ssl data. [%s]\n", filepath); exit(1);
    }

    // Find http
    //      - Must be http or https
    char* http_id = "http = ";
    char http_val[32] = {0};
    pointer = strstr(quinter, http_id);
    if (pointer == 0) {
        printf("Failed to find http. [%s]\n", filepath); exit(1);
    }
    pointer += strlen(http_id);
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

    // Find data_blob
    //      - Can be empty
    char* data_blob_id = "data_blob = ";
    char data_blob_val[1024] = {0};
    pointer = strstr(quinter, data_blob_id);
    if (pointer == 0) {
        printf("Failed to find data_blob. [%s]\n", filepath); exit(1);
    }
    pointer += strlen(data_blob_id);
    quinter = strstr(pointer, "\n");
    strncpy(data_blob_val, pointer, quinter - pointer + 1);

    // Find header
    //      - Can be empty
    //      - Can be garbage
    //      - get can have second value, which can be garbage
    char* header_id = "header = ";
    char header_val[1024] = {0};
    pointer = strstr(quinter, header_id);
    if (pointer == 0) {
        printf("Failed to find header. [%s]\n", filepath); exit(1);
    }
    pointer += strlen(header_id);
    quinter = strstr(pointer, "\n");
    if (quinter == 0 /*|| quinter - pointer <= strlen("get")*/) {
        printf("Failed to find header data. [%s]\n", filepath); exit(1);
    }
    if (strncmp(pointer, "garbage", 7) == 0) {
        fill_with_garbage(header_val, 1024);
    } else if (strncmp(pointer, "post", 4) == 0) {
        strcpy(header_val, "POST / HTTP/1.1\r\n");
    } else if (strncmp(pointer, "get", 3) == 0) {
        pointer += strlen("get");
        pointer += 1;
        char file_to_get[1024];
        if (strncmp(pointer, "garbage", 7)) {
            fill_with_garbage(file_to_get, 1024);
        } else {
            strncpy(file_to_get, pointer, quinter-pointer+1);
            strcat(header_val, "GET /");
            strcat(header_val, file_to_get);
        }
        strcat(header_val, " HTTP/1.1\r\n");
    } 

    // Find content_length
    //      - Can be empty
    //      - Can be auto (not now)
    char* content_length_id = "content_length = ";
    char content_length_val[1024] = {0};
    pointer = strstr(quinter, content_length_id);
    if (pointer == 0) {
        printf("Failed to find content_length. [%s]\n", filepath); exit(1);
    }
    pointer += strlen(content_length_id);
    quinter = strstr(pointer, "\n");
    if (quinter == 0) {
        printf("Failed to find content_length data. [%s]\n", filepath); exit(1);
    }
    if (strncmp(pointer, "auto", 4) == 0) {
        printf("Auto not implemented yet smile\n"); exit(1);
        strcpy(content_length_val, "auto");
    } else {
        strncpy(content_length_val, pointer, quinter - pointer + 1);
    }

    // Find content_type
    //      - Can be empty
    //      - Can be garbage
    //      - Can be auto
    char* content_type_id = "content_type = ";
    char content_type_val[1024] = {0};
    pointer = strstr(quinter, content_type_id);
    if (pointer == 0) {
        printf("Failed to find content_type. [%s]\n", filepath); exit(1);
    }
    pointer += strlen(content_type_id);
    quinter = strstr(pointer, "\n");
    if (quinter == 0) {
        printf("Failed to find content_type data. [%s]\n", filepath); exit(1);
    }
    if (strncmp(pointer, "garbage", 7) == 0) {
        fill_with_garbage(content_type_val, 1024);
    } else if (strncmp(pointer, "auto", 4) == 0) {
        strcpy(content_type_val, "Content-Type: multipart/form-data; ");
    } else {
        strncpy(content_type_val, pointer, quinter - pointer + 1);
    }

    // Find boundary
    //      - Can be empty
    //      - Can be garbage
    //      - Can be auto
    char* boundary_id = "boundary = ";
    char boundary_val[1024] = {0};
    pointer = strstr(quinter, boundary_id);
    if (pointer == 0) {
        printf("Failed to find boundary. [%s]\n", filepath); exit(1);
    }
    pointer += strlen(boundary_id);
    quinter = strstr(pointer, "\n");
    if (quinter == 0) {
        printf("Failed to find boundary data. [%s]\n", filepath); exit(1);
    }
    if (strncmp(pointer, "garbage", 7) == 0) {
        fill_with_garbage(boundary_val, 1024);
    } else if (strncmp(pointer, "auto", 4) == 0) {
        strcpy(boundary_val, "boundary=----WebKitFormBoundary9NJ1haiCuCNLrSLJ\r\n");
    } else {
        strncpy(boundary_val, pointer, quinter - pointer + 1);
    }

    // Find username
    //      - Can be empty
    //      - Can be garbage
    char* username_id = "username = ";
    char username_val[1024] = {0};
    pointer = strstr(quinter, username_id);
    if (pointer == 0) {
        printf("Failed to find username. [%s]\n", filepath); exit(1);
    }
    pointer += strlen(username_id);
    quinter = strstr(pointer, "\n");
    if (quinter == 0) {
        printf("Failed to find username data. [%s]\n", filepath); exit(1);
    }
    if (strncmp(pointer, "garbage", 7) == 0) {
        fill_with_garbage(username_val, 1024);
    } else {
        strcpy(username_val, "Content-Disposition: form-data; name=\"username\"\r\n");
        char username_buffer[1024] = {0};
        strncpy(username_buffer, pointer, quinter - pointer + 1);
        strcat(username_val, username_buffer);
    }

    // Find recipient
    //      - Can be empty
    //      - Can be garbage
    char* recipient_id = "recipient = ";
    char recipient_val[1024] = {0};
    pointer = strstr(quinter, recipient_id);
    if (pointer == 0) {
        printf("Failed to find recipient. [%s]\n", filepath); exit(1);
    }
    pointer += strlen(recipient_id);
    quinter = strstr(pointer, "\n");
    if (quinter == 0) {
        printf("Failed to find recipient data. [%s]\n", filepath); exit(1);
    }
    if (strncmp(pointer, "garbage", 7) == 0) {
        fill_with_garbage(recipient_val, 1024);
    } else {
        strcpy(recipient_val, "Content-Disposition: form-data; name=\"recipient\"\r\n");
        char recipient_buffer[1024] = {0};
        strncpy(recipient_buffer, pointer, quinter - pointer + 1);
        strcat(recipient_val, recipient_buffer);
    }

    // Find title
    //      - Can be empty
    //      - Can be garbage
    char* title_id = "title = ";
    char title_val[1024] = {0};
    pointer = strstr(quinter, title_id);
    if (pointer == 0) {
        printf("Failed to find title. [%s]\n", filepath); exit(1);
    }
    pointer += strlen(title_id);
    quinter = strstr(pointer, "\n");
    if (quinter == 0) {
        printf("Failed to find title data. [%s]\n", filepath); exit(1);
    }
    if (strncmp(pointer, "garbage", 7) == 0) {
        fill_with_garbage(title_val, 1024);
    } else {
        strcpy(title_val, "Content-Disposition: form-data; name=\"title\"\r\n");
        char title_buffer[1024] = {0};
        strncpy(title_buffer, pointer, quinter - pointer + 1);
        strcat(title_val, title_buffer);
    }

    // Find compose_text
    //      - Can be empty
    //      - Can be garbage
    char* compose_text_id = "compose_text = ";
    char compose_text_val[1024] = {0};
    pointer = strstr(quinter, compose_text_id);
    if (pointer == 0) {
        printf("Failed to find compose_text. [%s]\n", filepath); exit(1);
    }
    pointer += strlen(compose_text_id);
    quinter = strstr(pointer, "\n");
    if (quinter == 0) {
        printf("Failed to find compose_text data. [%s]\n", filepath); exit(1);
    }
    if (strncmp(pointer, "garbage", 7) == 0) {
        fill_with_garbage(compose_text_val, 1024);
    } else {
        strcpy(compose_text_val, "Content-Disposition: form-data; name=\"composeText\"\r\n");
        char compose_text_buffer[1024] = {0};
        strncpy(compose_text_buffer, pointer, quinter - pointer + 1);
        strcat(compose_text_val, compose_text_buffer);
    }

    char end[8] = "--\r\n\r\n";

    // Find format
    char* format_id = "format {";
    pointer = strstr(quinter, format_id);
    if (pointer == 0) {
        printf("Failed to find format. [%s]\n", filepath); exit(1);
    }
    pointer += strlen(format_id);
    char* end_of_format = strstr(pointer, "}");
    if (end_of_format == 0) {
        printf("Failed to find format data. [%s]\n", filepath); exit(1);
    }

    quinter = strstr(pointer, "\t");
    if (quinter == 0) {
        printf("Failed to find format data. [%s]\n", filepath); exit(1);
    }

    char write_buffer[1024] = {0};

    while (quinter != end_of_format) {
        pointer = quinter + 1;
        quinter = strstr(pointer, "\n");

        if (strncmp(pointer, "header", 6)) {
            strcat(write_buffer, header_val);
        } else if (strncmp(pointer, "content_length", 14)) {
            strcat(write_buffer, content_length_val);
        } else if (strncmp(pointer, "content_type", 12)) {
            strcat(write_buffer, content_type_val); //////////
        } else if (strncmp(pointer, "boundary", 8)) {
            strcat(write_buffer, boundary_val); //////
        } else if (strncmp(pointer, "username", 8)) {
            strcat(write_buffer, username_val); /////
        } else if (strncmp(pointer, "recipient", 9)) {
            strcat(write_buffer, recipient_val); /////////
        } else if (strncmp(pointer, "title", 5)) {
            strcat(write_buffer, title_val); ////
        } else if (strncmp(pointer, "compose_text", 12)) {
            strcat(write_buffer, compose_text_val); ///
        } else if (strncmp(pointer, "end", 3)) {
            strcat(write_buffer, end_of_format);
        } else if (strncmp(pointer, "data_blob", 9)) {
            strcat(write_buffer, data_blob_val);
        } else if (strncmp(pointer, "delay", 5)) {
            pointer += 5;
            char delay_as_char[32] = {0};
            strncmp(delay_as_char, pointer, pointer - quinter + 1);
            int delay = 0;
            delay = atoi(delay_as_char);

            // for now cause we're not sending anything
            strcat(write_buffer, "!!!!! delay ");
            strcat(write_buffer, delay_as_char);
            strcat(write_buffer, " !!!!!\n");
        }
        

        quinter = strstr(quinter, "\t");
        if (quinter == 0) {
            break;
        }

        printf("%s", write_buffer);
    }

}

int main() {
    srand(time(NULL));
    run_test("/test1");
}