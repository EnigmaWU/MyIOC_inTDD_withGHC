/**
 * @file _IOC_ConlesEvent.c
 * @brief This is ConlesEvent internal Design&Define&Implement file.
 *      And this file only implement interfaces defined in _IOC_ConlesEvent.h.
 * @attention
 *  Design is in current file's comments and _IOC_ConlesEvent.md which focus on graph of sequence and state machine.
 *  Define and Implement is in current file's following design comments.
 * @version
 *   - SPECv2
 */

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//======>>>>>>BEGIN OF DESIGN FOR ConlesEvent>>>>>>====================================================================
/**
 * An [AutoLinkID] is a unique predefined LinkID used by a group of EvtProducers and EvtConsumers in ConlesMode.
 *     IOC_CONLES_MODE_AUTO_LINK_ID is support by default, and other values are reserved for future use.
 * Each AutoLinkID corresponds to a [ConlesEventLinkObject](a.k.a ClsEvtLinkObj) as an aggregation of all.
 *
 * Each ClsEvtLinkObj has a [EvtDescQueue] to save all EvtDesc,
 *   which posted by EvtProducers use _IOC_postEVT_inConlesMode by default in async mode.
 *     And the EvtQueue is a FIFO queue, the size is limited by _CONLES_EVENT_MAX_QUEUING_EVTDESC.
 *          (TODO: adjust this static size when _IOC_initModule_inConlesMode)
 *     Then each EvtDesc in EvtDescQueue will be processed by a [EvtProcThread] for this AutoLinkID.
 *   If EvtProducers use _IOC_postEVT_inConlesMode in sync mode,
 *      the EvtDesc will be processed immediately if EvtQueue is empty, or wait until the EvtQueue is empty.
 *
 * A [ConlesEventSubscriber](a.k.a ClsEvtSuber) is a event subscriber who call _IOC_subEVT_inConlesMode
 *     with AutoLinkID and IOC_SubEvtArgs_T, and finally get an IOC_RESULT_SUCCESS.
 *     And he may call _IOC_unsubEVT_inConlesMode to unsubscribe the event any time.
 * Each ClsEvtSuber is saved in a [EvtSuberList] identified by AutoLinkID and only belongs to this AutoLinkID.
 *      And the EvtSuberList's size is limited by _CONLES_EVENT_MAX_SUBSCRIBER.
 *          (TODO: adjust this static size when _IOC_initModule_inConlesMode)
 *
 * RefDiagram: _IOC_ConlesEvent.md
 */
//======>>>>>>END OF DESIGN FOR ConlesEvent>>>>>>======================================================================

#include "_IOC_ConlesEvent.h"

#include <pthread.h>

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//======>>>>>>BEGIN OF DEFINE FOR ConlesEvent>>>>>>====================================================================
#define _CONLES_EVENT_MAX_SUBSCRIBER 16
#define _CONLES_EVENT_MAX_QUEUING_EVTDESC 32

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief DataType of EvtDescQueue
 *    A FIFO queue to save all EvtDesc.
 * @note
 *    May this queue be IOC's common queue in future?
 */
typedef struct _EvtDescQueueStru {
  pthread_mutex_t Mutex;
  /**
   * IF QueuedEvtNum == ProcedEvtNum, the queue is empty;
   * IF new postEVT, THEN alloc&copy a new pEvtDesc, AND save in
   *    pQueuedEvtDescs[QueuedEvtNum%_IOC_CONLES_MODE_MAX_QUEUING_EVTDESC_NUMBER] and QueuedEvtNum++;
   * IF QueuedEvtNum > ProcedEvtNum, THEN wakeup/loop the ConlesModeEvtCbThread, read pEvtDesc from
   *    pQueuedEvtDescs[ProcedEvtNum%_IOC_CONLES_MODE_MAX_QUEUING_EVTDESC_NUMBER], proc&free the pEvtDesc, and ProcedEvtNum++;
   */
  ULONG_T QueuedEvtNum, ProcedEvtNum;
  IOC_EvtDesc_pT pQueuedEvtDescs[_CONLES_EVENT_MAX_QUEUING_EVTDESC];
} _EvtDescQueue_T, *_EvtDescQueue_pT;

void _IOC_initEvtDescQueue(_EvtDescQueue_pT pEvtDescQueue);
void _IOC_deinitEvtDescQueue(_EvtDescQueue_pT pEvtDescQueue);

IOC_Result_T _IOC_isEmptyEvtDescQueue(_EvtDescQueue_pT pEvtDescQueue);

// Return: IOC_RESULT_SUCCESS or IOC_RESULT_TOO_MANY_QUEUING_EVTDESC
IOC_Result_T _IOC_enqueueEvtDescQueueLast(_EvtDescQueue_pT pEvtDescQueue, IOC_EvtDesc_pT pEvtDesc);
// Return: IOC_RESULT_SUCCESS or IOC_RESULT_EVENT_QUEUE_EMPTY
IOC_Result_T _IOC_dequeueEvtDescQueueFirst(_EvtDescQueue_pT pEvtDescQueue, IOC_EvtDesc_pT *ppEvtDesc);
//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief DataType of ClsEvtSuber
 */
typedef struct {
} _ClsEvtSuber_T, *_ClsEvtSuber_pT;

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief DataType of ClsEvtLinkObj
 */
typedef struct {
} _ClsEvtLinkObj_T, *_ClsEvtLinkObj_pT;

//======>>>>>>END OF DEFINE FOR ConlesEvent>>>>>>======================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//======>>>>>>BEGIN OF IMPLEMENT FOR ConlesEvent>>>>>>=================================================================

//======>>>>>>END OF IMPLEMENT FOR ConlesEvent>>>>>>===================================================================