#ifndef SETUP_H
#define SETUP_H

#include <netinet/in.h>

_Noreturn void usage(const char *prog_name, int exit_code, const char *message);
void parse_args(int argc, char **argv, char **address, char **port_str, in_port_t *port);
void find_address(in_addr_t *address, char *address_str);
int setup_server(struct sockaddr_in *addr);
void find_port(struct sockaddr_in *addr, const char host_address[INET_ADDRSTRLEN]);
int setup_client(struct sockaddr_in *addr, const char *addr_str, in_port_t port);

#endif // ! SETUP_H
