#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "http_utils.h"

// HTTP 응답 생성 함수
void send_response(int client_socket, const char *status, const char *body) {
    char response[1024];
    snprintf(response, sizeof(response),
             "HTTP/1.1 %s\r\n"
             "Content-Type: application/json\r\n"
             "Content-Length: %zu\r\n"
             "\r\n"
             "%s",
             status, strlen(body), body);
    write(client_socket, response, strlen(response));
}

// POST 요청 처리 함수
void handle_post_request(int client_socket, const char *data) {
    // 받은 데이터를 출력 (여기서 JSON 처리 가능)
    printf("Received data: %s\n", data);

    // JSON 응답 생성
    const char *json_response = "{\"status\": \"success\"}";
    
    // 응답 보내기
    send_response(client_socket, "200 OK", json_response);
}
