/*
 * LocalSharedFileManage.h
 *
 *  Created on: 2015-5-19
 *      Author: lsy
 */

#ifndef LOCALSHAREDFILEMANAGE_H_
#define LOCALSHAREDFILEMANAGE_H_

#define FILE_NAME_MAXLENGTH  100
#define FILE_PATH_MAXLENGTH  255
#define SHARED_FILE_MAX_COUNT  7
class LocalSharedFileManage {
    public:
        LocalSharedFileManage();
        virtual ~LocalSharedFileManage();
        char **mLocalSharedFileName;
        int mLocalSharedFileCount;
        char mLocalSharedFilePath[FILE_PATH_MAXLENGTH];

    public:
        int addNewSharedFile(char *fileName);
        int getSharedFileCount();
        bool getSharedFileName(int index, char *&fileName);
};

#endif /* LOCALSHAREDFILEMANAGE_H_ */
