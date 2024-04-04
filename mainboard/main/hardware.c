#include "driver/gpio.h"
#include <esp_system.h>
#include <esp_event.h>
#include <nvs_flash.h>
#include <esp_log.h>
#include <esp_event_base.h>
#include "hardware.h"
#include "common.h"
#include "driver/pcnt.h"
#include "led_sequence.h"
#include "common/ps5_data.h"
#include "common/i2c.h"
#include "common/deepsleep.h"
#include "hw_def.h"
#include "wifi.h"
#include "led.h"
#include "buzzer.h"
#include <memory.h>
#include "servo.h"
#include "arm.h"
#include "routerecorder.h"
#include "obstructions.h"
#include "fsm.h"
#include "uart.h"
#include "laser.h"
#include "sensors.h"
#include "pwm.h"

#define PCNT_UNIT_MOTOR1 PCNT_UNIT_0
#define PCNT_UNIT_MOTOR2 PCNT_UNIT_1
#define PCNT_UNIT_MOTOR3 PCNT_UNIT_2
#define PCNT_UNIT_MOTOR4 PCNT_UNIT_3
#define STATE_BIT_REVERSE 0
#define STATE_BIT_FWD 1
#define STATE_BIT_HORN 2
#define STATE_BIT_LED 3
#define STATE_CONTROLLER_CONNECTED 4
#define STATE_BIT_R3 5
#define STATE_BIT_CROSS 6
#define STATE_BIT_RECORD 7
#define STATE_BIT_REPLAY 8
#define STATE_BIT_AUTO 9

typedef struct
{
    int unit;
    int motor_revolutions;
    int motor_direction;
} motor_pcnt;

static motor_pcnt motor1_pcnt = {PCNT_UNIT_MOTOR1, 0, 0};
static motor_pcnt motor2_pcnt = {PCNT_UNIT_MOTOR2, 0, 0};
static motor_pcnt motor3_pcnt = {PCNT_UNIT_MOTOR3, 0, 0};
static motor_pcnt motor4_pcnt = {PCNT_UNIT_MOTOR4, 0, 0};

// This is a logarithmic curve generated by motorlogcurve.py
static uint16_t throttle_map_high[] = {0, 2457, 2936, 3276, 3540, 3755, 3938, 4095, 4235, 4359, 4472, 4574, 4669, 4757, 4838, 4914, 4986, 5054, 5117, 5178, 5236, 5291, 5343, 5394, 5442, 5488, 5533, 5576, 5617, 5657, 5696, 5733, 5770, 5805, 5839, 5873, 5905, 5937, 5967, 5997, 6026, 6055, 6083, 6110, 6136, 6162, 6188, 6213, 6237, 6261, 6284, 6307, 6330, 6352, 6373, 6395, 6416, 6436, 6456, 6476, 6496, 6515, 6534, 6553, 6571, 6589, 6607, 6624, 6641, 6658, 6675, 6692, 6708, 6724, 6740, 6756, 6771, 6786, 6801, 6816, 6831, 6845, 6860, 6874, 6888, 6902, 6915, 6929, 6942, 6955, 6968, 6981, 6994, 7007, 7019, 7032, 7044, 7056, 7068, 7080, 7092, 7103, 7115, 7126, 7138, 7149, 7160, 7171, 7182, 7193, 7203, 7214, 7224, 7235, 7245, 7255, 7265, 7275, 7285, 7295, 7305, 7315, 7325, 7334, 7344, 7353, 7362, 7372, 7381, 7390, 7399, 7408, 7417, 7426, 7435, 7443, 7452, 7460, 7469, 7477, 7486, 7494, 7503, 7511, 7519, 7527, 7535, 7543, 7551, 7559, 7567, 7575, 7582, 7590, 7598, 7605, 7613, 7620, 7628, 7635, 7643, 7650, 7657, 7664, 7672, 7679, 7686, 7693, 7700, 7707, 7714, 7721, 7728, 7734, 7741, 7748, 7755, 7761, 7768, 7774, 7781, 7788, 7794, 7800, 7807, 7813, 7820, 7826, 7832, 7838, 7845, 7851, 7857, 7863, 7869, 7875, 7881, 7887, 7893, 7899, 7905, 7911, 7917, 7922, 7928, 7934, 7940, 7945, 7951, 7957, 7962, 7968, 7973, 7979, 7984, 7990, 7995, 8001, 8006, 8012, 8017, 8022, 8028, 8033, 8038, 8043, 8049, 8054, 8059, 8064, 8069, 8074, 8079, 8084, 8090, 8095, 8100, 8105, 8109, 8114, 8119, 8124, 8129, 8134, 8139, 8144, 8148, 8153, 8158, 8163, 8167, 8172, 8177, 8181, 8186, 8191};
// static uint16_t throttle_map_high[] = {0, 1019, 1614, 2037, 2365, 2633, 2860, 3056, 3229, 3384, 3524, 3652, 3769, 3878, 3979, 4074, 4163, 4247, 4327, 4402, 4474, 4542, 4608, 4670, 4730, 4788, 4843, 4897, 4948, 4998, 5046, 5093, 5138, 5182, 5225, 5266, 5306, 5345, 5384, 5421, 5457, 5493, 5527, 5561, 5594, 5626, 5658, 5689, 5719, 5749, 5778, 5806, 5834, 5862, 5889, 5915, 5941, 5967, 5992, 6017, 6041, 6065, 6088, 6111, 6134, 6157, 6179, 6201, 6222, 6243, 6264, 6285, 6305, 6325, 6345, 6364, 6383, 6402, 6421, 6439, 6458, 6476, 6493, 6511, 6528, 6546, 6563, 6579, 6596, 6612, 6629, 6645, 6661, 6676, 6692, 6707, 6723, 6738, 6753, 6767, 6782, 6796, 6811, 6825, 6839, 6853, 6867, 6880, 6894, 6907, 6921, 6934, 6947, 6960, 6973, 6985, 6998, 7011, 7023, 7035, 7047, 7060, 7072, 7083, 7095, 7107, 7119, 7130, 7142, 7153, 7164, 7175, 7186, 7197, 7208, 7219, 7230, 7241, 7251, 7262, 7272, 7283, 7293, 7303, 7313, 7323, 7333, 7343, 7353, 7363, 7373, 7383, 7392, 7402, 7411, 7421, 7430, 7439, 7449, 7458, 7467, 7476, 7485, 7494, 7503, 7512, 7521, 7530, 7538, 7547, 7556, 7564, 7573, 7581, 7590, 7598, 7606, 7615, 7623, 7631, 7639, 7647, 7655, 7663, 7671, 7679, 7687, 7695, 7703, 7711, 7718, 7726, 7734, 7741, 7749, 7756, 7764, 7771, 7779, 7786, 7793, 7801, 7808, 7815, 7822, 7829, 7836, 7844, 7851, 7858, 7865, 7872, 7878, 7885, 7892, 7899, 7906, 7913, 7919, 7926, 7933, 7939, 7946, 7952, 7959, 7965, 7972, 7978, 7985, 7991, 7998, 8004, 8010, 8017, 8023, 8029, 8035, 8042, 8048, 8054, 8060, 8066, 8072, 8078, 8084, 8090, 8096, 8102, 8108, 8114, 8120, 8126, 8131, 8137, 8143, 8191};
//  static uint16_t throttle_map_low[] = {0, 501, 793, 1001, 1162, 1294, 1405, 1502, 1587, 1663, 1732, 1794, 1852, 1906, 1956, 2002, 2046, 2087, 2126, 2163, 2199, 2232, 2264, 2295, 2324, 2353, 2380, 2406, 2432, 2456, 2480, 2503, 2525, 2546, 2567, 2588, 2608, 2627, 2646, 2664, 2682, 2699, 2716, 2733, 2749, 2765, 2780, 2795, 2810, 2825, 2839, 2853, 2867, 2881, 2894, 2907, 2920, 2932, 2944, 2957, 2969, 2980, 2992, 3003, 3014, 3025, 3036, 3047, 3058, 3068, 3078, 3088, 3098, 3108, 3118, 3127, 3137, 3146, 3155, 3164, 3173, 3182, 3191, 3200, 3208, 3217, 3225, 3233, 3241, 3249, 3257, 3265, 3273, 3281, 3288, 3296, 3304, 3311, 3318, 3326, 3333, 3340, 3347, 3354, 3361, 3368, 3374, 3381, 3388, 3394, 3401, 3407, 3414, 3420, 3426, 3433, 3439, 3445, 3451, 3457, 3463, 3469, 3475, 3481, 3487, 3492, 3498, 3504, 3509, 3515, 3521, 3526, 3531, 3537, 3542, 3548, 3553, 3558, 3563, 3568, 3574, 3579, 3584, 3589, 3594, 3599, 3604, 3609, 3613, 3618, 3623, 3628, 3633, 3637, 3642, 3647, 3651, 3656, 3660, 3665, 3669, 3674, 3678, 3683, 3687, 3692, 3696, 3700, 3704, 3709, 3713, 3717, 3721, 3725, 3730, 3734, 3738, 3742, 3746, 3750, 3754, 3758, 3762, 3766, 3770, 3774, 3778, 3781, 3785, 3789, 3793, 3797, 3800, 3804, 3808, 3811, 3815, 3819, 3822, 3826, 3830, 3833, 3837, 3840, 3844, 3847, 3851, 3854, 3858, 3861, 3865, 3868, 3872, 3875, 3878, 3882, 3885, 3888, 3892, 3895, 3898, 3901, 3905, 3908, 3911, 3914, 3917, 3921, 3924, 3927, 3930, 3933, 3936, 3939, 3943, 3946, 3949, 3952, 3955, 3958, 3961, 3964, 3967, 3970, 3973, 3976, 3978, 3981, 3984, 3987, 3990, 3993, 3996, 3999, 4001, 4095};
static uint16_t *throttle_map;
static TaskHandle_t rpm_task;
static QueueHandle_t main_queue;

static fsm_state_t *fsm_current_state;
void process_state_during_error_mode(uint8_t type, void *data, uint32_t old_state, uint32_t new_state);
void process_state_during_driving_mode(uint8_t type, void *data, uint32_t old_state, uint32_t new_state);
void process_state_during_recording_mode(uint8_t type, void *data, uint32_t old_state, uint32_t new_state);
void process_state_during_replaying_mode(uint8_t type, void *data, uint32_t old_state, uint32_t new_state);
void process_state_during_arm_mode(uint8_t type, void *data, uint32_t old_state, uint32_t new_state);
void process_state_during_disconnected_mode(uint8_t type, void *data, uint32_t old_state, uint32_t new_state);
void process_state_during_autonomous_mode(uint8_t type, void *data, uint32_t old_state, uint32_t new_state);

static fsm_state_t fsm_state_error = {.cb = &process_state_during_error_mode};
static fsm_state_t fsm_state_autonomous = {.cb = &process_state_during_autonomous_mode};
static fsm_state_t fsm_state_driving = {.cb = &process_state_during_driving_mode};
static fsm_state_t fsm_state_recording = {.cb = &process_state_during_recording_mode};
static fsm_state_t fsm_state_replaying = {.cb = &process_state_during_replaying_mode};
static fsm_state_t fsm_state_arm = {.cb = &process_state_during_arm_mode};
static fsm_state_t fsm_state_disconnected = {.cb = &process_state_during_disconnected_mode};

metrics_t metrics = {0};

static const char *TAG = "hardware_c";
static bool arm_activated = false;
static uint8_t current_throttle;
static int8_t current_direction;
static mcpwm_cmpr_handle_t comparator_steering;

bool is_going_reverse(ps5_t *data)
{
    // We don't support braking for now (setting both directions high)
    // We should though. But setting both pins to 1 instead of PWM
    if (data->analog.button.r2)
    {
        return false;
    }
    else if (data->analog.button.l2)
    {
        return true;
    }

    return false;
}

bool is_going_forward(ps5_t *data)
{
    if (data->analog.button.r2)
    {
        return true;
    }
    return false;
}

void process_steering(int angle)
{
    metrics.steering_angle = angle;
    servo_set_angle(&comparator_steering, angle);
}

void set_throttle(uint8_t t, int8_t direction)
{
    current_throttle = t;
    current_direction = direction;
}

void process_throttle()
{
    broadcast_log("process_throttle1\n");

    // PS5 controller returns an 8bit value that needs to be mapped to 13bit for the PWM unit
    // I map this using a logarithmic curve rather than simply a linear map because at low values, the motor doesn't turn at all.
    uint16_t throttle = throttle_map[current_throttle];

    if (current_direction == 1)
    {
        metrics.direction = 1;
        metrics.throttle = current_throttle;
        metrics.pwm = throttle;
    }
    else
    {
        metrics.direction = -1;
        metrics.throttle = current_throttle;
        metrics.pwm = throttle;
    }

    // TODO
    // DC motor will briefly use 2 times the amount of stall current if it goes from one direction to the other.
    // so we need to carefully ramp this down instead. I think LEDC has a transitioning mechanism that could be used for that
    // ledc_set_fade_with_time(), ledc_set_fade_with_step(), ledc_set_fade()

    if (current_direction == 1)
    {
        pwm_set_motor_duty_cycles(throttle, 0);
    }
    else
    {
        pwm_set_motor_duty_cycles(0, throttle);
    }
}

void report_metrics()
{
    broadcast_metric(&metrics);
}

void check_rpm(void *arg)
{
    // unsigned long last_rpm_check = (esp_timer_get_time() / 1000ULL);
    while (1)
    {
        //  Check counter every second
        vTaskDelay(1000 / portTICK_PERIOD_MS);

        uint16_t rpm1 = (motor1_pcnt.motor_revolutions * 60) >> 3; // divide by 8 because the PCNT is set the trigger events 8 times faster
        uint16_t rpm2 = (motor2_pcnt.motor_revolutions * 60) >> 3; // divide by 8 because the PCNT is set the trigger events 8 times faster
        uint16_t rpm3 = (motor3_pcnt.motor_revolutions * 60) >> 3; // divide by 8 because the PCNT is set the trigger events 8 times faster
        uint16_t rpm4 = (motor4_pcnt.motor_revolutions * 60) >> 3; // divide by 8 because the PCNT is set the trigger events 8 times faster

        // TODO: For some reason, the RPM reported is twice as higher than the rated value of my motor. Why? We'll just divide by 2 for the moment
        metrics.rpm1 = rpm1 >> 1;
        metrics.rpm2 = rpm2 >> 1;
        metrics.rpm3 = rpm3 >> 1;
        metrics.rpm4 = rpm4 >> 1;

        portDISABLE_INTERRUPTS();
        motor1_pcnt.motor_revolutions = 0;
        motor2_pcnt.motor_revolutions = 0;
        motor3_pcnt.motor_revolutions = 0;
        motor4_pcnt.motor_revolutions = 0;
        portENABLE_INTERRUPTS();

        uint16_t mm_per_minutes = ((metrics.rpm1 + metrics.rpm2 + metrics.rpm3 + metrics.rpm4) / 4) * WHEEL_SIZE_MM;
        metrics.speed = mm_per_minutes / 1667; // should be 16670 but we will do fixed point maths to keep the decimal without using the FPU
        report_metrics();
    }
}

static bool was(uint32_t old_state, uint32_t new_state, uint32_t bit)
{
    return ((old_state & (1 << bit)) && !(new_state & (1 << bit)));
}

static bool is_now(uint32_t old_state, uint32_t new_state, uint32_t bit)
{
    return (!(old_state & (1 << bit)) && (new_state & (1 << bit)));
}

static bool is(uint32_t new_state, uint32_t bit)
{
    return (new_state & (1 << bit));
}

void on_enter_disconnected_mode()
{
    metrics.horn = 0;
    obstruction_enable_sweep(false);
    buzzer_set_off();
    LED_SET_DISCONNECTED_PATTERN();
    laser_trigger(false);
    laser_set_position(0, 0);

    fsm_current_state = &fsm_state_disconnected;
}

void on_exit_disconnected_mode()
{
}

void on_enter_arm_mode()
{
    obstruction_enable_sweep(false);
    laser_trigger(false);
    laser_set_position(0, 0);

    LED_SET_ARM_PATTERN();
    fsm_current_state = &fsm_state_arm;
    set_throttle(0, 0); // Stop moving
    arm_activated = true;
    arm_enable(arm_activated);

    controller_message msg = {1, 0, 0, 255};
    send_to_uart(&msg);
    controller_message msg2 = {2, 1, 1, 1};
    send_to_uart(&msg2);
}

void on_exit_arm_mode()
{
    arm_activated = false;
    arm_enable(arm_activated);
    controller_message msg = {2, 0, 1, 1};
    send_to_uart(&msg);
}

void on_enter_record_mode()
{
    obstruction_enable_sweep(false);
    laser_trigger(false);
    laser_set_position(0, 0);

    metrics.horn = 0;
    buzzer_set_off();
    LED_SET_RECORDING_PATTERN();
    fsm_current_state = &fsm_state_recording;
    controller_message msg = {1, 255, 0, 0};
    send_to_uart(&msg);
}

void on_exit_record_mode()
{
}

void on_enter_replay_mode()
{
    obstruction_enable_sweep(false);
    laser_trigger(false);
    laser_set_position(0, 0);

    metrics.horn = 0;
    buzzer_set_off();
    LED_SET_REPLAY_PATTERN();
    fsm_current_state = &fsm_state_replaying;
    controller_message msg = {1, 0, 255, 0};
    send_to_uart(&msg);
    controller_message msg2 = {2, 1, 1, 1};
    send_to_uart(&msg2);
}

void on_exit_replay_mode()
{
    controller_message msg = {2, 0, 1, 1};
    send_to_uart(&msg);
    controller_message msg2 = {3, 1, 0, 0};
    send_to_uart(&msg2);
}

void on_enter_driving_mode()
{

    // TODO: re-enable this obstruction_enable_sweep(true);
    laser_trigger(false);
    laser_set_position(0, 0);

    metrics.horn = 0;
    buzzer_set_off();
    LED_SET_IDLE_PATTERN();

    controller_message msg = {1, 255, 255, 255};
    send_to_uart(&msg);

    fsm_current_state = &fsm_state_driving;
}

void on_exit_driving_mode()
{
    laser_trigger(false);
    obstruction_enable_sweep(false);
}

void on_enter_autonomous_mode()
{
    LED_SET_AUTONOMOUS_PATTERN();
    fsm_current_state = &fsm_state_autonomous;

    laser_trigger(false);
    laser_set_position(0, 0);

    set_throttle(30, 0); // Start moving

    controller_message msg = {1, 0, 255, 255};
    send_to_uart(&msg);
    controller_message msg2 = {2, 1, 1, 1};
    send_to_uart(&msg2);
}

void on_exit_autonomous_mode()
{
    set_throttle(0, 0); // Stop moving
                        // TODO: abort scan
}

void process_state_during_disconnected_mode(uint8_t type, void *d, uint32_t old_state, uint32_t new_state)
{
    if (type != MESSAGE_PS5)
    {
        // This state does not need to do periodic checks
        return;
    }

    // Any incoming packet would trigger the connected state
    // if (is(new_state, STATE_CONTROLLER_CONNECTED))
    {
        on_exit_disconnected_mode();
        on_enter_driving_mode();
        return;
    }
}

void process_state_during_autonomous_mode(uint8_t type, void *d, uint32_t current_state, uint32_t new_state)
{
    if (type == MESSAGE_PS5)
    {
        if (is_now(current_state, new_state, STATE_BIT_AUTO))
        {
            on_exit_autonomous_mode();
            on_enter_driving_mode();
            return;
        }

        return;
    }

    // TODO: on obstruction event, start a sweep and take a decision
}

void on_enter_error_mode()
{
    metrics.horn = 0;
    obstruction_enable_sweep(false);
    buzzer_set_off();
    LED_SET_ERROR_PATTERN();
    laser_trigger(false);

    broadcast_log("error state\n");
    controller_message msg = {2, 1, 1, 1}; // lock forward buttons
    send_to_uart(&msg);
    controller_message msg2 = {3, 1, 0, 0}; // rumble
    send_to_uart(&msg2);
    set_throttle(0, 0);

    fsm_current_state = &fsm_state_error;
}

void on_exit_error_mode()
{
    controller_message msg = {2, 0, 1, 1};
    send_to_uart(&msg);
}

void process_state_during_error_mode(uint8_t type, void *d, uint32_t current_state, uint32_t new_state)
{
    if (type == MESSAGE_PS5)
    {
        if (is_now(current_state, new_state, STATE_BIT_CROSS))
        {
            on_exit_error_mode();
            on_enter_driving_mode();
            return;
        }

        return;
    }
}

void process_state_during_driving_mode(uint8_t type, void *d, uint32_t current_state, uint32_t new_state)
{
    if (type == MESSAGE_COLLISION)
    {
        on_exit_driving_mode();
        on_enter_error_mode();
        return;
    }

    if (type == MESSAGE_OBSTRUCTION_DETECTED)
    {
        on_exit_driving_mode();
        on_enter_error_mode();
        return;
    }

    if (type != MESSAGE_PS5)
    {
        return;
    }

    ps5_t *data = d;

    if (was(current_state, new_state, STATE_CONTROLLER_CONNECTED))
    {
        on_exit_driving_mode();
        on_enter_disconnected_mode();
        return;
    }

    // Entering "record" mode
    if (is_now(current_state, new_state, STATE_BIT_RECORD))
    {
        record_toggle_record();
        if (record_is_recording())
        {
            on_exit_driving_mode();
            on_enter_record_mode();
            return;
        }
    }

    // Entering "replay" mode
    if (is_now(current_state, new_state, STATE_BIT_REPLAY))
    {
        record_toggle_replay();
        if (record_is_replaying())
        {
            on_exit_driving_mode();
            on_enter_replay_mode();
            return;
        }
    }

    // Entering "arm" mode
    if (is_now(current_state, new_state, STATE_BIT_CROSS))
    {
        on_exit_driving_mode();
        on_enter_arm_mode();
        return;
    }

    if (is_now(current_state, new_state, STATE_BIT_AUTO))
    {
        on_exit_driving_mode();
        on_enter_autonomous_mode();
        return;
    }

    if (is_now(current_state, new_state, STATE_BIT_R3)) // Toggle between fast/slow throttle
    {
        laser_trigger(!laser_is_triggered());
    }

    if (is_now(current_state, new_state, STATE_BIT_HORN))
    {
        // only do this after the transition
        metrics.horn = 1;
        uint16_t f[] = BUZZER_CHIME_CUCARACHA();
        buzzer_set_freq(f, sizeof(f));
    }
    else if (was(current_state, new_state, STATE_BIT_HORN))
    {
        metrics.horn = 0;
        buzzer_set_off();
    }

    if (is_now(current_state, new_state, STATE_BIT_LED))
    {
        LED_SET_ATTENTION_PATTERN();
    }
    else if (was(current_state, new_state, STATE_BIT_LED))
    {
        LED_SET_IDLE_PATTERN();
    }

    // we want an angle between -31 to 32 and we have a number from -127 to 128
    int angle = data->analog.stick.lx >> 2;
    bool reverse = is_going_reverse(data);

    laser_set_position(data->analog.stick.rx, data->analog.stick.ry);

    /*
        int col = obstruction_get_colision_status();
        if (col & COLLISION_FRONT || col & COLLISION_REAR || obstruction_is_too_close())
        {
            on_exit_driving_mode();
            on_enter_error_mode();
            return;
        }
    */

    set_throttle(reverse ? data->analog.button.l2 : data->analog.button.r2, !reverse);
    process_steering(angle);
}

void process_state_during_recording_mode(uint8_t type, void *d, uint32_t current_state, uint32_t new_state)
{
    if (type != MESSAGE_PS5)
    {
        return;
    }

    if (was(current_state, new_state, STATE_CONTROLLER_CONNECTED))
    {
        on_exit_record_mode();
        on_enter_disconnected_mode();
        return;
    }

    if (is_now(current_state, new_state, STATE_BIT_RECORD))
    {
        record_toggle_record();
        if (!record_is_recording())
        {
            on_exit_record_mode();
            on_enter_driving_mode();
            return;
        }
    }

    ps5_t *data = d;
    // we want an angle between -31 to 32 and we have a number from -127 to 128
    int angle = data->analog.stick.lx >> 2;
    bool reverse = is_going_reverse(data);
    set_throttle(reverse ? data->analog.button.l2 : data->analog.button.r2, !reverse);
    process_steering(angle);
    record_process(&metrics);

    // Check if still recording. It's possible it was terminated because the buffer was full
    if (!record_is_recording())
    {
        on_exit_record_mode();
        on_enter_driving_mode();
        return;
    }
}

void process_state_during_replaying_mode(uint8_t type, void *d, uint32_t current_state, uint32_t new_state)
{
    if (type == MESSAGE_REPLAY_END)
    {
        // Periodic check to see if we're still replaying
        if (!record_is_replaying())
        {
            on_exit_replay_mode();
            on_enter_driving_mode();
        }
        return;
    }

    if (type != MESSAGE_PS5)
    {
        return;
    }

    if (was(current_state, new_state, STATE_CONTROLLER_CONNECTED))
    {
        on_exit_replay_mode();
        on_enter_disconnected_mode();
        return;
    }

    // Stop replaying
    if (is_now(current_state, new_state, STATE_BIT_REPLAY))
    {
        record_toggle_replay();
        if (!record_is_replaying())
        {
            on_exit_replay_mode();
            on_enter_driving_mode();
            return;
        }
    }
}

void process_state_during_arm_mode(uint8_t type, void *d, uint32_t current_state, uint32_t new_state)
{
    if (type != MESSAGE_PS5)
    {
        // This state does not need to do periodic checks
        return;
    }

    if (was(current_state, new_state, STATE_CONTROLLER_CONNECTED))
    {
        on_exit_arm_mode();
        on_enter_disconnected_mode();
        return;
    }

    // Exit "arm" mode
    if (is_now(current_state, new_state, STATE_BIT_CROSS))
    {
        on_exit_arm_mode();
        on_enter_driving_mode();
        return;
    }
    ps5_t *data = d;
    arm_move(data->analog.stick.lx, data->analog.stick.ly, data->analog.stick.rx, data->analog.stick.ry);
}

void process_other_message(uint8_t type, void *data)
{
    fsm_current_state->cb(type, 0, 0, 0);
    process_throttle();
}

void process_ps5_message(ps5_t *data)
{
    static uint32_t current_state = 0;

    metrics.main_battery = data->main_battery;
    bool pressing_cross = (data->button.cross != 0);
    bool controller_connected = data->controller_connected;
    bool pressing_led = (data->button.l1 != 0);
    bool pressing_r3 = (data->button.r3 != 0);
    bool pressing_horn = (data->button.r1 != 0);
    bool pressing_record = (data->button.circle != 0);
    bool pressing_replay = (data->button.triangle != 0);
    bool pressing_auto = (data->button.square != 0);
    bool reverse = is_going_reverse(data);
    bool fwd = is_going_forward(data);

    uint32_t new_state = (pressing_horn << STATE_BIT_HORN) |
                         (pressing_r3 << STATE_BIT_R3) |
                         (pressing_record << STATE_BIT_RECORD) |
                         (pressing_replay << STATE_BIT_REPLAY) |
                         (pressing_cross << STATE_BIT_CROSS) |
                         (pressing_auto << STATE_BIT_AUTO) |
                         (reverse << STATE_BIT_REVERSE) |
                         (fwd << STATE_BIT_FWD) |
                         (controller_connected << STATE_CONTROLLER_CONNECTED) |
                         (pressing_led << STATE_BIT_LED);

    metrics.controller_battery = data->status.battery;

    fsm_current_state->cb(MESSAGE_PS5, data, current_state, new_state);
    process_throttle();
    current_state = new_state;
}

static void IRAM_ATTR pcnt_isr(void *arg)
{
    motor_pcnt *pcnt = (motor_pcnt *)arg;
    pcnt->motor_revolutions++;
    pcnt->motor_direction = 1;

    uint32_t status = 0;
    pcnt_get_event_status(pcnt->unit, &status);

    pcnt->motor_revolutions++;
    if (status & PCNT_EVT_H_LIM)
    {
        pcnt->motor_direction = 1;
    }
    else if (status & PCNT_EVT_L_LIM)
    {
        pcnt->motor_direction = -1;
    }
}

esp_err_t pcnt_motor_init(motor_pcnt *pcnt, uint8_t pinA, uint8_t pinB)
{
    esp_err_t ret;
    static bool isr_installed = false;

    const pcnt_config_t encodercfg = {
        .pulse_gpio_num = pinA,
        .ctrl_gpio_num = pinB,
        .lctrl_mode = PCNT_MODE_REVERSE,
        .hctrl_mode = PCNT_MODE_KEEP,
        .channel = PCNT_CHANNEL_0,
        .unit = pcnt->unit,
        .pos_mode = PCNT_COUNT_INC,
        .neg_mode = PCNT_COUNT_DIS,

        // The motor we're using, FIT0186 does 16 pulses per rev before gears. This equals to 700 pulses per gear shaft rev.
        // The number of pulses per rev that it generates is 700. We'll trigger every 88 so that we can have a finer grain of detection
        // Increasing the granularity gives us amore precise reading
        .counter_h_lim = 88,
        .counter_l_lim = -88,
    };

    pcnt_unit_config(&encodercfg);

    pcnt_set_filter_value(pcnt->unit, 100);
    pcnt_filter_enable(pcnt->unit);

    pcnt_event_enable(pcnt->unit, PCNT_EVT_H_LIM);
    pcnt_event_enable(pcnt->unit, PCNT_EVT_L_LIM);
    if (!isr_installed)
    {
        pcnt_isr_service_install(0);
        isr_installed = true;
    }
    pcnt_isr_handler_add(pcnt->unit, pcnt_isr, (void *)pcnt);

    pcnt_counter_pause(pcnt->unit);
    pcnt_counter_clear(pcnt->unit);
    pcnt_counter_resume(pcnt->unit);
    return ESP_OK;
}

void on_start_sleep()
{
    LED_SET_GOING_TO_SLEEP_PATTERN();
    vTaskDelay(2500 / portTICK_PERIOD_MS);
    led_turnoff();
    vTaskDelay(500 / portTICK_PERIOD_MS);
}

void hardware_init()
{
    throttle_map = &throttle_map_high;
    main_queue = xQueueCreate(20, sizeof(queue_msg));

    init_deep_sleep(GPIO_SLEEP_BUTTON, on_start_sleep);

    uart_init();
    buzzer_init();
    led_init();
    servo_init();
    servo_create(GPIO_STEERING, &comparator_steering);
    arm_init();
    laser_init();
    pwm_init();
    pcnt_motor_init(&motor1_pcnt, MOTOR1SENSOR1, MOTOR1SENSOR2);
    pcnt_motor_init(&motor2_pcnt, MOTOR2SENSOR1, MOTOR2SENSOR2);
    pcnt_motor_init(&motor3_pcnt, MOTOR3SENSOR1, MOTOR3SENSOR2);
    pcnt_motor_init(&motor4_pcnt, MOTOR4SENSOR1, MOTOR4SENSOR2);

    sensors_init();
    servo_start();
    obstruction_init_detection();
    record_init();

    on_enter_disconnected_mode();

    // Using a task scheduler would be more appropriate here to be able to set priorities
    xTaskCreate(check_rpm, "check_rpm", 4096, NULL, 5, &rpm_task);
}

void hardware_suspend()
{
    queue_msg msg;
    msg.type = MESSAGE_STOP;
    hardware_send_message(&msg);
    uart_stop();
    vTaskSuspend(rpm_task);
}

void hardware_send_message(queue_msg *msg)
{
    xQueueGenericSend(main_queue, msg, 0, queueSEND_TO_BACK);
}

void hardware_run()
{
    queue_msg msg;

    while (1)
    {
        esp_err_t err;

        ps5_t data;
        size_t s = sizeof(ps5_t);

        if (xQueueReceive(main_queue, (void *)&msg, portMAX_DELAY))
        {
            if (msg.type == MESSAGE_STOP)
            {
                break;
            }
            else if (msg.type == MESSAGE_OBSTRUCTION_DETECTED)
            {
                process_other_message(msg.type, 0);
            }
            else if (msg.type == MESSAGE_OBSTRUCTION_CLEARED)
            {
                process_other_message(msg.type, 0);
            }
            else if (msg.type == MESSAGE_COLLISION)
            {
                process_other_message(msg.type, 0);
            }
            else if (msg.type == MESSAGE_REPLAY_END)
            {
                process_other_message(msg.type, 0);
            }
            else if (msg.type == MESSAGE_PS5)
            {
                ps5_t *data = (ps5_t *)msg.data;

                uint8_t received_checksum = data->checksum;
                data->checksum = 0;
                uint8_t c = 0;
                for (int i = 0; i < sizeof(ps5_t) - 1; i++)
                {
                    c += ((uint8_t *)data)[i];
                }
                if (received_checksum != c)
                {
                    ESP_LOGI(TAG, ">>>>>>>>>>>>>>>>>> Controller BAD CHECKSUM. RESETTING CONTROL %i/%i<<<<<<<<<<<<<<<<<<<<<", received_checksum, c);
                    memset(data, 0, sizeof(ps5_t));
                    data->latestPacket = 1;
                    data->controller_connected = 1;
                }

                if (!data->controller_connected)
                {
                    memset(data, 0, sizeof(ps5_t));
                    data->latestPacket = 1;
                }

                // uint8_t *d = &data;
                //  ESP_LOGI(TAG, "===========%i, %i, %i, %i, %i", (int8_t)d[0], (int8_t)d[1], (int8_t)d[2], (int8_t)d[3], d[sizeof(ps5_t) - 1]);
                if (data->latestPacket)
                {
                    ESP_LOGI(TAG, "New PS5 Packet. Throttle=%i", data->analog.button.r2);
                    process_ps5_message(data);
                }

                free(data);
            }
        }
    }

    broadcast_log("Suspending main task\n");
    vTaskSuspend(0);
}
