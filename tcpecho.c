/*
 * TCP Echo server implementation using worker pattern and channels.
 *
 * Main thread accpets connections from the server socket and pushes client file descriptors
 * to the channel. Worker threads read client FDs from channel and handles them.
 * Workers fetch new data from channel unless channel is closed and "also" empty. So while
 * the main thread is active and not closed the channel, workers will continue to work.
 *
 * Architecture:
 *
 *    ┌────────────────┐                                               ┌────────────────┐
 *    │                │                                               │                │
 *    │     client     ├───┐                                       ┌──►│   Thread #1    │
 *    │                │   │                                       │   │                │
 *    └────────────────┘   │                                       │   └────────────────┘
 *            .            │       ┌─────────────────────────┐     │           .         
 *            .            │       │                         │     │           .         
 *            .            ├──────►│   server (main thread)  ├─────┤           .         
 *            .            │       │                         │     │           .         
 *            .            │       └─────────────────────────┘     │   ┌────────────────┐
 *    ┌────────────────┐   │                                       │   │                │
 *    │                │   │                                       └──►│   Thread #N    │
 *    │     client     ├───┘                                           │                │
 *    │                │                                               └────────────────┘
 *    └────────────────┘                                                                 
 *
 * Made with https://asciiflow.com
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <pthread.h>
#include "chan.h"

// Up to NWORKERS concurrent connections
#define NWORKERS 16
#define SERVER_ADDR "127.0.0.1"
#define SERVER_PORT 8080
#define BUFFER_SIZE 128
#define SH_BUFFER_SIZE BUFFER_SIZE
#define LISTEN_BACKLOG NWORKERS

// Gracefully close the socket
// https://blog.netherlabs.nl/articles/2009/01/18/the-ultimate-so_linger-page-or-why-is-my-tcp-not-reliable
static int close_socket(int fd) {
    int err = 0;
    unsigned char buf[SH_BUFFER_SIZE];
    if ((err = shutdown(fd, SHUT_WR)) == -1) {
        perror("shutdown");
        goto close_ret;
    }
    while (read(fd, buf, SH_BUFFER_SIZE) > 0);
close_ret:
    close(fd);
    return err;
}

void handle_client(int fd) {
    printf("handling client %d\n", fd);
    size_t nread = 0;
    char buffer[BUFFER_SIZE] = {0};
    while ((nread = read(fd, buffer, BUFFER_SIZE)) > 0) {
        if (write(fd, buffer, nread) < 1)
            break;
    }
    memset(buffer, 0, BUFFER_SIZE);
}

void *worker(void *arg) {
    int clientfd = 0;
    Chan_t *ch = (Chan_t*)arg;
    while (chan_pop(ch, &clientfd) == 0) {
        printf("got client %d in worker\n", clientfd);
        handle_client(clientfd);
        close_socket(clientfd);
    }
    return NULL;
}

int main(void) {
    int i;
    int err = EXIT_SUCCESS;

    // server listen address
    struct sockaddr_in saddr = {0};
    saddr.sin_port = htons(SERVER_PORT);
    saddr.sin_family = AF_INET;
    if (inet_pton(AF_INET, SERVER_ADDR, &saddr.sin_addr) != 1) {
        perror("inet_pton SERVER_ADDR");
        exit(EXIT_FAILURE);
    }

    // create a tcp socket, bind it to an address and start listening.
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    if (bind(sockfd, (struct sockaddr*)&saddr, sizeof(saddr)) != 0) {
        perror("bind");
        err = 1;
        goto close_sock;
    }
    if (listen(sockfd, LISTEN_BACKLOG) != 0) {
        perror("listen");
        err = 1;
        goto close_sock;
    }

    // a channel to send client file descriptors to worker threads
    Chan_t *clientfds = chan_new(NWORKERS, sizeof(int));
    if (!clientfds)
        goto close_sock;

    pthread_t workers[NWORKERS] = {0};
    for (i = 0; i < NWORKERS; i++)
        pthread_create(&workers[i], NULL, worker, (void*)clientfds);

    printf("Listening on " SERVER_ADDR ":%d\n", SERVER_PORT);
    while (1) {
        int clientfd = accept(sockfd, NULL, NULL);
        if (clientfd == -1) {
            perror("accept");
            break;
        }
        printf("accpeted client with fd %d\n", clientfd);
        if (chan_push(clientfds, &clientfd) != 0) {
            printf("channel push error\n");
            break;
        }
    }

    chan_close(clientfds);
    for (i = 0; i < NWORKERS; i++)
        pthread_join(workers[i], NULL);
    chan_destroy(clientfds);

close_sock:
    close(sockfd);
    return err;
}
