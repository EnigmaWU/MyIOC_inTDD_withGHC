/**
 * @file UT_ConlesEventUseBoundary.cxx
 * @note Use Boundary to verify API is used in Min/Max/Beyond conditions.
 *---------------------------------------------------------------------------------------------------------------------
 *===> Begin DesignOfUT from Acceptace Creteria(a.k.a) <===
 *  1)
 *===> End DesignOfUT <===
 *---------------------------------------------------------------------------------------------------------------------
 *===> Begin DesignOfTestCase <===
 *  1) verifyPostProcEvtSuccess_by1xSubPostUnsubEvt
 *===> End DesignOfTestCase <===
 *---------------------------------------------------------------------------------------------------------------------
 * @note RefTemplate: TEMPLATE OF UT CASE in UT_FreelyDrafts.cxx
 */

#include <sys/_types/_null.h>

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

    IOC_UnsubEvtArgs_T UnsubArgs = {0};
    UnsubArgs.CbProcEvt_F        = _Case01_CbProcEvt;
    UnsubArgs.pCbPrivData        = &privData;

    Result = IOC_unsubEVT_inConlesMode(&UnsubArgs);
    ASSERT_EQ(Result, IOC_RESULT_SUCCESS) << "IOC_unsubEVT_inConlesMode failed Result=" << Result;

    EXPECT_EQ(privData.CbCnt, 1) << "CbProcEvt is MUST==1, while CbCnt=" << privData.CbCnt;  // KeyVerifyPoint
}