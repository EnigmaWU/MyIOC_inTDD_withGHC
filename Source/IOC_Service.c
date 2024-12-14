#include "_IOC.h"

//=================================================================================================
#define _MAX_IOC_SRV_OBJ_NUM 2
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

            pSrvObj->ID = i;
            pSrvObj->Args.SrvURI.pProtocol = strdup(pSrvArgs->SrvURI.pProtocol);
            pSrvObj->Args.SrvURI.pHost = strdup(pSrvArgs->SrvURI.pHost);
            pSrvObj->Args.SrvURI.pPath = strdup(pSrvArgs->SrvURI.pPath);
            pSrvObj->Args.SrvURI.Port = pSrvArgs->SrvURI.Port;
            pSrvObj->Args.UsageCapabilites = pSrvArgs->UsageCapabilites;
            pSrvObj->Args.Flags = pSrvArgs->Flags;

            _mIOC_SrvObjTbl[i] = pSrvObj;
            *ppSrvObj = pSrvObj;
            Result = IOC_RESULT_SUCCESS;
            //_IOC_LogNotTested();
            goto _RetResult;
        }
    }

    Result = IOC_RESULT_TOO_MANY_SERVICES;
    _IOC_LogNotTested();

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
#define _MAX_IOC_LINK_OBJ_NUM 8
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

    free(pLinkObj);
    //_IOC_LogNotTested();
}

_IOC_LinkObject_pT _IOC_getLinkObjByLinkID(IOC_LinkID_T LinkID) {
    return _mIOC_LinkObjTbl[___IOC_convertLinkIDToLinkObjTblIdx(LinkID)];
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
        _IOC_LogNotTested();
        return Result;
    }

    Result = __IOC_onlineServiceByProto(pSrvObj);
    if (IOC_RESULT_SUCCESS != Result) {
        __IOC_freeSrvObj(pSrvObj);
        _IOC_LogWarn("Failed to online service by URI, Resuld=%d", Result);
        _IOC_LogNotTested();
        return Result;
    } else {
        *pSrvID = pSrvObj->ID;
    }

    //_IOC_LogNotTested();
    return IOC_RESULT_SUCCESS;
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
        _IOC_LogWarn("Failed to get service object by SrvID=%zu", SrvID);
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
        _IOC_LogNotTested();
        return Result;
    } else {
        *pLinkID = pLinkObj->ID;
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