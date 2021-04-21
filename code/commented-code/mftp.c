#include "mftp.h"

void createClient(char*, char*);
void callCommands(char*, char *, int sockfd);
void serverCommand(int, char*, char*, char*);

void lsCommand();
void showCommand(int sockfd, char* command);
void cdCommand(char*);
void rcdCommand(char*, int sockfd);
void rlsCommand(int sockfd);
void exitCommand(int sockfd);
void getCommand(int, char*);
void putCommand(int, char*);

void showResult(int socketFd);
void parsePrint(char*, char*, char*);
bool stopHere(int socketfd);

bool debug = false;

int main(int argc, char* argv[]) {
    char portNumber[20];
    char hostAddress[20];

    // command line in form of ./mftp -d <port> <hostname | IP address>
    if (argc == 4 && strcmp(argv[1], "-d") == 0) {
        debug = setDebug(1);
        strcpy(portNumber, argv[2]);
        strcpy(hostAddress, argv[3]);
    }
    else if (argc == 3) { // command line in form of ./mftp <port> <hostname | IP address>
        strcpy(portNumber, argv[1]);
        strcpy(hostAddress, argv[2]);
    }
    else {
        printf("Usage: ./mftp [-d] <port> <hostname | IP address>\n");
        return 0;
    }

    createClient(hostAddress, portNumber);
    return 0;
}

void createClient(char* hostAddress, char* portNumber) {
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
    if (debug)
        printf("Connected to server %s\n", hostAddress);

    char prompt[] = "MFTP> ";
    int size, indexCounter, currentMaxSize;
    char buffer[1];
    char *tempStorage;

    while (1) {
        write(1, prompt, strlen(prompt));

        tempStorage = malloc(size);
        size = currentMaxSize = 10;
        indexCounter = 0;

        // repeat when asking for each command, get the user command and break out
        // create a char array with the input size from the user
        while (read(0, buffer, 1) > 0) {
            if (indexCounter == currentMaxSize) {  // user input size is more than expected size = 10
                currentMaxSize *= 2;
                // changes the size of the pointed memory block to double the previous
                tempStorage = realloc(tempStorage, currentMaxSize);
            }

            if (strncmp(buffer, "\n", 1) == 0) { // copy user command from char* into char[]
                tempStorage[indexCounter] = '\0';

                char command[strlen(tempStorage)];
                strcpy(command, tempStorage);
                free(tempStorage);

                if (strncmp("exit", command, 4) == 0) {
                    if (debug)
                        printf("--- Command string = '%s'\n", command);
                    exitCommand(socketfd);
                    close(socketfd);
                    freeaddrinfo(actualdata);
                    exit(0);
                }
                else
                    callCommands(hostAddress, command, socketfd);
                memset(buffer, 0, sizeof(buffer));
                break;
            }
            else {  // store input to the allocated char *tempStorage
                strncpy(&tempStorage[indexCounter], buffer, 1);
                indexCounter++;
            }
        }
    }
}

void callCommands(char* hostAddr, char *buffer, int socketfd) {
    char command[strlen(buffer)];
    char pathName[strlen(buffer)];

    if (strncmp("ls", buffer, 2) == 0) {
        if (debug)
            printf("--- Command string = '%s'\n", buffer);
        lsCommand();
        if (debug)
            printf("ls execution completed\n");
    }
    else if (strncmp("cd", buffer, 2) == 0) {
        if (strlen(buffer) == 2) {
            printf("Missing parameter for 'cd' command - ignored\n");
            return;
        }
        parsePrint(buffer, command, pathName);
        cdCommand(pathName);
    }
    else if (strcmp("rls", buffer) == 0) {
        if (debug) {
            printf("--- Command string = '%s'\n", buffer);
            printf("Executing remote ls command\n");
        }
        char *path = NULL;
        serverCommand(socketfd, buffer, path, hostAddr);
    }
    else if (strncmp("rcd", buffer, 3) == 0) {
        if (strlen(buffer) == 3) {
            printf("Missing parameter for 'rcd' command - ignored\n");
            return;
        }
        parsePrint(buffer, command, pathName);
        rcdCommand(pathName, socketfd);
    }
    else if (strncmp("get", buffer, 3) == 0) {
        if (strlen(buffer) == 3) {
            printf("Missing parameter for 'get' command - ignored\n");
            return;
        }

        parsePrint(buffer, command, pathName);
        char *fileName = ifPathParseName(pathName);
        if (fileName == NULL) { // path like "../"
            printf("Missing file name in the given path '%s' - ignored\n", pathName);
            return;
        }
        else if (isFileExist(fileName)) {
            printf("Open/creating local file: File exists - ignored\n");
            return;
        }
        else
            serverCommand(socketfd, command, pathName, hostAddr);
    }
    else if (strncmp("show", buffer, 4) == 0) {
        if (strlen(buffer) == 4) {
            printf("Missing parameter for 'show' command - ignored\n");
            return;
        }

        parsePrint(buffer, command, pathName);
        if (ifPathParseName(pathName) == NULL) { // path like "../"
            printf("Missing file name in the given path '%s' - ignored\n", pathName);
            return;
        }
        else
            serverCommand(socketfd, command, pathName, hostAddr);
    }
    else if (strncmp("put", buffer, 3) == 0) {
        if (strlen(buffer) == 3) {
            printf("Missing parameter for 'put' command - ignored\n");
            return;
        }

        parsePrint(buffer, command, pathName);

        if (!isFileExist(pathName)) {
            printf("Local file '%s' does not exist - ignored\n", pathName);
            return;
        }
        else if (isDirectory(pathName)) {
            printf("Local file '%s' is a Directory - ignored\n", pathName);
            return;
        }
        else if (!canRead(pathName)) {
            printf("Does not have permission to read file '%s' - ignored\n", pathName);
            return;
        }
        else if (isReg(pathName)) {
            serverCommand(socketfd, command, pathName, hostAddr);
        }
        else {
            printf("Error occurred when trying to read file '%s' - ignored\n", pathName);
            return;
        }
    }
    else {
        printf("--- Command '%s' is unknown - ignored\n", buffer);
        return;
    }
}

// parse string into command and parameter and print message if debug is enabled
void parsePrint(char* clientCommand, char* command, char* pathName) {
    char temp[strlen(clientCommand)];
    strcpy(temp, clientCommand);
    parse(temp, command, pathName);

    if (debug)
        printf("--- Command string = '%s' with parameter = '%s'\n", command, pathName);

    if (strcmp(command, "show") == 0) {
        if (debug)
            printf("Showing file with path '%s'\n", pathName);
    }
    else if (strcmp(command, "get") == 0) {
        if (debug)
            printf("Getting file with path '%s' from server and storing to local directory\n", pathName);
    }
    else if (strcmp(command, "cd") == 0) {
        if (debug)
            printf("Changing local working directory to '%s'\n", pathName);
    }
    else if (strcmp(command, "rcd") == 0) {
        if (debug)
            printf("Changing server current working directory to '%s'\n", pathName);
    }
    else if (strcmp(command, "put") == 0) {
        if (debug)
            printf("Transfer local file with path '%s' to server current working directory\n", pathName);
    }
}

void rcdCommand(char* path, int socketFd) {  //change the current working directory of the server child process
    char sendCommand[strlen(path) + 1];

    strcpy(sendCommand, "C");
    strncat(sendCommand, path, strlen(path));
    write(socketFd, sendCommand, strlen(sendCommand));

    if (stopHere(socketFd))
        return;

    printf("Changed remote directory to %s\n", path);
}

// establish the data connection and depending on 'myCommand', send call appropriate commands function
void serverCommand(int socketFd, char* myCommand, char* myPath, char* hostAddress) {
    int result;
    char buffer[6];  // server response
    char portString[6];

    write(socketFd, "D", 1);
    if (debug) {
        printf("Sent D command to server\n");
        printf("Awaiting server response\n");
    }
    while ((result = read(socketFd, buffer, 6)) > 0) {
        if (strncmp(buffer, "E", 1) == 0) {
            printf("Error occurred on the Server side - ignored command\n");
            return;
        }
        strncpy(portString, buffer, result);
        portString[result] = '\0';
        if (debug) {
            printf("Received server response '%s'\n", portString);
            printf("Obtained port number %s from server\n", portString);
        }
        break;
    }

    int socketfd2;
    struct addrinfo hints, *actualdata;
    memset(&hints, 0, sizeof(hints));
    int err;

    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_INET;
    err = getaddrinfo(hostAddress, portString, &hints, &actualdata);
    if (err != 0) {
        fprintf(stderr, "Error: %s\n", gai_strerror(err));
        return;
    }

    socketfd2 = socket(actualdata -> ai_family, actualdata -> ai_socktype, 0);
    if (socketfd2 < 0) {
        fprintf(stderr, "Error: %s\n", strerror(errno));
        return;
    }
    if (debug) {
        printf("Created socket with descriptor %d\n", socketfd2);
        printf("Data Socket Address/Port => %s:%s\n", hostAddress, portString);
        printf("Attempting to establish Data Connection...\n");
    }

    if (connect(socketfd2, actualdata -> ai_addr, actualdata -> ai_addrlen) < 0) {
        fprintf(stderr, "Error: %s\n", strerror(errno));
        return;
    }

    if (stopHere(socketFd)) { // check if there's error in the server side, skip and return back
        close(socketfd2);
        return;
    }

    if(debug)
        printf("Data connection to server established %s\n", hostAddress);

    if (strcmp("rls", myCommand) == 0) {
        rlsCommand(socketfd2);
    }
    else if(strncmp("show", myCommand, 4) == 0) {
        showCommand(socketfd2, myPath);
    }
    else if(strncmp("get", myCommand, 3) == 0) {
        getCommand(socketfd2, myPath);
    }
    else if(strncmp("put", myCommand, 3) == 0) {
        putCommand(socketfd2, myPath);
    }
    close(socketfd2);
}

void putCommand(int sockfd2, char* pathName) {  // transmitting content of the file to the server and store in cwd
    char sendToServer[strlen(pathName) + 1];
    strcpy(sendToServer, "P");
    strncat(sendToServer, pathName, strlen(pathName));
    write(sockfd2, sendToServer, strlen(sendToServer));

    if (stopHere(sockfd2))  // getting user response, whether 'A' to continue or 'E' to return back
        return;

    int file = open(pathName, O_RDONLY);  // open the file
    if (file < 0) {
        fprintf(stderr, "Error: %s\n", strerror(errno));
        return;
    }

    char fileContent[512];
    int readResult;
    readResult = read(file, fileContent, 512);
    while (readResult > 0) {  // read file content and write to the server using new listen fd from data connection
        write(sockfd2, fileContent, readResult);
        readResult = read(file, fileContent, 512);
    }
    close(file);

    if (debug) {
        printf("Sending file content to the server\n");
    }
    printf("'put' command completed\n");
}

// retrieve <pathname> from the server and store it locally in the client's cwd
void getCommand(int socketfd2, char* pathName) {

    char sendToServer[strlen(pathName) + 1];
    strcpy(sendToServer, "G");
    strncat(sendToServer, pathName, strlen(pathName));

    write(socketfd2, sendToServer, strlen(sendToServer));  // send to server in form of G<pathname>

    if (stopHere(socketfd2))  // getting user response, whether 'A' to continue or 'E' to return back
        return;

    char *fileName = ifPathParseName(pathName);
    if (debug)
        printf("Creating a file name '%s' in local directory\n", fileName);

    // create the file and give the user with permission to read, write and execute
    int file = open(fileName, O_RDWR | O_CREAT, S_IRWXU);
    if (file < 0) {
        fprintf(stderr, "Error: %s\n", strerror(errno));
        return;
    }

    char readBuffer[512];
    int result;
    while ((result = read(socketfd2, readBuffer, 512)) > 0) {  // read from socket and write content to file
        if (debug)
            printf("Read %d bytes from server, writing to local file\n", result);
        write(file, readBuffer, result);
    }
    close(file);
    printf("'get' command completed\n");
}

// retrieve the contents of the indicated remote <pathname>
// and write it to the client's standard output, 20 lines at a time
void showCommand(int socketfd2, char* pathName) {
    char sendToServer[strlen(pathName) + 1];
    strcpy(sendToServer, "G");
    strncat(sendToServer, pathName, strlen(pathName));

    write(socketfd2, sendToServer, strlen(sendToServer));  // using 'get' command to get file content from the server

    if (stopHere(socketfd2)) // getting user response, whether 'A' to continue or 'E' to return back
        return;

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

        printf("Data display & more command completed.\n");
        return;
    }
    else {
        showResult(socketfd2);  // redirect data from socketfd2 as a stdin to use execlp on 'more -20'
    }
}

void rlsCommand(int socketFd) {  // execute 'ls -l' on server side and show output at client stdout
    write(socketFd, "L", 1);

    if (stopHere(socketFd))  // getting user response, whether 'A' to continue or 'E' to return back
        return;

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
        showResult(socketFd); // redirect data from socketFd as a stdin to use execlp on 'more -20'
    }
}

void showResult(int socketFd) {  // redirect content from given file descriptor as a stdin and use execlp 'more -20'
    dup2(socketFd, STDIN_FILENO);
    close(socketFd);

    char *arg = "-20";
    char *command = "more";
    if (execlp(command, command, arg, NULL) < 0) {
        printf("%s\n", strerror(errno));
    }
    printf("Error occurred during 'execvp'\n");
}

void cdCommand(char *path) {  // change local current working directory to the given 'path'
    if (!isFileExist(path)) {
        printf("The given path '%s' does not exist - ignored\n", path);
        return;
    }
    if (!canRead(path)) {
        printf("The given path '%s' is not readable - ignored\n", path);
        return;
    }

    if (isDirectory(path)) {
        if (chdir(path) == 0) {
            if (debug) {
                printf("Changed local directory to %s\n", path);
            }
            printf("'cd' execution completed\n");
            return;
        }
        else {
            write(1, strerror(errno), strlen(strerror(errno)));
            return;
        }
    }
    printf("Error: the given path <%s> is not a directory - ignored\n", path);
}

void lsCommand() {  // execute 'ls -l' locally and output result 20 per lines to local stdout
    int stdIN = dup(STDIN_FILENO);
    int stdOUT = dup(STDOUT_FILENO);
    int pid = fork();

    if (pid != 0) {  // parent to wait for child to finish 'ls -l | more -20'
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
        if (pid2 != 0) { // parent to wait for the child to finish 'ls -l' and then execute 'more -20'
            close(writer);

            dup2(reader, STDIN_FILENO);  // connect reader and take output from child
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

void exitCommand(int socketFd) {  // terminate the client after instructing the server's child process to do the same
    char buffer[1];

    if (debug)
        printf("Exit command encountered\n");

    write(socketFd, "Q", 1);
    if (debug)
        printf("Awaiting server response\n");

    while (read(socketFd, buffer, 1) > 0) {  // waiting for server's acknowledgment
        if (strcmp(buffer, "A") == 0) {
            if (debug) {
                printf("Received server response: '%s'\n", buffer);
            }
            printf("Client exiting normally\n");
        }
        return;
    }
}

bool stopHere(int socketfd) {
    int bufSize = 100;
    char buffer[bufSize];
    int result;

    if (debug)
        printf("Awaiting server response\n");

    // get server's response and return False - server's positive acknowledgment, client can continue the process
    // or return True - there's an error in the server side and client doesn't have to continue the command process
    while ((result = read(socketfd, buffer, bufSize)) > 0) {
        if (strncmp(buffer, "A", 1) == 0) {
            if (debug)
                printf("Received server response: 'A'\n");
            return false;
        }
        else if (strncmp(buffer, "E", 1) == 0) {
            char temp[result];
            strcpy(temp, buffer);
            temp[result - 1] = '\0';
            char *copy = temp + 1;

            printf("Received server response: '%s'\n", copy);
            return true;
        }
    }
}
