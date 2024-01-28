#include "servo.h"
#include <esp_log.h>

// TODO: Set the right values for each servo, we are now assuming they are all the same
#define SERVO_MIN_PULSEWIDTH_US 500          // Minimum pulse width in microsecond
#define SERVO_MAX_PULSEWIDTH_US 2500         // Maximum pulse width in microsecond
#define SERVO_MIN_DEGREE -90                 // Minimum angle
#define SERVO_MAX_DEGREE 90                  // Maximum angle
#define SERVO_TIMEBASE_RESOLUTION_HZ 1000000 // 1MHz, 1us per tick
#define SERVO_TIMEBASE_PERIOD 20000          // 20000 ticks, 20ms

static const char *TAG = "servo_c";
static mcpwm_timer_handle_t timer = NULL;

static inline uint32_t angle_to_compare(int angle)
{
    return (angle - SERVO_MIN_DEGREE) * (SERVO_MAX_PULSEWIDTH_US - SERVO_MIN_PULSEWIDTH_US) / (SERVO_MAX_DEGREE - SERVO_MIN_DEGREE) + SERVO_MIN_PULSEWIDTH_US;
}

static mcpwm_oper_handle_t oper1 = NULL;
static mcpwm_oper_handle_t oper2 = NULL;
static mcpwm_oper_handle_t oper3 = NULL;

static int pwm_count = 0;

void servo_init()
{
    mcpwm_timer_config_t timer_config = {
        .group_id = 0,
        .clk_src = MCPWM_TIMER_CLK_SRC_DEFAULT,
        .resolution_hz = SERVO_TIMEBASE_RESOLUTION_HZ,
        .period_ticks = SERVO_TIMEBASE_PERIOD,
        .count_mode = MCPWM_TIMER_COUNT_MODE_UP,
    };
    ESP_ERROR_CHECK(mcpwm_new_timer(&timer_config, &timer));

    mcpwm_operator_config_t operator_config = {
        .group_id = 0, // operator must be in the same group to the timer
    };
    ESP_ERROR_CHECK(mcpwm_new_operator(&operator_config, &oper1));
    ESP_ERROR_CHECK(mcpwm_new_operator(&operator_config, &oper2));
    ESP_ERROR_CHECK(mcpwm_new_operator(&operator_config, &oper3));

    ESP_LOGI(TAG, "Connect timer and operator");
    ESP_ERROR_CHECK(mcpwm_operator_connect_timer(oper1, timer));
    ESP_ERROR_CHECK(mcpwm_operator_connect_timer(oper2, timer));
    ESP_ERROR_CHECK(mcpwm_operator_connect_timer(oper3, timer));
}
void servo_start()
{
    ESP_LOGI(TAG, "Enable and start timer");
    ESP_ERROR_CHECK(mcpwm_timer_enable(timer));
    ESP_ERROR_CHECK(mcpwm_timer_start_stop(timer, MCPWM_TIMER_START_NO_STOP));
}

void servo_create(uint8_t pin, mcpwm_cmpr_handle_t *comparator)
{
    // We can put 1 PWM pin in 1 comparator. 2 comparator in 1 operator. 3 operator per timers. And we have 2 timers
    static mcpwm_oper_handle_t *oper = NULL;
    if (pwm_count >> 1 == 0)
    {
        oper = &oper1;
    }
    if (pwm_count >> 1 == 1)
    {
        oper = &oper2;
    }
    if (pwm_count >> 1 == 2)
    {
        oper = &oper3;
    }

    mcpwm_gen_handle_t generator = NULL;
    mcpwm_generator_config_t generator_config = {
        .gen_gpio_num = pin,
    };
    ESP_ERROR_CHECK(mcpwm_new_generator(*oper, &generator_config, &generator));

    ESP_LOGI(TAG, "Create comparator and generator from the operator");
    mcpwm_comparator_config_t comparator_config = {
        .flags.update_cmp_on_tez = true,
    };

    ESP_ERROR_CHECK(mcpwm_new_comparator(*oper, &comparator_config, comparator));

    // set the initial compare value, so that the servo will spin to the center position
    ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(*comparator, angle_to_compare(0)));

    ESP_LOGI(TAG, "Set generator action on timer and compare event");
    // go high on counter empty
    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_timer_event(generator,
                                                              MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_EMPTY, MCPWM_GEN_ACTION_HIGH)));
    // go low on compare threshold
    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_compare_event(generator,
                                                                MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, *comparator, MCPWM_GEN_ACTION_LOW)));
    pwm_count++;

    // int angle = 0;
    // ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(comparator, angle_to_compare(angle)));
}

void servo_set_angle(mcpwm_cmpr_handle_t *comparator, int angle)
{
    ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(*comparator, angle_to_compare(angle)));
}
