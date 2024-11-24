#include <libwebsockets.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define PORT 9000
#define CANVAS_WIDTH 200
#define CANVAS_HEIGHT 200

// 전역 캔버스 데이터 (200x200 크기의 캔버스, 초기 색상은 흰색)
unsigned char canvas[CANVAS_WIDTH][CANVAS_HEIGHT][3] = {0};  // RGB로 저장

// 클라이언트 리스트
struct lws *clients[10]; // 최대 10명의 클라이언트를 추적 (예시로 10개로 설정)

// 웹소켓 처리 콜백 함수
static int callback_websocket(struct lws *wsi, enum lws_callback_reasons reason,
                              void *user, void *in, size_t len) {
    switch (reason) {
        case LWS_CALLBACK_ESTABLISHED:
            // 새 클라이언트 연결 시 클라이언트 리스트에 추가
            for (int i = 0; i < 10; i++) {
                if (clients[i] == NULL) {
                    clients[i] = wsi;
                    break;
                }
            }
            printf("웹소켓 연결이 성공적으로 이루어졌습니다.\n");
            break;

        case LWS_CALLBACK_RECEIVE:
            // 클라이언트로부터 픽셀 정보 수신 (좌표 + 색상)
            if (len == 5) {
                int x = ((unsigned char*)in)[0];   // x 좌표
                int y = ((unsigned char*)in)[1];   // y 좌표
                unsigned char r = ((unsigned char*)in)[2];  // R 색상
                unsigned char g = ((unsigned char*)in)[3];  // G 색상
                unsigned char b = ((unsigned char*)in)[4];  // B 색상

                // 캔버스에 색상 정보 업데이트
                if (x >= 0 && x < CANVAS_WIDTH && y >= 0 && y < CANVAS_HEIGHT) {
                    canvas[x][y][0] = r;
                    canvas[x][y][1] = g;
                    canvas[x][y][2] = b;

                    // 모든 클라이언트에게 캔버스 정보 전송
                    unsigned char update[5] = {x, y, r, g, b};
                    for (int i = 0; i < 10; i++) {
                        if (clients[i] != NULL) {
                            lws_write(clients[i], update, sizeof(update), LWS_WRITE_BINARY);
                        }
                    }
                }
            }
            break;

        case LWS_CALLBACK_CLOSED:
            // 클라이언트 연결 종료 시 클라이언트 리스트에서 제거
            for (int i = 0; i < 10; i++) {
                if (clients[i] == wsi) {
                    clients[i] = NULL;
                    break;
                }
            }
            printf("웹소켓 연결이 종료되었습니다.\n");
            break;

        default:
            break;
    }
    return 0;
}

// 웹소켓 프로토콜 정의
static struct lws_protocols protocols[] = {
    {
        "http-only", // 프로토콜 이름
        callback_websocket, // 콜백 함수
        0
    },
    { NULL, NULL, 0 } // 마지막에 NULL 프로토콜 추가
};

int main() {
    struct lws_context_creation_info info;
    struct lws_context *context;

    // 초기화
    memset(&info, 0, sizeof(info));
    info.port = PORT;  // 서버 포트 설정
    info.protocols = protocols;  // 사용할 프로토콜 등록

    // 컨텍스트 생성
    context = lws_create_context(&info);
    if (context == NULL) {
        fprintf(stderr, "웹소켓 서버 초기화 실패\n");
        return -1;
    }

    // 서버 실행
    printf("웹소켓 서버가 포트 %d에서 실행 중...\n", PORT);
    while (1) {
        lws_service(context, 100);
    }

    lws_context_destroy(context);
    return 0;
}
