#include "mftp.h"

int portNumber = 49999;
bool debug = false;
int max = 200;
char hostName[120] = {};

void createSever();
bool isNumber(const char argv[]);
int commandD(int sockfd);
void commandC(int sockfd, char *path);
void commandL(int sockfd);
void commandG(int sockfd, char *path);
void commandP(int sockfd, char *path);

int main(int argc, char const *argv[]) {
    bool error = true;

    if (argc == 1)  // ./mftpserve
        error = false;

    else if (argc == 2 && strcmp("-d", argv[1]) == 0) { // ./mftpserve -d
        error = false;
        debug = setDebug(0);
    }
    else if (argc == 3 && strcmp("-p", argv[1]) == 0 && isNumber(argv[2])) {  // ./mftpserve -p <port>
        portNumber = atoi(argv[2]);
        error = false;
    }
    else if (argc == 4 && strcmp("-d", argv[1]) == 0) {
        debug = setDebug(0);
        if (strcmp("-p", argv[2]) == 0 && isNumber(argv[3])) {  // ./mftpserve -d -p <port>
            portNumber = atoi(argv[3]);
            error = false;
        }
    }
    else if (argc == 4 && strcmp("-p", argv[1]) == 0 && isNumber(argv[2])) {
        portNumber = atoi(argv[2]);
        if (strcmp("-d", argv[3]) == 0) {  // ./mftpserve -p <port> -d
            debug = setDebug(0);
            error = false;
        }
    }

    if (error) {
        fprintf(stderr, "Usage: ./mftpserve [-d] [-p PORT_NUMBER]\n");
        fprintf(stderr, "    where: [commands] are optional\n");
        return 0;
    }
    else {
        createSever();
    }
    return 0;
}

void createSever() {
    int socketFd, listenFd;
    struct sockaddr_in servAddr, clientAddr;
    int length = sizeof(struct sockaddr_in);

    // make a socket
    socketFd = socket(AF_INET, SOCK_STREAM, 0);
    if (socketFd < 0) {
        fprintf(stderr, "Error: %s\n", strerror(errno));
        exit(1);
    }

    if (setsockopt(socketFd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0) {
        fprintf(stderr, "Error: %s\n", strerror(errno));
        exit(1);
    }
    if (debug)
        printf("Parent: socket created with descriptor %d\n", socketFd);

    // Bind the socket to a port
    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(portNumber);
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(socketFd, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0) {
        fprintf(stderr, "Error: %s\n", strerror(errno));
        exit(1);
    }
    if (debug)
        printf("Parent: socket bound to port %d\n", portNumber);

    // Listen with backlog of 4
    if (listen(socketFd, 4) < 0) {
        fprintf(stderr, "Error: %s\n", strerror(errno));
        exit(1);
    }
    if (debug)
        printf("Parent: listening with connection queue of 4\n");

    while(1) {
        if ((listenFd = accept(socketFd, (struct sockaddr *) &clientAddr, &length)) < 0) {  // accept client
            fprintf(stderr, "Error: %s\n", strerror(errno));
            exit(1);
        }
        if (debug)
            printf("Parent: accepted client connection with descriptor %d\n", listenFd);

        int pid = fork();
        int status;

        if (pid == 0) {  // child process
            close(socketFd);
            printf("Child %d: started\n", getpid());

            int hostEntry;
            hostEntry = getnameinfo((struct sockaddr *) &clientAddr,
                                    sizeof(clientAddr),
                                    hostName,
                                    sizeof(hostName),
                                    NULL,
                                    0,
                                    NI_NUMERICSERV);

            if (hostEntry != 0) {
                fprintf(stderr, "Error: %s\n", gai_strerror(hostEntry));
            }

            char *clientIp = inet_ntoa(clientAddr.sin_addr);
            if (debug) {
                printf("Child %d: Client IP address -> %s\n", getpid(), clientIp);
                printf("Child %d: Connection accepted from host %s\n", getpid(), clientIp);
            }

            char command[max];
            int readResult;

            while ((readResult = read(listenFd, command, max)) > 0) {
                if (strlen(command) > 0) {
                    if (strncmp("Q", command, 1) == 0) {
                        write(listenFd, "A", 1);

                        if (debug) {
                            printf("Child %d: Quitting\n", getpid());
                            printf("Child %d: Sending positive acknowledgement\n",
                                   getpid());
                            printf("Child %d: exiting normally.\n", getpid());
                        }
                        close(listenFd);
                        exit(EXIT_SUCCESS);
                    }
                    else if (strncmp("C", command, 1) == 0) {
                        char* temp = command + 1;
                        char path[strlen(command) - 1];
                        strcpy(path, temp);
                        commandC(listenFd, path);
                    }
                    else if (strncmp("D", command, 1) == 0) {
                        int newListenFd = commandD(listenFd);  //get a new listen fd after established a data connection
                        if (newListenFd > -1) {
                            memset(command, 0, sizeof(command));
                            while ((readResult = read(newListenFd, command, max)) > 0) {
                                if (strncmp(command, "L", 1) == 0) {
                                    commandL(newListenFd);
                                } else if (strncmp(command, "G", 1) == 0) {
                                    commandG(newListenFd, command);
                                } else if (strncmp(command, "P", 1) == 0) {
                                    commandP(newListenFd, command);
                                }
                                close(newListenFd);
                                break;
                            }
                        }
                    }
                }
                memset(command, 0, sizeof(command));
            }
            if (readResult <= 0) {  // when client hits Ctrl-C
                printf("Parent detected termination of child process %d, exit code: 0\n",
                       getpid());
                exit(0);
            }
        }
        else {
            if (debug)
                printf("Parent: spawned child %d, waiting for new connection\n", pid);
            waitpid(-1, &status, WNOHANG); // flag to tell the parent process not to wait
            close(listenFd);
        }
    }
}

void commandG(int newLisFd, char* command) { //transfer the contents of <pathname> to the client with data connection fd

    char pathName[strlen(command) - 1];
    char *temp = command + 1;
    strcpy(pathName, temp);

    if (!isFileExist(pathName)) {
        char *message = "EFile does not exist\n";
        fprintf(stderr, "Child %d: Sending acknowledgement -> %s", getpid(), message);
        write(newLisFd, message, strlen(message));
        return;
    }

    if (!canRead(pathName)) {
        char *message = "EDoes not have permission to read file\n";
        fprintf(stderr, "Child %d: Sending acknowledgement -> %s", getpid(), message);
        write(newLisFd, message, strlen(message));
        return;
    }
    else if (isDirectory(pathName)) {
        printf("Child %d: Attempt to read directory\n", getpid());
        char *message = "EFile is a directory\n";
        fprintf(stderr, "Child %d: Sending acknowledgement -> %s", getpid(), message);
        write(newLisFd, message, strlen(message));
        return;
    }
    else if (isReg(pathName)) {
        write(newLisFd, "A", 1);
        if (debug) {
            printf("Child %d: Sending positive acknowledgement\n", getpid());
            printf("Child %d: Reading file '%s'\n", getpid(), pathName);
        }

        int file = open(pathName, O_RDONLY);
        int readResult;
        char fileContent[512];
        readResult = read(file, fileContent, 512);
        while (readResult > 0) {  // read file content and write to the client using new listen fd from data connection
            write(newLisFd, fileContent, readResult);
            readResult = read(file, fileContent, 512);
        }
        close(file);

        if (debug) {
            printf("Child %d: Transmitting file '%s' to client\n", getpid(), pathName);
            printf("Child %d: Done transmitting file to client.\n", getpid());
        }
    }
}

void commandL(int newLisFd) {  // execute 'ls -l' and send content to client
    write(newLisFd, "A", 1);
    if (debug)
        printf("Child %d: Sending positive acknowledgement\n", getpid());

    int stdOut = dup(STDOUT_FILENO);

    int pid = fork();
    if (pid != 0) {
        if (debug)
            printf("Child %d: forking ls process\n", getpid());
        wait(&pid);

        dup2(stdOut, STDOUT_FILENO);
        close(stdOut);

        if(debug)
            printf("Child %d: ls command completed\n", getpid());
        return;
    }
    else {
        dup2(newLisFd, STDOUT_FILENO);  // redirect output to the given fd
        close(newLisFd);

        char *arg = "-l";
        char *command = "ls";
        if (execlp(command, command, arg, NULL) < 0) {
            fprintf(stderr, "%s\n", strerror(errno));
        }
        fprintf(stderr, "Error occurred during 'execlp'\n");
    }
}

void commandP(int sockfd, char *path) {
    char pathName[strlen(path) - 1];
    char *temp = path + 1;
    strcpy(pathName, temp);

    if (isFileExist(pathName)) {
        char *message = "EFile already exist in server local directory\n";
        fprintf(stderr, "Child %d: Sending acknowledgement -> %s", getpid(), message);
        write(sockfd, message, strlen(message));
        return;
    }

    if (debug)
        printf("Child %d: Sending acknowledgement -> 'A'\n", getpid());
    write(sockfd, "A", 1);

    char fileName[20];
    strcpy(fileName, ifPathParseName(pathName));

    int file = open(fileName, O_RDWR | O_CREAT, S_IRWXU);
    if (file < 0) {
        fprintf(stderr, "Error: %s\n", strerror(errno));
    }

    char readBuffer[512];
    int result;
    while ((result = read(sockfd, readBuffer, 512)) > 0) {
        if (debug)
            printf("Child %d: Read %d bytes from client, writing to local file\n", getpid(), result);
        write(file, readBuffer, result);
    }
    close(file);
    if (debug)
        printf("Child %d: Done writing to local file\n", getpid());
}

int commandD(int listenFd) {
    char *message = "EError occurred with command \n";
    int socketFd2;
    struct sockaddr_in clientAddr;

    // make a socket
    socketFd2 = socket(AF_INET, SOCK_STREAM, 0);
    if (socketFd2 < 0) {
        fprintf(stderr, "Error: %s\n", strerror(errno));
        write(listenFd, message, strlen(message));
        return -1;
    }
    if (debug)
        printf("Child %d: data socket created with descriptor %d\n", getpid(), socketFd2);

    struct sockaddr_in servAddr2;
    memset(&servAddr2, 0, sizeof(servAddr2));
    servAddr2.sin_family = AF_INET;
    servAddr2.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr2.sin_port = htons(0); // wildcard
    if (bind(socketFd2, (struct sockaddr *) &servAddr2, sizeof(servAddr2)) < 0) {
        fprintf(stderr, "Error: %s\n", strerror(errno));
        write(listenFd, message, strlen(message));
        return -1;
    }

    socklen_t length2 = sizeof(servAddr2);
    if (getsockname(socketFd2, (struct sockaddr *) &servAddr2, &length2) == -1) {
        fprintf(stderr, "Error: %s\n", strerror(errno));
    }
    else {
        int newPort = (int) ntohs(servAddr2.sin_port);
        if (debug)
            printf("Child %d: Data socket bound to port %d\n", getpid(), newPort);

        // Listen
        if (listen(socketFd2, 1) < 0) {
            fprintf(stderr, "Error: %s\n", strerror(errno));
            write(listenFd, message, strlen(message));
            return -1;
        }

        char sendPort[10];
        int convert = sprintf(sendPort, "%d", newPort);
        sendPort[5] = '\0';
        if (convert < 0) {
            fprintf(stderr, "Child %d: Failed to convert port number %d to string\n", getpid(), newPort);
            write(listenFd, message, strlen(message));
            return -1;
        }
        assert(convert > 0);

        write(listenFd, sendPort, strlen(sendPort));
        if (debug) {
            printf("Child %d: Sending port number -> %s\n", getpid(), sendPort);
            printf("Child %d: listening on data socket\n", getpid());
        }

        int listenFd2, length2;
        if ((listenFd2 = accept(socketFd2, (struct sockaddr *) &clientAddr, &length2)) < 0) {  //get a connection
            fprintf(stderr, "Error: %s\n", strerror(errno));
            write(listenFd, message, strlen(message));
            close(listenFd2);
            return -1;
        }
        write(listenFd, "A", 1);

        if (debug) {
            printf("Child %d: Accepted connection from host %s on the data socket with descriptor %d\n", getpid(), hostName, listenFd2);
            printf("Child %d: Data socket port number on the client end is %d\n", getpid(), ntohs(clientAddr.sin_port));
        }
        return listenFd2;
    }
}

void commandC(int socketFd, char *path) {  // change current working directory to the given 'path'
    if (isDirectory(path)) {
        if (!canRead(path)) {
            char *message = "EDoes not have permission to read\n";
            printf("Child %d: Sending acknowledgement -> %s", getpid(), message);
            write(socketFd, message, strlen(message));
            return;
        }

        if (chdir(path) == 0) {
            if (debug) {
                printf("Child %d: Changed current directory to %s\n", getpid(), path);
            }
            write(socketFd, "A", sizeof("A"));
            printf("Child %d: Sending positive acknowledgement\n", getpid());
            return;
        }
        else {
            char *message = "E";
            strncat(message, strerror(errno), strlen(strerror(errno)));
            strcat(message, "\n");
            write(socketFd, message, strlen(message));
            return;
        }
    }
    else {
        char *message = "EThe given path is not a directory - ignored\n";
        write(socketFd, message, strlen(message));
        printf("Child %d: The given path <%s> is not a directory - ignored\n", getpid(), path);
    }
}

bool isNumber(const char argv[]) {  // check if 'argv[]' is digit number
    for (int i = 0; i < strlen(argv); i++) {
        if (!isdigit(argv[i]))
            return false;
    }
    return true;
}
