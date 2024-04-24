#include <stdint.h>

#include "mb_crc.h"

uint16_t mb_crc(uint16_t crc, uint8_t databyte) {
    int i;
    crc = crc ^ databyte;
    for (i = 0; i < 8; ++i) {
        if (crc & 1)
            crc = (crc >> 1) ^ 0xA001;
        else
            crc = (crc >> 1);
    }
    return crc;
}

bool mb_crc_is_ok(uint8_t data[], uint8_t size){
	uint16_t crc = 0xFFFF;
	for(uint8_t i = 0; i < size; i++){
		crc = mb_crc(crc, data[i]);
	}
	if(crc){
		return false;
	}
	return true;
}
