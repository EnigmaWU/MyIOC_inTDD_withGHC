/**
 * @file _IOC_SrvProtoFifo.c
 * @brief This is IOC's Service Protocol FIFO internal Define&Design&Implement file.
 *
 *=================================================================================================
 *===>>>Definition of FIFO Protocol
 * @FIFO: First In First Out, a.k.a Link2Link Queue, used to transmit messages in FIFO order.
 *      @WEHN: Service onlined with IOC_SRV_PROTO_FIFO, @AND: Client connect to this service,
 *      @THEN: a FIFO is created and established by a pair of LinkIDs,
 *              one in server side named LinkID_atSrv, one in client side named LinkID_atCli.
 *      @SO IF: LinkID_atSrv is EvtProducer, LinkID_atCli is EvtConsumer,
 *          @THEN-A: EvtConsumer will subscribe events on LinkID_atCli,
 *          @THEN-B: EvtProducer will post events to LinkID_atSrv,
 *                      which will be transmited with the FIFO to LinkID_atCli.
 *          @THEN-C: EvtConsumer will process the events in CbProcEvt_F.
 *      @SO IF: LinkID_atSrv is EvtConsumer, LinkID_atCli is EvtProducer,
 *          ......(TODO)
 *      @SO IF: LinkID_atSrv is CmdExecutor, LinkID_atCli is CmdInitiator,
 *          ......(TODO)
 *      @SO IF: LinkID_atSrv is CmdInitiator, LinkID_atCli is CmdExecutor,
 *          ......(TODO)
 *      @SO IF: LinkID_atSrv is DatReceiver, LinkID_atCli is DatSender,
 *          ......(TODO)
 *      @SO IF: LinkID_atSrv is DatSender, LinkID_atCli is DatReceiver,
 *          ......(TODO)
 *
 */

#include "_IOC.h"

typedef struct _IOC_ProtoFifoLinkObjectStru _IOC_ProtoFifoLinkObject_T;
typedef _IOC_ProtoFifoLinkObject_T *_IOC_ProtoFifoLinkObject_pT;

/**
 * @brief FIFO Link Object, used to transmit messages in FIFO order.
 *  LinkObject's pProtoPriv points to this object.
 *
 * @details
 *    LinkFifoObj_atSrv: LinkID_atSrv's pProtoPriv points to this object,
 *      which is created when acceptClient, and destroyed when closeLink.
 *    LinkFifoObj_atCli: LinkID_atCli's pProtoPriv points to this object,
 *      which is created when connectService, and destroyed when closeLink.
 *    @AFTER: acceptClient vs connectService
 *      LinkFifoObj_atSrv->pPeer = LinkFifoObj_atCli,
 *      LinkFifoObj_atCli->pPeer = LinkFifoObj_atSrv.
 *
 */
struct _IOC_ProtoFifoLinkObjectStru {
    pthread_mutex_t Mutex;

    _IOC_ProtoFifoLinkObject_pT pPeer;
    union {
        IOC_SubEvtArgs_T SubEvtArgs;  // SET by subEvt, USED when postEvt
    };
};

typedef struct {
    _IOC_ServiceObject_pT pSrvObj;

    // HOLD when enter connectService, RELEASE when exit connectService, to let connect take place one by one.
    pthread_mutex_t ConnMutex;
    _IOC_LinkObject_pT pConnLinkObj;  // SET by connectService, USED by acceptClient

    pthread_mutex_t WaitAcceptMutex;
    pthread_cond_t WaitAcceptCond;

} _IOC_ProtoFifoServiceObject_T, *_IOC_ProtoFifoServiceObject_pT;

#define _MAX_PROTO_FIFO_SERVICES 16
static _IOC_ProtoFifoServiceObject_pT _mIOC_OnlinedSrvProtoFifoObjs[_MAX_PROTO_FIFO_SERVICES] = {};
static pthread_mutex_t _mIOC_OnlinedSrvProtoFifoObjsMutex = PTHREAD_MUTEX_INITIALIZER;

static _IOC_ProtoFifoServiceObject_pT __IOC_getSrvProtoObjBySrvURI(const IOC_SrvURI_pT pSrvURI) {
    for (int i = 0; i < _MAX_PROTO_FIFO_SERVICES; i++) {
        _IOC_ProtoFifoServiceObject_pT pFifoSrvObj = _mIOC_OnlinedSrvProtoFifoObjs[i];

        if (NULL != pFifoSrvObj &&
            (IOC_RESULT_YES == IOC_Helper_isEqualSrvURI(pSrvURI, &pFifoSrvObj->pSrvObj->Args.SrvURI))) {
            return pFifoSrvObj;
        }
    }

    return NULL;
}

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
    // Step-1: Check Parameters
    //...

    // Step-2:  get the service object by SrvURI
    _IOC_ProtoFifoServiceObject_pT pFifoSrvObj = __IOC_getSrvProtoObjBySrvURI(&pConnArgs->SrvURI);
    if (NULL == pFifoSrvObj) {
        _IOC_LogWarn("Failed to get the service object by SrvURI(%s)",
                     IOC_Helper_printSingleLineSrvURI(&pConnArgs->SrvURI, NULL, 0));
        _IOC_LogNotTested();
        return IOC_RESULT_NOT_EXIST_SERVICE;
    }

    // Step-3: Create a FifoLinkObj
    _IOC_ProtoFifoLinkObject_pT pFifoLinkObj = calloc(1, sizeof(_IOC_ProtoFifoLinkObject_T));
    if (NULL == pFifoLinkObj) {
        _IOC_LogBug("Failed to alloc a new FifoLinkObj when connect service");
        _IOC_LogNotTested();
        return IOC_RESULT_POSIX_ENOMEM;
    }

    pthread_mutex_init(&pFifoLinkObj->Mutex, NULL);
    pLinkObj->pProtoPriv = pFifoLinkObj;

    // Step-4: Set the connection link object
    pthread_mutex_lock(&pFifoSrvObj->ConnMutex);
    pFifoSrvObj->pConnLinkObj = pLinkObj;

    // Step-5: Wait for the acceptClient
    pthread_mutex_lock(&pFifoSrvObj->WaitAcceptMutex);
    pthread_cond_wait(&pFifoSrvObj->WaitAcceptCond, &pFifoSrvObj->WaitAcceptMutex);
    pthread_mutex_unlock(&pFifoSrvObj->WaitAcceptMutex);

    // Step-6: Release the connection link object
    pFifoSrvObj->pConnLinkObj = NULL;
    pthread_mutex_unlock(&pFifoSrvObj->ConnMutex);

    //_IOC_LogNotTested();
    return IOC_RESULT_SUCCESS;  // NOTHING DONE, JUST RETURN SUCCESS
}

static IOC_Result_T __IOC_acceptClient_ofProtoFifo(_IOC_ServiceObject_pT pSrvObj, _IOC_LinkObject_pT pLinkObj,
                                                   const IOC_Options_pT pOption) {
    //_IOC_LogNotTested();
    return IOC_RESULT_SUCCESS;  // NOTHING DONE, JUST RETURN SUCCESS
}

static IOC_Result_T __IOC_closeLink_ofProtoFifo(_IOC_LinkObject_pT pLinkObj) {
    _IOC_ProtoFifoLinkObject_pT pLinkFifoObj = (_IOC_ProtoFifoLinkObject_pT)pLinkObj->pProtoPriv;

    pthread_mutex_lock(&pLinkFifoObj->Mutex);
    if (NULL != pLinkFifoObj->pPeer) {
        pthread_mutex_lock(&pLinkFifoObj->pPeer->Mutex);
        pLinkFifoObj->pPeer->pPeer = NULL;
        pthread_mutex_unlock(&pLinkFifoObj->pPeer->Mutex);
    }
    pthread_mutex_unlock(&pLinkFifoObj->Mutex);

    free(pLinkFifoObj);

    //_IOC_LogNotTested();
    return IOC_RESULT_SUCCESS;  // NOTHING DONE, JUST RETURN SUCCESS
}

static IOC_Result_T __IOC_subEvt_ofProtoFifo(_IOC_LinkObject_pT pLinkObj, const IOC_SubEvtArgs_pT pSubEvtArgs) {
    _IOC_ProtoFifoLinkObject_pT pLinkFifoObj = (_IOC_ProtoFifoLinkObject_pT)pLinkObj->pProtoPriv;

    memcpy(&pLinkFifoObj->SubEvtArgs, pSubEvtArgs, sizeof(IOC_SubEvtArgs_T));

    pLinkFifoObj->SubEvtArgs.pEvtIDs = (IOC_EvtID_T *)malloc(pSubEvtArgs->EvtNum * sizeof(IOC_EvtID_T));
    memcpy(pLinkFifoObj->SubEvtArgs.pEvtIDs, pSubEvtArgs->pEvtIDs, pSubEvtArgs->EvtNum * sizeof(IOC_EvtID_T));

    _IOC_LogNotTested();
    return IOC_RESULT_SUCCESS;
}

static IOC_Result_T __IOC_unsubEvt_ofProtoFifo(_IOC_LinkObject_pT pLinkObj, const IOC_UnsubEvtArgs_pT pUnsubEvtArgs) {
    _IOC_ProtoFifoLinkObject_pT pLinkFifoObj = (_IOC_ProtoFifoLinkObject_pT)pLinkObj->pProtoPriv;

    if (pLinkFifoObj->SubEvtArgs.CbProcEvt_F == pUnsubEvtArgs->CbProcEvt_F &&
        pLinkFifoObj->SubEvtArgs.pCbPrivData == pUnsubEvtArgs->pCbPrivData) {
        free(pLinkFifoObj->SubEvtArgs.pEvtIDs);
        memset(&pLinkFifoObj->SubEvtArgs, 0, sizeof(IOC_SubEvtArgs_T));

        _IOC_LogNotTested();
        return IOC_RESULT_SUCCESS;
    }

    _IOC_LogNotTested();
    return IOC_RESULT_NOT_EXIST;
}

static IOC_Result_T __IOC_postEvt_ofProtoFifo(_IOC_LinkObject_pT pLinkObj, const IOC_EvtDesc_pT pEvtDesc,
                                              const IOC_Options_pT pOption) {
    _IOC_ProtoFifoLinkObject_pT pLinkFifoObj = (_IOC_ProtoFifoLinkObject_pT)pLinkObj->pProtoPriv;
    IOC_Result_T Result = IOC_RESULT_BUG;
    int ProcEvtSuberCnt = 0;

    pthread_mutex_lock(&pLinkFifoObj->Mutex);
    if (NULL != pLinkFifoObj->pPeer) {
        pthread_mutex_lock(&pLinkFifoObj->pPeer->Mutex);
        if (NULL != pLinkFifoObj->pPeer->SubEvtArgs.CbProcEvt_F) {
            for (int i = 0; i < pLinkFifoObj->pPeer->SubEvtArgs.EvtNum; i++) {
                if (pEvtDesc->EvtID == pLinkFifoObj->pPeer->SubEvtArgs.pEvtIDs[i]) {
                    pLinkFifoObj->pPeer->SubEvtArgs.CbProcEvt_F(pEvtDesc, pLinkFifoObj->pPeer->SubEvtArgs.pCbPrivData);
                    ProcEvtSuberCnt++;
                }
            }
        }
        pthread_mutex_unlock(&pLinkFifoObj->pPeer->Mutex);
    }
    pthread_mutex_unlock(&pLinkFifoObj->Mutex);

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