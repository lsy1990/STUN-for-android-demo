/*
 * RegisterRequest.h
 *
 *  Created on: 2015-5-19
 *      Author: lsy
 */

#ifndef REGISTERREQUEST_H_
#define REGISTERREQUEST_H_

#include "stdint.h"
#include "string.h"

class RegisterRequest {
    public:
        RegisterRequest(char* ip, uint16_t port);
        virtual ~RegisterRequest();
        int getDataSize();
        uint8_t* getData();
    private:
        uint8_t *mData;
};

#endif /* REGISTERREQUEST_H_ */
