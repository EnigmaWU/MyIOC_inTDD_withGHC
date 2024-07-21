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

#include <stdbool.h>

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//======>>>>>>BEGIN OF DEFINE FOR ConlesEvent>>>>>>====================================================================
#define _CONLES_EVENT_MAX_SUBSCRIBER 16
#define _CONLES_EVENT_MAX_QUEUING_EVTDESC 64

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
  atomic_ulong SuberNum;
  _ClsEvtSuber_T Subers[_CONLES_EVENT_MAX_SUBSCRIBER];
} _ClsEvtSuberList_T, *_ClsEvtSuberList_pT;

static void __IOC_initClsEvtSuberList(_ClsEvtSuberList_pT pEvtSuberList);
static void __IOC_deinitClsEvtSuberList(_ClsEvtSuberList_pT pEvtSuberList);

// Return: IOC_RESULT_SUCCESS or IOC_RESULT_TOO_MANY_EVENT_CONSUMER or IOC_RESULT_CONFLICT_EVENT_CONSUMER
static IOC_Result_T __IOC_insertClsEvtSuberList(_ClsEvtSuberList_pT pEvtSuberList, IOC_SubEvtArgs_pT pSubEvtArgs);
// Return: IOC_RESULT_SUCCESS or IOC_RESULT_NO_EVENT_CONSUMER
static IOC_Result_T __IOC_removeClsEvtSuberList(_ClsEvtSuberList_pT pEvtSuberList, IOC_UnsubEvtArgs_pT pUnsubEvtArgs);

// Return: IOC_RESULT_YES or IOC_RESULT_NO
static IOC_Result_T __IOC_isEmptyClsEvtSuberList(_ClsEvtSuberList_pT pEvtSuberList);

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

  /**
   * RefState: README_ArchDesign.md
   *    |-> State
   *      |-> EVT::Conles
   */
  struct {
    pthread_mutex_t Mutex;
    IOC_LinkState_T Main;
    IOC_LinkSubState_T Sub;
  } State;
} _ClsEvtLinkObj_T, *_ClsEvtLinkObj_pT;

// get ClsEvtLinkObj by AutoLinkID, not lock it by Mutex when return, and dont put it after use.
static _ClsEvtLinkObj_pT __IOC_getClsEvtLinkObjNotLocked(IOC_LinkID_T AutoLinkID);

// get ClsEvtLinkObj by AutoLinkID, lock it by Mutex when return, and put it after use.
static _ClsEvtLinkObj_pT __IOC_getClsEvtLinkObjLocked(IOC_LinkID_T AutoLinkID);
static void __IOC_putClsEvtLinkObj(_ClsEvtLinkObj_pT pLinkObj);

static void __IOC_wakeupClsEvtLinkObj(_ClsEvtLinkObj_pT pLinkObj);
static void __IOC_waitClsEvtLinkObj(_ClsEvtLinkObj_pT pLinkObj);

static void *__IOC_cbProcClsEvtLinkObjThread(void *arg);

// RefDoc: README_ArchDesign.md
//     |-> State
//       |-> EVT::Conles
typedef enum {
  Behavior_enterCbProcEvt = 1,
  Behavior_leaveCbProcEvt = 2,

  Behavior_enterSubEvt = 3,
  Behavior_leaveSubEvt = 4,

  Behavior_enterUnsubEvt = 5,
  Behavior_leaveUnsubEvt = 6,
} _ClsEvtLinkObjBehavior_T;

static void __IOC_transferClsEvtLinkObjStateByBehavior(_ClsEvtLinkObj_pT pLinkObj, _ClsEvtLinkObjBehavior_T Behavior);

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

  //_IOC_LogDebug("Enqueued EvtDesc(SeqID=%lu, EvtID(%lu,%lu)) to EvtDescQueue(Pos=%lu,Proced=%lu <= Queued=%lu)",
  //              pEvtDesc->MsgDesc.SeqID, IOC_getEvtClassID(pEvtDesc->EvtID), IOC_getEvtNameID(pEvtDesc->EvtID),
  //              NextQueuingPos, ProcedEvtNum, QueuedEvtNum);

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

  //_IOC_LogDebug("Dequeued EvtDesc(SeqID=%lu, EvtID(%lu,%lu)) from EvtDescQueue(Pos=%lu,Proced=%lu <= Queued=%lu)",
  //              pEvtDesc->MsgDesc.SeqID, IOC_getEvtClassID(pEvtDesc->EvtID), IOC_getEvtNameID(pEvtDesc->EvtID),
  //              NextProcingPos, ProcedEvtNum, QueuedEvtNum);

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
static IOC_Result_T __IOC_insertClsEvtSuberList(_ClsEvtSuberList_pT pEvtSuberList, IOC_SubEvtArgs_pT pSubEvtArgs) {
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

      // increase SuberNum atomically
      atomic_fetch_add(&pEvtSuberList->SuberNum, 1);
      break;
    }
  }

  pthread_mutex_unlock(&pEvtSuberList->Mutex);
  return IOC_RESULT_SUCCESS;
}
// Return: IOC_RESULT_SUCCESS or IOC_RESULT_NO_EVENT_CONSUMER
static IOC_Result_T __IOC_removeClsEvtSuberList(_ClsEvtSuberList_pT pEvtSuberList, IOC_UnsubEvtArgs_pT pUnsubEvtArgs) {
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

        // decrease SuberNum atomically
        atomic_fetch_sub(&pEvtSuberList->SuberNum, 1);
        break;
      }
    }
  }

  pthread_mutex_unlock(&pEvtSuberList->Mutex);
  return IOC_RESULT_SUCCESS;
}

static IOC_Result_T __IOC_isEmptyClsEvtSuberList(_ClsEvtSuberList_pT pEvtSuberList) {
  // read SuberNum atomically
  ULONG_T SuberNum    = atomic_load(&pEvtSuberList->SuberNum);
  IOC_Result_T Result = (SuberNum == 0) ? IOC_RESULT_YES : IOC_RESULT_NO;
  return Result;
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
/**
 * @brief Only LinkID==IOC_CONLES_MODE_AUTO_LINK_ID is supported now.
 *  TODO: support other LinkID in future of IOC_CONLES_MODE_AUTO_LINK_ID_1/2/3/...
 *
 */
static _ClsEvtLinkObj_T _mClsEvtLinkObjs[] = {
    {
        .LinkID = IOC_CONLES_MODE_AUTO_LINK_ID,
        .State =
            {
                .Main = IOC_LinkStateReady,
                .Sub  = IOC_LinkSubStateDefault,
            },
    },
};

// This is a helper function to get ClsEvtLinkObj by AutoLinkID
// Because the ClsEvtLinkObj is a static array, so we access it directly without lock.
__attribute__((no_sanitize_thread)) static _ClsEvtLinkObj_pT __IOC_getClsEvtLinkObjNotLocked(IOC_LinkID_T AutoLinkID) {
  _ClsEvtLinkObj_pT pLinkObj = NULL;
  ULONG_T TotalClsLinkObjNum = IOC_calcArrayElmtCnt(_mClsEvtLinkObjs);

  for (ULONG_T i = 0; i < TotalClsLinkObjNum; i++) {
    pLinkObj = &_mClsEvtLinkObjs[i];
    if (pLinkObj->LinkID == AutoLinkID) {
      return pLinkObj;
    }
  }

  return NULL;
}

static _ClsEvtLinkObj_pT __IOC_getClsEvtLinkObjLocked(IOC_LinkID_T AutoLinkID) {
  _ClsEvtLinkObj_pT pLinkObj = __IOC_getClsEvtLinkObjNotLocked(AutoLinkID);
  if (pLinkObj == NULL) {
    return NULL;
  }

  pthread_mutex_lock(&pLinkObj->Mutex);
  return pLinkObj;
}

static void __IOC_putClsEvtLinkObj(_ClsEvtLinkObj_pT pLinkObj) { pthread_mutex_unlock(&pLinkObj->Mutex); }

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

static void *__IOC_cbProcClsEvtLinkObjThread(void *arg) {
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

    //-----------------------------------------------------------------------------------------------------------------
    do {
      IOC_EvtDesc_T EvtDesc = {};
      IOC_Result_T Result = _IOC_dequeueEvtDescQueueFirst(&pLinkObj->EvtDescQueue, &EvtDesc);
      if (Result == IOC_RESULT_EVTDESC_QUEUE_EMPTY) {
        break;
      }

      __IOC_transferClsEvtLinkObjStateByBehavior(pLinkObj, Behavior_enterCbProcEvt);
      __IOC_cbProcEvtClsEvtSuberList(&pLinkObj->EvtSuberList, &EvtDesc);
      __IOC_transferClsEvtLinkObjStateByBehavior(pLinkObj, Behavior_leaveCbProcEvt);
    } while (0x20240714);
    //-----------------------------------------------------------------------------------------------------------------

  } while (0x20240709);

  pthread_exit(NULL);
}

// On libc's runtime init, this function will be called to create all ClsEvtLinkObj's EvtProcThread.
__attribute__((constructor)) static void __IOC_initClsEvtLinkObj(void) {
  ULONG_T TotalClsLinkObjNum = IOC_calcArrayElmtCnt(_mClsEvtLinkObjs);

  for (ULONG_T i = 0; i < TotalClsLinkObjNum; i++) {
    _ClsEvtLinkObj_pT pLinkObj = &_mClsEvtLinkObjs[i];

    _IOC_initEvtDescQueue(&pLinkObj->EvtDescQueue);
    __IOC_initClsEvtSuberList(&pLinkObj->EvtSuberList);

    pthread_mutex_init(&pLinkObj->Mutex, NULL);
    pthread_cond_init(&pLinkObj->Cond, NULL);
    pthread_mutex_init(&pLinkObj->CondMutex, NULL);

    pthread_create(&pLinkObj->ThreadID, NULL, __IOC_cbProcClsEvtLinkObjThread, pLinkObj);
  }
}

// RefDoc: README_ArchDesign.md
//     |-> State
//       |-> EVT::Conles
static void __IOC_transferClsEvtLinkObjStateByBehavior(_ClsEvtLinkObj_pT pLinkObj, _ClsEvtLinkObjBehavior_T Behavior) {
  pthread_mutex_lock(&pLinkObj->State.Mutex);
  IOC_LinkState_T MainState = pLinkObj->State.Main;

  switch (Behavior) {
    case Behavior_enterCbProcEvt: {
      if (MainState == IOC_LinkStateReady) {
        pLinkObj->State.Main = IOC_LinkStateBusyCbProcEvt;
      } else {
        _IOC_LogBug("Invalid State(Main=%d) to enterCbProcEvt, MUST in State(Main=%d)", MainState, IOC_LinkStateReady);
      }
    } break;

    case Behavior_leaveCbProcEvt: {
      if (MainState == IOC_LinkStateBusyCbProcEvt) {
        pLinkObj->State.Main = IOC_LinkStateReady;
      } else {
        _IOC_LogBug("Invalid State(Main=%d) to leaveCbProcEvt, MUST in State(Main=%d)", MainState,
                    IOC_LinkStateBusyCbProcEvt);
      }
    } break;

    case Behavior_enterSubEvt: {
      if (MainState == IOC_LinkStateReady) {
        pLinkObj->State.Main = IOC_LinkStateBusySubEvt;
      } else {
        _IOC_LogBug("Invalid State(Main=%d) to enterSubEvt, MUST in State(Main=%d)", MainState, IOC_LinkStateReady);
      }
    } break;

    case Behavior_leaveSubEvt: {
      if (MainState == IOC_LinkStateBusySubEvt) {
        pLinkObj->State.Main = IOC_LinkStateReady;
      } else {
        _IOC_LogBug("Invalid State(Main=%d) to leaveSubEvt, MUST in State(Main=%d)", MainState,
                    IOC_LinkStateBusySubEvt);
      }
    } break;

    case Behavior_enterUnsubEvt: {
      if (MainState == IOC_LinkStateReady) {
        pLinkObj->State.Main = IOC_LinkStateBusyUnsubEvt;
      } else {
        _IOC_LogBug("Invalid State(Main=%d) to enterUnsubEvt, MUST in State(Main=%d)", MainState, IOC_LinkStateReady);
      }
    } break;

    case Behavior_leaveUnsubEvt: {
      if (MainState == IOC_LinkStateBusyUnsubEvt) {
        pLinkObj->State.Main = IOC_LinkStateReady;
      } else {
        _IOC_LogBug("Invalid State(Main=%d) to leaveUnsubEvt, MUST in State(Main=%d)", MainState,
                    IOC_LinkStateBusyUnsubEvt);
      }
    } break;

    default:
      _IOC_LogBug("Invalid Behavior(%d)", Behavior);
  }

  pthread_mutex_unlock(&pLinkObj->State.Mutex);
}

//===> END IMPLEMENT FOR ClsEvtLinkObj
//---------------------------------------------------------------------------------------------------------------------

IOC_Result_T _IOC_isAutoLink_inConlesMode(
    /*ARG_IN*/ IOC_LinkID_T LinkID) {
  return (LinkID == IOC_CONLES_MODE_AUTO_LINK_ID) ? IOC_RESULT_YES : IOC_RESULT_NO;
}

IOC_Result_T _IOC_subEVT_inConlesMode(
    /*ARG_IN*/ const IOC_SubEvtArgs_pT pSubEvtArgs) {
  _ClsEvtLinkObj_pT pLinkObj = __IOC_getClsEvtLinkObjLocked(IOC_CONLES_MODE_AUTO_LINK_ID);
  if (pLinkObj == NULL) {
    return IOC_RESULT_BUG;
  }

  __IOC_transferClsEvtLinkObjStateByBehavior(pLinkObj, Behavior_enterSubEvt);
  IOC_Result_T Result = __IOC_insertClsEvtSuberList(&pLinkObj->EvtSuberList, pSubEvtArgs);
  __IOC_transferClsEvtLinkObjStateByBehavior(pLinkObj, Behavior_leaveSubEvt);

  __IOC_putClsEvtLinkObj(pLinkObj);

  if (IOC_RESULT_SUCCESS == Result) {
    //_IOC_LogDebug("AutoLinkID(%lu) new EvtSuber(CbProcEvt_F=%p,PrivData=%p)", IOC_CONLES_MODE_AUTO_LINK_ID,
    //              pSubEvtArgs->CbProcEvt_F, pSubEvtArgs->pCbPrivData);
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

  __IOC_transferClsEvtLinkObjStateByBehavior(pLinkObj, Behavior_enterUnsubEvt);
  IOC_Result_T Result = __IOC_removeClsEvtSuberList(&pLinkObj->EvtSuberList, pUnsubEvtArgs);
  __IOC_transferClsEvtLinkObjStateByBehavior(pLinkObj, Behavior_leaveUnsubEvt);

  __IOC_putClsEvtLinkObj(pLinkObj);

  if (IOC_RESULT_SUCCESS == Result) {
    //_IOC_LogDebug("AutoLinkID(%lu) remove EvtSuber(CbProcEvt_F=%p,PrivData=%p)", IOC_CONLES_MODE_AUTO_LINK_ID,
    //              pUnsubEvtArgs->CbProcEvt_F, pUnsubEvtArgs->pCbPrivData);
  } else {
    _IOC_LogWarn("AutoLinkID(%lu) remove EvtSuber(CbProcEvt_F=%p,PrivData=%p) failed(%s)", IOC_CONLES_MODE_AUTO_LINK_ID,
                 pUnsubEvtArgs->CbProcEvt_F, pUnsubEvtArgs->pCbPrivData, IOC_getResultStr(Result));
  }

  return Result;
}

IOC_Result_T _IOC_getLinkState_inConlesMode(
    /*ARG_IN*/ IOC_LinkID_T LinkID,
    /*ARG_OUT*/ IOC_LinkState_pT pLinkState,
    /*ARG_OUT_OPTIONAL*/ IOC_LinkSubState_pT pLinkSubState) {
  if (LinkID != IOC_CONLES_MODE_AUTO_LINK_ID) {
    _IOC_LogError("Invalid AutoLinkID(%llu)", LinkID);
    return IOC_RESULT_INVALID_AUTO_LINK_ID;
  }

  _ClsEvtLinkObj_pT pLinkObj = __IOC_getClsEvtLinkObjNotLocked(LinkID);
  if (pLinkObj == NULL) {
    _IOC_LogBug("No LinkObj of AutoLinkID(%llu)", LinkID);
    return IOC_RESULT_BUG;
  }

  pthread_mutex_lock(&pLinkObj->State.Mutex);
  *pLinkState = pLinkObj->State.Main;
  if (pLinkSubState != NULL) {
    *pLinkSubState = pLinkObj->State.Sub;
  }
  pthread_mutex_unlock(&pLinkObj->State.Mutex);

  return IOC_RESULT_SUCCESS;
}

IOC_Result_T _IOC_getCapabilty_inConlesMode(
    /*ARG_INOUT*/ IOC_CapabiltyDescription_pT pCapDesc) {
  IOC_Result_T Result = IOC_RESULT_BUG;

  switch (pCapDesc->CapID) {
    case IOC_CAPID_CONLES_MODE_EVENT: {
      pCapDesc->ConlesModeEvent.MaxEvtConsumer = _CONLES_EVENT_MAX_SUBSCRIBER;
      Result                                   = IOC_RESULT_SUCCESS;
    } break;
    default:
      _IOC_LogError("Not-Support CapID(%d)", pCapDesc->CapID);
      Result = IOC_RESULT_NOT_SUPPORT;
  }

  return Result;
}

void _IOC_forceProcEvt_inConlesMode(void) {
  // foreach ClsEvtLinkObj, getLock, dequeueEvtDescQueueFirst, cbProcEvtClsEvtSuberList,
  //    until EvtDescQueue is empty, then unlock
  ULONG_T TotalClsLinkObjNum = IOC_calcArrayElmtCnt(_mClsEvtLinkObjs);

  for (ULONG_T i = 0; i < TotalClsLinkObjNum; i++) {
    _ClsEvtLinkObj_pT pLinkObj = &_mClsEvtLinkObjs[i];

    while (true) {
      __IOC_wakeupClsEvtLinkObj(pLinkObj);

      usleep(1000);  // 1ms
      IOC_Result_T IsEmptyEvtDescQueue = _IOC_isEmptyEvtDescQueue(&pLinkObj->EvtDescQueue);
      if (IsEmptyEvtDescQueue == IOC_RESULT_YES) {
        break;
      }
    }
  }
}

/**
 * RefDiagram: _IOC_ConlesEvent.md
 *   |-> FlowChart Diagram
 *     |-> _IOC_postEVT_inConlesMode
 *
 * Path:
 *  A) 1) AsyncMode + enqueueSuccess
 *        2) MayBlockMode + waitSpaceEnqueueSuccess
 *        3) NonBlockOrTimeoutMode + failTooManyQueuingEvtDesc
 *        4) UnExceptError: failWithLogBug
 *  B) 1)SyncMode + cbProcEvtSuccess
 *        2) MayBlockMode + waitEmptyCbProcEvtSuccess
 *        3) NonBlockOrTimeoutMode + failTooLongEmptyingEvtDescQueue
 *  C) BugLikeError:
 *     1) invalidAutoLinkID if no LinkObj
 *     2) noEvtSuber if no EvtSuber of LinkObj
 *
 * @note:
 *  a) SUCCESS result with LogDebug, FAIL result with LogWarn or LogError or LogBug
 *  b) Each Result setting point comment with 'Path@[A/B/C]->[1/2/3]' and goto _returnResult
 *  c) Each Result of Path add _IOC_LogNotTested() to indicate need to check this path, comment out after test
 */
IOC_Result_T _IOC_postEVT_inConlesMode(
    /*ARG_IN*/ IOC_LinkID_T LinkID,
    /*ARG_IN*/ const IOC_EvtDesc_pT pEvtDesc,
    /*ARG_IN_OPTIONAL*/ const IOC_Options_pT pOption) {
  IOC_Result_T Result = IOC_RESULT_BUG;
  bool IsAsyncMode    = IOC_Option_isAsyncMode(pOption);
  bool IsNonBlockMode = IOC_Option_isNonBlockMode(pOption);

  _ClsEvtLinkObj_pT pLinkObj = __IOC_getClsEvtLinkObjLocked(LinkID);
  if (pLinkObj == NULL) {
    _IOC_LogError("Invalid AutoLinkID(%llu)", LinkID);
    _IOC_LogNotTested();                     // TODO: check this path, comment out after test
    return IOC_RESULT_INVALID_AUTO_LINK_ID;  // Path@C->1
  }

  IOC_Result_T IsEmptyEvtSuberList = __IOC_isEmptyClsEvtSuberList(&pLinkObj->EvtSuberList);
  if (IsEmptyEvtSuberList == IOC_RESULT_YES) {
    _IOC_LogWarn("No EvtSuber of AutoLinkID(%llu)", LinkID);
    Result = IOC_RESULT_NO_EVENT_CONSUMER;  // Path@C->2
    //_IOC_LogNotTested();
    goto _returnResult;
  }

  //---------------------------------------------------------------------------
  if (IsAsyncMode) {
    Result = _IOC_enqueueEvtDescQueueLast(&pLinkObj->EvtDescQueue, pEvtDesc);
    if (Result == IOC_RESULT_SUCCESS) {
      __IOC_wakeupClsEvtLinkObj(pLinkObj);
      //_IOC_LogDebug("AsyncMode: AutoLinkID(%llu) enqueued EvtDesc(SeqID=%lu,EvtID=(%lu,%lu))", LinkID,
      //              pEvtDesc->MsgDesc.SeqID, IOC_getEvtClassID(pEvtDesc->EvtID), IOC_getEvtNameID(pEvtDesc->EvtID));
      //_IOC_LogNotTested();
      goto _returnResult;  // Path@A->1
    } else if (Result == IOC_RESULT_TOO_MANY_QUEUING_EVTDESC) {
      if (IsNonBlockMode) {
        //_IOC_LogDebug("ASyncNonBlockMode: AutoLinkID(%llu) enqueue EvtDesc(SeqID=%lu,EvtID(%lu,%lu)) failed(%s)",
        //              LinkID, pEvtDesc->MsgDesc.SeqID, IOC_getEvtClassID(pEvtDesc->EvtID),
        //              IOC_getEvtNameID(pEvtDesc->EvtID), IOC_getResultStr(Result));
        Result = IOC_RESULT_TOO_MANY_QUEUING_EVTDESC;  // Path@A->3 of NonBlockMode
        //_IOC_LogNotTested();
        goto _returnResult;
      } else /* MayBlockMode */ {
        ULONG_T RetryUS = 9;  // 9us
        // MayBlockMode: calculate timeout, then retry enqueue every RetryUS until success or timeout
        ULONG_T TimeoutUS = IOC_Option_getTimeoutUS(pOption);
        // sanity check TimeoutUS>0
        _IOC_LogAssert(TimeoutUS > 0);
        bool IsLastRetry = false;

        while (!IsLastRetry) {
          usleep(RetryUS);

          Result = _IOC_enqueueEvtDescQueueLast(&pLinkObj->EvtDescQueue, pEvtDesc);
          if (Result == IOC_RESULT_SUCCESS) {
            __IOC_wakeupClsEvtLinkObj(pLinkObj);
            //_IOC_LogDebug("ASyncMayBlockMode: AutoLinkID(%llu) enqueued EvtDesc(SeqID=%lu,EvtID(%lu,%lu))", LinkID,
            //              pEvtDesc->MsgDesc.SeqID, IOC_getEvtClassID(pEvtDesc->EvtID),
            //              IOC_getEvtNameID(pEvtDesc->EvtID));
            break;  // Path@A->2 MayBlockMode of enqueueSuccess
          } else if (Result == IOC_RESULT_TOO_MANY_QUEUING_EVTDESC) {
            if (TimeoutUS > RetryUS) {
              TimeoutUS -= RetryUS;  // retry
            } else if (TimeoutUS > 0) {
              RetryUS = TimeoutUS;  // last retry
              TimeoutUS = 0;
            } else {
              _IOC_LogWarn("ASyncMayBlockMode: AutoLinkID(%llu) enqueue EvtDesc(%lu,%llu) failed(%s)", LinkID,
                           pEvtDesc->MsgDesc.SeqID, pEvtDesc->EvtID, IOC_getResultStr(Result));
              Result = IOC_RESULT_TOO_MANY_QUEUING_EVTDESC;  // Path@A->3 of Timeout
              IsLastRetry = true;
              break;
            }

            __IOC_wakeupClsEvtLinkObj(pLinkObj);  // try wakeup and try enqueue again
          } else {
            _IOC_LogBug("UnExceptError: AutoLinkID(%llu) enqueue EvtDesc(%lu,%llu) failed(%s)", LinkID, pEvtDesc->MsgDesc.SeqID,
                        pEvtDesc->EvtID, IOC_getResultStr(Result));
            Result = IOC_RESULT_BUG;  // Path@A->4
            break;
          }
        }  // END of while(TimeoutUS>0)

        _IOC_LogAssert((Result == IOC_RESULT_SUCCESS) || (Result == IOC_RESULT_TOO_MANY_QUEUING_EVTDESC));
        //_IOC_LogNotTested();
        goto _returnResult;
      }
    } else {
      _IOC_LogBug("UnExceptError: AutoLinkID(%llu) enqueue EvtDesc(%lu,%llu) failed(%s)", LinkID, pEvtDesc->MsgDesc.SeqID,
                  pEvtDesc->EvtID, IOC_getResultStr(Result));
      Result = IOC_RESULT_BUG;  // Path@A->4
      _IOC_LogNotTested();      // TODO: check this path, comment out after test
      goto _returnResult;
    }
  } else /*SyncMode*/ {
    bool IsEmptyEvtDescQueue = _IOC_isEmptyEvtDescQueue(&pLinkObj->EvtDescQueue);

    if (IsEmptyEvtDescQueue == IOC_RESULT_YES) {
      __IOC_cbProcEvtClsEvtSuberList(&pLinkObj->EvtSuberList, pEvtDesc);
      //_IOC_LogDebug("SyncMode: AutoLinkID(%llu) proc EvtDesc(%lu,%llu)", LinkID, pEvtDesc->MsgDesc.SeqID,
      //pEvtDesc->EvtID); _IOC_LogNotTested();
      Result = IOC_RESULT_SUCCESS;  // Path@B->1
    } else {
      if (IsNonBlockMode) {
        _IOC_LogWarn("SyncNonBlockMode: AutoLinkID(%llu) emptying EvtDescQueue failed(%s)", LinkID, IOC_getResultStr(Result));
        Result = IOC_RESULT_TOO_LONG_EMPTYING_EVTDESC_QUEUE;  // Path@B->3 of NonBlockMode
        _IOC_LogNotTested();                                  // TODO: check this path, comment out after test
        goto _returnResult;
      } else /* MayBlockMode */ {
        ULONG_T RetryUS = 9;  // 9us
        // MayBlockMode: calculate timeout, then retry emptying every RetryUS until success or timeout
        ULONG_T TimeoutUS = IOC_Option_getTimeoutUS(pOption);
        // sanity check TimeoutUS>0
        _IOC_LogAssert(TimeoutUS > 0);
        bool IsLastRetry = false;

        while (!IsLastRetry) {
          usleep(RetryUS);

          IsEmptyEvtDescQueue = _IOC_isEmptyEvtDescQueue(&pLinkObj->EvtDescQueue);
          if (IsEmptyEvtDescQueue == IOC_RESULT_YES) {
            __IOC_cbProcEvtClsEvtSuberList(&pLinkObj->EvtSuberList, pEvtDesc);
            _IOC_LogDebug("SyncMayBlockMode: AutoLinkID(%llu) proc EvtDesc(%lu,%llu)", LinkID, pEvtDesc->MsgDesc.SeqID,
                          pEvtDesc->EvtID);
            Result = IOC_RESULT_SUCCESS;  // Path@B->2 MayBlockMode of cbProcEvtSuccess
            break;
          } else {
            if (TimeoutUS > RetryUS) {
              TimeoutUS -= RetryUS;  // retry
            } else if (TimeoutUS > 0) {
              RetryUS   = TimeoutUS;  // last retry
              TimeoutUS = 0;
            } else {
              _IOC_LogWarn("SyncMayBlockMode: AutoLinkID(%llu) emptying EvtDescQueue failed(%s)", LinkID,
                           IOC_getResultStr(Result));
              Result      = IOC_RESULT_TOO_LONG_EMPTYING_EVTDESC_QUEUE;  // Path@B->3 of Timeout
              IsLastRetry = true;
              break;
            }
          }
        }  // END of while(TimeoutUS>0)

        _IOC_LogAssert((Result == IOC_RESULT_SUCCESS) || (Result == IOC_RESULT_TOO_LONG_EMPTYING_EVTDESC_QUEUE));
        _IOC_LogNotTested();  // TODO: check this path, comment out after test
        goto _returnResult;
      }
    }
  }

//-----------------------------------------------------------------------------
_returnResult:
  _IOC_LogAssert(pLinkObj != NULL);
  __IOC_putClsEvtLinkObj(pLinkObj);

  return Result;
}
//======>>>>>>END OF IMPLEMENT FOR ConlesEvent>>>>>>===================================================================