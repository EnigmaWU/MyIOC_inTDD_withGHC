/**
 * @file UT_ConlesEventUseBoundary.cxx
 * @note Use Boundary to verify API is used in Min/Max/Beyond conditions.
 *---------------------------------------------------------------------------------------------------------------------
 *===> Begin DesignOfUT from Acceptace Creteria(a.k.a AC) <===
 *  a) Min condition: verify subEVT, postEVT, unsubEVT success at Minimum operations.
 *===> End DesignOfUT <===
 *---------------------------------------------------------------------------------------------------------------------
 *===> Begin DesignOfTestCase <===
 *  1) verifyPostProcEvtSuccess_by1xSubPostUnsubEvt
 *===> End DesignOfTestCase <===
 *---------------------------------------------------------------------------------------------------------------------
 * @note RefTemplate: TEMPLATE OF UT CASE in UT_FreelyDrafts.cxx
 */

#include "_UT_IOC_Common.h"

/**
 * @[Name]: verifyPostProcEvtSuccess_by1xSubPostUnsubEvt
 * @[Purpose]: verify PostProcEvtSuccess at Minimum SubPostUnsubEvt condition.
 * @[Steps]:
 *  1) call subEVT as BEHAVIOR
 *     |-> Args[EvtID=TestKeepAlive, CbProcEvt_F=_Case01_CbProcEvt, pCbPrivData=_Case01_PrivData]
 *     |-> RefAPI: IOC_subEVT_inConlesMode in IOC.h
 *     |-> RefType: IOC_SubEvtArgs_T in IOC_Types.h
 *  2) call postEVT with EvtDesc::EvtID is TestKeepAlive as BEHAVIOR
 *     |-> RefAPI: IOC_postEVT in IOC.h
 *     |-> RefType: IOC_EvtDesc_T in IOC_Types.h
 *  3) call unsubEVT as BEHAVIOR
 * @[Expects]:
 *  a) subEVT, postEVT, unsubEVT return IOC_RESULT_SUCCESS
 *  b) CbProcEvt is called once, which means _Case01_PrivData.CbCnt is 1
 * @[Tips]:
 */

typedef struct {
    uint32_t CbCnt;
} _Case01_PrivData_T, *_Case01_PrivData_pT;

static IOC_Result_T _Case01_CbProcEvt(IOC_EvtDesc_T *pEvtDesc, void *pCbPrivData) {
    _Case01_PrivData_pT pPrivData = (_Case01_PrivData_pT)pCbPrivData;

    EXPECT_EQ(pEvtDesc->EvtID, IOC_EVTID_TEST_KEEPALIVE) << "EvtID is not IOC_EVTID_TEST_KEEPALIVE";
    pPrivData->CbCnt++;

    return IOC_RESULT_SUCCESS;
}

TEST(ConlesEventUseBoundary, verifyPostProcEvtSuccess_by1xSubPostUnsubEvt) {
    IOC_Result_T Result      = IOC_RESULT_BUG;
    IOC_SubEvtArgs_T SubArgs = {0};

    _Case01_PrivData_T privData = {0};

    SubArgs.CbProcEvt_F = _Case01_CbProcEvt;
    SubArgs.pCbPrivData = &privData;

    IOC_EvtID_T EvtIDs[] = {IOC_EVTID_TEST_KEEPALIVE};
    SubArgs.pEvtIDs      = EvtIDs;
    SubArgs.EvtNum       = IOC_calcArrayElmtCnt(EvtIDs);

    Result = IOC_subEVT_inConlesMode(&SubArgs);
    ASSERT_EQ(Result, IOC_RESULT_SUCCESS) << "IOC_subEVT_inConlesMode failed Result=" << Result;

    IOC_EvtDesc_T EvtDesc = {0};
    EvtDesc.EvtID         = IOC_EVTID_TEST_KEEPALIVE;
    Result                = IOC_postEVT_inConlesMode(&EvtDesc, NULL);
    ASSERT_EQ(Result, IOC_RESULT_SUCCESS) << "IOC_postEVT_inConlesMode failed Result=" << Result;

    IOC_forceProcEVT();  // force to process event

    IOC_UnsubEvtArgs_T UnsubArgs = {0};
    UnsubArgs.CbProcEvt_F        = _Case01_CbProcEvt;
    UnsubArgs.pCbPrivData        = &privData;

    Result = IOC_unsubEVT_inConlesMode(&UnsubArgs);
    ASSERT_EQ(Result, IOC_RESULT_SUCCESS) << "IOC_unsubEVT_inConlesMode failed Result=" << Result;

    ASSERT_EQ(privData.CbCnt, 1) << "CbProcEvt is MUST==1, while CbCnt=" << privData.CbCnt;  // KeyVerifyPoint
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct {
    uint32_t Sleep99MsCnt, Sleep999MsCnt;
} _Case02_PrivData_T, *_Case02_PrivData_pT;

static IOC_Result_T _Case02_CbProcEvt(IOC_EvtDesc_T *pEvtDesc, void *pCbPrivData) {
    _Case02_PrivData_pT pC02PrivData = (_Case02_PrivData_pT)pCbPrivData;

    if (pEvtDesc->EvtID == IOC_EVTID_TEST_SLEEP_99MS) {
      usleep(99 * 1000);  // 99ms
      pC02PrivData->Sleep99MsCnt++;
    } else if (pEvtDesc->EvtID == IOC_EVTID_TEST_SLEEP_999MS) {
      usleep(999 * 1000);  // 999ms
      pC02PrivData->Sleep999MsCnt++;
    } else {
      EXPECT_TRUE(false) << "EvtID is not IOC_EVTID_TEST_SLEEP_99MS or IOC_EVTID_TEST_SLEEP_999MS";
    }

    return IOC_RESULT_SUCCESS;
}

TEST(ConlesEventUseBoundary, verifyForceProcEVT_willWaitForLastBlockedCbProcEvtReturning) {
    //===SETUP===
    _Case02_PrivData_T C02PrivData = {
        .Sleep99MsCnt  = 0,
        .Sleep999MsCnt = 0,
    };

    IOC_EvtID_T SubEvtIDs[]  = {IOC_EVTID_TEST_SLEEP_99MS, IOC_EVTID_TEST_SLEEP_999MS};
    IOC_SubEvtArgs_T SubArgs = {
        .CbProcEvt_F = _Case02_CbProcEvt,
        .pCbPrivData = &C02PrivData,
        .EvtNum      = IOC_calcArrayElmtCnt(SubEvtIDs),
        .pEvtIDs     = SubEvtIDs,
    };

    IOC_Result_T Result = IOC_subEVT_inConlesMode(&SubArgs);
    ASSERT_EQ(Result, IOC_RESULT_SUCCESS) << "IOC_subEVT_inConlesMode failed Result=" << Result;

    //===BEHAVIOR===
    IOC_EvtDesc_T EvtDesc99Ms = {
        .EvtID = IOC_EVTID_TEST_SLEEP_99MS,
    };
    Result = IOC_postEVT_inConlesMode(&EvtDesc99Ms, NULL);
    ASSERT_EQ(Result, IOC_RESULT_SUCCESS) << "IOC_postEVT_inConlesMode failed Result=" << Result;

    IOC_EvtDesc_T EvtDesc999Ms = {
        .EvtID = IOC_EVTID_TEST_SLEEP_999MS,
    };
    Result = IOC_postEVT_inConlesMode(&EvtDesc999Ms, NULL);
    ASSERT_EQ(Result, IOC_RESULT_SUCCESS) << "IOC_postEVT_inConlesMode failed Result=" << Result;

    // post 999ms again to trigger warning message
    Result = IOC_postEVT_inConlesMode(&EvtDesc999Ms, NULL);
    ASSERT_EQ(Result, IOC_RESULT_SUCCESS) << "IOC_postEVT_inConlesMode failed Result=" << Result;

    //===VERIFY===
    IOC_forceProcEVT();  // force to process event

    ASSERT_EQ(C02PrivData.Sleep99MsCnt, 1) << "Sleep99MsCnt is MUST==1, while Sleep99MsCnt="
                                           << C02PrivData.Sleep99MsCnt;  // KeyVerifyPoint
    ASSERT_EQ(C02PrivData.Sleep999MsCnt, 2) << "Sleep999MsCnt is MUST==1, while Sleep999MsCnt="
                                            << C02PrivData.Sleep999MsCnt;  // KeyVerifyPoint

    //===CLEANUP===
    IOC_UnsubEvtArgs_T UnsubArgs = {
        .CbProcEvt_F = _Case02_CbProcEvt,
        .pCbPrivData = &C02PrivData,
    };

    Result = IOC_unsubEVT_inConlesMode(&UnsubArgs);
    ASSERT_EQ(Result, IOC_RESULT_SUCCESS) << "IOC_unsubEVT_inConlesMode failed Result=" << Result;
}