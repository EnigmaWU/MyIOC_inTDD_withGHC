#include "_UT_IOC_Common.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//===TEMPLATE OF UT CASE===
/**
 * @[Name]: ${verifyBehivorX_byDoABC}
 * @[Purpose]: ${what to verify}
 * @[Steps]: ${how to do}
 *   1. ...
 * @[Expect]: ${how to verify}
 * @[Notes]:
 */
TEST(UT_NameOfCategory, CaseNN_verifyBehivorX_byDoABC) {
  //===SETUP===
  // 1. ...

  //===BEHAVIOR===
  //@VerifyPoint xN(each case MAY have many 'ASSERT_XYZ' check points)

  //===VERIFY===
  //@KeyVerifyPoint<=3(each case SHOULD has less than 3 key 'ASSERT_XYZ' verify points)

  //===CLEANUP===
}

//---------------------------------------------------------------------------------------------------------------------
class UT_NameofCategoryFixture : public ::testing::Test {
 protected:
  void SetUp() override {
    // 1. ...
  }

  void TearDown() override {
    // 1. ...
  }
};

TEST_F(UT_NameofCategoryFixture, CaseNN_verifyBehivorX_byDoABC) {
  //===BEHAVIOR===
  //@VerifyPoint xN(each case MAY have many 'ASSERT_XYZ' check points)

  //===VERIFY===
  //@KeyVerifyPoint<=3(each case SHOULD has less than 3 key 'ASSERT_XYZ' verify points)
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// TODO(@W): Freely start a new UT first from here, and then refine it later, just keep smooth mind and low resistance.

/**
 * @[Name]: verifyPostEvt1v1_byOneObjPostEvtAndAnotherObjCbProcEvt
 * @[Purpose]: accord SPECv2-c) support 1:1 post event in ConlesMode, use this case to verify the 1:1 behavior.
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

TEST(UT_ConlesEvtTypical, Case01_verifyPostEvt1v1_byOneObjPostEvtAndAnotherObjCbProcEvt) {
  //===SETUP===
  _Case01_CbPrivData_T ObjA_CbPrivData = {.KeepAliveEvtCnt = 0};
  IOC_EvtID_T ObjA_SubEvtIDs[] = {IOC_EVTID_TEST_KEEPALIVE};
  IOC_SubEvtArgs_T ObjA_SubEvtArgs = {.CbProcEvt_F = _Case01_CbProcEvt_1v1,
                                      .pCbPriv = &ObjA_CbPrivData,
                                      .pEvtIDs = ObjA_SubEvtIDs,
                                      .EvtNum = IOC_calcArrayElmtCnt(ObjA_SubEvtIDs)};
  IOC_Result_T Result = IOC_subEVT_inConlesMode(&ObjA_SubEvtArgs);
  ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // CheckPoint

//===BEHAVIOR===
#define _Case01_KeepAliveEvtCnt 1024
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