#include "../include/setup.h"
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define MSGLEN 13

typedef struct data_t
{
    int                fd;
    int                cfd;
    struct sockaddr_in addr;
    socklen_t          addr_len;
} data_t;

static volatile sig_atomic_t running = 1;    // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

static void setup(data_t *d, char s[INET_ADDRSTRLEN]);
static void setup_sig_handler(void);
static void sig_handler(int sig);

int main(void)
{
    data_t data = {0};
    char   addr_str[INET_ADDRSTRLEN];
    int    retval = EXIT_SUCCESS;
    char   buf[MSGLEN + 1];    // TEST
    buf[MSGLEN] = '\0';

    setup(&data, addr_str);

    data.cfd = accept(data.fd, NULL, 0);
    if(data.cfd < 0)
    {
        retval = EXIT_FAILURE;
        goto cleanup;
    }

    // TEST

    while(running)
    {
        memset(buf, 0, MSGLEN);
        if(read(data.cfd, buf, MSGLEN) < 1)
        {
            break;
        }
        write(STDOUT_FILENO, buf, MSGLEN + 1);
    }

    // TEST

cleanup:
    close(data.fd);
    if(data.cfd > 0)
    {
        close(data.cfd);
    }
    exit(retval);
}

static void setup(data_t *d, char s[INET_ADDRSTRLEN])
{
    find_address(&(d->addr.sin_addr.s_addr), s);
    d->addr_len = sizeof(struct sockaddr_in);
    d->fd       = setup_server(&(d->addr));
    find_port(&(d->addr), s);
    setup_sig_handler();
}

/* Pairs SIGINT with sig_handler */
static void setup_sig_handler(void)
{
    struct sigaction sa;

    memset(&sa, 0, sizeof(sa));
#if defined(__clang__)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wdisabled-macro-expansion"
#endif
    sa.sa_handler = sig_handler;
#if defined(__clang__)
    #pragma clang diagnostic pop
#endif
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if(sigaction(SIGINT, &sa, NULL) == -1)
    {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

/* Write to stdout a shutdown message and set exit_flag to end while loop in main */
static void sig_handler(int sig)
{
    const char *message = "\nSIGINT received. Server shutting down.\n";
    write(1, message, strlen(message));
    running = 0;
}

#pragma GCC diagnostic pop
