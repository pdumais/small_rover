#include "uart.h"
#include "hardware.h"
#include <esp_system.h>
#include <esp_event.h>
#include <esp_log.h>
#include "driver/uart.h"
#include "hw_def.h"
#include "common/ps5_data.h"
#include "wifi.h"

static const char *TAG = "uart_c";
static TaskHandle_t uart_task;

void send_to_uart(controller_message *msg)
{
	uart_write_bytes(UART_NUM_2, msg, sizeof(controller_message));
}

void check_uart(void *arg)
{
	ps5_t *data = malloc(sizeof(ps5_t));
	while (1)
	{
		size_t size = uart_read_bytes(UART_NUM_2, data, sizeof(ps5_t), portMAX_DELAY);

		// char str[100];
		// sprintf(&str, "SIZE: %i\n", size);
		// broadcast_log(&str);

		if (size == sizeof(ps5_t))
		{

			queue_msg msg;
			msg.data = data;
			msg.type = MESSAGE_PS5;
			hardware_send_message(&msg);

			// Create another buffer since the last wont will be owned by the event receiver
			data = malloc(sizeof(ps5_t));
		}
	}
}

void uart_stop()
{
	vTaskSuspend(uart_task);
}

void uart_init(void)
{
	ESP_LOGI(TAG, "uart_init");

	ESP_ERROR_CHECK(uart_driver_install(UART_NUM_2, 1024, 0, 0, 0, 0));

	const uart_port_t uart_num = UART_NUM_2;
	uart_config_t uart_config = {
		.baud_rate = 115200,
		.data_bits = UART_DATA_8_BITS,
		.parity = UART_PARITY_DISABLE,
		.stop_bits = UART_STOP_BITS_1,
		.flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
		.rx_flow_ctrl_thresh = 122};
	ESP_ERROR_CHECK(uart_param_config(uart_num, &uart_config));
	ESP_ERROR_CHECK(uart_set_pin(UART_NUM_2, UART_TX_PIN, UART_RX_PIN, -1, -1));

	xTaskCreate(check_uart, "check_uart", 4096, NULL, 5, &uart_task);
}
