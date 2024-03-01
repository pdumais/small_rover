#include "sensors.h"
#include "math.h"
#include <esp_log.h>
#include "hw_def.h"
#include <esp_event.h>
#include "driver/i2c.h"
#include "bmx280.h"
#include "common.h"

#define LSM303_ADDR_ACCEL 0x19
#define LSM303_ADDR_MAG 0x1E
#define LSM303_CTRL_REG1_A 0x20
#define LSM303_CTRL_REG4_A 0x23
#define LSM303_OUT_X_L_A 0x28
#define LSM303_OUT_X_H_M 0x03

static const char *TAG = "sensors_c";
static bmx280_t *bmx280;
static TaskHandle_t check_sensors_task_handle;
static volatile double rightleft_tilt;
static volatile double frontrear_tilt;
extern metrics_t metrics;

static esp_err_t lsm303_read(uint8_t addr, uint8_t reg, uint8_t *dout, size_t size)
{
    esp_err_t err;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    if (cmd)
    {
        // Write register address
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
        i2c_master_write_byte(cmd, reg, true);
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_READ, true);
        i2c_master_read(cmd, dout, size, I2C_MASTER_LAST_NACK);
        i2c_master_stop(cmd);

        err = i2c_master_cmd_begin(I2C_NUM_0, cmd, 5);
        i2c_cmd_link_delete(cmd);
        return err;
    }
    else
    {
        return ESP_ERR_NO_MEM;
    }
}

static esp_err_t lsm303_write(uint8_t addr, const uint8_t *din, size_t size)
{
    esp_err_t err;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    if (cmd)
    {
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
        i2c_master_write(cmd, din, size, true);
        i2c_master_stop(cmd);

        err = i2c_master_cmd_begin(I2C_NUM_0, cmd, 5);
        i2c_cmd_link_delete(cmd);
        return err;
    }
    else
    {
        return ESP_ERR_NO_MEM;
    }
}

static int get_magneto_angle(double mx, double my, double mz)
{
    return (180.0 * (atan2(my, mx) - (3.1416 / 2.0)) / 3.1416);
}

static void calculate_tilt(float x, float y, float z)
{
    // TODO: This not not take into account if the Z axis gets flipped (sensor put upside down)
    float total_accel = sqrt((x * x) + (y * y) + (z * z));
    //  if the robot is experiencing another accelleration from moving
    //  then we want to ignore this. We want to calculte the tilt only when
    //  it is experiencing gravity
    if (total_accel > 9.5 && total_accel < 10.2)
    {
        frontrear_tilt = 180.0 * atan2(x, (z)) / 3.1416;
        rightleft_tilt = 180.0 * atan2(y, (z)) / 3.1416;
        metrics.roll = rightleft_tilt;
        metrics.pitch = frontrear_tilt;
        // ESP_LOGI(TAG, "f: %f, r: %f", 180.0 * frontrear_tilt / 3.1416, 180.0 * rightleft_tilt / 3.1416);
    }
}

static void sensor_task()
{
    int16_t min_ax = 32767, min_ay = 32767, min_az = 32767;
    int16_t max_ax = -32767, max_ay = -32767, max_az = -32767;
    int16_t _ax = 0, _ay = 0, _az = 0;
    float ax = 0, ay = 0, az = 0;
    int16_t _mx = 0, _my = 0, _mz = 0;
    float mx = 0, my = 0, mz = 0;
    float temperature, pressure, humidity;

    while (1)
    {

        bmx280_setMode(bmx280, BMX280_MODE_FORCE);
        while (bmx280_isSampling(bmx280))
        {
            vTaskDelay(20 / portTICK_PERIOD_MS);
        }
        bmx280_readoutFloat(bmx280, &temperature, &pressure, &humidity);
        metrics.temperature = temperature * 10;
        metrics.humidity = humidity * 10;
        metrics.pressure = pressure / 100;

        uint8_t data[6] = {0};
        lsm303_read(LSM303_ADDR_MAG, LSM303_OUT_X_H_M, data, 6);
        _mx = (int16_t)(data[0] << 8 | data[1]);
        _mz = (int16_t)(data[2] << 8 | data[3]);
        _my = (int16_t)(data[4] << 8 | data[5]);
        mx = ((float)_mx / 1100.0) * 100.0;
        my = ((float)_my / 1100.0) * 100.0;
        mz = ((float)_mz / 980.0) * 100.0;
        metrics.heading = get_magneto_angle(mx, my, mz);

        lsm303_read(LSM303_ADDR_ACCEL, LSM303_OUT_X_L_A | 0x80, data, 6);

        // The sensor is set to "normal mode" which has a 10bit resolution
        _ax = ((int16_t)(data[1] << 8 | data[0])) >> 6;
        _ay = ((int16_t)(data[3] << 8 | data[2])) >> 6;
        _az = ((int16_t)(data[5] << 8 | data[4])) >> 6;
        ax = ((float)_ax * 0.00782 * 9.80665F) + 0.537; // Offset observed when laying on a flat surface
        ay = ((float)_ay * 0.00782 * 9.80665F) + 0.0;   // Offset observed when laying on a flat surface
        az = ((float)_az * 0.00782 * 9.80665F) + 2.0F;  // Offset observed when laying on a flat surface
        calculate_tilt(ax, ay, az);
        // ESP_LOGI(TAG, "ax=%f, ay=%f, az=%f", ax, ay, az);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void sensors_init()
{
    frontrear_tilt = 0;
    rightleft_tilt = 0;

    i2c_config_t i2c_cfg = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = GPIO_SDA,
        .scl_io_num = GPIO_SCL,
        .sda_pullup_en = false,
        .scl_pullup_en = false,
        .master = {
            .clk_speed = 100000}};

    ESP_ERROR_CHECK(i2c_param_config(I2C_NUM_0, &i2c_cfg));
    ESP_ERROR_CHECK(i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0));

    bmx280 = bmx280_create(I2C_NUM_0);

    esp_err_t err = bmx280_init(bmx280);
    if (err != ESP_OK)
    {
        bmx280 = 0;
        ESP_LOGI(TAG, "BME280 not found");
        return;
    }
    else
    {

        bmx280_config_t bmx_cfg = BMX280_DEFAULT_CONFIG;
        if (bmx280_configure(bmx280, &bmx_cfg) != ESP_OK)
        {
            return;
        }
    }

    uint8_t accel_config[2] = {LSM303_CTRL_REG1_A, 0x67};
    lsm303_write(LSM303_ADDR_ACCEL, accel_config, 2);
    accel_config[0] = LSM303_CTRL_REG4_A;
    accel_config[1] = 0b10000; // Set scale to 4g
    lsm303_write(LSM303_ADDR_ACCEL, accel_config, 2);

    uint8_t mag_config[2] = {0x00, 0b11100};
    lsm303_write(LSM303_ADDR_MAG, mag_config, 2);
    mag_config[0] = 0x01;
    mag_config[1] = 0b00100000;
    lsm303_write(LSM303_ADDR_MAG, mag_config, 2);
    mag_config[0] = 0x02;
    mag_config[1] = 0x0;
    lsm303_write(LSM303_ADDR_MAG, mag_config, 2);

    xTaskCreate(sensor_task, "check_sensors", 4096, NULL, 5, &check_sensors_task_handle);
}