#include "../include/remoteshell.h"
#include "../include/setup.h"
#include <netinet/in.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

typedef struct data_t
{
    int                fd;
    int                cfd;
    struct sockaddr_in addr;
    socklen_t          addr_len;
} data_t;

static volatile sig_atomic_t running = 1;    // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

static void    setup(data_t *d, char s[INET_ADDRSTRLEN]);
static void    setup_sig_handler(void);
static void    sig_handler(int sig);
static ssize_t read_input(int fd, char buf[]);

int main(void)
{
    data_t data = {0};
    char   addr_str[INET_ADDRSTRLEN];
    int    retval = EXIT_SUCCESS;

    setup(&data, addr_str);

    while(running)
    {
        pid_t pid;
        data.cfd = accept(data.fd, NULL, 0);
        if(data.cfd < 0)
        {
            if(!running)
            {
                break;
            }
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
            close(data.cfd);
            break;
        }
        if(pid == 0)    // gen2
        {
            while(1)
            {
                char    buf[MAX_BUF];
                char   *new_args[MAX_ARGS];
                ssize_t bytes_read;
                int     cmdtype;

                memset(buf, 0, MAX_BUF);
                bytes_read = read_input(data.cfd, buf);
                if(bytes_read <= 0)
                {
                    const char *errmsg = "Client disconnected\n";
                    if(!running)
                    {
                        break;
                    }
                    write(1, errmsg, strlen(errmsg));
                    break;
                }

                splitargs(buf, new_args);
                cmdtype = getpath(new_args);
                if(cmdtype == -1)
                {
                    const char *errmsg = "Client disconnected\n";
                    write(1, errmsg, strlen(errmsg));
                    freeargs(new_args);
                    break;
                }
                if(cmdtype == 0)
                {
                    cmdtype = checkcommand(new_args[0]);
                    if(cmdtype == -1)
                    {
                        const char *errmsg = "Unrecognized or unsupported command\n";
                        uint16_t    len    = htons((uint16_t)strlen(errmsg));
                        write(data.cfd, &len, sizeof(uint16_t));
                        write(data.cfd, errmsg, strlen(errmsg));
                    }
                    if(cmdtype < C_LS)
                    {
                        if(handlebuiltin(new_args, cmdtype, data.cfd) == 1)
                        {
                            freeargs(new_args);
                            printf("Client exited\n");
                            break;
                        }
                    }
                }
                else
                {
                    pid_t pid_ex = fork();
                    if(pid_ex < 0)
                    {
                        perror("fork");
                    }
                    else if(pid_ex == 0)    // gen3
                    {
                        dup2(data.cfd, STDOUT_FILENO);
                        if(execv(new_args[0], new_args) == -1)
                        {
                            const char *errmsg = "No command found";
                            uint16_t    len    = htons((uint16_t)strlen(errmsg));
                            write(data.cfd, &len, sizeof(uint16_t));
                            write(data.cfd, errmsg, strlen(errmsg));
                            perror("execv");
                            retval = EXIT_FAILURE;
                        }
                        freeargs(new_args);
                        close(data.cfd);
                        exit(retval);
                    }
                    else    // gen2
                    {
                        waitpid(pid_ex, NULL, 0);
                    }
                }
                freeargs(new_args);
            }
            // gen2
            close(data.cfd);
            exit(retval);
        }
        // gen1
        close(data.cfd);
    }
    // gen1
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

static ssize_t read_input(int fd, char buf[])
{
    uint16_t len;
    ssize_t  bytes_read;

    if(read(fd, &len, sizeof(uint16_t)) <= 1)
    {
        return -1;
    }

    len = ntohs(len);

    bytes_read = read(fd, buf, len);
    if(bytes_read < len)
    {
        return -1;
    }

    return bytes_read;
}
