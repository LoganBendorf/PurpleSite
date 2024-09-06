
#ifndef SEND_STATUS_HEADER
#define SEND_STATUS_HEADER

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

#include "structs_and_macros.h"

void send_200(struct client_info* client, struct client_info** clients_ptr);
void send_301(struct client_info* client, struct client_info** clients_ptr);
void send_400(struct client_info* client, struct client_info** clients_ptr, char* string);
void send_404(struct client_info* client, struct client_info** clients_ptr);

#endif