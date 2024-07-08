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
#include <stdlib.h>
#include <string.h>

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
  pthread_mutex_t Mutex;  // Used to protect QueuedEvtNum, ProcedEvtNum, QueuedEvtDescs
  /**
   * IF QueuedEvtNum == ProcedEvtNum, the queue is empty;
   * WHEN postEVT new EvtDesc, DO copy&save enqueuing EvtDesc in
   *    QueuedEvtDescs[QueuedEvtNum%_CONLES_EVENT_MAX_QUEUING_EVTDESC] and QueuedEvtNum++;
   * WHILE QueuedEvtNum > ProcedEvtNum, DO wakeup/loop the EvtProcThread, read one EvtDesc from
   *    QueuedEvtDescs[ProcedEvtNum%_CONLES_EVENT_MAX_QUEUING_EVTDESC], proc one EvtDesc and ProcedEvtNum++;
   */
  ULONG_T QueuedEvtNum, ProcedEvtNum;  // ULONG_T type is long lone enough to avoid overflow even one event per nanosecond.
  IOC_EvtDesc_T QueuedEvtDescs[_CONLES_EVENT_MAX_QUEUING_EVTDESC];
} _IOC_EvtDescQueue_T, *_IOC_EvtDescQueue_pT;

void _IOC_initEvtDescQueue(_IOC_EvtDescQueue_pT pEvtDescQueue);
void _IOC_deinitEvtDescQueue(_IOC_EvtDescQueue_pT pEvtDescQueue);

// Return: IOC_RESULT_YES or IOC_RESULT_NO
IOC_Result_T _IOC_isEmptyEvtDescQueue(_IOC_EvtDescQueue_pT pEvtDescQueue);

// Return: IOC_RESULT_SUCCESS or IOC_RESULT_TOO_MANY_QUEUING_EVTDESC
IOC_Result_T _IOC_enqueueEvtDescQueueLast(_IOC_EvtDescQueue_pT pEvtDescQueue, /*ARG_IN*/ IOC_EvtDesc_pT pEvtDesc);
// Return: IOC_RESULT_SUCCESS or IOC_RESULT_EVENT_QUEUE_EMPTY
IOC_Result_T _IOC_dequeueEvtDescQueueFirst(_IOC_EvtDescQueue_pT pEvtDescQueue, /*ARG_OUT*/ IOC_EvtDesc_pT pEvtDesc);
//---------------------------------------------------------------------------------------------------------------------
typedef enum {
  UnSubed = 0,
  Subed   = 1,
} _ClsEvtSuberState_T;

/**
 * @brief DataType of one ClsEvtSuber
 */
typedef struct {
  _ClsEvtSuberState_T State;
  IOC_SubEvtArgs_T Args;
} _ClsEvtSuber_T, *_ClsEvtSuber_pT;

typedef struct {
  pthread_mutex_t Mutex;  // Used to protect Subers
  ULONG_T SuberNum;
  _ClsEvtSuber_T Subers[_CONLES_EVENT_MAX_SUBSCRIBER];
} _ClsEvtSuberList_T, *_ClsEvtSuberList_pT;

static void __IOC_initClsEvtSuberList(_ClsEvtSuberList_pT pEvtSuberList);
static void __IOC_deinitClsEvtSuberList(_ClsEvtSuberList_pT pEvtSuberList);

// Return: IOC_RESULT_SUCCESS or IOC_RESULT_TOO_MANY_EVENT_CONSUMER or IOC_RESULT_CONFLICT_EVENT_CONSUMER
static IOC_Result_T __IOC_addIntoClsEvtSuberList(_ClsEvtSuberList_pT pEvtSuberList, IOC_SubEvtArgs_pT pSubEvtArgs);
// Return: IOC_RESULT_SUCCESS or IOC_RESULT_NO_EVENT_CONSUMER
static IOC_Result_T __IOC_removeFromClsEvtSuberList(_ClsEvtSuberList_pT pEvtSuberList, IOC_UnsubEvtArgs_pT pUnsubEvtArgs);

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief DataType of ClsEvtLinkObj
 */
typedef struct {
} _ClsEvtLinkObj_T, *_ClsEvtLinkObj_pT;

//======>>>>>>END OF DEFINE FOR ConlesEvent>>>>>>======================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//======>>>>>>BEGIN OF IMPLEMENT FOR ConlesEvent>>>>>>=================================================================
void _IOC_initEvtDescQueue(_IOC_EvtDescQueue_pT pEvtDescQueue) {
  pthread_mutex_init(&pEvtDescQueue->Mutex, NULL);
  pEvtDescQueue->QueuedEvtNum = 0;
  pEvtDescQueue->ProcedEvtNum = 0;

  // clear all EvtDesc in QueuedEvtDescs
  memset(pEvtDescQueue->QueuedEvtDescs, 0, sizeof(pEvtDescQueue->QueuedEvtDescs));
}

void _IOC_deinitEvtDescQueue(_IOC_EvtDescQueue_pT pEvtDescQueue) {
  // deinit all checkable members only

  // check conditions which matching destoriability
  _IOC_LogAssert(pEvtDescQueue->QueuedEvtNum == pEvtDescQueue->ProcedEvtNum);

  int PosixResult = pthread_mutex_destroy(&pEvtDescQueue->Mutex);
  if (PosixResult != 0) {
    _IOC_LogAssert(PosixResult == 0);
  }
}

IOC_Result_T _IOC_isEmptyEvtDescQueue(_IOC_EvtDescQueue_pT pEvtDescQueue) {
  pthread_mutex_lock(&pEvtDescQueue->Mutex);
  IOC_Result_T Result = (pEvtDescQueue->QueuedEvtNum == pEvtDescQueue->ProcedEvtNum) ? IOC_RESULT_YES : IOC_RESULT_NO;
  pthread_mutex_unlock(&pEvtDescQueue->Mutex);
  return Result;
}

IOC_Result_T _IOC_enqueueEvtDescQueueLast(_IOC_EvtDescQueue_pT pEvtDescQueue, IOC_EvtDesc_pT pEvtDesc) {
  pthread_mutex_lock(&pEvtDescQueue->Mutex);

  ULONG_T QueuedEvtNum  = pEvtDescQueue->QueuedEvtNum;
  ULONG_T ProcedEvtNum  = pEvtDescQueue->ProcedEvtNum;
  ULONG_T QueuingEvtNum = QueuedEvtNum - ProcedEvtNum;

  // sanity check QueuingEvtNum
  _IOC_LogAssert(QueuingEvtNum >= 0);
  _IOC_LogAssert(QueuingEvtNum <= _CONLES_EVENT_MAX_QUEUING_EVTDESC);

  if (QueuingEvtNum == _CONLES_EVENT_MAX_QUEUING_EVTDESC) {
    pthread_mutex_unlock(&pEvtDescQueue->Mutex);
    return IOC_RESULT_TOO_MANY_QUEUING_EVTDESC;
  }

  //---------------------------------------------------------------------------
  // sanity check
  _IOC_LogAssert(pEvtDescQueue->QueuedEvtNum >= pEvtDescQueue->ProcedEvtNum);

  ULONG_T NextQueuingPos         = QueuedEvtNum % _CONLES_EVENT_MAX_QUEUING_EVTDESC;
  IOC_EvtDesc_pT pNextQueuingBuf = &pEvtDescQueue->QueuedEvtDescs[NextQueuingPos];

  memcpy(pNextQueuingBuf, pEvtDesc, sizeof(IOC_EvtDesc_T));
  pEvtDescQueue->QueuedEvtNum++;
  QueuedEvtNum = pEvtDescQueue->QueuedEvtNum;
  pthread_mutex_unlock(&pEvtDescQueue->Mutex);

  _IOC_LogDebug("Enqueued EvtDesc(%lu) to EvtDescQueue(Pos=%lu,Proced=%lu <= Queued=%lu)", pEvtDesc->MsgDesc.SeqID,
                NextQueuingPos, ProcedEvtNum, QueuedEvtNum);

  return IOC_RESULT_SUCCESS;
}

IOC_Result_T _IOC_dequeueEvtDescQueueFirst(_IOC_EvtDescQueue_pT pEvtDescQueue, IOC_EvtDesc_pT pEvtDesc) {
  pthread_mutex_lock(&pEvtDescQueue->Mutex);

  ULONG_T QueuedEvtNum  = pEvtDescQueue->QueuedEvtNum;
  ULONG_T ProcedEvtNum  = pEvtDescQueue->ProcedEvtNum;
  ULONG_T QueuingEvtNum = QueuedEvtNum - ProcedEvtNum;

  // sanity check QueuingEvtNum
  _IOC_LogAssert(QueuingEvtNum >= 0);
  _IOC_LogAssert(QueuingEvtNum <= _CONLES_EVENT_MAX_QUEUING_EVTDESC);

  if (QueuingEvtNum == 0) {
    pthread_mutex_unlock(&pEvtDescQueue->Mutex);
    return IOC_RESULT_EVTDESC_QUEUE_EMPTY;
  }

  //---------------------------------------------------------------------------
  ULONG_T NextProcingPos         = ProcedEvtNum % _CONLES_EVENT_MAX_QUEUING_EVTDESC;
  IOC_EvtDesc_pT pNextProcingBuf = &pEvtDescQueue->QueuedEvtDescs[NextProcingPos];

  memcpy(pEvtDesc, pNextProcingBuf, sizeof(IOC_EvtDesc_T));
  pEvtDescQueue->ProcedEvtNum++;
  ProcedEvtNum = pEvtDescQueue->ProcedEvtNum;

  pthread_mutex_unlock(&pEvtDescQueue->Mutex);

  _IOC_LogDebug("Dequeued EvtDesc(%lu) from EvtDescQueue(Pos=%lu,Proced=%lu <= Queued=%lu)", pEvtDesc->MsgDesc.SeqID,
                NextProcingPos, ProcedEvtNum, QueuedEvtNum);

  return IOC_RESULT_SUCCESS;
}

static void __IOC_initClsEvtSuberList(_ClsEvtSuberList_pT pEvtSuberList) {
  pthread_mutex_init(&pEvtSuberList->Mutex, NULL);
  pEvtSuberList->SuberNum = 0;
  memset(pEvtSuberList->Subers, 0, sizeof(pEvtSuberList->Subers));
}
static void __IOC_deinitClsEvtSuberList(_ClsEvtSuberList_pT pEvtSuberList) {
  // deinit all checkable members only

  // check conditions which matching destoriability
  _IOC_LogAssert(pEvtSuberList->SuberNum == 0);

  int PosixResult = pthread_mutex_destroy(&pEvtSuberList->Mutex);
  if (PosixResult != 0) {
    _IOC_LogAssert(PosixResult == 0);
  }
}

// Return: IOC_RESULT_SUCCESS or IOC_RESULT_TOO_MANY_EVENT_CONSUMER or IOC_RESULT_CONFLICT_EVENT_CONSUMER
static IOC_Result_T __IOC_addIntoClsEvtSuberList(_ClsEvtSuberList_pT pEvtSuberList, IOC_SubEvtArgs_pT pSubEvtArgs) {
  pthread_mutex_lock(&pEvtSuberList->Mutex);

  ULONG_T SuberNum = pEvtSuberList->SuberNum;
  if (SuberNum >= _CONLES_EVENT_MAX_SUBSCRIBER) {
    pthread_mutex_unlock(&pEvtSuberList->Mutex);
    return IOC_RESULT_TOO_MANY_EVENT_CONSUMER;
  }

  // check conflict
  for (ULONG_T i = 0; i < _CONLES_EVENT_MAX_SUBSCRIBER; i++) {
    _ClsEvtSuber_T *pEvtSuber = &pEvtSuberList->Subers[i];

    if (pEvtSuber->State == Subed) {
      // RefComments: IOC_SubEvtArgs_T how to identify a EvtConsumer
      if (pEvtSuber->Args.CbProcEvt_F == pSubEvtArgs->CbProcEvt_F && pEvtSuber->Args.pCbPrivData == pSubEvtArgs->pCbPrivData) {
        pthread_mutex_unlock(&pEvtSuberList->Mutex);
        return IOC_RESULT_CONFLICT_EVENT_CONSUMER;
      }
    }
  }

  // forloop to get the first empty slot
  for (ULONG_T i = 0; i < _CONLES_EVENT_MAX_SUBSCRIBER; i++) {
    _ClsEvtSuber_T *pEvtSuber = &pEvtSuberList->Subers[i];

    if (pEvtSuber->State == UnSubed) {
      pEvtSuber->State = Subed;

      // save direct args, and alloc new memory to save indirect EvtIDs
      pEvtSuber->Args.CbProcEvt_F = pSubEvtArgs->CbProcEvt_F;
      pEvtSuber->Args.pCbPrivData = pSubEvtArgs->pCbPrivData;
      pEvtSuber->Args.EvtNum      = pSubEvtArgs->EvtNum;
      pEvtSuber->Args.pEvtIDs     = (IOC_EvtID_T *)malloc(pSubEvtArgs->EvtNum * sizeof(IOC_EvtID_T));
      memcpy(pEvtSuber->Args.pEvtIDs, pSubEvtArgs->pEvtIDs, pSubEvtArgs->EvtNum * sizeof(IOC_EvtID_T));

      pEvtSuberList->SuberNum++;
      break;
    }
  }

  pthread_mutex_unlock(&pEvtSuberList->Mutex);
  return IOC_RESULT_SUCCESS;
}
// Return: IOC_RESULT_SUCCESS or IOC_RESULT_NO_EVENT_CONSUMER
static IOC_Result_T __IOC_removeFromClsEvtSuberList(_ClsEvtSuberList_pT pEvtSuberList, IOC_UnsubEvtArgs_pT pUnsubEvtArgs) {
  pthread_mutex_lock(&pEvtSuberList->Mutex);

  ULONG_T SuberNum = pEvtSuberList->SuberNum;
  if (SuberNum == 0) {
    pthread_mutex_unlock(&pEvtSuberList->Mutex);
    return IOC_RESULT_NO_EVENT_CONSUMER;
  }

  // forloop to find the first match slot
  for (ULONG_T i = 0; i < _CONLES_EVENT_MAX_SUBSCRIBER; i++) {
    _ClsEvtSuber_T *pEvtSuber = &pEvtSuberList->Subers[i];

    if (pEvtSuber->State == Subed) {
      // RefComments: IOC_SubEvtArgs_T how to identify a EvtConsumer
      if (pEvtSuber->Args.CbProcEvt_F == pUnsubEvtArgs->CbProcEvt_F &&
          pEvtSuber->Args.pCbPrivData == pUnsubEvtArgs->pCbPrivData) {
        pEvtSuber->State = UnSubed;

        // free indirect EvtIDs
        free(pEvtSuber->Args.pEvtIDs);

        pEvtSuberList->SuberNum--;
        break;
      }
    }
  }

  pthread_mutex_unlock(&pEvtSuberList->Mutex);
  return IOC_RESULT_SUCCESS;
}

//======>>>>>>END OF IMPLEMENT FOR ConlesEvent>>>>>>===================================================================