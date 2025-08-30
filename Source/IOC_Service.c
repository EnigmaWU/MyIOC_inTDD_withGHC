#include <pthread.h>

#include "IOC/IOC_EvtAPI.h"  // For IOC_subEVT auto-subscription
#include "_IOC.h"

//=================================================================================================
static _IOC_ServiceObject_pT _mIOC_SrvObjTbl[_MAX_IOC_SRV_OBJ_NUM] = {};
static pthread_mutex_t _mIOC_SrvObjTblMutex = PTHREAD_MUTEX_INITIALIZER;
static inline void ___IOC_lockSrvObjTbl(void) { pthread_mutex_lock(&_mIOC_SrvObjTblMutex); }
static inline void ___IOC_unlockSrvObjTbl(void) { pthread_mutex_unlock(&_mIOC_SrvObjTblMutex); }

static inline IOC_BoolResult_T ___IOC_isSrvObjConflicted(IOC_SrvArgs_pT pArgsNew) {
    return IOC_RESULT_NO;  // TODO: JUST ENOUGH CODE
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
                                      /*ARG_OUT*/ _IOC_ServiceObject_pT *ppSrvObj) {
    IOC_Result_T Result = IOC_RESULT_BUG;
    _IOC_ServiceObject_pT pSrvObj = NULL;

    ___IOC_lockSrvObjTbl();
    if (IOC_RESULT_YES == ___IOC_isSrvObjConflicted(pSrvArgs)) {
        Result = IOC_RESULT_CONFLICT_SRVARGS;
        _IOC_LogNotTested();
        goto _RetResult;
    }

    for (int i = 0; i < _MAX_IOC_SRV_OBJ_NUM; i++) {
        if (NULL == _mIOC_SrvObjTbl[i]) {
            pSrvObj = (_IOC_ServiceObject_pT)calloc(1, sizeof(_IOC_ServiceObject_T));
            if (NULL == pSrvObj) {
                Result = IOC_RESULT_POSIX_ENOMEM;
                _IOC_LogNotTested();
                goto _RetResult;
            }

            // Initialize manual accept tracking mutex
            if (pthread_mutex_init(&pSrvObj->ManualAccept.Mutex, NULL) != 0) {
                free(pSrvObj);
                Result = IOC_RESULT_POSIX_ENOMEM;
                _IOC_LogNotTested();
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

    free((char *)pSrvObj->Args.SrvURI.pProtocol);
    free((char *)pSrvObj->Args.SrvURI.pHost);
    free((char *)pSrvObj->Args.SrvURI.pPath);
    free(pSrvObj);

    //_IOC_LogNotTested();
}

static _IOC_ServiceObject_pT __IOC_getSrvObjBySrvID(IOC_SrvID_T SrvID) {
    if (SrvID < _MAX_IOC_SRV_OBJ_NUM) {
        return _mIOC_SrvObjTbl[SrvID];
    } else {
        _IOC_LogError("Invalid SrvID=%" PRIu64 "", SrvID);
    }

    _IOC_LogNotTested();
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
            pLinkObj = (_IOC_LinkObject_pT)calloc(1, sizeof(_IOC_LinkObject_T));
            if (NULL == pLinkObj) {
                _IOC_LogError("Failed to alloc a link object");
                _IOC_LogNotTested();
                return NULL;
            }

            pLinkObj->ID = ___IOC_convertLinkObjTblIdxToLinkID(i);

            // 🎯 TDD IMPLEMENTATION: Initialize DAT state for IOC_getLinkState() support
            pLinkObj->DatState.CurrentSubState = IOC_LinkSubStateDefault;
            if (pthread_mutex_init(&pLinkObj->DatState.SubStateMutex, NULL) != 0) {
                _IOC_LogError("Failed to initialize DAT substate mutex for LinkID=%llu", pLinkObj->ID);
                free(pLinkObj);
                return NULL;
            }
            pLinkObj->DatState.IsSending = false;
            pLinkObj->DatState.IsReceiving = false;
            pLinkObj->DatState.LastOperationTime = time(NULL);

            _mIOC_LinkObjTbl[i] = pLinkObj;
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

    // 🎯 TDD IMPLEMENTATION: Cleanup DAT state mutex
    pthread_mutex_destroy(&pLinkObj->DatState.SubStateMutex);

    free(pLinkObj);
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
        _IOC_LogNotTested();
        return IOC_RESULT_NO;
    }

    if (NULL == pSrvArgs->SrvURI.pProtocol) {
        _IOC_LogNotTested();
        return IOC_RESULT_NO;
    }

    if (NULL == pSrvArgs->SrvURI.pHost) {
        _IOC_LogNotTested();
        return IOC_RESULT_NO;
    }

    if (NULL == pSrvArgs->SrvURI.pPath) {
        _IOC_LogNotTested();
        return IOC_RESULT_NO;
    }

    if (!(pSrvArgs->UsageCapabilites & IOC_LinkUsageMask)) {
        _IOC_LogNotTested();
        return IOC_RESULT_NO;
    }

    // TODO: check port if needed

    return IOC_RESULT_YES;
}

//_mIOC_XYZ is the global variable used intra-CURRENT_FILE submodule.
static _IOC_SrvProtoMethods_pT _mIOC_SrvProtoMethods[] = {
    &_gIOC_SrvProtoFifoMethods,
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
static void *__IOC_ServiceBroadcastDaemonThread(void *pArg) {
    _IOC_ServiceObject_pT pSrvObj = (_IOC_ServiceObject_pT)pArg;
    _IOC_LogAssert(NULL != pSrvObj);

    while (1) {
        _IOC_LinkObject_pT pLinkObj = __IOC_allocLinkObj();
        if (NULL == pLinkObj) {
            _IOC_LogBug("Failed to alloc a new link object");
            _IOC_LogNotTested();
            break;
        }

        IOC_Result_T Result = pSrvObj->pMethods->OpAcceptClient_F(pSrvObj, pLinkObj, NULL /*TODO:TIMEOUT*/);
        if (IOC_RESULT_SUCCESS != Result) {
            _IOC_LogError("Failed to accept client, Result=%d", Result);
            __IOC_freeLinkObj(pLinkObj);
            _IOC_LogNotTested();
        } else {
            _IOC_LogInfo("Accepted a new client, LinkID=%" PRIu64 "", pLinkObj->ID);
            for (int i = 0; i < _MAX_BROADCAST_EVENT_ACCEPTED_LINK_NUM; i++) {
                if (NULL == pSrvObj->BroadcastEvent.pAcceptedLinks[i]) {
                    pSrvObj->BroadcastEvent.pAcceptedLinks[i] = pLinkObj;
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
static void *__IOC_ServiceAutoAcceptDaemonThread(void *pArg) {
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

            // [🔧 TDD GREEN] Initialize auto-accepted connection properties based on service configuration
            pLinkObj->Args.Usage = pSrvObj->Args.UsageCapabilites;  // Set Usage for IOC_getLinkState()
            pLinkObj->pMethods = pSrvObj->pMethods;                 // Copy protocol methods from Service

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
        _IOC_LogNotTested();
        return IOC_RESULT_INVALID_PARAM;
    }
    if (IOC_RESULT_YES != __IOC_isValidSrvArgs(pSrvArgs)) {
        _IOC_LogWarn("Invalid parameter, pSrvArgs is invalid");
        _IOC_LogNotTested();
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
        _IOC_LogNotTested();
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
    _IOC_LogNotTested();
    return Result;
}

IOC_Result_T IOC_offlineService(
    /*ARG_IN*/ IOC_SrvID_T SrvID) {
    _IOC_ServiceObject_pT pSrvObj = __IOC_getSrvObjBySrvID(SrvID);
    if (NULL == pSrvObj) {
        _IOC_LogWarn("Failed to get service object by SrvID=%" PRIu64 "", SrvID);
        _IOC_LogNotTested();
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
                if (NULL != pSrvObj->AutoAccept.pAcceptedLinks[i]) {
                    pSrvObj->pMethods->OpCloseLink_F(pSrvObj->AutoAccept.pAcceptedLinks[i]);
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
        _IOC_LogNotTested();
        return Result;
    }

    __IOC_freeSrvObj(pSrvObj);

    //_IOC_LogNotTested();
    return IOC_RESULT_SUCCESS;
}

IOC_Result_T IOC_acceptClient(
    /*ARG_IN*/ IOC_SrvID_T SrvID,
    /*ARG_OUT*/ IOC_LinkID_pT pLinkID,
    /*ARG_IN_OPTIONAL*/ const IOC_Options_pT pOption) {
    IOC_Result_T Result = IOC_RESULT_BUG;

    // Step-1: Get the Service Object by SrvID
    _IOC_ServiceObject_pT pSrvObj = __IOC_getSrvObjBySrvID(SrvID);
    if (NULL == pSrvObj) {
        _IOC_LogWarn("Failed to get service object by SrvID=%" PRIu64, SrvID);
        _IOC_LogNotTested();
        return IOC_RESULT_NOT_EXIST_SERVICE;
    }

    // Step-2: Create a Link Object
    _IOC_LinkObject_pT pLinkObj = __IOC_allocLinkObj();
    if (NULL == pLinkObj) {
        _IOC_LogWarn("SrvID(%" PRIu64 "): failed to alloc a new Link object when accept client", SrvID);
        //_IOC_LogNotTested();
        return IOC_RESULT_POSIX_ENOMEM;
    } else {
        pLinkObj->pMethods = pSrvObj->pMethods;

        // 🔧 TDD FIX: Inherit service configuration into the accepted client link
        // This ensures that the server-side link has the correct Usage and URI information
        // for proper command routing and path matching
        pLinkObj->Args.Usage = pSrvObj->Args.UsageCapabilites;
        pLinkObj->Args.SrvURI = pSrvObj->Args.SrvURI;  // Copy service URI including path

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
        free(pLinkObj);
        _IOC_LogWarn("Failed to accept client by protocol, Resuld=%d", Result);
        _IOC_LogNotTested();
        return Result;
    } else {
        *pLinkID = pLinkObj->ID;

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
        _IOC_LogNotTested();
        return IOC_RESULT_NO;
    }

    if (NULL == pConnArgs) {
        _IOC_LogWarn("Invalid parameter, pConnArgs is NULL");
        _IOC_LogNotTested();
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
        for (int CmpProtoIdx = 0; CmpProtoIdx < IOC_calcArrayElmtCnt(_mIOC_SrvProtoMethods); CmpProtoIdx++) {
            if (!strcmp(pConnArgs->SrvURI.pProtocol, _mIOC_SrvProtoMethods[CmpProtoIdx]->pProtocol)) {
                Result = _mIOC_SrvProtoMethods[CmpProtoIdx]->OpConnectService_F(pLinkObj, pConnArgs, pOption);
                if (IOC_RESULT_SUCCESS == Result) {
                    pLinkObj->pMethods = _mIOC_SrvProtoMethods[CmpProtoIdx];
                }
                break;
            }
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
        _IOC_LogNotTested();
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
IOC_Result_T IOC_getServiceLinkIDs(IOC_SrvID_T SrvID, IOC_LinkID_T *pLinkIDs, uint16_t MaxLinks,
                                   uint16_t *pActualCount) {
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
IOC_Result_T IOC_getServiceState(IOC_SrvID_T SrvID, void *pServiceState, uint16_t *pConnectedLinks) {
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

        *pConnectedLinks = Count;
    }

    // pServiceState is reserved for future service state structure
    // For now, we just ensure it's not used incorrectly
    if (pServiceState) {
        // Future: populate service state information
        memset(pServiceState, 0, sizeof(void *));  // Safe null operation
    }

    return IOC_RESULT_SUCCESS;
}