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

TEST(_IOC_ConlesEvent_ClsEvtSuberList, verifySubSuccessOrTooMany_bySubingUptoMaxSuber) {
  //===SETUP===
  _ClsEvtSuberList_T SUT_ClsEvtSuberList;
  __IOC_initClsEvtSuberList(&SUT_ClsEvtSuberList);

  ULONG_T MaxSuber = _CONLES_EVENT_MAX_SUBSCRIBER;

  for (ULONG_T i = 0; i < MaxSuber; i++) {
    IOC_SubEvtArgs_T SubEvtArgs = {
        .CbProcEvt_F = (IOC_CbProcEvt_F)(0x12345678 + i + 1),
        .pCbPrivData = (void *)(0x87654321 - i - 1),
    };

    //===BEHAVIOR===
    IOC_Result_T Result = __IOC_addIntoClsEvtSuberList(&SUT_ClsEvtSuberList, &SubEvtArgs);
    //===VERIFY===
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);           // KeyVerifyPoint
    ASSERT_EQ(i + 1, SUT_ClsEvtSuberList.SuberNum);  // KeyVerifyPoint
  }

  IOC_SubEvtArgs_T SubEvtArgs = {
      .CbProcEvt_F = (IOC_CbProcEvt_F)0x12345678,
      .pCbPrivData = (void *)0x87654321,
  };

  //===BEHAVIOR===
  IOC_Result_T Result = __IOC_addIntoClsEvtSuberList(&SUT_ClsEvtSuberList, &SubEvtArgs);
  //===VERIFY===
  ASSERT_EQ(IOC_RESULT_TOO_MANY_EVENT_CONSUMER, Result);  // KeyVerifyPoint
  ASSERT_EQ(MaxSuber, SUT_ClsEvtSuberList.SuberNum);      // KeyVerifyPoint

  //===CLEANUP===
  // deinit WILL fail because not remove all added, this is known issue of this SUT.
  // so don't call __IOC_deinitClsEvtSuberList(&SUT_ClsEvtSuberList);
}

TEST(_IOC_ConlesEvent_ClsEvtSuberList, verifySubConflict_bySubingSameConsumerIdentify) {
  //===SETUP===
  _ClsEvtSuberList_T SUT_ClsEvtSuberList;
  __IOC_initClsEvtSuberList(&SUT_ClsEvtSuberList);

  IOC_SubEvtArgs_T SubEvtArgs = {
      .CbProcEvt_F = (IOC_CbProcEvt_F)0x12345678,
      .pCbPrivData = (void *)0x87654321,
  };

  //===BEHAVIOR===
  IOC_Result_T Result = __IOC_addIntoClsEvtSuberList(&SUT_ClsEvtSuberList, &SubEvtArgs);
  //===VERIFY===
  ASSERT_EQ(IOC_RESULT_SUCCESS, Result);       // KeyVerifyPoint
  ASSERT_EQ(1, SUT_ClsEvtSuberList.SuberNum);  // KeyVerifyPoint

  //===BEHAVIOR===
  Result = __IOC_addIntoClsEvtSuberList(&SUT_ClsEvtSuberList, &SubEvtArgs);
  //===VERIFY===
  ASSERT_EQ(IOC_RESULT_CONFLICT_EVENT_CONSUMER, Result);  // KeyVerifyPoint
  ASSERT_EQ(1, SUT_ClsEvtSuberList.SuberNum);             // KeyVerifyPoint

  //===CLEANUP===
  // deinit WILL fail because not remove all added, this is known issue of this SUT.
  // so don't call __IOC_deinitClsEvtSuberList(&SUT_ClsEvtSuberList);
}

TEST(_IOC_ConlesEvent_ClsEvtSuberList, verifyUnsubSuccessOrNot_byUnsubTwice) {
  //===SETUP===
  _ClsEvtSuberList_T SUT_ClsEvtSuberList;
  __IOC_initClsEvtSuberList(&SUT_ClsEvtSuberList);

  IOC_EvtID_T EvtIDs[]        = {IOC_EVTID_TEST_KEEPALIVE, IOC_EVTID_TEST_KEEPALIVE_RELAY};
  IOC_SubEvtArgs_T SubEvtArgs = {
      .CbProcEvt_F = (IOC_CbProcEvt_F)0x12345678,
      .pCbPrivData = (void *)0x87654321,
      .EvtNum      = IOC_calcArrayElmtCnt(EvtIDs),
      .pEvtIDs     = EvtIDs,
  };

  IOC_Result_T Result = __IOC_addIntoClsEvtSuberList(&SUT_ClsEvtSuberList, &SubEvtArgs);
  ASSERT_EQ(IOC_RESULT_SUCCESS, Result);
  ASSERT_EQ(1, SUT_ClsEvtSuberList.SuberNum);

  //===BEHAVIOR===
  IOC_UnsubEvtArgs_T UnsubEvtArgs = {
      .CbProcEvt_F = (IOC_CbProcEvt_F)0x12345678,
      .pCbPrivData = (void *)0x87654321,
  };
  Result = __IOC_removeFromClsEvtSuberList(&SUT_ClsEvtSuberList, &UnsubEvtArgs);
  //===VERIFY===
  ASSERT_EQ(IOC_RESULT_SUCCESS, Result);       // KeyVerifyPoint
  ASSERT_EQ(0, SUT_ClsEvtSuberList.SuberNum);  // KeyVerifyPoint

  //===BEHAVIOR===
  Result = __IOC_removeFromClsEvtSuberList(&SUT_ClsEvtSuberList, &UnsubEvtArgs);
  //===VERIFY===
  ASSERT_EQ(IOC_RESULT_NO_EVENT_CONSUMER, Result);  // KeyVerifyPoint
  ASSERT_EQ(0, SUT_ClsEvtSuberList.SuberNum);       // KeyVerifyPoint

  //===CLEANUP===
  __IOC_deinitClsEvtSuberList(&SUT_ClsEvtSuberList);
}