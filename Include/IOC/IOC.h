// Description: This file is the header file for the MyIOC module.

#include "IOC_Helpers.h"
#include "IOC_Types.h"

#ifndef __INTER_OBJECT_COMMUNICATION_H__
#define __INTER_OBJECT_COMMUNICATION_H__
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief EvtProducer call this API to post an event to the LinkID,
 *  IOC will deliver this event to the corresponding EvtConsumer,
 *    who is subscribing or retriving this event by the LinkID.
 *
 * @param LinkID: the link ID between EvtProducer and EvtConsumer.
 *     RefMore: README_ArchDesign::Object::Link
 * @param pEvtDesc: the event description. IOC will COPY-OUT this event description if return SUCCESS.
 *     RefMore: README_ArchDesign::Concept::MSG::EVT
 * @param pOption: the options for this postEVT.
 *     Supported options: IOC_OPTID_TIMEOUT, IOC_OPTID_SYNC_MODE.
 *
 * @return IOC_RESULT_SUCCESS: postEVT successfully.
 * @return IOC_RESULT_NO_EVENT_CONSUMER: no EvtConsumer for this event.
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

/**
 * @brief EvtConsumer call this API to subscribe an event from the LinkID.
 *  IOC will deliver the event to the EvtConsumer who is subscribing this event by the LinkID.
 *  Here deliver means that IOC will call the callback function of the EvtConsumer specified in the IOC_SubEvtArgs_pT.
 * @param LinkID: the link ID between EvtProducer and EvtConsumer.
 *    RefMore: IOC_LinkID_T in IOC_Types.h
 *    RefMore: README_ArchDesign::Object::Link
 * @param pSubEvtArgs: the arguments for subscribing an event.
 *    RefMore: IOC_SubEvtArgs_T in IOC_Types.h
 * @return IOC_RESULT_SUCCESS: subscribe event successfully.
 *         IOC_RESULT_TOO_MANY_EVENT_CONSUMER: too many EvtConsumer for this event.
 *         IOC_RESULT_CONFLICT_EVENT_CONSUMER: conflict EvtConsumer for this event.
 *              RefUT: UT_ConlesEventMisuse.verifyConflictEvtConsumer_bySubSameFakeEvtSubArgsTwice
 */
IOC_Result_T IOC_subEVT(
    /*ARG_IN*/IOC_LinkID_T LinkID, 
    /*ARG_IN*/const IOC_SubEvtArgs_pT pSubEvtArgs);

#define IOC_subEVT_inConlesMode(pSubEvtArgs) IOC_subEVT(IOC_CONLES_MODE_AUTO_LINK_ID, pSubEvtArgs)

/**
 * @brief EvtConsumer call this API to unsubscribe an event from the LinkID.
 *  IOC will stop delivering the event to the EvtConsumer who is unsubscribing this event by the LinkID.
 *
 * @param LinkID: the link ID between EvtProducer and EvtConsumer.
 * @param pUnsubEvtArgs: the arguments for unsubscribing an event.
 *    RefMore: IOC_UnsubEvtArgs_T in IOC_Types.h
 * @return IOC_RESULT_SUCCESS: unsubscribe event successfully.
 *         IOC_RESULT_NO_EVENT_CONSUMER: no EvtConsumer for this event.
 *              RefUT: UT_ConlesEventMisuse.verifyNoEvtConsumer_byUnsubEvtWithFakeUnsubArgs
 * @attention
 *   According to the LinkState, if the LinkState is Busy, IOC may block unsubEVT until the LinkState is Ready.
 */
IOC_Result_T IOC_unsubEVT(
    /*ARG_IN*/ IOC_LinkID_T LinkID,
    /*ARG_IN_OPTIONAL*/ const IOC_UnsubEvtArgs_pT pUnsubEvtArgs);

#define IOC_unsubEVT_inConlesMode(pUnsubEvtArgs) IOC_unsubEVT(IOC_CONLES_MODE_AUTO_LINK_ID, pUnsubEvtArgs)

/**
 * @brief force IOC to process the event immediately.
 *  when return from this API, all pending events will be processed.
 * @attention
 *  So, this API is a blocking API, and IOC may use current thread to process the event.
 */
void IOC_forceProcEVT(void);
// TODO: add IOC_forceProcEVT_byLinkID
// TODO: add IOC_forceProcEVT_withOptionTimeout

/**
 * @brief get IOC's capability by CapID in pCapDesc.
 * @param pCapDesc: the capability description.
 *    RefMore: IOC_CapabiltyDescription_T in IOC_Types.h
 * @return IOC_RESULT_SUCCESS: get capability successfully.
 * @return IOC_RESULT_NOT_SUPPORT: the CapID is not supported.
 *
 * @attention
 *  IOC's capability is static or update once when call IOC_initModule.
 */
IOC_Result_T IOC_getCapabilty(
    /*ARG_INOUT*/ IOC_CapabiltyDescription_pT pCapDesc);

//===>Helper APIs for UnitTesting && Debugging=========================================================================
/**
 * @brief get LinkID's MainState and SubState.
 *      This is a helper API mainly used for UnitTesting && Debugging.
 *    RefMore: IOC_LinkState_T in IOC_Types.h
 *    RefMore: README_ArchDesign::State
 * @param LinkID: the link ID to get the state.
 * @param pLinkMainState: to store the main state of the LinkID.
 * @param pLinkSubState: to store the sub state of the LinkID.
 * @return IOC_RESULT_SUCCESS: get the state successfully.
 * @return IOC_RESULT_NOT_EXIST_LINK: the LinkID is not exist.
 */
IOC_Result_T IOC_getLinkState(
    /*ARG_IN*/ IOC_LinkID_T LinkID,
    /*ARG_OUT*/ IOC_LinkState_pT pLinkMainState,
    /*ARG_OUT_OPTIONAL*/ IOC_LinkSubState_pT pLinkSubState);

#define IOC_BugAbort()                                   \
  do {                                                   \
    fprintf(stderr, "BUG: %s:%d\n", __FILE__, __LINE__); \
    abort();                                             \
  } while (0)
//<===Helper APIs for UnitTesting && Debugging==========================================================================

#ifdef __cplusplus
}
#endif
#endif//__INTER_OBJECT_COMMUNICATION_H__
