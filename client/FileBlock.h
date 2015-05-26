/*
 * FileBlock.h
 *
 *  Created on: 2015-5-19
 *      Author: lsy
 */

#ifndef FILEBLOCK_H_
#define FILEBLOCK_H_

#include "stdint.h"
#include "string.h"

#define BLOCK_MAX_LENGTH 4096
#define MAX_TIMES_TRY_TO_SEND  10
class FileBlock {
    public:
        FileBlock(uint8_t *buff, int size,int index);
        virtual ~FileBlock();
    private:
        uint8_t *mBuffer;
        int mBufferSize;
        //块号标识
        int mIndex;
    public:
        uint8_t *getData();
        int getDataSize();
        int getIndex();
        bool addTryCountToUp();
};

#endif /* FILEBLOCK_H_ */
