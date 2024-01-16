#pragma once
#include "common.h"

#define WIFI_STATE_NONE 0
#define WIFI_STATE_AP 1
#define WIFI_STATE_CONNECTING 2
#define WIFI_STATE_CONNECTED 3

bool wifi_init();
void broadcast_metric(metrics_t *m);