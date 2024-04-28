/*
To Compile using makefile: make
To Run: ./P4-AveryFernandez <number of students> <number of chairs> <left interval of random time to study> <right interval of random time to study>
Example: ./P4-AveryFernandez 5 3 1 5
*/

/*
Purpose
Simulate student and teacher interactions using locks and semaphores or locks and condition variables.

Teacher:
* Have 4 states
    1. No students in queue, teacher does their own work
    2. Student arrives, teacher helps student
    3. Student arrives, teacher is with another student, student waits in queue
    4. Student arrives, teacher is with another student and queue is full, student leaves and comes back later
* Queue is first come first serve and is from the command line
* Teacher is an individual thread
* Teacher can help 1 student at a time
* After all students are done, teacher ends using pthread_cancel()

Student:
* Is an individual thread
* Total number of students is from the command line
* Students alternate between studying and asking for help
* Students only get help twice then end
* Student threads need to be joined at the end of the program

Inputs from command line in order:
1. Number of students
2. Number of chairs in queue
3. Left interval of random time to study
4. Right interval of random time to study

Functions given:
* mytime(int left, int right) -> returns a random time between left and right
    * srand(z) before calling mytime to get different random numbers
    * from mytime.c

(3) In the code, print out necessary info about the activities during the execution of the code.   (3.1) Before sleep, print out "Student (or Teacher) <ThreadID> to sleep X sec;".  (3.2) After wake up from sleep, print out "Student (or Teacher) Id <ThreadID> wake up;". (3.3) For either the student or teacher thread, before calling mutex locks, or wait on semaphores, or conditional 
variables, printout "Student (or Teacher) <ThreadID> will call mutex_lock / sem_wait / conditional variables 
cond_wait  <synch variable name> ". (3.4) For either the student or teacher thread, after calling to unlock mutex locks, or post on semaphores, or 
conditional variables, printout "Student (or Teacher) <ThreadID> call mutex_unlock / sem_post  / conditional 
variables cond_signal  <synch variable name> ". 

*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include "mytime.h"

// Struct for chair
typedef struct Chair
{
    int currentStudent;
    struct Chair *nextChair;
    sem_t studentSemaphore;
} Chair;

// Global variables
Chair *head = NULL;
Chair *tail = NULL;
sem_t helpStudentSemaphore;
pthread_mutex_t queueMutex;
int numChairs;
int numStudents;
int leftInterval;
int rightInterval;

// FIFO Functions
void addChair(Chair *chair)
{
    if (head == NULL){
        head = tail = chair;
    }
    else {
        tail->nextChair = chair;
        tail = chair;
    }
}

Chair *removeChair()
{
    if (head == NULL)
    {
        return NULL;
    }
    Chair *temp = head;
    head = head->nextChair;
    if (head == NULL)
    {
        tail = NULL;
    }
    return temp;
}

// Teacher thread
void *teacherThread()
{
    pthread_t threadID = pthread_self();
    while (1)
    {
        printf("Teacher %lu will call sem_wait helpStudentSemaphore\n", threadID);
        sem_wait(&helpStudentSemaphore); // Wait for student to arrive

        printf("Teacher %lu will call mutex_lock queueMutex\n", threadID);
        pthread_mutex_lock(&queueMutex); // Lock queue

        Chair *chair = removeChair(); // Remove student from queue
        
        printf("Teacher %lu call mutex_unlock queueMutex\n", threadID);
        pthread_mutex_unlock(&queueMutex); // Unlock queue

        if (chair != NULL)
        {
            int sleepTime = mytime(leftInterval, rightInterval); // Help student for random time
            
            printf("Teacher %lu to sleep %d sec;\n", threadID, sleepTime);            
            sleep(sleepTime); // Help student for random time
            printf("Teacher %lu Id wake up;\n", threadID);

            printf("Teacher %lu call sem_post chair->studentSemaphore\n", threadID);
            sem_post(&chair->studentSemaphore); // Signal student that help is done
            sem_destroy(&chair->studentSemaphore); // Destroy studentSemaphore
            free(chair); // Free chair
        }
    }
    return NULL;
}

// Student thread
void *studentThread()
{
    int helpCount = 0;
    pthread_t threadID = pthread_self();
    while (helpCount < 2)
    {
        int studyTime = mytime(leftInterval, rightInterval); // Study for random time
        
        printf("Student %lu to sleep %d sec;\n", threadID, studyTime);
        sleep(studyTime);
        printf("Student %lu Id wake up;\n", threadID);
        
        // Create chair for student that contains a semaphore for the student
        Chair *chair = (Chair *)malloc(sizeof(Chair));
        chair->currentStudent = threadID;
        chair->nextChair = NULL;
        sem_init(&chair->studentSemaphore, 0, 0);
        
        printf("Student %lu will call mutex_lock queueMutex\n", threadID);
        pthread_mutex_lock(&queueMutex); // Lock queue
        // Check if student can be added to queue
        if (tail == NULL || tail - head < numChairs - 1)
        {
            // Add student to queue
            addChair(chair);

            printf("Student %lu call mutex_unlock queueMutex\n", threadID);
            pthread_mutex_unlock(&queueMutex); // Unlock queue
            
            printf("Student %lu call sem_post helpStudentSemaphore\n", threadID);            
            // Signal teacher to help student
            sem_post(&helpStudentSemaphore);

            printf("Student %lu will call sem_wait chair->studentSemaphore\n", threadID);
            // check if student is in queue
            sem_wait(&chair->studentSemaphore);
            helpCount++;
        }
        else
        {
            printf("Student %lu call mutex_unlock queueMutex\n", threadID);
            pthread_mutex_unlock(&queueMutex); // Unlock queue
            sem_destroy(&chair->studentSemaphore); // Destroy studentSemaphore
            free(chair); // Free chair
            continue;
        }
    }

    return NULL;
}


int main(int argc, char *argv[])
{
    // Check for correct number of arguments
    if (argc != 5)
    {
        printf("Usage: %s <number of students> <number of chairs> <left interval of random time to study> <right interval of random time to study>\n", argv[0]);
        return 1;
    }

    // Get number of students
    numStudents = atoi(argv[1]);
    if (numStudents < 1)
    {
        printf("Number of students must be greater than 0\n");
        return 1;
    }

    // Get number of chairs
    numChairs = atoi(argv[2]);
    if (numChairs < 1)
    {
        printf("Number of chairs must be greater than 0\n");
        return 1;
    }

    // Get left interval of random time to study
    leftInterval = atoi(argv[3]);
    if (leftInterval < 0)
    {
        printf("Left interval of random time to study must be greater than or equal to 0\n");
        return 1;
    }

    // Get right interval of random time to study
    rightInterval = atoi(argv[4]);
    if (rightInterval < 0)
    {
        printf("Right interval of random time to study must be greater than or equal to 0\n");
        return 1;
    }

    // Initialize random number generator
    srand(time(NULL));

    // Create Semaphores
    sem_init(&helpStudentSemaphore, 0, 0); // Initialize helpStudentSemaphore to 0
    pthread_mutex_init(&queueMutex, NULL); // Initialize the mutex for queue access
    
    // Create teacher thread
    pthread_t teacher;
    pthread_create(&teacher, NULL, teacherThread, NULL);

    // Create student threads
    pthread_t students[numStudents];
    for (int i = 0; i < numStudents; i++)
    {
        pthread_create(&students[i], NULL, studentThread, NULL);
    }

    // Join student threads
    for (int i = 0; i < numStudents; i++)
    {
        pthread_join(students[i], NULL);
    }

    // Cancel teacher thread
    pthread_cancel(teacher);

    // Destroy Semaphores
    sem_destroy(&helpStudentSemaphore);
    pthread_mutex_destroy(&queueMutex);


    return 0;
}
