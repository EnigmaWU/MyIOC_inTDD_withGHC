#include "_IOC.h"

typedef struct {
} _IOC_SrvProtoFifoPriv_T, *_IOC_SrvProtoFifoPriv_pT;

static IOC_Result_T __IOC_onlineService_ofProtoFifo(_IOC_ServiceObject_pT pSrvObj) {
    _IOC_LogNotTested();
    return IOC_RESULT_NOT_IMPLEMENTED;
}

_IOC_SrvProtoMethods_T _gIOC_SrvProtoFifoMethods = {
    .pProtocol = IOC_SRV_PROTO_FIFO,

    .OpOnlineService_F = __IOC_onlineService_ofProtoFifo,

};