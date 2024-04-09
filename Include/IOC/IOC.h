// Description: This file is the header file for the MyIOC module.

#include "IOC_Types.h"

#ifndef __INTER_OBJECT_COMMUNICATION_H__
#define __INTER_OBJECT_COMMUNICATION_H__
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief EvtProducer call this API to post an event to IOC, which will be processed by EvtConsumer(s).
 *
 * @param LinkID: the link ID between EvtProducer and EvtConsumer.
 *     RefMore: README_ArchDesign::Object::Link
 * @param pEvtDesc: the event description. IOC will COPY-OUT this event description if return SUCCESS.
 *     RefMore: README_ArchDesign::Concept::MSG::EVT
 * @param pOption: the options for this postEVT.
 *     Supported options: IOC_OPTID_TIMEOUT, IOC_OPTID_SYNC_MODE.
 *
 * @return IOC_RESULT_SUCCESS: postEVT successfully.
 * @return IOC_RESULT_NO_EvtConsumer: no EvtConsumer for this event.
 *      RefUT: UT_ConlesEventMisuse.Case01_verifyNoEvtConsumer_byNotSubEvtButPostEvtDirectly
 * @return IOC_RESULT_TOO_MANY_QUEUING_EVTDESC: too many event description in IOC's queue.
 *      RefUT: UT_ConlesEventXXX
 * @return IOC_RESULT_XXX
 * @return IOC_RESULT_BUG: IOC internal bug.
 *
 * @note This API is thread-safe.
 *
 */
IOC_Result_T IOC_postEVT(
    /*ARG_IN*/IOC_LinkID_T LinkID, 
    /*ARG_IN*/const IOC_EvtDesc_pT pEvtDesc,
    /*ARG_IN_OPTIONAL*/IOC_Options_pT);

#define IOC_postEVT_inConlesMode(pEvtDesc, pOption) IOC_postEVT(IOC_CONLES_MODE_AUTO_LINK_ID, pEvtDesc, pOption)

IOC_Result_T IOC_subEVT(
    /*ARG_IN*/IOC_LinkID_T LinkID, 
    /*ARG_IN*/const IOC_SubEvtArgs_pT pSubEvtArgs);

#define IOC_subEVT_inConlesMode(pSubEvtArgs) IOC_subEVT(IOC_CONLES_MODE_AUTO_LINK_ID, pSubEvtArgs)

IOC_Result_T IOC_unsubEVT(
    /*ARG_IN*/ IOC_LinkID_T LinkID,
    /*ARG_IN_OPTIONAL*/ const IOC_UnsubEvtArgs_pT pUnsubEvtArgs);

#define IOC_unsubEVT_inConlesMode(pUnsubEvtArgs) IOC_unsubEVT(IOC_CONLES_MODE_AUTO_LINK_ID, pUnsubEvtArgs)

void IOC_forceProcEVT(void);
// TODO: add IOC_forceProcEVT_byLinkID

IOC_Result_T IOC_getCapabilty(
    /*ARG_INOUT*/ IOC_CapabiltyDescription_pT);

//===>Helper APIs
#define IOC_BugAbort()                                   \
  do {                                                   \
    fprintf(stderr, "BUG: %s:%d\n", __FILE__, __LINE__); \
    abort();                                             \
  } while (0)

#ifdef __cplusplus
}
#endif
#endif//__INTER_OBJECT_COMMUNICATION_H__
