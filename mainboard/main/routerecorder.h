#pragma once
#include <stdbool.h>
#include "common.h"

void record_init();
void record_toggle_record();
void record_toggle_replay();
bool record_is_recording();
bool record_is_replaying();
void record_process(metrics_t *m);