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

    printf("parse here <%s>\n", buffer);

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
    printf("parse here <%s> <%s>\n", command, path);
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

bool stopHere(int socketfd) {
    int bufSize = 100;
    char buffer[bufSize];
    int result;
    while ((result = read(socketfd, buffer, bufSize)) > 0) {
        printf("<%s>\n", buffer);
        if (strncmp(buffer, "A", 1) == 0) {
            printf("Received server response: 'A'\n");
//            memset(buffer, 0, sizeof(bufSize));
//            bzero(buffer, bufSize);
            return false;
        }
        else if (strncmp(buffer, "E", 1) == 0) {
            char *temp = buffer + 1;
            printf("Received server response: '%s'\n", temp);
//            memset(buffer, 0, sizeof(bufSize));
//            bzero(buffer, bufSize);
            return true;
        }
        else {
            printf("Received server response: '%s'\n", buffer);
        }
    }
}