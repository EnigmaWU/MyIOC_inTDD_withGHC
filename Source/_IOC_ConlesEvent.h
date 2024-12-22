/**
 * @file _IOC_ConlesEvent.h
 * @brief This is ConlesEvent internal header file, which is included by _IOC.h only.
 * @attention
 *   - LinkID in ConlesMode is predefined as IOC_CONLES_MODE_AUTO_LINK_ID(_0/_1/_2/...) in IOC_Types.h
 *      Currently, only IOC_CONLES_MODE_AUTO_LINK_ID is supported, and other values are reserved for future use.
 * @version
 *   - SPECv2
 */

#include "_IOC.h"

#ifndef __INTER_OBJECT_COMMUNICATION_CONLESEVENT_H__
#define __INTER_OBJECT_COMMUNICATION_CONLESEVENT_H__
#ifdef __cplusplus
extern "C" {
#endif

IOC_Result_T _IOC_subEVT_inConlesMode(
    /*ARG_IN*/ const IOC_SubEvtArgs_pT pSubEvtArgs);

IOC_Result_T _IOC_unsubEVT_inConlesMode(
    /*ARG_IN*/ const IOC_UnsubEvtArgs_pT pUnsubEvtArgs);

IOC_Result_T _IOC_postEVT_inConlesMode(
    /*ARG_IN*/ IOC_LinkID_T LinkID,
    /*ARG_IN*/ const IOC_EvtDesc_pT pEvtDesc,
    /*ARG_IN_OPTIONAL*/ const IOC_Options_pT pOption);

IOC_Result_T _IOC_getLinkState_inConlesMode(
    /*ARG_IN*/ IOC_LinkID_T LinkID,
    /*ARG_OUT*/ IOC_LinkState_pT pLinkState,
    /*ARG_OUT_OPTIONAL*/ IOC_LinkSubState_pT pLinkSubState);

IOC_Result_T _IOC_getCapability_inConlesMode(
    /*ARG_INOUT*/ IOC_CapabiltyDescription_pT pCapDesc);

void _IOC_forceProcEvt_inConlesMode(void);
void _IOC_wakeupProcEvt_inConlesMode(void);

IOC_BoolResult_T _IOC_isAutoLink_inConlesMode(
    /*ARG_IN*/ IOC_LinkID_T LinkID);

#ifdef __cplusplus
}
#endif
#endif  //__INTER_OBJECT_COMMUNICATION_CONLESEVENT_H__
