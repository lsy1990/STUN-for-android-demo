/*
 * FetchP2pServerRequest.cpp
 *
 *  Created on: 2015-5-20
 *      Author: lsy
 */
#include "protostruct.h"
#include    <stdio.h>
#include <commonincludes.hpp>
#include "FetchP2pServerRequest.h"
#define FETCH_P2P_SERVER_REQUEST_BODY_SIZE 0
FetchP2pServerRequest::FetchP2pServerRequest() {
    mData = new uint8_t[FETCH_P2P_SERVER_REQUEST_BODY_SIZE
            + PROTO_HEAD_BYTE_SIZE];
    memcpy((void*) mData, &FETCH_P2PSERVER_LIST, 4);
}

FetchP2pServerRequest::~FetchP2pServerRequest() {
    if (mData != NULL) {
        delete[] mData;
        mData = NULL;
    }
}

int FetchP2pServerRequest::getDataSize() {
    return FETCH_P2P_SERVER_REQUEST_BODY_SIZE + PROTO_HEAD_BYTE_SIZE;
}
uint8_t* FetchP2pServerRequest::getData() {
    return mData;
}
