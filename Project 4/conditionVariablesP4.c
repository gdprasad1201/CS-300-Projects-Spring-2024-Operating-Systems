#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#include "mytime.h"

#define MAX_HELP 2

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condition = PTHREAD_COND_INITIALIZER;

int studentsWaiting = 0, chairs, numStudents, leftBound, rightBound;
int helps = 0;

// Function for student threads
void* studentThread(void* arg) {
    int studentID = *(int*)arg;
    int helpCount = 0;
    
    while (helpCount < MAX_HELP) { // Each student seeks help twice
        printf("Student %d has arrived\n", studentID);
        int sleepTime = mytime(leftBound, rightBound); // Generate random sleep time
        printf("Student %d goes to sleep for %d seconds\n", studentID, sleepTime);
        sleep(sleepTime);

        printf("Student %d calls mutex_lock\n", studentID);
        pthread_mutex_lock(&mutex); // Lock mutex before accessing shared resources
        
        if (studentsWaiting < chairs) { // If there are chairs available
            studentsWaiting++;

            printf("Student %d sits on a chair. Total chairs occupied: %d\n", studentID, studentsWaiting);
            printf("Student %d calls cond_signal on condition\n", studentID);
            pthread_cond_signal(&condition); // Signal teacher that a student is waiting

            printf("Student %d calls mutex_unlock\n", studentID);
            pthread_mutex_unlock(&mutex); // Unlock mutex after accessing shared resources
            
            printf("Student %d calls mutex_lock\n", studentID);
            pthread_mutex_lock(&mutex); // Lock mutex before accessing shared resources

            printf("Student %d calls mutex_unlock\n", studentID);
            pthread_mutex_unlock(&mutex); // Unlock mutex after locking
        } 
        else {
            printf("Student %d goes back to study and calls mutex_unlock\n", studentID);
            pthread_mutex_unlock(&mutex); // Unlock mutex if no chairs are available
        }
    
        printf("Student %d calls mutex_lock\n", studentID);
        pthread_mutex_lock(&mutex); // Lock mutex before accessing shared resources

        helpCount++;
        helps++;

        printf("\t\t\t\tStudent %d recieved help %d times\n", studentID, helpCount);

        printf("Student %d calls mutex_unlock\n", studentID);
        pthread_mutex_unlock(&mutex); // Unlock mutex after accessing shared resources
    }
    
    pthread_exit(NULL); // Exit thread after getting help twice
}

// Function for teacher thread
void* teacherThread(void* arg) {
    while (helps < numStudents * MAX_HELP) { // Teacher keeps running until all students have been helped twice
        printf("Teacher calls mutex_lock\n");
        pthread_mutex_lock(&mutex); // Lock mutex before accessing shared resources

        if (studentsWaiting == 0) { // If no students are waiting
            printf("Teacher calls cond_wait on condition\n");
            pthread_cond_wait(&condition, &mutex); // Wait for signal from students
        }
        
        studentsWaiting--;
        printf("Teacher helps a student. Total chairs occupied: %d\n", studentsWaiting);

        printf("Teacher calls mutex_unlock\n");
        pthread_mutex_unlock(&mutex); // Unlock mutex after helping student
        
        int sleepTime = mytime(leftBound, rightBound); // Generate random sleep time
        printf("Teacher goes to sleep for %d seconds\n", sleepTime);
        sleep(sleepTime); // Simulate teacher helping a student
    }

    pthread_cancel(pthread_self()); // Cancel teacher thread
    pthread_exit(NULL); // Exit teacher thread
}

int main(int argc, char** argv) {
    if (argc != 5) { // Check if correct number of command-line arguments provided
        printf("Usage: %s <numStudents> <numChairs> <leftBound> <rightBound>\n", argv[0]);
        return 1;
    }
    
    srand(time(NULL)); // Seed the random number generator

    // Parse command-line arguments
    numStudents = atoi(argv[1]);
    chairs = atoi(argv[2]);
    leftBound = atoi(argv[3]);
    rightBound = atoi(argv[4]);
    
    pthread_t teacher;
    pthread_t* students = (pthread_t*)malloc(sizeof(pthread_t) * numStudents);
    int* studentIDs = (int*)malloc(sizeof(int) * numStudents);
    
    // Create teacher thread
    pthread_create(&teacher, NULL, teacherThread, NULL);
    
    // Create student threads
    for (int i = 0; i < numStudents; i++) {
        studentIDs[i] = i + 1;
        pthread_create(&students[i], NULL, studentThread, &studentIDs[i]);
        sleep(mytime(leftBound, rightBound)); // Sleep for random time before next student arrives
    }
    
    // Wait for all student threads to finish
    for (int i = 0; i < numStudents; i++) {
        pthread_join(students[i], NULL);
    }
    
    // Cancel teacher thread and wait for it to finish
    pthread_cancel(teacher);
    pthread_join(teacher, NULL);

    pthread_mutex_destroy(&mutex); // Destroy mutex
    pthread_cond_destroy(&condition); // Destroy condition variable

    free(students); // Free memory allocated for student threads
    free(studentIDs); // Free memory allocated for student IDs
    
    return 0;
}