#include "obstructions.h"
#include <stdbool.h>
#include <esp_log.h>
#include "hw_def.h"
#include "hcsr04.h"
#include <esp_system.h>
#include <esp_event.h>
#include <esp_timer.h>
#include "servo.h"
#include "hardware.h"
#include "driver/gpio.h"
#include "wifi.h"

#define SWITCH1 0x01
#define SWITCH2 0x02
#define SWITCH3 0x04
#define SWITCH4 0x08
#define BUMPER_FRONT 1
#define BUMPER_REAR 2

static const char *TAG = "obstructions_c";

static int8_t angles[] = {-70, -60, -50, -40, -30, -20, -10, 0, 10, 20, 30};
#define SONAR_DISTANCE_COUNT sizeof(angles)
static volatile bool sweep_enabled = false;
static volatile int distances[SONAR_DISTANCE_COUNT] = {0};
static bool obstructed = false;
static hcsr04_handle_t ultrasonic;
static TaskHandle_t check_obstructions_task_handle;
static mcpwm_cmpr_handle_t comparator;
static int servo_angle = 0;

// an obstruction is detected while probing at angle "angle"
static void on_obstruction_detected()
{
    queue_msg msg = {
        .type = MESSAGE_OBSTRUCTION_DETECTED,
    };
    hardware_send_message(&msg);
}

static void on_obstruction_cleared()
{
    queue_msg msg = {
        .type = MESSAGE_OBSTRUCTION_CLEARED,
    };
    hardware_send_message(&msg);
}

static void on_collision(uint8_t position)
{
    queue_msg msg = {
        .type = MESSAGE_COLLISION,
        .data = (void *)position, // we just put the raw value instead of a pointer since 4 bytes is enough
    };
    hardware_send_message(&msg);
}

static uint16_t check_sonar(int wait_time)
{
    uint16_t val = hcsr04_read(&ultrasonic, wait_time);

    if (val != 0)
    {
        // if we get 0xFFFF, it means there was a timeout, so the distance was too far
        // broadcast_log("check_sonar2\n");
        return val;
    }
    broadcast_log("check_sonar3\n");
    return 0;
}

bool obstruction_is_too_close()
{
    for (int i = 0; i < SONAR_DISTANCE_COUNT; i++)
    {
        if (distances[i] == 0)
        {
            continue;
        }
        if (distances[i] <= MIN_OBSTRUCTION_DISTANCE)
        {
            // char str[256];
            // sprintf(&str, "d=%i, i=%i\n", distances[i], i);
            // broadcast_log(&str);
            return true;
        }
    }

    return false;
}

static void check_obstructions_task()
{
    /* probe obstructions at angle 0.
     * update the "obstructed" flag.
     * if a change of obstruction state is detected, then call the on_XXX handler
     * */
    uint64_t start;
    uint64_t end;
    uint64_t wait_ms;
    int servo_inc = 5;
    int loop_wait_time = 200;
    bool something_too_close = false;
    bool front_collision = false;
    bool rear_collision = false;
    int angle_index = 0;
    while (1)
    {
        start = esp_timer_get_time();

        if (sweep_enabled)
        {
            servo_set_angle(&comparator, servo_angle);
            hcsr04_trigger(&ultrasonic);

            int16_t distance = check_sonar(loop_wait_time);
            // distance would come back 0 only if rmt_receive failed, but there would be a log. Or if a timeout occured. In which case this would mean the distance is too far
            if (distance != 0 && distance != distances[angle_index])
            {

                // char str[256];
                // sprintf(&str, "d=%i, index=%i\n", distance, angle_index);
                // broadcast_log(&str);

                distances[angle_index] = distance;
                // ESP_LOGI(TAG, "OBSTRUCTION at angle %i", servo_angle);

                bool too_close = obstruction_is_too_close();
                if (too_close && !something_too_close)
                {
                    on_obstruction_detected();
                }
                else if (!too_close && something_too_close)
                {
                    on_obstruction_cleared();
                }
                something_too_close = too_close;
            }

            servo_angle = angles[angle_index];
            angle_index++;
            if (angle_index >= SONAR_DISTANCE_COUNT)
            {
                angle_index = 0;
            }
        }

        if (gpio_get_level(GPIO_REAR_BUMPER))
        {
            if (!rear_collision)
            {
                on_collision(BUMPER_REAR);
            }
            rear_collision = true;
        }
        else
        {
            rear_collision = false;
        }
        if (gpio_get_level(GPIO_FRONT_BUMPER))
        {
            if (!front_collision)
            {
                on_collision(BUMPER_FRONT);
            }
            front_collision = true;
        }
        else
        {
            front_collision = false;
        }

        // We don't want to loop faster than 200ms
        // But if the function took more than 200ms to run, then dont delay.
        end = esp_timer_get_time();
        wait_ms = (end - start) / 1000;
        if (wait_ms < loop_wait_time)
        {
            vTaskDelay((loop_wait_time - wait_ms) / portTICK_PERIOD_MS);
        }
    }
}

void obstruction_enable_sweep(bool enable)
{
    sweep_enabled = enable;
}

int obstruction_get_colision_status()
{
    int ret = (gpio_get_level(GPIO_FRONT_BUMPER) ? COLLISION_FRONT : 0) | (gpio_get_level(GPIO_REAR_BUMPER) ? COLLISION_REAR : 0);
    return ret;
}

void obstruction_init_detection()
{
    gpio_config_t i_conf = {};
    i_conf.pin_bit_mask = (1ULL << GPIO_REAR_BUMPER) | (1ULL << GPIO_FRONT_BUMPER);
    i_conf.intr_type = GPIO_INTR_DISABLE;
    i_conf.mode = GPIO_MODE_INPUT;
    i_conf.pull_up_en = 1;
    i_conf.pull_down_en = 0;
    gpio_config(&i_conf);

    servo_create(GPIO_DISTANCE_SERVO, &comparator);
    servo_set_angle(&comparator, servo_angle);
    hcsr04_init(GPIO_DISTANCE_TRIGGER, GPIO_DISTANCE_ECHO, &ultrasonic);

    // TODO: Doesn't work well yet.
    xTaskCreate(check_obstructions_task, "check_obstructions", 4096, NULL, 5, &check_obstructions_task_handle);
}
