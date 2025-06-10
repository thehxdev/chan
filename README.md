# Chan
Chan is a generic, thread-safe, concurrent FIFO (Queue data structure) implementation similar to Go channles in C99.
It supports buffered and unbuffered modes.

> [!WARNING]
> This project is for learning purposes. Do NOT use this in production or your projects.

## Build
This library is written with `pthread` and it's a single `chan.c` file. To build the example program `main.c`:
```bash
make
```
This will build an executable that uses `chan.c` and `chan.h` files. The test creates 4 producer threads that push integer
values to the channel with 4 consumer threads. Consumer threads pop integer values from channel and print them.

## Public interface
Read `chan.h` file. It's easy to understand :)
