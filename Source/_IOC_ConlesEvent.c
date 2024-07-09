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
#include <sys/_pthread/_pthread_mutex_t.h>

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

static void __IOC_cbProcEvtClsEvtSuberList(_ClsEvtSuberList_pT pEvtSuberList, IOC_EvtDesc_pT pEvtDesc);

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief DataType of ClsEvtLinkObj
 */
typedef struct {
  IOC_LinkID_T LinkID;  // AutoLinkID = IOC_CONLES_MODE_AUTO_LINK_ID/...

  /**
   * @brief each postEVT to this LinkID will wakeup by
   */
  pthread_mutex_t Mutex;  // Used to protect EvtLinkObj

  pthread_cond_t Cond;        // Used to wakeup EvtProcThread
  pthread_mutex_t CondMutex;  // Used to protect Cond

  /**
   * @brief each LinkObj has a thread to procEvt=call each EvtSuber's CbProcEvt_F if EvtID matched.
   */
  pthread_t ThreadID;

  _IOC_EvtDescQueue_T EvtDescQueue;
  _ClsEvtSuberList_T EvtSuberList;
} _ClsEvtLinkObj_T, *_ClsEvtLinkObj_pT;

static void __IOC_wakeupClsEvtLinkObj(_ClsEvtLinkObj_pT pLinkObj);
static void __IOC_waitClsEvtLinkObj(_ClsEvtLinkObj_pT pLinkObj);

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

//---------------------------------------------------------------------------------------------------------------------
//===> BEGIN IMPLEMENT FOR ClsEvtSuberList
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

static void __IOC_cbProcEvtClsEvtSuberList(_ClsEvtSuberList_pT pEvtSuberList, IOC_EvtDesc_pT pEvtDesc) {
  pthread_mutex_lock(&pEvtSuberList->Mutex);

  for (ULONG_T i = 0; i < _CONLES_EVENT_MAX_SUBSCRIBER; i++) {
    _ClsEvtSuber_T *pEvtSuber = &pEvtSuberList->Subers[i];

    if (pEvtSuber->State == Subed) {
      for (ULONG_T j = 0; j < pEvtSuber->Args.EvtNum; j++) {
        if (pEvtDesc->EvtID == pEvtSuber->Args.pEvtIDs[j]) {
          // FIXME: IF ANY CbProcEvt_F STUCK, IT WILL BLOCK THE WHOLE THREAD, SO WE NEED TO HANDLE THIS CASE.
          // TODO: INSTALL A TIMER TO CATCH TIMEOUT, AND OUTPUT LOG TO INDICATE WHICH CbProcEvt_F STUCK.
          pEvtSuber->Args.CbProcEvt_F(pEvtDesc, pEvtSuber->Args.pCbPrivData);
        }
      }
    }
  }

  pthread_mutex_unlock(&pEvtSuberList->Mutex);
}

//===> END IMPLEMENT FOR ClsEvtSuberList
//---------------------------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------------------------
//===> BEGIN IMPLEMENT FOR ClsEvtLinkObj
static _ClsEvtLinkObj_T _mClsEvtLinkObjs[] = {
    {
        .LinkID = IOC_CONLES_MODE_AUTO_LINK_ID,
    },
};

static inline void __IOC_lockClsEvtLinkObj(_ClsEvtLinkObj_T *pLinkObj) { pthread_mutex_lock(&pLinkObj->Mutex); }

static inline void __IOC_unlockClsEvtLinkObj(_ClsEvtLinkObj_T *pLinkObj) { pthread_mutex_unlock(&pLinkObj->Mutex); }

static _ClsEvtLinkObj_pT __IOC_getClsEvtLinkObjLocked(IOC_LinkState_T AutoLinkID) {
  _ClsEvtLinkObj_pT pLinkObj = NULL;
  ULONG_T TotalClsLinkObjNum = IOC_calcArrayElmtCnt(_mClsEvtLinkObjs);

  for (ULONG_T i = 0; i < TotalClsLinkObjNum; i++) {
    pLinkObj = &_mClsEvtLinkObjs[i];
    if (pLinkObj->LinkID == AutoLinkID) {
      __IOC_lockClsEvtLinkObj(pLinkObj);
      return pLinkObj;
    }
  }

  return NULL;
}

static void __IOC_putClsEvtLinkObj(_ClsEvtLinkObj_pT pLinkObj) { __IOC_unlockClsEvtLinkObj(pLinkObj); }

static void __IOC_wakeupClsEvtLinkObj(_ClsEvtLinkObj_pT pLinkObj) {
  pthread_mutex_lock(&pLinkObj->CondMutex);
  pthread_cond_signal(&pLinkObj->Cond);
  pthread_mutex_unlock(&pLinkObj->CondMutex);
}
static void __IOC_waitClsEvtLinkObj(_ClsEvtLinkObj_pT pLinkObj) {
  // wait for signal or timeout 10ms
  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);

  ts.tv_nsec += 10000000;  // 10ms
  ts.tv_sec += ts.tv_nsec / 1000000000;
  ts.tv_nsec %= 1000000000;

  pthread_mutex_lock(&pLinkObj->CondMutex);
  pthread_cond_timedwait(&pLinkObj->Cond, &pLinkObj->CondMutex, &ts);
  pthread_mutex_unlock(&pLinkObj->CondMutex);
}

static void *__IOC_procClsEvtThread(void *arg) {
  _ClsEvtLinkObj_pT pLinkObj = (_ClsEvtLinkObj_pT)arg;

  /**
   * Steps:
   *  1) __IOC_waitClsEvtLinkObj
   *  2) __IOC_dequeueEvtDescQueueFirst
   *    |-> if IOC_RESULT_EVTDESC_QUEUE_EMPTY, goto 1)
   *  3) __IOC_cbProcEvtClsEvtSuberList
   */
  do {
    __IOC_waitClsEvtLinkObj(pLinkObj);

    IOC_EvtDesc_T EvtDesc;
    IOC_Result_T Result = _IOC_dequeueEvtDescQueueFirst(&pLinkObj->EvtDescQueue, &EvtDesc);
    if (Result == IOC_RESULT_EVTDESC_QUEUE_EMPTY) {
      continue;
    }

    __IOC_cbProcEvtClsEvtSuberList(&pLinkObj->EvtSuberList, &EvtDesc);
  } while (0x20240709);

  pthread_exit(NULL);
}

//===> END IMPLEMENT FOR ClsEvtLinkObj
//---------------------------------------------------------------------------------------------------------------------

IOC_Result_T _IOC_subEVT_inConlesMode(
    /*ARG_IN*/ const IOC_SubEvtArgs_pT pSubEvtArgs) {
  _ClsEvtLinkObj_pT pLinkObj = __IOC_getClsEvtLinkObjLocked(IOC_CONLES_MODE_AUTO_LINK_ID);
  if (pLinkObj == NULL) {
    return IOC_RESULT_BUG;
  }

  IOC_Result_T Result = __IOC_addIntoClsEvtSuberList(&pLinkObj->EvtSuberList, pSubEvtArgs);
  __IOC_putClsEvtLinkObj(pLinkObj);

  if (IOC_RESULT_SUCCESS == Result) {
    _IOC_LogDebug("AutoLinkID(%lu) new EvtSuber(CbProcEvt_F=%p,PrivData=%p)", IOC_CONLES_MODE_AUTO_LINK_ID,
                  pSubEvtArgs->CbProcEvt_F, pSubEvtArgs->pCbPrivData);
  } else {
    _IOC_LogWarn("AutoLinkID(%lu) new EvtSuber(CbProcEvt_F=%p,PrivData=%p) failed(%s)", IOC_CONLES_MODE_AUTO_LINK_ID,
                 pSubEvtArgs->CbProcEvt_F, pSubEvtArgs->pCbPrivData, IOC_getResultStr(Result));
  }

  return Result;
}

IOC_Result_T _IOC_unsubEVT_inConlesMode(
    /*ARG_IN*/ const IOC_UnsubEvtArgs_pT pUnsubEvtArgs) {
  _ClsEvtLinkObj_pT pLinkObj = __IOC_getClsEvtLinkObjLocked(IOC_CONLES_MODE_AUTO_LINK_ID);
  if (pLinkObj == NULL) {
    return IOC_RESULT_BUG;
  }

  IOC_Result_T Result = __IOC_removeFromClsEvtSuberList(&pLinkObj->EvtSuberList, pUnsubEvtArgs);
  __IOC_putClsEvtLinkObj(pLinkObj);

  if (IOC_RESULT_SUCCESS == Result) {
    _IOC_LogDebug("AutoLinkID(%lu) remove EvtSuber(CbProcEvt_F=%p,PrivData=%p)", IOC_CONLES_MODE_AUTO_LINK_ID,
                  pUnsubEvtArgs->CbProcEvt_F, pUnsubEvtArgs->pCbPrivData);
  } else {
    _IOC_LogWarn("AutoLinkID(%lu) remove EvtSuber(CbProcEvt_F=%p,PrivData=%p) failed(%s)", IOC_CONLES_MODE_AUTO_LINK_ID,
                 pUnsubEvtArgs->CbProcEvt_F, pUnsubEvtArgs->pCbPrivData, IOC_getResultStr(Result));
  }

  return Result;
}

IOC_Result_T _IOC_postEVT_inConlesMode(
    /*ARG_IN*/ IOC_LinkID_T LinkID,
    /*ARG_IN*/ const IOC_EvtDesc_pT pEvtDesc,
    /*ARG_IN_OPTIONAL*/ const IOC_Options_pT pOption) {
  if (LinkID != IOC_CONLES_MODE_AUTO_LINK_ID) {
    return IOC_RESULT_NOT_EXIST_LINK;
  }

  _ClsEvtLinkObj_pT pLinkObj = __IOC_getClsEvtLinkObjLocked(IOC_CONLES_MODE_AUTO_LINK_ID);
  if (pLinkObj == NULL) {
    return IOC_RESULT_BUG;
  }

  IOC_Result_T Result = _IOC_enqueueEvtDescQueueLast(&pLinkObj->EvtDescQueue, pEvtDesc);
  if (Result == IOC_RESULT_SUCCESS) {
    __IOC_wakeupClsEvtLinkObj(pLinkObj);
  }

  __IOC_putClsEvtLinkObj(pLinkObj);
}
//======>>>>>>END OF IMPLEMENT FOR ConlesEvent>>>>>>===================================================================