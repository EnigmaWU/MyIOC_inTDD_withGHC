///////////////////////////////////////////////////////////////////////////////////////////////////
// 🔴 RED-GREEN-REFACTOR: TCP Protocol Implementation (Minimal for TC-1)
// This file implements TCP socket-based service protocol for IOC framework
//
// TODO: Consider creating UT_ServiceTypicalTCP-diffProcess.cxx for real cross-process testing
//       (separate server/client executables) as integration/system test, not unit test.
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "IOC/IOC.h"
#include "_IOC.h"
#include "_IOC_Logging.h"
#include "_IOC_Types.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
// TCP Protocol Private Data Structures
///////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief TCP Message Types (simple protocol)
 */
typedef enum {
    TCP_MSG_EVENT = 1,
    TCP_MSG_COMMAND = 2,
    TCP_MSG_DATA = 3,
    TCP_MSG_SUBSCRIBE = 4,
    TCP_MSG_UNSUBSCRIBE = 5,
} TCPMessageType_T;

/**
 * @brief TCP Message Header (network protocol framing)
 */
typedef struct {
    uint32_t MsgType;   // TCPMessageType_T
    uint32_t DataSize;  // Size of message payload
} __attribute__((packed)) TCPMessageHeader_T;

/**
 * @brief TCP-specific service object
 */
typedef struct {
    _IOC_ServiceObject_pT pSrvObj;
    int ListenSocketFd;  // Listening socket file descriptor
    uint16_t Port;       // Bound port number
} _IOC_ProtoTCPServiceObject_T, *_IOC_ProtoTCPServiceObject_pT;

/**
 * @brief TCP-specific link object
 */
typedef struct {
    _IOC_LinkObject_pT pOwnerLinkObj;
    int SocketFd;  // Connected socket file descriptor
    pthread_mutex_t Mutex;

    // Event subscription storage
    IOC_SubEvtArgs_T SubEvtArgs;

    // Background receiver thread
    pthread_t RecvThread;
    int RecvThreadRunning;

    // Peer subscription tracking (for producer side)
    int PeerHasSubscription;
} _IOC_ProtoTCPLinkObject_T, *_IOC_ProtoTCPLinkObject_pT;

///////////////////////////////////////////////////////////////////////////////////////////////////
// TCP Protocol Method Implementations
///////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Send data over TCP socket (helper)
 */
static IOC_Result_T __TCP_sendAll(int SocketFd, const void* pData, size_t Size) {
    size_t TotalSent = 0;
    const uint8_t* pBuffer = (const uint8_t*)pData;

    while (TotalSent < Size) {
        ssize_t Sent = send(SocketFd, pBuffer + TotalSent, Size - TotalSent, 0);
        if (Sent < 0) {
            _IOC_LogError("TCP send failed");
            return IOC_RESULT_BUG;
        }
        TotalSent += Sent;
    }
    return IOC_RESULT_SUCCESS;
}

/**
 * @brief Receive data over TCP socket (helper)
 */
static IOC_Result_T __TCP_recvAll(int SocketFd, void* pData, size_t Size) {
    size_t TotalRecv = 0;
    uint8_t* pBuffer = (uint8_t*)pData;

    while (TotalRecv < Size) {
        ssize_t Recvd = recv(SocketFd, pBuffer + TotalRecv, Size - TotalRecv, 0);
        if (Recvd <= 0) {
            if (Recvd == 0) {
                _IOC_LogError("TCP connection closed");
            } else {
                _IOC_LogError("TCP recv failed");
            }
            return IOC_RESULT_BUG;
        }
        TotalRecv += Recvd;
    }
    return IOC_RESULT_SUCCESS;
}

/**
 * @brief Background thread to receive TCP messages
 */
static void* __TCP_recvThread(void* pArg) {
    _IOC_ProtoTCPLinkObject_pT pTCPLinkObj = (_IOC_ProtoTCPLinkObject_pT)pArg;

    while (pTCPLinkObj->RecvThreadRunning) {
        // Receive message header
        TCPMessageHeader_T Header;
        IOC_Result_T Result = __TCP_recvAll(pTCPLinkObj->SocketFd, &Header, sizeof(Header));
        if (Result != IOC_RESULT_SUCCESS) {
            break;
        }

        // Convert from network byte order
        uint32_t MsgType = ntohl(Header.MsgType);
        uint32_t DataSize = ntohl(Header.DataSize);

        if (MsgType == TCP_MSG_EVENT && DataSize == sizeof(IOC_EvtDesc_T)) {
            // Receive event descriptor
            IOC_EvtDesc_T EvtDesc;
            Result = __TCP_recvAll(pTCPLinkObj->SocketFd, &EvtDesc, sizeof(EvtDesc));
            if (Result != IOC_RESULT_SUCCESS) {
                break;
            }

            // Process event through callback
            pthread_mutex_lock(&pTCPLinkObj->Mutex);
            IOC_CbProcEvt_F CbProcEvt_F = pTCPLinkObj->SubEvtArgs.CbProcEvt_F;
            int EvtNum = pTCPLinkObj->SubEvtArgs.EvtNum;
            IOC_EvtID_T* pEvtIDs = pTCPLinkObj->SubEvtArgs.pEvtIDs;
            void* pCbPrivData = pTCPLinkObj->SubEvtArgs.pCbPrivData;
            pthread_mutex_unlock(&pTCPLinkObj->Mutex);

            if (CbProcEvt_F) {
                // Check if event is subscribed
                for (int i = 0; i < EvtNum; i++) {
                    if (EvtDesc.EvtID == pEvtIDs[i]) {
                        CbProcEvt_F(&EvtDesc, pCbPrivData);
                        break;
                    }
                }
            }
        } else if (MsgType == TCP_MSG_SUBSCRIBE) {
            // Peer subscribed - mark it
            pthread_mutex_lock(&pTCPLinkObj->Mutex);
            pTCPLinkObj->PeerHasSubscription = 1;
            pthread_mutex_unlock(&pTCPLinkObj->Mutex);
        } else if (MsgType == TCP_MSG_UNSUBSCRIBE) {
            // Peer unsubscribed - mark it
            pthread_mutex_lock(&pTCPLinkObj->Mutex);
            pTCPLinkObj->PeerHasSubscription = 0;
            pthread_mutex_unlock(&pTCPLinkObj->Mutex);
        }
    }

    return NULL;
}

/**
 * @brief Online TCP service - bind socket to port
 */
static IOC_Result_T __IOC_onlineService_ofProtoTCP(_IOC_ServiceObject_pT pSrvObj) {
    // 🟢 GREEN: Minimal implementation to pass TC-1

    // Allocate TCP service object
    _IOC_ProtoTCPServiceObject_pT pTCPSrvObj = calloc(1, sizeof(_IOC_ProtoTCPServiceObject_T));
    if (!pTCPSrvObj) {
        return IOC_RESULT_POSIX_ENOMEM;
    }

    pTCPSrvObj->pSrvObj = pSrvObj;
    pTCPSrvObj->Port = pSrvObj->Args.SrvURI.Port;

    // Create TCP listening socket
    int ListenFd = socket(AF_INET, SOCK_STREAM, 0);
    if (ListenFd < 0) {
        free(pTCPSrvObj);
        _IOC_LogError("Failed to create TCP socket");
        return IOC_RESULT_BUG;
    }

    // Set socket options (reuse address)
    int OptVal = 1;
    setsockopt(ListenFd, SOL_SOCKET, SO_REUSEADDR, &OptVal, sizeof(OptVal));

    // Bind to port
    struct sockaddr_in SrvAddr = {0};
    SrvAddr.sin_family = AF_INET;
    SrvAddr.sin_addr.s_addr = INADDR_ANY;
    SrvAddr.sin_port = htons(pTCPSrvObj->Port);

    if (bind(ListenFd, (struct sockaddr*)&SrvAddr, sizeof(SrvAddr)) < 0) {
        close(ListenFd);
        free(pTCPSrvObj);
        _IOC_LogError("Failed to bind TCP socket to port %u", pTCPSrvObj->Port);
        return IOC_RESULT_BUG;
    }

    // Listen for connections
    if (listen(ListenFd, 5) < 0) {
        close(ListenFd);
        free(pTCPSrvObj);
        _IOC_LogError("Failed to listen on TCP socket");
        return IOC_RESULT_BUG;
    }

    pTCPSrvObj->ListenSocketFd = ListenFd;
    pSrvObj->pProtoPriv = pTCPSrvObj;

    _IOC_LogInfo("TCP service onlined on port %u", pTCPSrvObj->Port);
    return IOC_RESULT_SUCCESS;
}

/**
 * @brief Offline TCP service - close listening socket
 */
static IOC_Result_T __IOC_offlineService_ofProtoTCP(_IOC_ServiceObject_pT pSrvObj) {
    _IOC_ProtoTCPServiceObject_pT pTCPSrvObj = (_IOC_ProtoTCPServiceObject_pT)pSrvObj->pProtoPriv;

    if (pTCPSrvObj) {
        if (pTCPSrvObj->ListenSocketFd >= 0) {
            close(pTCPSrvObj->ListenSocketFd);
        }
        free(pTCPSrvObj);
        pSrvObj->pProtoPriv = NULL;
    }

    return IOC_RESULT_SUCCESS;
}

/**
 * @brief Connect to TCP service - establish socket connection
 */
static IOC_Result_T __IOC_connectService_ofProtoTCP(_IOC_LinkObject_pT pLinkObj, const IOC_ConnArgs_pT pConnArgs,
                                                    const IOC_Options_pT pOption) {
    // Create TCP link object
    _IOC_ProtoTCPLinkObject_pT pTCPLinkObj = calloc(1, sizeof(_IOC_ProtoTCPLinkObject_T));
    if (!pTCPLinkObj) {
        return IOC_RESULT_POSIX_ENOMEM;
    }

    pTCPLinkObj->pOwnerLinkObj = pLinkObj;
    pthread_mutex_init(&pTCPLinkObj->Mutex, NULL);

    // Create socket
    int SocketFd = socket(AF_INET, SOCK_STREAM, 0);
    if (SocketFd < 0) {
        free(pTCPLinkObj);
        _IOC_LogError("Failed to create client TCP socket");
        return IOC_RESULT_BUG;
    }

    // Connect to server
    struct sockaddr_in SrvAddr = {0};
    SrvAddr.sin_family = AF_INET;
    SrvAddr.sin_addr.s_addr = inet_addr("127.0.0.1");  // localhost
    SrvAddr.sin_port = htons(pConnArgs->SrvURI.Port);

    if (connect(SocketFd, (struct sockaddr*)&SrvAddr, sizeof(SrvAddr)) < 0) {
        close(SocketFd);
        free(pTCPLinkObj);
        _IOC_LogError("Failed to connect to TCP service on port %u", pConnArgs->SrvURI.Port);
        return IOC_RESULT_NOT_EXIST_SERVICE;
    }

    pTCPLinkObj->SocketFd = SocketFd;
    pLinkObj->pProtoPriv = pTCPLinkObj;

    // Start background receiver thread
    pTCPLinkObj->RecvThreadRunning = 1;
    if (pthread_create(&pTCPLinkObj->RecvThread, NULL, __TCP_recvThread, pTCPLinkObj) != 0) {
        close(SocketFd);
        free(pTCPLinkObj);
        _IOC_LogError("Failed to create TCP receiver thread");
        return IOC_RESULT_BUG;
    }

    _IOC_LogInfo("Connected to TCP service on port %u", pConnArgs->SrvURI.Port);
    return IOC_RESULT_SUCCESS;
}

/**
 * @brief Accept TCP client connection
 */
static IOC_Result_T __IOC_acceptClient_ofProtoTCP(_IOC_ServiceObject_pT pSrvObj, _IOC_LinkObject_pT pLinkObj,
                                                  const IOC_Options_pT pOption) {
    _IOC_ProtoTCPServiceObject_pT pTCPSrvObj = (_IOC_ProtoTCPServiceObject_pT)pSrvObj->pProtoPriv;

    // Create TCP link object for accepted client
    _IOC_ProtoTCPLinkObject_pT pTCPLinkObj = calloc(1, sizeof(_IOC_ProtoTCPLinkObject_T));
    if (!pTCPLinkObj) {
        return IOC_RESULT_POSIX_ENOMEM;
    }

    pTCPLinkObj->pOwnerLinkObj = pLinkObj;
    pthread_mutex_init(&pTCPLinkObj->Mutex, NULL);

    // Accept incoming connection
    struct sockaddr_in CliAddr = {0};
    socklen_t CliLen = sizeof(CliAddr);

    int ClientFd = accept(pTCPSrvObj->ListenSocketFd, (struct sockaddr*)&CliAddr, &CliLen);
    if (ClientFd < 0) {
        free(pTCPLinkObj);
        _IOC_LogError("Failed to accept TCP client connection");
        return IOC_RESULT_TIMEOUT;
    }

    pTCPLinkObj->SocketFd = ClientFd;
    pLinkObj->pProtoPriv = pTCPLinkObj;

    // Start background receiver thread (optional for server side, but useful for bidirectional)
    pTCPLinkObj->RecvThreadRunning = 1;
    if (pthread_create(&pTCPLinkObj->RecvThread, NULL, __TCP_recvThread, pTCPLinkObj) != 0) {
        close(ClientFd);
        free(pTCPLinkObj);
        _IOC_LogError("Failed to create TCP receiver thread for accepted client");
        return IOC_RESULT_BUG;
    }

    _IOC_LogInfo("Accepted TCP client connection");
    return IOC_RESULT_SUCCESS;
}

/**
 * @brief Close TCP link - close socket connection
 */
static IOC_Result_T __IOC_closeLink_ofProtoTCP(_IOC_LinkObject_pT pLinkObj) {
    _IOC_ProtoTCPLinkObject_pT pTCPLinkObj = (_IOC_ProtoTCPLinkObject_pT)pLinkObj->pProtoPriv;

    if (pTCPLinkObj) {
        // Stop receiver thread
        if (pTCPLinkObj->RecvThreadRunning) {
            pTCPLinkObj->RecvThreadRunning = 0;

            // Shutdown and close socket to unblock recv()
            if (pTCPLinkObj->SocketFd >= 0) {
                shutdown(pTCPLinkObj->SocketFd, SHUT_RDWR);  // Shutdown both directions
                close(pTCPLinkObj->SocketFd);
                pTCPLinkObj->SocketFd = -1;
            }

            // Wait for thread to exit
            pthread_join(pTCPLinkObj->RecvThread, NULL);
        }

        // Free subscription data if allocated
        if (pTCPLinkObj->SubEvtArgs.pEvtIDs) {
            free(pTCPLinkObj->SubEvtArgs.pEvtIDs);
        }

        pthread_mutex_destroy(&pTCPLinkObj->Mutex);
        free(pTCPLinkObj);
        pLinkObj->pProtoPriv = NULL;
    }

    return IOC_RESULT_SUCCESS;
}

/**
 * @brief Subscribe to events over TCP
 */
static IOC_Result_T __IOC_subEvt_ofProtoTCP(_IOC_LinkObject_pT pLinkObj, const IOC_SubEvtArgs_pT pSubEvtArgs) {
    _IOC_ProtoTCPLinkObject_pT pTCPLinkObj = (_IOC_ProtoTCPLinkObject_pT)pLinkObj->pProtoPriv;

    pthread_mutex_lock(&pTCPLinkObj->Mutex);

    // Free old subscription if it exists
    if (pTCPLinkObj->SubEvtArgs.pEvtIDs) {
        free(pTCPLinkObj->SubEvtArgs.pEvtIDs);
    }

    // Copy subscription arguments
    memcpy(&pTCPLinkObj->SubEvtArgs, pSubEvtArgs, sizeof(IOC_SubEvtArgs_T));

    // Allocate and copy event IDs
    pTCPLinkObj->SubEvtArgs.pEvtIDs = (IOC_EvtID_T*)malloc(pSubEvtArgs->EvtNum * sizeof(IOC_EvtID_T));
    memcpy(pTCPLinkObj->SubEvtArgs.pEvtIDs, pSubEvtArgs->pEvtIDs, pSubEvtArgs->EvtNum * sizeof(IOC_EvtID_T));

    pthread_mutex_unlock(&pTCPLinkObj->Mutex);

    // Send SUBSCRIBE message to peer
    TCPMessageHeader_T Header;
    Header.MsgType = htonl(TCP_MSG_SUBSCRIBE);
    Header.DataSize = htonl(0);

    return __TCP_sendAll(pTCPLinkObj->SocketFd, &Header, sizeof(Header));
}

/**
 * @brief Unsubscribe from events over TCP
 */
static IOC_Result_T __IOC_unsubEvt_ofProtoTCP(_IOC_LinkObject_pT pLinkObj, const IOC_UnsubEvtArgs_pT pUnsubEvtArgs) {
    _IOC_ProtoTCPLinkObject_pT pTCPLinkObj = (_IOC_ProtoTCPLinkObject_pT)pLinkObj->pProtoPriv;

    pthread_mutex_lock(&pTCPLinkObj->Mutex);

    if (pTCPLinkObj->SubEvtArgs.CbProcEvt_F == pUnsubEvtArgs->CbProcEvt_F &&
        pTCPLinkObj->SubEvtArgs.pCbPrivData == pUnsubEvtArgs->pCbPrivData) {
        free(pTCPLinkObj->SubEvtArgs.pEvtIDs);
        memset(&pTCPLinkObj->SubEvtArgs, 0, sizeof(IOC_SubEvtArgs_T));

        pthread_mutex_unlock(&pTCPLinkObj->Mutex);

        // Send UNSUBSCRIBE message to peer
        TCPMessageHeader_T Header;
        Header.MsgType = htonl(TCP_MSG_UNSUBSCRIBE);
        Header.DataSize = htonl(0);

        return __TCP_sendAll(pTCPLinkObj->SocketFd, &Header, sizeof(Header));
    }

    pthread_mutex_unlock(&pTCPLinkObj->Mutex);
    return IOC_RESULT_NOT_EXIST;
}

/**
 * @brief Post event over TCP
 */
static IOC_Result_T __IOC_postEvt_ofProtoTCP(_IOC_LinkObject_pT pLinkObj, const IOC_EvtDesc_pT pEvtDesc,
                                             const IOC_Options_pT pOption) {
    _IOC_ProtoTCPLinkObject_pT pTCPLinkObj = (_IOC_ProtoTCPLinkObject_pT)pLinkObj->pProtoPriv;

    // Check if peer has active subscription
    pthread_mutex_lock(&pTCPLinkObj->Mutex);
    int PeerHasSubscription = pTCPLinkObj->PeerHasSubscription;
    pthread_mutex_unlock(&pTCPLinkObj->Mutex);

    if (!PeerHasSubscription) {
        return IOC_RESULT_NO_EVENT_CONSUMER;
    }

    // Send event over TCP socket with protocol framing
    TCPMessageHeader_T Header;
    Header.MsgType = htonl(TCP_MSG_EVENT);
    Header.DataSize = htonl(sizeof(IOC_EvtDesc_T));

    // Send header
    IOC_Result_T Result = __TCP_sendAll(pTCPLinkObj->SocketFd, &Header, sizeof(Header));
    if (Result != IOC_RESULT_SUCCESS) {
        return Result;
    }

    // Send event descriptor
    Result = __TCP_sendAll(pTCPLinkObj->SocketFd, pEvtDesc, sizeof(IOC_EvtDesc_T));
    return Result;
}

/**
 * @brief Execute command over TCP
 */
static IOC_Result_T __IOC_execCmd_ofProtoTCP(_IOC_LinkObject_pT pLinkObj, IOC_CmdDesc_pT pCmdDesc,
                                             const IOC_Options_pT pOption) {
    // 🟢 GREEN: Minimal implementation - return NOT_IMPLEMENTED for now
    _IOC_LogError("TCP Command protocol not yet implemented");
    return IOC_RESULT_NOT_IMPLEMENTED;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// TCP Protocol Method Table
///////////////////////////////////////////////////////////////////////////////////////////////////

_IOC_SrvProtoMethods_T _gIOC_SrvProtoTCPMethods = {
    .pProtocol = "tcp",

    .OpOnlineService_F = __IOC_onlineService_ofProtoTCP,
    .OpOfflineService_F = __IOC_offlineService_ofProtoTCP,
    .OpConnectService_F = __IOC_connectService_ofProtoTCP,
    .OpAcceptClient_F = __IOC_acceptClient_ofProtoTCP,
    .OpCloseLink_F = __IOC_closeLink_ofProtoTCP,

    .OpSubEvt_F = __IOC_subEvt_ofProtoTCP,
    .OpUnsubEvt_F = __IOC_unsubEvt_ofProtoTCP,
    .OpPostEvt_F = __IOC_postEvt_ofProtoTCP,
    .OpPullEvt_F = NULL,  // TODO: Implement for polling mode

    .OpExecCmd_F = __IOC_execCmd_ofProtoTCP,  // 🟢 GREEN: Minimal stub
    .OpWaitCmd_F = NULL,
    .OpAckCmd_F = NULL,

    .OpSendData_F = NULL,  // TODO: Implement for TC-9
    .OpRecvData_F = NULL,
};
