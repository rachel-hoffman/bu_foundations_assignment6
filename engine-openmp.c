#include "engine.h"
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/stat.h>
#include <omp.h>
#include <ctype.h>

// Notes on the OMP library:
//
// No thread handling needed; OMP automatically creates and joins the threads.
// Need the '-fopenmp' flag to compile.
// #pragma omp parallel {} runs the code in braces once on every thread in a team.
// omp_get_thread_num() returns the thread's ID.
// omp_get_num_threads() returns the team size.
// reduction(+:total) will total individual totals of each thread into a new variable. Use to avoid data races.

#define MAX_LINE_LENGTH 256
#define NUM_CHUNKS 4

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

    // Define worker arguments struct for each thread
    struct worker_args args[NUM_CHUNKS];
    
    for(int i = 0; i < NUM_CHUNKS; i++) {
        args[i] = (struct worker_args){
            filename, target,
            size*i / NUM_CHUNKS,
            size*(i + 1) / NUM_CHUNKS,
            count,
            NULL
        };
    }

    // Use OMP to parallelize
    #pragma omp parallel for num_threads(4)
    for (int i = 0; i < NUM_CHUNKS; i++) {
        count_worker(&args[i]);
    }
    
    // Calculate the final count
    int total = 0;
    for(int i = 0; i < NUM_CHUNKS; i++){
        total += args[i].count;
    }
    
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

    // Define worker arguments struct for each thread
    struct worker_args args[NUM_CHUNKS];
    
    for(int i = 0; i < NUM_CHUNKS; i++) {
        args[i] = (struct worker_args){
            filename, target,
            size*i / NUM_CHUNKS,
            size*(i + 1) / NUM_CHUNKS,
            count,
            NULL
        };
    }

    // Use OMP to parallelize
    #pragma omp parallel for num_threads(4)
    for (int i = 0; i < 4; i++) {
        instance_worker(&args[i]);
    }
    
    // Calculate the final count
    int total = 0;
    for(int i = 0; i < NUM_CHUNKS; i++){
        total += args[i].count;
    }
    
    // Join instance arrays
    char** instances = malloc((total ? total : 1) * sizeof(char*));
    // For each thread
    int i = 0;
    for(int j = 0; j < NUM_CHUNKS; j++){
        // For each instance
        for(int k = 0; k < NUM_CHUNKS; k++){
            instances[i++] = args[j].instances[k];
        }
        // Free the worker function mallocs for each thread
        free(args[j].instances);
    }

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