#include <IOC/IOC.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/_pthread/_pthread_mutex_t.h>
#include <sys/semaphore.h>
#include <unistd.h>

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//===>BEGIN: IOC Event in Conles Mode
#define _IOC_CONLES_MODE_MAX_EvtConsumer_NUM 16
static IOC_SubEvtArgs_T _mConlesModeSubEvtArgs[_IOC_CONLES_MODE_MAX_EvtConsumer_NUM] = {};

static pthread_mutex_t _mConlesModeSubedEvtArgsMutex                                 = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t _mConlesModeSubingEvtArgsMutex                                = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t _mConlesModeUnsubingEvtArgsMutex                              = PTHREAD_MUTEX_INITIALIZER;
static IOC_LinkState_T _mConlesModeLinkState                                         = IOC_LinkStateReady;
static IOC_LinkSubState_T _mConlesModeLinkSubState                                   = IOC_LinkSubState_ReadyIdle;

#define _IOC_CONLES_MODE_MAX_QUEUING_EVTDESC_NUMBER 20
typedef struct {
  /**
   * IF QueuedEvtNum == ProcedEvtNum, the queue is empty;
   * IF new postEVT, THEN alloc&copy a new pEvtDesc, AND save in
   *    pQueuedEvtDescs[QueuedEvtNum%_IOC_CONLES_MODE_MAX_QUEUING_EVTDESC_NUMBER] and QueuedEvtNum++;
   * IF QueuedEvtNum > ProcedEvtNum, THEN wakeup/loop the ConlesModeEvtCbThread, read pEvtDesc from
   *    pQueuedEvtDescs[ProcedEvtNum%_IOC_CONLES_MODE_MAX_QUEUING_EVTDESC_NUMBER], proc&free the pEvtDesc, and ProcedEvtNum++;
   */
  ULONG_T QueuedEvtNum, ProcedEvtNum;
  IOC_EvtDesc_pT pQueuedEvtDescs[_IOC_CONLES_MODE_MAX_QUEUING_EVTDESC_NUMBER];

  // pthread_mutex_t Mutex;
} _IOC_ConlesModeQueuingEvtDesc_T, *_IOC_ConlesModeQueuingEvtDesc_pT;

static _IOC_ConlesModeQueuingEvtDesc_T _mConlesModeQueuingEvtDesc[_IOC_CONLES_MODE_MAX_EvtConsumer_NUM] = {};

// ConlesModeEvtCbThread:
//   1) identify by _mConlesModeEvtCbThreadID and implement in __IOC_procEvtCbThread_inConlesMode
//   2) wakeup by __IOC_wakeupEvtCbTrhead_inConlesMode
//        |-> IF _mConlesModeEvtCbThreadID is not running, create it
//        |-> ELSE, wakeup it use pthread_cond_signal with _mConlesModeEvtCbThreadCond and _mConlesModeEvtCbThreadMutex

static pthread_t _mConlesModeEvtCbThreadID          = 0;
static pthread_cond_t _mConlesModeEvtCbThreadCond   = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t _mConlesModeEvtCbThreadMutex = PTHREAD_MUTEX_INITIALIZER;

static void* __IOC_procEvtCbThread_inConlesMode(void* pArg) {
  ULONG_T ProcedEvtNum = 0;

  pthread_cond_signal(&_mConlesModeEvtCbThreadCond);  // signal to wakeup the main thread to continue

  while (1) {
    if (!ProcedEvtNum) {
      pthread_mutex_lock(&_mConlesModeEvtCbThreadMutex);
      pthread_cond_wait(&_mConlesModeEvtCbThreadCond, &_mConlesModeEvtCbThreadMutex);
      pthread_mutex_unlock(&_mConlesModeEvtCbThreadMutex);
    } else {
      // IF ProcedEvtNum > 0, THEN dont wait, try to see if new incoming event from CbProcEvt_F
      ProcedEvtNum = 0;
    }

    pthread_mutex_lock(&_mConlesModeSubedEvtArgsMutex);
    for (int i = 0; i < _IOC_CONLES_MODE_MAX_EvtConsumer_NUM; i++) {  // forEach ConlesModeEvtConsumer
      IOC_SubEvtArgs_pT pSubEvtArgs = &_mConlesModeSubEvtArgs[i];
      if (!pSubEvtArgs->CbProcEvt_F) {
        continue;
      }

      _IOC_ConlesModeQueuingEvtDesc_pT pQueuingEvtDesc = &_mConlesModeQueuingEvtDesc[i];  // SubEvtArgsIdx==EvtQueuingIdx

      while (pQueuingEvtDesc->ProcedEvtNum < pQueuingEvtDesc->QueuedEvtNum) {  // forEach QueuedEvt
        ULONG_T NextProcEvtIdx  = pQueuingEvtDesc->ProcedEvtNum % _IOC_CONLES_MODE_MAX_QUEUING_EVTDESC_NUMBER;
        IOC_EvtDesc_pT pEvtDesc = pQueuingEvtDesc->pQueuedEvtDescs[NextProcEvtIdx];
        if (NULL == pEvtDesc) IOC_BugAbort();

        pQueuingEvtDesc->pQueuedEvtDescs[NextProcEvtIdx] = NULL;
        pQueuingEvtDesc->ProcedEvtNum++;

        //-------------------------------------------------------------------------------------------------------------
        // BEGIN: callback process event function
        for (int Idx = 0; Idx < pSubEvtArgs->EvtNum; Idx++) {  // forEach EvtConsumer's subedEvtID
          if (pEvtDesc->EvtID == pSubEvtArgs->pEvtIDs[Idx]) {
              pthread_mutex_lock(&_mConlesModeUnsubingEvtArgsMutex);
              pthread_mutex_lock(&_mConlesModeSubingEvtArgsMutex);
              pthread_mutex_unlock(&_mConlesModeSubedEvtArgsMutex);

              _mConlesModeLinkState    = IOC_LinkStateBusy;
              _mConlesModeLinkSubState = IOC_LinkSubState_BusyProcing;
              pSubEvtArgs->CbProcEvt_F(pEvtDesc, pSubEvtArgs->pCbPrivData);
              _mConlesModeLinkState    = IOC_LinkStateReady;
              _mConlesModeLinkSubState = IOC_LinkSubState_ReadyIdle;

              pthread_mutex_unlock(&_mConlesModeSubingEvtArgsMutex);
              pthread_mutex_unlock(&_mConlesModeUnsubingEvtArgsMutex);
              pthread_mutex_lock(&_mConlesModeSubedEvtArgsMutex);
          }
        }
        free(pEvtDesc);
        ProcedEvtNum++;
        // END: callback process event function

        //-------------------------------------------------------------------------------------------------------------
      }
    }
    pthread_mutex_unlock(&_mConlesModeSubedEvtArgsMutex);
  }

  return NULL;
}

static void __IOC_wakeupEvtCbTrhead_inConlesMode() {
  pthread_mutex_lock(&_mConlesModeEvtCbThreadMutex);
  if (_mConlesModeEvtCbThreadID == 0) {
    pthread_create(&_mConlesModeEvtCbThreadID, NULL, __IOC_procEvtCbThread_inConlesMode, NULL);
    // wait thread is created and entered the thread function
    pthread_cond_wait(&_mConlesModeEvtCbThreadCond, &_mConlesModeEvtCbThreadMutex);
  }
  pthread_mutex_unlock(&_mConlesModeEvtCbThreadMutex);

  pthread_cond_signal(&_mConlesModeEvtCbThreadCond);
}

static IOC_Result_T __IOC_subEVT_inConlesMode(
    /*ARG_IN*/ const IOC_SubEvtArgs_pT pSubingEvtArgs) {
  IOC_Result_T Result                = IOC_RESULT_BUG;
  _mConlesModeLinkSubState           = IOC_LinkSubState_ReadyLocked;
  IOC_SubEvtArgs_pT pSubedEvtArgs    = &_mConlesModeSubEvtArgs[0];

  pthread_mutex_lock(&_mConlesModeSubedEvtArgsMutex);
  pthread_mutex_lock(&_mConlesModeSubingEvtArgsMutex);
  for (int i = 0; i < _IOC_CONLES_MODE_MAX_EvtConsumer_NUM; i++) {
    if (pSubedEvtArgs->CbProcEvt_F == pSubingEvtArgs->CbProcEvt_F &&
        pSubedEvtArgs->pCbPrivData == pSubingEvtArgs->pCbPrivData) {
      pthread_mutex_unlock(&_mConlesModeSubedEvtArgsMutex);
      Result = IOC_RESULT_CONFLICT_EVENT_CONSUMER;
      goto _exitSubEVT;
    }

    if (pSubedEvtArgs->CbProcEvt_F == NULL) {
      pSubedEvtArgs->CbProcEvt_F = pSubingEvtArgs->CbProcEvt_F;
      pSubedEvtArgs->pCbPrivData = pSubingEvtArgs->pCbPrivData;

      pSubedEvtArgs->EvtNum  = pSubingEvtArgs->EvtNum;
      size_t EvtIDsSize      = pSubingEvtArgs->EvtNum * sizeof(IOC_EvtID_T);
      pSubedEvtArgs->pEvtIDs = malloc(EvtIDsSize);
      memcpy(pSubedEvtArgs->pEvtIDs, pSubingEvtArgs->pEvtIDs, EvtIDsSize);

      pthread_mutex_unlock(&_mConlesModeSubingEvtArgsMutex);
      pthread_mutex_unlock(&_mConlesModeSubedEvtArgsMutex);

      Result = IOC_RESULT_SUCCESS;
      goto _exitSubEVT;
    }
    pSubedEvtArgs++;
  }

  pthread_mutex_unlock(&_mConlesModeSubingEvtArgsMutex);
  pthread_mutex_unlock(&_mConlesModeSubedEvtArgsMutex);

  Result = IOC_RESULT_TOO_MANY_EVENT_CONSUMER;

_exitSubEVT:
  _mConlesModeLinkSubState = IOC_LinkSubState_ReadyIdle;
  return Result;
}

static IOC_Result_T __IOC_unsubEVT_inConlesMode(
    /*ARG_IN*/ const IOC_UnsubEvtArgs_pT pUnsubEvtArgs) {
  IOC_Result_T Result                = IOC_RESULT_BUG;
  _mConlesModeLinkSubState           = IOC_LinkSubState_ReadyLocked;
  IOC_SubEvtArgs_pT pSavedSubEvtArgs = &_mConlesModeSubEvtArgs[0];

  pthread_mutex_lock(&_mConlesModeUnsubingEvtArgsMutex);
  pthread_mutex_lock(&_mConlesModeSubedEvtArgsMutex);
  for (int i = 0; i < _IOC_CONLES_MODE_MAX_EvtConsumer_NUM; i++) {
    if (pSavedSubEvtArgs->CbProcEvt_F && (pSavedSubEvtArgs->CbProcEvt_F == pUnsubEvtArgs->CbProcEvt_F) &&
        (pSavedSubEvtArgs->pCbPrivData == pUnsubEvtArgs->pCbPrivData)) {
      if (pSavedSubEvtArgs->pEvtIDs) free(pSavedSubEvtArgs->pEvtIDs);
      memset(pSavedSubEvtArgs, 0, sizeof(IOC_SubEvtArgs_T));

      // Clear the QueuingEvtDesc
      _IOC_ConlesModeQueuingEvtDesc_pT pQueuingEvtDesc = &_mConlesModeQueuingEvtDesc[i];
      for (int j = 0; j < _IOC_CONLES_MODE_MAX_QUEUING_EVTDESC_NUMBER; j++) {
        if (pQueuingEvtDesc->pQueuedEvtDescs[j]) {
          free(pQueuingEvtDesc->pQueuedEvtDescs[j]);
          pQueuingEvtDesc->pQueuedEvtDescs[j] = NULL;
        }
      }
      pQueuingEvtDesc->QueuedEvtNum = 0;
      pQueuingEvtDesc->ProcedEvtNum = 0;

      pthread_mutex_unlock(&_mConlesModeSubedEvtArgsMutex);
      pthread_mutex_unlock(&_mConlesModeUnsubingEvtArgsMutex);
      Result = IOC_RESULT_SUCCESS;
      goto _exitUnsubEVT;
    }
    pSavedSubEvtArgs++;
  }
  pthread_mutex_unlock(&_mConlesModeSubedEvtArgsMutex);
  pthread_mutex_unlock(&_mConlesModeUnsubingEvtArgsMutex);
  Result = IOC_RESULT_NO_EVENT_CONSUMER;

_exitUnsubEVT:
  _mConlesModeLinkSubState = IOC_LinkSubState_ReadyIdle;
  return Result;
}

static IOC_Result_T __IOC_postEVT_inConlesMode(
    /*ARG_IN*/ const IOC_EvtDesc_pT pEvtDesc,
    /*ARG_IN_OPTIONAL*/ const IOC_Options_pT pOptions) {
  ULONG_T CbProcEvtCnt = 0;
  IOC_Result_T SyncModeResult = IOC_RESULT_BUG;

  bool IsNonBlockMode = true;
  // TODO: IF pOptions is not NULL, THEN parse the IOC_OPTID_NONBLOCK_MODE

  // IsSyncMode<DFT=FALSE> parsed from pOptions is not NULL
  bool IsSyncMode = false;
  if (pOptions != NULL) {
    IsSyncMode = (pOptions->IDs & IOC_OPTID_SYNC_MODE) ? true : false;
  }

  pthread_mutex_lock(&_mConlesModeSubedEvtArgsMutex);
  // forEach ConlesModeEvtConsumer, IF ANY NotProcedEvt, THEN return IOC_RESULT_TOO_MANY_QUEUING_EVTDESC
  for (int CosmerIdx = 0; CosmerIdx < _IOC_CONLES_MODE_MAX_EvtConsumer_NUM; CosmerIdx++) {
    _IOC_ConlesModeQueuingEvtDesc_pT pQueuingEvtDesc = &_mConlesModeQueuingEvtDesc[CosmerIdx];

    ULONG_T NotProcedEvtNum = pQueuingEvtDesc->QueuedEvtNum - pQueuingEvtDesc->ProcedEvtNum;
    if (NotProcedEvtNum >= _IOC_CONLES_MODE_MAX_QUEUING_EVTDESC_NUMBER) {
      pthread_mutex_unlock(&_mConlesModeSubedEvtArgsMutex);

      __IOC_wakeupEvtCbTrhead_inConlesMode();
      return IOC_RESULT_TOO_MANY_QUEUING_EVTDESC;
    }
  }

  for (int CosmerIdx = 0; CosmerIdx < _IOC_CONLES_MODE_MAX_EvtConsumer_NUM; CosmerIdx++) {  // forEach ConlesModeEvtConsumer
    IOC_SubEvtArgs_pT pSubedEvtArgs = &_mConlesModeSubEvtArgs[CosmerIdx];
    if (!pSubedEvtArgs->CbProcEvt_F) {
      continue;
    }

    for (int j = 0; j < pSubedEvtArgs->EvtNum; j++) {  // forEach EvtConsumer's subedEvtID
      if (pEvtDesc->EvtID != pSubedEvtArgs->pEvtIDs[j]) {
        continue;  // IF pEvtDesc->EvtID is not subed by the EvtConsumer
      }

      if (IsSyncMode) {
        SyncModeResult = pSubedEvtArgs->CbProcEvt_F(pEvtDesc, pSubedEvtArgs->pCbPrivData);
        if (IOC_RESULT_SUCCESS != SyncModeResult) {
          break;  // FIXME: IF Multi-EvtConsumer but one CbProcEvt fail, how to do?
        }
      } else {
        // Queuing the pEvtDesc
        _IOC_ConlesModeQueuingEvtDesc_pT pQueuingEvtDesc =
            &_mConlesModeQueuingEvtDesc[CosmerIdx];  // SubEvtArgsIdx==EvtQueuingIdx

        ULONG_T NextQueuingEvtIdx = pQueuingEvtDesc->QueuedEvtNum % _IOC_CONLES_MODE_MAX_QUEUING_EVTDESC_NUMBER;

        if (pQueuingEvtDesc->pQueuedEvtDescs[NextQueuingEvtIdx]) {
          IOC_BugAbort();
        }
        pQueuingEvtDesc->pQueuedEvtDescs[NextQueuingEvtIdx] = malloc(sizeof(IOC_EvtDesc_T));
        memcpy(pQueuingEvtDesc->pQueuedEvtDescs[NextQueuingEvtIdx], pEvtDesc, sizeof(IOC_EvtDesc_T));

        pQueuingEvtDesc->QueuedEvtNum++;

        __IOC_wakeupEvtCbTrhead_inConlesMode();
      }
      CbProcEvtCnt++;
    }
  }
  pthread_mutex_unlock(&_mConlesModeSubedEvtArgsMutex);

  if (IsSyncMode) {
    return SyncModeResult;
  } else if (CbProcEvtCnt > 0) {
    return IOC_RESULT_SUCCESS;
  } else {
    return IOC_RESULT_NO_EVENT_CONSUMER;
  }
}

static void __IOC_forceProcEVT_inConlesMode() {
  pthread_mutex_lock(&_mConlesModeSubedEvtArgsMutex);
  for (int i = 0; i < _IOC_CONLES_MODE_MAX_EvtConsumer_NUM; i++) {
    IOC_SubEvtArgs_pT pSubEvtArgs = &_mConlesModeSubEvtArgs[i];
    if (!pSubEvtArgs->CbProcEvt_F) {
      continue;
    }

    _IOC_ConlesModeQueuingEvtDesc_pT pQueuingEvtDesc = &_mConlesModeQueuingEvtDesc[i];
    while (pQueuingEvtDesc->ProcedEvtNum < pQueuingEvtDesc->QueuedEvtNum) {
      ULONG_T NextProcEvtIdx  = pQueuingEvtDesc->ProcedEvtNum % _IOC_CONLES_MODE_MAX_QUEUING_EVTDESC_NUMBER;
      IOC_EvtDesc_pT pEvtDesc = pQueuingEvtDesc->pQueuedEvtDescs[NextProcEvtIdx];
      if (NULL == pEvtDesc) IOC_BugAbort();

      pQueuingEvtDesc->pQueuedEvtDescs[NextProcEvtIdx] = NULL;
      pQueuingEvtDesc->ProcedEvtNum++;

      //-------------------------------------------------------------------------------------------------------------
      // BEGIN: callback process event function
      for (int Idx = 0; Idx < pSubEvtArgs->EvtNum; Idx++) {  // forEach EvtConsumer's subedEvtID
        if (pEvtDesc->EvtID == pSubEvtArgs->pEvtIDs[Idx]) {
          pSubEvtArgs->CbProcEvt_F(pEvtDesc, pSubEvtArgs->pCbPrivData);
        }
      }
      // END: callback process event function
      //-------------------------------------------------------------------------------------------------------------

      free(pEvtDesc);
    }
  }
  pthread_mutex_unlock(&_mConlesModeSubedEvtArgsMutex);
}

//===>END: IOC Event in Conles Mode
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IOC_Result_T IOC_postEVT(
    /*ARG_IN*/IOC_LinkID_T LinkID, 
    /*ARG_IN*/const IOC_EvtDesc_pT pEvtDesc,
    /*ARG_IN_OPTIONAL*/IOC_Options_pT pOptions)
{
  pEvtDesc->MsgDesc.TimeStamp = IOC_getCurrentTimeSpec();

  if (LinkID == IOC_CONLES_MODE_AUTO_LINK_ID) {
    return __IOC_postEVT_inConlesMode(pEvtDesc, pOptions);
  } else {
    return IOC_RESULT_NOT_IMPLEMENTED;
  }
}

IOC_Result_T IOC_subEVT(
    /*ARG_IN*/IOC_LinkID_T LinkID, 
    /*ARG_IN*/const IOC_SubEvtArgs_pT pSubEvtArgs)
{
  if (LinkID == IOC_CONLES_MODE_AUTO_LINK_ID) {
    return __IOC_subEVT_inConlesMode(pSubEvtArgs);
  } else {
    return IOC_RESULT_NOT_IMPLEMENTED;
  }
}

IOC_Result_T IOC_unsubEVT(
    /*ARG_IN*/IOC_LinkID_T LinkID, 
    /*ARG_IN*/const IOC_UnsubEvtArgs_pT pUnsubEvtArgs)
{
  if (LinkID == IOC_CONLES_MODE_AUTO_LINK_ID) {
    return __IOC_unsubEVT_inConlesMode(pUnsubEvtArgs);
  } else {
    return IOC_RESULT_NOT_IMPLEMENTED;
  }
}

void IOC_forceProcEVT(void) { __IOC_forceProcEVT_inConlesMode(); }

IOC_Result_T IOC_getCapabilty(
    /*ARG_INOUT*/ IOC_CapabiltyDescription_pT pCapDesc) {
  IOC_Result_T Result = IOC_RESULT_BUG;

  switch (pCapDesc->CapID) {
    case IOC_CAPID_CONLES_MODE_EVENT: {
      pCapDesc->ConlesModeEvent.MaxEvtConsumer = _IOC_CONLES_MODE_MAX_EvtConsumer_NUM;
      Result = IOC_RESULT_SUCCESS;
    } break;

    default:
      Result = IOC_RESULT_NOT_SUPPORT;
      break;
  }

  return Result;
}

IOC_Result_T IOC_getLinkState(
    /*ARG_IN*/ IOC_LinkID_T LinkID,
    /*ARG_OUT*/ IOC_LinkState_pT pLinkState,
    /*ARG_OUT_OPTIONAL*/ IOC_LinkSubState_pT pLinkSubState) {
  if (LinkID == IOC_CONLES_MODE_AUTO_LINK_ID) {
    *pLinkState    = _mConlesModeLinkState;
    *pLinkSubState = _mConlesModeLinkSubState;
    return IOC_RESULT_SUCCESS;
  } else {
    return IOC_RESULT_NOT_IMPLEMENTED;
  }
}
