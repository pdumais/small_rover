#include "arm.h"

#include <esp_log.h>
#include "hw_def.h"
#include "string.h"
#include <esp_event.h>
#include "servo.h"
#include "common.h"

#define COMMAND_STOP 1
#define COMMAND_MOVE_ANGLE 2
#define MAX_ANGLE1 55.0  // Base Rotation
#define MIN_ANGLE1 -30.0 // Base Rotation
#define MAX_ANGLE2 54.0  // Boom
#define MIN_ANGLE2 -10.0 // Boom
#define MAX_ANGLE3 30.0  // Grapple
#define MIN_ANGLE3 -35.0 // Grapple
#define MAX_ANGLE4 25.0  // Arm
#define MIN_ANGLE4 -90.0 // Arm

typedef struct
{
    uint8_t command;
    int8_t angle_velocity_lx;
    int8_t angle_velocity_ly;
    int8_t angle_velocity_rx;
    int8_t angle_velocity_ry;
} q_msg;

static QueueHandle_t xQueue;
static const char *TAG = "arm_c";
static bool status_arm_enabled = false;
static mcpwm_cmpr_handle_t arm_boom_rotator;
static mcpwm_cmpr_handle_t arm_boom;
static mcpwm_cmpr_handle_t arm_arm;
static mcpwm_cmpr_handle_t arm_grapple;

extern metrics_t metrics;

void arm_move(int8_t lx, int8_t ly, int8_t rx, int8_t ry)
{
    if (!status_arm_enabled)
    {
        return;
    }

    lx = lx / 2;
    ly = ly / 2;
    rx = rx / 2;
    ry = ry / 2;

    if ((lx > 4 || lx < -4) || (ly > 4 || ly < -4) || (rx > 4 || rx < -4) || (ry > 4 || ry < -4))
    {
        q_msg msg;
        msg.command = COMMAND_MOVE_ANGLE;
        msg.angle_velocity_lx = lx;
        msg.angle_velocity_ly = ly;
        msg.angle_velocity_rx = rx;
        msg.angle_velocity_ry = ry;
        xQueueSend(xQueue, (void *)&msg, 0);
    }
    else
    {
        // TODO: if already stopped, dont stop again or we will wakeup the task for nothing
        q_msg msg;
        msg.command = COMMAND_STOP;
        xQueueSend(xQueue, (void *)&msg, 0);
    }
}

void arm_enable(bool enabled)
{
    if (status_arm_enabled == enabled)
    {
        return;
    }

    status_arm_enabled = enabled;
    if (!status_arm_enabled)
    {
        q_msg msg;
        msg.command = COMMAND_STOP;
        xQueueSend(xQueue, (void *)&msg, 0);
    }
}

void arm_task(void *arg)
{

    q_msg msg;
    float angle1 = 0;
    float angle2 = MIN_ANGLE2;
    float angle3 = MIN_ANGLE3;
    float angle4 = MIN_ANGLE4;

    while (1)
    {
        ESP_LOGI(TAG, "Checking for ARM queue msg");
        if (xQueueReceive(xQueue, &msg, 60000))
        {
            ESP_LOGI(TAG, "ARM queue msg received");
            if (msg.command == COMMAND_STOP)
            {
                // Stop all. Don't do anything. Just go back to waiting for a message
            }
            else if (msg.command == COMMAND_MOVE_ANGLE)
            {
/*
Axis values range from -63 to 63
angles range from -90 to 90
an axis value 0f 63 will ramp up the angle from 0 to 63 within 1s
    so a speed of 6.3 per 100ms
*/
#define RESOLUTION 20
                float delta1 = (float)(0 - msg.angle_velocity_lx) / RESOLUTION;
                float delta2 = (float)msg.angle_velocity_ly / RESOLUTION;
                float delta3 = (float)msg.angle_velocity_rx / RESOLUTION;
                float delta4 = (float)(msg.angle_velocity_ry) / RESOLUTION; // reverse sign since motor is installed the other way around

                // always just take the highest delta for one stick. This is to avoid controlling 2 servos on the same stick
                if (abs(delta1) > abs(delta2))
                {
                    delta2 = 0;
                }
                else
                {
                    delta1 = 0;
                }
                if (abs(delta3) > abs(delta4))
                {
                    delta4 = 0;
                }
                else
                {
                    delta3 = 0;
                }

                while (!uxQueueMessagesWaiting(xQueue))
                {
                    angle1 += delta1;
                    angle2 += delta2;
                    angle3 += delta3;
                    angle4 += delta4;

                    if (angle1 > MAX_ANGLE1)
                    {
                        angle1 = MAX_ANGLE1;
                    }
                    if (angle2 > MAX_ANGLE2)
                    {
                        angle2 = MAX_ANGLE2;
                    }
                    if (angle3 > MAX_ANGLE3)
                    {
                        angle3 = MAX_ANGLE3;
                    }
                    if (angle4 > MAX_ANGLE4)
                    {
                        angle4 = MAX_ANGLE4;
                    }
                    if (angle1 < MIN_ANGLE1)
                    {
                        angle1 = MIN_ANGLE1;
                    }
                    if (angle2 < MIN_ANGLE2)
                    {
                        angle2 = MIN_ANGLE2;
                    }
                    if (angle3 < MIN_ANGLE3)
                    {
                        angle3 = MIN_ANGLE3;
                    }
                    if (angle4 < MIN_ANGLE4)
                    {
                        angle4 = MIN_ANGLE4;
                    }

                    metrics.angle_rotator = angle1;
                    metrics.angle_boom = angle2;
                    metrics.angle_grapple = angle3;
                    metrics.angle_arm = angle4;
                    servo_set_angle(&arm_boom_rotator, angle1);
                    servo_set_angle(&arm_boom, angle2);
                    servo_set_angle(&arm_grapple, angle3);
                    servo_set_angle(&arm_arm, angle4);

                    vTaskDelay((1000 / RESOLUTION) / portTICK_PERIOD_MS);
                }
            }
        }
    }
}

void arm_init()
{
    xQueue = xQueueCreate(4, sizeof(q_msg));

    servo_create(GPIO_ARM_SERVO1, &arm_boom_rotator);
    servo_create(GPIO_ARM_SERVO2, &arm_boom);
    servo_create(GPIO_ARM_SERVO3, &arm_grapple);
    servo_create(GPIO_ARM_SERVO4, &arm_arm);

    servo_set_angle(&arm_boom_rotator, 0);
    servo_set_angle(&arm_boom, MIN_ANGLE2);
    servo_set_angle(&arm_arm, MIN_ANGLE4);
    servo_set_angle(&arm_grapple, MIN_ANGLE3);

    xTaskCreate(arm_task, "arm_task", 4096, NULL, 5, NULL);
}
