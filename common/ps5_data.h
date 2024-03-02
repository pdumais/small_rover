#pragma once
#include <esp_system.h>

typedef struct
{
	int8_t lx;
	int8_t ly;
	int8_t rx;
	int8_t ry;
} __attribute__((packed)) ps5_analog_stick_t;

typedef struct
{
	uint8_t l2;
	uint8_t r2;
} __attribute__((packed)) ps5_analog_button_t;

typedef struct
{
	ps5_analog_stick_t stick;
	ps5_analog_button_t button;
} __attribute__((packed)) ps5_analog_t;

/*********************/
/*   B U T T O N S   */
/*********************/

typedef struct
{
	uint8_t right : 1;
	uint8_t down : 1;
	uint8_t up : 1;
	uint8_t left : 1;

	uint8_t square : 1;
	uint8_t cross : 1;
	uint8_t circle : 1;
	uint8_t triangle : 1;

	uint8_t upright : 1;
	uint8_t downright : 1;
	uint8_t upleft : 1;
	uint8_t downleft : 1;

	uint8_t l1 : 1;
	uint8_t r1 : 1;
	uint8_t l2 : 1;
	uint8_t r2 : 1;

	uint8_t share : 1;
	uint8_t options : 1;
	uint8_t l3 : 1;
	uint8_t r3 : 1;

	uint8_t ps : 1;
	uint8_t touchpad : 1;
} __attribute__((packed)) ps5_button_t;

/*******************************/
/*   S T A T U S   F L A G S   */
/*******************************/

typedef struct
{
	uint8_t battery;
	uint8_t charging : 1;
	uint8_t audio : 1;
	uint8_t mic : 1;
} __attribute__((packed)) ps5_status_t;

/********************/
/*   S E N S O R S  */
/********************/

typedef struct
{
	int16_t z;
} __attribute__((packed)) ps5_sensor_gyroscope_t;

typedef struct
{
	int16_t x;
	int16_t y;
	int16_t z;
} __attribute__((packed)) ps5_sensor_accelerometer_t;

typedef struct
{
	ps5_sensor_accelerometer_t accelerometer;
	ps5_sensor_gyroscope_t gyroscope;
} __attribute__((packed)) ps5_sensor_t;

/*******************/
/*    O T H E R    */
/*******************/

typedef struct
{
	uint8_t smallRumble;
	uint8_t largeRumble;
	uint8_t r, g, b;
	uint8_t flashOn;
	uint8_t flashOff; // Time to flash bright/dark (255 = 2.5 seconds)
} __attribute__((packed)) ps5_cmd_t;

typedef struct
{
	ps5_button_t button_down;
	ps5_button_t button_up;
	ps5_analog_t analog_move;
} __attribute__((packed)) ps5_event_t;

typedef struct
{
	ps5_analog_t analog;
	ps5_button_t button;
	ps5_status_t status;
	ps5_sensor_t sensor;
	bool latestPacket;
	uint8_t controller_connected;
	uint16_t main_battery;
	uint8_t checksum;
} __attribute__((packed)) ps5_t;
