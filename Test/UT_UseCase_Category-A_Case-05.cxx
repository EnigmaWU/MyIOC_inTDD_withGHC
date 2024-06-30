// Following UTs are User Acceptance Tests(a.k.a UAT) of Use Case Category-A(a.k.a UseCaseCatA) in file README_UseCase.md

// IOC's API to support UseCaseCatA is defined in IOC_${doXYZ}_inConlesMode style in file IOC.h, such as:
//  'post event' is IOC_postEVT_inConlesMode
//  'subscribe event' is IOC_subEVT_inConlesMode
//  'unsubscribe event' is IOC_unsubEVT_inConlesMode

// Include _UT_IOC_Common.h as it includes all the necessary headers including gtest as UTFWK and IOC headers
#include "_UT_IOC_Common.h"

// ALL UT must use TEMPLATE defined UT_FreelyDrafts.cxx and reference exist UT codes in UT_ConsleEvent*.cxx
//=====================================================================================================================

/**
 * Ref::README_UseCase.md
 *  |-> [Category-A]: post event in same process.
 *      |-> [Case-05]: ObjA|postEVT -> cbProcEvt|ObjB|postEVT -> cbProcEvt|ObjC
 */
// #####################################################################################################################

/**
 * @[Name]: <No1>::verifyPostEvtSuccess_byPostEvtInCbProcEvt
 * @[Purpose]: according to README_UseCase::Category-A::Case-05, use this case to verify postEVT in cbProcEvt.
 * @[Steps]:
 *   1) ObjB as EvtConsumer subEVT(TEST_KEEPALIVE) as SETUP
 *   2) ObjC as EvtConsumer subEVT(TEST_KEEPALIVE_RELAY) as SETUP
 *   3) ObjA as EvtProducer postEVT(TEST_KEEPALIVE) as BEHAVIOR
 *   4) ObjB's CbProcEvt is called once,
 *      |-> in CbProcEvt ObjB postEVT(TEST_KEEPALIVE_RELAY) as BEHAVIOR
 *   5) ObjC's CbProcEvt is called once as VERIFY
 * @[Expect]:
 *   a) Step4 and Step5 are all true.
 * @[Notes]:
 *   a) ObjA may _TotalKeepAliveEvtCnt=1000 in a usleep(1ms) loop to improve the robustness of this case.
 *   b) DuplicatedUT: ConlesEventTypical::verifyPostEvtInCbProcEvt_byObjAPostEvt_andObjBInCbProcEvt_postEvtToObjC
 *          In Chat @workcspace can't find postEVT in cbProcEvt, so I write this one,
 *              then I find the duplication by search EVTID_TEST_KEEPALIVE_RELAY.
 *      KEEP this UT just for a stupid mistake in Chat @workcspace.
 *
 */

typedef struct {
  uint32_t TotalKeepAliveEvtCnt;
} _No1_PrivData_T;

static IOC_Result_T _No1_CbProcEvt_ofObjB(IOC_EvtDesc_pT pEvtDesc, void *pCbPrivData) {
  _No1_PrivData_T *pPrivObjB = (_No1_PrivData_T *)pCbPrivData;

  EXPECT_EQ(pEvtDesc->EvtID, IOC_EVTID_TEST_KEEPALIVE) << "EvtID is not IOC_EVTID_TEST_KEEPALIVE";

  pPrivObjB->TotalKeepAliveEvtCnt++;

  IOC_EvtDesc_T EvtDesc = {0};
  EvtDesc.EvtID         = IOC_EVTID_TEST_KEEPALIVE_RELAY;
  IOC_Result_T Result   = IOC_postEVT_inConlesMode(&EvtDesc, NULL);
  EXPECT_EQ(Result, IOC_RESULT_SUCCESS) << "IOC_postEVT_inConlesMode failed Result=" << IOC_getResultStr(Result);

  return IOC_RESULT_SUCCESS;
}

static IOC_Result_T _No1_CbProcEvt_ofObjC(IOC_EvtDesc_pT pEvtDesc, void *pCbPrivData) {
  _No1_PrivData_T *pPrivObjC = (_No1_PrivData_T *)pCbPrivData;

  EXPECT_EQ(pEvtDesc->EvtID, IOC_EVTID_TEST_KEEPALIVE_RELAY) << "EvtID is not IOC_EVTID_TEST_KEEPALIVE_RELAY";

  pPrivObjC->TotalKeepAliveEvtCnt++;

  return IOC_RESULT_SUCCESS;
}

TEST(ConlesEventConcurrency, verifyPostEvtSuccess_byPostEvtInCbProcEvt) {
  IOC_Result_T Result = IOC_RESULT_BUG;

  //===SETUP ObjB===
  _No1_PrivData_T PrivObjB     = {0};
  IOC_SubEvtArgs_T SubArgsObjB = {0};
  IOC_EvtID_T EvtIDsObjB[]     = {IOC_EVTID_TEST_KEEPALIVE};

  SubArgsObjB.CbProcEvt_F = _No1_CbProcEvt_ofObjB;
  SubArgsObjB.pCbPrivData = &PrivObjB;
  SubArgsObjB.pEvtIDs     = EvtIDsObjB;
  SubArgsObjB.EvtNum      = IOC_calcArrayElmtCnt(EvtIDsObjB);

  Result = IOC_subEVT_inConlesMode(&SubArgsObjB);
  ASSERT_EQ(Result, IOC_RESULT_SUCCESS) << "IOC_subEVT_inConlesMode failed Result=" << IOC_getResultStr(Result);

  //===SETUP ObjC===
  _No1_PrivData_T PrivObjC     = {0};
  IOC_SubEvtArgs_T SubArgsObjC = {0};
  IOC_EvtID_T EvtIDsObjC[]     = {IOC_EVTID_TEST_KEEPALIVE_RELAY};

  SubArgsObjC.CbProcEvt_F = _No1_CbProcEvt_ofObjC;
  SubArgsObjC.pCbPrivData = &PrivObjC;
  SubArgsObjC.pEvtIDs     = EvtIDsObjC;
  SubArgsObjC.EvtNum      = IOC_calcArrayElmtCnt(EvtIDsObjC);

  Result = IOC_subEVT_inConlesMode(&SubArgsObjC);
  ASSERT_EQ(Result, IOC_RESULT_SUCCESS) << "IOC_subEVT_inConlesMode failed Result=" << IOC_getResultStr(Result);

//===BEHAVIOR===
#define _TotalKeepAliveEvtCnt 1000
  IOC_EvtDesc_T EvtDesc = {0};
  EvtDesc.EvtID         = IOC_EVTID_TEST_KEEPALIVE;
  for (uint32_t i = 0; i < _TotalKeepAliveEvtCnt; i++) {
    Result = IOC_postEVT_inConlesMode(&EvtDesc, NULL);
    ASSERT_EQ(Result, IOC_RESULT_SUCCESS) << "IOC_postEVT_inConlesMode<i=" << i
                                          << "> failed Result=" << IOC_getResultStr(Result);

    usleep(1000);  // 1ms
  }

  // force all events to be processed
  IOC_forceProcEVT();

  //===VERIFY===
  EXPECT_EQ(PrivObjB.TotalKeepAliveEvtCnt, _TotalKeepAliveEvtCnt) << "ObjB's TotalKeepAliveEvtCnt is not 1000";
  EXPECT_EQ(PrivObjC.TotalKeepAliveEvtCnt, _TotalKeepAliveEvtCnt)  // KeyVerifyPoint
      << "ObjC's TotalKeepAliveEvtCnt is not 1000";

  // CLEANUP
  IOC_UnsubEvtArgs_T UnsubArgsObjB = {0};

  UnsubArgsObjB.CbProcEvt_F = _No1_CbProcEvt_ofObjB;
  UnsubArgsObjB.pCbPrivData = &PrivObjB;
  Result                    = IOC_unsubEVT_inConlesMode(&UnsubArgsObjB);
  ASSERT_EQ(Result, IOC_RESULT_SUCCESS) << "IOC_unsubEVT_inConlesMode failed Result=" << IOC_getResultStr(Result);

  IOC_UnsubEvtArgs_T UnsubArgsObjC = {0};

  UnsubArgsObjC.CbProcEvt_F = _No1_CbProcEvt_ofObjC;
  UnsubArgsObjC.pCbPrivData = &PrivObjC;
  Result                    = IOC_unsubEVT_inConlesMode(&UnsubArgsObjC);
  ASSERT_EQ(Result, IOC_RESULT_SUCCESS) << "IOC_unsubEVT_inConlesMode failed Result=" << IOC_getResultStr(Result);
}