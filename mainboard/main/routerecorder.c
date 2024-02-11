#include "routerecorder.h"
#include <esp_log.h>
#include <esp_timer.h>
#include <esp_event.h>
#include <esp_system.h>

#include "wifi.h"
#include <stdio.h>
#include <stdbool.h>
#include "hardware.h"

#define SAMPLE_LIST_COUNT 300
static const char *TAG = "recorder_c";

static volatile bool recording = false;
static volatile bool replaying = false;
static unsigned long last_time_stamp = 0;
static uint16_t route_index = 0;
static TaskHandle_t replay_task_h;

typedef struct
{
    uint32_t ms_delta_since_last_sample;
    uint16_t throttle;
    int8_t direction;
    int8_t steering_angle;

} route_sample;

route_sample route[SAMPLE_LIST_COUNT];

void replay_task(void *arg)
{
    static uint16_t replay_cursor = 0;
    const char str[100];

    while (1)
    {
        if (!replaying)
        {
            replay_cursor = 0;
            vTaskSuspend(0);
            continue;
        }

        // We have the number of microseconds, vTaskDelay wants ticks
        vTaskDelay(route[replay_cursor].ms_delta_since_last_sample / portTICK_PERIOD_MS);
        if (!replaying) // Check again in case we woke up because of timer abort.
        {
            replay_cursor = 0;
            vTaskSuspend(0);
            continue;
        }

        sprintf(&str, "Replaying event at slot %i\n", replay_cursor);
        broadcast_log(&str);
        process_throttle(route[replay_cursor].throttle, route[replay_cursor].direction);
        process_steering(route[replay_cursor].steering_angle);

        replay_cursor++;
        if (replay_cursor >= route_index)
        {
            broadcast_log("Stoping Replay mode. End of route\n");
            queue_msg msg;
            msg.type = MESSAGE_REPLAY_END;
            hardware_send_message(&msg);

            replaying = false;
            replay_cursor = 0;
            vTaskSuspend(0);
            continue;
        }
    }
}

bool record_is_recording()
{
    return recording;
}

bool record_is_replaying()
{
    return replaying;
}

void record_init()
{
    ESP_LOGI(TAG, "Recorder buffer size=%i bytes", sizeof(route));
    xTaskCreate(replay_task, "replay_task", 4096, NULL, 5, &replay_task_h);
    vTaskSuspend(replay_task_h);
}

void record_start()
{
    if (recording || replaying)
    {
        return;
    }

    broadcast_log("Starting record\n");
    last_time_stamp = esp_timer_get_time();
    route_index = 0;
    recording = true;
}
void record_stop()
{
    if (!recording)
    {
        return;
    }

    broadcast_log("Stopping record\n");
    recording = false;
}

void replay_start()
{
    if (recording || replaying || (route_index == 0))
    {
        return;
    }

    broadcast_log("Starting Replay mode\n");
    replaying = true;
    vTaskResume(replay_task_h);
}

void replay_stop()
{
    if (!replaying)
    {
        return;
    }

    queue_msg msg;
    msg.type = MESSAGE_REPLAY_END;
    hardware_send_message(&msg);
    broadcast_log("Stoping Replay mode\n");
    replaying = false;
    xTaskAbortDelay(replay_task_h);
}

void record_toggle_record()
{
    if (recording)
    {
        record_stop();
    }
    else
    {
        record_start();
    }
}

void record_toggle_replay()
{
    if (replaying)
    {
        replay_stop();
    }
    else
    {
        replay_start();
    }
}

void record_process(metrics_t *m)
{
    char str[100];

    if (!recording)
    {
        return;
    }

    if (route_index >= SAMPLE_LIST_COUNT)
    {
        broadcast_log("Recording size reached. Stopping record\n");
        recording = false;
        return;
    }

    // TODO: should not process steering events if not moving
    if (route_index > 0)
    {
        // sprintf(&str, "%i/%i %i/%i %i/%i", route[route_index - 1].throttle, m->throttle, route[route_index - 1].direction, m->direction, route[route_index - 1].steering_angle, m->steering_angle);
        // broadcast_log(&str);
        if (
            route[route_index - 1].throttle == m->throttle &&
            route[route_index - 1].direction == m->direction &&
            route[route_index - 1].steering_angle == m->steering_angle)
        {
            // Discard identical events
            return;
        }
    }

    unsigned long ts = esp_timer_get_time();
    unsigned long delta = (ts - last_time_stamp) / 1000; // TODO: can this overflow?
    last_time_stamp = ts;

    sprintf(&str, "Recording event at slot %i, ts=%lu\n", route_index, delta);
    broadcast_log(&str);
    route[route_index].ms_delta_since_last_sample = delta;
    route[route_index].throttle = m->throttle;
    route[route_index].direction = m->direction;
    route[route_index].steering_angle = m->steering_angle;
    route_index++;
}
