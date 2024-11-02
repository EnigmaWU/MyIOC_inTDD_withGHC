#include <ctime>

#include "_UT_IOC_Common.h"

//======BEGIN OF OVERVIEW OF THIS UNIT TESTING FILE================================================
/**
 * MayBlock here means EvtPrducer call postEVT WILL wait for a moment IF:
 *  IOC AutoLink's internal EvtDescQueue is full in ASyncMode.
 *    OR
 *  IOC AutoLink's internal EvtDescQueue is not empty in SyncMode.
 *
 * RefDoc:
 *  1) README_UseCase.md
 *  2) UT_ConlesEventMayBlock.md
 */
//======END OF OVERVIEW OF THIS UNIT TESTING FILE==================================================

//======BEGIN OF UNIT TESTING DESIGN===============================================================
/**
 * @brief 【User Story】
 *
 *  US-1: AS an EvtProducer when I'm calling IOC_postEVT_inConlesMode,
 *        I WANT TO wait for a moment IF:
 *          AutoLink's internal EvtDescQueue is FULL in ASyncMode OR is NOT EMPTY in SyncMode,
 *        SO THAT I can make sure the posting EvtDesc will be processed by IOC.
 *
 */

/**
 * @brief 【Acceptance Criteria】
 *
 * AC-1@US-1: GIVEN EvtProducer calling IOC_postEVT_inConlesMode,
 *         WHEN IOC's EvtDescQueue is FULL in ASyncMode by a blocking EvtConsumer cbProcEvt,
 *         THEN EvtProducer WILL wait for a moment, until the EvtDescQueue has space,
 *          AND the posting EvtDesc will be processed by IOC in a reasonable SMALL time frame.
 *
 * AC-2@US-1: GIVEN EvtProducer calling IOC_postEVT_inConlesMode,
 *         WHEN IOC's EvtDescQueue is not empty in SyncMode,
 *         THEN EvtProducer WILL wait for a moment, until the EvtDescQueue is empty,
 *          AND the posting EvtDesc will be processed by IOC.
 *
 * AC-3@US-1: GIVEN EvtProducer calling IOC_postEVT_inConlesMode in high-load scenarios,
 *          WHEN IOC's EvtDescQueue is full or not empty,
 *          THEN the system WILL NOT crash,
 *            AND the posting EvtDesc will be processed by IOC within a reasonable time frame.
 *
 */

/**
 * @brief 【Test Cases】
 *
 * 【@AC-1】
 *   TC-1.1:
 *    @[Name]: verifyASyncBlock_byPostOneMoreEVT_whenEvtDescQueueFull
 *    @[Purpose]: According to AC-1, verify EvtProducer will wait for a moment,
 *       when IOC's EvtDescQueue is FULL in ASyncMode.
 *
 */

//======END OF UNIT TESTING DESIGN=================================================================

//======BEGIN OF UNIT TESTING IMPLEMENTATION=======================================================

/**
 * @[Name]: <TC-1.1>verifyASyncBlock_byPostOneMoreEVT_whenEvtDescQueueFull
 * @[Steps]:
 *    1) Get DepthEvtDescQueue by IOC_getCapabilty as SETUP
 *    2) EvtConsumer call IOC_subEVT_inConlesMode with CbProcEvt_F of:
 *       a) block on the first TEST_SLEEP_999MS event as SETUP
 *    3) EvtProducer call 1x IOC_postEVT_inConlesMode of TEST_SLEEP_999MS in ASyncMode as BEHAVIOR
 *    4) EvtProducer call up-to-full IOC_postEVT_inConlesMode of TEST_KEEPALIVE in ASyncMode as BEHAVIOR
 *       |-> 3/4's return value MUST be IOC_RESULT_SUCCESS as VERIFY
 *       |-> 3/4's post time MUST be LT 9us as VERIFY
 *    5) EvtProducer call 1x IOC_postEVT_inConlesMode of TEST_KEEPALIVE in ASyncMode as BEHAVIOR
 *       |-> 5's return value MUST be IOC_RESULT_SUCCESS as KEYVERIFY
 *       |-> 5's post time MUST be GT 999ms as KEYVERIFY
 */

typedef struct {
    ULONG_T ProcedSleep999MSCnt;
    ULONG_T ProcedKeepAliveCnt;
    sem_t *pSemSleep999MS;
} _TC01_EvtConsumerPriv_T, *_TC01_EvtConsumerPriv_pT;

static IOC_Result_T _TC01_CbProcEvt_F(IOC_EvtDesc_pT pEvtDesc, void *pCbPriv) {
    _TC01_EvtConsumerPriv_pT pEvtConsumerPriv = (_TC01_EvtConsumerPriv_pT)pCbPriv;

    if (IOC_EVTID_TEST_SLEEP_999MS == pEvtDesc->EvtID) {
        pEvtConsumerPriv->ProcedSleep999MSCnt++;
        sem_post(pEvtConsumerPriv->pSemSleep999MS);
        usleep(999 * 1000);
    } else if (IOC_EVTID_TEST_KEEPALIVE == pEvtDesc->EvtID) {
        pEvtConsumerPriv->ProcedKeepAliveCnt++;
    } else {
        EXPECT_FALSE(true) << "Unexpected EvtID: " << pEvtDesc->EvtID;
    }

    return IOC_RESULT_SUCCESS;
}

TEST(UT_ConlesEventMayBlock, verifyASyncBlock_byPostOneMoreEVT_whenEvtDescQueueFull) {
    //===SETUP===
    IOC_CapabiltyDescription_T CapDesc = {
        .CapID = IOC_CAPID_CONLES_MODE_EVENT,
    };
    IOC_Result_T Result = IOC_getCapabilty(&CapDesc);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);

    uint16_t DepthEvtDescQueue = CapDesc.ConlesModeEvent.DepthEvtDescQueue;

    // 2) EvtConsumer call IOC_subEVT_inConlesMode with CbProcEvt_F of:
    //    a) block on the first TEST_SLEEP_999MS event
    IOC_EvtID_T SubEvtIDs[] = {
        IOC_EVTID_TEST_SLEEP_999MS,
        IOC_EVTID_TEST_KEEPALIVE,
    };
    _TC01_EvtConsumerPriv_T EvtConsumerPriv = {};

    sem_unlink("UT_ConlesEventMayBlock_TC01");
    EvtConsumerPriv.pSemSleep999MS = sem_open("UT_ConlesEventMayBlock_TC01", O_CREAT, 0644, 0);
    ASSERT_NE(SEM_FAILED, EvtConsumerPriv.pSemSleep999MS);

    IOC_SubEvtArgs_T SubArgs = {
        .CbProcEvt_F = _TC01_CbProcEvt_F,
        .pCbPrivData = &EvtConsumerPriv,
        .EvtNum      = IOC_calcArrayElmtCnt(SubEvtIDs),
        .pEvtIDs     = SubEvtIDs,
    };

    Result = _IOC_subEVT_inConlesMode(&SubArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);

    //===BEHAVIOR & VERIFY & CLEANUP===
    struct timespec TS_Begin, TS_End;

    // 3) EvtProducer call 1x IOC_postEVT_inConlesMode of TEST_SLEEP_999MS in ASyncMode
    IOC_EvtDesc_T EvtDescSleep999MS = {
        .EvtID = IOC_EVTID_TEST_SLEEP_999MS,
    };
    TS_Begin                      = IOC_getCurrentTimeSpec();
    IOC_Result_T ResultSleep999MS = IOC_postEVT_inConlesMode(&EvtDescSleep999MS, NULL);  // OPT==NULL==ASyncMayBlock
    TS_End                        = IOC_getCurrentTimeSpec();
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultSleep999MS);
    EXPECT_LT(IOC_deltaTimeSpecInUS(&TS_Begin, &TS_End), 9);

    IOC_wakeupProcEVT();  // wakeup EvtDescSleep999MS to be processed.
    sem_wait(EvtConsumerPriv.pSemSleep999MS);

    // 4) EvtProducer call up-to-full IOC_postEVT_inConlesMode of TEST_KEEPALIVE in ASyncMode
    IOC_EvtDesc_T EvtDescKeepAlive = {
        .EvtID = IOC_EVTID_TEST_KEEPALIVE,
    };
    for (ULONG_T i = 0; i < DepthEvtDescQueue; i++) {
        TS_Begin = IOC_getCurrentTimeSpec();
        Result   = IOC_postEVT_inConlesMode(&EvtDescKeepAlive, NULL);  // OPT==NULL==ASyncMayBlock
        TS_End   = IOC_getCurrentTimeSpec();
        ASSERT_EQ(IOC_RESULT_SUCCESS, Result);
        EXPECT_LT(IOC_deltaTimeSpecInUS(&TS_Begin, &TS_End), 9);
    }

    // 5) EvtProducer call 1x IOC_postEVT_inConlesMode of TEST_KEEPALIVE in ASyncMode
    TS_Begin = IOC_getCurrentTimeSpec();
    Result   = IOC_postEVT_inConlesMode(&EvtDescKeepAlive, NULL);  // OPT==NULL==ASyncMayBlock
    TS_End   = IOC_getCurrentTimeSpec();
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);
    EXPECT_GE(IOC_deltaTimeSpecInMS(&TS_Begin, &TS_End), 999);

    //===CLEANUP===
    IOC_forceProcEVT();  // force all EvtDesc in IOC's EvtDescQueue to be processed.
    ASSERT_EQ(1, EvtConsumerPriv.ProcedSleep999MSCnt);
    ASSERT_EQ(DepthEvtDescQueue + 1, EvtConsumerPriv.ProcedKeepAliveCnt);

    IOC_UnsubEvtArgs_T UnsubArgs = {
        .CbProcEvt_F = _TC01_CbProcEvt_F,
        .pCbPrivData = &EvtConsumerPriv,
    };
    Result = _IOC_unsubEVT_inConlesMode(&UnsubArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);
}

//======END OF UNIT TESTING IMPLEMENTATION=========================================================