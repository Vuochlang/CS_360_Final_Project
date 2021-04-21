#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <ctype.h>
#include <stdbool.h>
#include <assert.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

void parse(char*, char*, char*);
int isDirectory (char* path);
bool setDebug(int i);
int isReg (char* path);
bool isFileExist (char *filename);
bool canRead(char* path);
char* ifPathParseName(char* path);
