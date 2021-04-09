#include "mftp.h"

void createClient();
void lsCommand();
void show(char* pathName);

bool debug = false;
char portNumber[20];
char hostAddress[20];

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
    int max = 200;
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
            if (strncmp("exit", buffer, 4) == 0) {
                if (debug) {
                    printf("Command string = 'exit'\n");
                    printf("Exit command encountered\n");
                }
                bzero(buffer, max);

                write(socketfd, "Q", 1);
                if (debug)  printf("Awaiting server response\n");
                while ((readResult = read(socketfd, buffer, max)) > 0) {
                    if (strcmp(buffer, "A") == 0 && debug) {
                        printf("Received server response: '%s'\n", buffer);
                        printf("Client exiting normally\n");
                        return;
                    }
                    return;
                }
            }
            else if (strncmp("ls", buffer, 2) == 0) {
                printf("Command string = %s", buffer);
                lsCommand();
            }
            else if (strcmp("rls", buffer) == 0) {
                printf("Command string = %s\n", buffer);
                write(socketfd, buffer, strlen(buffer));
                // get server responses
                while ((readResult = read(socketfd, buffer, max)) > 0) {
                    write(1, buffer, readResult);
                    break;
                }
            }
            else if(strncmp("cd" , buffer, 2) == 0) {
                printf("Command string = %s\n", buffer);
            }
            else {
                printf("Command string = %s\n", buffer);
                write(socketfd, buffer, strlen(buffer));
                // get server responses
                while ((readResult = read(socketfd, buffer, max)) > 0) {
                    write(1, buffer, readResult);
                    break;
                }
            }

            bzero(buffer, max); // empty buffer
            write(1, prompt, strlen(prompt));  // print the prompt
        }
    }
}

void lsCommand() {
    int pid = fork();

    if (pid != 0) {
        printf("Client parent waiting on child process %d to run ls locally\n", pid);
        wait(&pid);
        return;
    }
    else {
        printf("Client child process %d executing local ls | more\n", getpid());

        int fd[2], reader, writer;
        assert(pipe(fd) >= 0);
        reader = fd[0];
        writer = fd[1];

        printf("Child process %d starting more\n", getpid());
        int pid2 = fork();
        if (pid2 != 0) { // parent
            close(writer);

            // connect reader and take output from child
            dup2(reader, STDIN_FILENO);
            close(reader);
            wait(&pid2);

            char *arg[] = {"more", "-20", NULL};
            char *command = "more";

            if (execvp(command, arg) < 0) {
                printf("%s\n", strerror(errno));
            }
            printf("Error occurred during 'execvp'\n");
        }
        else {
            printf("Child process %d starting ls\n", getpid());

            // child process, run the 'ls -l' and redirect the output to stdin
            close(reader);
            dup2(writer, STDOUT_FILENO);  // connect stdout as the writer
            close(writer);

            char *arg[] = {"ls", "-l", NULL};
            char *command = "ls";
            if (execvp(command, arg) < 0) {
                printf("%s\n", strerror(errno));
            }
            printf("Error occurred during 'execvp'\n");
        }
    }
}
