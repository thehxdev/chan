#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <assert.h>
#include "chan.h"

// set this to zero to create an unbuffered channel
#define CHAN_CAP 5
#define CHAN_DSIZE sizeof(int)
#define TO_PUSH 5
#define NTHREADS 4

void *thread_routine(void *arg) {
    int i;
    Chan_t *ch = *(Chan_t**)arg;

    for (i = 0; i < TO_PUSH && !chan_isclosed(ch); i++)
        if (chan_push(ch, &i)) {
            printf("push error\n");
            break;
        }
    return NULL;
}

int main(void) {
    int i, v;

    Chan_t *ch = chan_new(CHAN_CAP, CHAN_DSIZE);
    assert(ch);

    pthread_t threads[NTHREADS];
    for (i = 0; i < NTHREADS; i++)
        pthread_create(&threads[i], NULL, thread_routine, &ch);

    for (i = 0; i < TO_PUSH*NTHREADS; i++) {
        if (chan_pop(ch, &v)) {
            printf("pop error\n");
            break;
        }
        printf("%d: %d\n", i, v);
    }

    for (i = 0; i < NTHREADS; i++)
        pthread_join(threads[i], NULL);

    chan_destroy(ch);
    return 0;
}
