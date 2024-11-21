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

#define _MAX_SUB_EVT_NUM 2
static IOC_SubEvtArgs_T _mIOC_SubEvtArgs[_MAX_SUB_EVT_NUM] = {};

static IOC_Result_T __IOC_subEvt_ofProtoFifo(_IOC_LinkObject_pT pLinkObj, const IOC_SubEvtArgs_pT pSubEvtArgs) {
    for (int i = 0; i < _MAX_SUB_EVT_NUM; i++) {
        if (0 == _mIOC_SubEvtArgs[i].CbProcEvt_F) {
            _mIOC_SubEvtArgs[i].CbProcEvt_F = pSubEvtArgs->CbProcEvt_F;
            _mIOC_SubEvtArgs[i].pCbPrivData = pSubEvtArgs->pCbPrivData;

            _mIOC_SubEvtArgs[i].EvtNum = pSubEvtArgs->EvtNum;
            _mIOC_SubEvtArgs[i].pEvtIDs = malloc(sizeof(IOC_EvtID_T) * pSubEvtArgs->EvtNum);
            memcpy(_mIOC_SubEvtArgs[i].pEvtIDs, pSubEvtArgs->pEvtIDs, sizeof(IOC_EvtID_T) * pSubEvtArgs->EvtNum);

            //_IOC_LogNotTested();
            return IOC_RESULT_SUCCESS;
        }
    }

    _IOC_LogNotTested();
    return IOC_RESULT_TOO_MANY_EVENT_CONSUMER;
}

static IOC_Result_T __IOC_unsubEvt_ofProtoFifo(_IOC_LinkObject_pT pLinkObj, const IOC_UnsubEvtArgs_pT pUnsubEvtArgs) {
    for (int i = 0; i < _MAX_SUB_EVT_NUM; i++) {
        if ((pUnsubEvtArgs->pCbPrivData == _mIOC_SubEvtArgs[i].pCbPrivData) &&
            (pUnsubEvtArgs->CbProcEvt_F == _mIOC_SubEvtArgs[i].CbProcEvt_F)) {
            _mIOC_SubEvtArgs[i].CbProcEvt_F = NULL;

            free(_mIOC_SubEvtArgs[i].pEvtIDs);
            _mIOC_SubEvtArgs[i].pEvtIDs = NULL;

            //_IOC_LogNotTested();
            return IOC_RESULT_SUCCESS;
        }
    }

    _IOC_LogNotTested();
    return IOC_RESULT_NOT_EXIST;
}

static IOC_Result_T __IOC_postEvt_ofProtoFifo(_IOC_LinkObject_pT pLinkObj, const IOC_EvtDesc_pT pEvtDesc,
                                              const IOC_Options_pT pOption) {
    IOC_Result_T Result = IOC_RESULT_BUG;
    int ProcEvtSuberCnt = 0;

    for (int i = 0; i < _MAX_SUB_EVT_NUM; i++) {
        if (0 != _mIOC_SubEvtArgs[i].CbProcEvt_F) {
            for (int j = 0; j < _mIOC_SubEvtArgs[i].EvtNum; j++) {
                if (pEvtDesc->EvtID == _mIOC_SubEvtArgs[i].pEvtIDs[j]) {
                    _mIOC_SubEvtArgs[i].CbProcEvt_F(pEvtDesc, _mIOC_SubEvtArgs[i].pCbPrivData);
                    ProcEvtSuberCnt++;
                }
            }
        }
    }

    if (ProcEvtSuberCnt > 0) {
        Result = IOC_RESULT_SUCCESS;
    } else {
        Result = IOC_RESULT_NO_EVENT_CONSUMER;
    }

    //_IOC_LogNotTested();
    return Result;
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