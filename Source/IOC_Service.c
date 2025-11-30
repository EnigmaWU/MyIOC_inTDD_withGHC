#include <pthread.h>

#include "IOC/IOC_EvtAPI.h"  // For IOC_subEVT auto-subscription
#include "_IOC.h"

//=================================================================================================
static _IOC_ServiceObject_pT _mIOC_SrvObjTbl[_MAX_IOC_SRV_OBJ_NUM] = {};
static pthread_mutex_t _mIOC_SrvObjTblMutex = PTHREAD_MUTEX_INITIALIZER;
static inline void ___IOC_lockSrvObjTbl(void) { pthread_mutex_lock(&_mIOC_SrvObjTblMutex); }
static inline void ___IOC_unlockSrvObjTbl(void) { pthread_mutex_unlock(&_mIOC_SrvObjTblMutex); }

// Unit-test helpers: allocation-failure injection and counters
static pthread_mutex_t _mIOC_TestHooksMutex = PTHREAD_MUTEX_INITIALIZER;
static int _mIOC_FailNextAllocCount = 0;  // when >0, next allocations will fail (decremented)
static uint16_t _mIOC_ServiceCount = 0;   // active services
static uint16_t _mIOC_LinkCount = 0;      // active link objects

void IOC_test_setFailNextAlloc(int count) {
    pthread_mutex_lock(&_mIOC_TestHooksMutex);
    _mIOC_FailNextAllocCount = count;
    pthread_mutex_unlock(&_mIOC_TestHooksMutex);
}

uint16_t IOC_getServiceCount(void) {
    uint16_t v;
    pthread_mutex_lock(&_mIOC_TestHooksMutex);
    v = _mIOC_ServiceCount;
    pthread_mutex_unlock(&_mIOC_TestHooksMutex);
    return v;
}

uint16_t IOC_getLinkCount(void) {
    uint16_t v;
    pthread_mutex_lock(&_mIOC_TestHooksMutex);
    v = _mIOC_LinkCount;
    pthread_mutex_unlock(&_mIOC_TestHooksMutex);
    return v;
}

// Internal allocation helpers that respect the fail-injection counter
static void* __IOC_test_calloc(size_t nmemb, size_t size) {
    void* p = NULL;
    pthread_mutex_lock(&_mIOC_TestHooksMutex);
    if (_mIOC_FailNextAllocCount > 0) {
        _mIOC_FailNextAllocCount--;
        pthread_mutex_unlock(&_mIOC_TestHooksMutex);
        return NULL;  // Simulate ENOMEM
    }
    pthread_mutex_unlock(&_mIOC_TestHooksMutex);
    p = calloc(nmemb, size);
    return p;
}

static inline IOC_BoolResult_T ___IOC_isSrvObjConflicted(IOC_SrvArgs_pT pArgsNew) {
    if (NULL == pArgsNew) {
        return IOC_RESULT_NO;
    }

    for (int i = 0; i < _MAX_IOC_SRV_OBJ_NUM; ++i) {
        _IOC_ServiceObject_pT pExisting = _mIOC_SrvObjTbl[i];
        if (NULL == pExisting) {
            continue;
        }

        const IOC_SrvURI_T* pExistingURI = &pExisting->Args.SrvURI;
        const IOC_SrvURI_T* pNewURI = &pArgsNew->SrvURI;

        bool protoEqual = (pExistingURI->pProtocol && pNewURI->pProtocol)
                              ? (strcmp(pExistingURI->pProtocol, pNewURI->pProtocol) == 0)
                              : (pExistingURI->pProtocol == pNewURI->pProtocol);
        bool hostEqual = (pExistingURI->pHost && pNewURI->pHost) ? (strcmp(pExistingURI->pHost, pNewURI->pHost) == 0)
                                                                 : (pExistingURI->pHost == pNewURI->pHost);
        bool pathEqual = (pExistingURI->pPath && pNewURI->pPath) ? (strcmp(pExistingURI->pPath, pNewURI->pPath) == 0)
                                                                 : (pExistingURI->pPath == pNewURI->pPath);
        bool portEqual = (pExistingURI->Port == pNewURI->Port);

        if (protoEqual && hostEqual && pathEqual && portEqual) {
            // Treat identical URI as conflicting regardless of flags/capabilities to keep uniqueness.
            return IOC_RESULT_YES;
        }
    }

    return IOC_RESULT_NO;
}

/**
 * @brief alloc a service object by the given service arguments.
 *
 * @param pSrvArgs: the service arguments.
 * @param ppSrvObj: the pointer to save the new allocated service object.
 *
 * @return IOC_Result_T: the result of this operation.
 *    IOC_RESULT_SUCCESS
 *    IOC_RESULT_TOO_MANY_SERVICES
 *    IOC_RESULT_CONFLICT_SRVARGS
 */
static IOC_Result_T __IOC_allocSrvObj(/*ARG_INCONST*/ IOC_SrvArgs_pT pSrvArgs,
                                      /*ARG_OUT*/ _IOC_ServiceObject_pT* ppSrvObj) {
    IOC_Result_T Result = IOC_RESULT_BUG;
    _IOC_ServiceObject_pT pSrvObj = NULL;

    ___IOC_lockSrvObjTbl();
    if (IOC_RESULT_YES == ___IOC_isSrvObjConflicted(pSrvArgs)) {
        Result = IOC_RESULT_CONFLICT_SRVARGS;
        _IOC_LogWarn("Service conflict detected for URI(%s)",
                     IOC_Helper_printSingleLineSrvURI(&pSrvArgs->SrvURI, NULL, 0));
        goto _RetResult;
    }

    for (int i = 0; i < _MAX_IOC_SRV_OBJ_NUM; i++) {
        if (NULL == _mIOC_SrvObjTbl[i]) {
            pSrvObj = (_IOC_ServiceObject_pT)__IOC_test_calloc(1, sizeof(_IOC_ServiceObject_T));
            if (NULL == pSrvObj) {
                Result = IOC_RESULT_POSIX_ENOMEM;
                // NOTE: This path is now tested via IOC_test_setFailNextAlloc() in unit tests
                goto _RetResult;
            }

            // Initialize manual accept tracking mutex
            if (pthread_mutex_init(&pSrvObj->ManualAccept.Mutex, NULL) != 0) {
                free(pSrvObj);
                Result = IOC_RESULT_POSIX_ENOMEM;
                // NOTE: Mutex init failure is rare but handled defensively
                goto _RetResult;
            }

            // Initialize manual accept tracking array
            for (int j = 0; j < _MAX_MANUAL_ACCEPT_ACCEPTED_LINK_NUM; j++) {
                pSrvObj->ManualAccept.AcceptedLinkIDs[j] = IOC_ID_INVALID;
            }
            pSrvObj->ManualAccept.AcceptedLinkCount = 0;

            pSrvObj->ID = i;
            pSrvObj->Args.SrvURI.pProtocol = strdup(pSrvArgs->SrvURI.pProtocol);
            pSrvObj->Args.SrvURI.pHost = strdup(pSrvArgs->SrvURI.pHost);
            pSrvObj->Args.SrvURI.pPath = strdup(pSrvArgs->SrvURI.pPath);
            pSrvObj->Args.SrvURI.Port = pSrvArgs->SrvURI.Port;
            pSrvObj->Args.UsageCapabilites = pSrvArgs->UsageCapabilites;
            pSrvObj->Args.Flags = pSrvArgs->Flags;

            // 🚨 WHY CRITICAL FIX: The original code was missing this UsageArgs copy, causing
            // DAT callback functions (CbRecvDat_F, pCbPrivData) to be lost when service objects
            // were created. This led to test failures where DatReceiverPrivData.CallbackExecuted
            // remained false because the callback was never registered with the protocol layer.
            //
            // 🔍 ROOT CAUSE: IOC_onlineService() → __IOC_allocSrvObj() only copied basic service
            // parameters but ignored UsageArgs containing critical callback configurations.
            // Without this, protocol layers couldn't access DAT receiver callbacks.
            //
            // 💡 SOLUTION: Perform shallow copy of UsageArgs structure. This preserves all
            // callback function pointers and private data needed for protocol-specific operations.
            pSrvObj->Args.UsageArgs = pSrvArgs->UsageArgs;

            // Also copy service-level callback hooks and private data
            // so the auto-accept daemon can notify immediately on new links.
            pSrvObj->Args.OnAutoAccepted_F = pSrvArgs->OnAutoAccepted_F;
            pSrvObj->Args.pSrvPriv = pSrvArgs->pSrvPriv;

            _mIOC_SrvObjTbl[i] = pSrvObj;
            // update test counter
            pthread_mutex_lock(&_mIOC_TestHooksMutex);
            _mIOC_ServiceCount++;
            pthread_mutex_unlock(&_mIOC_TestHooksMutex);
            *ppSrvObj = pSrvObj;
            Result = IOC_RESULT_SUCCESS;
            //_IOC_LogNotTested();
            goto _RetResult;
        }
    }

    Result = IOC_RESULT_TOO_MANY_SERVICES;
    //_IOC_LogNotTested();

_RetResult:
    ___IOC_unlockSrvObjTbl();
    //_IOC_LogNotTested();
    return Result;
}

static void __IOC_freeSrvObj(_IOC_ServiceObject_pT pSrvObj) {
    _IOC_LogAssert((NULL != pSrvObj) && (pSrvObj->ID < _MAX_IOC_SRV_OBJ_NUM));

    ___IOC_lockSrvObjTbl();
    _mIOC_SrvObjTbl[pSrvObj->ID] = NULL;
    ___IOC_unlockSrvObjTbl();

    // Clean up manual accept tracking mutex
    pthread_mutex_destroy(&pSrvObj->ManualAccept.Mutex);

    free((char*)pSrvObj->Args.SrvURI.pProtocol);
    free((char*)pSrvObj->Args.SrvURI.pHost);
    free((char*)pSrvObj->Args.SrvURI.pPath);
    free(pSrvObj);
    pthread_mutex_lock(&_mIOC_TestHooksMutex);
    if (_mIOC_ServiceCount > 0) _mIOC_ServiceCount--;
    pthread_mutex_unlock(&_mIOC_TestHooksMutex);

    //_IOC_LogNotTested();
}

static _IOC_ServiceObject_pT __IOC_getSrvObjBySrvID(IOC_SrvID_T SrvID) {
    if (SrvID < _MAX_IOC_SRV_OBJ_NUM) {
        return _mIOC_SrvObjTbl[SrvID];
    } else {
        _IOC_LogError("Invalid SrvID=%" PRIu64 "", SrvID);
    }

    return NULL;
}

// TODO: __IOC_putSrvObj

//=================================================================================================
#define _MAX_IOC_LINK_OBJ_NUM 32  // Increased from 8 to 32 to support auto-accept daemon threads and concurrent tests
static _IOC_LinkObject_pT _mIOC_LinkObjTbl[_MAX_IOC_LINK_OBJ_NUM] = {};
static pthread_mutex_t _mIOC_LinkObjTblMutex = PTHREAD_MUTEX_INITIALIZER;
static inline void ___IOC_lockLinkObjTbl(void) { pthread_mutex_lock(&_mIOC_LinkObjTblMutex); }
static inline void ___IOC_unlockLinkObjTbl(void) { pthread_mutex_unlock(&_mIOC_LinkObjTblMutex); }

static inline IOC_LinkID_T ___IOC_convertLinkObjTblIdxToLinkID(int Idx) {
    return Idx + IOC_CONLES_MODE_AUTO_LINK_ID_MAX + 1;
}

static inline int ___IOC_convertLinkIDToLinkObjTblIdx(IOC_LinkID_T LinkID) {
    int TblIdx = LinkID - IOC_CONLES_MODE_AUTO_LINK_ID_MAX - 1;
    _IOC_LogAssert(TblIdx >= 0 && TblIdx < _MAX_IOC_LINK_OBJ_NUM);
    return TblIdx;
}

_IOC_LinkObject_pT __IOC_allocLinkObj(void) {
    _IOC_LinkObject_pT pLinkObj = NULL;

    ___IOC_lockLinkObjTbl();
    for (int i = 0; i < _MAX_IOC_LINK_OBJ_NUM; i++) {
        if (NULL == _mIOC_LinkObjTbl[i]) {
            pLinkObj = (_IOC_LinkObject_pT)__IOC_test_calloc(1, sizeof(_IOC_LinkObject_T));
            if (NULL == pLinkObj) {
                _IOC_LogError("Failed to alloc a link object");
                _IOC_LogNotTested();
                return NULL;
            }

            pLinkObj->ID = ___IOC_convertLinkObjTblIdxToLinkID(i);

            // 🎯 TDD IMPLEMENTATION: Initialize Connection state for IOC_getLinkConnState() support (Level 1)
            pLinkObj->ConnState.CurrentState = IOC_LinkConnStateDisconnected;  // Start disconnected
            if (pthread_mutex_init(&pLinkObj->ConnState.StateMutex, NULL) != 0) {
                _IOC_LogError("Failed to initialize Connection state mutex for LinkID=%llu", pLinkObj->ID);
                free(pLinkObj);
                return NULL;
            }
            pLinkObj->ConnState.IsConnected = false;
            pLinkObj->ConnState.LastStateChangeTime = time(NULL);

            // 🎯 TDD IMPLEMENTATION: Initialize DAT state for IOC_getLinkState() support
            pLinkObj->DatState.CurrentSubState = IOC_LinkSubStateDefault;
            if (pthread_mutex_init(&pLinkObj->DatState.SubStateMutex, NULL) != 0) {
                _IOC_LogError("Failed to initialize DAT substate mutex for LinkID=%llu", pLinkObj->ID);
                pthread_mutex_destroy(&pLinkObj->ConnState.StateMutex);
                free(pLinkObj);
                return NULL;
            }
            pLinkObj->DatState.IsSending = false;
            pLinkObj->DatState.IsReceiving = false;
            pLinkObj->DatState.LastOperationTime = time(NULL);

            // 🎯 TDD IMPLEMENTATION: Initialize CMD state for IOC_getLinkState() support (US-2)
            pLinkObj->CmdState.CurrentSubState = IOC_LinkSubStateDefault;
            if (pthread_mutex_init(&pLinkObj->CmdState.SubStateMutex, NULL) != 0) {
                _IOC_LogError("Failed to initialize CMD substate mutex for LinkID=%llu", pLinkObj->ID);
                pthread_mutex_destroy(&pLinkObj->DatState.SubStateMutex);
                free(pLinkObj);
                return NULL;
            }
            pLinkObj->CmdState.IsExecuting = false;
            pLinkObj->CmdState.IsWaiting = false;
            pLinkObj->CmdState.IsProcessing = false;
            pLinkObj->CmdState.LastOperationTime = time(NULL);

            _mIOC_LinkObjTbl[i] = pLinkObj;
            pthread_mutex_lock(&_mIOC_TestHooksMutex);
            _mIOC_LinkCount++;
            pthread_mutex_unlock(&_mIOC_TestHooksMutex);
            break;
        }
    }
    ___IOC_unlockLinkObjTbl();

    if (NULL == pLinkObj) {
        _IOC_LogError("Failed to alloc a link object in LinkObjTbl[%d]", _MAX_IOC_LINK_OBJ_NUM);
    }

    //_IOC_LogNotTested();
    return pLinkObj;
}

void __IOC_freeLinkObj(_IOC_LinkObject_pT pLinkObj) {
    ___IOC_lockLinkObjTbl();
    _mIOC_LinkObjTbl[___IOC_convertLinkIDToLinkObjTblIdx(pLinkObj->ID)] = NULL;
    ___IOC_unlockLinkObjTbl();

    // 🎯 TDD IMPLEMENTATION: Cleanup state mutexes
    pthread_mutex_destroy(&pLinkObj->ConnState.StateMutex);
    pthread_mutex_destroy(&pLinkObj->DatState.SubStateMutex);
    pthread_mutex_destroy(&pLinkObj->CmdState.SubStateMutex);

    free(pLinkObj);
    pthread_mutex_lock(&_mIOC_TestHooksMutex);
    if (_mIOC_LinkCount > 0) _mIOC_LinkCount--;
    pthread_mutex_unlock(&_mIOC_TestHooksMutex);
    //_IOC_LogNotTested();
}

_IOC_LinkObject_pT _IOC_getLinkObjByLinkID(IOC_LinkID_T LinkID) {
    // Validate LinkID range before conversion to avoid assertion failure
    if (LinkID == IOC_ID_INVALID) {
        return NULL;
    }

    // Check if LinkID is in valid range for our link object table
    int TblIdx = LinkID - IOC_CONLES_MODE_AUTO_LINK_ID_MAX - 1;
    if (TblIdx < 0 || TblIdx >= _MAX_IOC_LINK_OBJ_NUM) {
        return NULL;
    }

    return _mIOC_LinkObjTbl[TblIdx];
}

// TODO: __IOC_putLinkObj

//=================================================================================================
static IOC_BoolResult_T __IOC_isValidSrvArgs(const IOC_SrvArgs_pT pSrvArgs) {
    if (NULL == pSrvArgs) {
        return IOC_RESULT_NO;
    }

    if (NULL == pSrvArgs->SrvURI.pProtocol) {
        return IOC_RESULT_NO;
    }

    if (NULL == pSrvArgs->SrvURI.pHost) {
        return IOC_RESULT_NO;
    }

    if (NULL == pSrvArgs->SrvURI.pPath) {
        return IOC_RESULT_NO;
    }

    if (!(pSrvArgs->UsageCapabilites & IOC_LinkUsageMask)) {
        return IOC_RESULT_NO;
    }

    // TODO: check port if needed

    return IOC_RESULT_YES;
}

//_mIOC_XYZ is the global variable used intra-CURRENT_FILE submodule.
extern _IOC_SrvProtoMethods_T _gIOC_SrvProtoTCPMethods;
static _IOC_SrvProtoMethods_pT _mIOC_SrvProtoMethods[] = {
    &_gIOC_SrvProtoFifoMethods,
    &_gIOC_SrvProtoTCPMethods,
};

IOC_Result_T __IOC_onlineServiceByProto(_IOC_ServiceObject_pT pSrvObj) {
    IOC_Result_T OnlineResult = IOC_RESULT_BUG;
    IOC_Bool_T IsProtoAuto = !strcmp(pSrvObj->Args.SrvURI.pProtocol, IOC_SRV_PROTO_AUTO);

    if (IsProtoAuto) {
        int TryProtoIdx = 0;
        for (; TryProtoIdx < IOC_calcArrayElmtCnt(_mIOC_SrvProtoMethods); TryProtoIdx++) {
            OnlineResult = _mIOC_SrvProtoMethods[TryProtoIdx]->OpOnlineService_F(pSrvObj);
            if (IOC_RESULT_SUCCESS != OnlineResult) {
                _IOC_LogNotTested();
                break;
            }
        }

        // IF ANY PROTO FAIL, OFFLINE ALL ONLINEED PROTO
        if (IOC_RESULT_SUCCESS != OnlineResult) {
            for (int OffIdx = 0; OffIdx < TryProtoIdx; OffIdx++) {
                IOC_Result_T OfflineResult = _mIOC_SrvProtoMethods[OffIdx]->OpOfflineService_F(pSrvObj);
                if (IOC_RESULT_SUCCESS != OfflineResult) {
                    _IOC_LogBug("Failed to offline service by proto, Resuld=%d", OfflineResult);
                }
            }
            _IOC_LogNotTested();
        } else {
            _IOC_LogNotTested();
        }
    } else {
        for (int i = 0; i < IOC_calcArrayElmtCnt(_mIOC_SrvProtoMethods); i++) {
            if (!strcmp(pSrvObj->Args.SrvURI.pProtocol, _mIOC_SrvProtoMethods[i]->pProtocol)) {
                OnlineResult = _mIOC_SrvProtoMethods[i]->OpOnlineService_F(pSrvObj);
                if (IOC_RESULT_SUCCESS == OnlineResult) {
                    pSrvObj->pMethods = _mIOC_SrvProtoMethods[i];
                }
                break;
            }
        }

        // 🎯 TDD GREEN FIX: Return error if no matching protocol found
        if (pSrvObj->pMethods == NULL) {
            _IOC_LogWarn("Unsupported protocol: %s", pSrvObj->Args.SrvURI.pProtocol);
            return IOC_RESULT_NOT_SUPPORT;
        }
        //_IOC_LogNotTested();
    }

    //_IOC_LogNotTested();
    return OnlineResult;
}

/**
 * @brief The broadcast daemon thread function is created
 *  when the service is onlineed with IOC_SRVFLAG_BROADCAST_EVENT flag.
 * @param pArg: the pointer to the service object.
 *
 * @details
 *    1) auto accept incoming client connections.
 *    2) auto close the link when the client closed by peer.
 */
static void* __IOC_ServiceBroadcastDaemonThread(void* pArg) {
    _IOC_ServiceObject_pT pSrvObj = (_IOC_ServiceObject_pT)pArg;
    _IOC_LogAssert(NULL != pSrvObj);

    while (1) {
        _IOC_LinkObject_pT pLinkObj = __IOC_allocLinkObj();
        if (NULL == pLinkObj) {
            _IOC_LogWarn("Failed to alloc a new link object, daemon will retry");
            usleep(100000);  // Wait 100ms before retry to avoid busy loop
            continue;
        }

        IOC_Result_T Result = pSrvObj->pMethods->OpAcceptClient_F(pSrvObj, pLinkObj, NULL /*TODO:TIMEOUT*/);
        if (IOC_RESULT_SUCCESS != Result) {
            _IOC_LogWarn("Failed to accept client, Result=%d, daemon continues", Result);
            __IOC_freeLinkObj(pLinkObj);
            usleep(10000);  // Wait 10ms before retry
            continue;
        } else {
            _IOC_LogInfo("Accepted a new client, LinkID=%" PRIu64 "", pLinkObj->ID);

            // Copy protocol methods from Service
            pLinkObj->pMethods = pSrvObj->pMethods;

            // Store the accepted link for tracking
            for (int i = 0; i < _MAX_BROADCAST_EVENT_ACCEPTED_LINK_NUM; i++) {
                if (NULL == pSrvObj->BroadcastEvent.pAcceptedLinks[i]) {
                    pSrvObj->BroadcastEvent.pAcceptedLinks[i] = pLinkObj;
                    pSrvObj->BroadcastEvent.AcceptedLinkCount++;
                    break;
                }
            }
        }
    };
}

/**
 * @brief The auto-accept daemon thread function is created
 *  when the service is onlined with IOC_SRVFLAG_AUTO_ACCEPT flag.
 * @param pArg: the pointer to the service object.
 *
 * @details
 *    1) auto accept incoming client connections for any service type (DAT, Event, etc.)
 *    2) automatically handle connection acceptance without manual IOC_acceptClient() calls
 *    3) works with P2P communication pattern (not broadcast)
 */
static void* __IOC_ServiceAutoAcceptDaemonThread(void* pArg) {
    _IOC_ServiceObject_pT pSrvObj = (_IOC_ServiceObject_pT)pArg;
    _IOC_LogAssert(NULL != pSrvObj);

    _IOC_LogInfo("Auto-accept daemon thread started for service (URI: %s)",
                 IOC_Helper_printSingleLineSrvURI(&pSrvObj->Args.SrvURI, NULL, 0));

    while (1) {
        _IOC_LinkObject_pT pLinkObj = __IOC_allocLinkObj();
        if (NULL == pLinkObj) {
            _IOC_LogWarn("Failed to alloc a new link object for auto-accept");
            // Brief sleep before retrying to avoid busy waiting
            usleep(10000);  // 10ms
            continue;
        }

        IOC_Result_T Result = pSrvObj->pMethods->OpAcceptClient_F(pSrvObj, pLinkObj, NULL);
        if (IOC_RESULT_SUCCESS != Result) {
            _IOC_LogDebug("Auto-accept waiting for connection, Result=%d", Result);
            __IOC_freeLinkObj(pLinkObj);
            // Brief sleep when no connections are available to avoid busy waiting
            usleep(10000);  // 10ms
        } else {
            _IOC_LogInfo("Auto-accepted new client connection, LinkID=%" PRIu64 "", pLinkObj->ID);

            // 🎯 [TDD GREEN] pLinkObj->Args.Usage is already set by OpAcceptClient_F with negotiated role
            // DO NOT overwrite with full service capabilities here!
            // OLD WRONG CODE: pLinkObj->Args.Usage = pSrvObj->Args.UsageCapabilites;

            pLinkObj->pMethods = pSrvObj->pMethods;  // Copy protocol methods from Service

            // 🎯 TDD GREEN: Initialize connection state to Connected for auto-accepted link
            // Bug found by UT_LinkStateOperation TC-1 testing both client and server sides
            pthread_mutex_lock(&pLinkObj->ConnState.StateMutex);
            pLinkObj->ConnState.CurrentState = IOC_LinkConnStateConnected;
            pLinkObj->ConnState.IsConnected = true;
            pLinkObj->ConnState.LastStateChangeTime = time(NULL);
            pthread_mutex_unlock(&pLinkObj->ConnState.StateMutex);

            // 🔧 [TDD GREEN] Copy DAT callback configuration to auto-accepted connection
            if (pSrvObj->Args.UsageArgs.pDat) {
                pLinkObj->Args.UsageArgs.pDat = pSrvObj->Args.UsageArgs.pDat;
            }

            // 🔧 [TDD GREEN] Copy CMD callback configuration to auto-accepted connection
            if (pSrvObj->Args.UsageArgs.pCmd) {
                pLinkObj->Args.UsageArgs.pCmd = pSrvObj->Args.UsageArgs.pCmd;
            }

            // Initialize SubState for auto-accepted connection based on service usage
            IOC_LinkSubState_T initialSubState = IOC_LinkSubStateDefault;
            if (pSrvObj->Args.UsageCapabilites & IOC_LinkUsageDatSender) {
                initialSubState = IOC_LinkSubStateDatSenderReady;
            } else if (pSrvObj->Args.UsageCapabilites & IOC_LinkUsageDatReceiver) {
                initialSubState = IOC_LinkSubStateDatReceiverReady;
            }
            _IOC_updateConlesEventSubState(pLinkObj->ID, initialSubState);

            // Store the accepted link for tracking
            for (int i = 0; i < _MAX_AUTO_ACCEPT_ACCEPTED_LINK_NUM; i++) {
                if (NULL == pSrvObj->AutoAccept.pAcceptedLinks[i]) {
                    pSrvObj->AutoAccept.pAcceptedLinks[i] = pLinkObj;
                    pSrvObj->AutoAccept.AcceptedLinkCount++;
                    break;
                }
            }

            // If a service-level OnAutoAccepted_F hook is provided, notify immediately.
            if (pSrvObj->Args.OnAutoAccepted_F) {
                pSrvObj->Args.OnAutoAccepted_F(pSrvObj->ID, pLinkObj->ID, pSrvObj->Args.pSrvPriv);
            }
        }
    }

    return NULL;
}

IOC_Result_T IOC_onlineService(
    /*ARG_OUT */ IOC_SrvID_pT pSrvID,
    /*ARG_IN*/ const IOC_SrvArgs_pT pSrvArgs) {
    if (NULL == pSrvID) {
        _IOC_LogWarn("Invalid parameter, pSrvID is NULL");
        return IOC_RESULT_INVALID_PARAM;
    }
    if (IOC_RESULT_YES != __IOC_isValidSrvArgs(pSrvArgs)) {
        _IOC_LogWarn("Invalid parameter, pSrvArgs is invalid");
        return IOC_RESULT_INVALID_PARAM;
    }

    //---------------------------------------------------------------------------
    IOC_Result_T Result = IOC_RESULT_BUG;
    _IOC_ServiceObject_pT pSrvObj = NULL;

    Result = __IOC_allocSrvObj(pSrvArgs, &pSrvObj);
    if (IOC_RESULT_SUCCESS != Result) {
        _IOC_LogWarn("Failed to alloc a service object, Resuld=%d", Result);
        //_IOC_LogNotTested();
        return Result;
    }

    Result = __IOC_onlineServiceByProto(pSrvObj);
    if (IOC_RESULT_SUCCESS != Result) {
        _IOC_LogWarn("Failed to online service of URI(%s), Resuld=%d",
                     IOC_Helper_printSingleLineSrvURI(&pSrvArgs->SrvURI, NULL, 0), Result);
        // Path tested by UT_CommandMisuseTCP/verifyTcpMisuse_byWrongProtocol_expectConfigError
        goto _RetFail_onlineServiceByProto;
    }

    if (pSrvArgs->Flags & IOC_SRVFLAG_BROADCAST_EVENT) {
        int PosixResult =
            pthread_create(&pSrvObj->BroadcastEvent.DaemonThreadID, NULL, __IOC_ServiceBroadcastDaemonThread, pSrvObj);
        if (0 != PosixResult) {
            _IOC_LogWarn("Failed to create broadcast daemon thread, PosixResult=%d", PosixResult);
            Result = -errno;
            _IOC_LogNotTested();
            goto _RetFail_createBroadcastDaemonThread;
        }
    }

    if (pSrvArgs->Flags & IOC_SRVFLAG_AUTO_ACCEPT) {
        int PosixResult =
            pthread_create(&pSrvObj->AutoAccept.DaemonThreadID, NULL, __IOC_ServiceAutoAcceptDaemonThread, pSrvObj);
        if (0 != PosixResult) {
            _IOC_LogWarn("Failed to create auto-accept daemon thread, PosixResult=%d", PosixResult);
            Result = -errno;
            _IOC_LogNotTested();
            goto _RetFail_createAutoAcceptDaemonThread;
        }
    }

    // finally we reach the success return point
    *pSrvID = pSrvObj->ID;

    // 🎯 TDD GREEN: Initialize ConlesEvent SubState based on service usage
    // This ensures tests can properly verify DAT SubStates from the start
    IOC_LinkSubState_T initialSubState = IOC_LinkSubStateDefault;
    if (pSrvArgs->UsageCapabilites & IOC_LinkUsageDatSender) {
        initialSubState = IOC_LinkSubStateDatSenderReady;
    } else if (pSrvArgs->UsageCapabilites & IOC_LinkUsageDatReceiver) {
        initialSubState = IOC_LinkSubStateDatReceiverReady;
    }

    if (initialSubState != IOC_LinkSubStateDefault) {
        _IOC_updateConlesEventSubState(IOC_CONLES_MODE_AUTO_LINK_ID, initialSubState);
    }

    //_IOC_LogNotTested();
    return IOC_RESULT_SUCCESS;

_RetFail_createAutoAcceptDaemonThread:
    if (pSrvArgs->Flags & IOC_SRVFLAG_BROADCAST_EVENT) {
        pthread_cancel(pSrvObj->BroadcastEvent.DaemonThreadID);
        pthread_join(pSrvObj->BroadcastEvent.DaemonThreadID, NULL);
    }
_RetFail_createBroadcastDaemonThread:
    Result = pSrvObj->pMethods->OpOfflineService_F(pSrvObj);
    if (IOC_RESULT_SUCCESS != Result) {
        _IOC_LogBug("Failed to offline service by protocol, Resuld=%d", Result);
    }
_RetFail_onlineServiceByProto:
    __IOC_freeSrvObj(pSrvObj);
    return Result;
}

IOC_Result_T IOC_offlineService(
    /*ARG_IN*/ IOC_SrvID_T SrvID) {
    _IOC_ServiceObject_pT pSrvObj = __IOC_getSrvObjBySrvID(SrvID);
    if (NULL == pSrvObj) {
        _IOC_LogWarn("Failed to get service object by SrvID=%" PRIu64 "", SrvID);
        return IOC_RESULT_NOT_EXIST_SERVICE;
    }

    _IOC_LogAssert(pSrvObj->pMethods != NULL);
    _IOC_LogAssert(pSrvObj->pMethods->OpOfflineService_F != NULL);

    if (pSrvObj->Args.Flags & IOC_SRVFLAG_BROADCAST_EVENT) {
        pthread_cancel(pSrvObj->BroadcastEvent.DaemonThreadID);
        pthread_join(pSrvObj->BroadcastEvent.DaemonThreadID, NULL);

        // Auto-close accepted links unless KEEP_ACCEPTED_LINK flag is set
        if (!(pSrvObj->Args.Flags & IOC_SRVFLAG_KEEP_ACCEPTED_LINK)) {
            for (int i = 0; i < _MAX_BROADCAST_EVENT_ACCEPTED_LINK_NUM; i++) {
                if (NULL != pSrvObj->BroadcastEvent.pAcceptedLinks[i]) {
                    pSrvObj->pMethods->OpCloseLink_F(pSrvObj->BroadcastEvent.pAcceptedLinks[i]);
                }
            }
        }
    }

    if (pSrvObj->Args.Flags & IOC_SRVFLAG_AUTO_ACCEPT) {
        pthread_cancel(pSrvObj->AutoAccept.DaemonThreadID);
        pthread_join(pSrvObj->AutoAccept.DaemonThreadID, NULL);

        // Auto-close accepted links unless KEEP_ACCEPTED_LINK flag is set
        if (!(pSrvObj->Args.Flags & IOC_SRVFLAG_KEEP_ACCEPTED_LINK)) {
            for (int i = 0; i < _MAX_AUTO_ACCEPT_ACCEPTED_LINK_NUM; i++) {
                _IOC_LinkObject_pT pLinkObj = pSrvObj->AutoAccept.pAcceptedLinks[i];
                if (NULL != pLinkObj) {
                    // Save LinkID before nulling the pointer
                    IOC_LinkID_T linkID = pLinkObj->ID;

                    // Null out array entry first to avoid double-access
                    pSrvObj->AutoAccept.pAcceptedLinks[i] = NULL;

                    // Verify link still exists in global table before closing
                    // (it may have been closed already by IOC_closeLink)
                    if (_IOC_getLinkObjByLinkID(linkID) != NULL) {
                        pSrvObj->pMethods->OpCloseLink_F(pLinkObj);
                        __IOC_freeLinkObj(pLinkObj);
                    }
                }
            }
        }
    }

    // Cleanup manually accepted links (from IOC_acceptClient) unless KEEP_ACCEPTED_LINK flag is set
    if (!(pSrvObj->Args.Flags & IOC_SRVFLAG_KEEP_ACCEPTED_LINK)) {
        pthread_mutex_lock(&pSrvObj->ManualAccept.Mutex);
        for (int i = 0; i < _MAX_MANUAL_ACCEPT_ACCEPTED_LINK_NUM; i++) {
            if (IOC_ID_INVALID != pSrvObj->ManualAccept.AcceptedLinkIDs[i]) {
                // Try to close the link - it may already be closed, which is fine
                IOC_closeLink(pSrvObj->ManualAccept.AcceptedLinkIDs[i]);
                pSrvObj->ManualAccept.AcceptedLinkIDs[i] = IOC_ID_INVALID;
                pSrvObj->ManualAccept.AcceptedLinkCount--;
            }
        }
        pthread_mutex_unlock(&pSrvObj->ManualAccept.Mutex);
    }

    IOC_Result_T Result = pSrvObj->pMethods->OpOfflineService_F(pSrvObj);
    if (IOC_RESULT_SUCCESS != Result) {
        _IOC_LogWarn("Failed to offline service by protocol, Resuld=%d", Result);
        return Result;
    }

    __IOC_freeSrvObj(pSrvObj);

    //_IOC_LogNotTested();
    return IOC_RESULT_SUCCESS;
}

/**
 * @brief Negotiate the actual link role for service based on client's requested role
 *
 * 🎯 TDD GREEN IMPLEMENTATION for US-3: Multi-Role Service State Verification
 *
 * When a multi-role service (e.g., CmdInitiator | CmdExecutor) accepts a client connection,
 * the service must act as the COMPLEMENTARY role to the client on that specific link:
 *
 * ROLE NEGOTIATION RULES:
 *  • If Client requests CmdExecutor role → Service acts as CmdInitiator on that link
 *  • If Client requests CmdInitiator role → Service acts as CmdExecutor on that link
 *  • Similar logic applies to Event and Data usage pairs
 *
 * ARCHITECTURE PRINCIPLE:
 *  • SERVICE Capabilities: UsageCapabilites = multiple roles (e.g., 0x0C = both Initiator+Executor)
 *  • LINK Usage: Each LinkID has ONLY ONE role pair (e.g., Service=Initiator ↔ Client=Executor)
 *  • Multi-Role Service = Service managing MULTIPLE links with DIFFERENT single roles
 *
 * @param ServiceCapabilities: The service's full capabilities bitmask (what it CAN do)
 * @param ClientRequestedUsage: The client's requested role on this link (what client WANTS to be)
 * @return IOC_LinkUsage_T: The service's actual role on this specific link (complementary to client)
 */
IOC_LinkUsage_T _IOC_negotiateLinkRole(IOC_LinkUsage_T ServiceCapabilities, IOC_LinkUsage_T ClientRequestedUsage) {
    IOC_LinkUsage_T ServiceLinkRole = IOC_LinkUsageUndefined;

    // Negotiate Command role
    if (ClientRequestedUsage & IOC_LinkUsageCmdExecutor) {
        // Client wants to be Executor → Service must be Initiator
        if (ServiceCapabilities & IOC_LinkUsageCmdInitiator) {
            ServiceLinkRole |= IOC_LinkUsageCmdInitiator;
        }
    }
    if (ClientRequestedUsage & IOC_LinkUsageCmdInitiator) {
        // Client wants to be Initiator → Service must be Executor
        if (ServiceCapabilities & IOC_LinkUsageCmdExecutor) {
            ServiceLinkRole |= IOC_LinkUsageCmdExecutor;
        }
    }

    // Negotiate Event role
    if (ClientRequestedUsage & IOC_LinkUsageEvtProducer) {
        // Client wants to be Producer → Service must be Consumer
        if (ServiceCapabilities & IOC_LinkUsageEvtConsumer) {
            ServiceLinkRole |= IOC_LinkUsageEvtConsumer;
        }
    }
    if (ClientRequestedUsage & IOC_LinkUsageEvtConsumer) {
        // Client wants to be Consumer → Service must be Producer
        if (ServiceCapabilities & IOC_LinkUsageEvtProducer) {
            ServiceLinkRole |= IOC_LinkUsageEvtProducer;
        }
    }

    // Negotiate Data role
    if (ClientRequestedUsage & IOC_LinkUsageDatReceiver) {
        // Client wants to be Receiver → Service must be Sender
        if (ServiceCapabilities & IOC_LinkUsageDatSender) {
            ServiceLinkRole |= IOC_LinkUsageDatSender;
        }
    }
    if (ClientRequestedUsage & IOC_LinkUsageDatSender) {
        // Client wants to be Sender → Service must be Receiver
        if (ServiceCapabilities & IOC_LinkUsageDatReceiver) {
            ServiceLinkRole |= IOC_LinkUsageDatReceiver;
        }
    }

    return ServiceLinkRole;
}

IOC_Result_T IOC_acceptClient(
    /*ARG_IN*/ IOC_SrvID_T SrvID,
    /*ARG_OUT*/ IOC_LinkID_pT pLinkID,
    /*ARG_IN_OPTIONAL*/ const IOC_Options_pT pOption) {
    IOC_Result_T Result = IOC_RESULT_BUG;

    // 🎯 TDD GREEN FIX: Validate output parameter
    if (!pLinkID) {
        return IOC_RESULT_INVALID_PARAM;
    }

    // Step-1: Get the Service Object by SrvID
    _IOC_ServiceObject_pT pSrvObj = __IOC_getSrvObjBySrvID(SrvID);
    if (NULL == pSrvObj) {
        _IOC_LogWarn("Failed to get service object by SrvID=%" PRIu64, SrvID);
        return IOC_RESULT_NOT_EXIST_SERVICE;
    }

    // 🐛 TDD FIX for US-4/AC-1: Reject manual accept on AUTO_ACCEPT services
    // AUTO_ACCEPT services manage their own accept loop via daemon thread
    if (pSrvObj->Args.Flags & IOC_SRVFLAG_AUTO_ACCEPT) {
        _IOC_LogWarn("Manual IOC_acceptClient not allowed on AUTO_ACCEPT service (SrvID=%" PRIu64 ")", SrvID);
        return IOC_RESULT_NOT_SUPPORT_MANUAL_ACCEPT;
    }

    // Step-2: Create a Link Object
    _IOC_LinkObject_pT pLinkObj = __IOC_allocLinkObj();
    if (NULL == pLinkObj) {
        _IOC_LogWarn("SrvID(%" PRIu64 "): failed to alloc a new Link object when accept client", SrvID);
        //_IOC_LogNotTested();
        return IOC_RESULT_POSIX_ENOMEM;
    } else {
        pLinkObj->pMethods = pSrvObj->pMethods;

        // 🎯 TDD GREEN FIX for US-3: DO NOT assign full service capabilities here!
        // The protocol layer's OpAcceptClient_F will negotiate and set the actual link role.
        // OLD WRONG CODE: pLinkObj->Args.Usage = pSrvObj->Args.UsageCapabilites;

        // However, we DO need to temporarily store service capabilities for protocol negotiation
        pLinkObj->Args.Usage = pSrvObj->Args.UsageCapabilites;  // Temp for negotiation, will be overwritten
        pLinkObj->Args.SrvURI = pSrvObj->Args.SrvURI;           // Copy service URI including path

        // Copy usage-specific arguments based on the inherited capabilities
        if (pSrvObj->Args.UsageCapabilites & IOC_LinkUsageEvtConsumer) {
            pLinkObj->Args.UsageArgs.pEvt = pSrvObj->Args.UsageArgs.pEvt;
        }
        if (pSrvObj->Args.UsageCapabilites & IOC_LinkUsageCmdExecutor) {
            pLinkObj->Args.UsageArgs.pCmd = pSrvObj->Args.UsageArgs.pCmd;
        }
        if (pSrvObj->Args.UsageCapabilites & IOC_LinkUsageDatReceiver) {
            pLinkObj->Args.UsageArgs.pDat = pSrvObj->Args.UsageArgs.pDat;
        }
    }

    // Step-3: Accept Client by Protocol
    _IOC_LogAssert(pSrvObj->pMethods != NULL);
    _IOC_LogAssert(pSrvObj->pMethods->OpAcceptClient_F != NULL);

    Result = pSrvObj->pMethods->OpAcceptClient_F(pSrvObj, pLinkObj, pOption);
    if (IOC_RESULT_SUCCESS != Result) {
        __IOC_freeLinkObj(pLinkObj);  // Proper cleanup: removes from table and decrements counter
        _IOC_LogWarn("Failed to accept client by protocol, Resuld=%d", Result);
        // Error path now tested by UT_ServiceMisuse/US-3/AC-2 (timeout scenario)
        return Result;
    } else {
        *pLinkID = pLinkObj->ID;

        // 🎯 TDD GREEN FIX for US-3: Role negotiation is done inside OpAcceptClient_F
        // The protocol layer has access to both peer links and performs the negotiation
        // pLinkObj->Args.Usage has been updated to the negotiated service role

        // Step-4: Track manually accepted link for cleanup during service shutdown
        pthread_mutex_lock(&pSrvObj->ManualAccept.Mutex);
        for (int i = 0; i < _MAX_MANUAL_ACCEPT_ACCEPTED_LINK_NUM; i++) {
            if (IOC_ID_INVALID == pSrvObj->ManualAccept.AcceptedLinkIDs[i]) {
                pSrvObj->ManualAccept.AcceptedLinkIDs[i] = pLinkObj->ID;
                pSrvObj->ManualAccept.AcceptedLinkCount++;
                break;
            }
        }
        pthread_mutex_unlock(&pSrvObj->ManualAccept.Mutex);

        // Step-5: Auto-subscribe if SrvArgs.UsageArgs.pEvt is provided for event consumer services
        // This implements the server-side auto-subscription feature for event consumption
        if ((pSrvObj->Args.UsageCapabilites & IOC_LinkUsageEvtConsumer) && (pSrvObj->Args.UsageArgs.pEvt != NULL)) {
            // Convert IOC_EvtUsageArgs_T to IOC_SubEvtArgs_T for IOC_subEVT
            IOC_SubEvtArgs_T SubEvtArgs = {.pEvtIDs = pSrvObj->Args.UsageArgs.pEvt->pEvtIDs,
                                           .EvtNum = pSrvObj->Args.UsageArgs.pEvt->EvtNum,
                                           .CbProcEvt_F = pSrvObj->Args.UsageArgs.pEvt->CbProcEvt_F,
                                           .pCbPrivData = pSrvObj->Args.UsageArgs.pEvt->pCbPrivData};

            // Automatically subscribe to the specified events on the accepted client link
            IOC_Result_T SubResult = IOC_subEVT(*pLinkID, &SubEvtArgs);
            if (IOC_RESULT_SUCCESS != SubResult) {
                // If auto-subscription fails, close the link and return the error
                IOC_closeLink(*pLinkID);
                *pLinkID = IOC_ID_INVALID;

                // Remove from manual accept tracking since we're failing
                pthread_mutex_lock(&pSrvObj->ManualAccept.Mutex);
                for (int i = 0; i < _MAX_MANUAL_ACCEPT_ACCEPTED_LINK_NUM; i++) {
                    if (pLinkObj->ID == pSrvObj->ManualAccept.AcceptedLinkIDs[i]) {
                        pSrvObj->ManualAccept.AcceptedLinkIDs[i] = IOC_ID_INVALID;
                        pSrvObj->ManualAccept.AcceptedLinkCount--;
                        break;
                    }
                }
                pthread_mutex_unlock(&pSrvObj->ManualAccept.Mutex);

                return SubResult;
            }
        }
    }

    //_IOC_LogNotTested();
    return IOC_RESULT_SUCCESS;
}

static IOC_BoolResult_T __IOC_isValidConnSrvArgs(
    /*ARG_OUT*/ IOC_LinkID_pT pLinkID,
    /*ARG_IN*/ const IOC_ConnArgs_pT pConnArgs,
    /*ARG_IN_OPTIONAL*/ const IOC_Options_pT pOption) {
    if (NULL == pLinkID) {
        _IOC_LogWarn("Invalid parameter, pLinkID is NULL");
        return IOC_RESULT_NO;
    }

    if (NULL == pConnArgs) {
        _IOC_LogWarn("Invalid parameter, pConnArgs is NULL");
        return IOC_RESULT_NO;
    }

    // TODO: check pConnArgs->...

    if (NULL != pOption) {
        // TODO: check pOption->...
    }

    return IOC_RESULT_YES;
}

static IOC_Result_T __IOC_connectServiceByProto(
    /*ARG_INOUT*/ _IOC_LinkObject_pT pLinkObj,
    /*ARG_IN*/ const IOC_ConnArgs_pT pConnArgs,
    /*ARG_IN_OPTIONAL*/ const IOC_Options_pT pOption) {
    IOC_Result_T Result = IOC_RESULT_BUG;

    // TDD FIX: Copy connection arguments to LinkObject before calling protocol-specific connect
    // This ensures that protocol implementations can access the connection parameters
    memcpy(&pLinkObj->Args, pConnArgs, sizeof(IOC_ConnArgs_T));

    // 🎯 TDD GREEN: Set connection state to Connecting before attempting connection
    pthread_mutex_lock(&pLinkObj->ConnState.StateMutex);
    pLinkObj->ConnState.CurrentState = IOC_LinkConnStateConnecting;
    pLinkObj->ConnState.LastStateChangeTime = time(NULL);
    pthread_mutex_unlock(&pLinkObj->ConnState.StateMutex);

    // IF ProtoAuto, TRY OneByOne Proto in _mIOC_SrvProtoMethods until the first success or all failed.
    // ELSE: TRY the specified Proto in _mIOC_SrvProtoMethods and return the result.
    IOC_Bool_T IsProtoAuto = !strcmp(pConnArgs->SrvURI.pProtocol, IOC_SRV_PROTO_AUTO);

    if (IsProtoAuto) {
        for (int TryProtoIdx = 0; TryProtoIdx < IOC_calcArrayElmtCnt(_mIOC_SrvProtoMethods); TryProtoIdx++) {
            Result = _mIOC_SrvProtoMethods[TryProtoIdx]->OpConnectService_F(pLinkObj, pConnArgs, pOption);
            if (IOC_RESULT_SUCCESS == Result) {
                _IOC_LogNotTested();
                break;
            }
        }
    } else {
        bool ProtocolFound = false;
        for (int CmpProtoIdx = 0; CmpProtoIdx < IOC_calcArrayElmtCnt(_mIOC_SrvProtoMethods); CmpProtoIdx++) {
            if (!strcmp(pConnArgs->SrvURI.pProtocol, _mIOC_SrvProtoMethods[CmpProtoIdx]->pProtocol)) {
                ProtocolFound = true;  // Mark protocol as found
                Result = _mIOC_SrvProtoMethods[CmpProtoIdx]->OpConnectService_F(pLinkObj, pConnArgs, pOption);
                if (IOC_RESULT_SUCCESS == Result) {
                    pLinkObj->pMethods = _mIOC_SrvProtoMethods[CmpProtoIdx];

                    // 🎯 TDD GREEN: Set connection state to Connected after successful connection
                    pthread_mutex_lock(&pLinkObj->ConnState.StateMutex);
                    pLinkObj->ConnState.CurrentState = IOC_LinkConnStateConnected;
                    pLinkObj->ConnState.IsConnected = true;
                    pLinkObj->ConnState.LastStateChangeTime = time(NULL);
                    pthread_mutex_unlock(&pLinkObj->ConnState.StateMutex);
                }
                // 🐛 BUG FIX #3 (TC-20): Break after finding protocol, even if connect failed
                // Don't mask connection errors with "protocol not supported"!
                break;
            }
        }

        // 🎯 TDD GREEN FIX: Return error if no matching protocol found
        // Only return NOT_SUPPORT if protocol wasn't found, not if connection failed!
        if (!ProtocolFound) {
            _IOC_LogWarn("Unsupported protocol: %s", pConnArgs->SrvURI.pProtocol);
            return IOC_RESULT_NOT_SUPPORT;
        }
        //_IOC_LogNotTested();
    }

    //_IOC_LogNotTested();
    return Result;
}

IOC_Result_T IOC_connectService(
    /*ARG_OUT*/ IOC_LinkID_pT pLinkID,
    /*ARG_IN*/ const IOC_ConnArgs_pT pConnArgs,
    /*ARG_IN_OPTIONAL*/ const IOC_Options_pT pOption) {
    IOC_Result_T Result = IOC_RESULT_BUG;

    // Step-1: Check Parameters
    if (IOC_RESULT_YES != __IOC_isValidConnSrvArgs(pLinkID, pConnArgs, pOption)) {
        return IOC_RESULT_INVALID_PARAM;
    }

    // Step-2: create a Link Object
    _IOC_LinkObject_pT pLinkObj = __IOC_allocLinkObj();
    if (NULL == pLinkObj) {
        _IOC_LogError("Failed to alloc a new Link object when connect service");
        //_IOC_LogNotTested();
        return IOC_RESULT_POSIX_ENOMEM;
    }

    // Step3-: Connect Service by Protocol
    Result = __IOC_connectServiceByProto(pLinkObj, pConnArgs, pOption);
    if (IOC_RESULT_SUCCESS != Result) {
        free(pLinkObj);
        return Result;
    } else {
        *pLinkID = pLinkObj->ID;
    }

    // Step4-: Auto-subscribe if ConnArgs.UsageArgs.pEvt is provided for event consumers
    // This implements the auto-subscription feature for polling-based event consumption
    if (pConnArgs->Usage == IOC_LinkUsageEvtConsumer && pConnArgs->UsageArgs.pEvt != NULL) {
        // Convert IOC_EvtUsageArgs_T to IOC_SubEvtArgs_T for IOC_subEVT
        IOC_SubEvtArgs_T SubEvtArgs = {.pEvtIDs = pConnArgs->UsageArgs.pEvt->pEvtIDs,
                                       .EvtNum = pConnArgs->UsageArgs.pEvt->EvtNum,
                                       .CbProcEvt_F = pConnArgs->UsageArgs.pEvt->CbProcEvt_F,
                                       .pCbPrivData = pConnArgs->UsageArgs.pEvt->pCbPrivData};

        // Automatically subscribe to the specified events
        IOC_Result_T SubResult = IOC_subEVT(*pLinkID, &SubEvtArgs);
        if (IOC_RESULT_SUCCESS != SubResult) {
            // If auto-subscription fails, close the link and return the error
            IOC_closeLink(*pLinkID);
            *pLinkID = IOC_ID_INVALID;
            return SubResult;
        }
    }

    //_IOC_LogNotTested();
    return IOC_RESULT_SUCCESS;
}

IOC_Result_T IOC_closeLink(
    /*ARG_IN*/ IOC_LinkID_T LinkID) {
    _IOC_LinkObject_pT pLinkObj = _IOC_getLinkObjByLinkID(LinkID);
    if (pLinkObj == NULL) {
        _IOC_LogError("Failed to get LinkObj by LinkID(%" PRIu64 ")", LinkID);
        return IOC_RESULT_NOT_EXIST_LINK;
    }

    _IOC_LogAssert(pLinkObj->pMethods != NULL);
    _IOC_LogAssert(pLinkObj->pMethods->OpCloseLink_F != NULL);

    // Update connection state to Disconnecting before closing (Level 1 - Connection State)
    // This provides observable state transition: Connected → Disconnecting → (freed)
    // Cross-reference: README_ArchDesign-State.md - Connection state lifecycle
    if (pthread_mutex_lock(&pLinkObj->ConnState.StateMutex) == 0) {
        pLinkObj->ConnState.CurrentState = IOC_LinkConnStateDisconnecting;
        pLinkObj->ConnState.IsConnected = false;
        pLinkObj->ConnState.LastStateChangeTime = time(NULL);
        pthread_mutex_unlock(&pLinkObj->ConnState.StateMutex);
        _IOC_LogDebug("Link connection state set to Disconnecting for LinkID=%" PRIu64, LinkID);
    }

    IOC_Result_T Result = pLinkObj->pMethods->OpCloseLink_F(pLinkObj);
    if (Result != IOC_RESULT_SUCCESS) {
        _IOC_LogError("Failed to closeLink by protocol, Result=%d", Result);
        return Result;
    }

    __IOC_freeLinkObj(pLinkObj);

    //_IOC_LogNotTested();
    return IOC_RESULT_SUCCESS;
}

IOC_Result_T IOC_broadcastEVT(
    /*ARG_IN*/ IOC_SrvID_T SrvID,
    /*ARG_IN*/ const IOC_EvtDesc_pT pEvtDesc,
    /*ARG_IN_OPTIONAL*/ IOC_Options_pT pOption) {
    _IOC_ServiceObject_pT pSrvObj = __IOC_getSrvObjBySrvID(SrvID);
    if (NULL == pSrvObj) {
        _IOC_LogError("Failed to get service object by SrvID=%" PRIu64, SrvID);
        return IOC_RESULT_NOT_EXIST_SERVICE;
    }

    // Check if the service supports broadcast event
    if (!(pSrvObj->Args.Flags & IOC_SRVFLAG_BROADCAST_EVENT)) {
        return IOC_RESULT_NOT_SUPPORT_BROADCAST_EVENT;
    }

    int PostEvtCnt = 0;

    for (int i = 0; i < _MAX_BROADCAST_EVENT_ACCEPTED_LINK_NUM; i++) {
        _IOC_LinkObject_pT pLinkObj = pSrvObj->BroadcastEvent.pAcceptedLinks[i];
        if (pLinkObj) {
            IOC_Result_T Result = pSrvObj->pMethods->OpPostEvt_F(pLinkObj, pEvtDesc, pOption);
            if (Result != IOC_RESULT_SUCCESS) {
                // This is expected when a client is connected but hasn't subscribed to this specific event
                _IOC_LogDebug("Failed to postEVT by protocol, Result=%d", Result);
            }

            PostEvtCnt++;
        }
    }

    //_IOC_LogNotTested();
    return (PostEvtCnt > 0) ? IOC_RESULT_SUCCESS : IOC_RESULT_NO_EVENT_CONSUMER;
}

//=================================================================================================
// NEW SERVICE STATE INSPECTION APIS
//=================================================================================================

/**
 * @brief Get service-side LinkIDs for state inspection and management.
 *      This enables querying receiver-side states and comprehensive service monitoring.
 */
IOC_Result_T IOC_getServiceLinkIDs(IOC_SrvID_T SrvID, IOC_LinkID_T* pLinkIDs, uint16_t MaxLinks,
                                   uint16_t* pActualCount) {
    if (!pLinkIDs || !pActualCount) {
        return IOC_RESULT_INVALID_PARAM;
    }

    _IOC_ServiceObject_pT pSrvObj = __IOC_getSrvObjBySrvID(SrvID);
    if (!pSrvObj) {
        return IOC_RESULT_NOT_EXIST_SERVICE;
    }

    uint16_t Count = 0;

    // Check auto-accept links (main data/cmd services)
    if (pSrvObj->Args.Flags & IOC_SRVFLAG_AUTO_ACCEPT) {
        for (int i = 0; i < _MAX_AUTO_ACCEPT_ACCEPTED_LINK_NUM && Count < MaxLinks; i++) {
            if (pSrvObj->AutoAccept.pAcceptedLinks[i]) {
                pLinkIDs[Count] = pSrvObj->AutoAccept.pAcceptedLinks[i]->ID;
                Count++;
            }
        }
    }

    // Check broadcast event links if enabled
    if (pSrvObj->Args.Flags & IOC_SRVFLAG_BROADCAST_EVENT) {
        for (int i = 0; i < _MAX_BROADCAST_EVENT_ACCEPTED_LINK_NUM && Count < MaxLinks; i++) {
            if (pSrvObj->BroadcastEvent.pAcceptedLinks[i]) {
                // Avoid duplicates by checking if LinkID already exists
                IOC_LinkID_T LinkID = pSrvObj->BroadcastEvent.pAcceptedLinks[i]->ID;
                bool IsAlreadyAdded = false;
                for (uint16_t j = 0; j < Count; j++) {
                    if (pLinkIDs[j] == LinkID) {
                        IsAlreadyAdded = true;
                        break;
                    }
                }
                if (!IsAlreadyAdded) {
                    pLinkIDs[Count] = LinkID;
                    Count++;
                }
            }
        }
    }

    *pActualCount = Count;

    // Check if buffer was too small (simple detection)
    bool BufferTooSmall = (Count == MaxLinks);
    if (BufferTooSmall && pSrvObj->Args.Flags & IOC_SRVFLAG_AUTO_ACCEPT) {
        // Check if there are more auto-accept links beyond buffer capacity
        for (int i = MaxLinks; i < _MAX_AUTO_ACCEPT_ACCEPTED_LINK_NUM; i++) {
            if (pSrvObj->AutoAccept.pAcceptedLinks[i]) {
                return IOC_RESULT_BUFFER_TOO_SMALL;
            }
        }
    }

    return IOC_RESULT_SUCCESS;
}

/**
 * @brief Get comprehensive service state including all connected links.
 *      This provides complete service monitoring capability.
 */
IOC_Result_T IOC_getServiceState(IOC_SrvID_T SrvID, void* pServiceState, uint16_t* pConnectedLinks) {
    _IOC_ServiceObject_pT pSrvObj = __IOC_getSrvObjBySrvID(SrvID);
    if (!pSrvObj) {
        return IOC_RESULT_NOT_EXIST_SERVICE;
    }

    if (pConnectedLinks) {
        uint16_t Count = 0;

        // Count auto-accept links
        if (pSrvObj->Args.Flags & IOC_SRVFLAG_AUTO_ACCEPT) {
            for (int i = 0; i < _MAX_AUTO_ACCEPT_ACCEPTED_LINK_NUM; i++) {
                if (pSrvObj->AutoAccept.pAcceptedLinks[i]) {
                    Count++;
                }
            }
        }

        // Count broadcast links
        if (pSrvObj->Args.Flags & IOC_SRVFLAG_BROADCAST_EVENT) {
            for (int i = 0; i < _MAX_BROADCAST_EVENT_ACCEPTED_LINK_NUM; i++) {
                if (pSrvObj->BroadcastEvent.pAcceptedLinks[i]) {
                    Count++;
                }
            }
        }

        *pConnectedLinks = Count;
    }

    // pServiceState is reserved for future service state structure
    // For now, we just ensure it's not used incorrectly
    if (pServiceState) {
        // Future: populate service state information
        memset(pServiceState, 0, sizeof(void*));  // Safe null operation
    }

    return IOC_RESULT_SUCCESS;
}

/**
 * @brief Get link connection state (Level 1 of 3-level hierarchy)
 * @param LinkID: The link ID to query
 * @param pState: Pointer to store the connection state
 * @return IOC_Result_T
 *   - IOC_RESULT_SUCCESS: State retrieved successfully
 *   - IOC_RESULT_INVALID_PARAM: NULL pointer or invalid LinkID
 *   - IOC_RESULT_NOT_EXIST_LINK: LinkID does not exist
 *
 * @architecture README_ArchDesign-State.md "Link Connection States (Level 1)"
 * @note This API is for ConetMode only (TCP/FIFO). ConlesMode has no connection state.
 * @note Connection State (L1) is independent of Operation State (L2) and SubState (L3)
 *
 * @[TDD Phase]: 🟢 GREEN - Implementing to make RED tests pass
 * @[Tests]: UT_LinkConnStateTCP.cxx TC1-TC3
 */
IOC_Result_T IOC_getLinkConnState(IOC_LinkID_T LinkID, IOC_LinkConnState_T* pState) {
    // Fast-fail validation (Fast-Fail Six #1: NULL pointer)
    if (NULL == pState) {
        _IOC_LogError("IOC_getLinkConnState: NULL state pointer");
        return IOC_RESULT_INVALID_PARAM;
    }

    // Fast-fail validation (Fast-Fail Six #4: Invalid ID)
    if (IOC_ID_INVALID == LinkID) {
        _IOC_LogError("IOC_getLinkConnState: Invalid LinkID");
        return IOC_RESULT_INVALID_PARAM;
    }

    // Get link object
    _IOC_LinkObject_pT pLinkObj = _IOC_getLinkObjByLinkID(LinkID);
    if (NULL == pLinkObj) {
        _IOC_LogError("IOC_getLinkConnState: LinkID %" PRIu64 " does not exist", LinkID);
        return IOC_RESULT_NOT_EXIST_LINK;
    }

    // Thread-safe state retrieval
    pthread_mutex_lock(&pLinkObj->ConnState.StateMutex);
    *pState = pLinkObj->ConnState.CurrentState;
    pthread_mutex_unlock(&pLinkObj->ConnState.StateMutex);

    _IOC_LogDebug("IOC_getLinkConnState: LinkID=%" PRIu64 " State=%d", LinkID, *pState);

    return IOC_RESULT_SUCCESS;
}