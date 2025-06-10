#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

typedef struct chan Chan_t;

// Create and construct a new channel.
extern Chan_t *chan_new(size_t cap, size_t data_size);

// Push a value pointed by the `val` pointer to channel.
// return: 0 -> Ok | 1 -> Error 
extern int chan_push(Chan_t *ch, void *val);

// Pop a value from channel and write it to `dest` pointer.
// return: 0 -> Ok | 1 -> Error 
extern int chan_pop(Chan_t *ch, void *dest);

// Close a channel. If you try to push values to a closed channel
// an assertion will cause the program to terminate.
extern int chan_close(Chan_t *ch);

// Destroy and free all the memory that used by a channel.
extern void chan_destroy(Chan_t *ch);

#ifdef __cplusplus
}
#endif
