#include "mftp.h"

bool isFileExist (char *filename) {  // check whether the file exists
    int result = access(filename, F_OK);
    if (result == 0)
        return true;
    return false;
}

bool canRead(char* path) {  // check whether the user can access the file for reading
    int result = access(path, R_OK);
    if (result == 0)
        return true;
    return false;
}

int isReg (char* path) {  // check if the given path is a regular file
    struct stat area, *s = &area;
    return ((lstat(path, s) == 0) && S_ISREG(s -> st_mode));
}

int isDirectory (char* path) {  // check if the given path is a directory
    struct stat area, *s = &area;
    return ((lstat(path, s) == 0) && S_ISDIR(s -> st_mode));
}

void parse(char *buffer, char *command, char *path) {  // parse the given buffer into a command and a parameter
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
    char* temp = strrchr(path, '/'); // last occurrence of '/'
    if (temp == NULL)
        return path;
    else {
        if (strcmp(&path[strlen(path) - 1], temp) == 0)  // if user provides path like "../" or "../../../"
            return NULL;
        return (temp + 1);  // a pointer to the index where the file name is
    }
}