#include "_IOC.h"

IOC_Result_T _IOC_subEVT_inConetMode(
    /*ARG_IN*/ IOC_LinkID_T LinkID,
    /*ARG_IN*/ const IOC_SubEvtArgs_pT pSubEvtArgs) {
    _IOC_LinkObject_pT pLinkObj = _IOC_getLinkObjByLinkID(LinkID);
    if (pLinkObj == NULL) {
        _IOC_LogError("Failed to get LinkObj by LinkID(%" PRIu64 ")", LinkID);
        return IOC_RESULT_NOT_EXIST_LINK;
    }

    _IOC_LogAssert(pLinkObj->pMethods != NULL);
    _IOC_LogAssert(pLinkObj->pMethods->OpSubEvt_F != NULL);

    IOC_Result_T Result = pLinkObj->pMethods->OpSubEvt_F(pLinkObj, pSubEvtArgs);
    if (Result != IOC_RESULT_SUCCESS) {
        _IOC_LogError("Failed to subEVT by protocol, Result=%d", Result);
        return Result;
    }

    //_IOC_LogNotTested();
    return IOC_RESULT_SUCCESS;
}

IOC_Result_T _IOC_unsubEVT_inConetMode(
    /*ARG_IN*/ IOC_LinkID_T LinkID,
    /*ARG_IN*/ const IOC_UnsubEvtArgs_pT pUnsubEvtArgs) {
    _IOC_LinkObject_pT pLinkObj = _IOC_getLinkObjByLinkID(LinkID);
    if (pLinkObj == NULL) {
        _IOC_LogError("Failed to get LinkObj by LinkID(%" PRIu64 ")", LinkID);
        return IOC_RESULT_NOT_EXIST_LINK;
    }

    _IOC_LogAssert(pLinkObj->pMethods != NULL);
    _IOC_LogAssert(pLinkObj->pMethods->OpUnsubEvt_F != NULL);

    IOC_Result_T Result = pLinkObj->pMethods->OpUnsubEvt_F(pLinkObj, pUnsubEvtArgs);
    if (Result != IOC_RESULT_SUCCESS) {
        _IOC_LogError("Failed to unsubEVT by protocol, Result=%d", Result);
        return Result;
    }

    //_IOC_LogNotTested();
    return IOC_RESULT_SUCCESS;
}

IOC_Result_T _IOC_postEVT_inConetMode(
    /*ARG_IN*/ IOC_LinkID_T LinkID,
    /*ARG_IN*/ const IOC_EvtDesc_pT pEvtDesc,
    /*ARG_IN_OPTIONAL*/ const IOC_Options_pT pOption) {
    _IOC_LinkObject_pT pLinkObj = _IOC_getLinkObjByLinkID(LinkID);
    if (pLinkObj == NULL) {
        _IOC_LogError("Failed to get LinkObj by LinkID(%" PRIu64 ")", LinkID);
        return IOC_RESULT_NOT_EXIST_LINK;
    }

    _IOC_LogAssert(pLinkObj->pMethods != NULL);
    _IOC_LogAssert(pLinkObj->pMethods->OpPostEvt_F != NULL);

    IOC_Result_T Result = pLinkObj->pMethods->OpPostEvt_F(pLinkObj, pEvtDesc, pOption);
    if (Result != IOC_RESULT_SUCCESS) {
        _IOC_LogError("Failed to postEVT by protocol, Result=%d", Result);
        return Result;
    }

    //_IOC_LogNotTested();
    return IOC_RESULT_SUCCESS;
}