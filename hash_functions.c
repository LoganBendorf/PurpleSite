
#include "hash_functions.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>


// Declared in https_server.c
extern int min_size_profanity;
extern int max_size_profanity;

extern char* profanity_list[128];
extern int profanity_hash_list[128];


#if PRINT_HASH == false
#define printf(...) 
#endif
// should only put kosher strings into this function
int hash_function(char* string) {
    
    unsigned char copy_string[1024] = {0};
    strcpy(copy_string, string);
    for (int i = 0; i < strlen(string); i++) {
        copy_string[i] = tolower(copy_string[i]);
    }
    printf("hash_function() received: %s\n", copy_string);
    
    int hash = 5381;
    for (int i = 0; i < strlen(string); i++) {

        printf("hash = %d\n", hash);
        // if emoji
        if (copy_string[i] >> 7 == 1) {
            printf("hash_function() detected emoji\n");
            hash = ((hash << 5) + hash) + 'o';
            i += 3;
            continue;
        }
        bool shouldSkip = false;
        switch(copy_string[i]) {
            case ' ': shouldSkip = true; break;
            case '_': shouldSkip = true; break;
            case '(': if (i + 1 < strlen(string) && copy_string[i+1] == ')') {
                hash = ((hash << 5) + hash) + 'o';
                i++;
                shouldSkip = true;
                } break;
            case '0': copy_string[i] = 'o'; break;
            case '@': copy_string[i] = 'a'; break;
            case 'i': copy_string[i] = 'l'; break;
            case '|': copy_string[i] = 'l'; break;
            case '!': copy_string[i] = 'l'; break;
            case '3': copy_string[i] = 'e'; break;
            case '$': copy_string[i] = 's'; break;
            default: break;
        }
        if (shouldSkip) {
            continue;
        }
        hash = ((hash << 5) + hash) + copy_string[i];
    }
    if (hash == 0) {
        printf("hash equaled 0, incrementing it by 1\n");
        return 1;
    }
    return hash;
}
#if PRINT_HASH == false
#undef printf
#endif

#if PROFANITY_PRINT == false
#define printf
#endif
// profanity is seperated by newline, if not it will break
void hash_profanity_list() {
    printf("\nPRINTING PROFANITY\n");
    FILE* file = fopen("profanity.txt", "r");
    int i = 0;
    char line[64] = {0};
    while (fgets(line, 64, file)) {
        if (i == 127) { // size of profanity_ist is 128, last slot need to be 0
            printf("too much profanity\n");
            exit(1);
        }

        profanity_list[i] = calloc(1, strlen(line)-1);
        strncpy(profanity_list[i], line, strlen(line)-1);
        int size = strlen(line) - 1;
        profanity_list[i][size] = 0;
        printf("size = %d\n", size);
        //printf("max_size_profanity = %d\n", max_size_profanity);

        if (size < min_size_profanity) {
            min_size_profanity = size;
            printf("min_size_profanity changed to %d\n", size);
        }
        if (size > max_size_profanity) {
            max_size_profanity = size;
            printf("max_size_profanity changed to %d\n", size);
        }
        printf("added %s to profanity list\n", profanity_list[i]);
        i++;
    }
    fclose(file);

    i = 0;
    while (profanity_list[i] != 0) {
        profanity_hash_list[i] = hash_function(profanity_list[i]);
        printf("hashed %s as %d\n", profanity_list[i], profanity_hash_list[i]);
        i++;
    }
    printf("DONE ADDING PROFANITY\n\n");
}

// should only give string with length less than 1024, last byte is for 0
// ONCE FOUND HASHES THAT MATCH CHECK TO MAKE SURE IT ACTUALLY IS PROFANITY
bool contains_profanity(char* string) {
    printf("contain_profanity() called with string (%s)\n", string);
    char bad_msg_buffer[1024] = {0};
    strcpy(bad_msg_buffer, string);
    static char temp[1024] = {0};
    int pointer;
    int quinter;
    printf("min_size_profanity = %d\n", min_size_profanity);
    printf("max_size_profanity = %d\n", max_size_profanity);
    for (int i = min_size_profanity; i <= max_size_profanity + 1; i++) {
        //printf("in for loop\n");
        pointer = 0;
        quinter = i;
        while (quinter <= strlen(bad_msg_buffer)) {
            //printf("in while loop\n");

            strcpy(temp, bad_msg_buffer);
            temp[quinter + 1] = 0;
            int hash = hash_function(temp + pointer);
            printf("hash = %d\n", hash);
            
            int j = 0;
            while (profanity_hash_list[j] != 0) {
                if (hash == profanity_hash_list[j]) {
                    printf("hash = %d: profanity hash = %d\n", hash, profanity_hash_list[j]);
                    printf("profanity found = %s\n", profanity_list[j]);
                    return true;
                }
                j++;
            }
            pointer++;
            quinter++;
        }
    }
    return false;
}

#if PROFANITY_PRINT == false
#undef printf
#endif