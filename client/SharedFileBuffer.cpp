/*
 * SharedFileBuffer.cpp
 *
 *  Created on: 2015-5-19
 *      Author: lsy
 */

#include "SharedFileBuffer.h"

#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#
SharedFileBuffer::SharedFileBuffer(char* fileName) {

    char filePath[255];
    strcpy(filePath, "/data/");
    strcat(filePath,fileName);
    mFiledescription = fopen(filePath, "r");
    printf("mFiledescription == %d",mFiledescription);

}

SharedFileBuffer::SharedFileBuffer(char* fileName, int mode) {

    char filePath[255];
    strcpy(filePath, "/data/");
    strcat(filePath,fileName);
    mFiledescription = fopen(filePath, "w");

}
bool SharedFileBuffer::loadFileToBuffer() {
    if (mFiledescription != NULL) {
        uint8_t *localBuffer = new uint8_t[BLOCK_MAX_LENGTH];
        int index = 0;
        ssize_t size = fread((void *) localBuffer, 1, BLOCK_MAX_LENGTH,
                mFiledescription);
        while (size > 0) {
            FileBlock *fileBlock = new FileBlock(localBuffer, size, index);
            mFileBlockList.push_back(fileBlock);
            index++;
            size = fread((void *) localBuffer, 1, BLOCK_MAX_LENGTH,
                    mFiledescription);
        }
        if (size == -1) {
            //error
            mFileBlockList.clear();
            return false;
        }
        return true;
    }
    return false;
}

bool SharedFileBuffer::writeData(uint8_t *pData, int size) {
    if (mFiledescription != NULL) {
        ssize_t realsize = fwrite((void *) pData, 1, size, mFiledescription);
        return true;
    }
    return false;
}
void SharedFileBuffer::flushData() {

}
SharedFileBuffer::~SharedFileBuffer() {
    if (mFiledescription != NULL) {
        fclose(mFiledescription);
    }
    mFileBlockList.clear();
}
int SharedFileBuffer::getBlockSize() {
    return mFileBlockList.size();
}
FileBlock* SharedFileBuffer::getAt(int i) {
    return mFileBlockList.operator [](i);
}

