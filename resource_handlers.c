
#include <errno.h>
#include <ctype.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h> 

#include <dirent.h>   // For directory operations
#include <sys/stat.h> // For struct stat

#include "connection_helpers.h"
#include "send_status.h"
#include "hash_functions.h"
#include "resource_handlers.h"

// Declared in https_server.c
extern const char* DELIMITER;
extern char* profanity_list[128];
extern int profanity_hash_list[128];


#define ERR_MSG_BUF_LEN 128

#define exit_print_line \
    printf("Line: %d\n", __LINE__); \
    exit(1)

#define print_msg_send400_return(x) \
        do { \
            char msg[256] = {0}; \
            strncpy(msg, x, 256); \
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
    printf("\ninitializing users into data structure (linked list)\n");

    char sender_search[16] = {0};
    sprintf(sender_search, "sender: ");

    char title_search[16] = {0};
    sprintf(title_search, "title: ");

    char hash_search[16] = {0};
    sprintf(hash_search, "hash: ");

    char body_search[16] = {0};
    sprintf(body_search, "body: ");

    char upvotes_search[16] = {0};
    sprintf(upvotes_search, "upvotes: ");

    char time_search[32] = {0};
    sprintf(time_search, "time_created: ");

    const char *users_path = "./public/users";
    struct dirent *users_dir;
    struct stat file_stat;
    DIR *dir = opendir(users_path);
    if (dir == NULL) {
        perror("Unable to open users directory"); exit(1);}

    struct user* users = {0};

    // like 10 MB
    #define MAX_FILE_SIZE 1024 * 1000 * 10
    char* file_buffer = (char*) calloc(1, MAX_FILE_SIZE);
    // Iterate through users
    while ((users_dir = readdir(dir)) != NULL) {
        // Skip the special entries "." and ".."
        if (strcmp(users_dir->d_name, ".") == 0 || strcmp(users_dir->d_name, "..") == 0) {
            continue;}

        char user_path[512];
        snprintf(user_path, 512, "%s/%s", users_path, users_dir->d_name);

        if (stat(users_path, &file_stat) == 0) {
            if (S_ISDIR(file_stat.st_mode)) {
                printf("Directory: %s\n", users_dir->d_name);}
        } else {
            perror("Unable to get file status"); exit(1);}


        struct dirent *user_dir;
        DIR *dir2 = opendir(user_path);
        if (dir2 == NULL) {
            perror("Unable to open users directory"); exit(1);}

        struct user* user = (struct user*) calloc(1, sizeof(struct user));
        struct email* emails = {0};
        struct vote* votes = {0};

        // Emails and votes
        while ((user_dir = readdir(dir2)) != NULL) {
            // Skip the special entries "." and ".."
            if (strcmp(user_dir->d_name, ".") == 0 || strcmp(user_dir->d_name, "..") == 0) {
                continue;}

            char component_path[1024];
            snprintf(component_path, 1024, "%s/%s", user_path, user_dir->d_name);

            if (stat(users_path, &file_stat) == 0) {
                if (!S_ISDIR(file_stat.st_mode)) {
                    printf("File: %s\n", user_dir->d_name);}
            } else {
                perror("Unable to get file status"); exit(1);}

            char* pointer;
            char* quinter;
            char* end;
            if (strcmp(user_dir->d_name, "emails") == 0) {
                FILE* fp = fopen(component_path, "r");
                if (fp == NULL) {
                    printf("couldn't open user emails file\n"); exit(1);}

                int bytes_read = fread(file_buffer, 1, MAX_FILE_SIZE, fp);
                if (bytes_read >= MAX_FILE_SIZE) {
                    printf("bruh, file it way too big\n"); exit(1);}

                printf("\nFILE:\n%s\n", file_buffer);

                pointer = file_buffer;
                end = file_buffer + bytes_read;
                
                while (pointer < end) {
                    printf("\nemail\n");
                    // Sender
                    char sender[MAX_NAME_BYTES + 1] = {0};
                    pointer = strstr(pointer, sender_search);
                    if (pointer == 0) {
                        printf("Couldn't find sender start\n"); exit(1);}
                    pointer += strlen(sender_search);
                    quinter = strstr(pointer, DELIMITER);
                    if (pointer == 0) {
                        printf("Couldn't find sender start delimiter\n"); exit(1);}
                    strncpy(sender, pointer, min(quinter - pointer, MAX_NAME_BYTES + 1));
                    printf("sender = %s\n", sender);

                    // Title
                    char title[MAX_NAME_BYTES + 1] = {0};
                    pointer = strstr(quinter, title_search);
                    if (pointer == 0) {
                        printf("Couldn't find title start\n"); exit(1);}
                    pointer += strlen(title_search);
                    quinter = strstr(pointer, DELIMITER);
                    if (pointer == 0) {
                        printf("Couldn't find title start delimiter\n"); exit(1);}
                    strncpy(title, pointer, min(quinter - pointer, MAX_TITLE_BYTES + 1));
                    printf("title = %s\n", title);

                    // Hash
                    char hash_as_string[32] = {0};
                    pointer = strstr(quinter, hash_search);
                    if (pointer == 0) {
                        printf("Couldn't find hash start\n"); exit(1);}
                    pointer += strlen(hash_search);
                    quinter = strstr(pointer, DELIMITER);
                    if (pointer == 0) {
                        printf("Couldn't find hash start delimiter\n"); exit(1);}
                    strncpy(hash_as_string, pointer, min(quinter - pointer, 32));
                    int hash = atoi(hash_as_string);
                    printf("hash = %d\n", hash);

                    // Body
                    char body[MAX_COMPOSE_BYTES + 1] = {0};
                    pointer = strstr(quinter, body_search);
                    if (pointer == 0) {
                        printf("Couldn't find body start\n"); exit(1);}
                    pointer += strlen(body_search);
                    quinter = strstr(pointer, DELIMITER);
                    if (pointer == 0) {
                        printf("Couldn't find body start delimiter\n"); exit(1);}
                    strncpy(body, pointer, min(quinter - pointer, MAX_COMPOSE_BYTES + 1));
                    printf("body = %s\n", body);

                    // Upvotes
                    char upvotes_as_string[32] = {0};
                    pointer = strstr(quinter, upvotes_search);
                    if (pointer == 0) {
                        printf("Couldn't find upvotes start\n"); exit(1);}
                    pointer += strlen(upvotes_search);
                    quinter = strstr(pointer, DELIMITER);
                    if (pointer == 0) {
                        printf("Couldn't find upvotes start delimiter\n"); exit(1);}
                    strncpy(upvotes_as_string, pointer, min(quinter - pointer, 32));
                    int upvotes = atoi(upvotes_as_string);
                    printf("upvotes = %d\n", upvotes);

                    printf("pointer before time = %s\n", pointer);
                    // Time
                    char time_as_string[32] = {0};
                    pointer = strstr(quinter, time_search);
                    printf("pointer after strstr = %s\n", pointer);
                    if (pointer == 0) {
                        printf("Couldn't find time start\n"); exit(1);}
                    pointer += strlen(time_search);
                    printf("pointer after += = %s\n", pointer);
                    quinter = strstr(pointer, DELIMITER);
                    printf("quinter after strstr = %s\n", quinter);
                    if (pointer == 0) {
                        printf("Couldn't find time start delimiter\n"); exit(1);}
                    strncpy(time_as_string, pointer, min(quinter - pointer, 32));
                    int time_created = atoi(time_as_string);
                    printf("time_created = %d\n", time_created);

                    pointer = quinter + strlen(DELIMITER);

                    struct email* email = (struct email*) calloc(1, sizeof(struct email));

                    strncpy(email->sender, sender, strlen(sender));
                    strncpy(email->title, title, strlen(title));
                    email->hash = hash;
                    strncpy(email->body, body, strlen(body));
                    email->upvotes = upvotes;
                    email->time_created = time_created;

                    email->next = emails;
                    emails = email;
                }
            } else if (strcmp(user_dir->d_name, "votes") == 0) {
                FILE* fp = fopen(component_path, "r");
                if (fp == NULL) {
                    printf("couldn't open user votes file\n"); exit(1);}

                int bytes_read = fread(file_buffer, 1, MAX_FILE_SIZE, fp);
                if (bytes_read >= MAX_FILE_SIZE) {
                    printf("bruh, file it way too big\n"); exit(1);}

                pointer = file_buffer;
                end = file_buffer + bytes_read;

                while (pointer < end) {
                    printf("\nvote\n");
                    // Sender
                    char sender[MAX_NAME_BYTES + 1] = {0};
                    pointer = strstr(pointer, sender_search);
                    if (pointer == 0) {
                        printf("Couldn't find sender start\n"); exit(1);}
                    pointer += strlen(sender_search);
                    quinter = strstr(pointer, DELIMITER);
                    if (pointer == 0) {
                        printf("Couldn't find sender start delimiter\n"); exit(1);}
                    strncpy(sender, pointer, min(quinter - pointer, MAX_NAME_BYTES + 1));
                    printf("sender = %s\n", sender);

                    // Title
                    char title[MAX_NAME_BYTES + 1] = {0};
                    pointer = strstr(quinter, title_search);
                    if (pointer == 0) {
                        printf("Couldn't find title start\n"); exit(1);}
                    pointer += strlen(title_search);
                    quinter = strstr(pointer, DELIMITER);
                    if (pointer == 0) {
                        printf("Couldn't find title start delimiter\n"); exit(1);}
                    strncpy(title, pointer, min(quinter - pointer, MAX_TITLE_BYTES + 1));
                    printf("title = %s\n", title);

                    // Hash
                    char hash_as_string[32] = {0};
                    pointer = strstr(quinter, hash_search);
                    if (pointer == 0) {
                        printf("Couldn't find hash start\n"); exit(1);}
                    pointer += strlen(hash_search);
                    quinter = strstr(pointer, DELIMITER);
                    if (pointer == 0) {
                        printf("Couldn't find hash start delimiter\n"); exit(1);}
                    strncpy(hash_as_string, pointer, min(quinter - pointer, 32));
                    int hash = atoi(hash_as_string);
                    printf("hash = %d\n", hash);

                    pointer = quinter + strlen(DELIMITER);

                    struct vote* vote = (struct vote*) calloc(1, sizeof(struct vote));

                    strncpy(vote->sender, sender, strlen(sender));
                    strncpy(vote->title, title, strlen(title));
                    vote->hash = hash;

                    vote->next = votes;
                    votes = vote;
                }
            } else {
                printf("bad file in user directory\n"); exit(1);}
        } // end of user component while loop
        closedir(dir2);
        memcpy(user->username, users_dir->d_name, MAX_NAME_BYTES + 1);
        user->emails = emails;
        user->votes = votes;
        user->next = users;
        users = user;
    } // end of users while loop
    closedir(dir);

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


void save_users_to_file(struct user* users) {
    printf("\nsaving users to file\n");
    if (users == NULL) {
        return;}
    
    struct stat st = {0};
    if (stat("public", &st) == -1) {
        perror("public doesn't exist\n");
        exit(1); // Would be weird if public didn't exist
    }

    if (stat("public/users", &st) == -1) {
        printf("Creating public/users");
        mkdir("public/users", 0770);}

    struct user* user_head = users;
    while (user_head) {
        printf("    |Saving user: %s|\n", user_head->username);

        char user_dir[128] = {0};
        sprintf(user_dir, "public/users/%s", user_head->username);
        if (stat(user_dir, &st) == -1) {
            mkdir(user_dir, 0770);
            perror("dir err if exists: ");
        }

        char user_emails_location[128] = {0};
        sprintf(user_emails_location, "public/users/%s/emails", user_head->username);
        FILE* fp = fopen(user_emails_location, "w");
        if (fp == NULL) {
            perror("couldn't open user email file\n"); return;}

        struct email* email_head = user_head->emails;
        while (email_head) {
            fprintf(fp, "sender: ");
            fprintf(fp, "%s", email_head->sender);
            fprintf(fp, "%s", DELIMITER);

            fprintf(fp, "title: ");
            fprintf(fp, "%s", email_head->title);
            fprintf(fp, "%s", DELIMITER);

            fprintf(fp, "hash: ");
            fprintf(fp, "%d", email_head->hash);
            fprintf(fp, "%s", DELIMITER);

            fprintf(fp, "body: ");
            fprintf(fp, "%s", email_head->body);
            fprintf(fp, "%s", DELIMITER);

            fprintf(fp, "upvotes: ");
            fprintf(fp, "%d", email_head->upvotes);
            fprintf(fp, "%s", DELIMITER);

            fprintf(fp, "time_created: ");
            fprintf(fp, "%ld", email_head->time_created);
            fprintf(fp, "%s", DELIMITER);

            email_head = email_head->next;
        }
        fclose(fp);
        
        char user_votes_location[128] = {0};
        sprintf(user_votes_location, "public/users/%s/votes", user_head->username);
        fp = fopen(user_votes_location, "w");
        if (fp == NULL) {
            perror("couldn't open user votes file\n");
            return;}
        
        struct vote* vote_head = user_head->votes;
        while (vote_head) {
            fprintf(fp, "sender: ");
            fprintf(fp, "%s", vote_head->sender);
            fprintf(fp, "%s", DELIMITER);

            fprintf(fp, "title: ");
            fprintf(fp, "%s", vote_head->title);
            fprintf(fp, "%s", DELIMITER);

            fprintf(fp, "hash: ");
            fprintf(fp, "%d", vote_head->hash);
            fprintf(fp, "%s", DELIMITER);

            vote_head = vote_head->next;
        }
        fclose(fp);

        user_head = user_head->next;
    }
}


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

typedef enum vote_types {
    UPVOTE, UNUPVOTE
} vote_types;

// Finding boundary and strstring that would be more robust
void handle_put(struct client_info* client, struct client_info** clients_ptr, struct user* users) {
    printf("handle_put()\n");

    char* end = client->received + client->request;
    char* pointer = client->request;
    char* quinter;
    
    char* header_type = "PUT / HTTP/1.1\r\n";
    if (pointer + strlen(header_type) >= end || strncmp(pointer, header_type, strlen(header_type))) {
        print_msg_send400_return("First put line malformed\n");}
    pointer += strlen(header_type);

    // Vote type
    char* vote_start = ("Content-Disposition: form-data; name=\"vote_type\"\r\n\r\n");
    if (pointer + strlen(vote_start) >= end) {
        print_msg_send400_return("Failed to find vote type start, not enough data\n");}
    pointer = strstr(pointer, vote_start);
    if (pointer == 0) {
        print_msg_send400_return("Failed to find vote type start\n");}
    pointer += strlen(vote_start);

    quinter = strstr(pointer, "\r\n");
    if (quinter == 0) {
        print_msg_send400_return("Failed to find vote type end\n");}
    
    if (quinter - pointer > strlen("unupvote")) {
        print_msg_send400_return("Vote type too large\n");}

    vote_types vote_type;
    if (strncmp(pointer, "upvote", 6) == 0) {
        vote_type = UPVOTE;
    } else if (strncmp(pointer, "unupvote", 8) == 0) {
        vote_type = UNUPVOTE;
    } else {
        print_msg_send400_return("Unknown vote type\n");}

    // Username
    char* usernameStart = ("Content-Disposition: form-data; name=\"username\"\r\n\r\n");
    if (pointer + strlen(usernameStart) >= end) {
        print_msg_send400_return("Failed to find username start, not enough data\n");
        }
    pointer = strstr(pointer, usernameStart);
    if (pointer == 0) {
        print_msg_send400_return("Failed to find username start\n");}
    pointer += strlen(usernameStart);

    quinter = strstr(pointer, "\r\n");
    if (quinter == 0) {
        print_msg_send400_return("Failed to find username end\n");}

    // Removes whitespace from the middle so Mike and M ike are the same in the database
    char username[MAX_NAME_BYTES + 1] = {0};
    char error_msg[ERR_MSG_BUF_LEN] = {0};
    memcpy(error_msg, "Username: ", 10);
    sanitize_name(pointer, username, min(quinter - pointer, MAX_NAME_BYTES + 1), error_msg + 10);
    if (strlen(error_msg) > 10) {
        print_msg_send400_return(error_msg);}
    printf("username = %s\n", username);


    // Sender
    char* senderStart = ("Content-Disposition: form-data; name=\"sender\"\r\n\r\n");
    if (pointer + strlen(senderStart) >= end) {
        print_msg_send400_return("Failed to find sender start, not enough data\n");}
    pointer = strstr(pointer, senderStart);
    if (pointer == 0) {
        print_msg_send400_return("Failed to find sender start\n");}
    pointer += strlen(senderStart);

    quinter = strstr(pointer, "\r\n");
    if (quinter == 0) {
        print_msg_send400_return("Failed to find sender end\n");}

    char sender[MAX_NAME_BYTES + 1] = {0};
    memset(error_msg, 0 , ERR_MSG_BUF_LEN);
    memcpy(error_msg, "Sender: ", 8);
    sanitize_name(pointer, sender, min(quinter - pointer, MAX_NAME_BYTES + 1), error_msg + 10);
    if (strlen(error_msg) > 8) {
        print_msg_send400_return(error_msg);}
    printf("sender = %s\n", sender);

    // Title 
    char* titleStart = ("Content-Disposition: form-data; name=\"title\"\r\n\r\n");
    if (pointer + strlen(titleStart) >= end) {
        print_msg_send400_return("Failed to find title start, not enough data\n");}
    pointer = strstr(pointer, titleStart);
    if (pointer == 0) {
        print_msg_send400_return("Failed to find title start\n");}
    pointer += strlen(titleStart);

    quinter = strstr(pointer, "\r\n");
    if (quinter == 0) {
        print_msg_send400_return("Failed to find title end\n");}

    char title[MAX_TITLE_BYTES + 1] = {0};
    memset(error_msg, 0 , ERR_MSG_BUF_LEN);
    sanitize_title(pointer, title, min(quinter - pointer, MAX_NAME_BYTES + 1), error_msg);
    if (strlen(error_msg) > 0) {
        print_msg_send400_return(error_msg);}
    printf("title = %s\n", title);


    // Hash
    char* hashStart = ("Content-Disposition: form-data; name=\"hash\"\r\n\r\n");
    if (pointer + strlen(hashStart) >= end) {
        print_msg_send400_return("Failed to find hash start, not enough data\n"); }
    pointer = strstr(pointer, hashStart);
    if (pointer == 0) {
        print_msg_send400_return("Failed to find hash start\n");}
    pointer += strlen(hashStart);

    quinter = strstr(pointer, "\r\n");
    if (quinter == 0) {
        print_msg_send400_return("Failed to find hash end\n");}

    // Biggest hash possible is 4,294,967,295, 10 characters
    if (quinter - pointer > 10 /*11?*/) {
        print_msg_send400_return("Hash is larger than possible\n");}

    char hash_as_string[32] = {0};
    for (int i = 0; i < quinter - pointer; i++) {
        if (!isdigit(pointer[i])) {
            print_msg_send400_return("Non-digit found in hash\n");}
        hash_as_string[i] = pointer[i];
    }

    if (strlen(hash_as_string) == 0) {
        print_msg_send400_return("Received empty hash\n");}
    
    int hash = atoi(hash_as_string);
    printf("hash = %d\n", hash);

    // Search 
    struct user* user = search_for_user(username, users);
    if (user == NULL) {
        print_msg_send400_return("Vote, user not found\n");}
    
    struct email* email = search_for_email(user, sender, title, hash);
    if (email == (struct email*) -1) {
        print_msg_send400_return("Error during email search\n");}
    if (email == NULL) {
        print_msg_send400_return("Trying to vote on email that doesn't exist\n");}

    struct vote* seen_vote = search_for_vote(user, sender, title, hash);
    if (seen_vote == (struct vote*) -1) {
        print_msg_send400_return("Error during vote search\n");}
    if (seen_vote != NULL && vote_type == UPVOTE) {
        print_msg_send400_return("Already upvoted this email\n");}
    if (seen_vote == NULL && vote_type == UNUPVOTE) {
        print_msg_send400_return("Can't unupvote when vote doesn't exist\n");}

    if (vote_type == UPVOTE) {
        struct vote* new_vote = (struct vote*) calloc(1, sizeof(struct vote));
        if (new_vote == NULL) {
            printf("Calloc fail\n");
            send_507(client, clients_ptr);
            return;
        }
        strncpy(new_vote->sender, sender, MAX_NAME_BYTES + 1);
        strncpy(new_vote->title, title, MAX_TITLE_BYTES + 1);
        new_vote->hash = hash;

        new_vote->next = user->votes;

        user->votes->prev = new_vote;
        user->votes = new_vote;

        email->upvotes += 1;
    }

    if (vote_type == UNUPVOTE) {
        if (email->upvotes <= 0) {
            char msg[256] = {0};
            strcpy(msg, "Somehow vote exists on emails with 0 or less upvotes (major error)\n");
            printf("%s", msg);
            send_500(client, clients_ptr, msg);
            return;
        }
        struct vote* temp = seen_vote;

        if (temp->prev = NULL) {
            if (user->votes == temp) {
                user->votes = temp->next;
            }
            temp->next->prev = NULL;
        } else {
            temp->prev->next = temp->next;
            temp->next->prev = temp->prev;
        }

        free(temp);

        email->upvotes -= 1;
    }


}

// Returns email* if found, NULL if not found, -1 if error
// Hashmap filled with seen usernames would be more robust
struct email* search_for_email(struct user* user, char* sender, char* title, int hash) {
    struct email* err_ret = (struct email*) -1;
    if (user == NULL) {
        printf("search_for_email called will NULL user\n"); return err_ret;}
    if (sender == NULL) {
        printf("search_for_email called will NULL sender\n"); return err_ret;}
    if (title == NULL) {
        printf("search_for_email called will NULL title\n"); return err_ret;}
    if (hash == 0) {
        printf("search_for_email called will 0 hash\n"); return err_ret;}

    int count = 0;
    const int max_loops = 10000;
    struct email* email_head = user->emails;
    while (email_head) {
        if (strncmp(email_head->sender, sender, MAX_NAME_BYTES + 1) == 0) {
            if (strncmp(email_head->title, title, MAX_TITLE_BYTES + 1) == 0) {
                if (email_head->hash == hash) {
                    break;
                }
            }
        }
        email_head = email_head->next;
        if (count++ > max_loops) {
            return err_ret;}
    }
    return email_head;
}

// Returns vote* if found, NULL if not found, -1 if error
// Hashmap filled with seen usernames would be more robust
struct vote* search_for_vote(struct user* user, char* sender, char* title, int hash) {
    struct vote* err_ret = (struct vote*) -1;
    if (user == NULL) {
        printf("search_for_vote called will NULL user\n"); return err_ret;}
    if (sender == NULL) {
        printf("search_for_vote called will NULL sender\n"); return err_ret;}
    if (title == NULL) {
        printf("search_for_vote called will NULL title\n"); return err_ret;}
    if (hash == 0) {
        printf("search_for_vote called will 0 hash\n"); return err_ret;}

    int count = 0;
    const int max_loops = 10000;
    struct vote* vote_head = user->votes;
    while (vote_head) {
        if (strncmp(vote_head->sender, sender, MAX_NAME_BYTES + 1) == 0) {
            if (strncmp(vote_head->title, title, MAX_TITLE_BYTES) == 0) {
                if (vote_head->hash == hash) {
                    break;
                }
            }
        }
        vote_head = vote_head->next;
        if (count++ > max_loops) {
            return err_ret;}
    }
    return vote_head;
}


// Returns user* if found, NULL if not found or error
// Hashmap filled with seen usernames would be more robust
struct user* search_for_user(char* username, struct user* users) {
    if (users == NULL) {
        printf("search_for_user called will NULL users\n"); return NULL;}
    struct user* user_head = users;
    int count = 0;
    const int max_loops = 10000;
    while (user_head) {
        if (strncmp(user_head->username, username, MAX_NAME_BYTES + 1) == 0) {
            break;}
        user_head = user_head->next;
        if (count++ > max_loops) {
            return NULL;}
    }
    return user_head;
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
        print_msg_send400_return("First post line malformed\n");}

    // FIND CONTENT LENGTH
    #define contentLengthFail(x) \
        printf("Failed to parse content length. Line %d\n", x); \
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
        send_400(client, clients_ptr, NULL); \
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

    quinter = strstr(pointer, boundary) - 2;
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

    //printf("BEFORE, POINTER = %s\n", pointer);
    //printf("BEFORE, QUINTER = %s\n", quinter);
    
    // Removes whitespace and tolowers so Mike and m ike are the same in the database
    char username[MAX_NAME_BYTES + 1] = {0};
    char error_msg[ERR_MSG_BUF_LEN] = {0};
    memcpy(error_msg, "Username: ", 10);
    sanitize_name(pointer, username, min(quinter - pointer, MAX_NAME_BYTES + 1), error_msg + 10);
    if (strlen(error_msg) > 10) {
        locateContentFail(__LINE__, error_msg);}

    //printf("USERNAME = %s\n", username);
    //printf("POINTER = %s\n", pointer);

    pointer = strstr(quinter, boundary);
    if (pointer == 0) {
        locateContentFail(__LINE__, "");}
    pointer = strstr(pointer, "\r\n");
    if (pointer == 0) {
        locateContentFail(__LINE__, "");}
    pointer += 2;
    // END USERNAME ///////////////////////////////////////////////

    // GET RECIPIENT ////////////////////////////////////////////////
    //printf("pointer = %s\n", pointer);
    if (end - pointer < 48 || strncmp(pointer, "Content-Disposition: form-data; name=\"recipient\"", 48) != 0) {
        locateContentFail(__LINE__, "");}
    pointer += 48;

    if (strncmp(pointer, "\r\n", 2) != 0) {
        locateContentFail(__LINE__, "");}
    pointer += 2;

    quinter = strstr(pointer, boundary) - 2;
    if (quinter - pointer <= 2 || quinter - pointer > MAX_NAME_BYTES + 2) {
        locateContentFail(__LINE__, "");}

    // Should skip through whitespace
    while (*pointer == ' ' || *pointer == '\t' || *pointer == '\n' || *pointer == '\r'){
        pointer++;
        if (pointer >= end) {
            locateContentFail(__LINE__, "");}
    }
    if (quinter - pointer > MAX_NAME_BYTES) {
        locateContentFail(__LINE__, "");}
    //printf("BEFORE, POINTER = %s\n", pointer);
    //printf("BEFORE, QUINTER = %s\n", quinter);
    
    // Removes whitespace and tolowers so Mike and m ike are the same in the database
    char recipient[MAX_NAME_BYTES + 1] = {0};
    memset(error_msg, 0 , ERR_MSG_BUF_LEN);
    memcpy(error_msg, "Username: ", 10);
    sanitize_name(pointer, recipient, min(quinter - pointer, MAX_NAME_BYTES + 1), error_msg + 10);
    if (strlen(error_msg) > 10) {
        locateContentFail(__LINE__, error_msg);}

    //printf("USERNAME = %s\n", username);
    //printf("POINTER = %s\n", pointer);

    pointer = strstr(quinter, boundary);
    if (pointer == 0) {
        locateContentFail(__LINE__, "");}
    pointer = strstr(pointer, "\r\n");
    if (pointer == 0) {
        locateContentFail(__LINE__, "");}
    pointer += 2;

    // Check if recipient is sender (username)
    bool recip_equals_sender = false;
    if (strlen(recipient) == strlen(username)) {
        recip_equals_sender = true;
        for (int i = 0; i < strlen(recipient); i++) {
            if (recipient[i] != username[i]) {
                recip_equals_sender = false;
                break;
            }
        }
    }
    // END RECIPIENT ////////////////////////////

    // GET TITLE ////////////////////////////////////////////////
    if (strncmp(pointer, "Content-Disposition: form-data; name=\"title\"", 44) != 0) {
        locateContentFail(__LINE__, "");}
    pointer += 44;

    if (strncmp(pointer, "\r\n", 2) != 0) {
        locateContentFail(__LINE__, "");}
    pointer += 2;

    quinter = strstr(pointer, boundary) - 2;
    if (quinter - pointer <= 2 || quinter - pointer > MAX_TITLE_BYTES) {
        locateContentFail(__LINE__, "");}

    // Should skip through whitespace
    while (*pointer == ' ' || *pointer == '\t' || *pointer == '\n' || *pointer == '\r') {
        pointer++;
        if (pointer >= end) {
            locateContentFail(__LINE__, "");}
    }

    char title[MAX_TITLE_BYTES + 1] = {0};
    memset(error_msg, 0 , ERR_MSG_BUF_LEN);
    sanitize_title(pointer, title, min(quinter - pointer, MAX_TITLE_BYTES + 1), error_msg);
    if (strlen(error_msg) > 0) {
        locateContentFail(__LINE__, error_msg);}


    pointer = strstr(quinter, boundary);
    if (pointer == 0) {
        locateContentFail(__LINE__, "");}
    pointer = strstr(pointer, "\r\n");
    if (pointer == 0) {
        locateContentFail(__LINE__, "");}
    pointer += 2;
    // END TITLE ////////////////////////////

    // GET BODY /////////////////////////////
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
    int whiteSpaceCount = 0;
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
        print_msg_send400_return("Received empty text area\n");}
    body[quinter - pointer - whiteSpaceCount] = 0;

    if (strlen(body) == 0) {
        locateContentFail(__LINE__, "Empty text area");}

    int kosher_chars = num_kosher_chars(body, 1);
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
            continue;}
        username[i] = tolower(username[i]);
    }
    printf("username = %s\n", username);

    pointer = recipient;
    for (int i = 0; i < strlen(recipient); i++) {
        // Skip tolower if unicode
        if (recipient[i] >> 7 & 1 == 1) {
            continue;}
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

    send_201(client, clients_ptr);

    drop_client(client, clients_ptr);
}
#if PRINT_POST_PARSE == false
#undef printf
#endif

int min(int a, int b) {
    if (a > b) {
        return b;}
    return a;
}


// Must have null terminating char, buffer should be able to contain name
void sanitize_name(char* name, char* buffer, int buffer_size, char* error_msg) {
    int length = strlen(name);
    int j = 0;
    for (int i = 0; i < length; i++) {
        if (name[i] == ' ' || name[i]  == '\t' || name[i]  == '\n' || name[i]  == '\r') {
            continue;}
        if (j == buffer_size) {
            break;}
        buffer[j++] = name[i];
    }

    if (strlen(buffer) == 0) {
        error_msg = "Empty name\n"; return;}

    int kosher_chars;
    kosher_chars = num_kosher_chars(buffer, 0);
    if (kosher_chars <= 0) {
        error_msg = "Name is not kosher\n"; return;}

    if (kosher_chars > MAX_NAME_CHARACTERS) {
        error_msg = "Name contained too many characters\n"; return;}

    if (contains_profanity(buffer)) {
        error_msg = "Name has swears );\n"; return;}
}

void sanitize_title(char* title, char* buffer, int buffer_size, char* error_msg) {
    // Skip beggining whitespace
    int i = 0;
    while (i < strlen(title) &&
          (title[i] == ' ' || title[i]  == '\t' || title[i]  == '\n' || title[i]  == '\r')) {
        i++;
    }

    for (int j = 0; j < buffer_size && j < strlen(title); j++) {
        buffer[j] = title[i++];
    }

    // Remove whitespace from end
    i = strlen(buffer);
    while (i >= 0 &&
          (buffer[i] == ' ' || buffer[i]  == '\t' || buffer[i]  == '\n' || buffer[i]  == '\r')) {
        buffer[i] = 0;
    }

    if (strlen(title) == 0) {
        error_msg = "Received empty title\n"; return;}

    int kosher_chars = num_kosher_chars(title, 0);
    if (kosher_chars <= 0) {
        error_msg = "Title is not kosher\n"; return;}

    if (kosher_chars > MAX_TITLE_CHARACTERS) {
        error_msg = "Title contained too many characters\n"; return;}

    if (contains_profanity(title)) {
        error_msg = "Title has swears );\n"; return;}
}