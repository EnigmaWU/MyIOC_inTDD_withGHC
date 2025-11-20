///////////////////////////////////////////////////////////////////////////////////////////////////
// 🔴 RED-GREEN-REFACTOR: TCP Protocol Implementation (Minimal for TC-1)
// This file implements TCP socket-based service protocol for IOC framework
//
// TODO: Consider creating UT_ServiceTypicalTCP-diffProcess.cxx for real cross-process testing
//       (separate server/client executables) as integration/system test, not unit test.
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
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
    TCP_MSG_USAGE_NEGOTIATION = 0,  // Client sends usage, server responds with negotiated usage
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

    // Command usage storage (for executor side)
    IOC_CmdUsageArgs_T CmdUsageArgs;

    // Command response waiting (for initiator side)
    pthread_cond_t CmdResponseCond;
    int CmdResponseReady;
    IOC_CmdDesc_T CmdResponse;

    // Data usage storage (for receiver side)
    IOC_DatUsageArgs_T DatUsageArgs;

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
                _IOC_LogInfo("TCP connection closed");
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
        } else if (MsgType == TCP_MSG_COMMAND && DataSize == sizeof(IOC_CmdDesc_T)) {
            // Receive command descriptor
            IOC_CmdDesc_T CmdDesc;
            Result = __TCP_recvAll(pTCPLinkObj->SocketFd, &CmdDesc, sizeof(CmdDesc));
            if (Result != IOC_RESULT_SUCCESS) {
                break;
            }

            // Check if this is a command request or response
            pthread_mutex_lock(&pTCPLinkObj->Mutex);
            IOC_CbExecCmd_F CbExecCmd_F = pTCPLinkObj->CmdUsageArgs.CbExecCmd_F;
            pthread_mutex_unlock(&pTCPLinkObj->Mutex);

            if (CbExecCmd_F) {
                // This is a command REQUEST - we are the executor

                // Check for IN payload (pointer-based)
                void* pInData = NULL;
                if (CmdDesc.InPayload.PtrDataLen > 0) {
                    TCPMessageHeader_T PayloadHeader;
                    Result = __TCP_recvAll(pTCPLinkObj->SocketFd, &PayloadHeader, sizeof(PayloadHeader));
                    if (Result == IOC_RESULT_SUCCESS) {
                        uint32_t PayloadMsgType = ntohl(PayloadHeader.MsgType);
                        uint32_t PayloadDataSize = ntohl(PayloadHeader.DataSize);

                        if (PayloadMsgType == TCP_MSG_DATA && PayloadDataSize == CmdDesc.InPayload.PtrDataLen) {
                            pInData = malloc(PayloadDataSize + 1);
                            if (pInData) {
                                Result = __TCP_recvAll(pTCPLinkObj->SocketFd, pInData, PayloadDataSize);
                                if (Result == IOC_RESULT_SUCCESS) {
                                    CmdDesc.InPayload.pData = pInData;
                                } else {
                                    free(pInData);
                                    pInData = NULL;
                                }
                            }
                        }
                    }
                }

                // Execute command through callback
                pthread_mutex_lock(&pTCPLinkObj->Mutex);
                void* pCbPrivData = pTCPLinkObj->CmdUsageArgs.pCbPrivData;
                pthread_mutex_unlock(&pTCPLinkObj->Mutex);

                IOC_LinkID_T LinkID = pTCPLinkObj->pOwnerLinkObj->ID;
                CbExecCmd_F(LinkID, &CmdDesc, pCbPrivData);

                if (pInData) free(pInData);

                // Send response back (CmdDesc + OUT payload data)
                TCPMessageHeader_T RespHeader;
                RespHeader.MsgType = htonl(TCP_MSG_COMMAND);
                RespHeader.DataSize = htonl(sizeof(IOC_CmdDesc_T));

                __TCP_sendAll(pTCPLinkObj->SocketFd, &RespHeader, sizeof(RespHeader));
                __TCP_sendAll(pTCPLinkObj->SocketFd, &CmdDesc, sizeof(CmdDesc));

                // Send OUT payload data if present (pointer-based only)
                // Embedded data is already sent within IOC_CmdDesc_T
                void* pOutData = IOC_CmdDesc_getOutData(&CmdDesc);
                ULONG_T OutDataLen = IOC_CmdDesc_getOutDataLen(&CmdDesc);

                // Only send separate payload if it's NOT embedded (PtrDataLen > 0)
                if (pOutData && OutDataLen > 0 && CmdDesc.OutPayload.PtrDataLen > 0) {
                    TCPMessageHeader_T PayloadHeader;
                    PayloadHeader.MsgType = htonl(TCP_MSG_DATA);
                    PayloadHeader.DataSize = htonl(OutDataLen);
                    __TCP_sendAll(pTCPLinkObj->SocketFd, &PayloadHeader, sizeof(PayloadHeader));
                    __TCP_sendAll(pTCPLinkObj->SocketFd, pOutData, OutDataLen);
                }
            } else {
                // This is a command RESPONSE - we are the initiator
                // Copy response to local storage
                pthread_mutex_lock(&pTCPLinkObj->Mutex);
                pTCPLinkObj->CmdResponse = CmdDesc;
                pthread_mutex_unlock(&pTCPLinkObj->Mutex);

                // Try to receive OUT payload data (if any)
                // Only if PtrDataLen > 0
                if (CmdDesc.OutPayload.PtrDataLen > 0) {
                    TCPMessageHeader_T PayloadHeader;
                    Result = __TCP_recvAll(pTCPLinkObj->SocketFd, &PayloadHeader, sizeof(PayloadHeader));
                    if (Result == IOC_RESULT_SUCCESS) {
                        uint32_t PayloadMsgType = ntohl(PayloadHeader.MsgType);
                        uint32_t PayloadDataSize = ntohl(PayloadHeader.DataSize);

                        if (PayloadMsgType == TCP_MSG_DATA && PayloadDataSize > 0) {
                            // Allocate buffer for OUT payload data
                            void* pPayloadData = malloc(PayloadDataSize);
                            if (pPayloadData) {
                                Result = __TCP_recvAll(pTCPLinkObj->SocketFd, pPayloadData, PayloadDataSize);
                                if (Result == IOC_RESULT_SUCCESS) {
                                    // Set OUT payload in response
                                    pthread_mutex_lock(&pTCPLinkObj->Mutex);
                                    IOC_CmdDesc_setOutPayload(&pTCPLinkObj->CmdResponse, pPayloadData, PayloadDataSize);
                                    pthread_mutex_unlock(&pTCPLinkObj->Mutex);
                                }
                                free(pPayloadData);
                            }
                        }
                    }
                }

                // Signal the waiting command execution
                pthread_mutex_lock(&pTCPLinkObj->Mutex);
                pTCPLinkObj->CmdResponseReady = 1;
                pthread_cond_signal(&pTCPLinkObj->CmdResponseCond);
                pthread_mutex_unlock(&pTCPLinkObj->Mutex);
            }
        } else if (MsgType == TCP_MSG_DATA) {
            // Receive data payload
            // Allocate buffer for data
            void* pDataBuffer = malloc(DataSize);
            if (!pDataBuffer) {
                _IOC_LogError("Failed to allocate buffer for data reception");
                break;
            }

            Result = __TCP_recvAll(pTCPLinkObj->SocketFd, pDataBuffer, DataSize);
            if (Result != IOC_RESULT_SUCCESS) {
                free(pDataBuffer);
                break;
            }

            // Process data through callback if receiver has one
            pthread_mutex_lock(&pTCPLinkObj->Mutex);
            IOC_CbRecvDat_F CbRecvDat_F = pTCPLinkObj->DatUsageArgs.CbRecvDat_F;
            void* pCbPrivData = pTCPLinkObj->DatUsageArgs.pCbPrivData;
            pthread_mutex_unlock(&pTCPLinkObj->Mutex);

            if (CbRecvDat_F) {
                // Create DatDesc for callback
                IOC_DatDesc_T DatDesc;
                IOC_initDatDesc(&DatDesc);
                DatDesc.Payload.pData = pDataBuffer;
                DatDesc.Payload.PtrDataSize = DataSize;
                DatDesc.Payload.PtrDataLen = DataSize;  // Set the actual received data length

                IOC_LinkID_T LinkID = pTCPLinkObj->pOwnerLinkObj->ID;
                CbRecvDat_F(LinkID, &DatDesc, pCbPrivData);
            }

            // Free the allocated buffer
            free(pDataBuffer);
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
    pthread_cond_init(&pTCPLinkObj->CmdResponseCond, NULL);
    pTCPLinkObj->CmdResponseReady = 0;

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

    // Send usage negotiation message to server
    TCPMessageHeader_T NegotiationHeader = {.MsgType = htonl(TCP_MSG_USAGE_NEGOTIATION),
                                            .DataSize = htonl(sizeof(IOC_LinkUsage_T))};

    if (__TCP_sendAll(SocketFd, &NegotiationHeader, sizeof(NegotiationHeader)) != IOC_RESULT_SUCCESS) {
        close(SocketFd);
        free(pTCPLinkObj);
        _IOC_LogError("Failed to send usage negotiation header");
        return IOC_RESULT_BUG;
    }

    IOC_LinkUsage_T ClientUsage = pLinkObj->Args.Usage;
    if (__TCP_sendAll(SocketFd, &ClientUsage, sizeof(ClientUsage)) != IOC_RESULT_SUCCESS) {
        close(SocketFd);
        free(pTCPLinkObj);
        _IOC_LogError("Failed to send client usage");
        return IOC_RESULT_BUG;
    }

    // Receive negotiated usage from server
    IOC_LinkUsage_T NegotiatedUsage = IOC_LinkUsageUndefined;
    if (__TCP_recvAll(SocketFd, &NegotiatedUsage, sizeof(NegotiatedUsage)) != IOC_RESULT_SUCCESS) {
        close(SocketFd);
        free(pTCPLinkObj);
        _IOC_LogError("Failed to receive negotiated usage from server");
        return IOC_RESULT_BUG;
    }

    // Verify negotiation succeeded
    if (NegotiatedUsage == IOC_LinkUsageUndefined) {
        close(SocketFd);
        free(pTCPLinkObj);
        _IOC_LogError("Usage negotiation failed - incompatible roles");
        return IOC_RESULT_INVALID_PARAM;
    }

    // Copy CmdUsageArgs from connection args if client is CmdExecutor
    if ((pLinkObj->Args.Usage & IOC_LinkUsageCmdExecutor) && pConnArgs->UsageArgs.pCmd) {
        memcpy(&pTCPLinkObj->CmdUsageArgs, pConnArgs->UsageArgs.pCmd, sizeof(IOC_CmdUsageArgs_T));
    }

    // Copy DatUsageArgs from connection args if client is DatReceiver
    if ((pLinkObj->Args.Usage & IOC_LinkUsageDatReceiver) && pConnArgs->UsageArgs.pDat) {
        memcpy(&pTCPLinkObj->DatUsageArgs, pConnArgs->UsageArgs.pDat, sizeof(IOC_DatUsageArgs_T));
    }

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
    pthread_cond_init(&pTCPLinkObj->CmdResponseCond, NULL);
    pTCPLinkObj->CmdResponseReady = 0;

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

    // Receive usage negotiation from client
    TCPMessageHeader_T NegotiationHeader = {0};
    if (__TCP_recvAll(ClientFd, &NegotiationHeader, sizeof(NegotiationHeader)) != IOC_RESULT_SUCCESS) {
        close(ClientFd);
        free(pTCPLinkObj);
        _IOC_LogError("Failed to receive usage negotiation header from client");
        return IOC_RESULT_BUG;
    }

    if (ntohl(NegotiationHeader.MsgType) != TCP_MSG_USAGE_NEGOTIATION) {
        close(ClientFd);
        free(pTCPLinkObj);
        _IOC_LogError("Expected usage negotiation message, got type %u", ntohl(NegotiationHeader.MsgType));
        return IOC_RESULT_BUG;
    }

    IOC_LinkUsage_T ClientUsage = IOC_LinkUsageUndefined;
    if (__TCP_recvAll(ClientFd, &ClientUsage, sizeof(ClientUsage)) != IOC_RESULT_SUCCESS) {
        close(ClientFd);
        free(pTCPLinkObj);
        _IOC_LogError("Failed to receive client usage");
        return IOC_RESULT_BUG;
    }

    // Negotiate server's link role based on client's usage
    IOC_LinkUsage_T ServiceCapabilities = pSrvObj->Args.UsageCapabilites;
    IOC_LinkUsage_T ServiceLinkRole = _IOC_negotiateLinkRole(ServiceCapabilities, ClientUsage);

    if (ServiceLinkRole == IOC_LinkUsageUndefined) {
        // Send failure response to client
        IOC_LinkUsage_T FailureResponse = IOC_LinkUsageUndefined;
        __TCP_sendAll(ClientFd, &FailureResponse, sizeof(FailureResponse));

        close(ClientFd);
        free(pTCPLinkObj);
        _IOC_LogError("Usage negotiation failed: ServiceCap=0x%X, ClientUsage=0x%X incompatible", ServiceCapabilities,
                      ClientUsage);
        return IOC_RESULT_INVALID_PARAM;
    }

    // Send negotiated server usage back to client
    if (__TCP_sendAll(ClientFd, &ServiceLinkRole, sizeof(ServiceLinkRole)) != IOC_RESULT_SUCCESS) {
        close(ClientFd);
        free(pTCPLinkObj);
        _IOC_LogError("Failed to send negotiated usage to client");
        return IOC_RESULT_BUG;
    }

    // Set server link's usage to negotiated role
    pLinkObj->Args.Usage = ServiceLinkRole;

    // Copy CmdUsageArgs from service to link (for command executor functionality)
    if (pSrvObj->Args.UsageArgs.pCmd) {
        memcpy(&pTCPLinkObj->CmdUsageArgs, pSrvObj->Args.UsageArgs.pCmd, sizeof(IOC_CmdUsageArgs_T));
    }

    // Copy DatUsageArgs from service to link (for data receiver functionality)
    if (pSrvObj->Args.UsageArgs.pDat) {
        memcpy(&pTCPLinkObj->DatUsageArgs, pSrvObj->Args.UsageArgs.pDat, sizeof(IOC_DatUsageArgs_T));
    }

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
        pthread_cond_destroy(&pTCPLinkObj->CmdResponseCond);
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
    _IOC_ProtoTCPLinkObject_T* pTCPLinkObj = (_IOC_ProtoTCPLinkObject_T*)pLinkObj->pProtoPriv;
    IOC_Result_T Result = IOC_RESULT_BUG;

    if (!pCmdDesc) {
        return IOC_RESULT_INVALID_PARAM;
    }

    pthread_mutex_lock(&pTCPLinkObj->Mutex);
    int SocketFd = pTCPLinkObj->SocketFd;

    if (SocketFd < 0) {
        pthread_mutex_unlock(&pTCPLinkObj->Mutex);
        return IOC_RESULT_NOT_EXIST_LINK;
    }

    // Clear previous response
    pTCPLinkObj->CmdResponseReady = 0;

    // Send command request
    TCPMessageHeader_T Header;
    Header.MsgType = htonl(TCP_MSG_COMMAND);
    Header.DataSize = htonl(sizeof(IOC_CmdDesc_T));

    pthread_mutex_unlock(&pTCPLinkObj->Mutex);

    Result = __TCP_sendAll(SocketFd, &Header, sizeof(Header));
    if (Result != IOC_RESULT_SUCCESS) {
        return Result;
    }

    Result = __TCP_sendAll(SocketFd, pCmdDesc, sizeof(IOC_CmdDesc_T));
    if (Result != IOC_RESULT_SUCCESS) {
        return Result;
    }

    // Send IN payload data if present (pointer-based only)
    // Embedded data is already sent within IOC_CmdDesc_T
    if (pCmdDesc->InPayload.PtrDataLen > 0 && pCmdDesc->InPayload.pData) {
        TCPMessageHeader_T PayloadHeader;
        PayloadHeader.MsgType = htonl(TCP_MSG_DATA);
        PayloadHeader.DataSize = htonl(pCmdDesc->InPayload.PtrDataLen);

        Result = __TCP_sendAll(SocketFd, &PayloadHeader, sizeof(PayloadHeader));
        if (Result != IOC_RESULT_SUCCESS) return Result;

        Result = __TCP_sendAll(SocketFd, pCmdDesc->InPayload.pData, pCmdDesc->InPayload.PtrDataLen);
        if (Result != IOC_RESULT_SUCCESS) return Result;
    }

    // Wait for response from receiver thread
    pthread_mutex_lock(&pTCPLinkObj->Mutex);

    // Extract round-trip timeout: Priority order - pOption > default network timeout
    // Note: pCmdDesc->TimeoutMs is for EXECUTOR execution time, not for round-trip wait
    ULONG_T roundTripTimeoutMs = 0;

    if (pOption && (pOption->IDs & IOC_OPTID_TIMEOUT)) {
        // Use API-level timeout (highest priority)
        roundTripTimeoutMs = pOption->Payload.TimeoutUS / 1000;  // Convert microseconds to milliseconds
    } else {
        // Default network round-trip timeout (executor timeout + network overhead)
        roundTripTimeoutMs = (pCmdDesc->TimeoutMs > 0) ? (pCmdDesc->TimeoutMs + 1000) : 10000;
        // Add 1s overhead for network, or use 10s default if no executor timeout specified
    }

    int WaitResult = 0;
    if (roundTripTimeoutMs > 0) {
        // Calculate absolute timeout time
        struct timespec AbsTimeout;
        clock_gettime(CLOCK_REALTIME, &AbsTimeout);

        // Convert roundTripTimeoutMs to seconds and nanoseconds
        long TimeoutSec = roundTripTimeoutMs / 1000;
        long TimeoutNsec = (roundTripTimeoutMs % 1000) * 1000000;

        // Add timeout to current time
        AbsTimeout.tv_sec += TimeoutSec;
        AbsTimeout.tv_nsec += TimeoutNsec;

        // Handle nanosecond overflow
        if (AbsTimeout.tv_nsec >= 1000000000) {
            AbsTimeout.tv_sec += 1;
            AbsTimeout.tv_nsec -= 1000000000;
        }

        // Wait with timeout
        while (!pTCPLinkObj->CmdResponseReady && WaitResult == 0) {
            WaitResult = pthread_cond_timedwait(&pTCPLinkObj->CmdResponseCond, &pTCPLinkObj->Mutex, &AbsTimeout);
        }
    } else {
        // No timeout specified, wait indefinitely
        while (!pTCPLinkObj->CmdResponseReady) {
            pthread_cond_wait(&pTCPLinkObj->CmdResponseCond, &pTCPLinkObj->Mutex);
        }
    }

    // Check if timeout occurred
    if (WaitResult == ETIMEDOUT) {
        pthread_mutex_unlock(&pTCPLinkObj->Mutex);
        return IOC_RESULT_TIMEOUT;
    }

    // Copy response back (including OUT payload)
    pCmdDesc->Status = pTCPLinkObj->CmdResponse.Status;
    pCmdDesc->Result = pTCPLinkObj->CmdResponse.Result;

    // Copy OUT payload data if present
    void* pOutData = IOC_CmdDesc_getOutData(&pTCPLinkObj->CmdResponse);
    ULONG_T OutDataLen = IOC_CmdDesc_getOutDataLen(&pTCPLinkObj->CmdResponse);
    if (pOutData && OutDataLen > 0) {
        IOC_CmdDesc_setOutPayload(pCmdDesc, pOutData, OutDataLen);
    }

    pthread_mutex_unlock(&pTCPLinkObj->Mutex);

    return IOC_RESULT_SUCCESS;
}

/**
 * @brief Send data over TCP
 * Implementation for TC-9, TC-10, TC-11
 */
static IOC_Result_T __IOC_sendData_ofProtoTCP(_IOC_LinkObject_pT pLinkObj, const IOC_DatDesc_pT pDatDesc,
                                              const IOC_Options_pT pOption) {
    _IOC_ProtoTCPLinkObject_pT pTCPLinkObj = (_IOC_ProtoTCPLinkObject_pT)pLinkObj->pProtoPriv;
    
    if (!pDatDesc) {
        return IOC_RESULT_INVALID_PARAM;
    }

    pthread_mutex_lock(&pTCPLinkObj->Mutex);
    int SocketFd = pTCPLinkObj->SocketFd;
    pthread_mutex_unlock(&pTCPLinkObj->Mutex);

    if (SocketFd < 0) {
        return IOC_RESULT_NOT_EXIST_LINK;
    }

    // Get data payload
    void* pData;
    ULONG_T DataSize;
    IOC_Result_T Result = IOC_getDatPayload(pDatDesc, &pData, &DataSize);
    if (Result != IOC_RESULT_SUCCESS) {
        return Result;
    }

    // Send data message over TCP socket with protocol framing
    TCPMessageHeader_T Header;
    Header.MsgType = htonl(TCP_MSG_DATA);
    Header.DataSize = htonl((uint32_t)DataSize);

    // Send header
    Result = __TCP_sendAll(SocketFd, &Header, sizeof(Header));
    if (Result != IOC_RESULT_SUCCESS) {
        return Result;
    }

    // Send data payload
    Result = __TCP_sendAll(SocketFd, pData, DataSize);
    return Result;
}

/**
 * @brief Receive data over TCP (polling mode)
 * Not implemented yet - receiver uses callback mode via receiver thread
 */
static IOC_Result_T __IOC_recvData_ofProtoTCP(_IOC_LinkObject_pT pLinkObj, IOC_DatDesc_pT pDatDesc,
                                              const IOC_Options_pT pOption) {
    // TODO: Implement polling mode if needed
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

    .OpSendData_F = __IOC_sendData_ofProtoTCP,  // 🟢 GREEN: Implemented for TC-9, TC-10, TC-11
    .OpRecvData_F = __IOC_recvData_ofProtoTCP,  // Not implemented - uses callback mode
};
