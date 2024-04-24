
#include "driver/rmt_rx.h"
#include "driver/rmt_tx.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "hardware.h"
#include "mb_crc.h"
#include "tinbus.h"

#define TINBUS_RESOLUTION_HZ 1000000 // 1 MHz resolution, 1 tick = 10 us

#define TINBUS_RMT_MEM_BLOCK_SYMBOLS 64
#define TINBUS_RX_QUEUE_SIZE 8
#define TINBUS_FRAME_SIZE 5

#define TINBUS_RX_MASK 0x80

typedef struct tinbus_rx_data_t {
    rmt_symbol_word_t symbols[TINBUS_RMT_MEM_BLOCK_SYMBOLS];
    size_t size;
} tinbus_rx_data_t;

static const char *TAG = "tinbus";

static rmt_channel_handle_t rx_channel = NULL;
static tinbus_rx_data_t tinbus_rx_data;
static QueueHandle_t rx_queue = NULL;

static rmt_receive_config_t receive_config = {
    .signal_range_min_ns = 3000,   // < 3 us signal will be treated as noise (hardware limit is 1000 * 255 / 80)
    .signal_range_max_ns = 400000, // > 8 T end of tinbus frame
};

static esp_err_t tinbus_parse_frame(const tinbus_rx_data_t *rx_data, tinbus_msg_t *msg) {
    uint8_t mask = TINBUS_RX_MASK;
    uint8_t buffer[TINBUS_FRAME_SIZE] = {0};
    uint8_t index = 0;

    if (rx_data->size != (5 * 8) + 1) {
        hardware_debug(HARDWARE_RELAY3);
        ESP_LOGI(TAG, "Bad frame size %d", rx_data->size);
        return ESP_FAIL;
    }
    for (size_t i = 0; i < (rx_data->size - 1); i++) { // ignore the last sysmbol
        if ((rx_data->symbols[i].level0 != 1) || (rx_data->symbols[i].level1 != 0)) {
            hardware_debug(HARDWARE_RELAY3);
            ESP_LOGI(TAG, "Bad polarity");
            return ESP_FAIL;
        }
        if ((rx_data->symbols[i].duration0 < 33) || (rx_data->symbols[i].duration0 > 75)) {
            hardware_debug(HARDWARE_RELAY3);
            ESP_LOGI(TAG, "Bad bus active duration %d us", rx_data->symbols[i].duration0);
            return ESP_FAIL;
        }
        if ((rx_data->symbols[i].duration1 < 66) || (rx_data->symbols[i].duration1 > 400)) {
            hardware_debug(HARDWARE_RELAY3);
            ESP_LOGI(TAG, "Bad bus idle duration %d us", rx_data->symbols[i].duration1);
            return ESP_FAIL;
        }
        if ((rx_data->symbols[i].duration0 + rx_data->symbols[i].duration1) > 200) {
            buffer[index] |= mask;
        }
        mask >>= 1;
        if (mask == 0) {
            mask = TINBUS_RX_MASK;
            index++;
        }
    }
    if (mb_crc_is_ok(buffer, TINBUS_FRAME_SIZE)) {
        msg->device_id = buffer[0];
        msg->value = ((uint16_t)buffer[1] << 8) | (uint16_t)buffer[2];
        return ESP_OK;
    }
    return ESP_FAIL;
}

static bool IRAM_ATTR tinbus_rmt_rx_done_callback(rmt_channel_handle_t channel, const rmt_rx_done_event_data_t *edata,
                                                  void *user_data) {
    BaseType_t high_task_wakeup = pdFALSE;
    QueueHandle_t queue = (QueueHandle_t)user_data;
    if (tinbus_rx_data.symbols == edata->received_symbols) {
        tinbus_rx_data.size = edata->num_symbols;
        // place rx data in queue ready for parser to process
        xQueueSendFromISR(queue, &tinbus_rx_data, &high_task_wakeup);
    }
    // restart the receiver
    rmt_receive(rx_channel, tinbus_rx_data.symbols, sizeof(tinbus_rx_data.symbols), &receive_config);
    return high_task_wakeup == pdTRUE;
}

esp_err_t tinbus_init(void) {

    ESP_LOGI(TAG, "create RMT RX channel");
    rmt_rx_channel_config_t rx_channel_cfg = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = TINBUS_RESOLUTION_HZ,
        .mem_block_symbols = TINBUS_RMT_MEM_BLOCK_SYMBOLS, // amount of RMT symbols that the channel can store at a time
        .gpio_num = HARDWARE_BUS_RX_GPIO,
        .flags.invert_in = true,
    };

    ESP_ERROR_CHECK(rmt_new_rx_channel(&rx_channel_cfg, &rx_channel));

    ESP_LOGI(TAG, "register RX done callback");
    rx_queue = xQueueCreate(TINBUS_RX_QUEUE_SIZE, sizeof(tinbus_rx_data_t));
    assert(rx_queue);

    rmt_rx_event_callbacks_t cbs = {
        .on_recv_done = tinbus_rmt_rx_done_callback,
    };
    ESP_ERROR_CHECK(rmt_rx_register_event_callbacks(rx_channel, &cbs, rx_queue));
    ESP_ERROR_CHECK(rmt_enable(rx_channel));
    ESP_ERROR_CHECK(rmt_receive(rx_channel, tinbus_rx_data.symbols, sizeof(tinbus_rx_data.symbols), &receive_config));
    return ESP_OK;
}

esp_err_t tinbus_read(tinbus_msg_t *msg) {
    tinbus_rx_data_t rx_data;
    if (xQueueReceive(rx_queue, &rx_data, 0) == pdPASS) {
        return tinbus_parse_frame(&rx_data, msg);
    }
    return ESP_FAIL;
}
