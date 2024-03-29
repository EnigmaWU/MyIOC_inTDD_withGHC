#include <_types/_uint16_t.h>

#include <thread>

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
 *   3. ObjC call unsubEVT(TEST_KEEPALIVE).
 *   4. ObjD/E/F subEVT(TEST_KEEPALIVE) with _Case02_CbProcEvt1vN.
 *   5. ObjA call postEVT(TEST_KEEPALIVE) with $_Case02_KeepAliveEvtCntR2 times.
 * @[Expect]: ObjB's _Case02_CbProcEvt1vN is callbacked $_Case02_KeepAliveEvtCntR1+$_Case02_KeepAliveEvtCntR2 times.
 *        ObjC's _Case02_CbProcEvt1vN is callbacked $_Case02_KeepAliveEvtCntR1 times.
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

  //===CLEANUP===
  IOC_UnsubEvtArgs_T ObjC_UnsubEvtArgs = {.CbProcEvt_F = _Case02_CbProcEvt_1vN, .pCbPriv = &ObjC_CbPrivData};
  Result = IOC_unsubEVT_inConlesMode(&ObjC_UnsubEvtArgs);
  ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // CheckPoint

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
  ASSERT_EQ(_Case02_KeepAliveEvtCntR1, ObjC_CbPrivData.KeepAliveEvtCnt);                              // KeyVerifyPoint
  ASSERT_EQ(_Case02_KeepAliveEvtCntR2, ObjD_CbPrivData.KeepAliveEvtCnt);                              // KeyVerifyPoint
  ASSERT_EQ(_Case02_KeepAliveEvtCntR2, ObjE_CbPrivData.KeepAliveEvtCnt);                              // KeyVerifyPoint
  ASSERT_EQ(_Case02_KeepAliveEvtCntR2, ObjF_CbPrivData.KeepAliveEvtCnt);                              // KeyVerifyPoint

  //===CLEANUP===
  IOC_UnsubEvtArgs_T ObjB_UnsubEvtArgs = {.CbProcEvt_F = _Case02_CbProcEvt_1vN, .pCbPriv = &ObjB_CbPrivData};
  Result = IOC_unsubEVT_inConlesMode(&ObjB_UnsubEvtArgs);
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

// Design a new case to replace UT_ConlesEventTypical, Case02_verifyPostEvt1vN
//   use getCapabilty to get the max EvtCosmer number assign to N, then use the N to verify the 1:N behavior.
//   and each EvtCosmer will callbacked step-by-step x1024 KeepAliveCnt times.

/**
 * @[Name]: verifyPostEvt1vN_byOneObjPostEvt_Min2MaxEvtCosmerCbProcEvt
 * @[Purpose]: accord [SPECv2-c.i] support 1:N post event in ConlesMode, use this case to verify the 1:N behavior.
 * @[Steps]:
 *   1. Get the max EvtCosmer number by IOC_getCapabilty(CAPID=CONLES_MODE_EVENT).
 *   2. First for each EvtCosmer loop:
 *        |-> call subEVT(TEST_KEEPALIVE) with _Case03_CbProcEvt1vN.
 *        |-> then call postEVT(TEST_KEEPALIVE) with $_Case03_KeepAliveEvtCnt times.
 *   3. Second for each EvtCosmer loop:
 *        |-> call postEVT(TEST_KEEPALIVE) with $_Case03_KeepAliveEvtCnt times.
 *        |-> then call unsubEVT(TEST_KEEPALIVE).
 * @[Expect]: each EvtCosmer's _Case03_CbProcEvt1vN is callbacked different times,
 *        the first EvtCosmer is callbacked $_Case03_KeepAliveEvtCnt * 2 * MaxEvtCosmer times,
 *        the last EvtCosmer is callbacked $_Case03_KeepAliveEvtCnt * 2 times.
 * @[Notes]:
 */

typedef struct {
  uint32_t KeepAliveEvtCnt;
} _Case03_CbPrivData_T;

static IOC_CbProcEvt_F _Case03_CbProcEvt1vN = _Case02_CbProcEvt_1vN;

TEST(UT_ConlesEventTypical, Case03_verifyPostEvt1vN_byOneObjPostEvt_Min2MaxEvtCosmerCbProcEvt) {
  //===SETUP===
  IOC_CapabiltyDescription_T CapDesc = {.CapID = IOC_CAPID_CONLES_MODE_EVENT};
  IOC_Result_T Result = IOC_getCapabilty(&CapDesc);
  ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // CheckPoint

  uint16_t MaxEvtCosmerNum = CapDesc.ConlesModeEvent.MaxEvtCosmer;

  _Case03_CbPrivData_T *pObjS_CbPrivData = (_Case03_CbPrivData_T *)malloc(MaxEvtCosmerNum * sizeof(_Case03_CbPrivData_T));
  ASSERT_NE(nullptr, pObjS_CbPrivData);  // CheckPoint

#define _Case03_KeepAliveEvtCnt 1024
  //===BEHAVIOR===
  // subEVT one-by-one incresely, then postEVT round-by-round.
  for (uint16_t i = 0; i < MaxEvtCosmerNum; i++) {
    pObjS_CbPrivData[i].KeepAliveEvtCnt = 0;
    IOC_EvtID_T ObjS_SubEvtIDs[] = {IOC_EVTID_TEST_KEEPALIVE};
    IOC_SubEvtArgs_T ObjS_SubEvtArgs = {
        .CbProcEvt_F = _Case03_CbProcEvt1vN,
        .pCbPrivData = &pObjS_CbPrivData[i],
        .EvtNum = IOC_calcArrayElmtCnt(ObjS_SubEvtIDs),
        .pEvtIDs = ObjS_SubEvtIDs,
    };
    Result = IOC_subEVT_inConlesMode(&ObjS_SubEvtArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // CheckPoint

    for (uint32_t j = 0; j < _Case03_KeepAliveEvtCnt; j++) {
      IOC_EvtDesc_T ObjA_EvtDesc = {.EvtID = IOC_EVTID_TEST_KEEPALIVE};
      Result = IOC_postEVT_inConlesMode(&ObjA_EvtDesc, NULL);
      ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // CheckPoint
    }
  }

  // postEVT round-by-round, then unsubEVT one-by-one reversely.
  for (uint16_t i = 0; i < MaxEvtCosmerNum; i++) {
    for (uint32_t j = 0; j < _Case03_KeepAliveEvtCnt; j++) {
      IOC_EvtDesc_T ObjA_EvtDesc = {.EvtID = IOC_EVTID_TEST_KEEPALIVE};
      Result = IOC_postEVT_inConlesMode(&ObjA_EvtDesc, NULL);
      ASSERT_EQ(IOC_RESULT_SUCCESS, Result) << "i= " << i << " j=" << j;  // CheckPoint
    }

    IOC_UnsubEvtArgs_T ObjS_UnsubEvtArgs = {.CbProcEvt_F = _Case03_CbProcEvt1vN,
                                            .pCbPriv = &pObjS_CbPrivData[MaxEvtCosmerNum - 1 - i] /*reverse*/};
    Result = IOC_unsubEVT_inConlesMode(&ObjS_UnsubEvtArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // CheckPoint
  }

  //===VERIFY===
  /**
   * @brief askCopilotChat why assert '_Case03_KeepAliveEvtCnt * 2 * (MaxEvtCosmerNum - i)'
    The multiplication by 2 could be due to the way the "keep alive" events are generated or counted in the system under test.
    For example, each event might be counted twice for some reason, or each event might generate two sub-events.

    The multiplication by (MaxEvtCosmerNum - i) suggests that the expected number of "keep alive" events decreases linearly for
   each event consumer as i increases. This could be due to the way the events are distributed among the consumers, or it could
   be a property of the specific test case.
   **/
  for (uint16_t i = 0; i < MaxEvtCosmerNum; i++) {
    ASSERT_EQ(_Case03_KeepAliveEvtCnt * 2 * (MaxEvtCosmerNum - i), pObjS_CbPrivData[i].KeepAliveEvtCnt)
        << "MaxEvtCosmrNum= " << MaxEvtCosmerNum << " i=" << i;  // KeyVerifyPoint
  }

  //===CLEANUP===
  free(pObjS_CbPrivData);
}

// Design a test case to verify SPECv2-c.i N:1 post event in ConlesMode.
// Define one EvtCosmer to subEVT(EVTID=TEST_KEEPALIVE) with _Case04_CbProcEvtNv1.
//  and lastly expect the EvtCosmer will be callbacked N * $_Case04_KeepAliveEvtCnt times.
// Define $_Case04_EvtPrduerNum as N,
//  then use the foreach N EvtPrduer in a thread to postEVT(EVTID=TEST_KEEPALIVE) _Case04_KeepAliveEvtCnt times.

/**
 * @[Name]: verifyPostEvtNv1_byNxEvtPrduerPostEvtAnd1xEvtCosmerCbProcEvt
 * @[Purpose]: accord [SPECv2-c.i] support N:1 post event in ConlesMode, use this case to verify the N:1 behavior.
 * @[Steps]:
 *   1. ObjA call subEVT(TEST_KEEPALIVE) with _Case04_CbProcEvtNv1.
 *   2. Define $_Case04_EvtPrduerNum as N, create N threads to postEVT(TEST_KEEPALIVE) with $_Case04_KeepAliveEvtCnt times.
 *   3. ObjA check the _Case04_CbProcEvtNv1 is callbacked $_Case04_EvtPrduerNum * $_Case04_KeepAliveEvtCnt times.
 * @[Expect]: Step 3 is passed.
 * @[Notes]:
 */

typedef _Case02_CbPrivData_T _Case04_CbPrivData_T;

static IOC_Result_T _Case04_CbProcEvt_Nv1(IOC_EvtDesc_pT pEvtDesc, void *pCbPriv) {
  return _Case02_CbProcEvt_1vN(pEvtDesc, pCbPriv);
}

TEST(UT_ConlesEventTypical, Case04_verifyPostEvtNv1_byNxEvtPrduerPostEvtAnd1xEvtCosmerCbProcEvt) {
  //===SETUP===
  _Case04_CbPrivData_T ObjA_CbPrivData = {.KeepAliveEvtCnt = 0};
  IOC_EvtID_T ObjA_SubEvtIDs[] = {IOC_EVTID_TEST_KEEPALIVE};
  IOC_SubEvtArgs_T ObjA_SubEvtArgs = {
      .CbProcEvt_F = _Case04_CbProcEvt_Nv1,
      .pCbPrivData = &ObjA_CbPrivData,
      .EvtNum = IOC_calcArrayElmtCnt(ObjA_SubEvtIDs),
      .pEvtIDs = ObjA_SubEvtIDs,
  };
  IOC_Result_T Result = IOC_subEVT_inConlesMode(&ObjA_SubEvtArgs);
  ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // CheckPoint

#define _Case04_EvtPrduerNum 8
#define _Case04_KeepAliveEvtCnt _Case03_KeepAliveEvtCnt

  std::thread EvtPrduerThreads[_Case04_EvtPrduerNum];
  for (uint32_t i = 0; i < _Case04_EvtPrduerNum; i++) {
    EvtPrduerThreads[i] = std::thread([i]() {
      for (uint32_t j = 0; j < _Case04_KeepAliveEvtCnt; j++) {
        IOC_EvtDesc_T ObjB_EvtDesc = {.EvtID = IOC_EVTID_TEST_KEEPALIVE};
        IOC_Result_T Result = IOC_postEVT_inConlesMode(&ObjB_EvtDesc, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // CheckPoint
      }
    });
  }

  for (uint32_t i = 0; i < _Case04_EvtPrduerNum; i++) {
    EvtPrduerThreads[i].join();
  }

  //===VERIFY===
  ASSERT_EQ(_Case04_KeepAliveEvtCnt * _Case04_EvtPrduerNum, ObjA_CbPrivData.KeepAliveEvtCnt);  // KeyVerifyPoint

  //===CLEANUP===
  IOC_UnsubEvtArgs_T ObjA_UnsubEvtArgs = {.CbProcEvt_F = _Case04_CbProcEvt_Nv1, .pCbPriv = &ObjA_CbPrivData};
  Result = IOC_unsubEVT_inConlesMode(&ObjA_UnsubEvtArgs);
  ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // CheckPoint
}

// Design a test case to verify SPECv2-c.i N:M postEVT and subEVT/cbEVT in ConlesMode.
// Define $_Case05_EvtPrduerNum as N, $_Case05_EvtCosmerNum as M.
//  then use the foreach N EvtPrduer in a thread to postEVT(EVTID=TEST_KEEPALIVE) _Case05_KeepAliveEvtCnt times.
//  and use the foreach M EvtCosmer to subEVT(EVTID=TEST_KEEPALIVE) with _Case05_CbProcEvtNvM.
//  and lastly expect each EvtCosmer will be callbacked N * $_Case05_KeepAliveEvtCnt times.

/**
 * @[Name]: verifyPostEvtNvM_byNxEvtPrduerPostEvtAndMxEvtCosmerCbProcEvt
 * @[Purpose]: accord [SPECv2-c.i] support N:M post event in ConlesMode, use this case to verify the N:M behavior.
 * @[Steps]:
 *   1. Define $_Case05_EvtPrduerNum as N, $_Case05_EvtCosmerNum as M.
 *   2. Create M EvtCosmer to subEVT(TEST_KEEPALIVE) with _Case05_CbProcEvtNvM.
 *   3. Create N threads to postEVT(TEST_KEEPALIVE) with $_Case05_KeepAliveEvtCnt times.
 *   4. Check each EvtCosmer is callbacked N * $_Case05_KeepAliveEvtCnt times.
 * @[Expect]: Step 4 is passed.
 * @[Notes]:
 */

typedef struct {
  uint32_t KeepAliveEvtCnt;
} _Case05_CbPrivData_T;

static IOC_Result_T _Case05_CbProcEvt_NvM(IOC_EvtDesc_pT pEvtDesc, void *pCbPriv) {
  _Case05_CbPrivData_T *pCbPrivData = (_Case05_CbPrivData_T *)pCbPriv;

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

TEST(UT_ConlesEventTypical, Case05_verifyPostEvtNvM_byNxEvtPrduerPostEvtAndMxEvtCosmerCbProcEvt) {
  //===SETUP===
  IOC_CapabiltyDescription_T CapDesc = {.CapID = IOC_CAPID_CONLES_MODE_EVENT};
  IOC_Result_T Result = IOC_getCapabilty(&CapDesc);
  ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // CheckPoint

  uint16_t MaxEvtCosmerNum = CapDesc.ConlesModeEvent.MaxEvtCosmer;

#define _Case05_EvtPrduerNum 8
#define _Case05_KeepAliveEvtCnt _Case03_KeepAliveEvtCnt

  // std::thread EvtCosmerThreads[MaxEvtCosmerNum];
  _Case05_CbPrivData_T *pObjS_CbPrivData = (_Case05_CbPrivData_T *)malloc(MaxEvtCosmerNum * sizeof(_Case05_CbPrivData_T));
  ASSERT_NE(nullptr, pObjS_CbPrivData);  // CheckPoint

  for (uint16_t i = 0; i < MaxEvtCosmerNum; i++) {
    pObjS_CbPrivData[i].KeepAliveEvtCnt = 0;
    IOC_EvtID_T ObjS_SubEvtIDs[] = {IOC_EVTID_TEST_KEEPALIVE};
    IOC_SubEvtArgs_T ObjS_SubEvtArgs = {
        .CbProcEvt_F = _Case05_CbProcEvt_NvM,
        .pCbPrivData = &pObjS_CbPrivData[i],
        .EvtNum = IOC_calcArrayElmtCnt(ObjS_SubEvtIDs),
        .pEvtIDs = ObjS_SubEvtIDs,
    };
    Result = IOC_subEVT_inConlesMode(&ObjS_SubEvtArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // CheckPoint
  }

  //===BEHAVIOR===
  std::thread EvtPrduerThreads[_Case05_EvtPrduerNum];
  for (uint32_t i = 0; i < _Case05_EvtPrduerNum; i++) {
    EvtPrduerThreads[i] = std::thread([i]() {
      for (uint32_t j = 0; j < _Case05_KeepAliveEvtCnt; j++) {
        IOC_EvtDesc_T ObjB_EvtDesc = {.EvtID = IOC_EVTID_TEST_KEEPALIVE};
        IOC_Result_T Result = IOC_postEVT_inConlesMode(&ObjB_EvtDesc, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // CheckPoint
      }
    });
  }

  for (uint32_t i = 0; i < _Case05_EvtPrduerNum; i++) {
    EvtPrduerThreads[i].join();
  }

  //===VERIFY===
  for (uint16_t i = 0; i < MaxEvtCosmerNum; i++) {
    ASSERT_EQ(_Case05_KeepAliveEvtCnt * _Case05_EvtPrduerNum, pObjS_CbPrivData[i].KeepAliveEvtCnt)  // KeyVerifyPoint
        << "MaxEvtCosmrNum= " << MaxEvtCosmerNum << " i=" << i;
  }

  //===CLEANUP===
  for (uint16_t i = 0; i < MaxEvtCosmerNum; i++) {
    IOC_UnsubEvtArgs_T ObjS_UnsubEvtArgs = {.CbProcEvt_F = _Case05_CbProcEvt_NvM, .pCbPriv = &pObjS_CbPrivData[i]};
    Result = IOC_unsubEVT_inConlesMode(&ObjS_UnsubEvtArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // CheckPoint
  }

  free(pObjS_CbPrivData);
}

// Design a test case based on Case05 to verify SPECv2-c.i N:M postEVT and subEVT/cbEVT different EvtID in ConlesMode,
//    by all N will postEVT(EVTID=TEST_KEEPALIVE), all M will subEVT(EVTID=TEST_KEEPALIVE) and cbEVT(EVTID=TEST_KEEPALIVE)
//    and by all odd N will postEVT(EVTID=TEST_HELLO_FROM_ODD_TO_EVEN),
//      and by all even N will postEVT(EVTID=TEST_HELLO_FROM_EVEN_TO_ODD),
//    and by all odd M will subEVT(EVTID=TEST_HELLO_FROM_EVEN_TO_ODD) and cbEVT(EVTID=TEST_HELLO_FROM_EVEN_TO_ODD),
//      and by all even M will subEVT(EVTID=TEST_HELLO_FROM_ODD_TO_EVEN) and  cbEVT(EVTID=TEST_HELLO_FROM_ODD_TO_EVEN).

/**
 * @[Name]: verifyPostEvtNvM_byNxEvtPrduerPostEvtAndMxEvtCosmerCbProcEvtInCrossOddEvenEvtID
 * @[Purpose]: based on Case05 to verify SPECv2-c.i N:M postEVT and subEVT/cbEVT different EvtID in ConlesMode,
 *      which means some N postEVT of specific EvtID to some M who subEVT specific EvtID will be callbacked,
 *      but will not be callbacked by other M who subEVT other EvtID.
 *    Also verify the N:M behavior of SPECv2-z.4.
 * @[Steps]:
 *  1) Define $_Case06_EvtPrduerNum as N, $_Case06_EvtCosmerNum as M.
 *      |-> If CAP::CONLES_MODE_EVENT.MaxEvtCosmer is less M, then use the MaxEvtCosmer as M.
 *  2) Create M EvtCosmer to subEVT(TEST_KEEPALIVE).
 *    2.1) Foreach M EvtCosmer, if M is odd then subEVT(TEST_HELLO_FROM_EVEN_TO_ODD), else subEVT(TEST_HELLO_FROM_ODD_TO_EVEN).
 *  3) Create N EvtPrduer, foreach N start a thread to postEVT(TEST_KEEPALIVE) of $_Case06_KeepAliveCnt.
 *    3.1) Foreach N EvtPrduer start a thread, if N is odd then postEVT(TEST_HELLO_FROM_ODD_TO_EVEN) of
 $_Case06_HelloFromOddToEvenCnt, else postEVT(TEST_HELLO_FROM_EVEN_TO_ODD) of $_Case06_HelloFromEvenToOddCnt.
 *  4) Check each EvtCosmer's KeepAliveCnt is callbacked $_Case06_KeepAliveCnt * $_Case06_EvtPrduerNum times,
 *    and if EvtCosmer is odd then HelloFromEvenToOddCnt is callbacked
          ($_Case06_HelloFromEvenToOddCnt*($_Case06_EvtPrduerNum/2)) while HelloFromOddToEvenCnt is ZERO,
 *    and if EvtCosmer is even then HelloFromOddToEvenCnt is callbacked
          ($_Case06_HelloFromOddToEvenCnt*($_Case06_EvtPrduerNum/2+$_Case06_EvtPrduer%2)) while HelloFromEvenToOddCnt is ZERO.
 * @[Expect]: Step 4 is passed.
 * @[Notes]:
 */

typedef struct {
  bool IsOdd, IsEven;
  uint32_t KeepAliveCnt;
  uint32_t HelloFromEvenToOddCnt;
  uint32_t HelloFromOddToEvenCnt;
} _Case06_CbPrivData_T;

static IOC_Result_T _Case06_CbProcEvt_NvM(IOC_EvtDesc_pT pEvtDesc, void *pCbPriv) {
  _Case06_CbPrivData_T *pCbPrivData = (_Case06_CbPrivData_T *)pCbPriv;

  switch (pEvtDesc->EvtID) {
    case IOC_EVTID_TEST_KEEPALIVE: {
      pCbPrivData->KeepAliveCnt++;
    } break;
    case IOC_EVTID_TEST_HELLO_FROM_EVEN_TO_ODD: {
      if (pCbPrivData->IsOdd) {
        pCbPrivData->HelloFromEvenToOddCnt++;
      } else {
        EXPECT_TRUE(false) << "BUG: Im Odd, expectEvenToOdd, Not " << pEvtDesc->EvtID;
      }
    } break;
    case IOC_EVTID_TEST_HELLO_FROM_ODD_TO_EVEN: {
      if (pCbPrivData->IsEven) {
        pCbPrivData->HelloFromOddToEvenCnt++;
      } else {
        EXPECT_TRUE(false) << "BUG: Im Even, expectOddToEven, Not " << pEvtDesc->EvtID;
      }
    } break;
    default: {
      EXPECT_TRUE(false) << "BUG: unexpected EvtID=" << pEvtDesc->EvtID;
    }
      return IOC_RESULT_BUG;
  }

  return IOC_RESULT_SUCCESS;
}

TEST(UT_ConlesEventTypical, Case06_verifyPostEvtNvM_byNxEvtPrduerPostEvtAndMxEvtCosmerCbProcEvtInCrossOddEvenEvtID) {
  //===SETUP===
  IOC_CapabiltyDescription_T CapDesc = {.CapID = IOC_CAPID_CONLES_MODE_EVENT};
  IOC_Result_T Result = IOC_getCapabilty(&CapDesc);
  ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // CheckPoint

  uint16_t MaxEvtCosmerNum = CapDesc.ConlesModeEvent.MaxEvtCosmer;

#define _Case06_EvtPrduerNum 8
#define _Case06_EvtCosmerNum (MaxEvtCosmerNum < 8 ? MaxEvtCosmerNum : 8)
#define _Case06_KeepAliveCnt _Case03_KeepAliveEvtCnt
#define _Case06_HelloFromEvenToOddCnt 1024
#define _Case06_HelloFromOddToEvenCnt 1024

  _Case06_CbPrivData_T *pObjS_CbPrivData = (_Case06_CbPrivData_T *)malloc(_Case06_EvtCosmerNum * sizeof(_Case06_CbPrivData_T));
  ASSERT_NE(nullptr, pObjS_CbPrivData);  // CheckPoint

  for (uint16_t i = 0; i < _Case06_EvtCosmerNum; i++) {
    pObjS_CbPrivData[i].IsOdd = (i % 2 == 1);
    pObjS_CbPrivData[i].IsEven = (i % 2 == 0);
    pObjS_CbPrivData[i].KeepAliveCnt = 0;
    pObjS_CbPrivData[i].HelloFromEvenToOddCnt = 0;
    pObjS_CbPrivData[i].HelloFromOddToEvenCnt = 0;

    if (pObjS_CbPrivData[i].IsOdd) {
      IOC_EvtID_T ObjS_SubEvtIDs[] = {IOC_EVTID_TEST_KEEPALIVE, IOC_EVTID_TEST_HELLO_FROM_EVEN_TO_ODD};
      IOC_SubEvtArgs_T ObjS_SubEvtArgs = {
          .CbProcEvt_F = _Case06_CbProcEvt_NvM,
          .pCbPrivData = &pObjS_CbPrivData[i],
          .EvtNum = IOC_calcArrayElmtCnt(ObjS_SubEvtIDs),
          .pEvtIDs = ObjS_SubEvtIDs,
      };
      Result = IOC_subEVT_inConlesMode(&ObjS_SubEvtArgs);
      ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // CheckPoint
    } else {
      IOC_EvtID_T ObjS_SubEvtIDs[] = {IOC_EVTID_TEST_KEEPALIVE, IOC_EVTID_TEST_HELLO_FROM_ODD_TO_EVEN};
      IOC_SubEvtArgs_T ObjS_SubEvtArgs = {
          .CbProcEvt_F = _Case06_CbProcEvt_NvM,
          .pCbPrivData = &pObjS_CbPrivData[i],
          .EvtNum = IOC_calcArrayElmtCnt(ObjS_SubEvtIDs),
          .pEvtIDs = ObjS_SubEvtIDs,
      };
      Result = IOC_subEVT_inConlesMode(&ObjS_SubEvtArgs);
      ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // CheckPoint
    }
  }

  //===BEHAVIOR===
  std::thread EvtPrduerPostKeepAliveEvtThreads[_Case06_EvtPrduerNum];
  for (uint32_t i = 0; i < _Case06_EvtPrduerNum; i++) {
    EvtPrduerPostKeepAliveEvtThreads[i] = std::thread([i, &pObjS_CbPrivData]() {
      for (uint32_t j = 0; j < _Case06_KeepAliveCnt; j++) {
        IOC_EvtDesc_T ObjB_EvtDesc = {.EvtID = IOC_EVTID_TEST_KEEPALIVE};
        IOC_Result_T Result = IOC_postEVT_inConlesMode(&ObjB_EvtDesc, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // CheckPoint
      }
    });
  }

  std::thread EvtPrduerPostHelloFromEvenOrOddEvtThreads[_Case06_EvtPrduerNum];
  for (uint32_t i = 0; i < _Case06_EvtPrduerNum; i++) {
    EvtPrduerPostHelloFromEvenOrOddEvtThreads[i] = std::thread([i, &pObjS_CbPrivData]() {
      if (i % 2 == 0) {
        for (uint32_t j = 0; j < _Case06_HelloFromEvenToOddCnt; j++) {
          IOC_EvtDesc_T ObjB_EvtDesc = {.EvtID = IOC_EVTID_TEST_HELLO_FROM_EVEN_TO_ODD};
          IOC_Result_T Result = IOC_postEVT_inConlesMode(&ObjB_EvtDesc, NULL);
          ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // CheckPoint
        }
      } else {
        for (uint32_t j = 0; j < _Case06_HelloFromOddToEvenCnt; j++) {
          IOC_EvtDesc_T ObjB_EvtDesc = {.EvtID = IOC_EVTID_TEST_HELLO_FROM_ODD_TO_EVEN};
          IOC_Result_T Result = IOC_postEVT_inConlesMode(&ObjB_EvtDesc, NULL);
          ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // CheckPoint
        }
      }
    });
  }

  for (uint32_t i = 0; i < _Case06_EvtPrduerNum; i++) {
    EvtPrduerPostKeepAliveEvtThreads[i].join();
    EvtPrduerPostHelloFromEvenOrOddEvtThreads[i].join();
  }

  //===VERIFY===
  for (uint16_t i = 0; i < _Case06_EvtCosmerNum; i++) {
    ASSERT_EQ(_Case06_KeepAliveCnt * _Case06_EvtPrduerNum, pObjS_CbPrivData[i].KeepAliveCnt)  // KeyVerifyPoint
        << "MaxEvtCosmrNum= " << MaxEvtCosmerNum << " i=" << i;

    if (pObjS_CbPrivData[i].IsOdd) {
      ASSERT_EQ(_Case06_HelloFromEvenToOddCnt * (_Case06_EvtPrduerNum / 2),
                pObjS_CbPrivData[i].HelloFromEvenToOddCnt)  // KeyVerifyPoint
          << "MaxEvtCosmrNum= " << MaxEvtCosmerNum << " i=" << i;
      ASSERT_EQ(0, pObjS_CbPrivData[i].HelloFromOddToEvenCnt)  // KeyVerifyPoint
          << "MaxEvtCosmrNum= " << MaxEvtCosmerNum << " i=" << i;
    } else {
      ASSERT_EQ(0, pObjS_CbPrivData[i].HelloFromEvenToOddCnt)  // KeyVerifyPoint
          << "MaxEvtCosmrNum= " << MaxEvtCosmerNum << " i=" << i;
      ASSERT_EQ(_Case06_HelloFromOddToEvenCnt * ((_Case06_EvtPrduerNum / 2) + (_Case06_EvtPrduerNum % 2)),
                pObjS_CbPrivData[i].HelloFromOddToEvenCnt)  // KeyVerifyPoint
          << "MaxEvtCosmrNum= " << MaxEvtCosmerNum << " i=" << i;
    }
  }

  //===CLEANUP===
  for (uint16_t i = 0; i < _Case06_EvtCosmerNum; i++) {
    IOC_UnsubEvtArgs_T ObjS_UnsubEvtArgs = {.CbProcEvt_F = _Case06_CbProcEvt_NvM, .pCbPriv = &pObjS_CbPrivData[i]};
    Result = IOC_unsubEVT_inConlesMode(&ObjS_UnsubEvtArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // CheckPoint
  }

  free(pObjS_CbPrivData);
}
