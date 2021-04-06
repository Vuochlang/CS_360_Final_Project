#include "mftp.h"

void createClient(char*);

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Error: ./mftp HOST_NAME\n");
        return 0;
    }

    char hostAddress[20];
    strcpy(hostAddress, argv[1]);
    printf("hostname = %s\n", hostAddress);

    createClient(hostAddress);

    return 0;
}

void createClient(char* hostAddress) {
    int max = 200;
    char buffer[max];
    char messageRead[max];

    int socketfd;
    struct addrinfo hints, *actualdata;
    memset(&hints, 0, sizeof(hints));
    int err;

    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_INET;
    err = getaddrinfo(hostAddress, "49999", &hints, &actualdata);
    if (err != 0) {
        fprintf(stderr, "Error: %s\n", gai_strerror(err));
        exit(1);
    }

    socketfd = socket(actualdata -> ai_family, actualdata -> ai_socktype, 0);
    if (socketfd < 0) {
        fprintf(stderr, "Error: %s\n", strerror(errno));
        exit(1);
    }

    if (connect(socketfd, actualdata -> ai_addr, actualdata -> ai_addrlen) < 0) {
        fprintf(stderr, "Error: %s\n", strerror(errno));
        exit(1);
    }

    int readResult = 0;
    bool isExit = false;
    char send[] = "TO SERVER: ";

    // get connection message
    while ((readResult = read(socketfd, buffer, max)) > 0) {
        write(1, buffer, strlen(buffer));
        break;
    }
    bzero(buffer, max);

    while (!isExit) {
        printf("\n");
        write(1, send, strlen(send));

        // read from command line
        while ((readResult = read(0, buffer, max)) > 0) {
            if (strncmp("exit", buffer, 4) == 0) {
                write(socketfd, "Q", 1);
                while ((readResult = read(socketfd, buffer, max)) > 0) {
                    write(1, buffer, strlen(buffer));
                    break;
                }
                return;
            }
            else {
                write(socketfd, buffer, strlen(buffer));
            }

            bzero(buffer, max);
            // get server responses
            while ((readResult = read(socketfd, buffer, max)) > 0) {
                write(1, buffer, strlen(buffer));
                break;
            }
            bzero(buffer, max);
            write(1, send, strlen(send));
        }
    }
}