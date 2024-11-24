#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libwebsockets.h>

#define PORT 5001
#define FILE_PATH "ex.json"
#define HTML_FILE "client1.html"

#define BUFFER_SIZE 4096

// 파일 내용을 읽어 클라이언트에 전송
void send_file_content(struct lws *wsi, const char *filename, const char *content_type) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        const char *response =
            "HTTP/1.1 404 Not Found\r\n"
            "Content-Type: text/plain\r\n"
            "Connection: close\r\n"
            "\r\n"
            "404 Not Found";
        lws_write(wsi, (unsigned char *)response, strlen(response), LWS_WRITE_TEXT);
        return;
    }

    // 파일 내용 전송
    char buffer[BUFFER_SIZE];
    size_t bytes;
    while ((bytes = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        lws_write(wsi, (unsigned char *)buffer, bytes, LWS_WRITE_TEXT);
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
    //밑에서 두번째 줄에 fp 옮김
    fseek(json_file, -2, SEEK_END);
    //거따가 
    fprintf(json_file, ",\n  %s\n]", data);
    fclose(json_file);
}

// 웹소켓 핸들러
static int callback(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len) {
    switch (reason) {
        case LWS_CALLBACK_ESTABLISHED:
            printf("웹소켓 연결이 성공적으로 이루어졌습니다.\n");
            break;
        case LWS_CALLBACK_RECEIVE:
            {
                char *message = (char *)in;
                if (strncmp(message, "GET /get-existing-pixels", 24) == 0) {
                    send_file_content(wsi, FILE_PATH, "application/json");
                } else if (strncmp(message, "GET / ", 6) == 0 || strncmp(message, "GET / HTTP", 10) == 0) {
                    send_file_content(wsi, HTML_FILE, "text/html");
                } else if (strncmp(message, "POST", 4) == 0) {
                    printf("POST 요청 데이터: %s\n", message);
                    append_to_json_file(message);
                    const char *response = 
                        "{ \"status\": \"success\" }";
                    lws_write(wsi, (unsigned char *)response, strlen(response), LWS_WRITE_TEXT);
                } else {
                    printf("알 수 없는 요청입니다.\n");
                }
            }
            break;
        case LWS_CALLBACK_CLOSED:
            printf("웹소켓 연결이 종료되었습니다.\n");
            break;
        default:
            break;
    }
    return 0;
}

// 웹소켓 프로토콜 설정
static struct lws_protocols protocols[] = {
    {
        .name = "http", 
        .callback = callback,
        .per_session_data_size = 0,
        .rx_buffer_size = 0,
    },
    { NULL, NULL, 0, 0 } // 끝을 나타내는 NULL 프로토콜
};

int main() {

    //웹소켓 디스크립터
    struct lws_context_creation_info info;
    //서버 상태 및 실행 컨텍스트 구조체
    struct lws_context *context;
    memset(&info, 0, sizeof(info));

    //디스크립터에 정보 저장
    info.port = PORT;
    info.protocols = protocols;

    //서버상 그룹 및 사용자 id 설정
    //-1은 기본 설정 사용한단 뜻
    info.gid = -1;
    info.uid = -1;
    info.iface = NULL;

    // 웹소켓 서버 컨텍스트 생성
    context = lws_create_context(&info);
    if (context == NULL) {
        fprintf(stderr, "웹소켓 서버 생성 실패\n");
        return -1;
    }

    printf("웹소켓 서버가 포트 %d에서 실행 중입니다.\n", PORT);

    // 서버 실행 루프
    while (1) {
        //서버의 이벤트 처리 함수
        // 서버 컨텍스트, 대기 시간(100ms후 이벤트 확인)
        lws_service(context, 100);
    }
    //서버 컨텍스트 해제 및 종료
    lws_context_destroy(context);
    return 0;
}
