#include "_UT_IOC_Common.h"

/**
 * @[Name]: verifyPostEvt1v1_byOneObjPostEvtAndAnotherObjCbProcEvt
 * @[Purpose]: accord [SPECv2-c.i] support 1:1 post event in ConlesMode, use this case to verify the 1:1 behavior.
 * @[Steps]:
 *   1. ObjA call subEVT(TEST_KEEPALIVE) with _Case01_CbProcEvt1v1.
 *   2. ObjB call postEVT(TEST_KEEPALIVE) with $_Case01_KeepAliveEvtCnt times.
 *   3. ObjA check the _Case01_CbProcEvt_1v1 is callbacked $_Case01_KeepAliveEvtCnt times.
 *        |-> Use _Case01_CbPrivData.KeepAliveEvtCnt to count the times in _Case01_CbProcEvt1v1.
 * @[Expect]: ObjA's _Case01_CbProcEvt_1v1 is callbacked $_Case01_KeepAliveEvtCnt times.
 * @[Notes]:
 */

typedef struct {
  uint32_t KeepAliveEvtCnt;
} _Case01_CbPrivData_T;

static IOC_Result_T _Case01_CbProcEvt_1v1(IOC_EvtDesc_pT pEvtDesc, void *pCbPriv) {
  _Case01_CbPrivData_T *pCbPrivData = (_Case01_CbPrivData_T *)pCbPriv;

  switch (pEvtDesc->EvtID) {
    case IOC_EVTID_TEST_KEEPALIVE: {
      pCbPrivData->KeepAliveEvtCnt++;
    } break;
    default: {
      EXPECT_TRUE(false) << "BUG: unexpected EvtID=" << pEvtDesc->EvtID;
    }
      return IOC_RESULT_BUG;
  }

  return IOC_RESULT_SUCCESS;
}

TEST(UT_ConlesEventTypical, Case01_verifyPostEvt1v1_byOneObjPostEvtAndAnotherObjCbProcEvt) {
  //===SETUP===
  _Case01_CbPrivData_T ObjA_CbPrivData = {.KeepAliveEvtCnt = 0};
  IOC_EvtID_T ObjA_SubEvtIDs[] = {IOC_EVTID_TEST_KEEPALIVE};
  IOC_SubEvtArgs_T ObjA_SubEvtArgs = {
      .CbProcEvt_F = _Case01_CbProcEvt_1v1,
      .pCbPrivData = &ObjA_CbPrivData,
      .EvtNum = IOC_calcArrayElmtCnt(ObjA_SubEvtIDs),
      .pEvtIDs = ObjA_SubEvtIDs,
  };
  IOC_Result_T Result = IOC_subEVT_inConlesMode(&ObjA_SubEvtArgs);
  ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // CheckPoint

#define _Case01_KeepAliveEvtCnt 1024
  //===BEHAVIOR===
  for (uint32_t i = 0; i < _Case01_KeepAliveEvtCnt; i++) {
    IOC_EvtDesc_T ObjB_EvtDesc = {.EvtID = IOC_EVTID_TEST_KEEPALIVE};
    Result = IOC_postEVT_inConlesMode(&ObjB_EvtDesc, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // CheckPoint
  }

  //===VERIFY===
  ASSERT_EQ(_Case01_KeepAliveEvtCnt, ObjA_CbPrivData.KeepAliveEvtCnt);  // KeyVerifyPoint

  //===CLEANUP===
  IOC_UnsubEvtArgs_T ObjA_UnsubEvtArgs = {.CbProcEvt_F = _Case01_CbProcEvt_1v1, .pCbPriv = &ObjA_CbPrivData};
  Result = IOC_unsubEVT_inConlesMode(&ObjA_UnsubEvtArgs);
  ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // CheckPoint
}

/**
 * @[Name]: verifyPostEvt1vN_byOneObjPostEvt_R1TwoObjCbProcEvt_R2ThreeMoreObjCbProcEvt
 * @[Purpose]: accord [SPECv2-c.i] support 1:N post event in ConlesMode, use this case to verify the 1:N behavior.
 * @[Steps]:
 *   1. ObjB/C call subEVT(TEST_KEEPALIVE) with _Case02_CbProcEvt1vN.
 *   2. ObjA call postEVT(TEST_KEEPALIVE) with $_Case02_KeepAliveEvtCntR1 times.
 *   3. ObjD/E/F subEVT(TEST_KEEPALIVE) with _Case02_CbProcEvt1vN.
 *   4. ObjA call postEVT(TEST_KEEPALIVE) with $_Case02_KeepAliveEvtCntR2 times.
 * @[Expect]: ObjB/C's _Case02_CbProcEvt1vN is callbacked $_Case02_KeepAliveEvtCntR1+$_Case02_KeepAliveEvtCntR2 times.
 *        ObjD/E/F's _Case02_CbProcEvt1vN is callbacked $_Case02_KeepAliveEvtCntR2 times.
 * @[Notes]:
 *      1. R1 means Round 1, R2 means Round 2.
 *      2. ObjB/C/D/E/F are different objects.
 *  @[RefCode]: TEST(UT_ConlesEventTypical, Case01_verifyPostEvt1v1_byOneObjPostEvtAndAnotherObjCbProcEvt)
 */

typedef struct {
  uint32_t KeepAliveEvtCnt;
} _Case02_CbPrivData_T;

static IOC_Result_T _Case02_CbProcEvt_1vN(IOC_EvtDesc_pT pEvtDesc, void *pCbPriv) {
  _Case02_CbPrivData_T *pCbPrivData = (_Case02_CbPrivData_T *)pCbPriv;

  switch (pEvtDesc->EvtID) {
    case IOC_EVTID_TEST_KEEPALIVE: {
      pCbPrivData->KeepAliveEvtCnt++;
    } break;
    default: {
      EXPECT_TRUE(false) << "BUG: unexpected EvtID=" << pEvtDesc->EvtID;
    }
      return IOC_RESULT_BUG;
  }

  return IOC_RESULT_SUCCESS;
}

TEST(UT_ConlesEventTypical, Case02_verifyPostEvt1vN_byOneObjPostEvt_R1TwoObjCbProcEvt_R2ThreeMoreObjCbProcEvt) {
  //===SETUP===
  _Case02_CbPrivData_T ObjB_CbPrivData = {.KeepAliveEvtCnt = 0};
  IOC_EvtID_T ObjB_SubEvtIDs[] = {IOC_EVTID_TEST_KEEPALIVE};
  IOC_SubEvtArgs_T ObjBC_SubEvtArgs = {
      .CbProcEvt_F = _Case02_CbProcEvt_1vN,
      .pCbPrivData = &ObjB_CbPrivData,
      .EvtNum = IOC_calcArrayElmtCnt(ObjB_SubEvtIDs),
      .pEvtIDs = ObjB_SubEvtIDs,
  };
  IOC_Result_T Result = IOC_subEVT_inConlesMode(&ObjBC_SubEvtArgs);
  ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // CheckPoint

  _Case02_CbPrivData_T ObjC_CbPrivData = {.KeepAliveEvtCnt = 0};
  IOC_EvtID_T ObjC_SubEvtIDs[] = {IOC_EVTID_TEST_KEEPALIVE};
  IOC_SubEvtArgs_T ObjCD_SubEvtArgs = {
      .CbProcEvt_F = _Case02_CbProcEvt_1vN,
      .pCbPrivData = &ObjC_CbPrivData,
      .EvtNum = IOC_calcArrayElmtCnt(ObjC_SubEvtIDs),
      .pEvtIDs = ObjC_SubEvtIDs,
  };

  Result = IOC_subEVT_inConlesMode(&ObjCD_SubEvtArgs);
  ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // CheckPoint

#define _Case02_KeepAliveEvtCntR1 1024
  //===BEHAVIOR===
  for (uint32_t i = 0; i < _Case02_KeepAliveEvtCntR1; i++) {
    IOC_EvtDesc_T ObjA_EvtDesc = {.EvtID = IOC_EVTID_TEST_KEEPALIVE};
    Result = IOC_postEVT_inConlesMode(&ObjA_EvtDesc, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // CheckPoint
  }

  //===SETUP===
  _Case02_CbPrivData_T ObjD_CbPrivData = {.KeepAliveEvtCnt = 0};
  IOC_EvtID_T ObjD_SubEvtIDs[] = {IOC_EVTID_TEST_KEEPALIVE};
  IOC_SubEvtArgs_T ObjDE_SubEvtArgs = {
      .CbProcEvt_F = _Case02_CbProcEvt_1vN,
      .pCbPrivData = &ObjD_CbPrivData,
      .EvtNum = IOC_calcArrayElmtCnt(ObjD_SubEvtIDs),
      .pEvtIDs = ObjD_SubEvtIDs,
  };

  Result = IOC_subEVT_inConlesMode(&ObjDE_SubEvtArgs);
  ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // CheckPoint

  _Case02_CbPrivData_T ObjE_CbPrivData = {.KeepAliveEvtCnt = 0};
  IOC_EvtID_T ObjE_SubEvtIDs[] = {IOC_EVTID_TEST_KEEPALIVE};
  IOC_SubEvtArgs_T ObjEF_SubEvtArgs = {
      .CbProcEvt_F = _Case02_CbProcEvt_1vN,
      .pCbPrivData = &ObjE_CbPrivData,
      .EvtNum = IOC_calcArrayElmtCnt(ObjE_SubEvtIDs),
      .pEvtIDs = ObjE_SubEvtIDs,
  };

  Result = IOC_subEVT_inConlesMode(&ObjEF_SubEvtArgs);
  ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // CheckPoint

  _Case02_CbPrivData_T ObjF_CbPrivData = {.KeepAliveEvtCnt = 0};
  IOC_EvtID_T ObjF_SubEvtIDs[] = {IOC_EVTID_TEST_KEEPALIVE};
  IOC_SubEvtArgs_T ObjFG_SubEvtArgs = {
      .CbProcEvt_F = _Case02_CbProcEvt_1vN,
      .pCbPrivData = &ObjF_CbPrivData,
      .EvtNum = IOC_calcArrayElmtCnt(ObjF_SubEvtIDs),
      .pEvtIDs = ObjF_SubEvtIDs,
  };

  Result = IOC_subEVT_inConlesMode(&ObjFG_SubEvtArgs);
  ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // CheckPoint

#define _Case02_KeepAliveEvtCntR2 2048
  //===BEHAVIOR===
  for (uint32_t i = 0; i < _Case02_KeepAliveEvtCntR2; i++) {
    IOC_EvtDesc_T ObjA_EvtDesc = {.EvtID = IOC_EVTID_TEST_KEEPALIVE};
    Result = IOC_postEVT_inConlesMode(&ObjA_EvtDesc, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // CheckPoint
  }

  //===VERIFY===
  ASSERT_EQ(_Case02_KeepAliveEvtCntR1 + _Case02_KeepAliveEvtCntR2, ObjB_CbPrivData.KeepAliveEvtCnt);  // KeyVerifyPoint
  ASSERT_EQ(_Case02_KeepAliveEvtCntR1 + _Case02_KeepAliveEvtCntR2, ObjC_CbPrivData.KeepAliveEvtCnt);  // KeyVerifyPoint
  ASSERT_EQ(_Case02_KeepAliveEvtCntR2, ObjD_CbPrivData.KeepAliveEvtCnt);                              // KeyVerifyPoint
  ASSERT_EQ(_Case02_KeepAliveEvtCntR2, ObjE_CbPrivData.KeepAliveEvtCnt);                              // KeyVerifyPoint
  ASSERT_EQ(_Case02_KeepAliveEvtCntR2, ObjF_CbPrivData.KeepAliveEvtCnt);                              // KeyVerifyPoint

  //===CLEANUP===
  IOC_UnsubEvtArgs_T ObjB_UnsubEvtArgs = {.CbProcEvt_F = _Case02_CbProcEvt_1vN, .pCbPriv = &ObjB_CbPrivData};
  Result = IOC_unsubEVT_inConlesMode(&ObjB_UnsubEvtArgs);
  ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // CheckPoint

  IOC_UnsubEvtArgs_T ObjC_UnsubEvtArgs = {.CbProcEvt_F = _Case02_CbProcEvt_1vN, .pCbPriv = &ObjC_CbPrivData};
  Result = IOC_unsubEVT_inConlesMode(&ObjC_UnsubEvtArgs);
  ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // CheckPoint

  IOC_UnsubEvtArgs_T ObjD_UnsubEvtArgs = {.CbProcEvt_F = _Case02_CbProcEvt_1vN, .pCbPriv = &ObjD_CbPrivData};
  Result = IOC_unsubEVT_inConlesMode(&ObjD_UnsubEvtArgs);
  ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // CheckPoint

  IOC_UnsubEvtArgs_T ObjE_UnsubEvtArgs = {.CbProcEvt_F = _Case02_CbProcEvt_1vN, .pCbPriv = &ObjE_CbPrivData};
  Result = IOC_unsubEVT_inConlesMode(&ObjE_UnsubEvtArgs);
  ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // CheckPoint

  IOC_UnsubEvtArgs_T ObjF_UnsubEvtArgs = {.CbProcEvt_F = _Case02_CbProcEvt_1vN, .pCbPriv = &ObjF_CbPrivData};
  Result = IOC_unsubEVT_inConlesMode(&ObjF_UnsubEvtArgs);
  ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // CheckPoint
}