#include "_IOC.h"

typedef struct {
} _IOC_SrvProtoFifoPriv_T, *_IOC_SrvProtoFifoPriv_pT;

static IOC_Result_T __IOC_onlineService_ofProtoFifo(_IOC_ServiceObject_pT pSrvObj) {
    //_IOC_LogNotTested();
    return IOC_RESULT_SUCCESS;  // NOTHING DONE, JUST RETURN SUCCESS
}

static IOC_Result_T __IOC_offlineService_ofProtoFifo(_IOC_ServiceObject_pT pSrvObj) {
    //_IOC_LogNotTested();
    return IOC_RESULT_SUCCESS;  // NOTHING DONE, JUST RETURN SUCCESS
}

static IOC_Result_T __IOC_connectService_ofProtoFifo(_IOC_LinkObject_pT pLinkObj, const IOC_ConnArgs_pT pConnArgs,
                                                     const IOC_Options_pT pOption) {
    //_IOC_LogNotTested();
    return IOC_RESULT_SUCCESS;  // NOTHING DONE, JUST RETURN SUCCESS
}

static IOC_Result_T __IOC_acceptClient_ofProtoFifo(_IOC_ServiceObject_pT pSrvObj, _IOC_LinkObject_pT pLinkObj,
                                                   const IOC_Options_pT pOption) {
    //_IOC_LogNotTested();
    return IOC_RESULT_SUCCESS;  // NOTHING DONE, JUST RETURN SUCCESS
}

static IOC_Result_T __IOC_closeLink_ofProtoFifo(_IOC_LinkObject_pT pLinkObj) {
    //_IOC_LogNotTested();
    return IOC_RESULT_SUCCESS;  // NOTHING DONE, JUST RETURN SUCCESS
}

static IOC_SubEvtArgs_T _mIOC_SubEvtArgs = {};

static IOC_Result_T __IOC_subEvt_ofProtoFifo(_IOC_LinkObject_pT pLinkObj, const IOC_SubEvtArgs_pT pSubEvtArgs) {
    memcpy(&_mIOC_SubEvtArgs, pSubEvtArgs, sizeof(IOC_SubEvtArgs_T));
    //_IOC_LogNotTested();
    return IOC_RESULT_SUCCESS;  // NOTHING DONE, JUST RETURN SUCCESS
}

static IOC_Result_T __IOC_unsubEvt_ofProtoFifo(_IOC_LinkObject_pT pLinkObj, const IOC_UnsubEvtArgs_pT pUnsubEvtArgs) {
    //_IOC_LogNotTested();
    return IOC_RESULT_SUCCESS;  // NOTHING DONE, JUST RETURN SUCCESS
}

static IOC_Result_T __IOC_postEvt_ofProtoFifo(_IOC_LinkObject_pT pLinkObj, const IOC_EvtDesc_pT pEvtDesc,
                                              const IOC_Options_pT pOption) {
    _mIOC_SubEvtArgs.CbProcEvt_F(pEvtDesc, _mIOC_SubEvtArgs.pCbPrivData);
    //_IOC_LogNotTested();
    return IOC_RESULT_SUCCESS;  // NOTHING DONE, JUST RETURN SUCCESS
}

_IOC_SrvProtoMethods_T _gIOC_SrvProtoFifoMethods = {
    .pProtocol = IOC_SRV_PROTO_FIFO,

    .OpOnlineService_F = __IOC_onlineService_ofProtoFifo,
    .OpOfflineService_F = __IOC_offlineService_ofProtoFifo,

    .OpConnectService_F = __IOC_connectService_ofProtoFifo,
    .OpAcceptClient_F = __IOC_acceptClient_ofProtoFifo,

    .OpCloseLink_F = __IOC_closeLink_ofProtoFifo,

    .OpSubEvt_F = __IOC_subEvt_ofProtoFifo,
    .OpUnsubEvt_F = __IOC_unsubEvt_ofProtoFifo,

    .OpPostEvt_F = __IOC_postEvt_ofProtoFifo,
};