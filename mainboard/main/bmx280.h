/**
 * BMX280 - BME280 & BMP280 Driver for Esspressif ESP-32.
 *
 * MIT License
 *
 * Copyright (C) 2020 Halit Utku Maden
 * Please contact at <utkumaden@hotmail.com>
 */

#ifndef _BMX280_H_
#define _BMX280_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include <limits.h>
#include "driver/i2c.h"
#include "sdkconfig.h"

// Register address for chip identification number.
#define BMX280_REG_CHPID 0xD0
// Value of REG_CHPID for BME280
#define BME280_ID 0x60
// Value of REG_CHPID for BMP280 (Engineering Sample 1)
#define BMP280_ID0 0x56
// Value of REG_CHPID for BMP280 (Engineering Sample 2)
#define BMP280_ID1 0x57
// Value of REG_CHPID for BMP280 (Production)
#define BMP280_ID2 0x58

#define BMXAPI extern

    struct bmx280_t
    {
        // I2C port.
        i2c_port_t i2c_port;
        // Slave Address of sensor.
        uint8_t slave;
        // Chip ID of sensor
        uint8_t chip_id;
        // Compensation data
        struct
        {
            uint16_t T1;
            int16_t T2;
            int16_t T3;
            uint16_t P1;
            int16_t P2;
            int16_t P3;
            int16_t P4;
            int16_t P5;
            int16_t P6;
            int16_t P7;
            int16_t P8;
            int16_t P9;
#if !(CONFIG_BMX280_EXPECT_BMP280)
            uint8_t H1;
            int16_t H2;
            uint8_t H3;
            int16_t H4;
            int16_t H5;
            int8_t H6;
#endif
        } cmps;
        // Storage for a variable proportional to temperature.
        int32_t t_fine;
    };

/**
 * Macro that identifies a chip id as BME280 or BMP280
 * @note Only use when the chip is verified to be either a BME280 or BMP280.
 * @see bmx280_verify
 * @param chip_id The chip id.
 */
#define bmx280_isBME(chip_id) ((chip_id) == BME280_ID)

    /**
     * Anonymous structure to driver settings.
     */
    typedef struct bmx280_t bmx280_t;

    typedef enum bmx280_tsmpl_t
    {
        BMX280_TEMPERATURE_OVERSAMPLING_NONE = 0x0,
        BMX280_TEMPERATURE_OVERSAMPLING_X1,
        BMX280_TEMPERATURE_OVERSAMPLING_X2,
        BMX280_TEMPERATURE_OVERSAMPLING_X4,
        BMX280_TEMPERATURE_OVERSAMPLING_X8,
        BMX280_TEMPERATURE_OVERSAMPLING_X16,
    } bmx280_tsmpl_t;

    typedef enum bmx280_psmpl_t
    {
        BMX280_PRESSURE_OVERSAMPLING_NONE = 0x0,
        BMX280_PRESSURE_OVERSAMPLING_X1,
        BMX280_PRESSURE_OVERSAMPLING_X2,
        BMX280_PRESSURE_OVERSAMPLING_X4,
        BMX280_PRESSURE_OVERSAMPLING_X8,
        BMX280_PRESSURE_OVERSAMPLING_X16,
    } bmx280_psmpl_t;

#if !(CONFIG_BMX280_EXPECT_BMP280)
    typedef enum bme280_hsmpl_t
    {
        BMX280_HUMIDITY_OVERSAMPLING_NONE = 0x0,
        BMX280_HUMIDITY_OVERSAMPLING_X1,
        BMX280_HUMIDITY_OVERSAMPLING_X2,
        BMX280_HUMIDITY_OVERSAMPLING_X4,
        BMX280_HUMIDITY_OVERSAMPLING_X8,
        BMX280_HUMIDITY_OVERSAMPLING_X16,
    } bme280_hsmpl_t;
#endif

    typedef enum bmx280_tstby_t
    {
        BMX280_STANDBY_0M5 = 0x0,
        BMX280_STANDBY_62M5,
        BMX280_STANDBY_125M,
        BMX280_STANDBY_250M,
        BMX280_STANDBY_500M,
        BMX280_STANDBY_1000M,
        BME280_STANDBY_10M,
        BME280_STANDBY_20M,
        BMP280_STANDBY_2000M = BME280_STANDBY_10M,
        BMP280_STANDBY_4000M = BME280_STANDBY_20M,
    } bmx280_tstby_t;

    typedef enum bmx280_iirf_t
    {
        BMX280_IIR_NONE = 0x0,
        BMX280_IIR_X1,
        BMX280_IIR_X2,
        BMX280_IIR_X4,
        BMX280_IIR_X8,
        BMX280_IIR_X16,
    } bmx280_iirf_t;

    typedef enum bmx280_mode_t
    {
        /** Sensor does no measurements. */
        BMX280_MODE_SLEEP = 0,
        /** Sensor is in a forced measurement cycle. Sleeps after finishing. */
        BMX280_MODE_FORCE = 1,
        /** Sensor does measurements. Never sleeps. */
        BMX280_MODE_CYCLE = 3,
    } bmx280_mode_t;

    typedef struct bmx280_config_t
    {
        bmx280_tsmpl_t t_sampling;
        bmx280_psmpl_t p_sampling;
        bmx280_tstby_t t_standby;
        bmx280_iirf_t iir_filter;
#if !(CONFIG_BMX280_EXPECT_BMP280)
        bme280_hsmpl_t h_sampling;
#endif
    } bmx280_config_t;

#define BMX280_DEFAULT_TEMPERATURE_OVERSAMPLING BMX280_TEMPERATURE_OVERSAMPLING_X2
#define BMX280_DEFAULT_PRESSURE_OVERSAMPLING BMX280_PRESSURE_OVERSAMPLING_X16
#define BMX280_DEFAULT_STANDBY BMX280_STANDBY_0M5
#define BMX280_DEFAULT_IIR BMX280_IIR_X16
#define BMX280_DEFAULT_HUMIDITY_OVERSAMPLING BMX280_HUMIDITY_OVERSAMPLING_X1

#if !(CONFIG_BMX280_EXPECT_BMP280)
#define BMX280_DEFAULT_CONFIG ((bmx280_config_t){BMX280_DEFAULT_TEMPERATURE_OVERSAMPLING, BMX280_DEFAULT_PRESSURE_OVERSAMPLING, BMX280_DEFAULT_STANDBY, BMX280_DEFAULT_IIR, BMX280_DEFAULT_HUMIDITY_OVERSAMPLING})
#else
#define BMX280_DEFAULT_CONFIG ((bmx280_config_t){BMX280_DEFAULT_TEMPERATURE_OVERSAMPLING, BMX280_DEFAULT_PRESSURE_OVERSAMPLING, BMX280_DEFAULT_STANDBY, BMX280_DEFAULT_IIR})
#endif

    /**
     * Create an instance of the BMX280 driver.
     * @param port The I2C port to use.
     * @return A non-null pointer to the driver structure on success.
     */
    BMXAPI bmx280_t *bmx280_create(i2c_port_t port);
    /**
     * Destroy your the instance.
     * @param bmx280 The instance to destroy.
     */
    BMXAPI void bmx280_close(bmx280_t *bmx280);

    /**
     * Probe for the sensor and read calibration data.
     * @param bmx280 Driver structure.
     */
    BMXAPI esp_err_t bmx280_init(bmx280_t *bmx280);
    /**
     * Configure the sensor with the given parameters.
     * @param bmx280 Driver structure.
     * @param configuration The configuration to use.
     */
    BMXAPI esp_err_t bmx280_configure(bmx280_t *bmx280, bmx280_config_t *cfg);

    /**
     * Set the sensor mode of operation.
     * @param bmx280 Driver structure.
     * @param mode The mode to set the sensor to.
     */
    BMXAPI esp_err_t bmx280_setMode(bmx280_t *bmx280, bmx280_mode_t mode);
    /**
     * Get the sensor current mode of operation.
     * @param bmx280 Driver structure.
     * @param mode Pointer to write current mode to.
     */
    BMXAPI esp_err_t bmx280_getMode(bmx280_t *bmx280, bmx280_mode_t *mode);

    /**
     * Returns true if sensor is currently sampling environment conditions.
     * @param bmx280 Driver structure.
     */
    BMXAPI bool bmx280_isSampling(bmx280_t *bmx280);

    /**
     * Read sensor values as fixed point numbers.
     * @param bmx280 Driver structure.
     * @param temperature The temperature in C (0.01 degree C increments)
     * @param pressure The pressure in Pa (1/256 Pa increments)
     * @param humidity The humidity in %RH (1/1024 %RH increments) (UINT32_MAX when invlaid.)
     */
    BMXAPI esp_err_t bmx280_readout(bmx280_t *bmx280, int32_t *temperature, uint32_t *pressure, uint32_t *humidity);

    /**
     * Convert sensor readout to floating point values.
     * @param tin Input temperature.
     * @param pin Input pressure.
     * @param hin Input humidity.
     * @param tout Output temperature. (C)
     * @param pout Output pressure. (Pa)
     * @param hout Output humidity. (%Rh)
     */
    static inline void bmx280_readout2float(int32_t *tin, uint32_t *pin, uint32_t *hin, float *tout, float *pout, float *hout)
    {
        if (tin && tout)
            *tout = (float)*tin * 0.01f;
        if (pin && pout)
            *pout = (float)*pin * (1.0f / 256.0f);
        if (hin && hout)
            *hout = (*hin == UINT32_MAX) ? -1.0f : (float)*hin * (1.0f / 1024.0f);
    }

    /**
     * Read sensor values as floating point numbers.
     * @param bmx280 Driver structure.
     * @param temperature The temperature in C.
     * @param pressure The pressure in Pa.
     * @param humidity The humidity in %RH.
     */
    static inline esp_err_t bmx280_readoutFloat(bmx280_t *bmx280, float *temperature, float *pressure, float *humidity)
    {
        int32_t t;
        uint32_t p, h;
        esp_err_t err = bmx280_readout(bmx280, &t, &p, &h);

        if (err == ESP_OK)
        {
            bmx280_readout2float(&t, &p, &h, temperature, pressure, humidity);
        }

        return err;
    }

#ifdef __cplusplus
};
#endif

#endif
