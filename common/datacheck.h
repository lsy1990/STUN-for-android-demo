/*
 * datacheck.h
 *
 *  Created on: 2015-6-19
 *      Author: lsy
 */

#ifndef DATACHECK_H_
#define DATACHECK_H_

#include "stdint.h"

//create 16bits check data
uint16_t dataCheck16(uint8_t *pData,int size);

uint8_t dataCheck8(uint8_t *pData,int size);
#endif /* DATACHECK_H_ */
