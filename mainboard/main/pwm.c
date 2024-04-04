#include "pwm.h"
#include "hw_def.h"
#include "driver/ledc.h"

#define CHANNEL_BUZZER LEDC_CHANNEL_2
#define TIMER_BUZZER LEDC_TIMER_1
#define CHANNEL_MOTOR_FWD LEDC_CHANNEL_0
#define CHANNEL_MOTOR_RV LEDC_CHANNEL_1
#define TIMER_MOTORS LEDC_TIMER_0

void pwm_set_motor_duty_cycles(int d1, int d2)
{

    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, CHANNEL_MOTOR_FWD, d1 >> 1)); // divide by 2 because resolution is now 12 instead of 13. We could just change the lookup table too
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, CHANNEL_MOTOR_FWD));
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, CHANNEL_MOTOR_RV, d2 >> 1)); // divide by 2 because resolution is now 12 instead of 13. We could just change the lookup table too
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, CHANNEL_MOTOR_RV));
}

void pwm_set_buzzer_freq(int f)
{
    static bool paused = false;

    if (f < 20)
    {
        ledc_timer_pause(LEDC_LOW_SPEED_MODE, LEDC_TIMER_1);
        paused = true;
    }
    else
    {

        if (paused)
        {
            ledc_timer_resume(LEDC_LOW_SPEED_MODE, LEDC_TIMER_1);
            paused = false;
        }
        ESP_ERROR_CHECK(ledc_set_freq(LEDC_LOW_SPEED_MODE, TIMER_BUZZER, f));
    }
}

void pwm_init()
{
    // Don't let the naming fool you. LEDC is not for LED control. It is the name of the peripheral
    // in the ESP32 for controlling a LED but it is really just a PWM controller

    // Buzzer PWMs
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_13_BIT,
        .timer_num = TIMER_BUZZER,
        .freq_hz = 10,
        .clk_cfg = LEDC_AUTO_CLK};
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    ledc_channel_config_t ledc_channel = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = CHANNEL_BUZZER,
        .timer_sel = TIMER_BUZZER,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = GPIO_BUZZER1,
        .duty = 75,
        .hpoint = 0};
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

    // Motor PWMs
    ledc_timer_config_t ledc_timer1 = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_12_BIT,
        .timer_num = TIMER_MOTORS,
        .freq_hz = 19500,
        .clk_cfg = LEDC_AUTO_CLK};
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer1));

    ledc_channel_config_t ledc_channel1 = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = CHANNEL_MOTOR_FWD,
        .timer_sel = TIMER_MOTORS,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = GPIO_MOTOR_PWM1,
        .duty = 0,
        .hpoint = 0};
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel1));

    ledc_channel_config_t ledc_channel2 = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = CHANNEL_MOTOR_RV,
        .timer_sel = TIMER_MOTORS,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = GPIO_MOTOR_PWM2,
        .duty = 0,
        .hpoint = 0};
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel2));
}