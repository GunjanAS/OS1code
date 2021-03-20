/*
*
*  
*           
*/

#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <pthread.h>


pthread_mutex_t mutex;

/***
 * busyWait - Function to mimic sleep() as sleep() wakes on alarm
 * @param 	i 	int 	Approx. duration of wait
 * @return 	null
 */
void busyWait(int i) {
	int j = 2147400;
	i = i < 0 ? 1 : i;
	while (i>=0) {
		while (j>=0) {j--;}
		i--;
	}
}
/***
 * thread1function - Function for thread 1. This is the function that is executed when thread 1 is scheduled.
 * @param 	null
 * @return 	null
 */
void *thread1function(void *arg) {
	printf("Thread  1 trying to lock the mutex now\n");
    pthread_mutex_lock(&mutex);
    printf("Thread  1 sucessfully locked the mutex!!\n");
    int i;
    for(i = 0; i < 10; i++){
        busyWait(1);
        
     }
    pthread_mutex_unlock(&mutex);
    printf("Thread  1 unlocked the mutex\n");
}

/***
 * thread2function - Function for thread 2. This is the function that is executed when thread 2 is scheduled.
 * @param 	null
 * @return 	null
 */
void *thread2function(void *arg) {
	printf("Thread  2 trying to lock the mutex now\n");
    pthread_mutex_lock(&mutex);
    printf("Thread  2 sucessfully locked the mutex!!\n");
    int i;
    for(i = 0; i < 3 ; i++) {
        busyWait(1);
        
    }
    pthread_mutex_unlock(&mutex);
    printf("Thread  2 unlocked the mutex\n");
    printf("Thread  2 EXITING!!!!!!!!\n");
    //pthread_exit(&i);
}

/***
 * thread3function - Function for thread 3. This is the function that is executed when thread 3 is scheduled.
 * @param 	null
 * @return 	null
 */
void *thread3function(void *arg) {
printf("Thread 3 started!\n");
    int i;
    long j;
    for(i = 0; i < 2 ; i++) {
        busyWait(1);
        
    }
    for(i = 0; i < 4 ; i++) {
		for(j=0;j<1000000000;j++){}
        
    }
    printf("Thread  3 is done!\n");
}
/***
 * thread4function - Function for thread 4. This is the function that is executed when thread 4 is scheduled.
 * @param 	null
 * @return 	null
 */
void *thread4function(void *arg) {
	
    printf("Thread 4 started!\n");
    int i;
    for(i = 0; i < 4 ; i++) {
        busyWait(1);
        
    }
    pthread_mutex_unlock(&mutex);
    
	printf("Thread  4 is done!\n");
}

int main(int argc, const char * argv[]) {
	struct timeval start, end;
	float delta;
	gettimeofday(&start, NULL);
	pthread_t thread1,thread2,thread3,thread4;
	thread1=1;
    thread2=2;
    thread3=3;
    thread4=4;
    pthread_mutex_init(&mutex, NULL);
    //Create threads
    pthread_create(&thread1, NULL, &thread1function,NULL);
    pthread_create(&thread2, NULL, &thread2function,NULL);
    pthread_create(&thread3, NULL, &thread3function,NULL);
    pthread_create(&thread4, NULL, &thread4function,NULL);
    //Call join on the threads
    pthread_join(thread1,NULL);
    pthread_join(thread2,NULL);
    pthread_join(thread3,NULL);
    pthread_join(thread4,NULL);
    //Destroying the mutex
    pthread_mutex_destroy(&mutex);
    gettimeofday(&end, NULL);
    delta = (((end.tv_sec  - start.tv_sec)*1000) + ((end.tv_usec - start.tv_usec)*0.001));
    printf("Execution time in Milliseconds: %f\n",delta);
    printf("Ending main!\n");
    return 0;
}
