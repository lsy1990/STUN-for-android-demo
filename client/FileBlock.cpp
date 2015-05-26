/*
 * FileBlock.cpp
 *
 *  Created on: 2015-5-19
 *      Author: lsy
 */

#include "FileBlock.h"


FileBlock::FileBlock(uint8_t *buff, int size, int index) {
    if (buff != NULL && size > 0 && size <= BLOCK_MAX_LENGTH) {
        mBuffer = new uint8_t[size];
        memcpy(mBuffer, buff, size);
        mBufferSize = size;
        mIndex = index;
    }
    else {
        mBuffer = NULL;
    }
}

FileBlock::~FileBlock() {
    if (mBuffer != NULL) {
        delete[] mBuffer;
        mBuffer = NULL;
    }
    mBufferSize = 0;
}
uint8_t *FileBlock::getData() {
    return mBuffer;
}
int FileBlock::getDataSize() {
    return mBufferSize;
}
int FileBlock::getIndex(){
    return mIndex;
}
bool FileBlock::addTryCountToUp(){
//    mTryCount++;
//    if(mTryCount < MAX_TIMES_TRY_TO_SEND){
//        return true;
//    }
    return false;
}
