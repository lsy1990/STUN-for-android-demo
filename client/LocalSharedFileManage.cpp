/*
 * LocalSharedFileManage.cpp
 *
 *  Created on: 2015-5-19
 *      Author: lsy
 */

#include "LocalSharedFileManage.h"
#include "string.h"
LocalSharedFileManage::LocalSharedFileManage() {
    // TODO Auto-generated constructor stub
    mLocalSharedFileName = new char*[SHARED_FILE_MAX_COUNT];
    for (int i = 0; i < SHARED_FILE_MAX_COUNT; i++) {
        mLocalSharedFileName[i] = new char[FILE_NAME_MAXLENGTH];
        memset(mLocalSharedFileName, FILE_NAME_MAXLENGTH, 0);
    }
    mLocalSharedFileCount = 0;
    strcpy(mLocalSharedFilePath, "/data/");
    addNewSharedFile("log.txt");
}

LocalSharedFileManage::~LocalSharedFileManage() {
    for (int i = 0; i < SHARED_FILE_MAX_COUNT; i++) {
        delete[] mLocalSharedFileName[i];
        mLocalSharedFileName[i] = NULL;
    }
    delete[] mLocalSharedFileName;
    mLocalSharedFileName = NULL;
}

int LocalSharedFileManage::addNewSharedFile(char *fileName) {
    if (mLocalSharedFileCount < SHARED_FILE_MAX_COUNT) {
        strcpy(mLocalSharedFileName[mLocalSharedFileCount], fileName);
        mLocalSharedFileCount++;
        return mLocalSharedFileCount;
    }
    return 0;
}

int LocalSharedFileManage::getSharedFileCount() {
    return mLocalSharedFileCount;
}

bool LocalSharedFileManage::getSharedFileName(int index, char *&fileName) {
    if (index >= 0 && index < mLocalSharedFileCount) {
        strncpy(fileName, mLocalSharedFilePath,FILE_PATH_MAXLENGTH);
        strncat(fileName, mLocalSharedFileName[index],FILE_PATH_MAXLENGTH);
        return true;
    }
    return false;
}
