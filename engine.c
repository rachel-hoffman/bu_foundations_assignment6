#include "engine.h"
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <ctype.h>

#define MAX_LINE_LENGTH 256

// Returns the integer count of all occurrences of the target word in the file
int search_count(char *filename, char *target) {
    int count = 0;
    char line[2048] = "";

    // Open file to read
    FILE* file = fopen(filename, "r");
    if (file == NULL) return -1;

    // Read each line of the file  
    while(fgets(line, sizeof(line), file) != NULL){
        // Set pointer to start of the line
        char* position = line;
        // Search for target word
        while((position = strstr(position, target)) != NULL){
            // We found an instance
            // Add one to the count
            count++;
            // Move poiner past the found instance to keep looking
            position += strlen(target);
        }
    }

    fclose(file);
    return count;
}

// Returns the count and an array of string "instances" of the word. The string will be the string line where the word was found
struct count_result search_instance(char *filename,char *target){
    int count = search_count(filename, target);
    char line[2048] = "";
    // malloc space for the array of char pointers
    char** instances = malloc(count * sizeof(char*));
    
    // Return null array if no instances were found
    if(count == 0) return (struct count_result){0, NULL};

    // Open file to read
    FILE* file = fopen(filename, "r");
    
    // Set index
    int i = 0;

    // Read each line of the file  
    while(fgets(line, sizeof(line), file) != NULL){
        // Set pointer to start of the line
        char* position = line;

        // Search for target word
        while((position = strstr(position, target)) != NULL){
            // We found an instance
            // malloc space for the line
            instances[i] = malloc(strlen(line) + 1); //+1 for null terminator
            // Remove the \n character (debugging for test 2)
            line[strcspn(line, "\n")] = '\0';
            // Add this line to the instances array
            strcpy(instances[i], line);
            
            // Move poiner past the found instance to keep looking
            position += strlen(target);
            // Increment the index
            i++;
        }
    }
    
    fclose(file);
    return (struct count_result){count, instances};
}