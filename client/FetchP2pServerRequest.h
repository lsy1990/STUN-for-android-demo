/*
 * FetchP2pServerRequest.h
 *
 *  Created on: 2015-5-20
 *      Author: lsy
 */

#ifndef FETCHP2PSERVERREQUEST_H_
#define FETCHP2PSERVERREQUEST_H_

class FetchP2pServerRequest {
    public:
        FetchP2pServerRequest();
        virtual ~FetchP2pServerRequest();
        int getDataSize();
        uint8_t* getData();
    private:
        uint8_t *mData;
};

#endif /* FETCHP2PSERVERREQUEST_H_ */
