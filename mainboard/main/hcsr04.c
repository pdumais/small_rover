#include "hcsr04.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_timer.h>
#include <esp_log.h>
#include "driver/gpio.h"
#include <rom/ets_sys.h>

static const char *TAG = "hcsr04_c";
static portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
static QueueHandle_t receive_queue;
static rmt_symbol_word_t raw_symbols[64]; // 64 symbols should be sufficient for a standard NEC frame

#define CM_US 10
static rmt_receive_config_t receive_config = {
    .signal_range_min_ns = CM_US,
    .signal_range_max_ns = 12000000,
};

static bool rmt_rx_done_callback(rmt_channel_handle_t channel, const rmt_rx_done_event_data_t *edata, void *user_data)
{
    BaseType_t high_task_wakeup = pdFALSE;
    QueueHandle_t receive_queue = (QueueHandle_t)user_data;
    xQueueSendFromISR(receive_queue, edata, &high_task_wakeup);
    return high_task_wakeup == pdTRUE;
}

uint16_t hcsr04_read(hcsr04_handle_t *h, uint32_t wait_time)
{
    rmt_rx_done_event_data_t rx_data;
    // Go thru the whole list if there are more than one event
    if (rmt_receive(h->rx_channel, raw_symbols, sizeof(raw_symbols), &receive_config) == ESP_FAIL)
    {
        return 0xFFFF;
    }
    if (xQueueReceive(receive_queue, &rx_data, wait_time / portTICK_PERIOD_MS))
    {
        return rx_data.received_symbols[0].duration0 / 58;
    }

    return 0xFFFF;
}

void hcsr04_trigger(hcsr04_handle_t *h)
{
    static const rmt_item32_t pulseRMT[] = {
        {{{10, 1, 1000000, 0}}},
    };
    rmt_transmit_config_t tx_config = {
        .loop_count = 0,
    };

    ESP_ERROR_CHECK(rmt_transmit(h->tx_channel, h->encoder, pulseRMT, 1 * sizeof(rmt_symbol_word_t), &tx_config));
}

void hcsr04_init(uint8_t trigger, uint8_t echo, hcsr04_handle_t *h)
{
    h->trigger_pin = trigger;
    h->echo_pin = echo;

    rmt_tx_channel_config_t tx_chan_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .gpio_num = trigger,
        .mem_block_symbols = 64,
        .resolution_hz = 1 * 1000 * 1000, // 1 MHz tick resolution, i.e., 1 tick = 1 µs
        .trans_queue_depth = 4,
        .flags.invert_out = false,
        .flags.with_dma = false,
    };
    ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_chan_config, &h->tx_channel));
    ESP_ERROR_CHECK(rmt_enable(h->tx_channel));

    rmt_rx_channel_config_t rx_chan_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .gpio_num = echo,
        .mem_block_symbols = 64,
        .resolution_hz = 1 * 1000 * 1000, // 1 MHz tick resolution, i.e., 1 tick = 1 µs
        .flags.invert_in = false,
        .flags.with_dma = false,
    };
    ESP_ERROR_CHECK(rmt_new_rx_channel(&rx_chan_config, &h->rx_channel));
    ESP_ERROR_CHECK(rmt_enable(h->rx_channel));

    rmt_copy_encoder_config_t encoder_config;

    ESP_ERROR_CHECK(rmt_new_copy_encoder(&encoder_config, &h->encoder));

    receive_queue = xQueueCreate(10, sizeof(rmt_rx_done_event_data_t));
    rmt_rx_event_callbacks_t cbs = {
        .on_recv_done = rmt_rx_done_callback,
    };
    ESP_ERROR_CHECK(rmt_rx_register_event_callbacks(h->rx_channel, &cbs, receive_queue));
}
