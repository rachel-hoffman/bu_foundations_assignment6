#include "engine.h"
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/stat.h>
#include <ctype.h>

#define MAX_LINE_LENGTH 256

struct worker_args {
    char *filename;
    char *target;

    long start; //start of the chunk to process
    long end; //end of the chunk to process

    int count;
};

void *count_worker(void *arg);
void *instance_worker(void *arg);

int search_count(char *filename, char *target) {
    // Determine the file size
    FILE* file = fopen(filename, "rb"); // Binary read mode needed here
    if (file == NULL) return -1;
    if (fseek(file, 0L, SEEK_END) != 0){
        fclose(file);
        printf("Seek failed");
        return -1;
    }
    // Read byte position
    long size = ftell(file);
    if (size == -1){printf("Error executing ftell on file"); return -1;};
    // Find a middle point
    long split = size / 2;
    // Close the file
    fclose(file);

    // Declare threads
    pthread_t thread1;
    pthread_t thread2;

    // Define worker arguments struct for each thread
    struct worker_args thread1_args = {filename, target, 0, split, 0};
    struct worker_args thread2_args = {filename, target, split, size, 0};

    // Create threads and link to helper fuction
    if(pthread_create(&thread1, NULL, count_worker, (void*) &thread1_args) != 0){
        printf("Error creating thread 1");
        return -1;
    }
    if(pthread_create(&thread2, NULL, count_worker, (void*) &thread2_args) != 0){
        printf("Error creating thread 2");
        return -1;
    }

    // Join threads
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
    // Final count
    int count = thread1_args.count + thread2_args.count;

    return count;
}

void *count_worker(void *arg) {
    char line[2048] = "";
    struct worker_args* arguments = (struct worker_args*) (arg);

    // Open file to read
    FILE* file = fopen(arguments->filename, "r");
    if (file == NULL){printf("Error opening file"); return NULL;}
    if (fseek(file, arguments->start, SEEK_SET) != 0){
        fclose(file);
        printf("Seek failed");
        return NULL;
    }

    // Read each line of the file within the start/end positions
    while(ftell(file) >= arguments->start && ftell(file) < arguments->end && fgets(line, sizeof(line), file) != NULL){
        // Set pointer to start of the line
        char* position = line;
        // Search for target word
        while((position = strstr(position, arguments->target)) != NULL){
            // We found an instance
            // Add one to the count
            arguments->count += 1;
            // Move poiner past the found instance to keep looking
            position += strlen(arguments->target);
        }
    }

    fclose(file);
    return NULL;
}



struct count_result search_instance(char *filename,char *target){
    // 
}

void *instance_worker(void *arg) {
    struct worker_args *args = arg;
    return NULL;
}


