#include "cachelab.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <getopt.h>
#include <stdbool.h>
#include <limits.h>
#include <errno.h>

/* no need to define block */
typedef struct{
    bool valid;
    uint64_t tag;
    uint64_t counter_time; /* for LRU */
} line;

typedef line *start_of_lines;
typedef start_of_lines *start_of_sets;

typedef struct{
    int hit;
    int miss;
    int eviction;
} res;

start_of_sets cache_initialization(uint64_t S, uint64_t E){
    start_of_sets cache;

    if ((cache = calloc(S, sizeof(start_of_lines))) == NULL ){
        perror("calloc sets fail");
		exit(-1);
    }
    for (uint64_t i=0; i<S; i++){
        if ((cache[i] = calloc(E, sizeof(line))) == NULL){
            perror("Failed to calloc line in sets");
        }
    }
    return cache;

}

void release(start_of_sets cache, uint64_t S, uint64_t E){
    for (uint64_t i=0; i<S; i++){
        free(cache[i]);
    }
    free(cache);
}

res count_hit_miss_eviction(start_of_lines start_line, res result, uint64_t E, uint64_t tag, bool verbose){
    uint64_t oldest = UINT64_MAX;
    uint64_t oldest_block = UINT64_MAX;
    uint64_t youngest = 0;
    bool flag = false;

    for (uint64_t i=0; i<E; i++){
        if (start_line[i].tag == tag && start_line[i].valid){
            if (verbose) printf("hit\n");
            flag = true;
            result.hit++;
            start_line[i].counter_time++;
            break;
        }

    }
    if (!flag){
        if (verbose) printf("miss");
        result.miss++;
        for (uint64_t i=0; i<E; i++){
            if (start_line[i].counter_time < oldest){
                oldest = start_line[i].counter_time;
                oldest_block = i;
            }
            if (start_line[i].counter_time > youngest){
                youngest = start_line[i].counter_time;
            }
        }
        start_line[oldest_block].counter_time = youngest+1;
        start_line[oldest_block].tag = tag;
        if (start_line[oldest_block].valid){
            if (verbose)  printf(" and eviction\n");
            result.eviction++;
        }else{
            if (verbose) printf("\n");
            start_line[oldest_block].valid = true;
        }

    }
    return result;

}

res test(FILE *tracefile, start_of_sets cache, uint64_t S, uint64_t E, uint64_t s, uint64_t b, bool verbose){
    res result = {0,0,0};
    char ch;
    uint64_t address;
    while(fscanf(tracefile, " %c %lx%*[^\n]", &ch, &address) == 2){
        if (ch == 'I'){
            continue;
        }else{
            uint64_t tag = (address>>b) >> s;
            uint64_t set_index_indicator = (1<<s)-1;
            uint64_t set_index = (address>>b) & set_index_indicator;
            start_of_lines start_line = cache[set_index];

            if (ch == 'L' || ch == 'S'){
                if (verbose) printf("%c %lx ", ch, address);
                result = count_hit_miss_eviction(start_line, result, E, tag, verbose);
            }else if(ch == 'M'){
                if (verbose) printf("%c %lx ", ch, address);
                result = count_hit_miss_eviction(start_line, result, E, tag, verbose);
                result = count_hit_miss_eviction(start_line, result, E, tag, verbose);
            }else{
                continue;
            }
        }
    }

    return result;
}




int main(int argc, char* argv[])
{
    res result = {0,0,0};
    start_of_sets cache = NULL;
    bool verbose = false;
    uint64_t s = 0;
    uint64_t b = 0;
    uint64_t S = 0;
    uint64_t E = 0;
    const char *help_message = "help";
    const char *command_options = "hvs:E:b:t:";
    FILE* tracefile = NULL;
    char ch;
    while ((ch = getopt(argc, argv, command_options)) != -1){
        switch(ch){
            case 'h':{
                printf("%s", help_message);
                exit(EXIT_SUCCESS);
            }
            case 'v':{
                verbose = true;
                break;
            }
            case 's':{
                if (atol(optarg) <= 0){
                    printf("%s", help_message);
                    exit(EXIT_FAILURE);
                }
                s = atol(optarg);
                S = 1<<s;
                break;
            }

            case 'E':{
                if (atol(optarg) <= 0){
                    printf("%s", help_message);
                    exit(EXIT_FAILURE);
                }
                E = atol(optarg);
                break;
            }

            case 'b':{
                if (atol(optarg) <= 0){
                        printf("%s", help_message);
                        exit(EXIT_FAILURE);
                    }
                    b = atol(optarg);
                    break;
            }

            case 't':{
                if ((tracefile = fopen(optarg, "r")) == NULL){
                        printf("%s", help_message);
                        exit(EXIT_FAILURE);
                }
                break;
            }
            default:{
                printf("%s", help_message);
                exit(EXIT_FAILURE);
            }

        }
    }
    cache = cache_initialization(S,E);
    result = test(tracefile, cache, S, E, s, b, verbose);
    release(cache, S, E);
    printSummary(result.hit, result.miss, result.eviction);
    return 0;
}
