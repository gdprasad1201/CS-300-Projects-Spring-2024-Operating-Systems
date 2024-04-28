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
    pthread_cond_t studentCond;
} Chair;

// Global variables
Chair *head = NULL;
Chair *tail = NULL;
pthread_cond_t helpStudentCond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t queueUpdateMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t queueMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t studentHelpMutex = PTHREAD_MUTEX_INITIALIZER;
int studentsWaiting = 0;
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

        while (studentsWaiting == 0)
        {
            printf("Teacher %lu will call mutex_lock queueUpdateMutex\n", threadID);
            pthread_mutex_lock(&queueUpdateMutex); // Lock queue

            printf("Teacher %lu will call cond_wait helpStudentCond\n", threadID);
            pthread_cond_wait(&helpStudentCond, &queueUpdateMutex);

            printf("Teacher %lu call mutex_unlock queueUpdateMutex\n", threadID);
            pthread_mutex_unlock(&queueUpdateMutex); // Unlock queue
        }

        printf("Teacher %lu will call mutex_lock queueMutex\n", threadID);
        pthread_mutex_lock(&queueMutex); // Lock queue

        Chair *chair = removeChair(); // Remove student from queue
        studentsWaiting--;
        
        printf("Teacher %lu call mutex_unlock queueMutex\n", threadID);
        pthread_mutex_unlock(&queueMutex); // Unlock queue

        if (chair != NULL)
        {
            int sleepTime = mytime(leftInterval, rightInterval); // Help student for random time
            
            printf("Teacher %lu to sleep %d sec;\n", threadID, sleepTime);            
            sleep(sleepTime); // Help student for random time
            printf("Teacher %lu Id wake up;\n", threadID);

            printf("Teacher %lu will call cond_signal chair->studentCond\n", threadID);
            pthread_cond_signal(&chair->studentCond);
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
        pthread_cond_init(&chair->studentCond, NULL);
        
        printf("Student %lu will call mutex_lock queueMutex\n", threadID);
        pthread_mutex_lock(&queueMutex); // Lock queue

        // Check if student can be added to queue
        if (studentsWaiting < numChairs)
        {
            // Add student to queue
            addChair(chair);
            studentsWaiting++;

            printf("Student %lu call mutex_unlock queueMutex\n", threadID);
            pthread_mutex_unlock(&queueMutex);

            printf("Student %lu call cond_signal helpStudentCond\n", threadID);
            pthread_cond_signal(&helpStudentCond);

            printf("Student %lu will call mutex_lock studentHelpMutex\n", threadID);
            pthread_mutex_lock(&studentHelpMutex); // Lock student help

            printf("Student %lu will call cond_wait chair->studentCond\n", threadID);
            pthread_cond_wait(&chair->studentCond, &studentHelpMutex);

            printf("Student %lu call mutex_unlock studentHelpMutex\n", threadID);
            pthread_mutex_unlock(&studentHelpMutex); // Unlock student help
            
            helpCount++;
        }
        else{
            printf("Student %lu call mutex_unlock queueMutex\n", threadID);
            pthread_mutex_unlock(&queueMutex); // Unlock queue
        }
        pthread_cond_destroy(&chair->studentCond); // Destroy studentCond
        free(chair); // Free chair
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

    // Create Conditionals
    pthread_cond_init(&helpStudentCond, NULL); // Initialize helpStudentCond
    pthread_mutex_init(&queueUpdateMutex, NULL); // Initialize the mutex for queue update
    pthread_mutex_init(&queueMutex, NULL); // Initialize the mutex for queue access
    pthread_mutex_init(&studentHelpMutex, NULL); // Initialize the mutex for student help
    
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

    // Destroy mutex and conditionals
    pthread_mutex_destroy(&queueUpdateMutex);
    pthread_mutex_destroy(&queueMutex);
    pthread_mutex_destroy(&studentHelpMutex);
    pthread_cond_destroy(&helpStudentCond);


    return 0;
}
