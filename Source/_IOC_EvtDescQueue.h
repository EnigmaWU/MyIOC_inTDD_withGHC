#include <pthread.h>

#include "_IOC_Logging.h"
#include "_IOC_Types.h"

#ifndef __IOC_EVTDESCQUEUE_H__
#define __IOC_EVTDESCQUEUE_H__
#ifdef __cplusplus
extern "C" {
#endif

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
   * IF QueuedEvtNum == ProcedEvtNum, the queue is empty.
   * IF QueuedEvtNum > ProcedEvtNum, the queue is not empty.
   * IF QueuedEvtNum - ProcedEvtNum == _CONLES_EVENT_MAX_QUEUING_EVTDESC, the queue is full.
   *
   * WHEN postEVT new EvtDesc, DO copy&save enqueuing EvtDesc in
   *    QueuedEvtDescs[QueuedEvtNum%_CONLES_EVENT_MAX_QUEUING_EVTDESC] and QueuedEvtNum++;
   * WHILE QueuedEvtNum > ProcedEvtNum, DO wakeup/loop the EvtProcThread, who will read one EvtDesc
   * from QueuedEvtDescs[ProcedEvtNum%_CONLES_EVENT_MAX_QUEUING_EVTDESC], proc one EvtDesc and
   * ProcedEvtNum++;
   */
  ULONG_T QueuedEvtNum, ProcedEvtNum;  // ULONG_T type is long lone enough to avoid overflow even
                                       // one event per nanosecond.
  IOC_EvtDesc_T QueuedEvtDescs[_CONLES_EVENT_MAX_QUEUING_EVTDESC];
} _IOC_EvtDescQueue_T, *_IOC_EvtDescQueue_pT;

void _IOC_EvtDescQueue_initOne(_IOC_EvtDescQueue_pT pEvtDescQueue);
void _IOC_EvtDescQueue_deinitOne(_IOC_EvtDescQueue_pT pEvtDescQueue);

// Return: IOC_RESULT_YES or IOC_RESULT_NO
IOC_BoolResult_T _IOC_EvtDescQueue_isEmpty(_IOC_EvtDescQueue_pT pEvtDescQueue);

// Return: IOC_RESULT_SUCCESS or IOC_RESULT_TOO_MANY_QUEUING_EVTDESC
IOC_Result_T _IOC_EvtDescQueue_enqueueElementLast(_IOC_EvtDescQueue_pT pEvtDescQueue,
                                                  /*ARG_IN*/ IOC_EvtDesc_pT pEvtDesc);
// Return: IOC_RESULT_SUCCESS or IOC_RESULT_EVENT_QUEUE_EMPTY
IOC_Result_T _IOC_EvtDescQueue_dequeueElementFirst(_IOC_EvtDescQueue_pT pEvtDescQueue,
                                                   /*ARG_OUT*/ IOC_EvtDesc_pT pEvtDesc);
#ifdef __cplusplus
}
#endif
#endif  // __IOC_EVTDESCQUEUE_H__