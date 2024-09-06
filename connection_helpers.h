
#ifndef CONNECTION_HELPERS_HEADER
#define CONNECTION_HELPERS_HEADER

#include <sys/select.h>

#include "structs_and_macros.h"

int create_socket(const char* host, const char* port);
struct client_info* get_client(int socket, struct client_info** clients_ptr);
void drop_client(struct client_info* client, struct client_info** clients_ptr);
const char* get_client_address(struct client_info** client_ptr);
fd_set wait_on_clients(int server, struct client_info** client_ptr);

#endif