#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 5001

#define BUFFER_SIZE 4096
#define FILE_PATH "ex.json"
#define HTML_FILE "client1.html"

// 파일 내용을 읽어 클라이언트에 전송
void send_file_content(int client_socket, const char *filename, const char *content_type) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        const char *response =
            "HTTP/1.1 404 Not Found\r\n"
            "Content-Type: text/plain\r\n"
            "Connection: close\r\n"
            "\r\n"
            "404 Not Found";
        send(client_socket, response, strlen(response), 0);
        return;
    }

    // 응답 헤더 전송
    char header[BUFFER_SIZE];
    snprintf(header, sizeof(header),
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: %s\r\n"
             "Connection: close\r\n"
             "\r\n",
             content_type);
    send(client_socket, header, strlen(header), 0);

    // 파일 내용 전송
    char buffer[BUFFER_SIZE];
    size_t bytes;
    while ((bytes = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        send(client_socket, buffer, bytes, 0);
    }
    fclose(file);
}

// JSON 데이터 저장
void append_to_json_file(const char *data) {
    FILE *json_file = fopen(FILE_PATH, "r+");
    if (json_file == NULL) {
        json_file = fopen(FILE_PATH, "w");
        fprintf(json_file, "[\n  %s\n]\n", data);
        fclose(json_file);
        return;
    }

    fseek(json_file, -2, SEEK_END);
    fprintf(json_file, ",\n  %s\n]", data);
    fclose(json_file);
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE] = {0};

    // 소켓 생성
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("소켓 실패");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("바인딩 실패");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("리스닝 실패");
        exit(EXIT_FAILURE);
    }

    printf("C 서버가 포트 %d에서 실행 중입니다.\n", PORT);

    while (1) {
        printf("클라이언트를 기다리는 중...\n");
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
            perror("연결 실패");
            exit(EXIT_FAILURE);
        }

        printf("클라이언트가 연결되었습니다.\n");
        memset(buffer, 0, BUFFER_SIZE);
        read(new_socket, buffer, BUFFER_SIZE);

        if (strncmp(buffer, "GET /favicon.ico", 16) == 0) {
            const char *response =
                "HTTP/1.1 204 No Content\r\n"
                "Content-Type: image/x-icon\r\n"
                "Connection: close\r\n"
                "\r\n";
            send(new_socket, response, strlen(response), 0);
        } else if (strncmp(buffer, "GET /get-existing-pixels", 24) == 0) {
            send_file_content(new_socket, FILE_PATH, "application/json");
        } else if (strncmp(buffer, "GET / ", 6) == 0 || strncmp(buffer, "GET / HTTP", 10) == 0) {
            send_file_content(new_socket, HTML_FILE, "text/html");
        } else if (strncmp(buffer, "POST", 4) == 0) {
            char *body = strstr(buffer, "\r\n\r\n");
            if (body != NULL) {
                body += 4;
                printf("POST 요청 데이터: %s\n", body);
                append_to_json_file(body);
                const char *response =
                    "HTTP/1.1 200 OK\r\n"
                    "Access-Control-Allow-Origin: *\r\n"
                    "Content-Type: application/json\r\n"
                    "Connection: close\r\n"
                    "\r\n"
                    "{ \"status\": \"success\" }";
                send(new_socket, response, strlen(response), 0);
            }
        } else {
            printf("알 수 없는 요청입니다.\n");
        }

        close(new_socket);
    }

    return 0;
}
