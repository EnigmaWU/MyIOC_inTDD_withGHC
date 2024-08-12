#include "_IOC_EvtDescQueue.h"

void _IOC_EvtDescQueue_initOne(_IOC_EvtDescQueue_pT pEvtDescQueue) {
  pthread_mutex_init(&pEvtDescQueue->Mutex, NULL);
  pEvtDescQueue->QueuedEvtNum = 0;
  pEvtDescQueue->ProcedEvtNum = 0;

  // clear all EvtDesc in QueuedEvtDescs
  memset(pEvtDescQueue->QueuedEvtDescs, 0, sizeof(pEvtDescQueue->QueuedEvtDescs));
}

void _IOC_EvtDescQueue_deinitOne(_IOC_EvtDescQueue_pT pEvtDescQueue) {
  // deinit all checkable members only

  // check conditions which matching destoriability
  _IOC_LogAssert(pEvtDescQueue->QueuedEvtNum == pEvtDescQueue->ProcedEvtNum);

  int PosixResult = pthread_mutex_destroy(&pEvtDescQueue->Mutex);
  if (PosixResult != 0) {
    _IOC_LogAssert(PosixResult == 0);
  }
}

IOC_BoolResult_T _IOC_EvtDescQueue_isEmpty(_IOC_EvtDescQueue_pT pEvtDescQueue) {
  pthread_mutex_lock(&pEvtDescQueue->Mutex);
  IOC_BoolResult_T Result =
      (pEvtDescQueue->QueuedEvtNum == pEvtDescQueue->ProcedEvtNum) ? IOC_RESULT_YES : IOC_RESULT_NO;
  pthread_mutex_unlock(&pEvtDescQueue->Mutex);
  return Result;
}

IOC_Result_T _IOC_EvtDescQueue_enqueueElementLast(_IOC_EvtDescQueue_pT pEvtDescQueue,
                                                  IOC_EvtDesc_pT pEvtDesc) {
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

  //_IOC_LogDebug("Enqueued EvtDesc(SeqID=%lu, EvtID(%lu,%lu)) to EvtDescQueue(Pos=%lu,Proced=%lu <=
  //Queued=%lu)",
  //              pEvtDesc->MsgDesc.SeqID, IOC_getEvtClassID(pEvtDesc->EvtID),
  //              IOC_getEvtNameID(pEvtDesc->EvtID), NextQueuingPos, ProcedEvtNum, QueuedEvtNum);

  return IOC_RESULT_SUCCESS;
}

IOC_Result_T _IOC_EvtDescQueue_dequeueElementFirst(_IOC_EvtDescQueue_pT pEvtDescQueue,
                                                   IOC_EvtDesc_pT pEvtDesc) {
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

  //_IOC_LogDebug("Dequeued EvtDesc(SeqID=%lu, EvtID(%lu,%lu)) from EvtDescQueue(Pos=%lu,Proced=%lu
  //<= Queued=%lu)",
  //              pEvtDesc->MsgDesc.SeqID, IOC_getEvtClassID(pEvtDesc->EvtID),
  //              IOC_getEvtNameID(pEvtDesc->EvtID), NextProcingPos, ProcedEvtNum, QueuedEvtNum);

  return IOC_RESULT_SUCCESS;
}