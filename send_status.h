
#ifndef SEND_STATUS_HEADER
#define SEND_STATUS_HEADER

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

#include "structs_and_macros.h"

void send_200(struct client_info* client, unsigned long content_length, const char* content_type);
void send_201(struct client_info* client, struct client_info** clients_ptr);
void send_301(struct client_info* client, struct client_info** clients_ptr);
void send_400(struct client_info* client, struct client_info** clients_ptr, char* msg);
void send_404(struct client_info* client, struct client_info** clients_ptr);
void send_500(struct client_info* client, struct client_info** clients_ptr, char* msg);
void send_507(struct client_info* client, struct client_info** clients_ptr);

#endif