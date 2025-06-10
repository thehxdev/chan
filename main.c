#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <assert.h>
#include "chan.h"

// set this to zero to create an unbuffered channel
#define CHAN_CAP 5
#define CHAN_DSIZE sizeof(int)
#define TO_PUSH 5
#define CONSUMERS_COUNT 4
#define PRODUCERS_COUNT CONSUMERS_COUNT

void *producer(void *arg) {
    int i;
    Chan_t *ch = *(Chan_t**)arg;

    for (i = 0; i < TO_PUSH; i++)
        if (chan_push(ch, &i)) {
            printf("push error\n");
            break;
        }
    return NULL;
}

void *consumer(void *arg) {
    int i, v;
    Chan_t *ch = *(Chan_t**)arg;
    for (i = 0; i < TO_PUSH; i++) {
        if (chan_pop(ch, &v)) {
            printf("pop error\n");
            break;
        }
        printf("%d: %d\n", i, v);
    }
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

    for (i = 0; i < CONSUMERS_COUNT; i++)
        pthread_create(&consumers[i], NULL, consumer, &ch);

    for (i = 0; i < PRODUCERS_COUNT; i++)
        pthread_join(producers[i], NULL);

    for (i = 0; i < CONSUMERS_COUNT; i++)
        pthread_join(consumers[i], NULL);

    chan_destroy(ch);
    return 0;
}
