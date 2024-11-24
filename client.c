#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080
#define BUFFER_SIZE 1024

int main() {
    int sockfd;
    struct sockaddr_in server_addr;
    char send_buffer[BUFFER_SIZE];
    char recv_buffer[BUFFER_SIZE];

    // 소켓 생성
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // 서버 주소 설정
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("Invalid server IP address");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // 서버 연결
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Connection failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("Connected to server\n");

    // 서버와 통신
    while (1) {
        printf("Enter message to send: ");
        fgets(send_buffer, BUFFER_SIZE, stdin);
        send_buffer[strcspn(send_buffer, "\n")] = '\0';

        // 종료 조건
        if (strcmp(send_buffer, "exit") == 0) {
            printf("Exiting client...\n");
            break;
        }

        send(sockfd, send_buffer, strlen(send_buffer), 0);

        int bytes_received = recv(sockfd, recv_buffer, BUFFER_SIZE - 1, 0);
        if (bytes_received <= 0) {
            printf("Server disconnected\n");
            break;
        }

        recv_buffer[bytes_received] = '\0';
        printf("Server: %s\n", recv_buffer);
    }

    close(sockfd);
    return 0;
}

