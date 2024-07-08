
#ifndef CONNECTION_HELPERS_HEADER
#define CONNECTION_HELPERS_HEADER

int create_socket(const char* host, const char* port);
struct client_info* get_client(int socket);
void drop_client(struct client_info* client);
const char* get_client_address(struct client_info* ci);
fd_set wait_on_clients(int server);

#endif