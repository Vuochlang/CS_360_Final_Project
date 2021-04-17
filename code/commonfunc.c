#include "mftp.h"

int isReg (char* path) {  // regular and user can read
    struct stat area, *s = &area;
    return ((lstat(path, s) == 0) && (s -> st_mode & S_IRUSR) && S_ISREG(s -> st_mode));
}

bool isFileExist (char *filename) {
    FILE *file;
    if (file = fopen(filename, "r")){
        fclose(file);
        return true;
    }
    return false;
}

void parse(char *buffer, char *command, char *path) {
    char *temp;
    int i = 0;

    bzero(command, strlen(command));
    bzero(path, strlen(path));

    temp = strtok(buffer, " ");
    while (temp != NULL) {
        if (i == 0)
            strncpy(command, temp, strlen(temp));
        else
            strncpy(path, temp, strlen(temp));
        temp = strtok(NULL, " ");
        i++;
    }
}

int isDirectory (char* path) {  // directory and user can read
    struct stat area, *s = &area;
    return ((lstat(path, s) == 0) && (s -> st_mode & S_IRUSR) && S_ISDIR(s -> st_mode));
}

bool setDebug(int i) {
    if (i == 0)
        printf("Parent: Debug output enabled.\n");
    else
        printf("Debug output enabled.\n");
    return true;
}

bool stopHere(int socketfd, bool debug) {
    int bufSize = 100;
    char buffer[bufSize];
    int result;

    if (debug)
        printf("Awaiting server response\n");

    while ((result = read(socketfd, buffer, bufSize)) > 0) {
        if (strncmp(buffer, "A", 1) == 0) {
            if (debug)
                printf("Received server response: 'A'\n");
            return false;
        }
        else if (strncmp(buffer, "E", 1) == 0) {
            if (debug) {
                char *temp = buffer + 1;
                printf("Received server response: '%s'\n", temp);
            }
            return true;
        }
    }
}