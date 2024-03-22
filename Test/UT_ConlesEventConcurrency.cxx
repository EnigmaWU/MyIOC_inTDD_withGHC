#include <sys/_types/_timeval.h>

#include "_UT_IOC_Common.h"
//===>RefMore: TEMPLATE OF UT CASE in UT_FreelyDrafts.cxx
//===>RefMore: ConsoleEventTypical UT_ConlesEventTypical.cxx
//===>RefMore: ConsoleEventCapabilty UT_ConlesEventCapabilty.cxx
//===>RefMore: SPECv2 in README.md

// Define a test case to verify SPECv2-z.5 in README.md

/**
 * @[Name]: verifyASync_byPostTestSleep9ms99msEvtEvery10msByEvtPrducerInSingleThread
 * @[Purpose]: verify SPECv2-z.5 in README.md
 * @[Steps]:
 *   1) ObjB as EvtCosmer subEVT(TEST_SLEEP_9MS), ObjC as EvtCosmer subEVT(TEST_SLEEP_99MS)
 *   2) ObjA as EvtPrducer postEVT(TEST_SLEEP_9MS) every 10ms and postEVT(TEST_SLEEP_99MS) every 100ms
 *       |-> ObjA in main/single thread, ObjA run in a usleep(10ms) loop in 100 times.
 *   3) ObjA's Posted TestSleep9msEvtCnt is 100 and Posted TestSleep99msEvtCnt is 10,
 *       |-> and ObjA's total sleep time is 100*10ms=1000ms
 *   4) ObjB's CbProced TestSleep9msEvtCnt is 100, ObjC's CbProced TestSleep99msEvtCnt is 10
 * @[Expect]: Step3 and Step4 are all true.
 * @[Notes]:
 *
 */

typedef struct {
  uint32_t TestSleep9msEvtCnt;
  uint32_t TestSleep99msEvtCnt;
} _Case01_CbPrivData_T;

static IOC_Result_T _Case01_CbProcEvt_doSleepByEvtID(IOC_EvtDesc_pT pEvtDesc, void *pCbPriv) {
  _Case01_CbPrivData_T *pCbPrivData = (_Case01_CbPrivData_T *)pCbPriv;

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

TEST(UT_ConlesEventConcurrency, Case01_verifyASync_byPostTestSleep9ms99msEvtEvery10msByEvtPrducerInSingleThread) {
  //===SETUP===
  _Case01_CbPrivData_T ObjB_CbProcedPrivData = {.TestSleep9msEvtCnt = 0, .TestSleep99msEvtCnt = 0};
  IOC_EvtID_T ObjB_SubEvtIDs[] = {IOC_EVTID_TEST_SLEEP_9MS};
  IOC_SubEvtArgs_T ObjB_SubEvtArgs = {
      .CbProcEvt_F = _Case01_CbProcEvt_doSleepByEvtID,
      .pCbPrivData = &ObjB_CbProcedPrivData,
      .EvtNum = IOC_calcArrayElmtCnt(ObjB_SubEvtIDs),
      .pEvtIDs = ObjB_SubEvtIDs,
  };
  IOC_Result_T Result = IOC_subEVT_inConlesMode(&ObjB_SubEvtArgs);
  ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // CheckPoint

  _Case01_CbPrivData_T ObjC_CbPrivData = {.TestSleep9msEvtCnt = 0, .TestSleep99msEvtCnt = 0};
  IOC_EvtID_T ObjC_SubEvtIDs[] = {IOC_EVTID_TEST_SLEEP_99MS};
  IOC_SubEvtArgs_T ObjC_SubEvtArgs = {
      .CbProcEvt_F = _Case01_CbProcEvt_doSleepByEvtID,
      .pCbPrivData = &ObjC_CbPrivData,
      .EvtNum = IOC_calcArrayElmtCnt(ObjC_SubEvtIDs),
      .pEvtIDs = ObjC_SubEvtIDs,
  };
  Result = IOC_subEVT_inConlesMode(&ObjC_SubEvtArgs);
  ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // CheckPoint

  //===BEHAVIOR===
  _Case01_CbPrivData_T ObjA_PostedPrivData = {.TestSleep9msEvtCnt = 0, .TestSleep99msEvtCnt = 0};
  struct timeval StartLoopTime, EndLoopTime;

  gettimeofday(&StartLoopTime, NULL);
  for (uint32_t i = 0; i < 100; i++) {
    // check every postEVT cost time less than 1ms
    struct timeval StartPost9msTime, EndPost9msTime;
    IOC_EvtDesc_T ObjA_EvtDesc = {.EvtID = IOC_EVTID_TEST_SLEEP_9MS};

    gettimeofday(&StartPost9msTime, NULL);
    Result = IOC_postEVT_inConlesMode(&ObjA_EvtDesc, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // CheckPoint
    gettimeofday(&EndPost9msTime, NULL);

    uint32_t Post9msCostTime = IOC_deltaTimevalInMS(&StartPost9msTime, &EndPost9msTime);
    ASSERT_LE(Post9msCostTime, 1)  // KeyVerifyPoint
        << "Post9msCostTime= " << Post9msCostTime;

    ObjA_PostedPrivData.TestSleep9msEvtCnt++;

    if (i % 10 == 0) {
      // check every postEVT cost time less than 1ms
      struct timeval StartPost99msTime, EndPost99msTime;
      IOC_EvtDesc_T ObjA_EvtDesc = {.EvtID = IOC_EVTID_TEST_SLEEP_99MS};

      gettimeofday(&StartPost99msTime, NULL);
      Result = IOC_postEVT_inConlesMode(&ObjA_EvtDesc, NULL);
      ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // CheckPoint
      gettimeofday(&EndPost99msTime, NULL);

      uint32_t Post99msCostTime = IOC_deltaTimevalInMS(&StartPost99msTime, &EndPost99msTime);
      ASSERT_LE(Post99msCostTime, 1)  // KeyVerifyPoint
          << "Post99msCostTime= " << Post99msCostTime;

      ObjA_PostedPrivData.TestSleep99msEvtCnt++;
    }

    usleep(8000);  // 8ms~10ms, not exactly 10ms
  }
  gettimeofday(&EndLoopTime, NULL);

  //===VERIFY===
  ASSERT_EQ(100, ObjA_PostedPrivData.TestSleep9msEvtCnt)  // KeyVerifyPoint
      << "ObjA_PrivData.TestSleep9msEvtCnt= " << ObjA_PostedPrivData.TestSleep9msEvtCnt;
  ASSERT_EQ(10, ObjA_PostedPrivData.TestSleep99msEvtCnt)  // KeyVerifyPoint
      << "ObjA_PrivData.TestSleep99msEvtCnt= " << ObjA_PostedPrivData.TestSleep99msEvtCnt;

  // ObjA's total sleep time is 100*10ms=1000ms
  uint32_t TotalLoopSleepTime = IOC_deltaTimevalInMS(&StartLoopTime, &EndLoopTime);
  ASSERT_LE(TotalLoopSleepTime, 1000)  // KeyVerifyPoint
      << "TotalSleepTime= " << TotalLoopSleepTime;

  ASSERT_EQ(100, ObjB_CbProcedPrivData.TestSleep9msEvtCnt)  // KeyVerifyPoint
      << "ObjB_CbPrivData.TestSleep9msEvtCnt= " << ObjB_CbProcedPrivData.TestSleep9msEvtCnt;
  ASSERT_EQ(10, ObjC_CbPrivData.TestSleep99msEvtCnt)  // KeyVerifyPoint
      << "ObjC_CbPrivData.TestSleep99msEvtCnt= " << ObjC_CbPrivData.TestSleep99msEvtCnt;

  //===CLEANUP===
  IOC_UnsubEvtArgs_T ObjB_UnsubEvtArgs = {.CbProcEvt_F = _Case01_CbProcEvt_doSleepByEvtID, .pCbPriv = &ObjB_CbProcedPrivData};
  Result = IOC_unsubEVT_inConlesMode(&ObjB_UnsubEvtArgs);
  ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // CheckPoint

  IOC_UnsubEvtArgs_T ObjC_UnsubEvtArgs = {.CbProcEvt_F = _Case01_CbProcEvt_doSleepByEvtID, .pCbPriv = &ObjC_CbPrivData};
  Result = IOC_unsubEVT_inConlesMode(&ObjC_UnsubEvtArgs);
  ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // CheckPoint
}