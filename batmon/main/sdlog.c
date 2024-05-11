#include <stdbool.h>

#include "sdlog.h"

#define SDLOG_MAGIC (0x666)

#define SDLOG_TASK_PRIO 8

typedef struct sdlog_header_t {
    uint16_t magic;
    uint16_t crc;
} sdlog_header_t;

#define SDLOG_DATA_SIZE (SDLOG_BLOCK_SIZE - sizeof(sdlog_header_t))

typedef struct sdlog_block_t {
    sdlog_header_t header;
    uint8_t data[SDLOG_DATA_SIZE];
} sdlog_block_t;

typedef struct sdlog_ctx_t {
    bool initialised;

    sdlog_block_t wr_buffer;
    uint16_t wr_buffer_size;

    uint32_t wr_block;
    uint32_t read_block;
} sdlog_ctx_t;

sdlog_ctx_t ctx = {0};

uint16_t sdlog_crc16(uint8_t *buf, uint16_t crc, uint16_t size) {
    for (uint16_t index = 0; index < size; index++) {
        crc = crc ^ buf[index];
        for (uint8_t i = 0; i < 8; ++i) {
            if (crc & 1)
                crc = (crc >> 1) ^ 0xA001;
            else
                crc = (crc >> 1);
        }
    }
    return crc;
}



sdlog_err_t sdlog_read(sdlog_msg_t *msg) {}

sdlog_err_t sdlog_write(sdlog_msg_t *msg) {
    if (msg->size <= SDLOG_DATA_SIZE - ctx.wr_buffer_size) {
        // if msg fits then append it to the current write buffer
        memcpy(&ctx.wr_buffer.data[ctx.wr_buffer_size], msg->data, msg->size);
        ctx.wr_buffer_size += msg->size;
    } else {
        // if msg doesnt fit then write the buffer to flash and start over
        ctx.wr_buffer.header.magic = SDLOG_MAGIC;
        ctx.wr_buffer.header.crc = sdlog_crc16(&ctx.wr_buffer, 0xFFFF, SDLOG_BLOCK_SIZE);
        sdlog_write_block(&ctx.wr_block, &ctx.wr_buffer, SDLOG_BLOCK_SIZE);
        memset(ctx.wr_buffer, 0x00, SDLOG_BLOCK_SIZE);
        memcpy(&ctx.wr_buffer.data[0], msg->data, msg->size);
        ctx.wr_buffer_size = msg->size;
    }
}

sdlog_err_t sdlog_erase(void) {}