#include <IOC/IOC.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//===>BEGIN: IOC Event in Conles Mode
#define _IOC_CONLES_MODE_MAX_EVTCOSMER_NUNBER 16
static IOC_SubEvtArgs_T _mConlesModeSubEvtArgs[_IOC_CONLES_MODE_MAX_EVTCOSMER_NUNBER] = {};
static pthread_mutex_t _mConlesModeSubEvtArgsMutex = PTHREAD_MUTEX_INITIALIZER;

#define _IOC_CONLES_MODE_MAX_QUEUING_EVTDESC_NUMBER 10
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
} _IOC_ConlesModeQueuingEvtDesc_T, *_IOC_ConlesModeQueuingEvtDesc_pT;

static _IOC_ConlesModeQueuingEvtDesc_T _mConlesModeQueuingEvtDesc[_IOC_CONLES_MODE_MAX_EVTCOSMER_NUNBER] = {};

// ConlesModeEvtCbThread:
//   1) identify by _mConlesModeEvtCbThreadID and implement in __IOC_procEvtCbThread_inConlesMode
//   2) wakeup by __IOC_wakeupEvtCbTrhead_inConlesMode
//        |-> IF _mConlesModeEvtCbThreadID is not running, create it
//        |-> ELSE, wakeup it use pthread_cond_signal with _mConlesModeEvtCbThreadCond and _mConlesModeEvtCbThreadMutex

static pthread_t _mConlesModeEvtCbThreadID          = 0;
static pthread_cond_t _mConlesModeEvtCbThreadCond   = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t _mConlesModeEvtCbThreadMutex = PTHREAD_MUTEX_INITIALIZER;

static void* __IOC_procEvtCbThread_inConlesMode(void* pArg) {
  while (1) {
    pthread_mutex_lock(&_mConlesModeEvtCbThreadMutex);
    pthread_cond_wait(&_mConlesModeEvtCbThreadCond, &_mConlesModeEvtCbThreadMutex);
    pthread_mutex_unlock(&_mConlesModeEvtCbThreadMutex);

    for (int i = 0; i < _IOC_CONLES_MODE_MAX_EVTCOSMER_NUNBER; i++) {
      IOC_SubEvtArgs_pT pSubEvtArgs = &_mConlesModeSubEvtArgs[i];

      if (pSubEvtArgs->CbProcEvt_F) {
        _IOC_ConlesModeQueuingEvtDesc_pT pQueuingEvtDesc = &_mConlesModeQueuingEvtDesc[i];
        ULONG_T MaxEvtNum                                = _IOC_CONLES_MODE_MAX_QUEUING_EVTDESC_NUMBER;

        while (pQueuingEvtDesc->ProcedEvtNum < pQueuingEvtDesc->QueuedEvtNum) {
          ULONG_T ProcedEvtNum    = pQueuingEvtDesc->ProcedEvtNum;
          IOC_EvtDesc_pT pEvtDesc = pQueuingEvtDesc->pQueuedEvtDescs[ProcedEvtNum % MaxEvtNum];
          pQueuingEvtDesc->ProcedEvtNum++;

          for (int j = 0; j < pSubEvtArgs->EvtNum; j++) {
            // if (pEvtDesc->EvtID == pSubEvtArgs->pEvtIDs[j])
            { pSubEvtArgs->CbProcEvt_F(pEvtDesc, pSubEvtArgs->pCbPrivData); }
          }
          free(pEvtDesc);
        }
      }
    }
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

  for (int i = 0; i < _IOC_CONLES_MODE_MAX_EVTCOSMER_NUNBER; i++) {
    if (pSavedSubEvtArgs->CbProcEvt_F == pSubEvtArgs->CbProcEvt_F &&
        pSavedSubEvtArgs->pCbPrivData == pSubEvtArgs->pCbPrivData) {
      return IOC_RESULT_CONFLICT_EVTCOSMER;
    }

    if (pSavedSubEvtArgs->CbProcEvt_F == NULL) {
      pSavedSubEvtArgs->CbProcEvt_F = pSubEvtArgs->CbProcEvt_F;
      pSavedSubEvtArgs->pCbPrivData = pSubEvtArgs->pCbPrivData;

      pSavedSubEvtArgs->EvtNum = pSubEvtArgs->EvtNum;
      size_t EvtIDsSize = pSubEvtArgs->EvtNum * sizeof(IOC_EvtID_T);
      pSavedSubEvtArgs->pEvtIDs = malloc(EvtIDsSize);
      memcpy(pSavedSubEvtArgs->pEvtIDs, pSubEvtArgs->pEvtIDs, EvtIDsSize);

      return IOC_RESULT_SUCCESS;
    }
    pSavedSubEvtArgs++;
  }

  return IOC_RESULT_TOO_MANY_EVTCOSMER;
}

static IOC_Result_T __IOC_unsubEVT_inConlesMode(
    /*ARG_IN*/ const IOC_UnsubEvtArgs_pT pUnsubEvtArgs) {
  IOC_SubEvtArgs_pT pSavedSubEvtArgs = &_mConlesModeSubEvtArgs[0];

  for (int i = 0; i < _IOC_CONLES_MODE_MAX_EVTCOSMER_NUNBER; i++) {
    if (pSavedSubEvtArgs->CbProcEvt_F && (pSavedSubEvtArgs->CbProcEvt_F == pUnsubEvtArgs->CbProcEvt_F) &&
        (pSavedSubEvtArgs->pCbPrivData == pUnsubEvtArgs->pCbPrivData)) {
      memset(pSavedSubEvtArgs, 0, sizeof(IOC_SubEvtArgs_T));
      return IOC_RESULT_SUCCESS;
    }
    pSavedSubEvtArgs++;
  }

  return IOC_RESULT_NO_EVTCOSMER;
}

static IOC_Result_T __IOC_postEVT_inConlesMode(
    /*ARG_IN*/ const IOC_EvtDesc_pT pEvtDesc,
    /*ARG_IN_OPTIONAL*/ const IOC_Options_pT pOptions) {
  ULONG_T CbProcEvtCnt = 0;
  IOC_SubEvtArgs_pT pSavedSubEvtArgs = &_mConlesModeSubEvtArgs[0];

  pthread_mutex_lock(&_mConlesModeSubEvtArgsMutex);
  for (int i = 0; i < _IOC_CONLES_MODE_MAX_EVTCOSMER_NUNBER; i++) {
    if (pSavedSubEvtArgs->CbProcEvt_F) {
      for (int j = 0; j < pSavedSubEvtArgs->EvtNum; j++) {
        if (pEvtDesc->EvtID == pSavedSubEvtArgs->pEvtIDs[j]) {
        // Queuing the pEvtDesc
        _IOC_ConlesModeQueuingEvtDesc_pT pQueuingEvtDesc = &_mConlesModeQueuingEvtDesc[i];
        pQueuingEvtDesc->pQueuedEvtDescs[pQueuingEvtDesc->QueuedEvtNum % _IOC_CONLES_MODE_MAX_QUEUING_EVTDESC_NUMBER] =
            malloc(sizeof(IOC_EvtDesc_T));
        memcpy(pQueuingEvtDesc->pQueuedEvtDescs[pQueuingEvtDesc->QueuedEvtNum % _IOC_CONLES_MODE_MAX_QUEUING_EVTDESC_NUMBER],
               pEvtDesc, sizeof(IOC_EvtDesc_T));
        pQueuingEvtDesc->QueuedEvtNum++;

        __IOC_wakeupEvtCbTrhead_inConlesMode();

        CbProcEvtCnt++;
        }
      }
    }
    pSavedSubEvtArgs++;
  }
  pthread_mutex_unlock(&_mConlesModeSubEvtArgsMutex);

  if (CbProcEvtCnt > 0) {
    return IOC_RESULT_SUCCESS;
  } else {
    return IOC_RESULT_NO_EVTCOSMER;
  }
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

IOC_Result_T IOC_getCapabilty(
    /*ARG_INOUT*/ IOC_CapabiltyDescription_pT pCapDesc) {
  IOC_Result_T Result = IOC_RESULT_BUG;

  switch (pCapDesc->CapID) {
    case IOC_CAPID_CONLES_MODE_EVENT: {
      pCapDesc->ConlesModeEvent.MaxEvtCosmer = _IOC_CONLES_MODE_MAX_EVTCOSMER_NUNBER;
      Result = IOC_RESULT_SUCCESS;
    } break;

    default:
      Result = IOC_RESULT_NOT_SUPPORT;
      break;
  }

  return Result;
}