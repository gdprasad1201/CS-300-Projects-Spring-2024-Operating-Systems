#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "common.h"
#include "common_threads.h"

int max;
volatile int counter = 0; // shared global variable
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER; // declare and initialize the mutex lock

void* mythread(void* arg) {
    char* letter = arg;
    int i; // stack (private per thread) 

    printf("%s: begin [addr of i: %p,] [addr of counter: %p]\n", letter, &i,(unsigned int) &counter);

    for (i = 0; i < max * 1000; i++) {
        Pthread_mutex_lock(&lock); // lock the mutex to protect the shared global variable counter 
        counter++; // shared: only one
        Pthread_mutex_unlock(&lock); // unlock the mutex
    }

    printf("%s: done\n", letter);

    return NULL;
}

int main(int argc, char** argv) {  
    pthread_t p1, p2;
    max = atoi(argv[1]);
    
    if (argc != 2) {
        fprintf(stderr, "usage: main-first <loopcount>\n");
        exit(1);
    }

    

    printf("main: begin [counter = %d] [%x]\n", counter, (unsigned int) &counter);

    Pthread_create(&p1, NULL, mythread, "A");
    Pthread_create(&p2, NULL, mythread, "B");

    // join waits for the threads to finish
    Pthread_join(p1, NULL);
    Pthread_join(p2, NULL);

    printf("main: done\n [counter: %d]\n [should: %d]\n", counter, max * 2 * 1000);

    return 0;
}