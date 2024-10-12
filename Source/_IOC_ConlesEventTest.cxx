#include <gtest/gtest.h>

#include "_IOC_ConlesEvent.c"  //All the implementation of ConlesEvent is SUT.

TEST(_IOC_ConlesEvent_ClsEvtSuberList, verifySubSuccessOrTooMany_bySubingUptoMaxSuber) {
  //===SETUP===
  _ClsEvtLinkObj_T SUT_ClsEvtLinkObj = {
      .LinkID = IOC_CONLES_MODE_AUTO_LINK_ID,
      .State =
          {
              .Main = IOC_LinkStateReady,
              .Sub  = IOC_LinkSubStateDefault,
          },
  };
  _ClsEvtSuberList_pT pSUT_ClsEvtSuberList = &SUT_ClsEvtLinkObj.EvtSuberList;
  __IOC_ClsEvt_initSuberList(pSUT_ClsEvtSuberList);

  ULONG_T MaxSuber = _CONLES_EVENT_MAX_SUBSCRIBER;

  for (ULONG_T i = 0; i < MaxSuber; i++) {
    IOC_SubEvtArgs_T SubEvtArgs = {
        .CbProcEvt_F = (IOC_CbProcEvt_F)(0x12345678 + i + 1),
        .pCbPrivData = (void *)(0x87654321 - i - 1),
    };

    //===BEHAVIOR===
    IOC_Result_T Result = __IOC_ClsEvt_insertSuberIntoLinkObj(&SUT_ClsEvtLinkObj, &SubEvtArgs);
    //===VERIFY===
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);             // KeyVerifyPoint
    ASSERT_EQ(i + 1, pSUT_ClsEvtSuberList->SuberNum);  // KeyVerifyPoint
  }

  IOC_SubEvtArgs_T SubEvtArgs = {
      .CbProcEvt_F = (IOC_CbProcEvt_F)0x12345678,
      .pCbPrivData = (void *)0x87654321,
  };

  //===BEHAVIOR===
  IOC_Result_T Result = __IOC_ClsEvt_insertSuberIntoLinkObj(&SUT_ClsEvtLinkObj, &SubEvtArgs);
  //===VERIFY===
  ASSERT_EQ(IOC_RESULT_TOO_MANY_EVENT_CONSUMER, Result);  // KeyVerifyPoint
  ASSERT_EQ(MaxSuber, pSUT_ClsEvtSuberList->SuberNum);    // KeyVerifyPoint

  //===CLEANUP===
  // deinit WILL fail because not remove all added, this is known issue of this SUT.
  // so don't call __IOC_ClsEvt_deinitSuberList(&SUT_ClsEvtSuberList);
}

TEST(_IOC_ConlesEvent_ClsEvtSuberList, verifySubConflict_bySubingSameConsumerIdentify) {
  //===SETUP===
  _ClsEvtLinkObj_T SUT_ClsEvtLinkObj = {
      .LinkID = IOC_CONLES_MODE_AUTO_LINK_ID,
      .State =
          {
              .Main = IOC_LinkStateReady,
              .Sub  = IOC_LinkSubStateDefault,
          },
  };
  _ClsEvtSuberList_pT pSUT_ClsEvtSuberList = &SUT_ClsEvtLinkObj.EvtSuberList;
  __IOC_ClsEvt_initSuberList(pSUT_ClsEvtSuberList);

  IOC_SubEvtArgs_T SubEvtArgs = {
      .CbProcEvt_F = (IOC_CbProcEvt_F)0x12345678,
      .pCbPrivData = (void *)0x87654321,
  };

  //===BEHAVIOR===
  IOC_Result_T Result = __IOC_ClsEvt_insertSuberIntoLinkObj(&SUT_ClsEvtLinkObj, &SubEvtArgs);
  //===VERIFY===
  ASSERT_EQ(IOC_RESULT_SUCCESS, Result);       // KeyVerifyPoint
  ASSERT_EQ(1, pSUT_ClsEvtSuberList->SuberNum);  // KeyVerifyPoint

  //===BEHAVIOR===
  Result = __IOC_ClsEvt_insertSuberIntoLinkObj(&SUT_ClsEvtLinkObj, &SubEvtArgs);
  //===VERIFY===
  ASSERT_EQ(IOC_RESULT_CONFLICT_EVENT_CONSUMER, Result);  // KeyVerifyPoint
  ASSERT_EQ(1, pSUT_ClsEvtSuberList->SuberNum);           // KeyVerifyPoint

  //===CLEANUP===
  // deinit WILL fail because not remove all added, this is known issue of this SUT.
  // so don't call __IOC_ClsEvt_deinitSuberList(&SUT_ClsEvtSuberList);
}

TEST(_IOC_ConlesEvent_ClsEvtSuberList, verifyUnsubSuccessOrNot_byUnsubTwice) {
  //===SETUP===
  _ClsEvtLinkObj_T SUT_ClsEvtLinkObj = {
      .LinkID = IOC_CONLES_MODE_AUTO_LINK_ID,
      .State =
          {
              .Main = IOC_LinkStateReady,
              .Sub  = IOC_LinkSubStateDefault,
          },
  };
  _ClsEvtSuberList_pT pSUT_ClsEvtSuberList = &SUT_ClsEvtLinkObj.EvtSuberList;
  __IOC_ClsEvt_initSuberList(pSUT_ClsEvtSuberList);

  IOC_EvtID_T EvtIDs[]        = {IOC_EVTID_TEST_KEEPALIVE, IOC_EVTID_TEST_KEEPALIVE_RELAY};
  IOC_SubEvtArgs_T SubEvtArgs = {
      .CbProcEvt_F = (IOC_CbProcEvt_F)0x12345678,
      .pCbPrivData = (void *)0x87654321,
      .EvtNum      = IOC_calcArrayElmtCnt(EvtIDs),
      .pEvtIDs     = EvtIDs,
  };

  IOC_Result_T Result = __IOC_ClsEvt_insertSuberIntoLinkObj(&SUT_ClsEvtLinkObj, &SubEvtArgs);
  ASSERT_EQ(IOC_RESULT_SUCCESS, Result);
  ASSERT_EQ(1, pSUT_ClsEvtSuberList->SuberNum);

  //===BEHAVIOR===
  IOC_UnsubEvtArgs_T UnsubEvtArgs = {
      .CbProcEvt_F = (IOC_CbProcEvt_F)0x12345678,
      .pCbPrivData = (void *)0x87654321,
  };
  Result = __IOC_ClsEvt_removeSuberFromLinkObj(&SUT_ClsEvtLinkObj, &UnsubEvtArgs);
  //===VERIFY===
  ASSERT_EQ(IOC_RESULT_SUCCESS, Result);       // KeyVerifyPoint
  ASSERT_EQ(0, pSUT_ClsEvtSuberList->SuberNum);  // KeyVerifyPoint

  //===BEHAVIOR===
  Result = __IOC_ClsEvt_removeSuberFromLinkObj(&SUT_ClsEvtLinkObj, &UnsubEvtArgs);
  //===VERIFY===
  ASSERT_EQ(IOC_RESULT_NO_EVENT_CONSUMER, Result);  // KeyVerifyPoint
  ASSERT_EQ(0, pSUT_ClsEvtSuberList->SuberNum);     // KeyVerifyPoint

  //===CLEANUP===
  __IOC_ClsEvt_deinitSuberList(pSUT_ClsEvtSuberList);
}

// ValidAutoLinkID=[IOC_CONLES_MODE_AUTO_LINK_ID_0, IOC_CONLES_MODE_AUTO_LINK_ID_MAX)
TEST(_IOC_postEVT_inConlesMode, verifyAutoLinkID_byInvalidAutoLinkID) {
  IOC_LinkID_T InvalidAutoLinkID = IOC_CONLES_MODE_AUTO_LINK_ID_MAX + 1;
  IOC_Result_T Result            = _IOC_postEVT_inConlesMode(InvalidAutoLinkID, NULL, NULL);
  ASSERT_EQ(IOC_RESULT_INVALID_AUTO_LINK_ID, Result);

  InvalidAutoLinkID = IOC_CONLES_MODE_AUTO_LINK_ID_MAX;
  Result            = _IOC_postEVT_inConlesMode(InvalidAutoLinkID, NULL, NULL);
  ASSERT_EQ(IOC_RESULT_INVALID_AUTO_LINK_ID, Result);

  InvalidAutoLinkID = IOC_CONLES_MODE_AUTO_LINK_ID_MAX - 1;
  Result            = _IOC_postEVT_inConlesMode(InvalidAutoLinkID, NULL, NULL);
  ASSERT_EQ(IOC_RESULT_INVALID_AUTO_LINK_ID, Result);
}