#include "../include/setup.h"
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <ifaddrs.h>
#include <inttypes.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define BASE_TEN 10
#define MAX_ARGS 3    // [program, address, port]
#define PREFIX "192.168"

_Noreturn void usage(const char *prog_name, int exit_code, const char *message)
{
    if(message)
    {
        fprintf(stderr, "%s\n", message);
    }
    fprintf(stderr, "usage: %s <address> <port>\n", prog_name);
    fputs("-h display this help message\n", stderr);
    exit(exit_code);
}

static in_port_t parse_port(const char *prog_name, const char *port_str)
{
    char     *endptr;
    uintmax_t parsed_val;
    errno      = 0;
    parsed_val = strtoumax(port_str, &endptr, BASE_TEN);
    if(errno != 0)
    {
        perror("Error parsing port");
        exit(EXIT_FAILURE);
    }
    if(*endptr != '\0')
    {
        usage(prog_name, EXIT_FAILURE, "Invalid characters in port.");
    }
    if(parsed_val > UINT16_MAX)
    {
        usage(prog_name, EXIT_FAILURE, "Entered port is out of range.");
    }
    return (in_port_t)parsed_val;
}

void parse_args(int argc, char **argv, char **address, char **port_str, in_port_t *port)
{
    int opt;
    opterr = 0;

    while((opt = getopt(argc, argv, "h")) != -1)
    {
        switch(opt)
        {
            case 'h':
                usage(argv[0], EXIT_SUCCESS, NULL);
            case '?':
                usage(argv[0], EXIT_FAILURE, "Error: Unknown option");
            default:
                usage(argv[0], EXIT_FAILURE, NULL);
        }
    }
    if(optind >= argc)
    {
        usage(argv[0], EXIT_FAILURE, "Error: Unexpected arguments");
    }
    if(argc > MAX_ARGS)
    {
        usage(argv[0], EXIT_FAILURE, "Error: Too many arguments");
    }
    if(argc % 2 == 0)    // Should be odd
    {
        usage(argv[0], EXIT_FAILURE, "Error: Address and Port cannot be entered without one another");
    }

    *address  = argv[optind];
    *port_str = argv[optind + 1];

    if((*address == NULL || *port_str == NULL))
    {
        usage(argv[0], EXIT_FAILURE, "Error: Address and Port cannot be entered without one another");
    }
    *port = parse_port(argv[0], *port_str);
}

void find_address(in_addr_t *address, char *address_str)
{
    struct ifaddrs       *ifaddr;
    const struct ifaddrs *ifa;

    if(getifaddrs(&ifaddr) == -1)
    {
        perror("getifaddrs");
        exit(EXIT_FAILURE);
    }
    for(ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
    {
        if(ifa->ifa_addr == NULL)
        {
            continue;
        }
        if(ifa->ifa_addr->sa_family == AF_INET)
        {
            if(getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), address_str, INET_ADDRSTRLEN, NULL, 0, NI_NUMERICHOST) != 0)
            {
                perror("getnameinfo");
                continue;
            }
            if(strncmp(address_str, PREFIX, strlen(PREFIX)) == 0)
            {
                inet_pton(AF_INET, address_str, address);
                break;
            }
        }
    }
    if(ifa == NULL)
    {
        freeifaddrs(ifaddr);
        perror("no address");
        exit(EXIT_FAILURE);
    }
    freeifaddrs(ifaddr);
}

static int setup_socket(void)
{
    int fd;

    fd = socket(AF_INET, SOCK_STREAM, 0);    // NOLINT(android-cloexec-socket)
    if(fd < 0)
    {
        perror("socket");
        return -1;
    }

    return fd;
}

int setup_server(struct sockaddr_in *addr)
{
    int       fd;
    socklen_t addr_len = sizeof(struct sockaddr);

    fd = setup_socket();
    if(fd < 0)
    {
        goto fail;
    }

    addr->sin_family = AF_INET;
    addr->sin_port   = 0;    // lets system decide the port

    if(bind(fd, (struct sockaddr *)addr, sizeof(struct sockaddr_in)) < 0)
    {
        perror("bind");
        goto fail;
    }

    if(listen(fd, SOMAXCONN) < 0)
    {
        perror("listen");
        goto fail;
    }

    if(getsockname(fd, (struct sockaddr *)addr, &addr_len) == -1)
    {
        perror("getsockname");
        goto fail;
    }
    return fd;
fail:
    if(fd >= 0)
    {
        close(fd);
    }
    exit(EXIT_FAILURE);
}

// cppcheck-suppress constParameterPointer
void find_port(struct sockaddr_in *addr, const char host_address[INET_ADDRSTRLEN])
{
    char port_str[NI_MAXSERV];
    if(getnameinfo((struct sockaddr *)addr, sizeof(struct sockaddr_in), NULL, 0, port_str, sizeof(port_str), NI_NUMERICSERV) != 0)
    {
        perror("getnameinfo");
        exit(EXIT_FAILURE);
    }
    printf("Listening on %s:%s\n", host_address, port_str);
}

int setup_client(struct sockaddr_in *addr, const char *addr_str, in_port_t port)
{
    int       fd;
    socklen_t addr_len = sizeof(struct sockaddr);

    if(inet_pton(AF_INET, addr_str, &addr->sin_addr) <= 0)
    {
        perror("Invalid address");
        exit(EXIT_FAILURE);
    }

    addr->sin_family = AF_INET;
    addr->sin_port   = htons(port);

    fd = setup_socket();
    if(fd < 0)
    {
        goto fail;
    }
    if(connect(fd, (struct sockaddr *)addr, addr_len) == -1)
    {
        perror("connect");
        goto fail;
    }

    printf("Connected to: %s:%u\n", addr_str, port);
    return fd;

fail:
    if(fd >= 0)
    {
        close(fd);
    }
    exit(EXIT_FAILURE);
}
