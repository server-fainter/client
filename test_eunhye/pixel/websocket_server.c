#include <json-c/json.h>  // JSON 파싱을 위해 추가

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
            // JSON 파싱을 위해 문자열 처리
            char *message = (char*)in;
            struct json_object *parsed_json;
            parsed_json = json_tokener_parse(message);

            struct json_object *x_obj, *y_obj, *color_obj;
            json_object_object_get_ex(parsed_json, "x", &x_obj);
            json_object_object_get_ex(parsed_json, "y", &y_obj);
            json_object_object_get_ex(parsed_json, "color", &color_obj);

            int x = json_object_get_int(x_obj);
            int y = json_object_get_int(y_obj);
            struct json_object *r_obj, *g_obj, *b_obj;
            json_object_object_get_ex(color_obj, "r", &r_obj);
            json_object_object_get_ex(color_obj, "g", &g_obj);
            json_object_object_get_ex(color_obj, "b", &b_obj);

            unsigned char r = json_object_get_int(r_obj);
            unsigned char g = json_object_get_int(g_obj);
            unsigned char b = json_object_get_int(b_obj);

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
