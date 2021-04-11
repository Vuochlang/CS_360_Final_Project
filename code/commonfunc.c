#include "mftp.h"

//void lsCommand(int outputFd, int isClient, bool debug) {
//    int pid = fork();
//
//    if (pid != 0) {
//        int stdOut = dup(STDOUT_FILENO);
//
//        if (debug && isClient)
//            printf("Client parent waiting on child process %d to run ls locally\n", pid);
//        wait(&pid);
//
//        dup2(stdOut, STDOUT_FILENO);
//
//        if (debug && isClient)
//            printf("Client parent continuing\n");
//        return;
//    }
//    else {
//        dup2(outputFd, STDOUT_FILENO);  // redirect output to the given fd
//        char *arg = "-l";
//        char *command = "ls";
//        if (execlp(command, command, arg, NULL) < 0) {
//            printf("%s\n", strerror(errno));
//        }
//        printf("Error occurred during 'execvp'\n");
//    }
//}

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