#include "../include/remoteshell.h"
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void splitargs(char buf[], char *args[])
{
    int         count = 0;
    const char *token;
    char       *ptr;

    token = strtok_r(buf, " ", &ptr);
    while(token != NULL && count < MAX_ARGS - 1)
    {
        args[count++] = strdup(token);
        token         = strtok_r(NULL, " ", &ptr);
    }
    args[count] = NULL;
}

int getpath(char *args[])
{
    if(args[0] == NULL)
    {
        return -1;
    }

    if(strcmp(args[0], "ls") == 0)
    {
        free(args[0]);
        args[0] = strdup("/bin/ls");
        return 1;
    }

    if(strcmp(args[0], "cat") == 0)
    {
        free(args[0]);
        args[0] = strdup("/bin/cat");
        return 1;
    }
    return 0;
}

int checkcommand(const char *cmd)
{
    const char *builtins[NUMBUILTINS] = {"pwd", "echo", "type", "cd", "exit"};
    if(strcmp(cmd, "ls") == 0)
    {
        return C_LS;    // means its ls
    }
    for(int i = 0; i < NUMBUILTINS; i++)
    {
        if(strcmp(cmd, builtins[i]) == 0)
        {
            return i;
        }
    }

    return -1;
}

static void send_pwd(int clientfd)
{
    char path[MAX_BUF];
    if(getcwd(path, MAX_BUF) != NULL)
    {
        uint16_t len = htons((uint16_t)strlen(path));
        write(clientfd, &len, sizeof(uint16_t));
        write(clientfd, path, MAX_BUF);
    }
    else
    {
        perror("getcwd");
    }
}

static void send_echo(char *args[], int clientfd)
{
    if(args[1] == NULL)
    {
        const char *errmsg = "No arguments provided to echo\n";
        uint16_t    len    = htons((uint16_t)strlen(errmsg));
        write(clientfd, &len, sizeof(uint16_t));
        write(clientfd, errmsg, strlen(errmsg));
    }
    else
    {
        char     buf[MAX_BUF];
        size_t   curr = 0;
        uint16_t len;

        memset(buf, 0, MAX_BUF);

        for(int i = 1; i < MAX_ARGS && args[i] != NULL; i++)
        {
            curr += strlcat(buf + curr, args[i], MAX_BUF - curr);

            if(args[i + 1] != NULL)
            {
                // add space if not last
                curr += strlcat(buf + curr, " ", MAX_BUF - curr);
            }
        }
        len = htons((uint16_t)curr);
        write(clientfd, &len, sizeof(uint16_t));
        write(clientfd, buf, curr);
    }
}

static void send_type(char *args[], int clientfd)
{
    uint16_t len;
    int      cmdtype;
    char     res[MAX_BUF];
    if(args[1] == NULL || checkcommand(args[1]) != -1)
    {
        snprintf(res, MAX_BUF, "type: not found\n");
        goto send;
    }

    cmdtype = checkcommand(args[1]);
    if(cmdtype == C_LS)
    {
        snprintf(res, MAX_BUF, "ls is aliased to ls --color=auto\n");
    }
    else
    {
        snprintf(res, MAX_BUF, "%s is a shell built-in\n", args[1]);
    }
send:
    len = htons((uint16_t)strlen(res));
    write(clientfd, &len, sizeof(uint16_t));
    write(clientfd, res, len);
}

// cppcheck-suppress constParameter
static void send_cd(char *args[], int clientfd)
{
    const char *res = NULL;
    uint16_t    len;
    if(args[1] == NULL)
    {
        res = "cd: No specified directory\n";
        goto send;
    }

    if(args[2] == NULL)
    {
        if(chdir(args[1]) == -1)
        {
            res = "cd: No such file or directory\n";
            goto send;
        }
        res = "Changed current dir\n";
    }
    if(res == NULL)
    {
        perror("no response");
        return;
    }
send:
    len = htons((uint16_t)strlen(res));
    write(clientfd, &len, sizeof(uint16_t));
    write(clientfd, res, strlen(res));
}

static void send_exit(int clientfd)
{
    const char *res = "exit";
    uint16_t    len = htons((uint8_t)strlen(res));
    write(clientfd, &len, sizeof(uint16_t));
    write(clientfd, res, strlen(res));
}

int handlebuiltin(char *args[], int cmdtype, int clientfd)
{
    switch(cmdtype)
    {
        case C_PWD:
            send_pwd(clientfd);
            break;
        case C_ECHO:
            send_echo(args, clientfd);
            break;
        case C_TYPE:
            send_type(args, clientfd);
            break;
        case C_CD:
            send_cd(args, clientfd);
            break;
        case C_EXIT:
            send_exit(clientfd);
            break;
        default:
            return 0;
    }
    if(cmdtype == C_EXIT)
    {
        return 1;
    }
    return 0;
}

void freeargs(char *args[])
{
    for(int i = 0; args[i] != NULL; i++)
    {
        free(args[i]);
    }
}
