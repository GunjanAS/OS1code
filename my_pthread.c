// File:	my_pthread.c
// Author:	Yujie REN
// Date:	09/23/2017

// name:Gunjan Singh, Meghana
// username of iLab:
// iLab Server:

#include "my_pthread_t.h"
#include <stdio.h>
#include <inttypes.h>
#include <ucontext.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

struct Queue queue[QUEUE_LEVELS];
int schedulerInitialization = 0;
struct itimerval timer;
static int threadYielded = 0;
static int currentQueue;
int threadNumber = 1;
int mutexUnlocked = 0;
static int waitingPriorityQ = QUEUE_LEVELS - 1;
static tcb *currentThread;
static tcb *nextThread;
static int criticalsectionThreadId = -1;
struct Queue waitingQueue;
static const tcb emptytcb;
int numberofthreads = 4;

long currentQueueQuanta;
long timetocallschedulemaintenance = 0;

int findMaxPriorityQ()
{
    for (int i = 0; i < QUEUE_LEVELS; i++)
    {
        if (!ifemptyQueue(&queue[i]))
        {
            return i;
        }
    }

    return -1;
}
void boostPriorty()
{
    struct Node *t1;
    for (int i = 0; i < QUEUE_LEVELS; i++)
    {
        t1 = queue[i].head;
        while (t1 != 0)
        {
            if (t1->thread->threadId == criticalsectionThreadId)
            {
                int size = queue[i].size;
                while (size-- > 0)
                {
                    tcb *t2;
                    if (queue[i].head->thread->threadId != criticalsectionThreadId)
                    {
                        dequeue(&queue[i], &t2);
                        enqueue(t2, &queue[i]);
                        //dequeue-ing and enqueue-ing from the queue until the head is the critical section thread
                    }
                    else
                    {
                        dequeue(&queue[i], &t2);
                        enqueue(t2, &queue[0]);

                        //once the CS thread is found , push it into the the queue[0]
                    }
                }
                return;
            }
            t1 = t1->next;
        }
    }
}

void scheduleMaintenance()
{
    //inheriting priority to remove priority inversion
    if (waitingPriorityQ < QUEUE_LEVELS - 1 && criticalsectionThreadId != -1)
    {
        boostPriorty();
    }
    //code for preventing starvation
    struct Node *temp = queue[QUEUE_LEVELS - 1].head;
    temp = queue[QUEUE_LEVELS - 1].head;
    tcb *t1;
    int size = queue[QUEUE_LEVELS - 1].size;
    while (size-- > 0)
    {
        temp = temp->next;
        dequeue(&queue[QUEUE_LEVELS - 1], &t1);
        if (t1->threadtimeSpentMsec >= 500)
        {
            enqueue(t1, &queue[0]);
        }
        else
        {

            enqueue(t1, &queue[QUEUE_LEVELS - 1]);
        }
    }
}

void changeFromWaitingQToCurrentQ()
{
    struct Node *thread = waitingQueue.head;
    thread = waitingQueue.head;
    tcb *threadReturnedAfterDeq;
    while (thread != 0)
    {
        dequeue(&waitingQueue, &threadReturnedAfterDeq);
        threadReturnedAfterDeq->threadWaiting = 0;
        enqueue(threadReturnedAfterDeq, &queue[0]);
        thread = thread->next;
    }
}

void scheduler()
{
    currentThread = (&queue[currentQueue])->head->thread;    //to get the first thread from the queue
    clock_gettime(CLOCK_REALTIME, (&currentThread->finish)); // setting the finish time of the thread
    numberOfTimesInScheduler++;
    if (currentThread->first_cycle == 1) //if thread is being scheduled for the first time , do not calculate the time spent
    {
        currentThread->first_cycle = 0;
    }
    else
    {
        double msecs, secs;
        secs = (double)(currentThread->finish.tv_sec - currentThread->start.tv_sec);
        if (secs == 0)
        {
            msecs = ((double)(currentThread->finish.tv_nsec - currentThread->start.tv_nsec)) / 1000000;
        }
        else if (secs >= 1)
        {
            secs = secs - 1;
            msecs = ((double)(999999999 - (currentThread->start.tv_nsec)) + (currentThread->finish.tv_nsec)) / 1000000;
        }
        currentThread->threadtimeSpentSec += secs;
        currentThread->threadtimeSpentMsec += msecs;
    }
    currentQueueQuanta = QUEUE_QUANTA * (currentQueue + 1); // formula for updating the quanta for the queue working on
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = currentQueueQuanta;
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = 0;
    tcb *tempThread;
    dequeue(&queue[currentQueue], &tempThread);
    if (mutexUnlocked)
    {
        mutexUnlocked = 0;
        changeFromWaitingQToCurrentQ();
    }
    if (currentThread->threadFinished == 1) //if thread is finished , find the maximum priority queue and run thread from that queue
    {
        int nextQueue = findMaxPriorityQ();
        if (nextQueue != -1)
        {
            nextThread = queue[nextQueue].head->thread;
            currentThread->threadCleaned = 1;
            currentQueue = nextQueue;
            currentQueueQuanta = QUEUE_QUANTA * (currentQueue + 1);
            timer.it_value.tv_sec = 0;
            timer.it_value.tv_usec = currentQueueQuanta;
            timer.it_interval.tv_sec = 0;
            timer.it_interval.tv_usec = 0;
            setcontext(&(nextThread->context));
            setitimer(ITIMER_REAL, &timer, 0);
            swapcontext(&(currentThread->context), &(nextThread->context));
        }
        else
        {
            printf("All the queues are empty.\n");
        }
    }
    else
    {
        //thread is not finished and if it has yielded , remove it from the ready queue and insert it into the waiting queue
        if (threadYielded && currentThread->threadWaiting)
        {
            (currentThread->yieldCount)++;
            enqueue(tempThread, &waitingQueue);
        }
        else
        //therad is not finished, push the the thread into next queue(lower priority), if it is in 2 then let it be in 2
        {
            int shiftingCurrentThreadToAnotherQ = currentQueue == 2 ? 2 : currentQueue + 1;
            if (shiftingCurrentThreadToAnotherQ != currentQueue)
            {
                currentThread->yieldCount = 0;
            }
            enqueue(tempThread, &queue[shiftingCurrentThreadToAnotherQ]);
        }
        threadYielded = 0;
    }
    timetocallschedulemaintenance += currentQueueQuanta;
    if (timetocallschedulemaintenance == 400000)
    {
        timetocallschedulemaintenance = 0;
        scheduleMaintenance();
    }
    int nextQueue = findMaxPriorityQ();
    currentQueue = nextQueue;
    currentQueueQuanta = QUEUE_QUANTA * (currentQueue + 1);
    timer.it_value.tv_sec = 0; /* set timer for "INTERVAL (10) seconds */
    timer.it_value.tv_usec = currentQueueQuanta;
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = 0;
    nextThread = queue[nextQueue].head->thread;
    setitimer(ITIMER_REAL, &timer, 0);
    clock_gettime(CLOCK_REALTIME, (&(&queue[currentQueue])->head->thread->start));
    swapcontext(&(currentThread->context), &(nextThread->context));
}

void threadStart(void (*threadFunction)(void))
{
    threadFunction();
    my_pthread_exit(NULL);
}

void firstThreadForScheduler()
{
    tcb *firstThread = (tcb *)malloc(sizeof(tcb));
    firstThread->threadId = 0;
    getcontext(&(firstThread->context));
    firstThread->stacksize = malloc(THREAD_STACK_SIZE);
    firstThread->start.tv_sec = 0;
    firstThread->start.tv_nsec = 0;
    firstThread->finish.tv_sec = 0;
    firstThread->finish.tv_nsec = 0;
    firstThread->threadFinished = 0;
    firstThread->threadCleaned = 0;
    firstThread->threadtimeSpentSec = 0;
    firstThread->threadtimeSpentMsec = 0;
    firstThread->threadWaiting = 0;
    firstThread->yieldCount = 0;
    enqueue(firstThread, &queue[0]);
}

void initialization_of_scheduler(long microseconds)
{
    signal(SIGALRM, scheduler);
    schedulerInitialization = 1;
    currentQueue = 0;
    firstThreadForScheduler();
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = microseconds;
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = 0;
    setitimer(ITIMER_REAL, &timer, 0);
}

/* create a new thread */
int my_pthread_create(my_pthread_t *thread, pthread_attr_t *attr, void *(*function)(void *), void *arg)
{
    // printf("entered \n");
    if (schedulerInitialization == 0)
    {
        initialization_of_scheduler(QUEUE_QUANTA);
    }
    tcb *newThread = (tcb *)malloc(sizeof(tcb));
    newThread->userthreadId = *thread;
    *thread = threadNumber++;
    newThread->threadId = *thread;
    getcontext(&(newThread->context));
    newThread->stacksize = malloc(THREAD_STACK_SIZE);
    newThread->start.tv_sec = 0;
    newThread->first_cycle = 1;
    newThread->start.tv_nsec = 0;
    newThread->finish.tv_sec = 0;
    newThread->finish.tv_nsec = 0;
    newThread->threadFinished = 0;
    newThread->threadCleaned = 0;
    newThread->threadtimeSpentSec = 0;
    newThread->threadtimeSpentMsec = 0;
    newThread->threadWaiting = 0;
    newThread->yieldCount = 0;
    newThread->context.uc_stack.ss_sp = newThread->stacksize;
    newThread->context.uc_stack.ss_size = THREAD_STACK_SIZE;
    newThread->context.uc_stack.ss_flags = 0;
    if (newThread->stacksize == 0)
    {
        printf("Error in allocating stack to the thread! \n");
    }
    enqueue(newThread, &queue[0]);
    makecontext(&newThread->context, (void (*)(void)) & threadStart, 1, function);
    return 0;
};

/* give CPU pocession to other user level threads voluntarily */
int my_pthread_yield()
{
    threadYielded = 1;
    raise(SIGALRM);
    return 0;
};

/* terminate a thread */
void my_pthread_exit(void *value_ptr)
{

    currentThread = queue[currentQueue].head->thread;
    // printf("%d \n", currentThread->userthreadId);

    currentThread->threadFinished = 1;
    numberofthreads--;
    // printf("%d \n", currentThread->threadFinished);
    // printf("%d \n", currentThread->threadCleaned);
    my_pthread_yield();
};

/* wait for thread termination */
int my_pthread_join(my_pthread_t thread, void **value_ptr)
{
    tcb *currentTcb;
    int found = 0;
    while (1)
    {
        for (int i = 0; i < QUEUE_LEVELS; i++)
        {
            struct Node *temp;
            temp = queue[i].head;
            while (temp != 0)
            {
                if (temp->thread->userthreadId == thread)
                {
                    currentTcb = temp->thread;
                    found = 1;
                    break;
                }
                temp = temp->next;
            }
            if (found == 1)
            {
                break;
            }
        }

        if (currentTcb->threadCleaned == 1 && currentTcb->threadFinished == 1)
        {
            return 0;
        }
        my_pthread_yield();
    }
};

/* initial the mutex lock */
int my_pthread_mutex_init(my_pthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr)
{
    mutex->threadlocked = 0;
    return 0;
};

void changeWaitingPriority()
{
    waitingPriorityQ = currentQueue < waitingPriorityQ ? currentQueue : waitingPriorityQ;
    queue[currentQueue].head->thread->threadWaiting = 1;
}

/* aquire the mutex lock */
int my_pthread_mutex_lock(my_pthread_mutex_t *mutex)
{
    while (1)
    {
        // printf("i am trying to lock\n");
        // printf("%d \n", mutex->threadlocked);
        if (mutex->threadlocked == 1)
        {
            changeWaitingPriority();
            my_pthread_yield();
        }
        else
        {
            // printf("i am here \n");
            mutex->threadlocked = 1;
            criticalsectionThreadId = queue[currentQueue].head->thread->threadId;
            mutexUnlocked = 0;
            return 0;
        }
    }
};

/* release the mutex lock */
int my_pthread_mutex_unlock(my_pthread_mutex_t *mutex)
{
    if (mutex == NULL)
    {
        my_pthread_mutex_init(mutex, NULL);
    }
    if (mutex->threadlocked)
    {
        mutex->threadlocked = 0;
        mutexUnlocked = 1;
        // printf("i unlocked\n");
    }
    // printf("%d\n", mutex->threadlocked);
    return 0;
};

/* destroy the mutex */
int my_pthread_mutex_destroy(my_pthread_mutex_t *mutex)
{
    my_pthread_mutex_unlock(mutex);
    mutex = NULL;
    return 0;
};

/*************************************Functions for Queue***********************************/
int enqueue(tcb *thread, struct Queue *queue)
{
    if (queue->head == 0)
    {
        queue->head = malloc(sizeof(struct Node));
        queue->head->thread = thread;
        queue->tail = queue->head;
        queue->tail->next = 0;
        queue->head->next = 0;
        queue->size = 1;
        return 1;
    }
    else
    {
        queue->tail->next = malloc(sizeof(struct Node));
        queue->tail = queue->tail->next;
        queue->tail->thread = thread;
        queue->tail->next = 0;
        queue->size++;
    }
    return 1;
}

int dequeue(struct Queue *queue, tcb **thread)
{
    //check if the que is alrrady empty. Do noting if Yes
    if (queue->head == 0)
    {
        return 0;
    }
    //Check if this is the last element in the que
    if (queue->head == queue->tail)
    {
        *thread = queue->head->thread;
        free(queue->head);
        queue->head = 0;
        queue->tail = 0;
    }
    else
    {
        *thread = queue->head->thread;
        struct Node *temp;
        temp = queue->head;
        queue->head = queue->head->next;
        free(temp);
    }
    queue->size--;
    return 1;
}

int ifemptyQueue(struct Queue *queue)
{
    //check if the que is alrrady empty. Do noting if Yes
    if (queue->head == 0 || queue->size == 0)
    {
        return 1;
    }
    return 0;
}
