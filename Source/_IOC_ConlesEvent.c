/**
 * @file _IOC_ConlesEvent.c
 * @brief This is ConlesEvent internal Design&Define&Implement file.
 *      And this file only implement interfaces defined in _IOC_ConlesEvent.h.
 *  ConlesEvent is short of ConnectionLessEvent, a.k.a ClsEvt in current file.
 *
 * @attention
 *  Detail Design is in current file's comments and
 *    in _IOC_ConlesEvent.md which focus on graph of sequence and state machine.
 *  Type Define and Code Implement is in current file's following design comments.
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

#include <stdatomic.h>
#include <unistd.h>

#include "_IOC_EvtDescQueue.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//======>>>>>>BEGIN OF DEFINE FOR ConlesEvent>>>>>>====================================================================
#define _CONLES_EVENT_MAX_SUBSCRIBER 16

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

#if 0
static void __IOC_ClsEvt_initSuberList(_ClsEvtSuberList_pT pEvtSuberList);
static void __IOC_ClsEvt_deinitSuberList(_ClsEvtSuberList_pT pEvtSuberList);

// Return: IOC_RESULT_YES or IOC_RESULT_NO
static IOC_BoolResult_T __IOC_ClsEvt_isEmptySuberList(_ClsEvtSuberList_pT pEvtSuberList);
#endif

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

  // atomic counter:
  //   1) how many EvtDesc enqueued EvtDescQueue
  //   2) how many EvtDesc is callbacked by EvtProcThread
  //   WHEN QueuedEvtNum == CallbacedEvtNum, LinkObj has no EvtDesc queueing or callbacking.
  atomic_ulong QueuedEvtNum, CallbacedEvtNum;

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
#if 0
// get ClsEvtLinkObj by AutoLinkID, not lock it by Mutex when return, and dont put it after use.
static _ClsEvtLinkObj_pT __IOC_ClsEvt_getLinkObjNotLocked(IOC_LinkID_T AutoLinkID);

// get ClsEvtLinkObj by AutoLinkID, lock it by Mutex when return, and put it after use.
static _ClsEvtLinkObj_pT __IOC_ClsEvt_getLinkObjLocked(IOC_LinkID_T AutoLinkID);
static void __IOC_ClsEvt_putLinkObj(_ClsEvtLinkObj_pT pLinkObj);

static void __IOC_ClsEvt_wakeupLinkObjThread(_ClsEvtLinkObj_pT pLinkObj);
static void __IOC_ClsEvt_waitLinkObjNewEvtDesc(_ClsEvtLinkObj_pT pLinkObj);

static void *__IOC_ClsEvt_callbackProcEvtThread(void *arg);
#endif

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

#if 0
static void __IOC_ClsEvt_transferLinkObjStateByBehavior(_ClsEvtLinkObj_pT pLinkObj,
                                                        _ClsEvtLinkObjBehavior_T Behavior);

// Following EvtSuberList operation need a EvtLinkObj as parent in context to transfer state under EvtSuberList's Mutex.
//  Return: IOC_RESULT_SUCCESS or IOC_RESULT_TOO_MANY_EVENT_CONSUMER or IOC_RESULT_CONFLICT_EVENT_CONSUMER
static IOC_Result_T __IOC_ClsEvt_insertSuberIntoLinkObj(_ClsEvtLinkObj_pT pEvtLinkObj,
                                                        IOC_SubEvtArgs_pT pSubEvtArgs);
// Return: IOC_RESULT_SUCCESS or IOC_RESULT_NO_EVENT_CONSUMER
static IOC_Result_T __IOC_ClsEvt_removeSuberFromLinkObj(_ClsEvtLinkObj_pT pEvtLinkObj,
                                                        IOC_UnsubEvtArgs_pT pUnsubEvtArgs);

static void __IOC_ClsEvt_callbackProcEvtOverSuberList(_ClsEvtLinkObj_pT pEvtLinkObj,
                                                      IOC_EvtDesc_pT pEvtDesc);
#endif

// RefDoc: README_ArchDesign.md
//     |-> State
//       |-> EVT::Conles
static void __IOC_ClsEvt_transferLinkObjStateByBehavior(_ClsEvtLinkObj_pT pLinkObj,
                                                        _ClsEvtLinkObjBehavior_T Behavior) {
  pthread_mutex_lock(&pLinkObj->State.Mutex);
  IOC_LinkState_T MainState = pLinkObj->State.Main;

  switch (Behavior) {
    case Behavior_enterCbProcEvt: {
      if (MainState == IOC_LinkStateReady) {
        pLinkObj->State.Main = IOC_LinkStateBusyCbProcEvt;
      } else {
        _IOC_LogBug("Invalid State(Main=%d) to enterCbProcEvt, MUST in State(Main=%d)", MainState,
                    IOC_LinkStateReady);
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
        _IOC_LogBug("Invalid State(Main=%d) to enterSubEvt, MUST in State(Main=%d)", MainState,
                    IOC_LinkStateReady);
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
        _IOC_LogBug("Invalid State(Main=%d) to enterUnsubEvt, MUST in State(Main=%d)", MainState,
                    IOC_LinkStateReady);
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
//======>>>>>>END OF DEFINE FOR ConlesEvent>>>>>>======================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//======>>>>>>BEGIN OF IMPLEMENT FOR
//ConlesEvent>>>>>>=================================================================

//---------------------------------------------------------------------------------------------------------------------
//===> BEGIN IMPLEMENT FOR ClsEvtSuberList
static void __IOC_ClsEvt_initSuberList(_ClsEvtSuberList_pT pEvtSuberList) {
  pthread_mutex_init(&pEvtSuberList->Mutex, NULL);
  pEvtSuberList->SuberNum = 0;
  memset(pEvtSuberList->Subers, 0, sizeof(pEvtSuberList->Subers));
}
static void __IOC_ClsEvt_deinitSuberList(_ClsEvtSuberList_pT pEvtSuberList) {
  // deinit all checkable members only

  // check conditions which matching destoriability
  _IOC_LogAssert(pEvtSuberList->SuberNum == 0);

  int PosixResult = pthread_mutex_destroy(&pEvtSuberList->Mutex);
  if (PosixResult != 0) {
    _IOC_LogAssert(PosixResult == 0);
  }
}

// Return: IOC_RESULT_SUCCESS or IOC_RESULT_TOO_MANY_EVENT_CONSUMER or IOC_RESULT_CONFLICT_EVENT_CONSUMER
static IOC_Result_T __IOC_ClsEvt_insertSuberIntoLinkObj(_ClsEvtLinkObj_pT pEvtLinkObj,
                                                        IOC_SubEvtArgs_pT pSubEvtArgs) {
  IOC_Result_T Result = IOC_RESULT_BUG;
  _ClsEvtSuberList_pT pSuberList = &pEvtLinkObj->EvtSuberList;

  pthread_mutex_lock(&pSuberList->Mutex);
  __IOC_ClsEvt_transferLinkObjStateByBehavior(pEvtLinkObj, Behavior_enterSubEvt);

  ULONG_T SuberNum = pSuberList->SuberNum;
  if (SuberNum >= _CONLES_EVENT_MAX_SUBSCRIBER) {
    Result = IOC_RESULT_TOO_MANY_EVENT_CONSUMER;
    goto _returnResult;
  }

  // check conflict
  for (ULONG_T i = 0; i < _CONLES_EVENT_MAX_SUBSCRIBER; i++) {
    _ClsEvtSuber_T *pSuber = &pSuberList->Subers[i];

    if (pSuber->State == Subed) {
      // RefComments: IOC_SubEvtArgs_T how to identify a EvtConsumer
      if (pSuber->Args.CbProcEvt_F == pSubEvtArgs->CbProcEvt_F &&
          pSuber->Args.pCbPrivData == pSubEvtArgs->pCbPrivData) {
        Result = IOC_RESULT_CONFLICT_EVENT_CONSUMER;
        goto _returnResult;
      }
    }
  }

  // forloop to get the first empty slot
  for (ULONG_T i = 0; i < _CONLES_EVENT_MAX_SUBSCRIBER; i++) {
    _ClsEvtSuber_T *pSuber = &pSuberList->Subers[i];

    if (pSuber->State == UnSubed) {
      pSuber->State = Subed;

      // save direct args, and alloc new memory to save indirect EvtIDs
      pSuber->Args.CbProcEvt_F = pSubEvtArgs->CbProcEvt_F;
      pSuber->Args.pCbPrivData = pSubEvtArgs->pCbPrivData;
      pSuber->Args.EvtNum      = pSubEvtArgs->EvtNum;
      pSuber->Args.pEvtIDs     = (IOC_EvtID_T *)malloc(pSubEvtArgs->EvtNum * sizeof(IOC_EvtID_T));
      memcpy(pSuber->Args.pEvtIDs, pSubEvtArgs->pEvtIDs, pSubEvtArgs->EvtNum * sizeof(IOC_EvtID_T));

      // increase SuberNum atomically
      atomic_fetch_add(&pSuberList->SuberNum, 1);
      break;
    }
  }

  Result = IOC_RESULT_SUCCESS;

_returnResult:
  __IOC_ClsEvt_transferLinkObjStateByBehavior(pEvtLinkObj, Behavior_leaveSubEvt);
  pthread_mutex_unlock(&pSuberList->Mutex);
  return Result;
}
// Return: IOC_RESULT_SUCCESS or IOC_RESULT_NO_EVENT_CONSUMER
static IOC_Result_T __IOC_ClsEvt_removeSuberFromLinkObj(_ClsEvtLinkObj_pT pEvtLinkObj,
                                                        IOC_UnsubEvtArgs_pT pUnsubEvtArgs) {
  IOC_Result_T Result = IOC_RESULT_BUG;
  _ClsEvtSuberList_pT pSuberList = &pEvtLinkObj->EvtSuberList;

  pthread_mutex_lock(&pSuberList->Mutex);
  __IOC_ClsEvt_transferLinkObjStateByBehavior(pEvtLinkObj, Behavior_enterUnsubEvt);

  ULONG_T SuberNum = pSuberList->SuberNum;
  if (SuberNum == 0) {
    Result = IOC_RESULT_NO_EVENT_CONSUMER;
    goto _returnResult;
  }

  // forloop to find the first match slot
  for (ULONG_T i = 0; i < _CONLES_EVENT_MAX_SUBSCRIBER; i++) {
    _ClsEvtSuber_T *pSuber = &pSuberList->Subers[i];

    if (pSuber->State == Subed) {
      // RefComments: IOC_SubEvtArgs_T how to identify a EvtConsumer
      if (pSuber->Args.CbProcEvt_F == pUnsubEvtArgs->CbProcEvt_F &&
          pSuber->Args.pCbPrivData == pUnsubEvtArgs->pCbPrivData) {
        pSuber->State = UnSubed;

        // free indirect EvtIDs
        free(pSuber->Args.pEvtIDs);

        // decrease SuberNum atomically
        atomic_fetch_sub(&pSuberList->SuberNum, 1);
        break;
      }
    }
  }

  Result = IOC_RESULT_SUCCESS;

_returnResult:
  __IOC_ClsEvt_transferLinkObjStateByBehavior(pEvtLinkObj, Behavior_leaveUnsubEvt);
  pthread_mutex_unlock(&pSuberList->Mutex);
  return Result;
}

static IOC_BoolResult_T __IOC_ClsEvt_isEmptySuberList(_ClsEvtSuberList_pT pEvtSuberList) {
  // read SuberNum atomically
  ULONG_T SuberNum    = atomic_load(&pEvtSuberList->SuberNum);
  IOC_BoolResult_T Result = (SuberNum == 0) ? IOC_RESULT_YES : IOC_RESULT_NO;
  return Result;
}

static void __IOC_ClsEvt_callbackProcEvtOverSuberList(_ClsEvtLinkObj_pT pEvtLinkObj,
                                                      IOC_EvtDesc_pT pEvtDesc) {
  _ClsEvtSuberList_pT pSuberList = &pEvtLinkObj->EvtSuberList;
  pthread_mutex_lock(&pSuberList->Mutex);

  for (ULONG_T i = 0; i < _CONLES_EVENT_MAX_SUBSCRIBER; i++) {
    _ClsEvtSuber_T *pSuber = &pSuberList->Subers[i];

    if (pSuber->State == Subed) {
      for (ULONG_T j = 0; j < pSuber->Args.EvtNum; j++) {
        if (pEvtDesc->EvtID == pSuber->Args.pEvtIDs[j]) {
          __IOC_ClsEvt_transferLinkObjStateByBehavior(pEvtLinkObj, Behavior_enterCbProcEvt);
          // FIXME: IF ANY CbProcEvt_F STUCK, IT WILL BLOCK THE WHOLE THREAD, SO WE NEED TO HANDLE THIS CASE.
          // TODO: INSTALL A TIMER TO CATCH TIMEOUT, AND OUTPUT LOG TO INDICATE WHICH CbProcEvt_F STUCK.
          pSuber->Args.CbProcEvt_F(pEvtDesc, pSuber->Args.pCbPrivData);
          __IOC_ClsEvt_transferLinkObjStateByBehavior(pEvtLinkObj, Behavior_leaveCbProcEvt);
        }
      }
    }
  }

  pthread_mutex_unlock(&pSuberList->Mutex);
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
__attribute__((no_sanitize_thread)) static _ClsEvtLinkObj_pT __IOC_ClsEvt_getLinkObjNotLocked(
    IOC_LinkID_T AutoLinkID) {
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

static _ClsEvtLinkObj_pT __IOC_ClsEvt_getLinkObjLocked(IOC_LinkID_T AutoLinkID) {
  IOC_BoolResult_T IsAutoLink = _IOC_isAutoLink_inConlesMode(AutoLinkID);
  if (IsAutoLink == IOC_RESULT_NO) {
    return NULL;
  }

  _ClsEvtLinkObj_pT pLinkObj = __IOC_ClsEvt_getLinkObjNotLocked(AutoLinkID);
  if (pLinkObj == NULL) {
    return NULL;
  }

  pthread_mutex_lock(&pLinkObj->Mutex);
  return pLinkObj;
}

static void __IOC_ClsEvt_putLinkObj(_ClsEvtLinkObj_pT pLinkObj) {
  pthread_mutex_unlock(&pLinkObj->Mutex);
}

static void __IOC_ClsEvt_wakeupLinkObjThread(_ClsEvtLinkObj_pT pLinkObj) {
  pthread_mutex_lock(&pLinkObj->CondMutex);
  pthread_cond_signal(&pLinkObj->Cond);
  pthread_mutex_unlock(&pLinkObj->CondMutex);
}
static void __IOC_ClsEvt_waitLinkObjNewEvtDesc(_ClsEvtLinkObj_pT pLinkObj) {
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

static void *__IOC_ClsEvt_callbackProcEvtThread(void *arg) {
  _ClsEvtLinkObj_pT pLinkObj = (_ClsEvtLinkObj_pT)arg;

  /**
   * Steps:
   *  1) __IOC_ClsEvt_waitLinkObjNewEvtDesc
   *  2) __IOC_EvtDescQueue_dequeueElementFirst
   *    |-> if IOC_RESULT_EVTDESC_QUEUE_EMPTY, goto 1)
   *  3) __IOC_ClsEvt_callbackProcEvtOverSuberList
   */
  do {
    __IOC_ClsEvt_waitLinkObjNewEvtDesc(pLinkObj);

    //-----------------------------------------------------------------------------------------------------------------
    do {
      IOC_EvtDesc_T EvtDesc = {};
      IOC_Result_T Result =
          _IOC_EvtDescQueue_dequeueElementFirst(&pLinkObj->EvtDescQueue, &EvtDesc);
      if (Result == IOC_RESULT_EVTDESC_QUEUE_EMPTY) {
        break;
      }

      __IOC_ClsEvt_callbackProcEvtOverSuberList(pLinkObj, &EvtDesc);

      atomic_fetch_add(&pLinkObj->CallbacedEvtNum, 1);
    } while (0x20240714);
    //-----------------------------------------------------------------------------------------------------------------

  } while (0x20240709);

  pthread_exit(NULL);
}

// HAS = EvtDescQueue is not empty || one EvtDesc is callbacking by EvtProcThread
static IOC_BoolResult_T __IOC_ClsEvt_hasEvtDescInLinkObj(_ClsEvtLinkObj_pT pLinkObj) {
  // read QueuedEvtNum and CallbacedEvtNum atomically
  ULONG_T QueuedEvtNum    = atomic_load(&pLinkObj->QueuedEvtNum);
  ULONG_T CallbacedEvtNum = atomic_load(&pLinkObj->CallbacedEvtNum);
  IOC_BoolResult_T Result = (QueuedEvtNum != CallbacedEvtNum) ? IOC_RESULT_YES : IOC_RESULT_NO;
  return Result;
}

// On libc's runtime init, this function will be called to create all ClsEvtLinkObj's EvtProcThread.
__attribute__((constructor)) static void __IOC_LibCRT_initClsEvtLinkObj(void) {
  ULONG_T TotalClsLinkObjNum = IOC_calcArrayElmtCnt(_mClsEvtLinkObjs);

  for (ULONG_T i = 0; i < TotalClsLinkObjNum; i++) {
    _ClsEvtLinkObj_pT pLinkObj = &_mClsEvtLinkObjs[i];

    _IOC_EvtDescQueue_initOne(&pLinkObj->EvtDescQueue);
    __IOC_ClsEvt_initSuberList(&pLinkObj->EvtSuberList);

    pthread_mutex_init(&pLinkObj->Mutex, NULL);
    pthread_cond_init(&pLinkObj->Cond, NULL);
    pthread_mutex_init(&pLinkObj->CondMutex, NULL);

    pthread_create(&pLinkObj->ThreadID, NULL, __IOC_ClsEvt_callbackProcEvtThread, pLinkObj);
  }
}

//===> END IMPLEMENT FOR ClsEvtLinkObj
//---------------------------------------------------------------------------------------------------------------------

IOC_BoolResult_T _IOC_isAutoLink_inConlesMode(
    /*ARG_IN*/ IOC_LinkID_T LinkID) {
  return (LinkID == IOC_CONLES_MODE_AUTO_LINK_ID) ? IOC_RESULT_YES : IOC_RESULT_NO;
}

IOC_Result_T _IOC_subEVT_inConlesMode(
    /*ARG_IN*/ const IOC_SubEvtArgs_pT pSubEvtArgs) {
  _ClsEvtLinkObj_pT pLinkObj = __IOC_ClsEvt_getLinkObjLocked(IOC_CONLES_MODE_AUTO_LINK_ID);
  if (pLinkObj == NULL) {
    return IOC_RESULT_BUG;
  }

  IOC_Result_T Result = __IOC_ClsEvt_insertSuberIntoLinkObj(pLinkObj, pSubEvtArgs);
  if (IOC_RESULT_SUCCESS == Result) {
    //_IOC_LogDebug("AutoLinkID(%lu) new EvtSuber(CbProcEvt_F=%p,PrivData=%p)", IOC_CONLES_MODE_AUTO_LINK_ID,
    //              pSubEvtArgs->CbProcEvt_F, pSubEvtArgs->pCbPrivData);
  } else {
    _IOC_LogWarn("AutoLinkID(%u) new EvtSuber(CbProcEvt_F=%p,PrivData=%p) failed(%s)",
                 IOC_CONLES_MODE_AUTO_LINK_ID, pSubEvtArgs->CbProcEvt_F, pSubEvtArgs->pCbPrivData,
                 IOC_getResultStr(Result));
  }

  __IOC_ClsEvt_putLinkObj(pLinkObj);
  return Result;
}

IOC_Result_T _IOC_unsubEVT_inConlesMode(
    /*ARG_IN*/ const IOC_UnsubEvtArgs_pT pUnsubEvtArgs) {
  _ClsEvtLinkObj_pT pLinkObj = __IOC_ClsEvt_getLinkObjLocked(IOC_CONLES_MODE_AUTO_LINK_ID);
  if (pLinkObj == NULL) {
    return IOC_RESULT_BUG;
  }

  IOC_Result_T Result = __IOC_ClsEvt_removeSuberFromLinkObj(pLinkObj, pUnsubEvtArgs);
  if (IOC_RESULT_SUCCESS == Result) {
    //_IOC_LogDebug("AutoLinkID(%lu) remove EvtSuber(CbProcEvt_F=%p,PrivData=%p)", IOC_CONLES_MODE_AUTO_LINK_ID,
    //              pUnsubEvtArgs->CbProcEvt_F, pUnsubEvtArgs->pCbPrivData);
  } else {
    _IOC_LogWarn("AutoLinkID(%u) remove EvtSuber(CbProcEvt_F=%p,PrivData=%p) failed(%s)",
                 IOC_CONLES_MODE_AUTO_LINK_ID, pUnsubEvtArgs->CbProcEvt_F,
                 pUnsubEvtArgs->pCbPrivData, IOC_getResultStr(Result));
  }

  __IOC_ClsEvt_putLinkObj(pLinkObj);
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

  _ClsEvtLinkObj_pT pLinkObj = __IOC_ClsEvt_getLinkObjNotLocked(LinkID);
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
      pCapDesc->ConlesModeEvent.MaxEvtConsumer    = _CONLES_EVENT_MAX_SUBSCRIBER;
      pCapDesc->ConlesModeEvent.DepthEvtDescQueue = _CONLES_EVENT_MAX_QUEUING_EVTDESC;

      Result = IOC_RESULT_SUCCESS;
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

  struct timespec TS_TickBegin, TS_TickLastWarn, TS_TickNow;
  TS_TickLastWarn = TS_TickBegin = IOC_getCurrentTimeSpec();

  for (ULONG_T i = 0; i < TotalClsLinkObjNum; i++) {
    _ClsEvtLinkObj_pT pLinkObj = &_mClsEvtLinkObjs[i];

    while (true) {
      __IOC_ClsEvt_wakeupLinkObjThread(pLinkObj);

      usleep(1000);  // 1ms

      IOC_BoolResult_T HasEvtDesc = __IOC_ClsEvt_hasEvtDescInLinkObj(pLinkObj);
      if (HasEvtDesc == IOC_RESULT_NO) {
        break;
      }

      TS_TickNow        = IOC_getCurrentTimeSpec();
      ULONG_T ElapsedMS = IOC_deltaTimeSpecInMS(&TS_TickLastWarn, &TS_TickNow);
      if (ElapsedMS >= 1000) {
        _IOC_LogWarn("AutoLinkID(%llu) still HAS EvtDesc, keep waiting +1s", pLinkObj->LinkID);
        TS_TickLastWarn = TS_TickNow;
      }

      // check if forceProcEvt is too long to bug&abort
    }
  }
}

static IOC_Result_T __IOC_postEVT_inConlesModeAsyncTimed(
    /*ARG_IN*/ _ClsEvtLinkObj_pT pLinkObj,
    /*ARG_IN*/ const IOC_EvtDesc_pT pEvtDesc,
    /*ARG_IN*/ ULONG_T TimeoutUS);
static IOC_Result_T __IOC_postEVT_inConlesModeAsyncBlocked(
    /*ARG_IN*/ _ClsEvtLinkObj_pT pLinkObj,
    /*ARG_IN*/ const IOC_EvtDesc_pT pEvtDesc) {
  return __IOC_postEVT_inConlesModeAsyncTimed(pLinkObj, pEvtDesc, ULONG_MAX);
}

static IOC_Result_T __IOC_postEVT_inConlesModeSyncTimed(
    /*ARG_IN*/ _ClsEvtLinkObj_pT pLinkObj,
    /*ARG_IN*/ const IOC_EvtDesc_pT pEvtDesc,
    /*ARG_IN*/ ULONG_T TimeoutUS);
static IOC_Result_T __IOC_postEVT_inConlesModeSyncBlocked(
    /*ARG_IN*/ _ClsEvtLinkObj_pT pLinkObj,
    /*ARG_IN*/ const IOC_EvtDesc_pT pEvtDesc) {
  return __IOC_postEVT_inConlesModeSyncTimed(pLinkObj, pEvtDesc, ULONG_MAX);
}

/**
 * @brief Implementation of the _IOC_postEVT_inConlesMode.
 *
 * @param
 * - LinkID: use predefined AutoID.
 * - pEvtDesc: A pointer to the readonly event descriptor.
 * - pOption: An optional pointer to the options.
 *    such as Async or Sync, MayBlock or NonBlock or Timeout.
 *
 * @return see following function flowchart diagram and paths.
 *-------------------------------------------------------------------------------------------------
 * The function follows the following flowchart diagram:
 * - RefDiagram: _IOC_ConlesEvent.md
 *   - FlowChart Diagram
 *     - _IOC_postEVT_inConlesMode
 *
 * The function follows the following paths:
 * - A) AsyncMode
 *   - 1) enqueueSuccess_ifHasSpaceInEvtDescQueue
 *          |-> return IOC_RESULT_SUCCESS
 *   - 2) NonBlockOrTimeoutMode_returnImmediatelyOrWaitTimeoutOrEnqueueSuccess
 *          -> return IOC_RESULT_TOO_MANY_QUEUING_EVTDESC OR IOC_RESULT_SUCCESS
 *   - 3) MayBlockMode_waitUntilHasSpaceAndEnqueueSuccess
 *          |-> return IOC_RESULT_SUCCESS OR BLOCK_FOREVER
 *   - 4) UnExceptError: failWithLogBugMsg
 * - B) SyncMode
 *   - 1) cbProcEvt_ifIsEmptyEvtDescQueue
 *        |-> return IOC_RESULT_SUCCESS
 *   - 2) NonBlockOrTimeoutMode_returnImmediatelyOrWaitTimeoutOrCbProcEvt
 *        |-> return IOC_RESULT_TOO_LONG_EMPTYING_EVTDESC_QUEUE OR IOC_RESULT_SUCCESS
 *   - 3) MayBlockMode_waitEvtDescQueueBecomeEmptyThenCbProcEvt
 *        |-> return IOC_RESULT_SUCCESS OR BLOCK_FOREVER
 *   - 4) UnExceptError: failWithLogBug
 * - C) BugLikeError
 *   - 1) not a valid AutoLinkID or not exist LinkObj
 *        |-> return IOC_RESULT_INVALID_AUTO_LINK_ID
 *   - 2) noEvtSuber if no EvtSuber of LinkObj
 *        |-> return IOC_RESULT_NO_EVENT_CONSUMER
 *
 * @note
 * - SUCCESS result with LogDebug, FAIL result with LogWarn or LogError or LogBug.
 * - Each Result return value set and comment with 'Path@[A/B/C]->[1/2/3/4]',
 *    and add _IOC_LogNotTested() next line for later comment out by unit testing of this path,
 *      and then goto _returnResult lable to clean up and return Result.
 */
IOC_Result_T _IOC_postEVT_inConlesMode(
    /*ARG_IN*/ IOC_LinkID_T LinkID,
    /*ARG_IN*/ const IOC_EvtDesc_pT pEvtDesc,
    /*ARG_IN_OPTIONAL*/ const IOC_Options_pT pOption) {
  IOC_Result_T Result          = IOC_RESULT_BUG;
  IOC_BoolResult_T IsAsyncMode = IOC_Option_isAsyncMode(pOption);

  _ClsEvtLinkObj_pT pLinkObj = __IOC_ClsEvt_getLinkObjLocked(LinkID);
  if (pLinkObj == NULL) {
    _IOC_LogError("[ConlesEvent]: No LinkObj of LinkID(%llu)", LinkID);
    //_IOC_LogNotTested();
    return IOC_RESULT_INVALID_AUTO_LINK_ID;  // Path@C->[1]
  }

  IOC_BoolResult_T IsEmptySuberList = __IOC_ClsEvt_isEmptySuberList(&pLinkObj->EvtSuberList);
  if (IsEmptySuberList == IOC_RESULT_YES) {
    _IOC_LogWarn("[ConlesEvent]: No EvtSuber of AutoLinkID(%llu)", LinkID);
    // _IOC_LogNotTested();
    Result = IOC_RESULT_NO_EVENT_CONSUMER;  // Path@C->[2]
    goto _returnResult;
  }

  //-------------------------------------------------------------------------------------------------------------------
  if (IsAsyncMode) {
    // 1) enqueueSuccess_ifHasSpaceInEvtDescQueue
    Result = _IOC_EvtDescQueue_enqueueElementLast(&pLinkObj->EvtDescQueue, pEvtDesc);
    if (Result == IOC_RESULT_SUCCESS) {
      atomic_fetch_add(&pLinkObj->QueuedEvtNum, 1);
      __IOC_ClsEvt_wakeupLinkObjThread(pLinkObj);

      // _IOC_LogDebug("[ConlesEvent::ASync]: AutoLinkID(%llu) postEvtDesc(%s) success", LinkID,
      //               IOC_EvtDesc_printDetail(pEvtDesc, NULL, 0));
      Result = IOC_RESULT_SUCCESS;  // Path@A->[1]
      //_IOC_LogNotTested();
      goto _returnResult;
    }

    // 2.1) NonBlock_returnImmediately
    if (IOC_Option_isNonBlockMode(pOption)) {
      _IOC_LogWarn("[ConlesEvent::ASync::NonBlock]: AutoLinkID(%llu) postEvtDesc(%s) failed",
                   LinkID, IOC_EvtDesc_printDetail(pEvtDesc, NULL, 0));
      Result = IOC_RESULT_TOO_MANY_QUEUING_EVTDESC;  // Path@A->[2]
      //_IOC_LogNotTested();
      goto _returnResult;
    }

    // 2.2) TimeoutMode_waitTimeoutOrEnqueueSuccess
    if (IOC_Option_isTimeoutMode(pOption)) {
      ULONG_T TimeoutUS = IOC_Option_getTimeoutUS(pOption);

      Result = __IOC_postEVT_inConlesModeAsyncTimed(pLinkObj, pEvtDesc, TimeoutUS);  // Path@A->[2]
      _IOC_LogAssert(Result == IOC_RESULT_TOO_MANY_QUEUING_EVTDESC || Result == IOC_RESULT_SUCCESS);

      if (Result == IOC_RESULT_TOO_MANY_QUEUING_EVTDESC) {
        _IOC_LogWarn("[ConlesEvent::ASync::Timeout]: AutoLinkID(%llu) postEvtDesc(%s) failed",
                     LinkID, IOC_EvtDesc_printDetail(pEvtDesc, NULL, 0));
      }

      if (Result == IOC_RESULT_SUCCESS) {
        atomic_fetch_add(&pLinkObj->QueuedEvtNum, 1);
        _IOC_LogDebug("[ConlesEvent::ASync::Timeout]: AutoLinkID(%llu) postEvtDesc(%s) success",
                      LinkID, IOC_EvtDesc_printDetail(pEvtDesc, NULL, 0));
      }

      _IOC_LogNotTested();  // comment out by unit testing
      goto _returnResult;
    }

    // 3) MayBlockMode_waitUntilHasSpaceAndEnqueueSuccess
    if (IOC_Option_isMayBlockMode(pOption)) {
      Result = __IOC_postEVT_inConlesModeAsyncBlocked(pLinkObj, pEvtDesc);  // Path@A->[3]
      _IOC_LogAssert(Result == IOC_RESULT_SUCCESS);

      atomic_fetch_add(&pLinkObj->QueuedEvtNum, 1);

      // _IOC_LogDebug("[ConlesEvent::ASync::MayBlock]: AutoLinkID(%llu) postEvtDesc(%s) success",
      //               LinkID, IOC_EvtDesc_printDetail(pEvtDesc, NULL, 0));
      //_IOC_LogNotTested();
      goto _returnResult;
    } else {
      _IOC_LogBug("[ConlesEvent]: BUG");
      Result = IOC_RESULT_BUG;  // Path@A->[4]
      _IOC_LogNotTested();      // comment out by unit testing
      goto _returnResult;
    }

  } else /*SyncMode*/
  {
    // 1) cbProcEvt_ifIsEmptyEvtDescQueue
    if (__IOC_ClsEvt_hasEvtDescInLinkObj(pLinkObj) == IOC_RESULT_NO) {
      __IOC_ClsEvt_callbackProcEvtOverSuberList(pLinkObj, pEvtDesc);

      // _IOC_LogDebug("[ConlesEvent::Sync]: AutoLinkID(%llu) postEvtDesc(%s) success", LinkID,
      //               IOC_EvtDesc_printDetail(pEvtDesc, NULL, 0));
      Result = IOC_RESULT_SUCCESS;  // Path@B->[1]
      //_IOC_LogNotTested();
      goto _returnResult;
    }

    // 2.1) NonBlock_returnImmediately
    if (IOC_Option_isNonBlockMode(pOption)) {
      _IOC_LogWarn("[ConlesEvent::Sync::NonBlock]: AutoLinkID(%llu) postEvtDesc(%s) failed", LinkID,
                   IOC_EvtDesc_printDetail(pEvtDesc, NULL, 0));
      Result = IOC_RESULT_TOO_LONG_EMPTYING_EVTDESC_QUEUE;  // Path@B->[2]
      //_IOC_LogNotTested();
      goto _returnResult;
    }

    // 2.2) TimeoutMode_waitTimeoutOrCbProcEvt
    if (IOC_Option_isTimeoutMode(pOption)) {
      ULONG_T TimeoutUS = IOC_Option_getTimeoutUS(pOption);

      Result = __IOC_postEVT_inConlesModeSyncTimed(pLinkObj, pEvtDesc, TimeoutUS);  // Path@B->[2]
      _IOC_LogAssert(Result == IOC_RESULT_TOO_LONG_EMPTYING_EVTDESC_QUEUE ||
                     Result == IOC_RESULT_SUCCESS);

      if (Result == IOC_RESULT_TOO_LONG_EMPTYING_EVTDESC_QUEUE) {
        _IOC_LogWarn("[ConlesEvent::Sync::Timeout]: AutoLinkID(%llu) postEvtDesc(%s) failed",
                     LinkID, IOC_EvtDesc_printDetail(pEvtDesc, NULL, 0));
      }

      if (Result == IOC_RESULT_SUCCESS) {
        _IOC_LogDebug("[ConlesEvent::Sync::Timeout]: AutoLinkID(%llu) postEvtDesc(%s) success",
                      LinkID, IOC_EvtDesc_printDetail(pEvtDesc, NULL, 0));
      }

      _IOC_LogNotTested();  // comment out by
      goto _returnResult;
    }

    // 3) MayBlockMode_waitEvtDescQueueBecomeEmptyThenCbProcEvt
    if (IOC_Option_isMayBlockMode(pOption)) {
      Result = __IOC_postEVT_inConlesModeSyncBlocked(pLinkObj, pEvtDesc);  // Path@B->[3]
      _IOC_LogAssert(Result == IOC_RESULT_SUCCESS);

      _IOC_LogDebug("[ConlesEvent::Sync::MayBlock]: AutoLinkID(%llu) postEvtDesc(%s) success",
                    LinkID, IOC_EvtDesc_printDetail(pEvtDesc, NULL, 0));

      _IOC_LogNotTested();  // comment out by unit testing
      goto _returnResult;
    } else {
      _IOC_LogBug("[ConlesEvent]: BUG");
      Result = IOC_RESULT_BUG;  // Path@B->[4]
      _IOC_LogNotTested();      // comment out by unit testing
      goto _returnResult;
    }
  }

//-------------------------------------------------------------------------------------------------------------------
_returnResult:
  __IOC_ClsEvt_putLinkObj(pLinkObj);
  return Result;
}

static IOC_Result_T __IOC_postEVT_inConlesModeAsyncTimed(
    /*ARG_IN*/ _ClsEvtLinkObj_pT pLinkObj,
    /*ARG_IN*/ const IOC_EvtDesc_pT pEvtDesc,
    /*ARG_IN*/ ULONG_T TimeoutUS) {
  IOC_Result_T Result = IOC_RESULT_BUG;

  struct timespec TS_Begin;
  clock_gettime(CLOCK_REALTIME, &TS_Begin);

  do {
    Result = _IOC_EvtDescQueue_enqueueElementLast(&pLinkObj->EvtDescQueue, pEvtDesc);
    if (Result == IOC_RESULT_SUCCESS) {
      __IOC_ClsEvt_wakeupLinkObjThread(pLinkObj);
      //_IOC_LogNotTested();
      break;
    }

    struct timespec TS_End;
    clock_gettime(CLOCK_REALTIME, &TS_End);

    ULONG_T ElapsedUS = IOC_deltaTimeSpecInUS(&TS_Begin, &TS_End);
    if (ElapsedUS >= TimeoutUS) {
      Result = IOC_RESULT_TOO_MANY_QUEUING_EVTDESC;
      _IOC_LogNotTested();  // comment out by unit testing
      break;
    }

    usleep(1000);  // 1ms
  } while (0x20240810);

  return Result;
}

static IOC_Result_T __IOC_postEVT_inConlesModeSyncTimed(
    /*ARG_IN*/ _ClsEvtLinkObj_pT pLinkObj,
    /*ARG_IN*/ const IOC_EvtDesc_pT pEvtDesc,
    /*ARG_IN*/ ULONG_T TimeoutUS) {
  IOC_Result_T Result = IOC_RESULT_BUG;

  struct timespec TS_Begin;
  clock_gettime(CLOCK_REALTIME, &TS_Begin);

  do {
    if (__IOC_ClsEvt_hasEvtDescInLinkObj(pLinkObj) == IOC_RESULT_NO) {
      __IOC_ClsEvt_callbackProcEvtOverSuberList(pLinkObj, pEvtDesc);
      _IOC_LogNotTested();  // comment out by unit testing
      Result = IOC_RESULT_SUCCESS;
      break;
    } else {
      __IOC_ClsEvt_wakeupLinkObjThread(pLinkObj);
    }

    struct timespec TS_End;
    clock_gettime(CLOCK_REALTIME, &TS_End);

    ULONG_T ElapsedUS = IOC_deltaTimeSpecInUS(&TS_Begin, &TS_End);
    if (ElapsedUS >= TimeoutUS) {
      Result = IOC_RESULT_TOO_LONG_EMPTYING_EVTDESC_QUEUE;
      _IOC_LogNotTested();  // comment out by unit testing
      break;
    }

    usleep(1000);  // 1ms
  } while (0x20240810);

  return Result;
}

//======>>>>>>END OF IMPLEMENT FOR ConlesEvent>>>>>>===================================================================