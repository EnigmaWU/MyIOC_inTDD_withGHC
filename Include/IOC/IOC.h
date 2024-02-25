// Description: This file is the header file for the MyIOC module.

#include "IOC_Types.h"

#ifndef __INTER_OBJECT_COMMUNICATION_H__
#define __INTER_OBJECT_COMMUNICATION_H__
#ifdef __cplusplus
extern "C" {
#endif

IOC_Result_T IOC_postEVT(
    /*ARG_IN*/IOC_LinkID_T LinkID, 
    /*ARG_IN*/const IOC_EvtDesc_pT pEvtDesc,
    /*ARG_IN_OPTIONAL*/IOC_Options_pT);

#define IOC_postEVT_inConlesMode(pEvtDesc, pOption) PLT_IOC_postEVT(IOC_CONLES_MODE_AUTO_LINK_ID, pEvtDesc, pOption)

IOC_Result_T IOC_subEVT(
    /*ARG_IN*/IOC_LinkID_T LinkID, 
    /*ARG_IN*/const IOC_SubEvtArgs_pT pSubEvtArgs);

#define IOC_subEVT_inConlesMode(pSubEvtArgs) PLT_IOC_subEVT(IOC_CONLES_MODE_AUTO_LINK_ID, pSubEvtArgs)

IOC_Result_T IOC_unsubEVT(
    /*ARG_IN*/ IOC_LinkID_T LinkID,
    /*ARG_IN_OPTIONAL*/ const IOC_UnsubEvtArgs_pT pUnsubEvtArgs);

#define IOC_unsubEVT_inConlesMode(pUnsubEvtArgs) IOC_unsubEVT(IOC_CONLES_MODE_AUTO_LINK_ID, pUnsubEvtArgs)

#ifdef __cplusplus
}
#endif
#endif//__INTER_OBJECT_COMMUNICATION_H__
