#include "_IOC.h"

//=================================================================================================
#define _MAX_IOC_SRV_OBJ_NUM 1
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
            pSrvObj = (_IOC_ServiceObject_pT)malloc(sizeof(_IOC_ServiceObject_T));
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
    ___IOC_lockSrvObjTbl();
    _mIOC_SrvObjTbl[pSrvObj->ID] = NULL;
    ___IOC_unlockSrvObjTbl();

    if (NULL != pSrvObj) {
        free((char *)pSrvObj->Args.SrvURI.pProtocol);
        free((char *)pSrvObj->Args.SrvURI.pHost);
        free((char *)pSrvObj->Args.SrvURI.pPath);
        free(pSrvObj);
    }

    _IOC_LogNotTested();
}
static _IOC_ServiceObject_pT __IOC_getSrvObjBySrvID(IOC_SrvID_T SrvID) {
    _IOC_LogNotTested();
    return NULL;
}

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
        }
    } else {
        for (int i = 0; i < IOC_calcArrayElmtCnt(_mIOC_SrvProtoMethods); i++) {
            if (!strcmp(pSrvObj->Args.SrvURI.pProtocol, _mIOC_SrvProtoMethods[i]->pProtocol)) {
                OnlineResult = _mIOC_SrvProtoMethods[i]->OpOnlineService_F(pSrvObj);
                break;
            }
        }
    }

    _IOC_LogNotTested();
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
    }

    _IOC_LogNotTested();
    return IOC_RESULT_SUCCESS;
}
IOC_Result_T IOC_offlineService(
    /*ARG_IN*/ IOC_SrvID_T SrvID) {
    _IOC_LogNotTested();
    return IOC_RESULT_NOT_IMPLEMENTED;
}

IOC_Result_T IOC_acceptClient(
    /*ARG_IN*/ IOC_SrvID_T SrvID,
    /*ARG_OUT*/ IOC_LinkID_pT pLinkID,
    /*ARG_IN_OPTIONAL*/ const IOC_Options_pT pOption) {
    _IOC_LogNotTested();
    return IOC_RESULT_NOT_IMPLEMENTED;
}

IOC_Result_T IOC_connectService(
    /*ARG_OUT*/ IOC_LinkID_pT pLinkID,
    /*ARG_IN*/ const IOC_ConnArgs_pT pConnArgs,
    /*ARG_IN_OPTIONAL*/ const IOC_Options_pT pOption) {
    _IOC_LogNotTested();
    return IOC_RESULT_NOT_IMPLEMENTED;
}

IOC_Result_T IOC_closeLink(
    /*ARG_IN*/ IOC_LinkID_T LinkID) {
    _IOC_LogNotTested();
    return IOC_RESULT_NOT_IMPLEMENTED;
}