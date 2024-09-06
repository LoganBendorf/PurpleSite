
#include <errno.h>
#include <ctype.h>

#include "connection_helpers.h"
#include "send_status.h"
#include "hash_functions.h"
#include "resource_handlers.h"

// Declared in https_server.c
extern const char* DELIMITER;
extern char* profanity_list[128];
extern int profanity_hash_list[128];


struct email {
    char sender[MAX_NAME_BYTES + 1];
    int hash;
    char title[MAX_TITLE_BYTES + 1];
    char body[MAX_COMPOSE_BYTES + 1];
    int upvotes;
    time_t time_created;

    struct email* next;
};

struct vote {
    char sender[MAX_NAME_BYTES + 1];
    char title[MAX_TITLE_BYTES + 1];
    int hash;

    struct vote* next;
};

struct user {
    char username[MAX_NAME_BYTES + 1];
    struct email* emails;
    struct vote* votes;

    struct user* next;
};

#define exit_print_line \
    printf("Line: %d\n", __LINE__); \
    exit(1)

#define print_msg_send400_return(x) \
        do { \
            char* msg = x; \
            printf("%s", msg); \
            send_400(client, clients_ptr, msg); \
            return; \
        } while (0)
    
static void user_data_search(char** pointer_ptr, char** quinter_ptr, char* search, char* buffer, int buffer_size) {
    *pointer_ptr = strstr(*pointer_ptr, search);          
    if (*pointer_ptr == 0) {                         
        exit_print_line; }                      

    *pointer_ptr += strlen(search);                  
    *quinter_ptr = strstr(*pointer_ptr, DELIMITER);       
    if (*quinter_ptr == 0 || *quinter_ptr - *pointer_ptr <= 0) {
        exit_print_line; }
                                                
    if (*quinter_ptr - *pointer_ptr >= buffer_size) {
        exit_print_line; }
    memcpy(buffer, *pointer_ptr, *quinter_ptr - *pointer_ptr);
    *pointer_ptr = *quinter_ptr;
}

#if DEBUG_INIT_USERS != true
#define printf(...) 
#endif

struct user* init_users() {
    printf("initializing users into data structure (linked list)\n");

    //FILE* fp = fopen("fake_emails.txt", "rba");
    FILE* fp = fopen("public/emails.txt", "rba");

    if (fp == NULL) {
        exit_print_line;
    }
    
    fseek(fp, 0L, SEEK_END);
    size_t emails_length = ftell(fp);
    rewind(fp);

    if (emails_length <= 0) {
        exit_print_line; }

    if (emails_length > 5001001) {
        exit_print_line; }

    // Malloc should be fine i think
    char* buffer = (char*) malloc(emails_length);
    if (buffer == NULL) {
        exit_print_line; }

    int read = fread(buffer, 1, emails_length, fp);
    if (read != emails_length) {
        exit_print_line; }

    char* pointer = buffer;
    char* quinter;

    struct user* users = NULL;

    if (pointer >= emails_length + buffer) {
        exit_print_line;}
    while (pointer < emails_length + buffer) {
        struct user* user = (struct user*) calloc(1, sizeof(struct user));

        char username_start[16] = {0};
        sprintf(username_start, "%susername: ", DELIMITER);
        pointer = strstr(pointer, username_start);
        if (pointer == 0) {
            break;}
        pointer += strlen(username_start);

        quinter = strstr(pointer, DELIMITER);
        if (quinter == 0) {
            break;}
        strncpy(user->username, pointer, quinter - pointer);

        char emails_start_search[16] = {0};
        sprintf(emails_start_search, "%semails:", DELIMITER);

        char* emails_start = strstr(pointer, emails_start_search);
        if (emails_start == 0) {
            exit_print_line; }

        char voted_start_search[16] = {0};
        sprintf(voted_start_search, "%svoted:", DELIMITER);

        char* voted_start = strstr(emails_start, voted_start_search);
        if (voted_start == 0) {
            exit_print_line; }

        struct email* emails = NULL;

        pointer = emails_start + strlen(emails_start_search);
        // Has emails
        if (pointer != voted_start) {

            char sender_search[16] = {0};
            sprintf(sender_search, "%ssender: ", DELIMITER);

            char title_search[16] = {0};
            sprintf(title_search, "%stitle: ", DELIMITER);

            char hash_search[16] = {0};
            sprintf(hash_search, "%shash: ", DELIMITER);

            char body_search[16] = {0};
            sprintf(body_search, "%sbody: ", DELIMITER);

            char upvotes_search[16] = {0};
            sprintf(upvotes_search, "%supvotes: ", DELIMITER);

            char time_search[32] = {0};
            sprintf(time_search, "%stime created: ", DELIMITER);

            // Data should already be clean
            int num_of_emails = 0;
            while (pointer < voted_start) {
                printf("\nemail\n");

                // Sender
                char sender[MAX_NAME_BYTES + 1] = {0};
                user_data_search(&pointer, &quinter, sender_search, sender, MAX_NAME_BYTES + 1);
                printf("sender = %s\n", sender);

                // Title
                char title[MAX_TITLE_BYTES + 1] = {0};
                user_data_search(&pointer, &quinter, title_search, title, MAX_TITLE_BYTES + 1);
                printf("title = %s\n", title);

                // Hash
                char hash_as_string[32] = {0};
                user_data_search(&pointer, &quinter, hash_search, hash_as_string, 32);
                printf("hash_as_string = %s\n", hash_as_string);

                // Will segfault if string is to big
                int hash = atoi(hash_as_string);

                // Body               
                char body[MAX_COMPOSE_BYTES + 1] = {0};
                user_data_search(&pointer, &quinter, body_search, body, MAX_COMPOSE_BYTES + 1); 
                printf("body = %s\n", body);

                // Upvotes
                char upvotes_as_string[32] = {0};
                user_data_search(&pointer, &quinter, upvotes_search, upvotes_as_string, 32);
                printf("upvotes_as_string = %s\n", upvotes_as_string);

                int upvotes = atoi(upvotes_as_string);
                
                // Date
                char time_as_string[64] = {0};
                user_data_search(&pointer, &quinter, time_search, time_as_string, 64);
                printf("time_as_string = %s\n", time_as_string);

                size_t time = atoi(time_as_string);

                pointer += strlen(DELIMITER);

                struct email* email = (struct email*) calloc(1, sizeof(struct email));

                strncpy(email->sender, sender, strlen(sender));
                strncpy(email->title, title, strlen(title));
                email->hash = hash;
                strncpy(email->body, body, strlen(body));
                email->upvotes = upvotes;
                email->time_created = time;

                email->next = emails;
                emails = email;
                num_of_emails++;
            }
            if (num_of_emails == 0) {
                exit_print_line; }
        }

        char end_search[16] = {0};
        sprintf(end_search, "%suser_end:", DELIMITER);

        char* end = strstr(emails_start, end_search);
        if (end == 0) {
            exit_print_line; }

        struct vote* votes = NULL;

        pointer = voted_start;
        // Has votes
        if (pointer + strlen(voted_start_search) != end) {
            printf("\nvote\n");

            char sender_search[16] = {0};
            sprintf(sender_search, "%ssender: ", DELIMITER);

            char title_search[16] = {0};
            sprintf(title_search, "%stitle: ", DELIMITER);

            char hash_search[16] = {0};
            sprintf(hash_search, "%shash: ", DELIMITER);

            int num_of_votes = 0;
            while (pointer < end) {
                // Sender
                char sender[MAX_NAME_BYTES + 1] = {0};
                user_data_search(&pointer, &quinter, sender_search, sender, MAX_NAME_BYTES + 1);

                // Title
                char title[MAX_TITLE_BYTES + 1] = {0};
                user_data_search(&pointer, &quinter, title_search, title, MAX_TITLE_BYTES + 1);

                // Hash
                char hash_as_string[32] = {0};
                user_data_search(&pointer, &quinter, hash_search, hash_as_string, 32);

                // Will segfault if string is to big
                int hash = atoi(hash_as_string);

                pointer += strlen(DELIMITER);

                struct vote* vote = (struct vote*) calloc(1, sizeof(struct vote));

                strncpy(vote->sender, sender, strlen(sender));
                strncpy(vote->title, title, strlen(title));
                vote->hash = hash;

                vote->next = votes;
                votes = vote;
                num_of_votes++;
            }
            if (num_of_votes == 0) {
                exit_print_line; }
        }

        user->emails = emails;
        user->votes = votes;
        user->next = users;
        users = user;
    }

    printf("    |Printing Users|\n");
    if (users == NULL) {
        printf("found no users\n");
    }
    print_users(users);

    return users;
}

#if DEBUG_INIT_USERS != true
#undef printf
#endif


void print_users(struct user* users) {
    struct user* user_head = users;
    while (user_head) {
        print_user(user_head);
        user_head = user_head->next;
    }
}

void print_user(struct user* user) {
    printf("Username: %s\n", user->username);
    struct email* email_head = user->emails;
    int count = 1;
    printf("Emails:\n");
    while (email_head) {
        printf("%d.\n", count++);
        printf("    Sender: %s\n", email_head->sender);
        printf("    Title:  %s\n", email_head->title);
        printf("    Hash:   %d\n", email_head->hash);
        printf("    Body:   %s\n", email_head->body);
        printf("    Upvotes %d\n", email_head->upvotes);
        printf("    Time:   %ld\n", email_head->time_created);
        email_head = email_head->next;
    }
    struct vote* vote_head = user->votes;
    count = 1;
    printf("Votes:\n");
    while (vote_head) {
        printf("%d.\n", count++);
        printf("    Sender: %s\n", vote_head->sender);
        printf("    Title:  %s\n", vote_head->title);
        printf("    Hash:   %d\n", vote_head->hash);
        vote_head = vote_head->next;
    }
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


void serve_resource(struct client_info* client, struct client_info** clients_ptr, struct user* users, char* path) {
    if (client == NULL) {
        fprintf(stderr, "serve_resource(). Called with NULL client\n");
        return;}
    printf("serve_resource() %s %s\n", get_client_address(&client), path);
    
    // Home page
    if (strcmp(path, "/") == 0) path = "/index.html";

    // Hall of fame
    if (strcmp(path, "/hall_of_fame") == 0) {
        printf("Recursing chat\n");
        serve_resource(client, clients_ptr, users, "/hall_of_fame/index.html");
        serve_resource(client, clients_ptr, users, "/hall_of_fame/styles.css");
        serve_resource(client, clients_ptr, users, "/hall_of_fame/favicon.ico");
        return;
    }

    // User info
    if (strncmp(path, "/users/", 7) == 0) {
        printf("User emails requested\n");
        char* pointer = path + 7;
        char* quinter = strstr(pointer, "/");
        if (quinter == 0) {
            print_msg_send400_return("Malformed user info request\n");}

        char username[MAX_NAME_BYTES + 1] = {0};
        strncpy(username, pointer, quinter - pointer);

        bool found = false;
        struct user* user_head = users;
        while (user_head) {
            if (strcmp(user_head->username, username) == 0) {
                found = true;
                break;}
            user_head = user_head->next;
        }

        if (!found) {
            print_msg_send400_return("Requested user not found\n");}

        pointer += strlen(username);
        if (pointer > path + strlen(path)) {
            print_msg_send400_return("Malformed user info request, no info after user name\n");}

        quinter = strstr(pointer, "/emails");
        if (quinter == 0) {
            quinter = strstr(pointer, "/votes");
            if (quinter == 0) {
                print_msg_send400_return("Unknown request for user\n");}
            // Find votes
            struct vote* vote_head = user_head->votes;
            while (vote_head) {
                int bytes = 0;
                char buffer[256] = {0};

                strcat(buffer, DELIMITER);
                bytes += strlen(DELIMITER);

                strcat(buffer, vote_head->sender);
                bytes += strlen(vote_head->sender);
                strcat(buffer, DELIMITER);
                bytes += strlen(DELIMITER);
                
                strcat(buffer, vote_head->title);
                bytes += strlen(vote_head->title);
                strcat(buffer, DELIMITER);
                bytes += strlen(DELIMITER);
                
                char hash_as_string[32] = {0};
                sprintf(hash_as_string, "%d", vote_head->hash);
                strcat(buffer, hash_as_string);
                bytes += strlen(hash_as_string);
                strcat(buffer, DELIMITER);
                bytes += strlen(DELIMITER);

                SSL_write(client->ssl, buffer, bytes);
                vote_head = vote_head->next;
            }
        } else {
            // Find emails
            struct email* email_head = user_head->emails;
            while (email_head) {
                int bytes = 0;
                // Max email size is about 1500 I think
                char buffer[2048] = {0};

                strcat(buffer, DELIMITER);
                bytes += strlen(DELIMITER);

                strcat(buffer, email_head->sender);
                bytes += strlen(email_head->sender);
                strcat(buffer, DELIMITER);
                bytes += strlen(DELIMITER);
                
                strcat(buffer, email_head->title);
                bytes += strlen(email_head->title);
                strcat(buffer, DELIMITER);
                bytes += strlen(DELIMITER);
                
                char hash_as_string[32] = {0};
                sprintf(hash_as_string, "%d", email_head->hash);
                strcat(buffer, hash_as_string);
                bytes += strlen(hash_as_string);
                strcat(buffer, DELIMITER);
                bytes += strlen(DELIMITER);

                strcat(buffer, email_head->body);
                bytes += strlen(email_head->body);
                strcat(buffer, DELIMITER);
                bytes += strlen(DELIMITER);

                char upvotes_as_string[32] = {0};
                sprintf(upvotes_as_string, "%d", email_head->upvotes);
                strcat(buffer, upvotes_as_string);
                bytes += strlen(upvotes_as_string);
                strcat(buffer, DELIMITER);
                bytes += strlen(DELIMITER);

                char time_as_string[64] = {0};
                sprintf(time_as_string, "%ld", email_head->time_created);
                strcat(buffer, time_as_string);
                bytes += strlen(time_as_string);
                strcat(buffer, DELIMITER);
                bytes += strlen(DELIMITER);

                SSL_write(client->ssl, buffer, bytes);
                email_head = email_head->next;
            }
        }
        return;
    }

    if (strstr(path, "..")) {
        printf("Path contained ..\n");
        send_404(client, clients_ptr);
        return;
    }

    // If public is somehow not found
    if (strstr(path, "public") != 0) {
        printf("sussy request\n");
        send_404(client, clients_ptr);
        return;
    }

    char full_path[128];
    sprintf(full_path, "public%s", path);
    
    FILE* fp = fopen(full_path, "rba");

    if (fp == NULL) {
        printf("File failed to open, full_path = %s. Errno = %d, errstr = %s\n", full_path, errno, strerror(errno));
        send_404(client, clients_ptr);
        return;
    }
    
    fseek(fp, 0L, SEEK_END);
    size_t content_length = ftell(fp);
    rewind(fp);

    const char* content_type = get_content_type(full_path);
    

    #define BSIZE 1024
    char buffer[BSIZE] = {0};

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
    
    int bytes_read = fread(buffer, 1, BSIZE, fp);
    while (bytes_read != 0) {
        SSL_write(client->ssl, buffer, bytes_read);
        bytes_read = fread(buffer, 1, BSIZE, fp);
    }

    fclose(fp);
}

typedef union {
    char c[4];
    int i;
} raw_int;

// Counts number of legal charcters (funny ascii symbols, letters, digits)
// Only allows newline in the text area
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
            if (i + strlen(DELIMITER) - 1 < strlen(string)) {
                bool contains_delimiter = true;
                for (int j = 0; j < strlen(DELIMITER); j++) {
                    if (string[i + j] != DELIMITER[j]) {
                        contains_delimiter = false;
                        break;
                    }
                }
                if (contains_delimiter) {
                    printf("String illegally contains delimter \"%s\"", DELIMITER);
                    return -1;
                }
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

void handle_put(struct client_info* client, struct client_info** clients_ptr) {
    printf("handle_put()\n");

    char* end = client->received + client->request;
    char* pointer = client->request;
    char* quinter;
    
    char* headerType = "PUT / HTTP1.1\r\n";
    if (pointer + strlen(headerType) >= end || strncmp(pointer, headerType, strlen(headerType))) {
        print_msg_send400_return("First put line malformed\n");
    }
    pointer += strlen(headerType);

    // Username
    char* usernameStart = ("Content-Disposition: form-data; name=\"username\"\r\n");
    if (pointer + strlen(usernameStart) >= end) {
        print_msg_send400_return("Failed to find username start, not enough data\n");
    }
    pointer = strstr(pointer, usernameStart);
    if (pointer == NULL) {
        print_msg_send400_return("Failed to find username start\n");
    }
    pointer += strlen(usernameStart);

    quinter = strstr(pointer, "\r\n");
    if (quinter == 0) {
        print_msg_send400_return("Failed to find username end\n");
    }

    // Removes whitespace from the middle so Mike and M ike are the same in the database
    char username[MAX_NAME_BYTES + 1] = {0};
    for (int i = 0; i < quinter - pointer; i++) {
        if (*pointer == ' ' || *pointer == '\t' || *pointer == '\n' || *pointer == '\r'){
            continue;
        }
        username[i] = pointer[i];
    }

    // Remove whitespace from end
    char* end_of_username = quinter - 1;
    int white_space_count = 0;
    while (*end_of_username == ' ' || *end_of_username == '\t' || *end_of_username == '\n' || *end_of_username == '\r') {
        if (end_of_username <= pointer) {
            break;
        }
        *end_of_username = 0;
        end_of_username--;
        white_space_count++;
    }

    if (quinter - pointer - white_space_count <= 0) {
        print_msg_send400_return("Received empty username\n");
    }
    username[quinter - pointer - white_space_count] = 0;
    if (strlen(username) == 0) {
        print_msg_send400_return("Received empty username\n");
    }

    int kosher_chars;
    kosher_chars = num_kosher_chars(username, 0);
    if (kosher_chars <= 0) {
        print_msg_send400_return("Username is not kosher");
    }
    if (kosher_chars > MAX_NAME_CHARACTERS) {
        print_msg_send400_return("Username contained too many characterss");
    }

    if (contains_profanity(username)) {
        print_msg_send400_return("Username has swears );");
    }


    // Sender
    char* senderStart = ("Content-Disposition: form-data; name=\"sender\"\r\n");
    if (pointer + strlen(senderStart) >= end) {
        print_msg_send400_return("Failed to find sender start, not enough data\n");
    }
    pointer = strstr(pointer, senderStart);
    if (pointer == NULL) {
        print_msg_send400_return("Failed to find sender start\n");
    }
    pointer += strlen(senderStart);

    quinter = strstr(pointer, "\r\n");
    if (quinter == 0) {
        print_msg_send400_return("Failed to find sender end\n");
    }

    // Removes whitespace from the middle so Mike and M ike are the same in the database,
    //      should probably also tolower() it
    char sender[MAX_NAME_BYTES + 1] = {0};
    for (int i = 0; i < quinter - pointer; i++) {
        if (*pointer == ' ' || *pointer == '\t' || *pointer == '\n' || *pointer == '\r'){
            continue;
        }
        sender[i] = pointer[i];
    }

    // Remove whitespace from end
    char* end_of_sender = quinter - 1;
    white_space_count = 0;
    while (*end_of_sender == ' ' || *end_of_sender == '\t' || *end_of_sender == '\n' || *end_of_sender == '\r') {
        if (end_of_sender <= pointer) {
            break;
        }
        *end_of_sender = 0;
        end_of_sender--;
        white_space_count++;
    }

    if (quinter - pointer - white_space_count <= 0) {
        print_msg_send400_return("Received empty sender\n");
    }
    sender[quinter - pointer - white_space_count] = 0;
    if (strlen(sender) == 0) {
        print_msg_send400_return("Received empty sender\n");
    }

    kosher_chars = num_kosher_chars(sender, 0);
    if (kosher_chars <= 0) {
        print_msg_send400_return("Sender is not kosher");
    }
    if (kosher_chars > MAX_NAME_CHARACTERS) {
        print_msg_send400_return("Sender contained too many characterss");
    }

    if (contains_profanity(sender)) {
        print_msg_send400_return("Sender has swears );");
    }

    // Hash
    char* hashStart = ("Content-Disposition: form-data; name=\"hash\"\r\n");
    if (pointer + strlen(hashStart) >= end) {
        print_msg_send400_return("Failed to find hash start, not enough data\n");
    }
    pointer = strstr(pointer, hashStart);
    if (pointer == NULL) {
        print_msg_send400_return("Failed to find hash start\n");
    }
    pointer += strlen(hashStart);

    quinter = strstr(pointer, "\r\n");
    if (quinter == 0) {
        print_msg_send400_return("Failed to find hash end\n");
    }

    // Biggest hash possible is 4,294,967,295, 10 characters
    if (quinter - pointer > 10 /*11?*/) {
        print_msg_send400_return("Hash is larger than possible\n");
    }

    char hash[16] = {0};
    for (int i = 0; i < quinter - pointer; i++) {
        if (!isdigit(pointer[i])) {
            print_msg_send400_return("Non-digit found in hash\n");
        }
        sender[i] = pointer[i];
    }

    if (strlen(hash) == 0) {
        print_msg_send400_return("Received empty hash\n");
    }

    // Search 
}


#if PRINT_POST_PARSE == false
#define printf(...) 
#endif

void handle_post(struct client_info* client, struct client_info** clients_ptr, struct user** users_ptr) {
    printf("handle_post()\n");

    char* end = client->received + client->request;
    char* pointer = client->request;
    char* quinter;
    
    // pointer + 17 >= end is untested, might reject valid posts
    if (pointer + 17 >= end || strncmp(pointer, "POST / HTTP/1.1\r\n", 17) != 0) {
        printf("First post line malformed\n");
        send_400(client, clients_ptr, NULL);
        return;
    }

    // FIND CONTENT LENGTH
    #define contentLengthFail(x) printf("Failed to parse content length. Line %d\n", x); \
        send_400(client, clients_ptr, NULL); \
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
        send_400(client, clients_ptr, NULL); \
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
        send_400(client, clients_ptr, NULL); \
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
    char boundary[69] = {0};
    strncpy(boundary, "--", 2);
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
        if (y != 0) { \
            printf("Error = %s\n", y); \
        } \
        if ((pointer - client->request) < contentLength && client->parseFailures < 2) {\
            printf("parse failure incremented\n"); \
            client->parseFailures += 1; \
            return; \
        }\
        printf("max parse failures\n"); \
        send_400(client, clients_ptr, y); \
        return; \

    // GET USERNAME ///////////////////////////////////////////////
    if (pointer >= end) {
        locateContentFail(__LINE__, "");
    }
    quinter = strstr(pointer, boundary);
    if (quinter == 0) {
        printf("boundary = %s\n", boundary);
        printf("pointer: %.*s\n", 10, pointer);
        locateContentFail(__LINE__, "");
    }  
    pointer = quinter;
    pointer += strlen(boundary);
    if (strncmp(pointer, "\r\n", 2) != 0) {
        fprintf(stderr, "pointer = %s\n", pointer);
        locateContentFail(__LINE__, "");
    }
    pointer += 2;

    if (strncmp(pointer, "Content-Disposition: form-data; name=\"username\"", 47) != 0) {
        locateContentFail(__LINE__, "content-disposition username line not found\n");
    }
    pointer += 47;
    if (strncmp(pointer, "\r\n", 2) != 0) {
        locateContentFail(__LINE__, "content-disposition username line \\r\\n delimiter not found\n");
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
            locateContentFail(__LINE__, "");
        }
    }
    if (quinter - pointer > MAX_NAME_BYTES) {
        printf("pointer = %p\n", pointer);
        printf("quinter = %p\n", quinter);
        locateContentFail(__LINE__, "");
    }
    
    // Removes whitespace from the middle so Mike and M ike are the same in the database,
    //      should probably also tolower() it
    char username[MAX_NAME_BYTES + 1] = {0};
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
        send_400(client, clients_ptr, "Empty username");
        return;
    }
    username[quinter - pointer - whiteSpaceCount] = 0;

    // Check if username string is empty
    if (strlen(username) == 0) {
        printf("Received empty username\n");
        send_400(client, clients_ptr, "Empty username");
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
        locateContentFail(__LINE__, "");
    }
    pointer += 2;
    // END USERNAME ///////////////////////////////////////////////

    // GET RECIPIENT ////////////////////////////////////////////////
    if (strncmp(pointer, "Content-Disposition: form-data; name=\"recipient\"", 48) != 0) {
        locateContentFail(__LINE__, "");
    }
    pointer += 48;

    if (strncmp(pointer, "\r\n", 2) != 0) {
        locateContentFail(__LINE__, "");
    }
    pointer += 2;

    quinter = strstr(pointer, boundary);
    if (quinter - pointer <= 2 || quinter - pointer > MAX_NAME_BYTES + 2) {
        locateContentFail(__LINE__, "");
    }

    // Should skip through whitespace
    while (*pointer == ' ' || *pointer == '\t' || *pointer == '\n' || *pointer == '\r'){
        pointer++;
        if (pointer >= end) {
            locateContentFail(__LINE__, "");
        }
    }
    if (quinter - pointer > MAX_NAME_BYTES) {
        locateContentFail(__LINE__, "");
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
        send_400(client, clients_ptr, "Empty recipient");
        return;
    }
    recipient[quinter - pointer - whiteSpaceCount] = 0;

    // Check if recipient string is empty
    if (strlen(recipient) == 0) {
        printf("Received empty recipient\n");
        send_400(client, clients_ptr, "Empty recipient");
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
        locateContentFail(__LINE__, "");
    }
    pointer += 2;
    // END RECIPIENT ////////////////////////////

    // GET TITLE ////////////////////////////////////////////////
    if (strncmp(pointer, "Content-Disposition: form-data; name=\"title\"", 44) != 0) {
        locateContentFail(__LINE__, "");
    }
    pointer += 44;

    if (strncmp(pointer, "\r\n", 2) != 0) {
        locateContentFail(__LINE__, "");
    }
    pointer += 2;

    quinter = strstr(pointer, boundary);
    if (quinter - pointer <= 2 || quinter - pointer > MAX_TITLE_BYTES) {
        locateContentFail(__LINE__, "");
    }

    // Should skip through whitespace
    while (*pointer == ' ' || *pointer == '\t' || *pointer == '\n' || *pointer == '\r') {
        pointer++;
        if (pointer >= end) {
            locateContentFail(__LINE__, "");
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
        send_400(client, clients_ptr, "Empty title");
        return;
    }
    title[quinter - pointer - whiteSpaceCount] = 0;

    // Check if title string is empty
    if (strlen(title) == 0) {
        printf("Received empty title\n");
        send_400(client, clients_ptr, "Empty title");
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
        locateContentFail(__LINE__, "");
    }
    pointer += 2;

    // END TITLE ////////////////////////////

    if (strncmp(pointer, "Content-Disposition: form-data; name=\"composeText\"", 50) != 0) {
        locateContentFail(__LINE__, "");
    }
    pointer += 50;

    if (strncmp(pointer, "\r\n", 2) != 0) {
        locateContentFail(__LINE__, "");
    }
    pointer += 2;

    quinter = strstr(pointer, boundary);
    if (quinter - pointer <= 2 || quinter - pointer > MAX_COMPOSE_BYTES) {
        locateContentFail(__LINE__, "");
    }

    // Should skip through whitespace
    while (*pointer == ' ' || *pointer == '\t' || *pointer == '\n' || *pointer == '\r') {
        pointer++;
        if (pointer >= end) {
            locateContentFail(__LINE__, "");
        }
    }
    char body[MAX_COMPOSE_BYTES + 1];
    for (int i = 0; i < quinter - pointer; i++) {
        body[i] = pointer[i];
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
        send_400(client, clients_ptr, "Empty text area");
        return;
    }
    body[quinter - pointer - whiteSpaceCount] = 0;

    if (strlen(body) == 0) {
        locateContentFail(__LINE__, "Empty text area");}

    kosher_chars = num_kosher_chars(body, 1);
    if (kosher_chars <= 0) {
        locateContentFail(__LINE__, "Text area is not kosher");}

    if (kosher_chars > MAX_COMPOSE_CHARACTERS) {
        locateContentFail(__LINE__, "Text area contained too many characters");}

    if (contains_profanity(body)) {
        locateContentFail(__LINE__, "Text area has swears );");}

    int body_characters = kosher_chars;
    int body_hash = hash_function(body);
    // END OF TEXT AREA

    #undef locateContentFail
    //END OF LOCATE CONTENT
    printf("Content-Length = %d\n", contentLength);
    printf("Content-Type = %s\n", contentTypeBuffer);
    pointer = username;
    for (int i = 0; i < strlen(username); i++) {
        // Skip tolower if unicode
        if (username[i] >> 7 & 1 == 1) {
            continue;
        }
        username[i] = tolower(username[i]);
    }
    printf("username = %s\n", username);

    pointer = recipient;
    for (int i = 0; i < strlen(recipient); i++) {
        // Skip tolower if unicode
        if (recipient[i] >> 7 & 1 == 1) {
            continue;
        }
        recipient[i] = tolower(recipient[i]);
    }
    printf("recipient = %s\n", recipient);
    printf("title = %s\n", title);
    printf("body = %s\n", body);
    printf("\nsize of body = %ld\n", strlen(body));
    printf("body_characters = %d\n", body_characters);
    printf("\n");
    printf("body hash = %d\n", body_hash);
    printf("End of content\n");
    
    // Create email 
    struct email* new_email = (struct email*) calloc(1, sizeof(struct email));
    if (new_email == NULL) {
        printf("Error: New email equaled null, out of memory?");
        return;}
    memcpy(new_email->sender, username, MAX_NAME_BYTES + 1);
    memcpy(new_email->title, title, MAX_TITLE_BYTES + 1);
    new_email->hash = body_hash;
    memcpy(new_email->body, body, MAX_COMPOSE_BYTES + 1);
    new_email->upvotes = 0;  
    time_t rawtime;
    time(&rawtime);
    printf("time created = %ld\n", rawtime);
    new_email->time_created = rawtime;
    
    // Append email to user
    bool in_struct = false;
    struct user* user_head = *users_ptr;
    while (user_head) {
        printf("head = %s, ", user_head->username);
        printf("recip = %s\n", recipient);
        if (strcmp(user_head->username, recipient) == 0) {
            printf("old user\n");
            in_struct = true;
            break;
        }
        user_head = user_head->next;
    }

    if (!in_struct) {
        printf("new user\n");
        struct user* new_user = (struct user*) calloc(1, sizeof(struct user));
        if (new_user == 0) {
            printf("Error: new_user equaled null, out of memory");
            return;}
        memcpy(new_user->username, recipient, MAX_NAME_BYTES + 1);
        new_user->next = *users_ptr;
        *users_ptr = new_user;
        user_head = new_user;
    }

    new_email->next = user_head->emails;
    user_head->emails = new_email;

    print_users(*users_ptr);

    send_200(client, clients_ptr);

    drop_client(client, clients_ptr);
}
#if PRINT_POST_PARSE == false
#undef printf
#endif