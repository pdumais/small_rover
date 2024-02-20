#pragma once
#include <esp_http_server.h>
#include "cJSON.h"

#define MAX_WS_CLIENTS 10

typedef int (*jsonrpchandler)(const char *method, cJSON *params, cJSON *ret);
typedef esp_err_t (*httpd_req_handler_t)(httpd_req_t *req);

void start_webserver();
void stop_webserver();
httpd_handle_t get_webserver();
void register_uri(const char *uri, httpd_method_t method, esp_err_t (*handler)(httpd_req_t *r));
void register_jsonrpc_handler(jsonrpchandler handler);
esp_err_t submit_async_req(httpd_req_t *req, httpd_req_handler_t handler);
void init_ws();
void broadcast_ws(const char *data, int ctx);