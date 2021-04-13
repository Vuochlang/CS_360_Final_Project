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

    temp = strtok(buffer, " ");
    while (temp != NULL) {
        if (i == 0)
            strcpy(command, temp);
        else
            strcpy(path, temp);
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