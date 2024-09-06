#ifndef RESOURCE_HANDLERS_HEADER
#define RESOURCE_HANDLERS_HEADER

#include "structs_and_macros.h"

static void user_data_search(char** pointer_ptr, char** quinter_ptr, char* search, char* buffer, int buffer_size);
void init_users();
const char* get_content_type(const char* path);
void serve_resource(struct client_info* client, struct client_info** clients_ptr, const char* path);
int num_kosher_chars(char* string, int type);
void handle_put(struct client_info* client, struct client_info** clients_ptr);
void handle_post(struct client_info* client, struct client_info** clients_ptr);

#endif