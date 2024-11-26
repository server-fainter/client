//http통신 관리 함수 선언
#define HTTP_UTILS_H

// HTTP 응답 생성 함수 선언
void send_response(int client_socket, const char *status, const char *body);

// POST 요청 처리 함수 선언
void handle_post_request(int client_socket, const char *data);
