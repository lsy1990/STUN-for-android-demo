/*
 * RegisterRequest.cpp
 *
 *  Created on: 2015-5-19
 *      Author: lsy
 */

#include "RegisterRequest.h"
#include "protostruct.h"
#include    <stdio.h>
#include <commonincludes.hpp>
#define REGISTER_REQUEST_BODY_SIZE 6
RegisterRequest::RegisterRequest(char* ip, uint16_t port) {
    if (ip != NULL && port > 0) {
        mData = new uint8_t[REGISTER_REQUEST_BODY_SIZE + PROTO_HEAD_BYTE_SIZE];
        memcpy((void*) mData, &REGISTER_TO_SERVER, 4);

        sockaddr_in addr4 = { };
        addr4.sin_family = AF_INET;
        ::inet_pton(AF_INET, ip, &(addr4.sin_addr));
        memcpy((void*) mData + 4, &addr4.sin_addr, sizeof(uint32_t)); //net
        memcpy((void*) mData + 8, &port, sizeof(uint16_t));
    }
    else {
        mData = NULL;
    }
}

RegisterRequest::~RegisterRequest() {
    if (mData != NULL) {
        delete[] mData;
        mData = NULL;
    }
}
int RegisterRequest::getDataSize() {
    return REGISTER_REQUEST_BODY_SIZE + PROTO_HEAD_BYTE_SIZE;
}
uint8_t* RegisterRequest::getData() {
    return mData;
}
