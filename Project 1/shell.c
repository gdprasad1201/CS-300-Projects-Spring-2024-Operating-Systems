/**
 * @file shell.c
 * @brief This program implements a shell in C. The shell should operate in this basic way: when you type in a command (in response to its prompt), the shell creates a child process that executes the command you entered and then prompts for more user input when it has finished. 
 * @version 0.1
 * @date 2024-01-25
 * 
 * @copyright Copyright (c) 2024
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>

#define MAX_LINE 80 /*The maximum length command*/

int main() {
    char** args = malloc(sizeof(char*) * (MAX_LINE / 2 + 1)); /*command line arguments*/
    int should_run = 1; /*flag to determine when to exit program*/
    const char delim[] = " \t\r\n\v\f"; // delimiter to remove all spaces and extraneous chars
    bool ampersand = false;

    while (should_run) {
        printf("osh-%d> ", getpid());
        fflush(stdout);

        char input[MAX_LINE];
        fgets(input, MAX_LINE, stdin);

        // Tokenize the input
        char* token = strtok(input, delim);
        int i = 0;
        while (token != NULL) {
            args[i] = token;
            token = strtok(NULL, delim);
            i++;
        }
        args[i] = NULL;

        if (args[0] == NULL) {
            continue;
        }

        // Check if the user wants to exit
        if (strcmp(args[0], "exit") == 0) {
            should_run = 0;
            break;
        }

        for (int j = 0; j < i; j++) {
            if (strcmp(args[j], "&") == 0) {
                ampersand = true;
                break;
            }
        }

        pid_t pid = fork();
        if (pid < 0) {
            fprintf(stderr, "Fork failed\n");
            return 1;
        } 
        else if (pid == 0) {
            // Child process
            execvp(args[0], args);
            fprintf(stderr, "Exec failed\n");
            return 1;
        } 
        else {
            // Parent process
            if (!ampersand) {
                wait(NULL);
            }
        }
    }

    free(args);
    
    return 0;
}
