/**
 * @file _IOC_Types.h
 * @brief This is internal header file used to define types for internal use only.
 */
#include <IOC/IOC.h>  //Module IOC's public header file
#include <pthread.h>  //POSIX pthread

#ifndef __INTER_OBJECT_COMMUNICATION_TYPES_H__
#define __INTER_OBJECT_COMMUNICATION_TYPES_H__
#ifdef __cplusplus
extern "C" {
#endif

typedef struct _IOC_SrvProtoMethodsStru _IOC_SrvProtoMethods_T;
typedef _IOC_SrvProtoMethods_T *_IOC_SrvProtoMethods_pT;

typedef struct _IOC_LinkObjectStru _IOC_LinkObject_T;
typedef _IOC_LinkObject_T *_IOC_LinkObject_pT;

typedef struct {
    IOC_SrvID_T ID;
    IOC_SrvArgs_T Args;

    _IOC_SrvProtoMethods_pT pMethods;

    // WHEN Flags has IOC_SRVFLAG_BROADCAST_EVENT
    struct {
        pthread_t DaemonThreadID;

#define _MAX_BROADCAST_EVENT_ACCEPTED_LINK_NUM 3
        _IOC_LinkObject_pT pAcceptedLinks[_MAX_BROADCAST_EVENT_ACCEPTED_LINK_NUM];
    } BroadcastEvent;

    // WHEN Flags has IOC_SRVFLAG_AUTO_ACCEPT
    struct {
        pthread_t DaemonThreadID;

#define _MAX_AUTO_ACCEPT_ACCEPTED_LINK_NUM 16  // Increased to support more concurrent connections
        _IOC_LinkObject_pT pAcceptedLinks[_MAX_AUTO_ACCEPT_ACCEPTED_LINK_NUM];
        int AcceptedLinkCount;
    } AutoAccept;

    void *pProtoPriv;
} _IOC_ServiceObject_T, *_IOC_ServiceObject_pT;

struct _IOC_LinkObjectStru {
    IOC_LinkID_T ID;
    IOC_ConnArgs_T Args;

    _IOC_SrvProtoMethods_pT pMethods;

    void *pProtoPriv;
};

_IOC_LinkObject_pT _IOC_getLinkObjByLinkID(IOC_LinkID_T LinkID);

struct _IOC_SrvProtoMethodsStru {
    const char *pProtocol;

    IOC_Result_T (*OpOnlineService_F)(_IOC_ServiceObject_pT);
    IOC_Result_T (*OpOfflineService_F)(_IOC_ServiceObject_pT);

    IOC_Result_T (*OpConnectService_F)(_IOC_LinkObject_pT, const IOC_ConnArgs_pT, const IOC_Options_pT);
    IOC_Result_T (*OpAcceptClient_F)(_IOC_ServiceObject_pT, _IOC_LinkObject_pT, const IOC_Options_pT);
    IOC_Result_T (*OpCloseLink_F)(_IOC_LinkObject_pT);

    IOC_Result_T (*OpSubEvt_F)(_IOC_LinkObject_pT, const IOC_SubEvtArgs_pT);
    IOC_Result_T (*OpUnsubEvt_F)(_IOC_LinkObject_pT, const IOC_UnsubEvtArgs_pT);

    IOC_Result_T (*OpPostEvt_F)(_IOC_LinkObject_pT, const IOC_EvtDesc_pT, const IOC_Options_pT);

    // 🔧 WHY ADD DAT METHODS: The original framework only supported EVT (events), but was missing
    // DAT (data transfer) protocol layer abstraction. Without these methods, DAT operations like
    // IOC_sendDAT() bypassed the protocol layer entirely, using global variables instead.
    // This caused cross-protocol communication to fail because each protocol needs its own
    // data transmission mechanism (e.g., FIFO uses direct callbacks, TCP would use sockets).
    //
    // 📋 DESIGN CHOICE: Following SrvProto.md specification, we use "OpSendData_F/OpRecvData_F"
    // naming convention (not OpSendDAT_F) to maintain consistency with the framework's
    // method naming pattern where operations are prefixed with "Op" and suffixed with "_F".
    IOC_Result_T (*OpSendData_F)(_IOC_LinkObject_pT, const IOC_DatDesc_pT, const IOC_Options_pT);
    IOC_Result_T (*OpRecvData_F)(_IOC_LinkObject_pT, IOC_DatDesc_pT, const IOC_Options_pT);
};

//_gIOC_XYZ is the global variable used intra-IOC module.
extern _IOC_SrvProtoMethods_T _gIOC_SrvProtoFifoMethods;

#ifdef __cplusplus
}
#endif
#endif  //__INTER_OBJECT_COMMUNICATION_TYPES_H__