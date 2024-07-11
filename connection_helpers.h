
#ifndef CONNECTION_HELPERS_HEADER
#define CONNECTION_HELPERS_HEADER

#include <sys/select.h>

#include "structs_and_macros.h"

int create_socket(const char* host, const char* port);
struct client_info* get_client(int socket, struct client_info** clientsPtr);
void drop_client(struct client_info* client, struct client_info** clientsPtr);
const char* get_client_address(struct client_info** clientPtr);
fd_set wait_on_clients(int server, struct client_info** clientsPtr);

#endif