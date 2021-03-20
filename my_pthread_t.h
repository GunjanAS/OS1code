// File:	my_pthread_t.h
// Author:	Gunjan Singh, Meghana
// Date:	03/20/2021

// name:Gunjan Singh, Meghana
// username of iLab:
// iLab Server: 
#ifndef MY_PTHREAD_T_H
#define MY_PTHREAD_T_H

#define _GNU_SOURCE

/* include lib header files that you need here: */
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>

#define QUEUE_LEVELS 3
#define QUEUE_QUANTA 25000//microseconds
#define THREAD_STACK_SIZE 1024*64 //size of the stack= 64kB

typedef uint my_pthread_t;

typedef struct threadControlBlock {
	my_pthread_t userthreadId;
	int first_cycle;
	int threadId;
    ucontext_t context;
    void* stacksize;
    struct timespec start, finish;
    int threadFinished;
    int threadCleaned;
    double threadtimeSpentSec;
    double threadtimeSpentMsec;
    int threadWaiting;
    int  yieldCount;
} tcb; 
int numberOfTimesInScheduler;
/* mutex struct definition */
/**
 * Defining data structure for mutex.
 */
typedef struct my_pthread_mutex_t {
	int threadlocked;
} my_pthread_mutex_t;

/* define your data structures here: */

// Feel free to add your own auxiliary data structures
/**
 * Defining data structure for the node of the queue
 */
struct Node {
    tcb *thread;
    struct Node *next;
};
/**
 *  Defining the data structure for the queue
 */ 
struct Queue {
    struct Node *head;
    struct Node *tail;
    int size;
};




/* Function Declarations: */

int enqueue(tcb *thread, struct Queue *queue);

int dequeue(struct Queue *queue,tcb **thread);

int ifemptyQueue(struct Queue *queue);

/* create a new thread */
int my_pthread_create(my_pthread_t * thread, pthread_attr_t * attr, void *(*function)(void*), void * arg);

/* give CPU pocession to other user level threads voluntarily */
int my_pthread_yield();

/* terminate a thread */
void my_pthread_exit(void *value_ptr);

/* wait for thread termination */
int my_pthread_join(my_pthread_t thread, void **value_ptr);

/* initial the mutex lock */
int my_pthread_mutex_init(my_pthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr);

/* aquire the mutex lock */
int my_pthread_mutex_lock(my_pthread_mutex_t *mutex);

/* release the mutex lock */
int my_pthread_mutex_unlock(my_pthread_mutex_t *mutex);

/* destroy the mutex */
int my_pthread_mutex_destroy(my_pthread_mutex_t *mutex);

#endif
