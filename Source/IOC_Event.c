#include <IOC/IOC.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//===>BEGIN: IOC Event in Conles Mode
#define _IOC_CONLES_MODE_MAX_EvtConsumer_NUNBER 16
static IOC_SubEvtArgs_T _mConlesModeSubEvtArgs[_IOC_CONLES_MODE_MAX_EvtConsumer_NUNBER] = {};
static pthread_mutex_t _mConlesModeSubEvtArgsMutex = PTHREAD_MUTEX_INITIALIZER;

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

static _IOC_ConlesModeQueuingEvtDesc_T _mConlesModeQueuingEvtDesc[_IOC_CONLES_MODE_MAX_EvtConsumer_NUNBER] = {};

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

  while (1) {
    if (!ProcedEvtNum) {
      pthread_mutex_lock(&_mConlesModeEvtCbThreadMutex);
      pthread_cond_wait(&_mConlesModeEvtCbThreadCond, &_mConlesModeEvtCbThreadMutex);
      pthread_mutex_unlock(&_mConlesModeEvtCbThreadMutex);
    } else {
      // IF ProcedEvtNum > 0, THEN dont wait, try to see if new incoming event from CbProcEvt_F
      ProcedEvtNum = 0;
    }

    pthread_mutex_lock(&_mConlesModeSubEvtArgsMutex);
    for (int i = 0; i < _IOC_CONLES_MODE_MAX_EvtConsumer_NUNBER; i++) {  // forEach ConlesModeEvtConsumer
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
        pthread_mutex_unlock(&_mConlesModeSubEvtArgsMutex);
        // BEGIN: callback process event function
        for (int Idx = 0; Idx < pSubEvtArgs->EvtNum; Idx++) {  // forEach EvtConsumer's subedEvtID
          if (pEvtDesc->EvtID == pSubEvtArgs->pEvtIDs[Idx]) {
            pSubEvtArgs->CbProcEvt_F(pEvtDesc, pSubEvtArgs->pCbPrivData);
          }
        }
        free(pEvtDesc);
        ProcedEvtNum++;
        // END: callback process event function
        pthread_mutex_lock(&_mConlesModeSubEvtArgsMutex);
        //-------------------------------------------------------------------------------------------------------------
      }
    }
    pthread_mutex_unlock(&_mConlesModeSubEvtArgsMutex);
  }

  return NULL;
}

static void __IOC_wakeupEvtCbTrhead_inConlesMode() {
  pthread_mutex_lock(&_mConlesModeEvtCbThreadMutex);
  if (_mConlesModeEvtCbThreadID == 0) {
    pthread_create(&_mConlesModeEvtCbThreadID, NULL, __IOC_procEvtCbThread_inConlesMode, NULL);
  } else {
    pthread_cond_signal(&_mConlesModeEvtCbThreadCond);
  }
  pthread_mutex_unlock(&_mConlesModeEvtCbThreadMutex);
}

static IOC_Result_T __IOC_subEVT_inConlesMode(
    /*ARG_IN*/ const IOC_SubEvtArgs_pT pSubEvtArgs) {
  IOC_SubEvtArgs_pT pSavedSubEvtArgs = &_mConlesModeSubEvtArgs[0];

  pthread_mutex_lock(&_mConlesModeSubEvtArgsMutex);
  for (int i = 0; i < _IOC_CONLES_MODE_MAX_EvtConsumer_NUNBER; i++) {
    if (pSavedSubEvtArgs->CbProcEvt_F == pSubEvtArgs->CbProcEvt_F &&
        pSavedSubEvtArgs->pCbPrivData == pSubEvtArgs->pCbPrivData) {
      pthread_mutex_unlock(&_mConlesModeSubEvtArgsMutex);
      return IOC_RESULT_CONFLICT_EvtConsumer;
    }

    if (pSavedSubEvtArgs->CbProcEvt_F == NULL) {
      pSavedSubEvtArgs->CbProcEvt_F = pSubEvtArgs->CbProcEvt_F;
      pSavedSubEvtArgs->pCbPrivData = pSubEvtArgs->pCbPrivData;

      pSavedSubEvtArgs->EvtNum = pSubEvtArgs->EvtNum;
      size_t EvtIDsSize = pSubEvtArgs->EvtNum * sizeof(IOC_EvtID_T);
      pSavedSubEvtArgs->pEvtIDs = malloc(EvtIDsSize);
      memcpy(pSavedSubEvtArgs->pEvtIDs, pSubEvtArgs->pEvtIDs, EvtIDsSize);

      pthread_mutex_unlock(&_mConlesModeSubEvtArgsMutex);
      return IOC_RESULT_SUCCESS;
    }
    pSavedSubEvtArgs++;
  }

  pthread_mutex_unlock(&_mConlesModeSubEvtArgsMutex);
  return IOC_RESULT_TOO_MANY_EvtConsumer;
}

static IOC_Result_T __IOC_unsubEVT_inConlesMode(
    /*ARG_IN*/ const IOC_UnsubEvtArgs_pT pUnsubEvtArgs) {
  IOC_SubEvtArgs_pT pSavedSubEvtArgs = &_mConlesModeSubEvtArgs[0];

  pthread_mutex_lock(&_mConlesModeSubEvtArgsMutex);
  for (int i = 0; i < _IOC_CONLES_MODE_MAX_EvtConsumer_NUNBER; i++) {
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

      pthread_mutex_unlock(&_mConlesModeSubEvtArgsMutex);
      return IOC_RESULT_SUCCESS;
    }
    pSavedSubEvtArgs++;
  }
  pthread_mutex_unlock(&_mConlesModeSubEvtArgsMutex);

  return IOC_RESULT_NO_EvtConsumer;
}

static IOC_Result_T __IOC_postEVT_inConlesMode(
    /*ARG_IN*/ const IOC_EvtDesc_pT pEvtDesc,
    /*ARG_IN_OPTIONAL*/ const IOC_Options_pT pOptions) {
  ULONG_T CbProcEvtCnt = 0;

  bool IsNonBlockMode = true;
  // TODO: IF pOptions is not NULL, THEN parse the IOC_OPTID_NONBLOCK_MODE

  // IsSyncMode<DFT=FALSE> parsed from pOptions is not NULL
  bool IsSyncMode = false;
  if (pOptions != NULL) {
    IsSyncMode = (pOptions->IDs & IOC_OPTID_SYNC_MODE) ? true : false;
  }

  pthread_mutex_lock(&_mConlesModeSubEvtArgsMutex);
  // forEach ConlesModeEvtConsumer, IF ANY NotProcedEvt, THEN return IOC_RESULT_TOO_MANY_QUEUING_EVTDESC
  for (int CosmerIdx = 0; CosmerIdx < _IOC_CONLES_MODE_MAX_EvtConsumer_NUNBER; CosmerIdx++) {
    _IOC_ConlesModeQueuingEvtDesc_pT pQueuingEvtDesc = &_mConlesModeQueuingEvtDesc[CosmerIdx];

    ULONG_T NotProcedEvtNum = pQueuingEvtDesc->QueuedEvtNum - pQueuingEvtDesc->ProcedEvtNum;
    if (NotProcedEvtNum >= _IOC_CONLES_MODE_MAX_QUEUING_EVTDESC_NUMBER) {
      pthread_mutex_unlock(&_mConlesModeSubEvtArgsMutex);

      __IOC_wakeupEvtCbTrhead_inConlesMode();
      return IOC_RESULT_TOO_MANY_QUEUING_EVTDESC;
    }
  }

  for (int CosmerIdx = 0; CosmerIdx < _IOC_CONLES_MODE_MAX_EvtConsumer_NUNBER; CosmerIdx++) {  // forEach ConlesModeEvtConsumer
    IOC_SubEvtArgs_pT pSavedSubEvtArgs = &_mConlesModeSubEvtArgs[CosmerIdx];
    if (!pSavedSubEvtArgs->CbProcEvt_F) {
      continue;
    }

    for (int j = 0; j < pSavedSubEvtArgs->EvtNum; j++) {  // forEach EvtConsumer's subedEvtID
      if (pEvtDesc->EvtID != pSavedSubEvtArgs->pEvtIDs[j]) {
        continue;  // IF pEvtDesc->EvtID is not subed by the EvtConsumer
      }

      if (IsSyncMode) {
        pSavedSubEvtArgs->CbProcEvt_F(pEvtDesc, pSavedSubEvtArgs->pCbPrivData);
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
  pthread_mutex_unlock(&_mConlesModeSubEvtArgsMutex);

  if (CbProcEvtCnt > 0) {
    return IOC_RESULT_SUCCESS;
  } else {
    return IOC_RESULT_NO_EvtConsumer;
  }
}

static void __IOC_forceProcEVT_inConlesMode() {
  pthread_mutex_lock(&_mConlesModeSubEvtArgsMutex);
  for (int i = 0; i < _IOC_CONLES_MODE_MAX_EvtConsumer_NUNBER; i++) {
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
  pthread_mutex_unlock(&_mConlesModeSubEvtArgsMutex);
}

//===>END: IOC Event in Conles Mode
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IOC_Result_T IOC_postEVT(
    /*ARG_IN*/IOC_LinkID_T LinkID, 
    /*ARG_IN*/const IOC_EvtDesc_pT pEvtDesc,
    /*ARG_IN_OPTIONAL*/IOC_Options_pT pOptions)
{
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
      pCapDesc->ConlesModeEvent.MaxEvtConsumer = _IOC_CONLES_MODE_MAX_EvtConsumer_NUNBER;
      Result = IOC_RESULT_SUCCESS;
    } break;

    default:
      Result = IOC_RESULT_NOT_SUPPORT;
      break;
  }

  return Result;
}