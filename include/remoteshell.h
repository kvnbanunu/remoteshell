#ifndef REMOTESHELL_H
#define REMOTESHELL_H

#define MAX_BUF 1024
#define MAX_ARGS 10
#define BUILTIN 1
#define NUMBUILTINS 5

enum Commands
{
    C_PWD,
    C_ECHO,
    C_TYPE,
    C_CD,
    C_EXIT,
    C_LS
};

void splitargs(char buf[], char *args[]);
int  getpath(char *args[]);
int  checkcommand(const char *cmd);
int  handlebuiltin(char *args[], int cmdtype, int clientfd);
void freeargs(char *args[]);

#endif    // !REMOTESHELL_H
