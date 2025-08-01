#include <stdatomic.h>

#include "_IOC.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IOC_Result_T IOC_postEVT(
    /*ARG_IN*/ IOC_LinkID_T LinkID,
    /*ARG_IN*/ const IOC_EvtDesc_pT pEvtDesc,
    /*ARG_IN_OPTIONAL*/ IOC_Options_pT pOptions) {
    if (NULL == pEvtDesc) {
        return IOC_RESULT_INVALID_PARAM;
    }

    //---------------------------------------------------------------------------
    //===>>>Set EvtDesc's metadata: SeqID, TimeStamp
    //=>[SeqID]: define a static atomic variable to keep SeqID++
    static atomic_ulong SeqID = 0;
    atomic_fetch_add(&SeqID, 1);
    pEvtDesc->MsgDesc.SeqID = SeqID;
    //=>[TimeStamp]
    pEvtDesc->MsgDesc.TimeStamp = IOC_getCurrentTimeSpec();

    //---------------------------------------------------------------------------
    if (IOC_RESULT_YES == _IOC_isAutoLink_inConlesMode(LinkID)) {
        return _IOC_postEVT_inConlesMode(LinkID, pEvtDesc, pOptions);
    } else {
        return _IOC_postEVT_inConetMode(LinkID, pEvtDesc, pOptions);
    }
}

IOC_Result_T IOC_subEVT(
    /*ARG_IN*/ IOC_LinkID_T LinkID,
    /*ARG_IN*/ const IOC_SubEvtArgs_pT pSubEvtArgs) {
    if (IOC_RESULT_YES == _IOC_isAutoLink_inConlesMode(LinkID)) {
        return _IOC_subEVT_inConlesMode(pSubEvtArgs);
    } else {
        return _IOC_subEVT_inConetMode(LinkID, pSubEvtArgs);
    }
}

IOC_Result_T IOC_unsubEVT(
    /*ARG_IN*/ IOC_LinkID_T LinkID,
    /*ARG_IN*/ const IOC_UnsubEvtArgs_pT pUnsubEvtArgs) {
    if (IOC_RESULT_YES == _IOC_isAutoLink_inConlesMode(LinkID)) {
        return _IOC_unsubEVT_inConlesMode(pUnsubEvtArgs);
    } else {
        return _IOC_unsubEVT_inConetMode(LinkID, pUnsubEvtArgs);
    }
}

void IOC_forceProcEVT(void) {
    _IOC_forceProcEvt_inConlesMode();
    // TODO: forceProcEvt_inConetMode
}

void IOC_wakeupProcEVT(void) {
    _IOC_wakeupProcEvt_inConlesMode();
    // TODO: wakeupProcEvt_inConetMode
}

IOC_Result_T IOC_getLinkState(
    /*ARG_IN*/ IOC_LinkID_T LinkID,
    /*ARG_OUT*/ IOC_LinkState_pT pLinkState,
    /*ARG_OUT_OPTIONAL*/ IOC_LinkSubState_pT pLinkSubState) {
    if (IOC_RESULT_YES == _IOC_isAutoLink_inConlesMode(LinkID)) {
        return _IOC_getLinkState_inConlesMode(LinkID, pLinkState, pLinkSubState);
    } else {
        // 🎯 TDD IMPLEMENTATION: Now supporting Conet Mode for DAT links
        return _IOC_getLinkState_inConetMode(LinkID, pLinkState, pLinkSubState);
    }
}
