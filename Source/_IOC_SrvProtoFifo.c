/**
 * @file _IOC_SrvProtoFifo.c
 * @brief This is IOC's Service Protocol FIFO internal Define&Design&Implement file.
 *
 *=================================================================================================
 *===>>>Definition of FIFO Protocol
 * @FIFO: First In First Out, a.k.a Link2Link Queue, used to transmit messages in FIFO order.
 *      @WEHN: Service onlined with IOC_SRV_PROTO_FIFO got SrvID, @AND: Client connect to this service,
 *      @THEN: the FIFO is created and established by a pair of LinkIDs,
 *              one on server side named LinkID_atSrv(aka SrvLinkID),
 *              one on client side named LinkID_atCli(aka CliLinkID).
 *
 *      @------------------------------------------------------------------------------------------
 *      @SO IF: LinkID_atSrv is EvtProducer, LinkID_atCli is EvtConsumer,
 *          @THEN-A: EvtConsumer will subscribe events on LinkID_atCli with CbProcEvt_F,
 *          @THEN-B: EvtProducer will post events to LinkID_atSrv,
 *                      which will be transmitted via the FIFO to LinkID_atCli.
 *          @THEN-C: EvtConsumer will process the events in CbProcEvt_F.
 *          @ALSO: SrvID MAY as EvrProducer, THEN post events to this SrvID will broadcast to all clients.
 *      @SO IF: LinkID_atSrv is EvtConsumer, LinkID_atCli is EvtProducer,
 *          @THEN-A: EvtConsumer will subscribe events on LinkID_atSrv with CbProcEvt_F,
 *          @THEN-B: EvtProducer will post events to LinkID_atCli,
 *                      which will be transmitted via the FIFO to LinkID_atSrv.
 *          @THEN-C: EvtConsumer will process the events in CbProcEvt_F.
 *
 *      @------------------------------------------------------------------------------------------
 *      @SO IF: LinkID_atSrv is CmdExecutor, LinkID_atCli is CmdInitiator,
 *          ......(TODO)
 *      @SO IF: LinkID_atSrv is CmdInitiator, LinkID_atCli is CmdExecutor,
 *          ......(TODO)
 *
 *      @------------------------------------------------------------------------------------------
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
 * @brief ProtoFIFO's 【Link Object】(a.k.a FifoLinkObj), used to transmit MSG<EVT/CMD/DATA> in FIFO order.
 *  LinkObject->pProtoPriv = (ProtoFifoLinkObject_pT)pFifoLinkObj.
 *
 * @details
 *    LinkFifoObj_atSrv == LinkObject_atSrv->pProtoPriv
 *      which is created when acceptClient, and destroyed when closeLink.
 *    LinkFifoObj_atCli == LinkObject_atCli->pProtoPriv
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

/**
 * @brief ProtoFIFO's 【Service Object】(a.k.a FifoSrvObj), used to establish a pair of FifoLinkObj.
 *  ServiceObject->pProtoPriv = (ProtoFifoServiceObject_pT)pFifoSrvObj.
 *
 * @details
 *    FifoSrvObj: created when onlineService, destroyed when offlineService.
 *    @SRVSIDE: wait new connection in acceptClient, by WaitNewConn[Mutex|Cond]，
 *      and ACCEPT the new incoming peer LinkObject which is saved in pConnLinkObj.
 *    @CLISIDE: lock the connection in connectService, by ConnMutex,
 *      then set the CONNECT LinkObject in pConnLinkObj.
 *      Afterly wakeup service to ACCEPT connection by WaitNewConn[Mutex|Cond],
 *      and lastly wait for the connection to be accepted by WaitAccept[Mutex|Cond].
 */
typedef struct {
    _IOC_ServiceObject_pT pSrvObj;

    // HOLD when enter connectService, RELEASE when exit connectService, to let connect take place one by one.
    pthread_mutex_t ConnMutex;
    _IOC_LinkObject_pT pConnLinkObj;  // SET by connectService, USED by acceptClient

    pthread_mutex_t WaitAccptedMutex;
    pthread_cond_t WaitAccptedCond;

    pthread_mutex_t WaitNewConnMutex;
    pthread_cond_t WaitNewConnCond;

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
/**
 * @brief Initializes and brings online a ProtoFifo service object.
 *
 * This function performs the following steps:
 * 1. Validates the input service object parameters.
 * 2. Allocates and initializes a new ProtoFifoServiceObject, including setting up necessary mutexes and condition
 * variables required for MacOS.
 * 3. Stores the initialized ProtoFifoServiceObject in the global online services array.
 *
 * @param pSrvObj Pointer to the IOC_ServiceObject structure to be initialized.
 *
 * @return IOC_RESULT_SUCCESS on successful initialization,
 *         IOC_RESULT_POSIX_ENOMEM if memory allocation fails.
 */

static IOC_Result_T __IOC_onlineService_ofProtoFifo(_IOC_ServiceObject_pT pSrvObj) {
    // Step-1: Check Parameters
    //...

    // Step-2: Create a ProtoFifoServiceObject
    _IOC_ProtoFifoServiceObject_pT pFifoSrvObj = calloc(1, sizeof(_IOC_ProtoFifoServiceObject_T));
    if (NULL == pFifoSrvObj) {
        _IOC_LogBug("Failed to alloc a new ProtoFifoServiceObject when online service");
        _IOC_LogNotTested();
        return IOC_RESULT_POSIX_ENOMEM;
    } else {
        pSrvObj->pProtoPriv = pFifoSrvObj;
        pFifoSrvObj->pSrvObj = pSrvObj;

        // Init Mutex&Cond is MUST on MacOS
        pthread_mutex_init(&pFifoSrvObj->ConnMutex, NULL);
        pthread_mutex_init(&pFifoSrvObj->WaitAccptedMutex, NULL);
        pthread_cond_init(&pFifoSrvObj->WaitAccptedCond, NULL);
        pthread_mutex_init(&pFifoSrvObj->WaitNewConnMutex, NULL);
        pthread_cond_init(&pFifoSrvObj->WaitNewConnCond, NULL);
    }

    // Step-3: Save the ProtoFifoServiceObject
    pthread_mutex_lock(&_mIOC_OnlinedSrvProtoFifoObjsMutex);
    for (int i = 0; i < _MAX_PROTO_FIFO_SERVICES; i++) {
        if (NULL == _mIOC_OnlinedSrvProtoFifoObjs[i]) {
            _mIOC_OnlinedSrvProtoFifoObjs[i] = pFifoSrvObj;
            break;
        }
    }
    pthread_mutex_unlock(&_mIOC_OnlinedSrvProtoFifoObjsMutex);

    //_IOC_LogNotTested();
    return IOC_RESULT_SUCCESS;  // NOTHING DONE, JUST RETURN SUCCESS
}

/**
 * @brief Handles the offline processing of a ProtoFifo service.
 *
 * This function performs the necessary cleanup when a ProtoFifo service is taken offline.
 * It validates the provided service object, retrieves the associated ProtoFifoServiceObject,
 * and removes it from the list of online ProtoFifo services in a thread-safe manner.
 *
 * @param pSrvObj Pointer to the service object to be processed.
 * @return IOC_RESULT_SUCCESS Indicates successful completion of the offline process.
 */
static IOC_Result_T __IOC_offlineService_ofProtoFifo(_IOC_ServiceObject_pT pSrvObj) {
    // Step-1: Check Parameters
    //...

    // Step-2: Get the ProtoFifoServiceObject
    _IOC_ProtoFifoServiceObject_pT pFifoSrvObj = (_IOC_ProtoFifoServiceObject_pT)pSrvObj->pProtoPriv;

    // Step-3: Clear the ProtoFifoServiceObject
    pthread_mutex_lock(&_mIOC_OnlinedSrvProtoFifoObjsMutex);
    for (int i = 0; i < _MAX_PROTO_FIFO_SERVICES; i++) {
        if (pFifoSrvObj == _mIOC_OnlinedSrvProtoFifoObjs[i]) {
            _mIOC_OnlinedSrvProtoFifoObjs[i] = NULL;
            break;
        }
    }
    pthread_mutex_unlock(&_mIOC_OnlinedSrvProtoFifoObjsMutex);

    // 销毁互斥量和条件变量
    pthread_mutex_destroy(&pFifoSrvObj->ConnMutex);
    pthread_mutex_destroy(&pFifoSrvObj->WaitAccptedMutex);
    pthread_cond_destroy(&pFifoSrvObj->WaitAccptedCond);
    pthread_mutex_destroy(&pFifoSrvObj->WaitNewConnMutex);
    pthread_cond_destroy(&pFifoSrvObj->WaitNewConnCond);

    // 释放服务对象内存
    free(pFifoSrvObj);

    //_IOC_LogNotTested();
    return IOC_RESULT_SUCCESS;  // NOTHING DONE, JUST RETURN SUCCESS
}
/**
 * @brief Establishes a connection to a ProtoFifo service.
 *
 * This function connects to the specified service using the ProtoFifo protocol by performing the following steps:
 * 1. Validates the input parameters.
 * 2. Retrieves the service object based on the provided service URI.
 * 3. Allocates and initializes a new ProtoFifo link object.
 * 4. Locks the connection acceptance process to ensure thread safety.
 * 5. Signals the accept client to handle the new connection and waits for the client to accept.
 * 6. Releases the connection link object and cleans up resources.
 *
 * @param pLinkObj Pointer to the link object that represents the connection.
 * @param pConnArgs Pointer to the connection arguments, including the service URI.
 * @param pOption Pointer to the connection options.
 *
 * @return IOC_Result_T
 *         - `IOC_RESULT_SUCCESS` on successful connection.
 *         - `IOC_RESULT_NOT_EXIST_SERVICE` if the service does not exist.
 *         - `IOC_RESULT_POSIX_ENOMEM` if memory allocation fails.
 *         - Other relevant error codes as defined in `IOC_Result_T`.
 */

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

    // Step-4: Lock the connection accepting process
    pthread_mutex_lock(&pFifoSrvObj->ConnMutex);
    _IOC_LogAssert(NULL == pFifoSrvObj->pConnLinkObj);

    // Step-5: Wakeup&Wait for the acceptClient
    pthread_mutex_lock(&pFifoSrvObj->WaitAccptedMutex);
    pFifoSrvObj->pConnLinkObj = pLinkObj;  // set the new incoming connection link object

    pthread_cond_signal(&pFifoSrvObj->WaitNewConnCond);  // wakeup the acceptClient if it is waiting

    pthread_cond_wait(&pFifoSrvObj->WaitAccptedCond,
                      &pFifoSrvObj->WaitAccptedMutex);  // wait for the acceptClient to complete

    // MAKE SURE the connection is accepted by the acceptClient
    _IOC_LogAssert(NULL != pFifoLinkObj->pPeer);

    // pFifoSrvObj->pConnLinkObj = NULL;  // clear the connection link object
    pthread_mutex_unlock(&pFifoSrvObj->WaitAccptedMutex);

    // Step-6: Release the connection link object
    pthread_mutex_unlock(&pFifoSrvObj->ConnMutex);

    //_IOC_LogNotTested();
    return IOC_RESULT_SUCCESS;
}
/**
 * @brief Accepts a client connection for the ProtoFifo service.
 *
 * This function performs the following steps:
 * 1. Validates the input parameters.
 * 2. Allocates and initializes a new ProtoFifoLinkObject.
 * 3. Attempts to accept an incoming connection immediately if available.
 *    - If a connection is waiting, it establishes the peer relationship.
 *    - If no connection is available, it waits for a new connection with a 10ms timeout.
 *
 * @param pSrvObj Pointer to the service object.
 * @param pLinkObj Pointer to the link object associated with the client.
 * @param pOption Pointer to the options for accepting the client.
 *
 * @return IOC_Result_T
 *         - `IOC_RESULT_SUCCESS` on successful acceptance.
 *         - `IOC_RESULT_POSIX_ENOMEM` if memory allocation fails.
 */

static IOC_Result_T __IOC_acceptClient_ofProtoFifo(_IOC_ServiceObject_pT pSrvObj, _IOC_LinkObject_pT pLinkObj,
                                                   const IOC_Options_pT pOption) {
    // Step-1: Check Parameters
    //...

    // Step-2: create a new FifoLinkObj
    _IOC_ProtoFifoLinkObject_pT pAceptedFifoLinkObj = calloc(1, sizeof(_IOC_ProtoFifoLinkObject_T));
    if (NULL == pAceptedFifoLinkObj) {
        _IOC_LogBug("Failed to alloc a new FifoLinkObj when accept client");
        _IOC_LogNotTested();
        return IOC_RESULT_POSIX_ENOMEM;
    }

    pthread_mutex_init(&pAceptedFifoLinkObj->Mutex, NULL);
    pLinkObj->pProtoPriv = pAceptedFifoLinkObj;

    // Step-3: IF new incoming connection is waiting, then accept it immediately, ELSE wait for it.
    _IOC_ProtoFifoServiceObject_pT pFifoSrvObj = (_IOC_ProtoFifoServiceObject_pT)pSrvObj->pProtoPriv;

    do {
        pthread_mutex_lock(&pFifoSrvObj->WaitAccptedMutex);
        if (NULL != pFifoSrvObj->pConnLinkObj) {
            _IOC_ProtoFifoLinkObject_pT pConnFifoLinkObj =
                (_IOC_ProtoFifoLinkObject_pT)pFifoSrvObj->pConnLinkObj->pProtoPriv;

            pAceptedFifoLinkObj->pPeer = pConnFifoLinkObj;
            pConnFifoLinkObj->pPeer = pAceptedFifoLinkObj;

            pFifoSrvObj->pConnLinkObj = NULL;  // clear the connection link object

            pthread_cond_signal(&pFifoSrvObj->WaitAccptedCond);
            pthread_mutex_unlock(&pFifoSrvObj->WaitAccptedMutex);
            break;  // ACCEPTed the new incoming connection
        } else {
            pthread_mutex_unlock(&pFifoSrvObj->WaitAccptedMutex);
        }

        pthread_mutex_lock(&pFifoSrvObj->WaitNewConnMutex);
        // wait for the new incoming connection with 10ms timeout
        struct timespec TimeOut = {0};
        clock_gettime(CLOCK_REALTIME, &TimeOut);
        TimeOut.tv_nsec += 10 * 1000 * 1000;  // 10ms
        pthread_cond_timedwait(&pFifoSrvObj->WaitNewConnCond, &pFifoSrvObj->WaitNewConnMutex, &TimeOut);
        pthread_mutex_unlock(&pFifoSrvObj->WaitNewConnMutex);

    } while (0x20241124);

    //_IOC_LogNotTested();
    return IOC_RESULT_SUCCESS;  // NOTHING DONE, JUST RETURN SUCCESS
}
/**
 * @brief Closes the protocol FIFO link.
 *
 * This function safely closes a protocol FIFO link by handling the associated peer.
 * It locks the necessary mutexes to ensure thread safety while updating the link's peer reference.
 *
 * @param pLinkObj Pointer to the link object to be closed.
 * @return IOC_RESULT_SUCCESS on successful closure.
 */

static IOC_Result_T __IOC_closeLink_ofProtoFifo(_IOC_LinkObject_pT pLinkObj) {
    _IOC_ProtoFifoLinkObject_pT pFifoLinkObj = (_IOC_ProtoFifoLinkObject_pT)pLinkObj->pProtoPriv;

    pthread_mutex_lock(&pFifoLinkObj->Mutex);
    if (NULL != pFifoLinkObj->pPeer) {
        _IOC_ProtoFifoLinkObject_pT pPeerFifoLinkObj = pFifoLinkObj->pPeer;
        pFifoLinkObj->pPeer = NULL;  // clear the peer

        int LockResult = pthread_mutex_trylock(&pPeerFifoLinkObj->Mutex);
        if (LockResult == (0) /*LockSuccess*/) {
            pPeerFifoLinkObj->pPeer = NULL;  // clear the peer's peer
            pthread_mutex_unlock(&pPeerFifoLinkObj->Mutex);
        }

        pthread_mutex_unlock(&pFifoLinkObj->Mutex);
    } else {
        pthread_mutex_unlock(&pFifoLinkObj->Mutex);
    }

    _IOC_LogAssert(NULL == pFifoLinkObj->pPeer);
    free(pFifoLinkObj);

    //_IOC_LogNotTested();
    return IOC_RESULT_SUCCESS;  // NOTHING DONE, JUST RETURN SUCCESS
}

/**
 * @brief Handles sub-events for the Proto FIFO link object.
 *
 * This function processes sub-event arguments by copying them into the
 * Proto FIFO link object's internal state. It allocates memory for
 * event IDs based on the number of events specified and copies the
 * provided event IDs into the allocated space.
 *
 * @param pLinkObj Pointer to the link object containing protocol-specific data.
 * @param pSubEvtArgs Pointer to the sub-event arguments containing event details.
 * @return IOC_Result_T Returns IOC_RESULT_SUCCESS on successful execution.
 */
static IOC_Result_T __IOC_subEvt_ofProtoFifo(_IOC_LinkObject_pT pLinkObj, const IOC_SubEvtArgs_pT pSubEvtArgs) {
    _IOC_ProtoFifoLinkObject_pT pLinkFifoObj = (_IOC_ProtoFifoLinkObject_pT)pLinkObj->pProtoPriv;

    memcpy(&pLinkFifoObj->SubEvtArgs, pSubEvtArgs, sizeof(IOC_SubEvtArgs_T));

    pLinkFifoObj->SubEvtArgs.pEvtIDs = (IOC_EvtID_T *)malloc(pSubEvtArgs->EvtNum * sizeof(IOC_EvtID_T));
    memcpy(pLinkFifoObj->SubEvtArgs.pEvtIDs, pSubEvtArgs->pEvtIDs, pSubEvtArgs->EvtNum * sizeof(IOC_EvtID_T));

    //_IOC_LogNotTested();
    return IOC_RESULT_SUCCESS;
}

/**
 * @brief Unsubscribes an event from the protocol FIFO associated with the given link object.
 *
 * This function compares the provided callback function and private data with the currently
 * subscribed event arguments in the protocol FIFO link object. If a match is found, it frees
 * the allocated event IDs, resets the subscription arguments, and returns a success result.
 * If no matching subscription is found, it logs the event and returns a not exist result.
 *
 * @param pLinkObj       Pointer to the link object containing the protocol FIFO.
 * @param pUnsubEvtArgs  Pointer to the unsubscribe event arguments containing the callback
 *                       function and private data to be removed.
 *
 * @return IOC_RESULT_SUCCESS       If the unsubscription was successful.
 * @return IOC_RESULT_NOT_EXIST    If the specified event subscription does not exist.
 */
static IOC_Result_T __IOC_unsubEvt_ofProtoFifo(_IOC_LinkObject_pT pLinkObj, const IOC_UnsubEvtArgs_pT pUnsubEvtArgs) {
    _IOC_ProtoFifoLinkObject_pT pLinkFifoObj = (_IOC_ProtoFifoLinkObject_pT)pLinkObj->pProtoPriv;

    if (pLinkFifoObj->SubEvtArgs.CbProcEvt_F == pUnsubEvtArgs->CbProcEvt_F &&
        pLinkFifoObj->SubEvtArgs.pCbPrivData == pUnsubEvtArgs->pCbPrivData) {
        free(pLinkFifoObj->SubEvtArgs.pEvtIDs);
        memset(&pLinkFifoObj->SubEvtArgs, 0, sizeof(IOC_SubEvtArgs_T));

        //_IOC_LogNotTested();
        return IOC_RESULT_SUCCESS;
    }

    _IOC_LogNotTested();
    return IOC_RESULT_NOT_EXIST;
}
/**
 * @brief Posts an event to the Protocol FIFO.
 *
 * This function handles the posting of an event to the protocol FIFO by locking the necessary mutexes,
 * checking for subscribed event consumers, and invoking their callback functions if the event ID matches.
 *
 * @param pLinkObj Pointer to the link object associated with the Protocol FIFO.
 * @param pEvtDesc Pointer to the event descriptor containing event details.
 * @param pOption Pointer to the options for event processing.
 *
 * @return IOC_Result_T
 *         - IOC_RESULT_SUCCESS: Event was successfully processed by at least one consumer.
 *         - IOC_RESULT_NO_EVENT_CONSUMER: No consumers were available for the event.
 *         - IOC_RESULT_BUG: An unexpected error occurred.
 */

static IOC_Result_T __IOC_postEvt_ofProtoFifo(_IOC_LinkObject_pT pLinkObj, const IOC_EvtDesc_pT pEvtDesc,
                                              const IOC_Options_pT pOption) {
    _IOC_ProtoFifoLinkObject_pT pLocalFifoLinkObj = (_IOC_ProtoFifoLinkObject_pT)pLinkObj->pProtoPriv;
    IOC_Result_T Result = IOC_RESULT_BUG;
    int ProcEvtSuberCnt = 0;

    pthread_mutex_lock(&pLocalFifoLinkObj->Mutex);
    _IOC_ProtoFifoLinkObject_pT pPeerFifoLinkObj = pLocalFifoLinkObj->pPeer;

    if (NULL != pPeerFifoLinkObj) {
        pthread_mutex_lock(&pPeerFifoLinkObj->Mutex);
        IOC_CbProcEvt_F CbProcEvt_F = pPeerFifoLinkObj->SubEvtArgs.CbProcEvt_F;

        if (NULL != CbProcEvt_F) {
            for (int i = 0; i < pPeerFifoLinkObj->SubEvtArgs.EvtNum; i++) {
                if (pEvtDesc->EvtID == pPeerFifoLinkObj->SubEvtArgs.pEvtIDs[i]) {
                    CbProcEvt_F(pEvtDesc, pPeerFifoLinkObj->SubEvtArgs.pCbPrivData);
                    ProcEvtSuberCnt++;
                }
            }
        }
        pthread_mutex_unlock(&pPeerFifoLinkObj->Mutex);
    }
    pthread_mutex_unlock(&pLocalFifoLinkObj->Mutex);

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