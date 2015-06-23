/*
 * datacheck.cpp
 *
 *  Created on: 2015-6-19
 *      Author: lsy
 */

#include "datacheck.h"

uint16_t dataCheck16(uint8_t *pData, int size) {
    uint16_t *pU16Data = (uint16_t*) pData;
    unsigned long cksum = 0;

    while (size > 1) {
        cksum += *pU16Data++;
        size -= sizeof(uint16_t);
    }
    if (size > 0) {
        cksum += *(uint8_t *) pU16Data;
    }

    while((cksum >> 16) > 0)
    {
        cksum = (cksum >> 16) + (cksum & 0xffff);
    }
    return (uint16_t)(~cksum);
}

uint8_t dataCheck8(uint8_t *pData, int size) {
    uint8_t *pU8Data = (uint8_t*) pData;
    unsigned long cksum = 0;

    while (size > 0) {
        cksum += *pU8Data++;
        size -= sizeof(uint8_t);
    }
    while((cksum >> 8) > 0)
    {
        cksum = (cksum >> 8) + (cksum & 0xff);
    }
    return (uint16_t)(~cksum);
}

