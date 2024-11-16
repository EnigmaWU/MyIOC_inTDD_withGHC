/**
 * @file _IOC_Types.h
 * @brief This is internal header file used to define types for internal use only.
 */

#include <IOC/IOC.h>  //Module IOC's public header file

#ifndef __INTER_OBJECT_COMMUNICATION_TYPES_H__
#define __INTER_OBJECT_COMMUNICATION_TYPES_H__
#ifdef __cplusplus
extern "C" {
#endif

typedef struct _IOC_SrvProtoMethodsStru _IOC_SrvProtoMethods_T;
typedef _IOC_SrvProtoMethods_T *_IOC_SrvProtoMethods_pT;

typedef struct {
    IOC_SrvID_T ID;
    IOC_SrvArgs_T Args;

    _IOC_SrvProtoMethods_pT pMethods;

    void *pProtoPriv;
} _IOC_ServiceObject_T, *_IOC_ServiceObject_pT;

typedef struct {
    IOC_LinkID_T ID;
    IOC_ConnArgs_T Args;

    _IOC_SrvProtoMethods_pT pMethods;

    void *pProtoPriv;
} _IOC_LinkObject_T, *_IOC_LinkObject_pT;

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
};

//_gIOC_XYZ is the global variable used intra-IOC module.
extern _IOC_SrvProtoMethods_T _gIOC_SrvProtoFifoMethods;

#ifdef __cplusplus
}
#endif
#endif  //__INTER_OBJECT_COMMUNICATION_TYPES_H__