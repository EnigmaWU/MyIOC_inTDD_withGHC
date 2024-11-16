/**
 * @file _IOC_ConetEvent.h
 * @brief This is ConetEvent internal header file, which is included by _IOC.h only.
 *
 */

#include "_IOC_Types.h"

#ifndef __INTER_OBJECT_COMMUNICATION_CONETEVENT_H__
#define __INTER_OBJECT_COMMUNICATION_CONETEVENT_H__
#ifdef __cplusplus
extern "C" {
#endif

IOC_Result_T _IOC_subEVT_inConetMode(
    /*ARG_IN*/ IOC_LinkID_T LinkID,
    /*ARG_IN*/ const IOC_SubEvtArgs_pT pSubEvtArgs);
IOC_Result_T _IOC_unsubEVT_inConetMode(
    /*ARG_IN*/ IOC_LinkID_T LinkID,
    /*ARG_IN*/ const IOC_UnsubEvtArgs_pT pUnsubEvtArgs);

IOC_Result_T _IOC_postEVT_inConetMode(
    /*ARG_IN*/ IOC_LinkID_T LinkID,
    /*ARG_IN*/ const IOC_EvtDesc_pT pEvtDesc,
    /*ARG_IN_OPTIONAL*/ const IOC_Options_pT pOption);

#ifdef __cplusplus
}
#endif
#endif  //__INTER_OBJECT_COMMUNICATION_CONETEVENT_H__