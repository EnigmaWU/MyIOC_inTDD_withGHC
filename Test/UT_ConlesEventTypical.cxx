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
