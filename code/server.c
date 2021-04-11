#include "mftp.h"

int portNumber = 49999;
bool debug = false;
int max = 200;
char hostName[120] = {};

void createSever();
bool isNumber(const char argv[]);
void setDebug();
void commandD(int sockfd, struct sockaddr_in*);

int main(int argc, char const *argv[]) {
    bool error = true;

    if (argc == 1)
        error = false;

    else if (argc == 2 && strcmp("-d", argv[1]) == 0) {
        error = false;
        setDebug();
    }
    else if (argc == 3 && strcmp("-p", argv[1]) == 0 && isNumber(argv[2])) {
        portNumber = atoi(argv[2]);
        error = false;
    }
    else if (argc == 4 && strcmp("-d", argv[1]) == 0) {
        setDebug();
        if (strcmp("-p", argv[2]) == 0 && isNumber(argv[3])) {
            portNumber = atoi(argv[3]);
            error = false;
        }
    }
    else if (argc == 4 && strcmp("-p", argv[1]) == 0 && isNumber(argv[2])) {
        portNumber = atoi(argv[2]);
        if (strcmp("-d", argv[3]) == 0) {
            setDebug();
            error = false;
        }
    }

    if (error) {
        printf("Usage: ./mftpserve [-d] [-p PORT_NUMBER]\n");
        printf("    where: [commands] are optional\n");
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

    // Listen and Accept connections
    if (listen(socketFd, 4) < 0) {
        fprintf(stderr, "Error: %s\n", strerror(errno));
        exit(1);
    }
    if (debug)
        printf("Parent: listening with connection queue of 4\n");

    while(1) {
        if ((listenFd = accept(socketFd, (struct sockaddr *) &clientAddr,
                               &length)) < 0) {
            fprintf(stderr, "Error: %s\n", strerror(errno));
            exit(1);
        }
        if (debug)
            printf("Parent: accepted client connection with descriptor %d\n", listenFd);

        bool isExit = false;
        int pid = fork();
        int status;

        if (pid == 0) {
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
            printf("Child %d: Client IP address -> %s\n", getpid(), clientIp);
            printf("Child %d: Connection accepted from host %s\n", getpid(), clientIp);

            close(socketFd);

            char command[max];
            int readResult;

            bzero(command, max);
            while ((readResult = read(listenFd, command, max)) > 0) {
                if (strlen(command) > 0) {
                    if (strcmp("Q", command) == 0) {
                        printf("Child %d: Quitting\n", getpid());

                        write(listenFd, "A", 1);

                        if (debug) {
                            printf("Child %d: Sending positive acknowledgement\n",
                                   getpid());
                            printf("Child %d: exiting normally.\n", getpid());
                        }

                        close(listenFd);
                        exit(EXIT_SUCCESS);
                    }
                    else if (strcmp("D", command) == 0) {
                        commandD(listenFd, &clientAddr);
                    }
                    else {
                        char temp[] = "I got your command ";
                        strcat(temp, command);
                        printf("---got %s\n", temp);
                        write(listenFd, temp, strlen(temp));
                    }
                }
                bzero(command, max);
            }
        }
        else {
            printf("Parent: spawned child %d, waiting for new connection\n", pid);
            wait(&status);

            // if failed to disconnect propoerly
            if (WIFEXITED(status) && WEXITSTATUS(status) == 1)
                printf("Parent detected termination of child process %d, exit code: %d\n", pid, WEXITSTATUS(status));
            close(listenFd);
        }
    }
}

void commandD(int listenFd, struct sockaddr_in* clientAddr) {
    int socketFd2;

    // make a socket
    socketFd2 = socket(AF_INET, SOCK_STREAM, 0);
    if (socketFd2 < 0) {
        fprintf(stderr, "Error: %s\n", strerror(errno));
        exit(1);
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
        exit(1);
    }

    socklen_t length2 = sizeof(servAddr2);
    if (getsockname(socketFd2, (struct sockaddr *) &servAddr2, &length2) == -1) {
        fprintf(stderr, "Error: %s\n", strerror(errno));
    }
    else {
        int newPort = (int) ntohs(servAddr2.sin_port);
        printf("data socket bound to port %d\n", newPort);

        // Listen and Accept connections
        if (listen(socketFd2, 0) < 0) {
            fprintf(stderr, "Error: %s\n", strerror(errno));
            exit(1);
        }

        char sendPort[max];
        if (sprintf(sendPort, "%d", newPort) < 0) {
            printf("Failed to convert port number %d to string\n", newPort);
        }
        char send[max];
        strcpy(send, "A");
        strcat(send, sendPort);
        printf("sending %s\n", send);
        write(listenFd, send, strlen(send));

        int listenFd2, length2;

        printf("Child %d: listening on data socket\n", getpid());
        while(1) {
            if ((listenFd2 = accept(socketFd2,
                                    (struct sockaddr *) &clientAddr,
                                    &length2)) < 0) {
                fprintf(stderr, "Error: %s\n",
                        strerror(errno));
                exit(1);
            }

            write(listenFd2, "A", 1);
            printf("Child %d: Sending acknowledgement -> %s\n", getpid(), send);

            if (debug) {
                printf("Child %d: Accepted connection from host %s on the data socket with descriptor %d\n",
                       getpid(),
                       hostName, listenFd2);
                printf("Data socket port number on the client end is %d\n", ntohs(clientAddr -> sin_port));
                printf("Child %d: Sending positive acknowledgement\n", getpid());
            }

            close(listenFd2);
            break;
        }

    }
}

bool isNumber(const char argv[]) {
    for (int i = 0; i < strlen(argv); i++) {
        if (!isdigit(argv[i]))
            return false;
    }
    return true;
}

void setDebug() {
    debug = true;
    printf("Parent: Debug output enabled.\n");
}