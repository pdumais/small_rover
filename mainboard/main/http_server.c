#include "http_server.h"
#include <esp_log.h>
#include "common.h"
#include "taskpool.h"

static httpd_handle_t server = NULL;
static const char *TAG = "http_server_c";

typedef struct
{
    int fd;
    size_t len;
    char data[];
} ws_client_t;

static ws_client_t ws_clients[MAX_WS_CLIENTS];

httpd_handle_t get_webserver()
{
    return server;
}

static void async_ws_send(ws_client_t *swc)
{
    int fd = swc->fd;
    httpd_ws_frame_t pkt;
    memset(&pkt, 0, sizeof(httpd_ws_frame_t));
    pkt.payload = (uint8_t *)swc->data;
    pkt.len = swc->len;
    pkt.type = HTTPD_WS_TYPE_TEXT;

    httpd_ws_send_frame_async(server, fd, &pkt);

    free(swc);
}

// ctx == 1 for telemetry, 2 for logs
void broadcast_ws(const char *data, int ctx)
{
    int fds[MAX_WS_CLIENTS] = {0};
    size_t client_count = MAX_WS_CLIENTS;
    size_t len = strlen(data);
    esp_err_t err = httpd_get_client_list(server, &client_count, (int *)&fds);
    for (int i = 0; i < client_count; i++)
    {
        int fd = fds[i];
        int* user_ctx = httpd_sess_get_ctx(server, fd);
        if (user_ctx == 0 || *user_ctx != ctx)
        {
            continue;
        }
        if (httpd_ws_get_fd_info(server, fd) == HTTPD_WS_CLIENT_WEBSOCKET)
        {
            ws_client_t *swc = malloc(sizeof(ws_client_t) + len + 1);
            swc->fd = fd;
            swc->len = len;
            strcpy(swc->data, data);
            if (httpd_queue_work(server, async_ws_send, swc) != ESP_OK)
            {
                free(swc);
            }
        }
    }
}


void stop_webserver()
{
    if (server)
    {
        stop_pool();
        /* Stop the httpd server */
        httpd_stop(server);
    }
}

typedef struct
{
    httpd_req_t *req;
    httpd_req_handler_t handler;
} async_http_handler_t;

esp_err_t http_async_handler(async_http_handler_t *arg)
{
    arg->handler(arg->req);
    httpd_req_async_handler_complete(arg->req);
    free(arg);

    return ESP_OK;
}

esp_err_t submit_async_req(httpd_req_t *req, httpd_req_handler_t handler)
{
    httpd_req_t *copy = 0;
    esp_err_t err = httpd_req_async_handler_begin(req, &copy);
    if (err != ESP_OK)
        return err;

    async_http_handler_t *ah = malloc(sizeof(async_http_handler_t));
    ah->req = copy;
    ah->handler = handler;

    if (schedule_task(http_async_handler, ah) != ESP_OK)
    {
        httpd_req_async_handler_complete(copy); // cleanup
        return ESP_FAIL;
    }

    return ESP_OK;
}

static void free_func(void *ctx)
{
    free(ctx);
}

static esp_err_t ws_handler(httpd_req_t *req)
{
    if (req->method == HTTP_GET)
    {
        req->sess_ctx = malloc(sizeof(int));
        *(int*)req->sess_ctx = 1;
        req->free_ctx = free_func;
        return ESP_OK;
    }
    uint8_t buf[256] = {0};
    httpd_ws_frame_t pkt;
    pkt.payload = buf;

    httpd_ws_recv_frame(req, &pkt, sizeof(buf) - 1);
    if (!pkt.len)
        return ESP_FAIL;

    buf[pkt.len] = 0;

    return ESP_OK;
}

static esp_err_t ws_logs_handler(httpd_req_t *req)
{
    if (req->method == HTTP_GET)
    {
        req->sess_ctx = malloc(sizeof(int));
        *(int*)req->sess_ctx = 2;
        req->free_ctx = free_func;
        return ESP_OK;
    }

    uint8_t buf[256] = {0};
    httpd_ws_frame_t pkt;
    pkt.payload = buf;

    httpd_ws_recv_frame(req, &pkt, sizeof(buf) - 1);
    if (!pkt.len)
        return ESP_FAIL;

    buf[pkt.len] = 0;

    return ESP_OK;
}

void init_ws()
{
    for (int i = 0; i < MAX_WS_CLIENTS; i++)
        ws_clients[i].fd = 0;

    httpd_uri_t h = {0};
    h.uri = "/telemetry";
    h.method = HTTP_GET;
    h.handler = ws_handler;
    h.is_websocket = true;
    h.handle_ws_control_frames = false;
    httpd_register_uri_handler(server, &h);

    httpd_uri_t h2 = {0};
    h2.uri = "/logs";
    h2.method = HTTP_GET;
    h2.handler = ws_logs_handler;
    h2.is_websocket = true;
    h2.handle_ws_control_frames = false;
    httpd_register_uri_handler(server, &h2);
}

// This is used for the Provisioning module
void register_uri(const char *uri, httpd_method_t method, esp_err_t (*handler)(httpd_req_t *r))
{
    httpd_uri_t h;
    h.uri = uri;
    h.method = method;
    h.handler = handler;
    h.user_ctx = 0;

    httpd_register_uri_handler(server, &h);
}

void start_webserver()
{
    start_pool();

    /* Generate default configuration */
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.uri_match_fn = httpd_uri_match_wildcard;

    /* Start the httpd server */
    int err = httpd_start(&server, &config);
    if (err != ESP_OK)
    {
        ESP_LOGI(TAG, "start_webserver error: %04X", err);
        return;
    }

}

