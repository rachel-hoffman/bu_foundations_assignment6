#include "engine.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, char** argv) {
    // TODO: parse the arguments in argv.
    // You can expect argv[1] to be the mode
    // You can expect argv[2] to be the filepath
    // You can expect argv[3] to be the target word

    if (argc != 4) {
        printf(stderr,
                "Usage: %s <count|instance> <input_file> <target_word>\n",
                argv[0]);
        return 1;
    }

    // Parsing the arguments
    char* mode = argv[1];
    char* file = argv[2];
    char* word = argv[3];

    // Checking given mode
    if(strcmp(mode, "count") == 0){
        // Count mode
        struct count_result result = search_instance(file, word);
        printf("Found: %d of %s in %s\n", result.count, word, file);
    }
    else if(strcmp(mode, "instance") == 0){
        // Instance mode
        struct count_result result = search_instance(file, word);
        printf("Found: %d of %s in %s\n", result.count, word, file);
        for(int i=0; i < result.count; i++){
            printf("res.instances[%d]: %s\n", i, result.instances[i]);
            free(result.instances[i]);
        }
        free(result.instances);
    }
    else{
        printf("Error: 'count' or 'instance' expected for the mode argument, but %s was given.", mode);
        return -1;
    }

    
    return 0;
}