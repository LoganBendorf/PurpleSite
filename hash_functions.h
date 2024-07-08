
#ifndef HASH_FUNCTIONS_HEADER
#define HASH_FUNCTIONS_HEADER

#include <stdbool.h>
#include "structs_and_macros.h"

int hash_function(char* string);
void hash_profanity_list(int profanity_hash_list[], char* profanity_list[], int* max_size_profanity, int* min_size_profanity);
bool contains_profanity(char* string, int profanity_hash_list[], char* profanity_list[], int* max_size_profanity, int* min_size_profanity);

#endif