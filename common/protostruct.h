/*
 * protostruct.h
 *
 *  Created on: 2015-5-19
 *      Author: lsy
 */

#ifndef PROTOSTRUCT_H_
#define PROTOSTRUCT_H_

const int PROTO_HEAD_BYTE_SIZE = 4;

const int REGISTER_TO_SERVER = 1; //注册到服务器
const int FETCH_P2PSERVER_LIST = 2; //获取可分享终端
const int REQUEST_SERVER_TODO_CONNECT_P2P = 3; //请求服务器让远端与本地链接，发送ready 指令

//REGISTER_TO_SERVER[4] + ip [20] port[4]
//FETCH_P2PSERVER_LIST[4]

////////////////////////////////////////
//客户端格式
//格式：
// 10 byte【request】【respond】
// 10 byte【status】【data】 [apk]
// 10status->[ready][notready] data->[doConnect][p2pList][fileName] apk->[filename][sector]
//
//  :
//control -> ip [16] port[4]
//

//data: fileName [255]sector[4][len][data]

#endif /* PROTOSTRUCT_H_ */
