#ifndef MB_CRC_H
#define MB_CRC_H

#include <stdint.h>
#include <stdbool.h>

uint16_t mb_crc(uint16_t crc, uint8_t databyte);

bool mb_crc_is_ok(uint8_t data[], uint8_t size);

#endif //MB_CRC_H
