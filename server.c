#include "server.h"
#include "clientmanager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <fcntl.h>


#define PORT 8080
//#define BUFFER_SIZE 4096
#define FILE_PATH "ex.json"
#define HTML_FILE "cli.html"
//#define MAX_CLIENTS 1000

// 파일 디스크립터를 non-blocking 모드로 설정하는 함수
int set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0); 
    if (flags == -1) {
        perror("fcntl F_GETFL");
        return -1;
    }
    flags |= O_NONBLOCK;  
    if (fcntl(fd, F_SETFL, flags) == -1) {
        perror("fcntl F_SETFL");
        return -1;
    }
    return 0;
}

// 서버 초기화 함수
void init_server(ServerManager *sm) {
    sm->server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (sm->server_socket == -1) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    if (set_nonblocking(sm->server_socket) == -1) {
        perror("set_nonblocking failed");
        close(sm->server_socket);
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(sm->server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind failed");
        close(sm->server_socket);
        exit(EXIT_FAILURE);
    }

    if (listen(sm->server_socket, MAX_CLIENTS) == -1) {
        perror("listen failed");
        close(sm->server_socket);
        exit(EXIT_FAILURE);
    }

    sm->epoll_fd = epoll_create1(0);
    if (sm->epoll_fd == -1) {
        perror("epoll_create1 failed");
        close(sm->server_socket);
        exit(EXIT_FAILURE);
    }

    sm->ev.events = EPOLLIN | EPOLLET;
    sm->ev.data.fd = sm->server_socket;
    if (epoll_ctl(sm->epoll_fd, EPOLL_CTL_ADD, sm->server_socket, &sm->ev) == -1) {
        perror("epoll_ctl: server_socket");
        close(sm->server_socket);
        close(sm->epoll_fd);
        exit(EXIT_FAILURE);
    }

    printf("Success Init Server!\nServer is listening on port %d\n", PORT);
}

// HTTP 요청에 대한 파일 전송 처리
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

    char header[BUFFER_SIZE];
    snprintf(header, sizeof(header),
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: %s\r\n"
             "Connection: close\r\n"
             "\r\n",
             content_type);
    send(client_socket, header, strlen(header), 0);

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
        // 파일이 없거나 비어 있는 경우 새로 생성
        json_file = fopen(FILE_PATH, "w");
        fprintf(json_file, "[\n  %s\n]\n", data);
        fclose(json_file);
        return;
    }

    // 파일 내용 확인
    fseek(json_file, 0, SEEK_END);
    long file_size = ftell(json_file);
    fseek(json_file, 0, SEEK_SET);

    if (file_size == 0) {
        // 파일이 비어 있으면 새 JSON 배열로 초기화
        fprintf(json_file, "[\n  %s\n]\n", data);
    } else {
        // 파일이 비어 있지 않으면 끝에서 두 번째 문자(닫는 대괄호 앞)로 이동
        fseek(json_file, -2, SEEK_END);

        // 닫는 대괄호가 없을 경우 추가
        char last_char;
        fread(&last_char, 1, 1, json_file);
        if (last_char != ']') {
            fprintf(json_file, "]\n");
        }

        // 새 데이터를 추가
        fseek(json_file, -2, SEEK_END);
        fprintf(json_file, ",\n  %s\n]", data);
    }
    fclose(json_file);
}



// HTTP 요청 처리
void handle_http_request(int client_socket, const char *buffer) {
    if (strncmp(buffer, "GET /favicon.ico", 16) == 0) {
        const char *response =
            "HTTP/1.1 204 No Content\r\n"
            "Content-Type: image/x-icon\r\n"
            "Connection: close\r\n"
            "\r\n";
        send(client_socket, response, strlen(response), 0);
    } else if (strncmp(buffer, "GET /get-existing-pixels", 24) == 0) {
        send_file_content(client_socket, FILE_PATH, "application/json");
    } else if (strncmp(buffer, "GET / ", 6) == 0 || strncmp(buffer, "GET / HTTP", 10) == 0) {
        send_file_content(client_socket, HTML_FILE, "text/html");
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
            send(client_socket, response, strlen(response), 0);
        }
    } else {
        printf("알 수 없는 요청입니다.\n");
    }
}

// 서버 이벤트 루프 함수 추가
void server_event_loop(ServerManager *sm) {
    struct epoll_event events[MAX_CLIENTS];
    char buffer[BUFFER_SIZE];

    while (1) {
        int nfds = epoll_wait(sm->epoll_fd, events, MAX_CLIENTS, -1);
        for (int i = 0; i < nfds; i++) {
            if (events[i].data.fd == sm->server_socket) {
                accept_new_client(sm);
            } else {
                memset(buffer, 0, BUFFER_SIZE);
                int client_socket = events[i].data.fd;
                int bytes_read = read(client_socket, buffer, BUFFER_SIZE - 1);
                if (bytes_read > 0) {
                    handle_http_request(client_socket, buffer);
                }
                close(client_socket);
            }
        }
    }
}


// 서버 종료 함수
void close_server(ServerManager *sm){
    close(sm->server_socket);
    close(sm->epoll_fd);
}

// 새로운 클라이언트를 ACCEPT하는 함수
int accept_new_client(ServerManager *sm)
{
    // 클라이언트 연결 수락
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int client_socket = accept(sm->server_socket, (struct sockaddr*)&client_addr, &client_addr_len);

    if (client_socket == -1) {
        perror("accept failed");
        return -1;
    }

    // 클라이언트를 non-blocking 모드로 설정
    if (set_nonblocking(client_socket) == -1) {
        close(client_socket);
        return -1;
    }

    // 클라이언트를 epoll 이벤트에 등록
    sm->ev.events = EPOLLIN | EPOLLET; 
    sm->ev.data.fd = client_socket;
    if (epoll_ctl(sm->epoll_fd, EPOLL_CTL_ADD, client_socket, &sm->ev) == -1) {
        perror("epoll_ctl add client failed");
        close(client_socket);
    }

    // 클라이언트 연결 성공 메시지 출력
    printf("Accepted new client: %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port)); 
    return 0;
}