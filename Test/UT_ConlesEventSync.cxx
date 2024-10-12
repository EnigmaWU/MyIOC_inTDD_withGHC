#include "_UT_IOC_Common.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Summary of UT_ConlesEventConcurrency
 * 1) verifySync_byPostTestSleep99msEvtWithSyncOpt_updateAndCheckSyncFlagValueAfterSleepInCbProcEvt
 */

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
  if (pCbPriv == nullptr || pEvtDesc == nullptr) {
    return IOC_RESULT_BUG;
  }

  _Case02_PrivData_T *pCbPrivData = (_Case02_PrivData_T *)pCbPriv;

  switch (pEvtDesc->EvtID) {
    case IOC_EVTID_TEST_SLEEP_99MS: {
      usleep(99000);
      pCbPrivData->SyncFlagValue = true;
      return IOC_RESULT_SUCCESS;
    }

    default: {
      EXPECT_TRUE(false) << "BUG: unexpected EvtID=" << pEvtDesc->EvtID;
      return IOC_RESULT_BUG;
    }
  }
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