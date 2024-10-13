#include "_UT_IOC_Common.h"
//===>RefMore: TEMPLATE OF UT CASE in UT_FreelyDrafts.cxx
//===>RefMore: ConsoleEventTypical UT_ConlesEventTypical.cxx
//===>RefMore: ConsoleEventCapabilty UT_ConlesEventCapabilty.cxx
//===>RefMore: SPECv2 in README_Specification.md

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Summary of UT_ConlesEventASync
 * 1) verifyEachPostEvtCall_LT1ms_bySingleEvtProducerPostSleep9ms99msEvtEvery10ms
 */

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Define a test case to verify SPECv2-z.5 in README.md

/**
 * @[Name]: verifyEachPostEvtCall_LT1ms_bySingleEvtProducerPostSleep9ms99msEvtEvery10ms
 * @[Purpose]: accord SPECv2-z.5 in README.md, use this case to verify postEVT in ASync mode,
 *    by ObjA's call postEVT time COST(<1ms) is much less than ObjB/ObjC's CbProcEvt of each SLEEP 9ms/99ms.
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

// Disable thread satity check use attribute for this test case
__attribute__((no_sanitize("thread")))

static IOC_Result_T
_Case01_CbProcEvt_doSleepByEvtID(IOC_EvtDesc_pT pEvtDesc, void *pCbPriv) {
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
      return IOC_RESULT_BUG;
    }
  }

  return IOC_RESULT_SUCCESS;
}

TEST(UT_ConlesEventASync, verifyEachPostEvtCall_LT1ms_bySingleEvtProducerPostSleep9ms99msEvtEvery10ms) {
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

    do {
      gettimeofday(&StartPost9msTime, NULL);
      Result = IOC_postEVT_inConlesMode(&ObjA_EvtDescTestSleep9ms, NULL);
      gettimeofday(&EndPost9msTime, NULL);
      if (IOC_RESULT_TOO_MANY_QUEUING_EVTDESC == Result) {
        usleep(1);
      } else {
        break;
      }
    } while (0x20240525);

    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // CheckPoint

    uint32_t Post9msCostTime = IOC_deltaTimevalInMS(&StartPost9msTime, &EndPost9msTime);
    ASSERT_LE(Post9msCostTime, 3)  // KeyVerifyPoint
        << "Post9msCostTime = " << Post9msCostTime;

    ObjA_PostedPrivData.TestSleep9msEvtCnt++;

    if (NextEvtID % 10 == 0) {
      // check every postEVT cost time less than 1ms
      struct timeval StartPost99msTick, EndPost99msTick;
      IOC_EvtDesc_T ObjA_EvtDescTestSleep99ms = {.EvtID = IOC_EVTID_TEST_SLEEP_99MS};

      do {
        IOC_Option_defineNonBlock(OptNonBlock);
        gettimeofday(&StartPost99msTick, NULL);
        Result = IOC_postEVT_inConlesMode(&ObjA_EvtDescTestSleep99ms, &OptNonBlock);
        gettimeofday(&EndPost99msTick, NULL);
        if (IOC_RESULT_TOO_MANY_QUEUING_EVTDESC == Result) {
          usleep(1000);
        } else {
          break;
        }
      } while (0x20240525);

      ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // CheckPoint

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
  ASSERT_LE(TotalLoopSleepTime, 1500)  // KeyVerifyPoint
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
