#include <gtest/gtest.h>

#include "_IOC_ConlesEvent.c"  //All the implementation of ConlesEvent is SUT.

TEST(_IOC_ConlesEvent_EvtDescQueue, verifyEnqueueSuccessOrTooMany_byEnqueueingUptoMaxQueuingEvtDesc) {
  //===SETUP===
  _IOC_EvtDescQueue_T SUT_EvtDescQueue;
  _IOC_initEvtDescQueue(&SUT_EvtDescQueue);
  ULONG_T MaxQueuingEvtDesc = _CONLES_EVENT_MAX_QUEUING_EVTDESC;

  for (ULONG_T i = 0; i < MaxQueuingEvtDesc; i++) {
    IOC_EvtDesc_T EvtDesc = {
        .MsgDesc =
            {
                .SeqID = i,
            },
        .EvtValue = i,
    };

    //===BEHAVIOR===
    IOC_Result_T Result = _IOC_enqueueEvtDescQueueLast(&SUT_EvtDescQueue, &EvtDesc);
    //===VERIFY===
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);            // KeyVerifyPoint
    ASSERT_EQ(i + 1, SUT_EvtDescQueue.QueuedEvtNum);  // KeyVerifyPoint
    ASSERT_EQ(0, SUT_EvtDescQueue.ProcedEvtNum);      // KeyVerifyPoint
  }

  IOC_EvtDesc_T EvtDesc = {
      .MsgDesc =
          {
              .SeqID = MaxQueuingEvtDesc,
          },
      .EvtValue = MaxQueuingEvtDesc,
  };

  //===BEHAVIOR===
  IOC_Result_T Result = _IOC_enqueueEvtDescQueueLast(&SUT_EvtDescQueue, &EvtDesc);
  //===VERIFY===
  ASSERT_EQ(IOC_RESULT_TOO_MANY_QUEUING_EVTDESC, Result);       // KeyVerifyPoint
  ASSERT_EQ(MaxQueuingEvtDesc, SUT_EvtDescQueue.QueuedEvtNum);  // KeyVerifyPoint
  ASSERT_EQ(0, SUT_EvtDescQueue.ProcedEvtNum);                  // KeyVerifyPoint

  //===EXTRA BEHAVIOR&VERIFY===
  Result = _IOC_isEmptyEvtDescQueue(&SUT_EvtDescQueue);
  ASSERT_EQ(IOC_RESULT_NO, Result);  // KeyVerifyPoint

  //===CLEANUP===
  // deinit WILL fail because not dequeue all enqueued, this is known issue of this SUT.
  // so don't call _IOC_deinitEvtDescQueue(&SUT_EvtDescQueue);
}

TEST(_IOC_ConlesEvent_EvtDescQueue, verifyDequeueSuccessOrEmpty_byDequeueingUptoMaxQueuingEvtDesc) {
  //===SETUP===
  _IOC_EvtDescQueue_T SUT_EvtDescQueue;
  _IOC_initEvtDescQueue(&SUT_EvtDescQueue);
  IOC_EvtDesc_T DequeuedEvtDesc;
  ULONG_T MaxQueuingEvtDesc = _CONLES_EVENT_MAX_QUEUING_EVTDESC;

  for (ULONG_T i = 0; i < MaxQueuingEvtDesc; i++) {
    IOC_EvtDesc_T EvtDesc = {
        .MsgDesc =
            {
                .SeqID = i,
            },
        .EvtValue = i,
    };

    IOC_Result_T Result = _IOC_enqueueEvtDescQueueLast(&SUT_EvtDescQueue, &EvtDesc);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);
  }

  for (ULONG_T i = 0; i < MaxQueuingEvtDesc; i++) {
    //===BEHAVIOR===
    IOC_Result_T Result = _IOC_dequeueEvtDescQueueFirst(&SUT_EvtDescQueue, &DequeuedEvtDesc);
    //===VERIFY===
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);                        // KeyVerifyPoint
    ASSERT_EQ(i, DequeuedEvtDesc.MsgDesc.SeqID);                  // KeyVerifyPoint
    ASSERT_EQ(i + 1, SUT_EvtDescQueue.ProcedEvtNum);              // KeyVerifyPoint
    ASSERT_EQ(MaxQueuingEvtDesc, SUT_EvtDescQueue.QueuedEvtNum);  // KeyVerifyPoint
  }

  //===BEHAVIOR===
  IOC_Result_T Result = _IOC_dequeueEvtDescQueueFirst(&SUT_EvtDescQueue, &DequeuedEvtDesc);
  //===VERIFY===
  ASSERT_EQ(IOC_RESULT_EVTDESC_QUEUE_EMPTY, Result);            // KeyVerifyPoint
  ASSERT_EQ(MaxQueuingEvtDesc, SUT_EvtDescQueue.ProcedEvtNum);  // KeyVerifyPoint
  ASSERT_EQ(MaxQueuingEvtDesc, SUT_EvtDescQueue.QueuedEvtNum);  // KeyVerifyPoint

  //===EXTRA BEHAVIOR&VERIFY===
  Result = _IOC_isEmptyEvtDescQueue(&SUT_EvtDescQueue);
  ASSERT_EQ(IOC_RESULT_YES, Result);  // KeyVerifyPoint

  //===CLEANUP===
  _IOC_deinitEvtDescQueue(&SUT_EvtDescQueue);
}
