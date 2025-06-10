/*
 * Thread-Safe Queue data structure implementation.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>

#include "chan.h"

#define chan_cap(c) ((c)+1)
#define QUEUE_REAR_INITIAL  (0)
#define QUEUE_FRONT_INITIAL (0)

enum {
    OP_DEQ,
    OP_ENQ,
};

typedef unsigned char byte;

typedef struct queue {
    size_t cap, dsize;
    size_t rear, front;
    byte *data;
    int last_op;
} Queue_t;

// A channel is simply a thread-safe wrapper for a normal queue.
struct chan {
    pthread_cond_t  pushed;
    pthread_cond_t  poped;
    pthread_mutex_t lock;
    Queue_t q;
    int closed;
};


// If no error occures, this function returns 0 otherwise 1.
static int queue_init(Queue_t *q, size_t cap, size_t data_size) {
    assert(q);
    assert(data_size > 0);
    assert(cap > 0);
    *q = (Queue_t){
        .cap   = cap,
        .dsize = data_size,
        .rear  = QUEUE_REAR_INITIAL,
        .front = QUEUE_FRONT_INITIAL,
        .data  = calloc(cap*data_size, sizeof(char)),
    };
    // useful for debug builds
    assert(q->data);
    return (q->data == NULL);
}

// Check if queue is full
static int queue_isfull(Queue_t *q) {
    assert(q);
    if (q->cap == 1)
        return (q->last_op == OP_ENQ);
    return ((q->last_op == OP_ENQ && q->front == q->rear) || (q->rear == 0 && q->front == q->cap-1));
}

// Check if queue is empty
static int queue_isempty(Queue_t *q) {
    assert(q);
    if (q->cap == 1)
        return (q->last_op == OP_DEQ);
    return (q->last_op == OP_DEQ && q->rear == q->front);
}

// Push an element to queue.
// return: 0 -> Ok | 1 -> Error 
static int queue_enqueue(Queue_t *q, void *val) {
    assert(q);
    assert(val);
    if (queue_isfull(q))
        return 1;
    byte *dest = q->data + ((q->front)*(q->dsize));
    memmove(dest, val, q->dsize);
    q->front = (q->front+1) % q->cap;
    q->last_op = OP_ENQ;
    return 0;
}

// Remove (pop) the first element from queue and store it to `dest` pointer.
// return: 0 -> Ok | 1 -> Error 
static int queue_dequeue(Queue_t *q, void *dest) {
    assert(q);
    if (queue_isempty(q))
        return 1;
    byte *src = q->data + ((q->rear)*(q->dsize));
    if (dest)
        memmove(dest, src, q->dsize);
    memset(src, 0, q->dsize);
    q->rear = (q->rear+1) % q->cap;
    q->last_op = OP_DEQ;
    return 0;
}

// Free all the memory that allocated by the queue.
// If you want to re-use the same queue, call `queue_init` after calling `queue_destroy`.
static void queue_destroy(Queue_t *q) {
    assert(q);
    free(q->data);
    q->data = NULL;
    q->rear = QUEUE_REAR_INITIAL;
    q->front = QUEUE_FRONT_INITIAL;
}

extern Chan_t *chan_new(size_t cap, size_t data_size) {
    Chan_t *ch = (Chan_t*)calloc(1, sizeof(*ch));
    // assert for debug builds
    assert(ch);
    if (!ch)
        return NULL;

    assert(!queue_init(&ch->q, chan_cap(cap), data_size));
    assert(!pthread_mutex_init(&ch->lock, NULL));
    assert(!pthread_cond_init(&ch->pushed, NULL));
    assert(!pthread_cond_init(&ch->poped, NULL));

    return ch;
}

extern int chan_push(Chan_t *ch, void *val) {
    assert(ch);
    assert(val);
    int err = 0;
    pthread_mutex_lock(&ch->lock);
    // crash if channel is closed
    assert(!ch->closed);
    while (queue_isfull(&ch->q)) {
        pthread_cond_wait(&ch->poped, &ch->lock);
        // exit the function if a channel is closed after waiting
        if (ch->closed) {
            err = 1;
            goto ret;
        }
    }
    assert(!queue_enqueue(&ch->q, val));
    pthread_cond_signal(&ch->pushed);
ret:
    pthread_mutex_unlock(&ch->lock);
    return err;
}

extern int chan_pop(Chan_t *ch, void *dest) {
    assert(ch);
    int err = 0;
    pthread_mutex_lock(&ch->lock);
    if (ch->closed) {
        if (queue_isempty(&ch->q)) {
            err = 1;
            goto unlock_ret;
        }
        goto cont;
    }
    while (queue_isempty(&ch->q))
        pthread_cond_wait(&ch->pushed, &ch->lock);
cont:
    assert(!queue_dequeue(&ch->q, dest));
    pthread_cond_signal(&ch->poped);
unlock_ret:
    pthread_mutex_unlock(&ch->lock);
    return err;
}

extern int chan_close(Chan_t *ch) {
    assert(ch);
    pthread_mutex_lock(&ch->lock);
    ch->closed = 1;
    pthread_mutex_unlock(&ch->lock);
    pthread_cond_broadcast(&ch->pushed);
    pthread_cond_broadcast(&ch->poped);
    return 0;
}

extern int chan_isclosed(Chan_t *ch) {
    assert(ch);
    return ch->closed;
}

extern void chan_destroy(Chan_t *ch) {
    queue_destroy(&ch->q);
    pthread_mutex_destroy(&ch->lock);
    pthread_cond_destroy(&ch->pushed);
    pthread_cond_destroy(&ch->poped);
    free(ch);
}

#ifdef __cplusplus
}
#endif
