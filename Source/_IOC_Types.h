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

typedef struct {
    IOC_SrvID_T ID;
    IOC_SrvArgs_T Args;

    void *pProtoPriv;
} _IOC_ServiceObject_T, *_IOC_ServiceObject_pT;

typedef struct {
    const char *pProtocol;

    IOC_Result_T (*OpOnlineService_F)(_IOC_ServiceObject_pT pSrvObj);
    IOC_Result_T (*OpOfflineService_F)(_IOC_ServiceObject_pT pSrvObj);

} _IOC_SrvProtoMethods_T, *_IOC_SrvProtoMethods_pT;

//_gIOC_XYZ is the global variable used intra-IOC module.
extern _IOC_SrvProtoMethods_T _gIOC_SrvProtoFifoMethods;

#ifdef __cplusplus
}
#endif
#endif  //__INTER_OBJECT_COMMUNICATION_TYPES_H__