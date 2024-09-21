#ifndef RESOURCE_HANDLERS_HEADER
#define RESOURCE_HANDLERS_HEADER

#include "structs_and_macros.h"

struct email {
    char sender[MAX_NAME_BYTES + 1];
    char title[MAX_TITLE_BYTES + 1];
    int hash;
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
    struct vote* prev;
};

struct user {
    char username[MAX_NAME_BYTES + 1];
    struct email* emails;
    struct vote* votes;

    struct user* next;
};

static void user_data_search(char** pointer_ptr, char** quinter_ptr, char* search, char* buffer, int buffer_size);
struct user* init_users();
void save_users_to_file(struct user* users);
void print_user(struct user* user);
void print_users(struct user* users);

const char* get_content_type(const char* path);
void serve_resource(struct client_info* client, struct client_info** clients_ptr, struct user* users, char* path);
int num_kosher_chars(char* string, int type);
void handle_put(struct client_info* client, struct client_info** clients_ptr, struct user* users);
struct email* search_for_email(struct user* user, char* sender, char* title, int hash);
struct user* search_for_user(char* username, struct user* users);
struct vote* search_for_vote(struct user* user, char* sender, char* title, int hash);
void handle_post(struct client_info* client, struct client_info** clients_ptr, struct user** users_ptr);
int min(int a, int b);
void sanitize_name(char* name, char* buffer, int buffer_size, char* error_msg);
void sanitize_title(char* title, char* buffer, int buffer_size, char* error_msg);


#endif