#include "obstacles.h"
#include <stdbool.h>
#include <esp_log.h>
#include "hw_def.h"
#include "hcsr04.h"
#include <esp_system.h>
#include <esp_event.h>
#include <esp_timer.h>
#include "servo.h"

#define SWITCH1 0x01
#define SWITCH2 0x02
#define SWITCH3 0x04
#define SWITCH4 0x08

static const char *TAG = "obstacles_c";

static bool obstructed = false;
static hcsr04_handle_t ultrasonic;
static TaskHandle_t check_obstacles_task_handle;
static mcpwm_cmpr_handle_t comparator;
static int servo_angle = 0;

static void block_forward()
{
}

static void block_reverse()
{
}

// an obstacle is detected while probing at angle "angle"
static void on_obstacle_detected(int angle)
{
}

static void on_obstacle_cleared(int angle)
{
}

static void on_collision(uint8_t field)
{
}

static int8_t check_sonar(int wait_time)
{
    static uint16_t old_val = 0;

    uint16_t val = hcsr04_read(&ultrasonic, wait_time);
    if (old_val == val)
    {
        return -1;
    }
    old_val = val;
    if (val != 0xFFFF && val != 0)
    {
        // ESP_LOGI(TAG, "Distance: %i", val);
        return (val < MIN_OBSTACLE_DISTANCE) ? 1 : 0;
    }
    return 0;
}

static uint8_t check_switches()
{
    return 0;
}

static void check_obstacles_task()
{
    /* probe obstacles at angle 0.
     * update the "obstructed" flag.
     * if a change of obstruction state is detected, then call the on_XXX handler
     * */
    uint64_t start;
    uint64_t end;
    uint64_t wait_ms;
    int servo_inc = 5;
    int loop_wait_time = 200;

    while (1)
    {
        start = esp_timer_get_time();
        servo_set_angle(&comparator, servo_angle);
        hcsr04_trigger(&ultrasonic);

        // TODO: Check switches meanwhile
        if (check_sonar(loop_wait_time) == 1)
        {
            ESP_LOGI(TAG, "OBSTACLE at angle %i", servo_angle);
            // TODO: check the position of the servo to know at which angle the obstacle is
        }

        servo_angle += servo_inc;
        if (servo_angle >= 40)
        {
            servo_inc = -10;
        }
        else if (servo_angle <= -40)
        {
            servo_inc = 10;
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

void scan_obstacles()
{
    // Do a sweep of the whole 180 degrees.
    // return the angles that not obstructed
    // we shouldn't be moving during this sweep. but this is a responsibility of the calling function.
}

void obstacle_init_detection()
{
    servo_create(GPIO_DISTANCE_SERVO, &comparator);
    servo_set_angle(&comparator, servo_angle);
    hcsr04_init(GPIO_DISTANCE_TRIGGER, GPIO_DISTANCE_ECHO, &ultrasonic);
    xTaskCreate(check_obstacles_task, "check_obstacles", 4096, NULL, 5, &check_obstacles_task_handle);
}
