#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>

#include "mytime.h"

#define MAX_HELP 2

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
sem_t semStudents, semTeacher, semChairs;

int* chairQueue;
int chairs = 0, front = 0, rear = -1, count = 0, left, right, helps = 0;
int numStudents;

void intializeSem() { // Initializes the semaphores
	sem_init(&semStudents, 0, 0);
	sem_init(&semTeacher, 0, 0);
	sem_init(&semChairs, 0, chairs);
}

void enqueue(int studentID) { // Adds the studentID to the queue 
	rear = (rear + 1) % chairs;
	chairQueue[rear] = studentID;
	count++;
	printf("Student %d is sitting down in a chair %d\n", studentID, rear + 1);
}

void dequeue() { // Removes the front studentID from the queue
	int studentID = chairQueue[front];
	front = (front + 1) % chairs;
	count--;
	printf("Student %d is leaving to study\n", studentID);
}

void* studentThread(void* arg) { // Function for student threads
	int studentID = *(int*)arg;
	int helpCount = 0;

	while (helpCount < MAX_HELP) { // Each student seeks help twice
		printf("Student %d is arriving\n", studentID);
		int sleepTime = mytime(left, right);
		printf("Student %d goes to sleep for %d seconds\n", studentID, sleepTime); // Generate random sleep time
		sleep(sleepTime);

		printf("Student %d calls sem_wait on semChairs\n", studentID); // Wait for a chair to be available
		sem_wait(&semChairs); // Decrement semChairs

		printf("Student %d calls mutex_lock\n", studentID);	// Lock mutex before accessing shared resources
		pthread_mutex_lock(&mutex); // Lock mutex

		if (count < chairs) { // If there are chairs available
			enqueue(studentID); // Add student to queue
			printf("Student %d calls sem_post on semTeacher\n", studentID);
			sem_post(&semTeacher); // Signal teacher that a student is waiting
			helpCount++;
			helps++;
			printf("\t\t\t\tStudent %d received help %d times\n", studentID, helpCount);
		}

		printf("Student %d calls mutex_unlock\n", studentID);
		pthread_mutex_unlock(&mutex); // Unlock mutex after accessing shared resources

		printf("Student %d calls sem_wait on semStudents\n", studentID);
		sem_wait(&semStudents); // Wait for teacher to help

		printf("Student %d calls sem_post on semChairs\n", studentID);
		sem_post(&semChairs); // Increment semChairs

		sleepTime = mytime(left, right);
		printf("Student %d goes to sleep for %d seconds\n", studentID, sleepTime);
		sleep(sleepTime);
	}

	pthread_exit(NULL); // Exit thread after getting help twice
}

void* teacherThread(void* arg) {
	while (helps < numStudents * MAX_HELP) { // Teacher keeps running until all students have been helped twice
		printf("Teacher calls sem_wait on semTeacher\n");
		sem_wait(&semTeacher); // Wait for a student to be waiting

		printf("Teacher calls mutex_lock\n");
		pthread_mutex_lock(&mutex); // Lock mutex

		while (count > 0) { // If there are students waiting
			printf("Teacher calls sem_post on semStudents\n");
			sem_post(&semStudents); // Signal student that teacher is ready to help

			dequeue(); // Remove student from queue

			int sleepTime = mytime(left, right);
			printf("Teacher goes to sleep for %d seconds\n", sleepTime);
			sleep(sleepTime);
		}

		printf("Teacher calls mutex_unlock\n");
		pthread_mutex_unlock(&mutex); // Unlock mutex after helping student
	}

	pthread_cancel(pthread_self()); // Cancel teacher thread
	pthread_exit(NULL);
}

int main(int argc, char** argv) {
	if (argc != 5) {
		printf("Usage: %s <numStudents> <numChairs> <leftBound> <rightBound>\n", argv[0]);
		return 1;
	}

	srand(time(NULL)); // Seed the random number generator

	numStudents = atoi(argv[1]); // Parse command-line arguments
	chairs = atoi(argv[2]); // Number of chairs
	left = atoi(argv[3]); // Left bound for random sleep time
	right = atoi(argv[4]); // Right bound for random sleep time

	pthread_t teacher; // Teacher thread
	pthread_t* students = (pthread_t*)malloc(sizeof(pthread_t) * numStudents); // Array of student threads
	chairQueue = (int*)malloc(sizeof(int) * chairs); // Array of chairs

	intializeSem(); // Initialize semaphores

	pthread_create(&teacher, NULL, teacherThread, NULL); // Create teacher thread
	int* studentIDs = (int*)malloc(sizeof(int) * numStudents); // Array of student IDs
	for (int i = 0; i < numStudents; i++) { // Create student threads
		studentIDs[i] = i + 1;
		pthread_create(&students[i], NULL, studentThread, &studentIDs[i]);
		sleep(mytime(left, right)); // Sleep for random time
	}

	for (int i = 0; i < numStudents; i++) {
		pthread_join(students[i], NULL);
	}

	pthread_cancel(teacher); // Cancel teacher thread
	pthread_join(teacher, NULL); // Wait for teacher thread to finish

	pthread_mutex_destroy(&mutex); // Destroy mutex
	sem_destroy(&semStudents); // Destroy semaphores
	sem_destroy(&semTeacher);
	sem_destroy(&semChairs);
	
	free(chairQueue); // Free allocated memory
	free(students);
	free(studentIDs);

	return 0;
}