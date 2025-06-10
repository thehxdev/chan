#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <assert.h>
#include <stdlib.h>
#include <semaphore.h>
#include "chan.h"

// threads count in thread pool
#define NTHREADS 16

typedef struct task {
    void*(*fn)(void*);
    void *arg;
    void **ret;
    sem_t sem;
} Task_t;

typedef struct array {
    int *nums;
    int len;
} Array_t;

void *sum_array(void *arg) {
    int i;
    int *sum = malloc(sizeof(*sum));
    Array_t *arr = (Array_t*)arg;
    for (i = 0; i < arr->len; i++)
        *sum += arr->nums[i];
    return sum;
}

void *thread_routine(void *arg) {
    Task_t t;
    Chan_t *ch = (Chan_t*)arg;
    // while the channel is open fetch the tasks
    while (chan_pop(ch, &t) == 0) {
        // execute tasks and set the result
        *t.ret = t.fn(t.arg);
        // notify the sleeping thread that the result is ready
        sem_post(&t.sem);
    }
    return NULL;
}

int main(void) {
    int i;
    pthread_t threads[NTHREADS] = {0};

    // create a channel to pass data between threads
    Chan_t *ch = chan_new(NTHREADS, sizeof(Task_t));
    assert(ch);

    // create a thread pool
    for (i = 0; i < NTHREADS; i++)
        pthread_create(&threads[i], NULL, thread_routine, ch);

    // create a task
    // this task calculates sum of the numbers in an array
    int *res;
    Array_t arr = (Array_t){
        .nums = (int[5]){1, 2, 3, 4, 5},
        .len = 5,
    };
    Task_t t = (Task_t) {
        .fn = sum_array,
        .arg = &arr,
        .ret = (void**)&res,
    };
    // need a semaphore to ensure that we received the data
    // from the thread
    sem_init(&t.sem, 0, 1);

    chan_push(ch, &t);
    // closing the channels will cause the `chan_pop` function
    // return a non-zero integer and all the threads will terminate.
    chan_close(ch);

    // wait for the result
    sem_wait(&t.sem);

    // join the threads
    for (i = 0; i < NTHREADS; i++)
        pthread_join(threads[i], NULL);

    // print the result and cleanup
    printf("%d\n", *res);
    sem_destroy(&t.sem);
    chan_destroy(ch);
    free(res);
    return 0;
}
