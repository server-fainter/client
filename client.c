#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SER_PORT 8080
#define SER_IP "127.0.0.1"

int main() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("Socket creation failed");
        return -1;
    }

    struct sockaddr_in ser_addr;
    memset(&ser_addr, 0, sizeof(ser_addr));
    ser_addr.sin_family = AF_INET;
    ser_addr.sin_port = htons(SER_PORT);
    ser_addr.sin_addr.s_addr = inet_addr(SER_IP);

    if (connect(sock, (struct sockaddr*)&ser_addr, sizeof(ser_addr)) == -1) {
        perror("Connection failed");
        close(sock);
        return -1;
    }

    printf("Connected to server at %s:%d\n", SER_IP, SER_PORT);

    // 현재 경로를 얻어서 WSL에서 Windows 경로로 변환하여 브라우저 open
    printf("Opening browser...\n");

    // getcwd()로 현재 경로 얻기
    char cwd[256];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        perror("getcwd failed");
        close(sock);
        return -1;
    }

    char exec[100];
    snprintf(exec, sizeof(exec), "cmd.exe /C start http://127.0.0.1:8080");

    // 명령어 실행
    system(exec);

    close(sock);
    return 0;
}
