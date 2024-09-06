
#ifndef HASH_FUNCTIONS_HEADER
#define HASH_FUNCTIONS_HEADER

#include <stdbool.h>
#include "structs_and_macros.h"

int hash_function(char* string);
void hash_profanity_list();
bool contains_profanity(char* string);

#endif