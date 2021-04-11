#include "mftp.h"

void createClient();
void lsCommand();
void show(char* pathName);
void parse(char*, char*, char*);
void cdCommand(char*);
int isDirectory (char* path);
void callCommands(char *, int sockfd);
void exitCommand(int sockfd);
void serverCommand(int, char*);

bool debug = false;
char portNumber[20];
char hostAddress[20];
int max = 200;

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
    printf("Connected to server %s\n", hostAddress);

    int readResult = 0;
    bool isExit = false;
    char prompt[] = "MFTP> ";

    while (!isExit) {
        write(1, prompt, strlen(prompt));

        // read from command line
        while ((readResult = read(0, buffer, max)) > 0) {
            callCommands(buffer, socketfd);
            bzero(buffer, max); // empty buffer
            write(1, prompt, strlen(prompt));  // print the prompt
        }
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
        parse(buffer, command, pathName);
        if (debug)
            printf("--- Command string = '%s' with parameter = '%s'\n", command, pathName);
        cdCommand(pathName);
    }
    else if (strcmp("rls", buffer) == 0) {
        serverCommand(socketfd, buffer);
        if (debug)
            printf("--- Command string = '%s'\n", buffer);
    }
    else if (strncmp("rcd", buffer, 3) == 0) {
        serverCommand(socketfd, buffer);
        parse(buffer, command, pathName);
        if (debug)
            printf("--- Command string = '%s' with parameter = '%s'\n", command, pathName);
    }
    else if (strncmp("get", buffer, 3) == 0) {
        parse(buffer, command, pathName);
        if (debug)
            printf("--- Command string = '%s' with parameter = '%s'\n", command, pathName);
    }
    else if (strncmp("show", buffer, 4) == 0) {
        parse(buffer, command, pathName);
        if (debug)
            printf("--- Command string = '%s' with parameter = '%s'\n", command, pathName);
    }
    else if (strncmp("put", buffer, 3) == 0) {
        parse(buffer, command, pathName);
        if (debug)
            printf("--- Command string = '%s' with parameter = '%s'\n", command, pathName);
    }
    else {
        printf("--- Command '%s' is unknown - ignored\n", buffer);
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
        strcpy(portString, buffer);
        printf("Received server response '%s'\n", portString);
        break;
    }

    // split and get port number
    char* temp = buffer + 1;
    char newPortNumber[6];
    strncpy(newPortNumber, temp, 6);
//    strcpy(newPortNumber, temp);
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
        char * hostIp = inet_ntoa (*((struct in_addr*)host_entry->h_addr_list[0]))
        printf("Data Socket Address/Port => %s:%s\n", inet_ntoa(*((struct in_addr*)
                host_entry->h_addr_list[0])), newPortNumber);
        printf("Attempting to establish Data Connection...\n");
    }

    if (connect(socketfd2, actualdata -> ai_addr, actualdata -> ai_addrlen) < 0) {
        fprintf(stderr, "Error: %s\n", strerror(errno));
        exit(1);
    }
    printf("Data connection to server established %s\n", hostAddress);

    printf("Awaiting server response\n");
    while ((result = read(socketfd2, buffer, max)) > 0) {
        if(strcmp(buffer, "A") == 0 && debug) {
            printf("Received server response: 'A'\n");
            printf("Displaying response from server\n");
        }
        break;
    }
    close(socketfd2);
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
        else
            write(1, strerror(errno), strlen(strerror(errno)));
    }
    printf("Error: the given path <%s> is not a directory - ignored\n", path);
}

int isDirectory (char* path) {  // directory and user can read
    struct stat area, *s = &area;
    return ((lstat(path, s) == 0) && (s -> st_mode & S_IRUSR) && S_ISDIR(s -> st_mode));
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


void lsCommand() {
    int pid = fork();

    if (pid != 0) {
        if (debug)
            printf("Client parent waiting on child process %d to run ls locally\n", pid);
        wait(&pid);
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