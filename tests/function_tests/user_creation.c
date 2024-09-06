
#include "../../resource_handlers.h"

const char* DELIMITER = "''\n";
int min_size_profanity = 999;
int max_size_profanity = -1;
char* profanity_list[128] = {0};
int profanity_hash_list[128] = {0};

int main() {

    init_users();

}