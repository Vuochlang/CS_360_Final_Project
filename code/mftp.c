#include "mftp.h"

void createClient();

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
                printf("Command string = 'exit'\n");
                printf("Exit command encountered\n");
                bzero(buffer, max);

                write(socketfd, "Q", 1);
                printf("Awaiting server response\n");
                while ((readResult = read(socketfd, buffer, max)) > 0) {
                    if (strcmp(buffer, "A") == 0) {
                        printf("Received server response: '%s'\n", buffer);
                        printf("Client exiting normally\n");
                        return;
                    }
                }
            }
            else {
                write(socketfd, buffer, strlen(buffer));
            }

            bzero(buffer, max);
            // get server responses
            while ((readResult = read(socketfd, buffer, max)) > 0) {
                write(1, buffer, readResult);
                break;
            }
            bzero(buffer, max);
            write(1, prompt, strlen(prompt));
        }
    }
}
