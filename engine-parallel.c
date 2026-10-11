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
    char** instances;
};


void *count_worker(void *arg);
void *instance_worker(void *arg);
static int seek_to_chunk(FILE *file, long start);

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
    struct worker_args thread1_args = {filename, target, 0, split, 0, NULL};
    struct worker_args thread2_args = {filename, target, split, size, 0, NULL};

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
    fprintf(stderr, "t1=%d t2=%d\n", thread1_args.count, thread2_args.count);
    int count = thread1_args.count + thread2_args.count;
    fprintf("Total count: %d", count);
    return count;
}

void *count_worker(void *arg) {
    char line[2048] = "";
    struct worker_args* arguments = arg;
    arguments->count = 0;

    // Open file to read
    FILE* file = fopen(arguments->filename, "r");
    if (file == NULL){printf("Error opening file"); return NULL;}
    if (fseek(file, arguments->start, SEEK_SET) != 0){
        fclose(file);
        printf("Seek failed");
        return NULL;
    }

    // Catch thread 2 starting in the middle of a line
    if (arguments->start != 0) {
        fseek(file, arguments->start - 1, SEEK_SET);
        fgets(line, sizeof(line), file);
    }

    // Length of target word
    size_t length = strlen(arguments->target);

    // Read each line of the file within the start/end positions
    while(ftell(file) < arguments->end && fgets(line, sizeof(line), file)){
        for (char* p = line; (p = strstr(p, arguments->target)); p+= length)
        arguments->count++;
    }
    fclose(file);
    return NULL;
}



struct count_result search_instance(char *filename,char *target){
    int count = search_count(filename, target);
    if(count < 0){
        return (struct count_result) {-1, NULL};
    }

    // Determine the file size
    FILE* file = fopen(filename, "rb"); // Binary read mode needed here
    if (file == NULL){printf("Error opening file"); return (struct count_result){-1, NULL};}
    if (fseek(file, 0L, SEEK_END) != 0){
        fclose(file);
        printf("Seek failed");
        return (struct count_result){-1, NULL};
    }
    // Read byte position
    long size = ftell(file);
    if (size == -1){printf("Error executing ftell on file"); fclose(file); return (struct count_result){-1, NULL};}
    // Find a middle point
    long split = size / 2;
    
    // Close the file
    fclose(file);

    // Declare threads
    pthread_t thread1;
    pthread_t thread2;

    // Define worker arguments struct for each thread
    struct worker_args thread1_args = {filename, target, 0, split, count, NULL}; // Temporarily passing total count to each thread so I malloc enough space later.
    struct worker_args thread2_args = {filename, target, split, size, count, NULL};

    // Create threads and link to helper fuction
    if(pthread_create(&thread1, NULL, instance_worker, (void*) &thread1_args) != 0){
        printf("Error creating thread 1");
        return (struct count_result){-1, NULL};
    }
    if(pthread_create(&thread2, NULL, instance_worker, (void*) &thread2_args) != 0){
        printf("Error creating thread 2");
        return (struct count_result){-1, NULL};
    }

    // Join threads
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
    
    // Join count values
    int total = thread1_args.count + thread2_args.count;
    
    // Join instance arrays
    char** instances = malloc((total ? total : 1) * sizeof(char*));
    for(int i = 0; i < thread1_args.count; i++){
        instances[i] = thread1_args.instances[i];
    }
    for(int i = 0; i < thread2_args.count; i++){
        instances[thread1_args.count + i] = thread2_args.instances[i];
    }


    // Free the worker function mallocs
    free(thread1_args.instances);
    free(thread2_args.instances);

    return (struct count_result){total, instances};
}

void *instance_worker(void *arg) {
    char line[2048] = "";
    struct worker_args* arguments = (struct worker_args*) (arg);
    
    // Store capacity and resent count
    int capacity = arguments->count;
    arguments->count = 0;
    arguments->instances = NULL;
    // Return null array if no instances were found
    if(capacity <= 0) return NULL;

    // malloc space for the array of char pointers
    arguments->instances = malloc(capacity * sizeof(char*));
    if(!arguments->instances) return NULL;

    // Open file to read
    FILE* file = fopen(arguments->filename, "r");
    if (!file) return NULL;
    if (seek_to_chunk(file, arguments->start) != 0){fclose(file); return NULL;}

    // Length of target word
    size_t length = strlen(arguments->target);
    
    // Read each line of the file within the start/end positions
    while(ftell(file) < arguments->end && fgets(line, sizeof(line), file)){
        // Replace \n with \0
        line[strcspn(line, "\n")] = '\0';

        for(char* p = line; (p = strstr(p, arguments->target)); p += length){
            if(arguments->count >= capacity) break;
            arguments->instances[arguments->count++] = strdup(line);
        }
    }
    fclose(file);
    return NULL;
}


static int seek_to_chunk(FILE *file, long start) {
    if (start == 0) return fseek(file, 0, SEEK_SET);
    if (fseek(file, start - 1, SEEK_SET) != 0) return -1;
    char skip[2048];
    fgets(skip, sizeof(skip), file);   // discard rest of the line we're inside
    return 0;
}