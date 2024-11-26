#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include "http_utils.h"

#define PORT 8080
#define MAX_BUF_SIZE 1024

int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    char buffer[MAX_BUF_SIZE];
    ssize_t received_len;

    // 소켓 생성
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // 서버 주소 설정
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // 소켓에 주소 바인딩
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    // 클라이언트 연결 대기
    if (listen(server_socket, 10) < 0) {
        perror("Listen failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", PORT);

    while (1) {
        // 클라이언트 연결 수락
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_socket < 0) {
            perror("Client connection failed");
            continue;
        }

        // HTTP 요청 받기
        memset(buffer, 0, sizeof(buffer));
        received_len = read(client_socket, buffer, sizeof(buffer) - 1);
        if (received_len < 0) {
            perror("Read failed");
            close(client_socket);
            continue;
        }

        // POST 요청인지 확인
        if (strstr(buffer, "POST") != NULL) {
            // POST 본문을 찾기 위해 요청 헤더 부분은 제외하고 본문만 추출
            char *body = strstr(buffer, "\r\n\r\n");
            if (body != NULL) {
                body += 4; // 본문 시작 부분으로 이동
                handle_post_request(client_socket, body);
            } else {
                send_response(client_socket, "400 Bad Request", "{\"status\": \"error\", \"message\": \"No body in request\"}");
            }
        } else {
            send_response(client_socket, "405 Method Not Allowed", "{\"status\": \"error\", \"message\": \"Only POST requests allowed\"}");
        }

        // 클라이언트 연결 종료
        close(client_socket);
    }

    // 서버 종료
    close(server_socket);
    return 0;
}
