#include "../include/setup.h"
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define IN_MAX 255 // max 1 byte

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
static int read_input(int fd, char *buf);
static ssize_t read_fully(int fd, void *buf, size_t count);

int main(void)
{
    data_t data = {0};
    char   addr_str[INET_ADDRSTRLEN];
    int    retval = EXIT_SUCCESS;
    pid_t pid;

    setup(&data, addr_str);

    while(running)
    {
        data.cfd = accept(data.fd, NULL, 0);
        if(data.cfd < 0)
        {
            perror("accept");
            retval = EXIT_FAILURE;
            break;
        }

        printf("Client connected\n");

        pid = fork();
        if(pid < 0)
        {
            perror("fork");
            retval = EXIT_FAILURE;
            break;
        }
        if(pid == 0)
        {
            while(1)
            {
                char *buf;
                ssize_t bytes_read;

                bytes_read = read_input(data.cfd, buf);
                if(bytes_read <= 0)
                {
                    // error or smt

                }
            }
            close(data.cfd);
            exit(retval);
        }
        close(data.cfd);
    }
    close(data.fd);
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

static int read_input(int fd, char *buf)
{
    uint8_t len;
    ssize_t bytes_read;

    if(read(fd, &len, 1) < 1)
    {
        perror("read len");
        return -1;
    }

    buf = (char *)malloc(len + 1);
    bytes_read = read(fd, buf, len);
    if(bytes_read < len)
    {
        perror("read payload");
        return -1;
    }
    
    buf[len] = '\0';

    return bytes_read;
}

static ssize_t read_fully(int fd, void *buf, size_t count)
{
    size_t bytes_read = 0;
    while(bytes_read < count)
    {
        ssize_t res = read(fd, (char *)buf + bytes_read, count - bytes_read);
        if (res == 0)
        {
            break; // EOF reached
        }
        if(res == -1)
        {
            if(errno == EINTR)
            {
                continue; // Interrupted, retry
            }
            return -1;
        }
        bytes_read += res;
    }
    return bytes_read;
}
