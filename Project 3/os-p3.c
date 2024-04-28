#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "common.h"
#include "common_threads.h"

int max;
volatile int counter = 0; // shared global variable

void *mythread(void *arg) {
    char *letter = arg;
    int i; // stack (private per thread) 

    printf("%s: begin [addr of i: %p,] [addr of counter: %p]\n", letter, &i,(unsigned int) &counter);

    for (i = 0; i < max * 1000; i++) {
	    counter = counter + 1; // shared: only one
    }

    printf("%s: done\n", letter);

    return NULL;
}
                                                                             
int main(int argc, char *argv[]) {                    
    if (argc != 2) {
	    fprintf(stderr, "usage: main-first <loopcount>\n");
	    exit(1);
    }

    max = atoi(argv[1]);

    pthread_t p1, p2;
    printf("main: begin [counter = %d] [%x]\n", counter, (unsigned int) &counter);
    Pthread_create(&p1, NULL, mythread, "A"); 
    Pthread_create(&p2, NULL, mythread, "B");
    
    // join waits for the threads to finish
    Pthread_join(p1, NULL); 
    Pthread_join(p2, NULL); 
    printf("main: done\n [counter: %d]\n [should: %d]\n", counter, max * 2 * 1000);
    return 0;
}

// Explain why the output is different from the original program.
/*
The output is different from the original program because the original program uses a mutex lock to protect the shared global variable counter. In the modified program, the shared global variable counter is 
not protected by a mutex lock. This means that the threads can access and modify the counter variable concurrently without any synchronization. As a result, the threads can interfere with each other and the
final value of the counter variable is not guaranteed to be equal to max * 2 * 1000. This is why the output is different from the original program.
*/