#include "IOC_EvtDesc.h"
#include "IOC_Option.h"
#include "IOC_SrvTypes.h"

#ifndef __IOC_EVENT_API_H__
#define __IOC_EVENT_API_H__
#ifdef __cplusplus
extern "C" {
#endif

typedef IOC_Result_T (*IOC_CbProcEvt_F)(IOC_EvtDesc_pT pEvtDesc, void *pCbPriv);
typedef struct {
    /**
     * @brief CbProcEvt_F + pCbPrivData is used to IDENTIFY one EvtConsumer.
     *    RefMore: IOC_UnsubEvtArgs_T
     */
    IOC_CbProcEvt_F CbProcEvt_F;  // Callback function for processing the event
    void *pCbPrivData;            // Callback private context data

    ULONG_T EvtNum;        // number of EvtIDs, IOC_calcArrayElmtCnt(SubEvtIDs)
    IOC_EvtID_T *pEvtIDs;  // EvtIDs to subscribe

    // TODO: AutoLinkID
} IOC_SubEvtArgs_T, *IOC_SubEvtArgs_pT;

typedef struct {
    IOC_CbProcEvt_F CbProcEvt_F;
    union {
        void *pCbPrivData;
        void *pCbPriv;  // Deprecated
    };
} IOC_UnsubEvtArgs_T, *IOC_UnsubEvtArgs_pT;

/**
 * @brief EvtProducer call this API to post an event to the LinkID,
 *  IOC will deliver this event to the corresponding EvtConsumer,
 *    who is subscribing or retriving this event by the LinkID.
 *
 * @param LinkID: the link ID between EvtProducer and EvtConsumer.
 *     RefMore: README_ArchDesign::Object::Link
 * @param pEvtDesc: the event description. IOC will COPY-OUT this event description if return
 * SUCCESS. RefMore: README_ArchDesign::Concept::MSG::EVT
 * @param pOption: the options for this postEVT.
 *     Supported options: IOC_OPTID_TIMEOUT, IOC_OPTID_SYNC_MODE.
 *
 * @return IOC_RESULT_SUCCESS: postEVT successfully.
 * @return IOC_RESULT_NO_EVENT_CONSUMER: no EvtConsumer for this event.
 *      RefUT: UT_ConlesEventMisuse.Case01_verifyNoEvtConsumer_byNotSubEvtButPostEvtDirectly
 * @return IOC_RESULT_TOO_MANY_QUEUING_EVTDESC: too many event description in IOC's queue.
 * @return IOC_RESULT_TOO_LONG_EMPTYING_EVTDESC_QUEUE: too long time to emptying the event description
 *      RefUT: UT_ConlesEventXXX
 * @return IOC_RESULT_XXX
 * @return IOC_RESULT_BUG: IOC internal bug.
 *
 * @note This API is thread-safe.
 *
 */
IOC_Result_T IOC_postEVT(
    /*ARG_IN*/ IOC_LinkID_T LinkID,
    /*ARG_IN*/ const IOC_EvtDesc_pT pEvtDesc,
    /*ARG_IN_OPTIONAL*/ IOC_Options_pT);

#define IOC_postEVT_inConlesMode(pEvtDesc, pOption) IOC_postEVT(IOC_CONLES_MODE_AUTO_LINK_ID, pEvtDesc, pOption)

IOC_Result_T IOC_broadcastEVT(
    /*ARG_IN*/ IOC_SrvID_T SrvID,
    /*ARG_IN*/ const IOC_EvtDesc_pT pEvtDesc,
    /*ARG_IN_OPTIONAL*/ IOC_Options_pT pOption);

/**
 * @brief EvtConsumer call this API to subscribe an event from the LinkID.
 *  IOC will deliver the event to the EvtConsumer who is subscribing this event by the LinkID.
 *  Here deliver means that IOC will call the callback function of the EvtConsumer specified in the
 * IOC_SubEvtArgs_pT.
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
    /*ARG_IN*/ IOC_LinkID_T LinkID,
    /*ARG_IN*/ const IOC_SubEvtArgs_pT pSubEvtArgs);

#define IOC_subEVT_inConlesMode(pSubEvtArgs) IOC_subEVT(IOC_CONLES_MODE_AUTO_LINK_ID, pSubEvtArgs)

/**
 * @brief EvtConsumer call this API to unsubscribe an event from the LinkID.
 *  IOC will stop delivering the event to the EvtConsumer who is unsubscribing this event by the
 * LinkID.
 *
 * @param LinkID: the link ID between EvtProducer and EvtConsumer.
 * @param pUnsubEvtArgs: the arguments for unsubscribing an event.
 *    RefMore: IOC_UnsubEvtArgs_T in IOC_Types.h
 * @return IOC_RESULT_SUCCESS: unsubscribe event successfully.
 *         IOC_RESULT_NO_EVENT_CONSUMER: no EvtConsumer for this event.
 *              RefUT: UT_ConlesEventMisuse.verifyNoEvtConsumer_byUnsubEvtWithFakeUnsubArgs
 * @attention
 *   According to the LinkState, if the LinkState is Busy, IOC may block unsubEVT until the
 * LinkState is Ready.
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
void IOC_wakeupProcEVT(void);
// TODO: add IOC_forceProcEVT_byLinkID
// TODO: add IOC_forceProcEVT_withOptionTimeout
#ifdef __cplusplus
}
#endif
#endif  // __IOC_EVENT_API_H__