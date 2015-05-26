/*
 * SharedFileBuffer.h
 *
 *  Created on: 2015-5-19
 *      Author: lsy
 */

#ifndef SHAREDFILEBUFFER_H_
#define SHAREDFILEBUFFER_H_

#include <vector>
#include <FileBlock.h>
#include <stdio.h>

class SharedFileBuffer {
    public:
        SharedFileBuffer(char* fileName);
        SharedFileBuffer(char* fileName,int mode);
        virtual ~SharedFileBuffer();
        bool loadFileToBuffer();
        bool writeData(uint8_t *pData,int size);
        void flushData();
    private:
        FILE *mFiledescription; //
        std::vector<FileBlock*> mFileBlockList;
    public:
        int getBlockSize();
        FileBlock* getAt(int i);
        void removeBackFileBlock();
};

#endif /* SHAREDFILEBUFFER_H_ */
