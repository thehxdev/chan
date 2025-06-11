#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <assert.h>
#include "chan.h"

// set this to zero to create an unbuffered channel
#define CHAN_CAP 5
#define CHAN_DSIZE sizeof(int)
#define THREAD_PUSH_LIMIT 5
#define PRODUCERS_COUNT 5
#define CONSUMERS_COUNT 3

void *producer(void *arg) {
    int i;
    Chan_t *ch = *(Chan_t**)arg;
    for (i = 0; i < THREAD_PUSH_LIMIT; i++)
        if (chan_push(ch, &i)) {
            printf("push error\n");
            break;
        }
    return NULL;
}

void *consumer(void *arg) {
    int v;
    Chan_t *ch = *(Chan_t**)arg;
    // while the channel is open, fetch data
    while (chan_pop(ch, &v) == 0)
        printf("%d\n", v);
    return NULL;
}

int main(void) {
    int i;
    pthread_t producers[PRODUCERS_COUNT];
    pthread_t consumers[CONSUMERS_COUNT];

    Chan_t *ch = chan_new(CHAN_CAP, CHAN_DSIZE);
    assert(ch);

    // Create producer threads
    for (i = 0; i < PRODUCERS_COUNT; i++)
        pthread_create(&producers[i], NULL, producer, &ch);

    // Create consumer threads
    for (i = 0; i < CONSUMERS_COUNT; i++)
        pthread_create(&consumers[i], NULL, consumer, &ch);

    // Wait for all producer threads to terminate
    for (i = 0; i < PRODUCERS_COUNT; i++)
        pthread_join(producers[i], NULL);

    // Close the channel. Closing the channel causes `chan_pop` return
    // non-zero integer indicating that channel is closed and also empty.
    // So consumer threads terminate.
    chan_close(ch);
    for (i = 0; i < CONSUMERS_COUNT; i++)
        pthread_join(consumers[i], NULL);

    chan_destroy(ch);
    return 0;
}
