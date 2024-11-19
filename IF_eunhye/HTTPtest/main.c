/*
멀티프로세스로 구현한  TCP 소켓 
*/
#define BUF_SIZE 1000
#define HEADER_FMT "HTTP/1.1 %d %s\nContent-Length: %ld\nContent-Type: %s\n\n"

#define NOT_FOUND_CONTENT       "<h1>404 Not Found</h1>\n"
#define SERVER_ERROR_CONTENT    "<h1>500 Internal Server Error</h1>\n"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <arpa/inet.h>


/*
    @func   assign address to the created socket lsock(sd)
    @return bind() return value
*/
int bind_lsock(int lsock, int port) {
    struct sockaddr_in sin;

    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    sin.sin_port = htons(port);

    return bind(lsock, (struct sockaddr *)&sin, sizeof(sin));
}

/* ...skip: codes related to HTTP... */

int main(int argc, char **argv) {
    int port, pid;
    int lsock, asock;

    struct sockaddr_in remote_sin;
    socklen_t remote_sin_len;

    if (argc < 2) {
        printf("Usage: \n");
        printf("\t%s {port}: runs mini HTTP server.\n", argv[0]);
        exit(0);
    }

    port = atoi(argv[1]);
    printf("[INFO] The server will listen to port: %d.\n", port);

    lsock = socket(AF_INET, SOCK_STREAM, 0);
    if (lsock < 0) {
        perror("[ERR] failed to create lsock.\n");
        exit(1);
    }

    if (bind_lsock(lsock, port) < 0) {
        perror("[ERR] failed to bind lsock.\n");
        exit(1);
    }

    if (listen(lsock, 10) < 0) {
        perror("[ERR] failed to listen lsock.\n");
        exit(1);
    }

    // to handle zombie process
    signal(SIGCHLD, SIG_IGN);

    while (1) {
        printf("[INFO] waiting...\n");
        asock = accept(lsock, (struct sockaddr *)&remote_sin, &remote_sin_len);
        if (asock < 0) {
            perror("[ERR] failed to accept.\n");
            continue;
        }

        pid = fork();
        if (pid == 0) {
            close(lsock); http_handler(asock); close(asock);
            exit(0);
        }

        if (pid != 0)   { close(asock); }
        if (pid < 0)    { perror("[ERR] failed to fork.\n"); }
    }
}