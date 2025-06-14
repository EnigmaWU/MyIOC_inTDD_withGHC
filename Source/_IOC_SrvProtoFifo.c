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

    // 📦 WHY ADD DAT SUPPORT: ProtoFifo previously only supported EVT (events), but the
    // framework promised DAT (data transfer) capability. Without this structure, DAT
    // receiver callbacks couldn't be stored per-link, causing all data to be lost.
    //
    // 🔗 DESIGN CHOICE: Store DAT receiver info in each LinkObject instead of globally
    // because each link connection might have different receiver configurations:
    // - Link A: Uses callback mode with custom private data
    // - Link B: Uses polling mode (future feature)
    // - Link C: No DAT capability at all
    //
    // 🎯 THREAD SAFETY: Protected by the existing pLinkObj->Mutex to ensure
    // callback registration and data transmission are atomic operations.
    struct {
        IOC_CbRecvDat_F CbRecvDat_F;  // Callback for receiving data
        void *pCbPrivData;            // Private data for callback
        bool IsReceiverRegistered;    // Whether this link has a receiver callback

        // 📦 POLLING MODE SUPPORT: Buffer for storing data when no callback is registered
        // This enables hybrid mode - data can be delivered via callback OR stored for polling
        struct {
            char *pDataBuffer;                 // Circular buffer for received data
            size_t BufferSize;                 // Total buffer size (allocated)
            size_t DataStart;                  // Start position of valid data (read pointer)
            size_t DataEnd;                    // End position of valid data (write pointer)
            size_t AvailableData;              // Amount of valid data in buffer
            pthread_cond_t DataAvailableCond;  // Condition variable for blocking reads
            bool IsPollingMode;                // True if receiver is in polling mode (no callback)
        } PollingBuffer;
    } DatReceiver;
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
#define _PROTO_FIFO_POLLING_BUFFER_SIZE (64 * 1024)  // 64KB buffer for polling mode
static _IOC_ProtoFifoServiceObject_pT _mIOC_OnlinedSrvProtoFifoObjs[_MAX_PROTO_FIFO_SERVICES] = {};
static pthread_mutex_t _mIOC_OnlinedSrvProtoFifoObjsMutex = PTHREAD_MUTEX_INITIALIZER;

// Forward declarations for DAT support functions
static IOC_Result_T __IOC_setupDatReceiver_ofProtoFifo(_IOC_LinkObject_pT pLinkObj, _IOC_ServiceObject_pT pSrvObj);
static IOC_Result_T __IOC_initPollingBuffer_ofProtoFifo(_IOC_ProtoFifoLinkObject_pT pFifoLinkObj);
static void __IOC_cleanupPollingBuffer_ofProtoFifo(_IOC_ProtoFifoLinkObject_pT pFifoLinkObj);
static IOC_Result_T __IOC_storeDataInPollingBuffer(_IOC_ProtoFifoLinkObject_pT pFifoLinkObj, const void *pData,
                                                   size_t DataSize);
static IOC_Result_T __IOC_readDataFromPollingBuffer(_IOC_ProtoFifoLinkObject_pT pFifoLinkObj, void *pBuffer,
                                                    size_t BufferSize, size_t *pBytesRead, bool IsBlocking);

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
        return IOC_RESULT_NOT_EXIST_SERVICE;
    }

    // Step-2.5: Note the service mode (broadcast auto-accept vs manual accept)
    // Both modes allow connections, but they handle acceptance differently
    bool IsAutoAcceptMode = (pFifoSrvObj->pSrvObj->Args.Flags & IOC_SRVFLAG_BROADCAST_EVENT);
    if (!IsAutoAcceptMode) {
        _IOC_LogInfo("Service in manual accept mode, connection will wait for manual accept");
    }

    // Step-3: Create a FifoLinkObj
    _IOC_ProtoFifoLinkObject_pT pFifoLinkObj = calloc(1, sizeof(_IOC_ProtoFifoLinkObject_T));
    if (NULL == pFifoLinkObj) {
        _IOC_LogBug("Failed to alloc a new FifoLinkObj when connect service");
        _IOC_LogNotTested();
        return IOC_RESULT_POSIX_ENOMEM;
    }

    pthread_mutex_init(&pFifoLinkObj->Mutex, NULL);

    // Initialize polling buffer for potential DAT operations
    IOC_Result_T BufferResult = __IOC_initPollingBuffer_ofProtoFifo(pFifoLinkObj);
    if (BufferResult != IOC_RESULT_SUCCESS) {
        free(pFifoLinkObj);
        _IOC_LogBug("Failed to initialize polling buffer for FifoLinkObj");
        return BufferResult;
    }

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

    // Initialize polling buffer for potential DAT operations
    IOC_Result_T BufferResult = __IOC_initPollingBuffer_ofProtoFifo(pAceptedFifoLinkObj);
    if (BufferResult != IOC_RESULT_SUCCESS) {
        free(pAceptedFifoLinkObj);
        _IOC_LogBug("Failed to initialize polling buffer for accepted FifoLinkObj");
        return BufferResult;
    }

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

            // 🔧 WHY SETUP DAT RECEIVER HERE: This is the critical moment when server-side
            // link is fully established and peer connection is ready. We need to transfer
            // DAT receiver configuration from service to the newly accepted link.
            //
            // 🎯 TIMING IS CRUCIAL: Must happen AFTER peer link setup but BEFORE signaling
            // connection acceptance. This ensures that when the client-side gets the
            // connection confirmation, the server-side is already ready to receive data.
            //
            // 💡 WHY NOT IN CONNECT: Client side doesn't know service's DAT configuration,
            // only server side (which called IOC_onlineService) has the receiver callbacks.
            __IOC_setupDatReceiver_ofProtoFifo(pLinkObj, pSrvObj);

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

    // Clean up polling buffer before freeing the link object
    __IOC_cleanupPollingBuffer_ofProtoFifo(pFifoLinkObj);

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

//=================================================================================================
// DAT (Data Transfer) Operations for ProtoFifo
//=================================================================================================

/**
 * @brief Send data through ProtoFifo protocol
 * @param pLinkObj Pointer to the link object for data transmission
 * @param pDatDesc Pointer to data description containing payload
 * @param pOption Optional parameters (can be NULL)
 * @return IOC_RESULT_SUCCESS on successful data transmission
 */
static IOC_Result_T __IOC_sendData_ofProtoFifo(_IOC_LinkObject_pT pLinkObj, const IOC_DatDesc_pT pDatDesc,
                                               const IOC_Options_pT pOption) {
    if (!pLinkObj || !pDatDesc) {
        return IOC_RESULT_INVALID_PARAM;
    }

    _IOC_ProtoFifoLinkObject_pT pLocalFifoLinkObj = (_IOC_ProtoFifoLinkObject_pT)pLinkObj->pProtoPriv;
    if (!pLocalFifoLinkObj) {
        return IOC_RESULT_NOT_EXIST_LINK;
    }

    pthread_mutex_lock(&pLocalFifoLinkObj->Mutex);
    _IOC_ProtoFifoLinkObject_pT pPeerFifoLinkObj = pLocalFifoLinkObj->pPeer;

    if (!pPeerFifoLinkObj) {
        pthread_mutex_unlock(&pLocalFifoLinkObj->Mutex);
        return IOC_RESULT_NOT_EXIST_LINK;
    }

    // 🔒 WHY DUAL MUTEX LOCKING: We need to access peer's callback info safely.
    // Lock sequence: Local first, then Peer to prevent deadlock (consistent ordering).
    // We copy callback info under mutex protection, then release locks before
    // calling the callback to avoid holding locks during user code execution.
    pthread_mutex_lock(&pPeerFifoLinkObj->Mutex);
    IOC_CbRecvDat_F CbRecvDat_F = pPeerFifoLinkObj->DatReceiver.CbRecvDat_F;
    void *pCbPrivData = pPeerFifoLinkObj->DatReceiver.pCbPrivData;
    bool IsReceiverRegistered = pPeerFifoLinkObj->DatReceiver.IsReceiverRegistered;
    pthread_mutex_unlock(&pPeerFifoLinkObj->Mutex);

    pthread_mutex_unlock(&pLocalFifoLinkObj->Mutex);

    // ⚡ WHY IMMEDIATE DELIVERY: ProtoFifo implements zero-latency, zero-copy data transfer.
    // Unlike network protocols that buffer data, FIFO delivers directly to receiver callback.
    // This achieves maximum performance for intra-process communication.
    //
    // 🎯 DESIGN RATIONALE:
    // - No intermediate buffering = lower memory usage
    // - Direct callback = minimal latency
    // - Synchronous execution = simpler error handling
    // - Peer link pattern = bidirectional communication support
    if (IsReceiverRegistered && CbRecvDat_F) {
        // Callback mode: deliver data immediately via callback
        IOC_Result_T CallbackResult =
            CbRecvDat_F(pLinkObj->ID, pDatDesc->Payload.pData, pDatDesc->Payload.PtrDataSize, pCbPrivData);
        return CallbackResult;
    } else {
        // 📦 POLLING MODE SUPPORT: If no callback is registered, store data in polling buffer
        // This enables hybrid operation where some links use callbacks and others use polling
        pPeerFifoLinkObj->DatReceiver.PollingBuffer.IsPollingMode = true;

        IOC_Result_T StoreResult =
            __IOC_storeDataInPollingBuffer(pPeerFifoLinkObj, pDatDesc->Payload.pData, pDatDesc->Payload.PtrDataSize);

        if (StoreResult == IOC_RESULT_SUCCESS) {
            return IOC_RESULT_SUCCESS;
        } else if (StoreResult == IOC_RESULT_BUFFER_FULL) {
            // Buffer is full, this could be treated as back-pressure
            return IOC_RESULT_BUFFER_FULL;
        } else {
            return StoreResult;
        }
    }
}

/**
 * @brief Receive data through ProtoFifo protocol (polling mode)
 * @param pLinkObj Pointer to the link object for data reception
 * @param pDatDesc Pointer to data description buffer
 * @param pOption Optional parameters (can be NULL)
 * @return IOC_RESULT_SUCCESS on successful data reception
 */
static IOC_Result_T __IOC_recvData_ofProtoFifo(_IOC_LinkObject_pT pLinkObj, IOC_DatDesc_pT pDatDesc,
                                               const IOC_Options_pT pOption) {
    if (!pLinkObj || !pDatDesc) {
        return IOC_RESULT_INVALID_PARAM;
    }

    _IOC_ProtoFifoLinkObject_pT pFifoLinkObj = (_IOC_ProtoFifoLinkObject_pT)pLinkObj->pProtoPriv;
    if (!pFifoLinkObj) {
        return IOC_RESULT_NOT_EXIST_LINK;
    }

    // 🔒 THREAD SAFETY: Lock the link object mutex for polling buffer access
    pthread_mutex_lock(&pFifoLinkObj->Mutex);

    // 📊 CHECK POLLING MODE: Only proceed if this link is configured for polling
    // Links with callbacks should use callback delivery, not polling
    if (!pFifoLinkObj->DatReceiver.PollingBuffer.IsPollingMode) {
        pthread_mutex_unlock(&pFifoLinkObj->Mutex);
        return IOC_RESULT_NOT_SUPPORT;
    }

    // 🎛️ DETERMINE BLOCKING MODE: Check if options specify blocking or non-blocking
    // Non-blocking mode is indicated by TIMEOUT option with TimeoutUS = 0
    bool IsBlocking = true;  // Default to blocking
    if (pOption && (pOption->IDs & IOC_OPTID_TIMEOUT) && (pOption->Payload.TimeoutUS == 0)) {
        IsBlocking = false;
    }

    // 📦 READ DATA FROM POLLING BUFFER
    size_t BytesRead = 0;
    IOC_Result_T ReadResult = __IOC_readDataFromPollingBuffer(pFifoLinkObj, pDatDesc->Payload.pData,
                                                              pDatDesc->Payload.PtrDataSize, &BytesRead, IsBlocking);

    pthread_mutex_unlock(&pFifoLinkObj->Mutex);

    if (ReadResult == IOC_RESULT_SUCCESS) {
        // Update the data descriptor with actual bytes read
        pDatDesc->Payload.PtrDataSize = BytesRead;
        pDatDesc->Status = IOC_DAT_STATUS_STREAM_READY;
        pDatDesc->Result = IOC_RESULT_SUCCESS;

        printf("IOC_recvDAT: Received %zu bytes from polling buffer on LinkID=%llu\n", BytesRead, pLinkObj->ID);
    } else if (ReadResult == IOC_RESULT_NO_DATA) {
        // No data available in non-blocking mode
        pDatDesc->Payload.PtrDataSize = 0;
        pDatDesc->Status = IOC_DAT_STATUS_STREAM_READY;
        pDatDesc->Result = IOC_RESULT_NO_DATA;
    } else {
        // Other error
        pDatDesc->Status = IOC_DAT_STATUS_STREAM_ERROR;
        pDatDesc->Result = ReadResult;
    }

    return ReadResult;
}

/**
 * @brief Setup DAT receiver callback for a link
 * @param pLinkObj Pointer to the link object
 * @param pSrvObj Pointer to the service object containing DAT usage args
 * @return IOC_RESULT_SUCCESS on successful callback registration
 */
static IOC_Result_T __IOC_setupDatReceiver_ofProtoFifo(_IOC_LinkObject_pT pLinkObj, _IOC_ServiceObject_pT pSrvObj) {
    if (!pLinkObj || !pSrvObj) {
        return IOC_RESULT_INVALID_PARAM;
    }

    _IOC_ProtoFifoLinkObject_pT pFifoLinkObj = (_IOC_ProtoFifoLinkObject_pT)pLinkObj->pProtoPriv;
    if (!pFifoLinkObj) {
        return IOC_RESULT_NOT_EXIST_LINK;
    }

    // 🔍 WHY CAPABILITY CHECK: Not all services support DAT operations. We need to verify:
    // 1. Service declares IOC_LinkUsageDatReceiver capability
    // 2. Service provides actual DAT usage arguments (callback functions)
    // This prevents runtime errors when trying to setup DAT on non-DAT services.
    if ((pSrvObj->Args.UsageCapabilites & IOC_LinkUsageDatReceiver) && pSrvObj->Args.UsageArgs.pDat) {
        pthread_mutex_lock(&pFifoLinkObj->Mutex);

        // 📋 WHY COPY CALLBACK INFO: Extract DAT receiver configuration from service
        // and store it in the link object. This allows each link to have independent
        // DAT receiver settings, supporting scenarios where one service handles
        // multiple client connections with different callback configurations.
        pFifoLinkObj->DatReceiver.CbRecvDat_F = pSrvObj->Args.UsageArgs.pDat->CbRecvDat_F;
        pFifoLinkObj->DatReceiver.pCbPrivData = pSrvObj->Args.UsageArgs.pDat->pCbPrivData;
        pFifoLinkObj->DatReceiver.IsReceiverRegistered = (pFifoLinkObj->DatReceiver.CbRecvDat_F != NULL);

        pthread_mutex_unlock(&pFifoLinkObj->Mutex);

        return IOC_RESULT_SUCCESS;
    }

    return IOC_RESULT_NOT_SUPPORT;
}

/**
 * @brief Initialize polling buffer for a ProtoFifo link object
 * @param pFifoLinkObj Pointer to the ProtoFifo link object
 * @return IOC_RESULT_SUCCESS on success, IOC_RESULT_POSIX_ENOMEM on memory allocation failure
 */
static IOC_Result_T __IOC_initPollingBuffer_ofProtoFifo(_IOC_ProtoFifoLinkObject_pT pFifoLinkObj) {
    if (!pFifoLinkObj) {
        return IOC_RESULT_INVALID_PARAM;
    }

    // Initialize polling buffer for potential use
    pFifoLinkObj->DatReceiver.PollingBuffer.pDataBuffer = malloc(_PROTO_FIFO_POLLING_BUFFER_SIZE);
    if (!pFifoLinkObj->DatReceiver.PollingBuffer.pDataBuffer) {
        return IOC_RESULT_POSIX_ENOMEM;
    }

    pFifoLinkObj->DatReceiver.PollingBuffer.BufferSize = _PROTO_FIFO_POLLING_BUFFER_SIZE;
    pFifoLinkObj->DatReceiver.PollingBuffer.DataStart = 0;
    pFifoLinkObj->DatReceiver.PollingBuffer.DataEnd = 0;
    pFifoLinkObj->DatReceiver.PollingBuffer.AvailableData = 0;
    pFifoLinkObj->DatReceiver.PollingBuffer.IsPollingMode = false;  // Default to callback mode

    pthread_cond_init(&pFifoLinkObj->DatReceiver.PollingBuffer.DataAvailableCond, NULL);

    return IOC_RESULT_SUCCESS;
}

/**
 * @brief Clean up polling buffer for a ProtoFifo link object
 * @param pFifoLinkObj Pointer to the ProtoFifo link object
 */
static void __IOC_cleanupPollingBuffer_ofProtoFifo(_IOC_ProtoFifoLinkObject_pT pFifoLinkObj) {
    if (!pFifoLinkObj) {
        return;
    }

    if (pFifoLinkObj->DatReceiver.PollingBuffer.pDataBuffer) {
        free(pFifoLinkObj->DatReceiver.PollingBuffer.pDataBuffer);
        pFifoLinkObj->DatReceiver.PollingBuffer.pDataBuffer = NULL;
    }

    pthread_cond_destroy(&pFifoLinkObj->DatReceiver.PollingBuffer.DataAvailableCond);
}

/**
 * @brief Store data in polling buffer (circular buffer implementation)
 * @param pFifoLinkObj Pointer to the ProtoFifo link object
 * @param pData Pointer to data to store
 * @param DataSize Size of data to store
 * @return IOC_RESULT_SUCCESS on success, IOC_RESULT_BUFFER_FULL if buffer is full
 */
static IOC_Result_T __IOC_storeDataInPollingBuffer(_IOC_ProtoFifoLinkObject_pT pFifoLinkObj, const void *pData,
                                                   size_t DataSize) {
    if (!pFifoLinkObj || !pData || DataSize == 0) {
        return IOC_RESULT_INVALID_PARAM;
    }

    // Get direct reference to the polling buffer
    char *pDataBuffer = pFifoLinkObj->DatReceiver.PollingBuffer.pDataBuffer;
    size_t BufferSize = pFifoLinkObj->DatReceiver.PollingBuffer.BufferSize;
    size_t DataEnd = pFifoLinkObj->DatReceiver.PollingBuffer.DataEnd;
    size_t AvailableData = pFifoLinkObj->DatReceiver.PollingBuffer.AvailableData;

    // Check if there's enough space in the buffer
    size_t FreeSpace = BufferSize - AvailableData;
    if (DataSize > FreeSpace) {
        return IOC_RESULT_BUFFER_FULL;
    }

    // Store data in circular buffer
    const char *pSrcData = (const char *)pData;
    size_t BytesToEnd = BufferSize - DataEnd;

    if (DataSize <= BytesToEnd) {
        // Data fits without wrapping
        memcpy(pDataBuffer + DataEnd, pSrcData, DataSize);
        pFifoLinkObj->DatReceiver.PollingBuffer.DataEnd = (DataEnd + DataSize) % BufferSize;
    } else {
        // Data needs to wrap around
        memcpy(pDataBuffer + DataEnd, pSrcData, BytesToEnd);
        memcpy(pDataBuffer, pSrcData + BytesToEnd, DataSize - BytesToEnd);
        pFifoLinkObj->DatReceiver.PollingBuffer.DataEnd = DataSize - BytesToEnd;
    }

    pFifoLinkObj->DatReceiver.PollingBuffer.AvailableData += DataSize;

    // Signal that data is available for polling
    pthread_cond_signal(&pFifoLinkObj->DatReceiver.PollingBuffer.DataAvailableCond);

    return IOC_RESULT_SUCCESS;
}

/**
 * @brief Read data from polling buffer (circular buffer implementation)
 * @param pFifoLinkObj Pointer to the ProtoFifo link object
 * @param pBuffer Destination buffer to store read data
 * @param BufferSize Size of destination buffer
 * @param pBytesRead Pointer to store actual bytes read
 * @param IsBlocking Whether to block when no data is available
 * @return IOC_RESULT_SUCCESS on success, IOC_RESULT_NO_DATA if no data available in non-blocking mode
 */
static IOC_Result_T __IOC_readDataFromPollingBuffer(_IOC_ProtoFifoLinkObject_pT pFifoLinkObj, void *pBuffer,
                                                    size_t BufferSize, size_t *pBytesRead, bool IsBlocking) {
    if (!pFifoLinkObj || !pBuffer || !pBytesRead || BufferSize == 0) {
        return IOC_RESULT_INVALID_PARAM;
    }

    *pBytesRead = 0;

    // Wait for data if blocking mode and no data available
    if (IsBlocking) {
        while (pFifoLinkObj->DatReceiver.PollingBuffer.AvailableData == 0) {
            pthread_cond_wait(&pFifoLinkObj->DatReceiver.PollingBuffer.DataAvailableCond, &pFifoLinkObj->Mutex);
        }
    } else if (pFifoLinkObj->DatReceiver.PollingBuffer.AvailableData == 0) {
        return IOC_RESULT_NO_DATA;
    }

    // Read data from circular buffer
    size_t AvailableData = pFifoLinkObj->DatReceiver.PollingBuffer.AvailableData;
    size_t DataToRead = (BufferSize < AvailableData) ? BufferSize : AvailableData;

    char *pSrcBuffer = pFifoLinkObj->DatReceiver.PollingBuffer.pDataBuffer;
    size_t DataStart = pFifoLinkObj->DatReceiver.PollingBuffer.DataStart;
    size_t TotalBufferSize = pFifoLinkObj->DatReceiver.PollingBuffer.BufferSize;

    char *pDestBuffer = (char *)pBuffer;
    size_t BytesToEnd = TotalBufferSize - DataStart;

    if (DataToRead <= BytesToEnd) {
        // Data doesn't wrap around
        memcpy(pDestBuffer, pSrcBuffer + DataStart, DataToRead);
        pFifoLinkObj->DatReceiver.PollingBuffer.DataStart = (DataStart + DataToRead) % TotalBufferSize;
    } else {
        // Data wraps around
        memcpy(pDestBuffer, pSrcBuffer + DataStart, BytesToEnd);
        memcpy(pDestBuffer + BytesToEnd, pSrcBuffer, DataToRead - BytesToEnd);
        pFifoLinkObj->DatReceiver.PollingBuffer.DataStart = DataToRead - BytesToEnd;
    }

    pFifoLinkObj->DatReceiver.PollingBuffer.AvailableData -= DataToRead;
    *pBytesRead = DataToRead;

    return IOC_RESULT_SUCCESS;
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

    // 🚀 WHY ADD DAT METHODS: Completing the ProtoFifo protocol implementation to support
    // all three communication paradigms promised by the IOC framework:
    // ✅ EVT (Events): Publisher/Subscriber - already implemented
    // ✅ CMD (Commands): Request/Response - TODO (future implementation)
    // ✅ DAT (Data): Stream/Transfer - NOW IMPLEMENTED
    //
    // 🎯 IMPLEMENTATION STRATEGY: ProtoFifo optimizes for intra-process communication:
    // - OpSendData_F: Zero-copy, immediate callback delivery
    // - OpRecvData_F: Not supported (polling conflicts with callback design)
    //
    // 📈 PERFORMANCE BENEFITS: Direct memory access, no serialization, no buffering
    .OpSendData_F = __IOC_sendData_ofProtoFifo,
    .OpRecvData_F = __IOC_recvData_ofProtoFifo,
};