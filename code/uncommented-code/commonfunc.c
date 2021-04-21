#include "mftp.h"

bool isFileExist (char *filename) {
    int result = access(filename, F_OK);
    if (result == 0)
        return true;
    return false;
}

bool canRead(char* path) {
    int result = access(path, R_OK);
    if (result == 0)
        return true;
    return false;
}

int isReg (char* path) {
    struct stat area, *s = &area;
    return ((lstat(path, s) == 0) && S_ISREG(s -> st_mode));
}

int isDirectory (char* path) {
    struct stat area, *s = &area;
    return ((lstat(path, s) == 0) && S_ISDIR(s -> st_mode));
}

void parse(char *buffer, char *command, char *path) {
    char *temp;
    int i = 0;

    temp = strtok(buffer, " ");
    while (temp != NULL) {
        if (i == 0) {
            strcpy(command, temp);
            command[strlen(command)] = '\0';
        }
        else {
            strcpy(path, temp);
            path[strlen(path)] = '\0';
        }
        temp = strtok(NULL, " ");
        i++;
    }
}

bool setDebug(int i) {
    if (i == 0)
        printf("Parent: Debug output enabled.\n");
    else
        printf("Debug output enabled.\n");
    return true;
}

char* ifPathParseName(char* path) {
    char* temp = strrchr(path, '/');
    if (temp == NULL)
        return path;
    else {
        if (strcmp(&path[strlen(path) - 1], temp) == 0)
            return NULL;
        return (temp + 1);
    }
}