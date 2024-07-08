
#ifndef SEND_STATUS_HEADER
#define SEND_STATUS_HEADER

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

#include "structs_and_macros.h"

void send_301(struct client_info* client);
void send_400(struct client_info* client, char* string);
void send_404(struct client_info* client);

#endif