#include "sensors.h"
#include "math.h"
#include <esp_log.h>
#include "hw_def.h"
#include <esp_event.h>

static const char *TAG = "sensors_c";
static TaskHandle_t check_sensors_task_handle;
static volatile double rightleft_tilt;
static volatile double frontrear_tilt;

static void compensate_magneto_with_tilt(int *mx, int *my, int *mz)
{
    *mx = *mx * cos(frontrear_tilt);
    *my = *my * cos(rightleft_tilt);

}

static int get_magneto_angle(double mx, double my, double mz)
{
    return (180.0*atan2(my,mx)/3.1416)-90.0;
}

static void calculate_tilt(double x, double y, double z)
{
    double total_accel = sqrt(x*x + y*y + z*z);
       // ESP_LOGI(TAG, "mag: %f", total_accel);
    // if the robot is experiencing another accelleration from moving
    // then we want to ignore this. We want to calculte the tilt only when
    // it is experiencing gravity
    if (total_accel >=9.2 && total_accel <10.4)
    {
        frontrear_tilt = atan2(x,(z));
        rightleft_tilt = atan2(y,(z));
        //ESP_LOGI(TAG, "f: %f, r: %f", frontrear_tilt, rightleft_tilt);
    }
}

static void sensor_task()
{
    double ax=0,ay=0,az = 0;
    double mx=0,my=0,mz = 0;
    while (1)
    {
        ax=0;
        ay=4.2;
        az=8.8;
        mx=39;
        my=-4;
        mz=-4;
        calculate_tilt(ax,ay,az);
        compensate_magneto_with_tilt(&mx, &my, &mz);
        int direction = get_magneto_angle(mx,my,mz);
        ESP_LOGI(TAG, "dir1=%i",direction);

        ax=0;
        ay=7;
        az=6.7;
        mx=16;
        my=-31;
        mz=16;
        calculate_tilt(ax,ay,az);
        compensate_magneto_with_tilt(&mx, &my, &mz);
        direction = get_magneto_angle(mx,my,mz);
        ESP_LOGI(TAG, "dir2=%i",direction);

/*
az=149
a7.38
a0
a6.46
m-27
m17
m13

az=149

a-8
a0.3
a5.3
m26
m-18
m19
*/


        vTaskDelay(1000/portTICK_PERIOD_MS);
    }
}

void sensors_init()
{
    frontrear_tilt = 0;
    rightleft_tilt = 0;
    xTaskCreate(sensor_task, "check_sensors", 4096, NULL, 5, &check_sensors_task_handle);

}