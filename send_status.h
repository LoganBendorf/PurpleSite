
#ifndef SEND_STATUS_HEADER
#define SEND_STATUS_HEADER

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

#include "structs_and_macros.h"

void send_301(struct client_info* client, struct client_info** clientsPtr);
void send_400(struct client_info* client, struct client_info** clientsPtr, char* string);
void send_404(struct client_info* client, struct client_info** clientsPtr);

#endif