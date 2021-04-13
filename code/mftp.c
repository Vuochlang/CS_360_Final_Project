#include "mftp.h"

void createClient();
void lsCommand();
void showCommand(int sockfd, char* command);
void cdCommand(char*);
void rcdCommand(char*, int sockfd);
void rlsCommand(int sockfd);
void callCommands(char *, int sockfd);
void exitCommand(int sockfd);
void serverCommand(int, char*);
void getCommand(int, char*);
void getCommand(int, char*);
void putCommand(int, char*);

void showResult(int socketFd);
void parsePrint(char*, char*);
void ctrlCHandler();

bool debug = false;
char portNumber[20];
char hostAddress[20];
int max = 200;
bool stillRunning = true;
int serverSocketFd;

int main(int argc, char* argv[]) {
    if (argc == 4 && strcmp(argv[1], "-d") == 0) {
        debug = true;
        printf("Debug output enabled.\n");
        strcpy(portNumber, argv[2]);
        strcpy(hostAddress, argv[3]);
    }
    else if (argc == 3) {
        strcpy(portNumber, argv[1]);
        strcpy(hostAddress, argv[2]);
    }
    else {
        printf("Usage: ./mftp [-d] <port> <hostname | IP address>\n");
        return 0;
    }

    createClient();
    return 0;
}

void createClient() {
    char buffer[max];
    char messageRead[max];

    int socketfd;
    struct addrinfo hints, *actualdata;
    memset(&hints, 0, sizeof(hints));
    int err;

    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_INET;
    err = getaddrinfo(hostAddress, portNumber, &hints, &actualdata);
    if (err != 0) {
        fprintf(stderr, "Error: %s\n", gai_strerror(err));
        exit(1);
    }

    socketfd = socket(actualdata -> ai_family, actualdata -> ai_socktype, 0);
    if (socketfd < 0) {
        fprintf(stderr, "Error: %s\n", strerror(errno));
        exit(1);
    }
    if (debug)
        printf("Created socket with descriptor %d\n", socketfd);

    if (connect(socketfd, actualdata -> ai_addr, actualdata -> ai_addrlen) < 0) {
        fprintf(stderr, "Error: %s\n", strerror(errno));
        exit(1);
    }
    serverSocketFd = socketfd;
    printf("Connected to server %s\n", hostAddress);

    int readResult = 0;
    bool isExit = false;
    char prompt[] = "MFTP> ";

    signal(SIGINT, ctrlCHandler);

    while (stillRunning) {
        write(1, prompt, strlen(prompt));

//        // read from command line
//        while ((readResult = read(0, buffer, max)) > 0) {
//            callCommands(buffer, socketfd);
//            bzero(buffer, max); // empty buffer
//            write(1, prompt, strlen(prompt));  // print the prompt
//        }
        // read from command line
        while (fgets(buffer, max, stdin) != NULL) {
            callCommands(buffer, socketfd);
            bzero(buffer, max); // empty buffer
            write(1, prompt, strlen(prompt));  // print the prompt
        }
        puts("Got NULL\n");
    }
}

void callCommands(char *buffer, int socketfd) {
    char command[max];
    char pathName[max];
    int readResult;

    // eliminate the new line
    buffer[strlen(buffer) - 1] = '\0';

    if (strncmp("exit", buffer, 4) == 0) {
        if (debug)
            printf("--- Command string = '%s'\n", buffer);
        exitCommand(socketfd);
        exit(0);
    }
    else if (strncmp("ls", buffer, 2) == 0) {
        if (debug)
            printf("--- Command string = '%s'\n", buffer);
        lsCommand();
        if (debug)
            printf("--- ls execution completed\n");
    }
    else if (strncmp("cd", buffer, 2) == 0) {
        if (debug)
            parsePrint(buffer, pathName);
        cdCommand(pathName);
    }
    else if (strcmp("rls", buffer) == 0) {
        if (debug) {
            printf("--- Command string = '%s'\n", buffer);
            printf("Executing remote ls command\n");
        }
        serverCommand(socketfd, buffer);
    }
    else if (strncmp("rcd", buffer, 3) == 0) {
        if (strlen(buffer) == 3) {
            printf("Missing parameter for 'rcd' command - ignored\n");
        }
        else {
            if (debug)
                parsePrint(buffer, pathName);
            rcdCommand(pathName, socketfd);
        }
    }
    else if (strncmp("get", buffer, 3) == 0) {
        if (strlen(buffer) == 3) {
            printf("Missing parameter for 'get' command - ignored\n");
        }
        else {
            if (debug)
                parsePrint(buffer, pathName);

            if (isFileExist(pathName))
                printf("Open/creating local file: File exists\n");
//            else
//                serverCommand(socketfd, buffer);
        }
    }
    else if (strncmp("show", buffer, 4) == 0) {
        if (strlen(buffer) == 4)
            printf("Missing parameter for 'show' command - ignored\n");
        else {
            if (debug)
                parsePrint(buffer, pathName);
            serverCommand(socketfd, buffer);
        }
    }
    else if (strncmp("put", buffer, 3) == 0) {
        if (strlen(buffer) == 3) {
            printf("Missing parameter for 'put' command - ignored\n");
        }
        else {
            if (debug)
                parsePrint(buffer, pathName);
//            serverCommand(socketfd, buffer);
        }
    }
    else {
        printf("--- Command '%s' is unknown - ignored\n", buffer);
    }
    bzero(pathName, max);
    bzero(buffer, max);
}

void parsePrint(char* clientCommand, char* pathName) {
    char command[max];
    char temp[strlen(clientCommand)];
    strcpy(temp, clientCommand);
    parse(temp, command, pathName);

    printf("--- Command string = '%s' with parameter = '%s'\n", command, pathName);
    if (strcmp(command, "show") == 0)
        printf("Showing file %s\n", pathName);
    else if (strcmp(command, "get") == 0)
        printf("Getting file %s from server and storing to local directory\n", pathName);
    else if (strcmp(command, "rcd") == 0)
        printf("Changing server current working directory to '%s'\n", pathName);
    else if (strcmp(command, "put") == 0)
        printf("Transfer local file %s to server current working directory\n", pathName);
}

void rcdCommand(char* path, int socketFd) {
    char serverCommand[strlen(path) + 1];
    char response[max];
    int result;

    strcpy(serverCommand, "C");
    strncat(serverCommand, path, strlen(path));
    write(socketFd, serverCommand, strlen(serverCommand));

    printf("Awaiting server response\n");
    bzero(response, max);
    while ((result = read(socketFd, response, max)) > 0) {
        if (strncmp(response, "A", 1) == 0 && debug) {
            printf("Received server response 'A'\n");
            printf("Changed remote directory to %s\n", path);
        }
        else if (strncmp(response, "E", 1) == 0) {
            char *temp = response + 1;
            printf("Received server response: '%s'\n", temp);
        }
        break;
    }
}

void serverCommand(int socketFd, char* myCommand) {
    int result;
    char buffer[max];
    char portString[16];

    write(socketFd, "D", 1);
    printf("Sent D command to server\n");

    printf("Awaiting server response\n");
    while ((result = read(socketFd, buffer, max)) > 0) {
        strncpy(portString, buffer, result);
        printf("Received server response '%s'\n", portString);
        break;
    }

    // split and get port number
    char* temp = portString + 1;
    char newPortNumber[6];
    strncpy(newPortNumber, temp, 6);
    printf("Obtained port number %s from server\n", newPortNumber);

    int socketfd2;
    struct addrinfo hints, *actualdata;
    memset(&hints, 0, sizeof(hints));
    int err;

    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_INET;
    err = getaddrinfo(hostAddress, newPortNumber, &hints, &actualdata);
    if (err != 0) {
        fprintf(stderr, "Error: %s\n", gai_strerror(err));
        exit(1);
    }

    socketfd2 = socket(actualdata -> ai_family, actualdata -> ai_socktype, 0);
    if (socketfd2 < 0) {
        fprintf(stderr, "Error: %s\n", strerror(errno));
        exit(1);
    }
    if (debug) {
        printf("Created socket with descriptor %d\n", socketfd2);
        printf("Data Socket Address/Port => %s:%s\n", hostAddress, newPortNumber);
        printf("Attempting to establish Data Connection...\n");
    }

    if (connect(socketfd2, actualdata -> ai_addr, actualdata -> ai_addrlen) < 0) {
        fprintf(stderr, "Error: %s\n", strerror(errno));
        exit(1);
    }
    if(debug)
        printf("Data connection to server established %s\n", hostAddress);

    if (strcmp("rls", myCommand) == 0) {
        rlsCommand(socketfd2);
    }
    else if(strncmp("show", myCommand, 4) == 0) {
        showCommand(socketfd2, myCommand);
    }
    else if(strncmp("get", myCommand, 3) == 0) {
        getCommand(socketfd2, myCommand);
    }
    else if(strncmp("put", myCommand, 3) == 0) {
        putCommand(socketfd2, myCommand);
    }
    close(socketfd2);
}

void putCommand(int sockfd2, char* myCommand) {
    char pathName[strlen(myCommand - 4)];
    char command[4];
    parse(myCommand, command, pathName);

    if (!isFileExist(pathName)) {
        write(sockfd2, "E", 1);
        printf("Local file '%s' does not exist - ignored\n", pathName);
        return;
    }
    else if (isDirectory(pathName)) {
        write(sockfd2, "E", 1);
        printf("Local file '%s' is a Directory - ignored\n", pathName);
        return;
    }
    else if (isReg(pathName)){
        char* message = "P";
        strcat(message, pathName);
        write(sockfd2, message, strlen(message));

        char buffer[max];
        int result;
        while ((result = read(sockfd2, buffer, max)) > 0) {
            if (strncmp(buffer, "A", 1) == 0) {
                printf("Received server response: 'A'\n");
                printf("Sending file content to the server\n");
                break;
            }
            else if (strncmp(buffer, "E", 1) == 0) {
                char *temp = buffer + 1;
                printf("Received server response: '%s'\n", temp);
                return;
            }
        }

        int file = open(pathName, O_RDWR);
        if (file < 0) {
            fprintf(stderr, "Error: %s\n", strerror(errno));
        }

        int fileSize = lseek(file, 0, SEEK_END);
        if (fileSize < 0) {
            fprintf(stderr, "Error: %s\n", strerror(errno));
        }

        lseek(file, 0, SEEK_SET);

        char fileContent[fileSize];
        if (read(file, fileContent, fileSize) < 0) {
            printf("Child %d: %s\n", getpid(), strerror(errno));
        }
        write(sockfd2, fileContent, fileSize);
        close(file);
    }
}

void getCommand(int socketfd2, char* myCommand) {
    char pathName[strlen(myCommand - 4)];
    char command[4];
    parse(myCommand, command, pathName);

    char sendToServer[strlen(pathName) + 1];
    strcpy(sendToServer, "G");
    strncat(sendToServer, pathName, strlen(pathName));

    write(socketfd2, sendToServer, strlen(sendToServer));
    printf("Awaiting server response\n");

    char buffer[max];
    int result;
    while ((result = read(socketfd2, buffer, max)) > 0) {
        if (strncmp(buffer, "A", 1) == 0) {
            printf("Received server response: 'A'\n");
            printf("Displaying response from server\n");
            break;
        }
        else if (strncmp(buffer, "E", 1) == 0) {
            char *temp = buffer + 1;
            printf("Received server response: '%s'\n", temp);
            return;
        }
    }

    FILE* fp = fopen(pathName, "w");
    if (fp == NULL) {
        fprintf(stderr, "Error: %s\n", strerror(errno));
        return;
    }

    char readBuffer[512];
    while ((result = read(socketfd2, readBuffer, 512)) > 0) {
        printf("Read %d bytes from server, writing to local file\n", result);
        fputs(readBuffer, fp);
    }
    fclose(fp);
}

void showCommand(int socketfd2, char* myCommand) {
    char pathName[strlen(myCommand - 4)];
    char command[4];
    parse(myCommand, command, pathName);

    char sendToServer[strlen(pathName) + 1];
    strcpy(sendToServer, "G");
    strncat(sendToServer, pathName, strlen(pathName));

    write(socketfd2, sendToServer, strlen(sendToServer));
    if (debug)
        printf("Awaiting server response\n");

    char readBuffer[max];
    int result;
    while ((result = read(socketfd2, readBuffer, max)) > 0) {
        if (strcmp(readBuffer, "A") == 0) {
            printf("Received server response: 'A'\n");
            printf("Displaying response from server\n");
            break;
        }
        else if (strncmp(readBuffer, "E", 1) == 0) {
            char *temp = readBuffer + 1;
            printf("Received server response: '%s'\n", temp);
            return;
        }
    }

    if (debug)
        printf("Displaying data from server & forking to 'more'...\n");

    int stdIN = dup(STDIN_FILENO);
    int pid = fork();
    if (pid != 0) {
        if (debug)
            printf("Waiting for child process %d to complete execution of more\n", pid);
        wait(&pid);

        dup2(stdIN, STDIN_FILENO);
        close(stdIN);

        if (debug)
            printf("Data display & more command completed.\n");
        return;
    }
    else {
        showResult(socketfd2);
    }
}

void rlsCommand(int socketFd) {
    write(socketFd, "L", 1);

    if (debug)
        printf("Awaiting server response\n");

    int result;
    char buffer[2];
    while ((result = read(socketFd, buffer, 2)) > 0) {
        if (strncmp(buffer, "A", 1) == 0) {
            printf("Received server response: 'A'\n");
            printf("Displaying response from server\n");
            break;
        }
        else if (strncmp(buffer, "E", 1) == 0) {
            char *temp = buffer + 1;
            printf("Received server response: '%s'\n", temp);
            return;
        }
    }

    if (debug)
        printf("Displaying data from server & forking to 'more'...\n");

    int stdIN = dup(STDIN_FILENO);

    int pid = fork();
    if (pid != 0) {
        if (debug)
            printf("Waiting for child process %d to complete execution of more\n", pid);
        wait(&pid);

        dup2(stdIN, STDIN_FILENO);
        close(stdIN);

        if (debug)
            printf("Data display & more command completed.\n");
        return;
    }
    else {
        showResult(socketFd);
    }
}

void showResult(int socketFd) {
    dup2(socketFd, STDIN_FILENO);
    close(socketFd);

    char *arg = "-20";
    char *command = "more";
    if (execlp(command, command, arg, NULL) < 0) {
        printf("%s\n", strerror(errno));
    }
    printf("Error occurred during 'execvp'\n");
}

void cdCommand(char *path) {
    if (isDirectory(path)) {
        if (chdir(path) == 0) {
            if (debug) {
                printf("Changed local directory to %s\n", path);
                printf("--- cd execution completed\n");
            }
            return;
        }
        else {
            write(1, strerror(errno), strlen(strerror(errno)));
            return;
        }
    }
    printf("Error: the given path <%s> is not a directory - ignored\n", path);
}

void lsCommand() {
    int stdIN = dup(STDIN_FILENO);
    int stdOUT = dup(STDOUT_FILENO);
    int pid = fork();

    if (pid != 0) {
        if (debug)
            printf("Client parent waiting on child process %d to run ls locally\n", pid);
        wait(&pid);

        dup2(stdIN, STDIN_FILENO);
        dup2(stdOUT, STDOUT_FILENO);
        close(stdIN);
        close(stdOUT);

        if (debug)
            printf("Client parent continuing\n");
        return;
    }
    else {
        if(debug)
            printf("Client child process %d executing local ls | more\n", getpid());

        int fd[2], reader, writer;
        assert(pipe(fd) >= 0);
        reader = fd[0];
        writer = fd[1];

        if(debug)
            printf("Child process %d starting more\n", getpid());
        int pid2 = fork();
        if (pid2 != 0) { // parent
            close(writer);

            // connect reader and take output from child
            dup2(reader, STDIN_FILENO);
            close(reader);
            wait(&pid2);

            char *arg = "-20";
            char *command = "more";
            if (execlp(command, command, arg, NULL) < 0) {
                printf("%s\n", strerror(errno));
            }
            printf("Error occurred during 'execvp'\n");
        }
        else {
            if(debug)
                printf("Child process %d starting ls\n", getpid());

            // child process, run the 'ls -l' and redirect the output to stdin
            close(reader);
            dup2(writer, STDOUT_FILENO);  // connect stdout as the writer
            close(writer);

            char *arg = "-l";
            char *command = "ls";
            if (execlp(command, command, arg, NULL) < 0) {
                printf("%s\n", strerror(errno));
            }
            printf("Error occurred during 'execvp'\n");
        }
    }
}

void exitCommand(int socketFd) {
    char buffer[2];
    int readResult;

    if (debug)
        printf("Exit command encountered\n");

    write(socketFd, "Q", 1);
    if (debug)
        printf("Awaiting server response\n");

    while ((readResult = read(socketFd, buffer, 2)) > 0) {
        if (strcmp(buffer, "A") == 0 && debug) {
            printf("Received server response: '%s'\n", buffer);
            printf("--- Client exiting normally\n");
            return;
        }
        return;
    }
}

void ctrlCHandler() {
    stillRunning = false;
    printf("\n");
    if(debug)
        printf("Ctrl-C encountered, sending notice to server, goodbye\n");
    write(serverSocketFd, "E", 1);
    signal (SIGINT, SIG_DFL);
    close(serverSocketFd);
    exit(0);
}