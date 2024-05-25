#include "_UT_IOC_Common.h"
//===>RefMore: TEMPLATE OF UT CASE in UT_FreelyDrafts.cxx
//===>RefMore: ConsoleEventTypical UT_ConlesEventTypical.cxx
//===>RefMore: ConsoleEventCapabilty UT_ConlesEventCapabilty.cxx
//===>RefMore: SPECv2 in README.md

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Summary of UT_ConlesEventConcurrency
 * 1) verifyASync_bySingleEvtProducerPostTestSleep9ms99msEvtEvery10ms_whileProcedAsyncInDifferentEvtConsumerCallback
 * 2) verifySync_byPostTestSleep99msEvtWithSyncOpt_updateAndCheckSyncFlagValueAfterSleepInCbProcEvt
 */

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Define a test case to verify SPECv2-z.5 in README.md

/**
 * @[Name]: verifyASync_bySingleEvtProducerPostTestSleep9ms99msEvtEvery10ms_whileProcedAsyncInDifferentEvtConsumerCallback
 * @[Purpose]: accord SPECv2-z.5 in README.md, use this case to verify postEVT in ASync mode,
 *    by ObjA's call postEVT time cost(<1ms) is much less than ObjB/ObjC's CbProcEvt of each sleep 9ms/99ms.
 * @[Steps]:
 *   1) ObjB as EvtConsumer subEVT(TEST_SLEEP_9MS), ObjC as EvtConsumer subEVT(TEST_SLEEP_99MS)
 *   2) ObjA as EvtPrducer postEVT(TEST_SLEEP_9MS) every 10ms and postEVT(TEST_SLEEP_99MS) every 100ms
 *       |-> ObjA in main/single thread, ObjA run in a usleep(10ms) loop in 100 times.
 *   3) ObjA's Posted TestSleep9msEvtCnt is 100 and Posted TestSleep99msEvtCnt is 10,
 *       |-> and ObjA's total sleep time is 100*10ms=1000ms
 *   4) ObjB's CbProced TestSleep9msEvtCnt is 100, ObjC's CbProced TestSleep99msEvtCnt is 10
 * @[Expect]:
 *    a) Step3 and Step4 are all true.
 *    b) ObjA's postEVT cost time is less than 1ms.
 * @[Notes]:
 *
 */

typedef struct {
  uint32_t TestSleep9msEvtCnt;
  uint32_t TestSleep99msEvtCnt;
} _Case01_PrivData_T;

static IOC_Result_T _Case01_CbProcEvt_doSleepByEvtID(IOC_EvtDesc_pT pEvtDesc, void *pCbPriv) {
  _Case01_PrivData_T *pCbPrivData = (_Case01_PrivData_T *)pCbPriv;

  switch (pEvtDesc->EvtID) {
    case IOC_EVTID_TEST_SLEEP_9MS: {
      pCbPrivData->TestSleep9msEvtCnt++;
      usleep(8000);  // 8ms~10ms, not exactly 9ms
    } break;
    case IOC_EVTID_TEST_SLEEP_99MS: {
      pCbPrivData->TestSleep99msEvtCnt++;
      usleep(98000);  // 98ms~100ms, not exactly 99ms
    } break;
    default: {
      EXPECT_TRUE(false) << "BUG: unexpected EvtID=" << pEvtDesc->EvtID;
    }
      return IOC_RESULT_BUG;
  }

  return IOC_RESULT_SUCCESS;
}

uint32_t IOC_deltaTimevalInMS(const struct timeval *pFromTV, const struct timeval *pToTV) {
  return (pToTV->tv_sec - pFromTV->tv_sec) * 1000 + pToTV->tv_usec / 1000 - pFromTV->tv_usec / 1000;
}

TEST(UT_ConlesEventConcurrency,
     Case01_verifyASync_bySingleEvtProducerPostTestSleep9ms99msEvtEvery10ms_whileProcedAsyncInDifferentEvtConsumerCallback) {
  //===SETUP===
  _Case01_PrivData_T ObjB_CbProcedPrivData = {
      .TestSleep9msEvtCnt  = 0,
      .TestSleep99msEvtCnt = 0,
  };
  IOC_EvtID_T ObjB_SubEvtIDs[] = {
      IOC_EVTID_TEST_SLEEP_9MS,
  };
  IOC_SubEvtArgs_T ObjB_SubEvtArgs = {
      .CbProcEvt_F = _Case01_CbProcEvt_doSleepByEvtID,
      .pCbPrivData = &ObjB_CbProcedPrivData,
      .EvtNum      = IOC_calcArrayElmtCnt(ObjB_SubEvtIDs),
      .pEvtIDs     = ObjB_SubEvtIDs,
  };
  IOC_Result_T Result = IOC_subEVT_inConlesMode(&ObjB_SubEvtArgs);
  ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // CheckPoint

  _Case01_PrivData_T ObjC_CbProcedPrivData = {
      .TestSleep9msEvtCnt  = 0,
      .TestSleep99msEvtCnt = 0,
  };
  IOC_EvtID_T ObjC_SubEvtIDs[] = {
      IOC_EVTID_TEST_SLEEP_99MS,
  };
  IOC_SubEvtArgs_T ObjC_SubEvtArgs = {
      .CbProcEvt_F = _Case01_CbProcEvt_doSleepByEvtID,
      .pCbPrivData = &ObjC_CbProcedPrivData,
      .EvtNum      = IOC_calcArrayElmtCnt(ObjC_SubEvtIDs),
      .pEvtIDs     = ObjC_SubEvtIDs,
  };
  Result = IOC_subEVT_inConlesMode(&ObjC_SubEvtArgs);
  ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // CheckPoint

  //===BEHAVIOR===
  _Case01_PrivData_T ObjA_PostedPrivData = {.TestSleep9msEvtCnt = 0, .TestSleep99msEvtCnt = 0};
  struct timeval StartLoopTime, EndLoopTime;

  gettimeofday(&StartLoopTime, NULL);
  for (uint32_t NextEvtID = 0; NextEvtID < 100; NextEvtID++) {
    // check every postEVT cost time less than 1ms
    struct timeval StartPost9msTime, EndPost9msTime;
    IOC_EvtDesc_T ObjA_EvtDescTestSleep9ms = {.EvtID = IOC_EVTID_TEST_SLEEP_9MS};

    gettimeofday(&StartPost9msTime, NULL);
    Result = IOC_postEVT_inConlesMode(&ObjA_EvtDescTestSleep9ms, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // CheckPoint
    gettimeofday(&EndPost9msTime, NULL);

    uint32_t Post9msCostTime = IOC_deltaTimevalInMS(&StartPost9msTime, &EndPost9msTime);
    ASSERT_LE(Post9msCostTime, 1)  // KeyVerifyPoint
        << "Post9msCostTime = " << Post9msCostTime;

    ObjA_PostedPrivData.TestSleep9msEvtCnt++;

    if (NextEvtID % 10 == 0) {
      // check every postEVT cost time less than 1ms
      struct timeval StartPost99msTick, EndPost99msTick;
      IOC_EvtDesc_T ObjA_EvtDescTestSleep99ms = {.EvtID = IOC_EVTID_TEST_SLEEP_99MS};

      gettimeofday(&StartPost99msTick, NULL);
      Result = IOC_postEVT_inConlesMode(&ObjA_EvtDescTestSleep99ms, NULL);
      ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // CheckPoint
      gettimeofday(&EndPost99msTick, NULL);

      uint32_t Post99msCostTime = IOC_deltaTimevalInMS(&StartPost99msTick, &EndPost99msTick);
      ASSERT_LE(Post99msCostTime, 1)  // KeyVerifyPoint
          << "Post99msCostTime= " << Post99msCostTime;

      ObjA_PostedPrivData.TestSleep99msEvtCnt++;
    }

    usleep(8000);  // 8ms~10ms, not exactly 10ms
  }
  gettimeofday(&EndLoopTime, NULL);

  // sleep(1);
  IOC_forceProcEVT();

  //===VERIFY===
  ASSERT_EQ(100, ObjA_PostedPrivData.TestSleep9msEvtCnt)  // KeyVerifyPoint
      << "ObjA_PrivData.TestSleep9msEvtCnt= " << ObjA_PostedPrivData.TestSleep9msEvtCnt;
  ASSERT_EQ(10, ObjA_PostedPrivData.TestSleep99msEvtCnt)  // KeyVerifyPoint
      << "ObjA_PrivData.TestSleep99msEvtCnt= " << ObjA_PostedPrivData.TestSleep99msEvtCnt;

  // ObjA's total sleep time is 100*10ms=1000ms
  uint32_t TotalLoopSleepTime = IOC_deltaTimevalInMS(&StartLoopTime, &EndLoopTime);
  ASSERT_LE(TotalLoopSleepTime, 1000)  // KeyVerifyPoint
      << "TotalSleepTime= " << TotalLoopSleepTime;

  // ObjB's TestSleep9msEvtCnt is 100
  ASSERT_EQ(100, ObjB_CbProcedPrivData.TestSleep9msEvtCnt)  // KeyVerifyPoint
      << "ObjB_CbPrivData.TestSleep9msEvtCnt= " << ObjB_CbProcedPrivData.TestSleep9msEvtCnt;
  // ObjB's TestSleep99msEvtCnt is 0
  ASSERT_EQ(0, ObjB_CbProcedPrivData.TestSleep99msEvtCnt)  // KeyVerifyPoint
      << "ObjB_CbPrivData.TestSleep99msEvtCnt= " << ObjB_CbProcedPrivData.TestSleep99msEvtCnt;

  // ObjC's TestSleep99msEvtCnt is 10
  ASSERT_EQ(10, ObjC_CbProcedPrivData.TestSleep99msEvtCnt)  // KeyVerifyPoint
      << "ObjC_CbPrivData.TestSleep99msEvtCnt= " << ObjC_CbProcedPrivData.TestSleep99msEvtCnt;
  // ObjC's TestSleep9msEvtCnt is 0
  ASSERT_EQ(0, ObjC_CbProcedPrivData.TestSleep9msEvtCnt)  // KeyVerifyPoint
      << "ObjC_CbPrivData.TestSleep9msEvtCnt= " << ObjC_CbProcedPrivData.TestSleep9msEvtCnt;

  //===CLEANUP===
  IOC_UnsubEvtArgs_T ObjB_UnsubEvtArgs = {
      .CbProcEvt_F = _Case01_CbProcEvt_doSleepByEvtID,
      .pCbPriv     = &ObjB_CbProcedPrivData,
  };
  Result = IOC_unsubEVT_inConlesMode(&ObjB_UnsubEvtArgs);
  ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // CheckPoint

  IOC_UnsubEvtArgs_T ObjC_UnsubEvtArgs = {
      .CbProcEvt_F = _Case01_CbProcEvt_doSleepByEvtID,
      .pCbPriv     = &ObjC_CbProcedPrivData,
  };
  Result = IOC_unsubEVT_inConlesMode(&ObjC_UnsubEvtArgs);
  ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // CheckPoint
}

/**
 * @[Name]: verifySync_byPostTestSleep99msEvtWithSyncOpt_updateAndCheckSyncFlagValueAfterSleepInCbProcEvt
 * @[Purpose]: verify case SPECv2-z.7 in README.md, which is a Sync event case.
 *      Sync here means EvtPrducer postEVT WILL wait for EvtConsumer's CbProcEvt done.
 * @[Steps]:
 *   1) ObjA as EvtConsumer subEVT(TEST_SLEEP_99MS), and ObjA's CbPrivData.SyncFlagValue is FALSE.
 *       |-> ObjA's CbProcEvt do sleep 99ms and update SyncFlagValue to TRUE in its CbPrivData after sleep.
 *   2) ObjB as EvtPrducer postEVT(TEST_SLEEP_99MS) with option(IOC_OPTION_SYNC_EVT)
 *   3) ObjB's postEVT cost time is >= 99ms, and ObjA's CbPrivData.SyncFlagValue is updated.
 * @[Expect]: Step3 is true.
 * @[Notes]:
 */

typedef struct {
  bool SyncFlagValue;
} _Case02_PrivData_T;

static IOC_Result_T _Case02_CbProcEvt_doSleepByEvtID(IOC_EvtDesc_pT pEvtDesc, void *pCbPriv) {
  _Case02_PrivData_T *pCbPrivData = (_Case02_PrivData_T *)pCbPriv;

  switch (pEvtDesc->EvtID) {
    case IOC_EVTID_TEST_SLEEP_99MS: {
      usleep(99000);  // 99ms~100ms, not exactly 99ms
      pCbPrivData->SyncFlagValue = true;
    } break;
    default: {
      EXPECT_TRUE(false) << "BUG: unexpected EvtID=" << pEvtDesc->EvtID;
    }
      return IOC_RESULT_BUG;
  }

  return IOC_RESULT_SUCCESS;
}

TEST(UT_ConlesEventConcurrency,
     Case02_verifySync_byPostTestSleep99msEvtWithSyncOpt_updateAndCheckSyncFlagValueAfterSleepInCbProcEvt) {
  //===SETUP===
  _Case02_PrivData_T ObjA_CbProcedPrivData = {
      .SyncFlagValue = false,
  };
  IOC_EvtID_T ObjA_SubEvtIDs[] = {
      IOC_EVTID_TEST_SLEEP_99MS,
  };
  IOC_SubEvtArgs_T ObjA_SubEvtArgs = {
      .CbProcEvt_F = _Case02_CbProcEvt_doSleepByEvtID,
      .pCbPrivData = &ObjA_CbProcedPrivData,
      .EvtNum      = IOC_calcArrayElmtCnt(ObjA_SubEvtIDs),
      .pEvtIDs     = ObjA_SubEvtIDs,
  };
  IOC_Result_T Result = IOC_subEVT_inConlesMode(&ObjA_SubEvtArgs);
  ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // CheckPoint

  //===BEHAVIOR===
  struct timeval StartPost99msTick, EndPost99msTick;
  IOC_EvtDesc_T ObjB_EvtDescTestSleep99ms = {
      .EvtID = IOC_EVTID_TEST_SLEEP_99MS,
  };
  IOC_Options_T ObjB_Options = {
      .IDs = IOC_OPTID_SYNC_MODE,
  };

  gettimeofday(&StartPost99msTick, NULL);
  Result = IOC_postEVT_inConlesMode(&ObjB_EvtDescTestSleep99ms, &ObjB_Options);
  ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // CheckPoint
  gettimeofday(&EndPost99msTick, NULL);

  //===VERIFY===
  uint32_t Post99msCostTime = IOC_deltaTimevalInMS(&StartPost99msTick, &EndPost99msTick);
  ASSERT_GE(Post99msCostTime, 99)  // KeyVerifyPoint
      << "Post99msCostTime= " << Post99msCostTime;

  ASSERT_TRUE(ObjA_CbProcedPrivData.SyncFlagValue)  // KeyVerifyPoint
      << "ObjA_CbPrivData.SyncFlagValue= " << ObjA_CbProcedPrivData.SyncFlagValue;

  //===CLEANUP===
  IOC_UnsubEvtArgs_T ObjA_UnsubEvtArgs = {
      .CbProcEvt_F = _Case02_CbProcEvt_doSleepByEvtID,
      .pCbPriv     = &ObjA_CbProcedPrivData,
  };
  Result = IOC_unsubEVT_inConlesMode(&ObjA_UnsubEvtArgs);
  ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // CheckPoint
}