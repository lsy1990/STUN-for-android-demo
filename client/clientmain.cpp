/*
 Copyright 2011 John Selbie

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 */

#include "commonincludes.hpp"
#include "stuncore.h"
#include "socketrole.h" // so we can re-use the "SocketRole" definitions again
#include "stunsocket.h"
#include "cmdlineparser.h"
#include "recvfromex.h"
#include "resolvehostname.h"
#include "stringhelper.h"
#include "adapters.h"
#include "oshelper.h"
#include "prettyprint.h"
#include <vector>
#include "SharedFileBuffer.h"
// These files are in ../resources
#include "stunclient.txtcode"
#include "stunclient_lite.txtcode"

#include "RegisterRequest.h"
#include "FetchP2pServerRequest.h"
#include "protostruct.h"

struct ClientCmdLineArgs {
        std::string strRemoteServer;
        std::string strRemotePort;
        std::string strLocalAddr;
        std::string strLocalPort;
        std::string strMode;
        std::string strFamily;
        std::string strProtocol;
        std::string strVerbosity;
        std::string strHelp;
};

struct ClientSocketConfig {
        int family;
        int socktype;
        CSocketAddress addrLocal;
};

void DumpConfig(StunClientLogicConfig& config,
        ClientSocketConfig& socketConfig) {
    std::string strAddr;

    Logging::LogMsg(LL_DEBUG, "config.fBehaviorTest = %s",
            config.fBehaviorTest ? "true" : "false");
    Logging::LogMsg(LL_DEBUG, "config.fFilteringTest = %s",
            config.fFilteringTest ? "true" : "false");
    Logging::LogMsg(LL_DEBUG, "config.timeoutSeconds = %d",
            config.timeoutSeconds);
    Logging::LogMsg(LL_DEBUG, "config.uMaxAttempts = %d", config.uMaxAttempts);

    config.addrServer.ToString(&strAddr);
    Logging::LogMsg(LL_DEBUG, "config.addrServer = %s", strAddr.c_str());

    socketConfig.addrLocal.ToString(&strAddr);
    Logging::LogMsg(LL_DEBUG, "socketconfig.addrLocal = %s", strAddr.c_str());

}

void PrintUsage(bool fSummaryUsage) {
    size_t width = GetConsoleWidth();
    const char* psz = fSummaryUsage ? stunclient_lite_text : stunclient_text;

    // save some margin space
    if (width > 2) {
        width -= 2;
    }

    PrettyPrint(psz, width);
}

HRESULT CreateConfigFromCommandLine(ClientCmdLineArgs& args,
        StunClientLogicConfig* pConfig, ClientSocketConfig* pSocketConfig) {
    HRESULT hr = S_OK;
    StunClientLogicConfig& config = *pConfig;
    ClientSocketConfig& socketconfig = *pSocketConfig;
    int ret;
    uint16_t localport = 0;
    uint16_t remoteport = 0;
    int nPort = 0;
    char szIP[100];
    bool fTCP = false;

    config.fBehaviorTest = false;
    config.fFilteringTest = false;
    config.fTimeoutIsInstant = false;
    config.timeoutSeconds = 0; // use default
    config.uMaxAttempts = 0;

    socketconfig.family = AF_INET;
    socketconfig.socktype = SOCK_DGRAM;
    socketconfig.addrLocal = CSocketAddress(0, 0);

    ChkIfA(pConfig == NULL, E_INVALIDARG);
    ChkIfA(pSocketConfig==NULL, E_INVALIDARG);

    // family (protocol type) ------------------------------------
    if (StringHelper::IsNullOrEmpty(args.strFamily.c_str()) == false) {
        int optvalue = atoi(args.strFamily.c_str());
        switch (optvalue) {
            case 4:
                socketconfig.family = AF_INET;
            break;
            case 6:
                socketconfig.family = AF_INET6;
            break;
            default: {
                Logging::LogMsg(LL_ALWAYS,
                        "Family option must be either 4 or 6");
                Chk(E_INVALIDARG);
            }
        }
    }
    
    // protocol --------------------------------------------
    StringHelper::ToLower(args.strProtocol);
    if (StringHelper::IsNullOrEmpty(args.strProtocol.c_str()) == false) {
        if ((args.strProtocol != "udp") && (args.strProtocol != "tcp")) {
            Logging::LogMsg(LL_ALWAYS,
                    "Only udp and tcp are supported protocol versions");
            Chk(E_INVALIDARG);
        }
        
        if (args.strProtocol == "tcp") {
            fTCP = true;
            socketconfig.socktype = SOCK_STREAM;
            config.uMaxAttempts = 1;
        }
    }

    // remote port ---------------------------------------------
    if (StringHelper::IsNullOrEmpty(args.strRemotePort.c_str()) == false) {
        ret = StringHelper::ValidateNumberString(args.strRemotePort.c_str(), 1,
                0xffff, &nPort);
        if (ret < 0) {
            Logging::LogMsg(LL_ALWAYS, "Remote port must be between 1 - 65535");
            Chk(E_INVALIDARG);
        }
        remoteport = (uint16_t) (unsigned int) nPort;
    }
    else {
        remoteport = DEFAULT_STUN_PORT;
    }

    // remote server -----------------------------------------
    if (StringHelper::IsNullOrEmpty(args.strRemoteServer.c_str())) {
        Logging::LogMsg(LL_ALWAYS, "No server address specified");
        Chk(E_INVALIDARG);
    }
    
    hr = ::ResolveHostName(args.strRemoteServer.c_str(), socketconfig.family,
            false, &config.addrServer);
    
    if (FAILED(hr)) {
        Logging::LogMsg(LL_ALWAYS, "Unable to resolve hostname for %s",
                args.strRemoteServer.c_str());
        Chk(hr);
    }
    
    config.addrServer.ToStringBuffer(szIP, ARRAYSIZE(szIP));
    Logging::LogMsg(LL_DEBUG, "Resolved %s to %s", args.strRemoteServer.c_str(),
            szIP);
    config.addrServer.SetPort(remoteport);

    // local port --------------------------------------------
    if (StringHelper::IsNullOrEmpty(args.strLocalPort.c_str()) == false) {
        ret = StringHelper::ValidateNumberString(args.strLocalPort.c_str(), 1,
                0xffff, &nPort);
        if (ret < 0) {
            Logging::LogMsg(LL_ALWAYS, "Local port must be between 1 - 65535");
            Chk(E_INVALIDARG);
        }
        localport = (uint16_t) (unsigned int) nPort;
    }

    // local address ------------------------------------------
    if (StringHelper::IsNullOrEmpty(args.strLocalAddr.c_str()) == false) {
        hr = GetSocketAddressForAdapter(socketconfig.family,
                args.strLocalAddr.c_str(), localport, &socketconfig.addrLocal);
        if (FAILED(hr)) {
            Logging::LogMsg(LL_ALWAYS,
                    "Unable to find matching adapter or interface for local address option");
            Chk(hr);
        }
    }
    else {
        if (socketconfig.family == AF_INET6) {
            sockaddr_in6 addr6 = { };
            addr6.sin6_family = AF_INET6;
            socketconfig.addrLocal = CSocketAddress(addr6);
            socketconfig.addrLocal.SetPort(localport);
        }
        else {
            socketconfig.addrLocal = CSocketAddress(0, localport);
        }
    }

    // mode ---------------------------------------------
    StringHelper::ToLower(args.strMode);
    if (StringHelper::IsNullOrEmpty(args.strMode.c_str()) == false) {
        if (args.strMode == "basic") {
            ;
        }
        else if (args.strMode == "full") {
            config.fBehaviorTest = true;
            config.fFilteringTest = (fTCP == false); // impossible to to a filtering test in TCP
        }
        else {
            Logging::LogMsg(LL_ALWAYS, "Mode option must be 'full' or 'basic'");
        }
    }

    Cleanup: return hr;
}

void NatBehaviorToString(NatBehavior behavior, std::string* pStr) {
    std::string& str = *pStr;

    switch (behavior) {
        case UnknownBehavior:
            str = "Unknown Behavior";
        break;
        case DirectMapping:
            str = "Direct Mapping";
        break;
        case EndpointIndependentMapping:
            str = "Endpoint Independent Mapping";
        break;
        case AddressDependentMapping:
            str = "Address Dependent Mapping";
        break;
        case AddressAndPortDependentMapping:
            str = "Address and Port Dependent Mapping";
        break;
        default:
            ASSERT(false);
            str = "";
        break;
    }
}

void NatFilteringToString(NatFiltering filtering, std::string* pStr) {
    std::string& str = *pStr;

    switch (filtering) {
        case UnknownFiltering:
            str = "Unknown Behavior";
        break;
        case DirectConnectionFiltering:
            str = "Direct Mapping";
        break;
        case EndpointIndependentFiltering:
            str = "Endpoint Independent Filtering";
        break;
        case AddressDependentFiltering:
            str = "Address Dependent Filtering";
        break;
        case AddressAndPortDependentFiltering:
            str = "Address and Port Dependent Filtering";
        break;
        default:
            ASSERT(false);
            str = "";
        break;
    }
}

void DumpResults(StunClientLogicConfig& config, StunClientResults& results) {
    char szBuffer[100];
    const int buffersize = 100;
    std::string strResult;

    Logging::LogMsg(LL_ALWAYS, "Binding test: %s",
            results.fBindingTestSuccess ? "success" : "fail");
    if (results.fBindingTestSuccess) {
        results.addrLocal.ToStringBuffer(szBuffer, buffersize);
        Logging::LogMsg(LL_ALWAYS, "Local address: %s", szBuffer);

        results.addrMapped.ToStringBuffer(szBuffer, buffersize);
        Logging::LogMsg(LL_ALWAYS, "Mapped address: %s", szBuffer);
    }

    if (config.fBehaviorTest) {

        Logging::LogMsg(LL_ALWAYS, "Behavior test: %s",
                results.fBehaviorTestSuccess ? "success" : "fail");
        if (results.fBehaviorTestSuccess) {
            NatBehaviorToString(results.behavior, &strResult);
            Logging::LogMsg(LL_ALWAYS, "Nat behavior: %s", strResult.c_str());
        }
    }

    if (config.fFilteringTest) {
        Logging::LogMsg(LL_ALWAYS, "Filtering test: %s",
                results.fFilteringTestSuccess ? "success" : "fail");
        if (results.fFilteringTestSuccess) {
            NatFilteringToString(results.filtering, &strResult);
            Logging::LogMsg(LL_ALWAYS, "Nat filtering: %s", strResult.c_str());
        }
    }
}

void TcpClientLoop(StunClientLogicConfig& config,
        ClientSocketConfig& socketconfig) {
    
    HRESULT hr = S_OK;
    CStunSocket stunsocket;
    CStunClientLogic clientlogic;
    int sock;
    CRefCountedBuffer spMsg(new CBuffer(1500));
    CRefCountedBuffer spMsgReader(new CBuffer(1500));
    CSocketAddress addrDest, addrLocal;
    HRESULT hrRet, hrResult;
    int ret;
    size_t bytes_sent, bytes_recv;
    size_t bytes_to_send, max_bytes_recv, remaining;
    uint8_t* pData = NULL;
    size_t readsize;
    CStunMessageReader reader;
    StunClientResults results;
    
    hr = clientlogic.Initialize(config);
    if (FAILED(hr)) {
        Logging::LogMsg(LL_ALWAYS, "clientlogic.Initialize failed (hr == %x)",
                hr);
        Chk(hr);
    }
    
    while (true) {

        stunsocket.Close();
        hr = stunsocket.TCPInit(socketconfig.addrLocal, RolePP, true);
        if (FAILED(hr)) {
            Logging::LogMsg(LL_ALWAYS,
                    "Unable to create local socket for TCP connection (hr == %x)",
                    hr);
            Chk(hr);
        }
        
        hrRet = clientlogic.GetNextMessage(spMsg, &addrDest,
                ::GetMillisecondCounter());
        
        if (hrRet == E_STUNCLIENT_RESULTS_READY) {
            // clean exit
            break;
        }

        // we should never get a "still waiting" return with TCP, because config.timeout is 0
        ASSERT(hrRet != E_STUNCLIENT_STILL_WAITING);
        
        if (FAILED(hrRet)) {
            Chk(hrRet);
        }
        
        // connect to server
        sock = stunsocket.GetSocketHandle();
        
        ret = ::connect(sock, addrDest.GetSockAddr(),
                addrDest.GetSockAddrLength());
        
        if (ret == -1) {
            hrResult = ERRNOHR;
            Logging::LogMsg(LL_ALWAYS, "Can't connect to server (hr == %x)",
                    hrResult);
            Chk(hrResult);
        }
        
        Logging::LogMsg(LL_DEBUG, "Connected to server");
        
        bytes_to_send = (int) (spMsg->GetSize());
        
        bytes_sent = 0;
        pData = spMsg->GetData();
        while (bytes_sent < bytes_to_send) {
            ret = ::send(sock, pData + bytes_sent, bytes_to_send - bytes_sent,
                    0);
            if (ret < 0) {
                hrResult = ERRNOHR;
                Logging::LogMsg(LL_ALWAYS, "Send failed (hr == %x)", hrResult);
                Chk(hrResult);
            }
            bytes_sent += ret;
        }
        
        Logging::LogMsg(LL_DEBUG, "Request sent - waiting for response");
        
        // consume the response
        reader.Reset();
        reader.GetStream().Attach(spMsgReader, true);
        pData = spMsg->GetData();
        bytes_recv = 0;
        max_bytes_recv = spMsg->GetAllocatedSize();
        remaining = max_bytes_recv;
        
        while (remaining > 0) {
            readsize = reader.HowManyBytesNeeded();
            
            if (readsize == 0) {
                break;
            }
            
            if (readsize > remaining) {
                // technically an error, but the client logic will figure it out
                ASSERT(false);
                break;
            }
            
            ret = ::recv(sock, pData + bytes_recv, readsize, 0);
            if (ret == 0) {
                // server cut us off before we got all the bytes we thought we were supposed to get?
                ASSERT(false);
                break;
            }
            if (ret < 0) {
                hrResult = ERRNOHR;
                Logging::LogMsg(LL_ALWAYS, "Recv failed (hr == %x)", hrResult);
                Chk(hrResult);
            }
            
            reader.AddBytes(pData + bytes_recv, ret);
            bytes_recv += ret;
            remaining = max_bytes_recv - bytes_recv;
            spMsg->SetSize(bytes_recv);
        }
        
        // now feed the response into the client logic
        stunsocket.UpdateAddresses();
        addrLocal = stunsocket.GetLocalAddress();
        clientlogic.ProcessResponse(spMsg, addrDest, addrLocal);
    }
    
    stunsocket.Close();

    results.Init();
    clientlogic.GetResults(&results);
    ::DumpResults(config, results);
    
    Cleanup: return;
    
}

void registerToServerRequest(StunClientResults &results,
        CSocketAddress &addrDest, int sock) {
    simple_sockaddr appAddr;
    memcpy(&appAddr, addrDest.GetSockAddr(), sizeof(simple_sockaddr));
    int port = ntohs(appAddr.addr4.sin_port) + 1;
    appAddr.addr4.sin_port = htons(port);

    char szBuffer[100];
    int buffersize = 100;

    results.addrMapped.OnlyIpToStringBuffer(szBuffer, buffersize);

    RegisterRequest *request = new RegisterRequest(szBuffer,
            results.addrMapped.GetPort());
    int ret = ::sendto(sock, (void*) request->getData(), request->getDataSize(),
            0, &appAddr.addr, sizeof(appAddr.addr4));
    if (ret > 0) {
        printf("registerToServerRequest\r\n");
    }
}

void sendReadyToP2pServerRequest(simple_sockaddr &p2pAddr, int sock) {
    char buff[30];
    size_t size = 10;
    char head1[10] = "request";
    char head2[10] = "status";
    char head3[10] = "ready";
    memcpy((void*) buff, head1, size);
    memcpy(&buff[10], head2, 10);
    memcpy(&buff[20], head3, 10);
    p2pAddr.addr4.sin_family = AF_INET;
    int ret = ::sendto(sock, (void*) buff, 30, 0, &p2pAddr.addr,
            sizeof(p2pAddr.addr4));
    if (ret > 0) {
        char stringIp[50];
        ::inet_ntop(AF_INET, &(p2pAddr.addr4.sin_addr), stringIp, 50);
        printf("sendReadyToP2pServerRequest :ip = %s port = %d   \r\n",
                stringIp, ntohs(p2pAddr.addr4.sin_port));
    }
}

void sendToServerToDoConnectRequest(simple_sockaddr &appAddr,
        simple_sockaddr &p2pAddr, int sock) {
    char buff[10];
    memcpy(buff, &REQUEST_SERVER_TODO_CONNECT_P2P, 4);
    memcpy(&buff[4], &p2pAddr.addr4.sin_addr.s_addr, 4);
    uint16_t port = ntohs(p2pAddr.addr4.sin_port);
    memcpy(&buff[8], &port, 2);
    int ret = ::sendto(sock, (void*) buff, 30, 0, &appAddr.addr,
            sizeof(appAddr.addr4));
    if (ret > 0) {
        printf("sendToServerToDoConnectRequest\r\n");
    }
}

void fetchP2pServerRequest(StunClientResults &results, CSocketAddress &addrDest,
        int sock) {
    simple_sockaddr appAddr;
    memcpy(&appAddr, addrDest.GetSockAddr(), sizeof(simple_sockaddr));
    int port = ntohs(appAddr.addr4.sin_port) + 1;
    appAddr.addr4.sin_port = htons(port);

    FetchP2pServerRequest *request = new FetchP2pServerRequest();
    int ret = ::sendto(sock, (void*) request->getData(), request->getDataSize(),
            0, &appAddr.addr, sizeof(appAddr.addr4));
    if (ret > 0) {
        printf("do fetchP2pServerRequest\r\n");
    }
}
//2.索取可分享的P2p终端
// 3 获取服务器返回数据，如果服务返回数据为空 ，则重新请求
void doFetchP2pServer(StunClientResults &results, CSocketAddress &addrDest,
        int sock, StunClientLogicConfig& config,
        std::vector<simple_sockaddr> &p2pServer) {
    int tryCount = 0;
    DOFETCT: fetchP2pServerRequest(results, addrDest, sock);

    fd_set set;
    timeval tv = { };

    FD_ZERO(&set);
    FD_SET(sock, &set);
    tv.tv_usec = 3000000; // 3s
    tv.tv_sec = config.timeoutSeconds;

    int ret = select(sock + 1, &set, NULL, NULL, &tv);
    uint8_t *buff = new uint8_t[8192];
    if (ret > 0) {
        simple_sockaddr remoteaddr;
        socklen_t addrlength;
        addrlength = sizeof(remoteaddr);
        ret = ::recvfrom(sock, buff, 8192, 0, (sockaddr*) &remoteaddr,
                &addrlength);
        if (ret > 0) {
            if (ret >= 30) {
                char head1[10];
                char head2[10];
                char head3[10];
                memcpy(head1, buff, 10);
                memcpy(head2, &buff[10], 10);
                memcpy(head3, &buff[20], 10);
                if (strcmp(head1, "respond") == 0 && strcmp(head2, "data") == 0
                        && strcmp(head3, "p2pList") == 0) {
                    int lastlen = ret - 30;
                    if (lastlen >= 4) {
                        int len = 0;
                        memcpy(&len, &buff[30], 4);
                        if (len > 0) {
                            printf("we have fetch the p2p server list:\r\n");
                            for (int i = 0; i < len; i++) {
                                simple_sockaddr addrServer;
                                addrServer.addr4.sin_family = AF_INET;
                                memcpy(
                                        (void*) &addrServer.addr4.sin_addr.s_addr,
                                        &buff[34 + i * 6], 4);
                                uint16_t port = 0;
                                memcpy((void*) &port,
                                        (void*) &buff[34 + i * 6 + 4], 2);
                                addrServer.addr4.sin_port = htons(port);
                                p2pServer.push_back(addrServer);
                                char stringIp[50];
                                ::inet_ntop(AF_INET,
                                        &(addrServer.addr4.sin_addr), stringIp,
                                        50);
                                printf("server ip %s port %d \r\n", stringIp,
                                        ntohs(addrServer.addr4.sin_port));
                            }
                        }
                    }
                }
            }
        }
    }
    if (p2pServer.size() == 0) {
        tryCount++;
        if (tryCount < 10) {
            usleep(1000 * 500);
            goto DOFETCT;
        }
    }
}
typedef struct thread_struct {
        timeval tv;
        int sock;
} thread_struct_def;
typedef struct send_thread_struct {
        SharedFileBuffer *fileReadBuff;
        int *pos;
        int sock;
        simple_sockaddr remoteaddr;
} send_thread_struct_def;
void sendSectorData(SharedFileBuffer *fileReadBuffer, int sendSectorPosition,
        int socket, simple_sockaddr remoteaddr) {
    char *respondBuff = new char[38 + 4096];
    char head1[10], head2[10], head3[10];
    strcpy(head1, "respond");
    strcpy(head2, "data");
    strcpy(head3, "sector");
    memcpy(respondBuff, head1, 10);
    memcpy(&respondBuff[10], head2, 10);
    memcpy(&respondBuff[20], head3, 10);
    printf("1 \r\n");
    int len = fileReadBuffer->getAt(sendSectorPosition)->getDataSize();
    memcpy(&respondBuff[30], &len, 4);
    printf("2 \r\n");
    int index = fileReadBuffer->getAt(sendSectorPosition)->getIndex();
    memcpy(&respondBuff[34], &index, 4);
    printf("3 \r\n");
    memcpy(&respondBuff[38],
            fileReadBuffer->getAt(sendSectorPosition)->getData(), len);
    printf("4 \r\n");
    int ret = ::sendto(socket, (void*) respondBuff, len + 38, 0,
            &remoteaddr.addr, sizeof(remoteaddr.addr4));
    delete[] respondBuff;
    if (ret > 0) {
        char stringIp[50];
        ::inet_ntop(AF_INET, &(remoteaddr.addr4.sin_addr), stringIp, 50);
        printf(
                "The p2p client has send file secotor %d to: ip: %s port %d \r\n",
                index, stringIp, ntohs(remoteaddr.addr4.sin_port));
    }
}
void* sendThreadFunction(void* pThis) {
    send_thread_struct *pStruct = (send_thread_struct*) pThis;
    while (1) {
        int position = *pStruct->pos;
        if (position < pStruct->fileReadBuff->getBlockSize()) {
            sendSectorData(pStruct->fileReadBuff, position, pStruct->sock,
                    pStruct->remoteaddr);
            usleep(50 * 1000);
        }
        else {
            break;
        }
    }
    return NULL;
}
void initSendThread(send_thread_struct_def *thread_struct) {

    printf("::pthread_create(&_pthread, NULL, initSendThread, socket)\r\n");
    pthread_t _pthread;
    int err = ::pthread_create(&_pthread, NULL, sendThreadFunction,
            thread_struct);
}
void sendSectorApk(int sock, int index, simple_sockaddr &remoteaddr) {
    char head1[10], head2[10], head3[10];
    strcpy(head1, "respond");
    strcpy(head2, "apk");
    strcpy(head3, "sector");
    char respondBuff[34];
    memcpy(respondBuff, head1, 10);
    memcpy(&respondBuff[10], head2, 10);
    memcpy(&respondBuff[20], head3, 10);
    memcpy(&respondBuff[30], &index, 4);
    int ret = ::sendto(sock, (void*) respondBuff, 34, 0, &remoteaddr.addr,
            sizeof(remoteaddr.addr4));
    if (ret > 0) {
        char stringIp[50];
        ::inet_ntop(AF_INET, &(remoteaddr.addr4.sin_addr), stringIp, 50);
        printf(
                "The p2p client has send file secotor %d  apk to: ip: %s port %d \r\n",
                index, stringIp, ntohs(remoteaddr.addr4.sin_port));
    }
}
void sendRequestFileInfo(int sock, char *localFileName,
        simple_sockaddr &remoteaddr) {
    char respondBuff[80];
    char head1[10], head2[10], head3[10];
    strcpy(head1, "request");
    strcpy(head2, "data");
    strcpy(head3, "fileName");
    memcpy(respondBuff, head1, 10);
    memcpy(&respondBuff[10], head2, 10);
    memcpy(&respondBuff[20], head3, 10);
    char fileName[50];
    memset(fileName, 0, 50);
    printf("please input the shared file name:");
    scanf("%s", fileName);
    printf("file name:%s", fileName);
    strcpy(localFileName, fileName);
    memcpy(&respondBuff[30], fileName, 50);
    int ret = ::sendto(sock, (void*) respondBuff, 80, 0, &remoteaddr.addr,
            sizeof(remoteaddr.addr4));
    char stringIp[50];
    ::inet_ntop(AF_INET, &(remoteaddr.addr4.sin_addr), stringIp, 50);
    if (ret > 0) {
        printf(
                "The p2p client has send request file name:: ip: %s port %d \r\n",
                stringIp, ntohs(remoteaddr.addr4.sin_port));
    }

}
void sendRespondFileInfo(int size, int sock, simple_sockaddr &remoteaddr) {
    char head1[10], head2[10], head3[10];
    char respondBuff[34];
    strcpy(head1, "respond");
    strcpy(head2, "data");
    strcpy(head3, "fileDesc");
    memcpy(respondBuff, head1, 10);
    memcpy(&respondBuff[10], head2, 10);
    memcpy(&respondBuff[20], head3, 10);
    memcpy(&respondBuff[30], &size, 4);
    int ret = ::sendto(sock, (void*) respondBuff, 34, 0, &remoteaddr.addr,
            sizeof(remoteaddr.addr4));
    char stringIp[50];
    ::inet_ntop(AF_INET, &(remoteaddr.addr4.sin_addr), stringIp, 50);
    if (ret > 0) {
        printf("The p2p server has send file descrip to: ip: %s port %d \r\n",
                stringIp, ntohs(remoteaddr.addr4.sin_port));
    }
}
void* receiveThreadFunction(void* pThis) {
    thread_struct_def *pStruct = (thread_struct_def*) pThis;
    fd_set set;
    FD_ZERO(&set);
    FD_SET(pStruct->sock, &set);

    int ret = select(pStruct->sock + 1, &set, NULL, NULL, &pStruct->tv);

    uint8_t *buff = new uint8_t[8192];
    simple_sockaddr remoteaddr;
    socklen_t addrlength;
    addrlength = sizeof(remoteaddr);

    SharedFileBuffer *fileReadBuff = NULL;
    SharedFileBuffer *fileWriteBuff = NULL;
    int fileBlockSize = 0;
    int currentfileBlockPosition = -1;
    int currentWritefileBlockPosition = 0;
    char localFileName[255];
    send_thread_struct_def *send_thread_struct = NULL;
    bool mConnection = false;
    while (1) {
        int ret = ::recvfrom(pStruct->sock, buff, 8192, 0,
                (sockaddr*) &remoteaddr, &addrlength);
        if (ret > 0) {
            if (ret >= 30) {
                char head1[10];
                char head2[10];
                char head3[10];
                memcpy(head1, buff, 10);
                memcpy(head2, &buff[10], 10);
                memcpy(head3, &buff[20], 10);
                if (strcmp(head1, "request") == 0 && strcmp(head2, "data") == 0
                        && strcmp(head3, "doConnect") == 0) {
                    simple_sockaddr requestaddr;
                    requestaddr.addr4.sin_family = AF_INET;
                    memcpy(&requestaddr.addr4.sin_addr.s_addr, &buff[30], 4);
                    uint16_t port = 0;
                    memcpy(&port, &buff[34], 2);
                    requestaddr.addr4.sin_port = htons(port);
                    char respondBuff[30];
                    strcpy(head1, "respond");
                    strcpy(head2, "status");
                    strcpy(head3, "ready");
                    memcpy(respondBuff, head1, 10);
                    memcpy(&respondBuff[10], head2, 10);
                    memcpy(&respondBuff[20], head3, 10);
                    printf(
                            "The p2p client has received request: request data doConnect\r\n");
                    ret = ::sendto(pStruct->sock, (void*) respondBuff, 30, 0,
                            &requestaddr.addr, sizeof(requestaddr.addr4));
                    char stringIp[50];
                    ::inet_ntop(AF_INET, &(requestaddr.addr4.sin_addr),
                            stringIp, 50);
                    if (ret > 0) {
                        printf(
                                "The p2p client has send respond: respond status ready: ip: %s port %d \r\n",
                                stringIp, ntohs(requestaddr.addr4.sin_port));
                    }
                    else {
                        printf(
                                "The p2p client sending respond failed: respond status ready: ip: %s port %d \r\n",
                                stringIp, ntohs(requestaddr.addr4.sin_port));
                    }
                }
                else if (strcmp(head1, "respond") == 0
                        && strcmp(head2, "status") == 0
                        && strcmp(head3, "ready") == 0) {
                    if (mConnection == false) {
                        printf("The p2p connect have been created!\r\n");
                        mConnection = true;
                        sendRequestFileInfo(pStruct->sock,
                                (char*) localFileName, remoteaddr);
                    }

                }
                else if (strcmp(head1, "request") == 0
                        && strcmp(head2, "data") == 0
                        && strcmp(head3, "fileName") == 0) {
                    char fileName[50];
                    memset(fileName, 0, 50);
                    memcpy(fileName, &buff[30], 50);
                    printf("The p2p server receive share file request %s\r\n",
                            fileName);

                    if (fileReadBuff == NULL) {
                        fileReadBuff = new SharedFileBuffer(fileName);
                        fileReadBuff->loadFileToBuffer();
                    }
                    if (fileReadBuff != NULL
                            && fileReadBuff->getBlockSize() > 0) {
                        sendRespondFileInfo(fileReadBuff->getBlockSize(),
                                pStruct->sock, remoteaddr);
                    }
                }
                else if (strcmp(head1, "respond") == 0
                        && strcmp(head2, "data") == 0
                        && strcmp(head3, "fileDesc") == 0) {
                    memcpy(&fileBlockSize, &buff[30], 4);
                    printf("The p2p  client receive  file descrip  block size = %d\r\n",
                            fileBlockSize);
                    char respondBuff[30];
                    strcpy(head1, "respond");
                    strcpy(head2, "apk");
                    strcpy(head3, "fileDesc");
                    memcpy(respondBuff, head1, 10);
                    memcpy(&respondBuff[10], head2, 10);
                    memcpy(&respondBuff[20], head3, 10);
                    ret = ::sendto(pStruct->sock, (void*) respondBuff, 30, 0,
                            &remoteaddr.addr, sizeof(remoteaddr.addr4));

                    if (ret > 0) {
                        char stringIp[50];
                        ::inet_ntop(AF_INET, &(remoteaddr.addr4.sin_addr),
                                stringIp, 50);
                        printf(
                                "The p2p client has send file descrip apk to: ip: %s port %d \r\n",
                                stringIp, ntohs(remoteaddr.addr4.sin_port));
                    }
                }
                else if (strcmp(head1, "respond") == 0
                        && strcmp(head2, "apk") == 0
                        && strcmp(head3, "fileDesc") == 0) {

                    bool bFirstRun = false;
                    if (currentfileBlockPosition == -1) {
                        currentfileBlockPosition = 0;
                        bFirstRun = true;
                    }
                    if (bFirstRun && fileReadBuff->getBlockSize() > 0
                            && currentfileBlockPosition
                                    < fileReadBuff->getBlockSize()) {
                        printf(
                                "The p2p  server receive  file descrip apk and start send file\r\n");
                        send_thread_struct = new send_thread_struct_def;
                        send_thread_struct->fileReadBuff = fileReadBuff;
                        send_thread_struct->pos = &currentfileBlockPosition; // 5s
                        memcpy((void*) &(send_thread_struct->remoteaddr.addr),
                                &(remoteaddr.addr), sizeof(remoteaddr.addr4));
                        send_thread_struct->sock = pStruct->sock;
                        initSendThread(send_thread_struct);
                    }
                }
                else if (strcmp(head1, "respond") == 0
                        && strcmp(head2, "data") == 0
                        && strcmp(head3, "sector") == 0) {

                    int len = 0;
                    int index = 0;
                    memcpy(&len, &buff[30], 4);
                    memcpy(&index, &buff[34], 4);
                    uint8_t *fileDataBuffer = new uint8_t[len];
                    printf(
                            "The p2p  client receive  file sector  = %d  len = %d\r\n",
                            index, len);
                    memcpy(fileDataBuffer, &buff[38], len);
                    if (fileWriteBuff == NULL) {
                        fileWriteBuff = new SharedFileBuffer(localFileName, 1);
                    }
                    if (currentWritefileBlockPosition == index) {
                        fileWriteBuff->writeData(fileDataBuffer, len);
                        currentWritefileBlockPosition++;
                    }
                    sendSectorApk(pStruct->sock, index, remoteaddr);
                    if (index == (fileBlockSize - 1)) {
                        printf("receive complete!\r\n");
                        delete fileWriteBuff;
                    }
                }
                else if (strcmp(head1, "respond") == 0
                        && strcmp(head2, "apk") == 0
                        && strcmp(head3, "sector") == 0) {

                    printf("The p2p  server receive  file sector apk \r\n");
                    int index = 0;
                    memcpy(&index, &buff[30], 4);
                    int currentIndex = currentfileBlockPosition;
                    if (currentIndex == index) {
                        currentfileBlockPosition++;
                    }

                }
                else if (strcmp(head1, "request") == 0
                        && strcmp(head2, "status") == 0
                        && strcmp(head3, "ready") == 0) {
                    printf(
                            "The p2p server has received request: request status ready\r\n");
                    char respondBuff[30];
                    strcpy(head1, "respond");
                    strcpy(head2, "status");
                    strcpy(head3, "ready");
                    memcpy(respondBuff, head1, 10);
                    memcpy(&respondBuff[10], head2, 10);
                    memcpy(&respondBuff[20], head3, 10);
                    ret = ::sendto(pStruct->sock, (void*) respondBuff, 30, 0,
                            &remoteaddr.addr, sizeof(remoteaddr.addr4));
                    char stringIp[50];
                    ::inet_ntop(AF_INET, &(remoteaddr.addr4.sin_addr), stringIp,
                            50);
                    if (ret > 0) {
                        printf(
                                "The p2p client has send respond: respond status ready: ip: %s port %d \r\n",
                                stringIp, ntohs(remoteaddr.addr4.sin_port));
                    }
                    else {
                        printf(
                                "The p2p client sending respond failed: respond status ready: ip: %s port %d \r\n",
                                stringIp, ntohs(remoteaddr.addr4.sin_port));
                    }
                }
                else {
                    printf(
                            "the client thread has  received data 30 ,but is wrong!\r\n");
                }

            }
            else {
                printf(
                        "the client thread has  received data ,but lower than 30!\r\n");
            }
        }
        else {
            printf("the client thread has not received data!\r\n");
        }

    }
    return NULL;
}

void initReceiveThread(int sock, StunClientLogicConfig& config) {
    thread_struct_def *thread_struct = new thread_struct_def[1];

    thread_struct->tv = {};
    thread_struct->tv.tv_usec = 5000000; // 5s
    thread_struct->tv.tv_sec = config.timeoutSeconds;
    thread_struct->sock = sock;
    printf(
            "::pthread_create(&_pthread, NULL, receiveThreadFunction, socket)\r\n");
    pthread_t _pthread;
    int err = ::pthread_create(&_pthread, NULL, receiveThreadFunction,
            thread_struct);
}

void WaitForAppExitSignal() {
    while (true) {
        sigset_t sigs;
        sigemptyset(&sigs);
        sigaddset(&sigs, SIGINT);
        sigaddset(&sigs, SIGTERM);
        int sig = 0;

        int ret = sigwait(&sigs, &sig);
        if ((sig == SIGINT) || (sig == SIGTERM)) {
            break;
        }
    }
}
HRESULT UdpClientLoop(StunClientLogicConfig& config,
        const ClientSocketConfig& socketconfig) {
    HRESULT hr = S_OK;
    CRefCountedStunSocket spStunSocket;
    CStunSocket stunSocket;

    CRefCountedBuffer spMsg(new CBuffer(MAX_STUN_MESSAGE_SIZE));
    int sock = -1;
    CSocketAddress addrDest;   // who we send to
    CSocketAddress addrRemote; // who we
    CSocketAddress addrLocal;
    int ret;
    fd_set set;
    timeval tv = { };
    std::string strAddr;
    std::string strAddrLocal;
    StunClientResults results;

    CStunClientLogic clientlogic;

    std::vector<simple_sockaddr> p2pServer;

    hr = clientlogic.Initialize(config);

    if (FAILED(hr)) {
        Logging::LogMsg(LL_ALWAYS, "Unable to initialize client: (error = x%x)",
                hr);
        Chk(hr);
    }

    hr = stunSocket.UDPInit(socketconfig.addrLocal, RolePP);
    if (FAILED(hr)) {
        Logging::LogMsg(LL_ALWAYS,
                "Unable to create local socket: (error = x%x)", hr);
        Chk(hr);
    }

    stunSocket.EnablePktInfoOption(true);

    sock = stunSocket.GetSocketHandle();

    // let's get a loop going!

    while (true) {
        HRESULT hrRet;
        spMsg->SetSize(0);
        hrRet = clientlogic.GetNextMessage(spMsg, &addrDest,
                GetMillisecondCounter());

        if (SUCCEEDED(hrRet)) {
            addrDest.ToString(&strAddr);
            ASSERT(spMsg->GetSize() > 0);

            if (Logging::GetLogLevel() >= LL_DEBUG) {
                std::string strAddr;
                addrDest.ToString(&strAddr);
                Logging::LogMsg(LL_DEBUG, "Sending message to %s",
                        strAddr.c_str());
            }
            
            ret = ::sendto(sock, spMsg->GetData(), spMsg->GetSize(), 0,
                    addrDest.GetSockAddr(), addrDest.GetSockAddrLength());

            if (ret <= 0) {
                Logging::LogMsg(LL_DEBUG, "ERROR.  sendto failed (errno = %d)",
                        errno);
            }
            // there's not much we can do if "sendto" fails except time out and try again
        }
        else if (hrRet == E_STUNCLIENT_STILL_WAITING) {
            Logging::LogMsg(LL_DEBUG, "Continuing to wait for response...");
        }
        else if (hrRet == E_STUNCLIENT_RESULTS_READY) {
            break;
        }
        else {
            Logging::LogMsg(LL_DEBUG, "Fatal error (hr == %x)", hrRet);
            Chk(hrRet);
        }

        // now wait for a response
        spMsg->SetSize(0);
        FD_ZERO(&set);
        FD_SET(sock, &set);
        tv.tv_usec = 500000; // half-second
        tv.tv_sec = config.timeoutSeconds;

        ret = select(sock + 1, &set, NULL, NULL, &tv);
        if (ret > 0) {
            ret = ::recvfromex(sock, spMsg->GetData(),
                    spMsg->GetAllocatedSize(), MSG_DONTWAIT, &addrRemote,
                    &addrLocal);
            if (ret > 0) {
                addrRemote.ToString(&strAddr);
                addrLocal.ToString(&strAddrLocal);
                Logging::LogMsg(LL_DEBUG,
                        "Got response (%d bytes) from %s on interface %s", ret,
                        strAddr.c_str(), strAddrLocal.c_str());
                spMsg->SetSize(ret);
                clientlogic.ProcessResponse(spMsg, addrRemote, addrLocal);
            }
        }
    }

    results.Init();
    clientlogic.GetResults(&results);
    DumpResults(config, results);

    registerToServerRequest(results, addrDest, sock); //1.向服务器注册本地分享能力

    doFetchP2pServer(results, addrDest, sock, config, p2pServer);
    if (p2pServer.size() > 0) {
        printf("There are some p2p to share!\r\n");
        simple_sockaddr &p2pServerAddr = p2pServer.front();
        char stringIp[50];
        ::inet_ntop(AF_INET, &(p2pServerAddr.addr4.sin_addr), stringIp, 50);
        printf("p2pServer is %s\r\n", stringIp);
    }
    else {
        printf("There are no p2p to share!\r\n");
    }
    //3.本地P2p终端 准备好 ready -->远端p2p request ready
    if (p2pServer.size() > 0) {
        simple_sockaddr &p2pServerAddr = p2pServer.front();
        sendReadyToP2pServerRequest(p2pServerAddr, sock);
        simple_sockaddr appServer;
        memcpy(&appServer, addrDest.GetSockAddr(), sizeof(simple_sockaddr));
        int port = ntohs(appServer.addr4.sin_port) + 1;
        appServer.addr4.sin_port = htons(port);
        //4.请求链接P2p终端 ->server ->远端p2p
        //5。服務器到遠端p2p-> request ->doConnect
        sendToServerToDoConnectRequest(appServer, p2pServerAddr, sock);

        //6.远端P2p终端->ready ->本地
        //7.本地发送请求文件名称 ->远端P2p终端
        //8.远端P2p终端 ->会送包
        //9.本地接收包buff
        //10.合成文件

        initReceiveThread(sock, config);
        // initSendReadyToP2pThread(sock, p2pServerAddr);
        WaitForAppExitSignal();
    }

    Cleanup: return hr;
}

int main(int argc, char** argv) {
    CCmdLineParser cmdline;
    ClientCmdLineArgs args;
    StunClientLogicConfig config;
    ClientSocketConfig socketconfig;
    bool fError = false;
    uint32_t loglevel = LL_ALWAYS;

#ifdef DEBUG
    loglevel = LL_DEBUG;
#endif
    Logging::SetLogLevel(loglevel);

    cmdline.AddNonOption(&args.strRemoteServer);
    cmdline.AddNonOption(&args.strRemotePort);
    cmdline.AddOption("localaddr", required_argument, &args.strLocalAddr);
    cmdline.AddOption("localport", required_argument, &args.strLocalPort);
    cmdline.AddOption("mode", required_argument, &args.strMode);
    cmdline.AddOption("family", required_argument, &args.strFamily);
    cmdline.AddOption("protocol", required_argument, &args.strProtocol);
    cmdline.AddOption("verbosity", required_argument, &args.strVerbosity);
    cmdline.AddOption("help", no_argument, &args.strHelp);

    if (argc <= 1) {
        PrintUsage(true);
        return -1;
    }

    cmdline.ParseCommandLine(argc, argv, 1, &fError);

    if (args.strHelp.length() > 0) {
        PrintUsage(false);
        return -2;
    }

    if (fError) {
        PrintUsage(true);
        return -3;
    }

    if (args.strVerbosity.length() > 0) {
        int level = atoi(args.strVerbosity.c_str());
        if (level >= 0) {
            Logging::SetLogLevel(level);
        }
    }

    if (FAILED(CreateConfigFromCommandLine(args, &config, &socketconfig))) {
        Logging::LogMsg(LL_ALWAYS, "Can't start client");
        PrintUsage(true);
        return -4;
    }

    DumpConfig(config, socketconfig);

    if (socketconfig.socktype == SOCK_STREAM) {
        TcpClientLoop(config, socketconfig);
    }
    else {
        UdpClientLoop(config, socketconfig);
    }

    return 0;
}
