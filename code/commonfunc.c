#include "mftp.h"

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