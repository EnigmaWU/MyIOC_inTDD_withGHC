#include <pthread.h>

#include "_UT_IOC_Common.h"

/**
 * NonBlock here means EvtPrducer call postEVT WON'T wait for a moment IF:
 *  IOC AutoLink's internal EvtDescQueue is full in ASyncMode.
 *    OR
 *  IOC AutoLink's internal EvtDescQueue is not empty in SyncMode.
 *
 * RefDoc:
 *  1) README_UseCase.md
 *  2) UT_ConlesEventNonBlock.md
 */

//======BEGIN OF UNIT TESTING DESIGN===============================================================
/**
 * @brief 【User Story】
 *
 *  US-1: AS an EvtProducer when I'm calling IOC_postEVT_inConlesMode,
 *        I WANT TO return immediately without waiting for a moment IF:
 *          AutoLink's internal EvtDescQueue is FULL in ASyncMode OR is NOT EMPTY in SyncMode,
 *        SO THAT I can continue my work without accidential BLOCKING.
 *
 */

/**
 * @brief 【Acceptance Criteria】
 *
 * AC-1@US-1: GIVEN EvtProducer calling IOC_postEVT_inConlesMode,
 *         WHEN IOC's EvtDescQueue is full in ASyncMode by a blocking EvtConsumer cbProcEvt,
 *         THEN EvtProducer can return immediately without waiting for a moment,
 *           AND the posting EvtDesc will never be processed by IOC.
 * AC-2@US-1: GIVEN EvtProducer calling IOC_postEVT_inConlesMode,
 *         WHEN IOC's EvtDescQueue is not empty in SyncMode,
 *         THEN EvtProducer can return immediately without waiting for a moment.
 *          AND the posting EvtDesc will never be processed by IOC.
 *
 * AC-3@US-1: GIVEN EvtConsumer's CbProcEvt_F MAY be blocked accedentially,
 *          WHEN many EvtProducer call IOC_postEVT_inConlesMode both in ASyncMode or SyncMode,
 *          THEN EvtProducer WILL return immediately without waiting for a moment IF:
 *            IOC's EvtDescQueue is FULL in ASyncMode OR is NOT EMPTY in SyncMode.
 *
 */

/**
 * @brief 【Test Cases】
 *
 * TC-1: verifyASyncNonblock_byPostOneMoreEVT_whenEvtDescQueueFull
 * TC-2: verifySyncNonblock_byPostOneMoreEVT_whenEvtDescQueueNotEmpty
 * TC-3:
 *    @[Name]: verifyHybridNonblock_byAlternatelyCbProcEvtBlockedOrNot_withHighConcurrency
 *    @[Purpose]: According to AC-3 verify EvtProducer will return immediately without waiting for a moment when IOC's
 * EvtDescQueue is FULL in ASyncMode OR is NOT EMPTY in SyncMode.
 *
 */

//======END OF UNIT TESTING DESIGN=================================================================

/**
 * @[Name]: <TC-1>verifyASyncNonblock_byPostOneMoreEVT_whenEvtDescQueueFull
 * @[Purpose]: According to AC-1, verify EvtProducer can return immediately without waiting for a
 * moment IF IOC's EvtDescQueue is full in ASyncMode.
 * @[Steps]: ${how to do}
 *   1) call IOC_getCapability to know QUEUE_DEPTH of AutoLink's EvtDescQueue, as SETUP
 *   2) call IOC_subEVT(TEST_KEEPALIVE) with __TC1_cbProcEvt as SETUP
 *   3) call first IOC_postEVT(TEST_KEEPALIVE) in ASyncMode as BEHAVIOR
 *      3.1) wait for __TC1_cbProcEvt to be called and block it, to avoid further event processing.
 *      3.2) call more IOC_postEVT(TEST_KEEPALIVE) in ASyncMode to fullfill the EvtDescQueue.
 *   4) call one more IOC_postEVT(TEST_KEEPALIVE) in ASyncMode as VERIFY
 *      4.1) check the return value is IOC_RESULT_TOO_MANY_QUEUING_EVTDESC.
 *   5) call IOC_unsubEVT(TEST_KEEPALIVE) as CLEANUP
 * @[Expect]: Step 4) return value is IOC_RESULT_TOO_MANY_QUEUING_EVTDESC.
 * @[Notes]:
 *   KeepAliveCnt++ in __TC1_cbProcEvt, and check it lastly equal to QUEUE_DEPTH.
 */

typedef struct {
    ULONG_T KeepAliveCnt;

    // Main lockIT+postEVT+lockIT, ...until..., Cb unlockIT, Main continue
    // which means: Main know Cb is called.
    pthread_mutex_t FirstCbEnterMutex;

    // Main lockIT, ..., Cb lockIT, ...untile..., Main lastly postEVT+unlockIT
    // which means: Cb will be blocked by Main until Main post last EvtDesc.
    pthread_mutex_t WaitMainLastPostEvtMutex;  // Last=QUEUE_DEPTH+1

} _TC1_PrivData_T, *_TC1_PrivData_pT;

// TC-1's callback function(RefAPI: IOC_CbProcEvt_F in IOC_EvtAPI.h)
static IOC_Result_T __TC1_cbProcEvt(IOC_EvtDesc_pT pEvtDesc, void *pCbPrivData) {
    _TC1_PrivData_pT pTC1PrivData = (_TC1_PrivData_pT)pCbPrivData;

    if (pEvtDesc->EvtID == IOC_EVTID_TEST_KEEPALIVE) {
        pTC1PrivData->KeepAliveCnt++;
    } else {
        EXPECT_FALSE(true) << "Unexpected EvtID: " << pEvtDesc->EvtID;
    }

    if (pTC1PrivData->KeepAliveCnt == 1) {
        // RefStep: 3.1) wait for __TC1_cbProcEvt to be called and block it.
        pthread_mutex_unlock(&pTC1PrivData->FirstCbEnterMutex);
        pthread_mutex_lock(&pTC1PrivData->WaitMainLastPostEvtMutex);
    }

    return IOC_RESULT_SUCCESS;
}

TEST(UT_ConlesEventConcurrency, verifyASyncNonblock_byPostOneMoreEVT_whenEvtDescQueueFull) {
    //===SETUP===
    IOC_CapabiltyDescription_T CapDesc = {
        .CapID = IOC_CAPID_CONLES_MODE_EVENT,
    };
    IOC_Result_T Result = IOC_getCapability(&CapDesc);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);

    ULONG_T QUEUE_DEPTH = CapDesc.ConlesModeEvent.DepthEvtDescQueue;
    ASSERT_GT(QUEUE_DEPTH, 1);

    //---------------------------------------------------------------------------
    _TC1_PrivData_T TC1PrivData = {
        .KeepAliveCnt = 0,
        .FirstCbEnterMutex = PTHREAD_MUTEX_INITIALIZER,
        .WaitMainLastPostEvtMutex = PTHREAD_MUTEX_INITIALIZER,
    };
    IOC_EvtID_T SubEvtIDs[] = {
        IOC_EVTID_TEST_KEEPALIVE,
    };
    IOC_SubEvtArgs_T SubArgs = {
        .CbProcEvt_F = __TC1_cbProcEvt,
        .pCbPrivData = &TC1PrivData,
        .EvtNum = IOC_calcArrayElmtCnt(SubEvtIDs),
        .pEvtIDs = SubEvtIDs,
    };

    Result = _IOC_subEVT_inConlesMode(&SubArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);

    //===BEHAVIOR===
    pthread_mutex_lock(&TC1PrivData.FirstCbEnterMutex);
    pthread_mutex_lock(&TC1PrivData.WaitMainLastPostEvtMutex);

    IOC_EvtDesc_T EvtDesc = {
        .EvtID = IOC_EVTID_TEST_KEEPALIVE,
    };
    IOC_Option_defineNonBlock(OptNonBlock);

    Result = IOC_postEVT_inConlesMode(&EvtDesc, &OptNonBlock);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);

    // RefStep: 3.1) wait for __TC1_cbProcEvt to be called and block it.
    pthread_mutex_lock(&TC1PrivData.FirstCbEnterMutex);

    // RefStep: 3.2) call more IOC_postEVT(TEST_KEEPALIVE) in ASyncMode to fullfill the EvtDescQueue.
    for (ULONG_T i = 0; i < QUEUE_DEPTH; i++) {
        Result = IOC_postEVT_inConlesMode(&EvtDesc, &OptNonBlock);
        ASSERT_EQ(IOC_RESULT_SUCCESS, Result);
    }

    //===VERIFY===
    // RefStep: 4.1) check the return value is IOC_RESULT_TOO_MANY_QUEUING_EVTDESC.
    Result = IOC_postEVT_inConlesMode(&EvtDesc, &OptNonBlock);
    ASSERT_EQ(IOC_RESULT_TOO_MANY_QUEUING_EVTDESC, Result);  // KeyVerifyPoint

    //===CLEANUP===
    // RefStep: 3.1) wait for __TC1_cbProcEvt to be called and block it.
    pthread_mutex_unlock(&TC1PrivData.WaitMainLastPostEvtMutex);

    IOC_forceProcEVT();  // force all EvtDesc in IOC's EvtDescQueue to be processed.
    ASSERT_EQ(QUEUE_DEPTH + 1, TC1PrivData.KeepAliveCnt);

    IOC_UnsubEvtArgs_T UnsubArgs = {
        .CbProcEvt_F = __TC1_cbProcEvt,
        .pCbPrivData = &TC1PrivData,
    };
    Result = _IOC_unsubEVT_inConlesMode(&UnsubArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);
}

/**
 * @[Name]: <TC-2>verifySyncNonblock_byPostOneMoreEVT_whenEvtDescQueueNotEmpty
 * @[Purpose]: According to AC-2, verify EvtProducer can return immediately without waiting for a
 *    moment IF IOC's EvtDescQueue is not empty in SyncMode.
 * @[Steps]:
 *   1) call IOC_subEVT(TEST_KEEPALIVE) with __TC2_cbProcEvt as SETUP
 *   2) call first IOC_postEVT(TEST_KEEPALIVE) in ASyncMode as BEHAVIOR
 *    a) wait for __TC2_cbProcEvt to be called and block it, to indicate EvtDescQueue is not empty.
 *   3) call one more IOC_postEVT(TEST_KEEPALIVE) in SyncMode as VERIFY
 *    a) check the return value is IOC_RESULT_TOO_LONG_EMPTYING_EVTDESC_QUEUE
 *   4) call IOC_unsubEVT(TEST_KEEPALIVE) as CLEANUP
 * @[Expect]: Step 3) return value is IOC_RESULT_TOO_LONG_EMPTYING_EVTDESC_QUEUE.
 * @[Notes]:
 */

typedef struct {
    ULONG_T KeepAliveCnt;

    // Main lockIT+postEVT+lockIT, ...until..., Cb unlockIT, Main continue
    // which means: Main know Cb is called.
    pthread_mutex_t FirstCbEnterMutex;

    // Main lockIT, ..., Cb lockIT, ...untile..., Main lastly postEVT+unlockIT
    // which means: Cb will be blocked by Main until Main post last EvtDesc.
    pthread_mutex_t WaitMainLastPostEvtMutex;  // Last=1

} _TC2_PrivData_T, *_TC2_PrivData_pT;

// TC-2's callback function(RefAPI: IOC_CbProcEvt_F in IOC_EvtAPI.h)
static IOC_Result_T __TC2_cbProcEvt(IOC_EvtDesc_pT pEvtDesc, void *pCbPrivData) {
    _TC2_PrivData_pT pTC2PrivData = (_TC2_PrivData_pT)pCbPrivData;

    if (pEvtDesc->EvtID == IOC_EVTID_TEST_KEEPALIVE) {
        pTC2PrivData->KeepAliveCnt++;
    } else {
        EXPECT_FALSE(true) << "Unexpected EvtID: " << pEvtDesc->EvtID;
    }

    if (pTC2PrivData->KeepAliveCnt == 1) {
        // RefStep: 2.a) wait for __TC2_cbProcEvt to be called and block it.
        pthread_mutex_unlock(&pTC2PrivData->FirstCbEnterMutex);
        pthread_mutex_lock(&pTC2PrivData->WaitMainLastPostEvtMutex);
    }

    return IOC_RESULT_SUCCESS;
}

TEST(UT_ConlesEventConcurrency, verifySyncNonblock_byPostOneMoreEVT_whenEvtDescQueueNotEmpty) {
    //===SETUP===
    _TC2_PrivData_T TC2PrivData = {
        .KeepAliveCnt = 0,
        .FirstCbEnterMutex = PTHREAD_MUTEX_INITIALIZER,
        .WaitMainLastPostEvtMutex = PTHREAD_MUTEX_INITIALIZER,
    };
    IOC_EvtID_T SubEvtIDs[] = {
        IOC_EVTID_TEST_KEEPALIVE,
    };
    IOC_SubEvtArgs_T SubArgs = {
        .CbProcEvt_F = __TC2_cbProcEvt,
        .pCbPrivData = &TC2PrivData,
        .EvtNum = IOC_calcArrayElmtCnt(SubEvtIDs),
        .pEvtIDs = SubEvtIDs,
    };

    IOC_Result_T Result = _IOC_subEVT_inConlesMode(&SubArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);

    //===BEHAVIOR===
    pthread_mutex_lock(&TC2PrivData.FirstCbEnterMutex);
    pthread_mutex_lock(&TC2PrivData.WaitMainLastPostEvtMutex);

    IOC_EvtDesc_T EvtDesc = {
        .EvtID = IOC_EVTID_TEST_KEEPALIVE,
    };

    Result = IOC_postEVT_inConlesMode(&EvtDesc, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);

    // RefStep: 2.a) wait for __TC2_cbProcEvt to be called and block it.
    pthread_mutex_lock(&TC2PrivData.FirstCbEnterMutex);

    //===VERIFY===
    // RefStep: 3.a) check the return value is IOC_RESULT_TOO_LONG_EMPTYING_EVTDESC_QUEUE.
    IOC_Option_defineSyncNonBlock(OptSyncNonBlock);
    Result = IOC_postEVT_inConlesMode(&EvtDesc, &OptSyncNonBlock);
    ASSERT_EQ(IOC_RESULT_TOO_LONG_EMPTYING_EVTDESC_QUEUE, Result);  // KeyVerifyPoint

    //===CLEANUP===
    // RefStep: 2.a) wait for __TC2_cbProcEvt to be called and block it.
    pthread_mutex_unlock(&TC2PrivData.WaitMainLastPostEvtMutex);

    IOC_forceProcEVT();  // force all EvtDesc in IOC's EvtDescQueue to be processed.
    ASSERT_EQ(1, TC2PrivData.KeepAliveCnt);

    IOC_UnsubEvtArgs_T UnsubArgs = {
        .CbProcEvt_F = __TC2_cbProcEvt,
        .pCbPrivData = &TC2PrivData,
    };
    Result = _IOC_unsubEVT_inConlesMode(&UnsubArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);
}

/**
 * @[Name]: <TC-3>verifyHybridNonblock_byAlternatelyCbProcEvtBlockedOrNot_withHighConcurrency
 * @[Steps]:
 *  1) call IOC_subEVT(TEST_KEEPALIVE and TEST_SLEEP_9US) with __TC3_cbProcEvt as SETUP
 *      |-> in __TC3_cbProcEvt,
 *          |-> if EvtID is TEST_KEEPALIVE, then CbKeepAliveCnt++.
 *          |-> if EvtID is TEST_SLEEP_9US, then sleep 9us and CbSleep9USCnt++.
 *  2) create _TC3_MAX_N_ASYNC_THREADS and _TC3_MAX_M_SYNC_THREADS as SETUP
 *  3) In each async thread, call _TC3_MAX_NN_EVENTS(>=10M) postEVT in ASyncMode as BEHAVIOR
 *      |-> TEST_KEEPALIVE by default, and TEST_SLEEP_9US every 10000 events.
 *      |-> if IOC_RESULT_SUCCESS, then ASyncPostSuccessCnt++.
 *      |-> if IOC_RESULT_TOO_MANY_QUEUING_EVTDESC, then ASyncPostNonBlockCnt++.
 *  4) In each sync thread, call _TC3_MAX_MM_EVENTS(>=10M) postEVT in SyncMode as BEHAVIOR
 *      RefStep: 3) for each async thread.
 *  5) check ASyncPostSuccessCnt, ASyncPostNonBlockCnt, SyncPostSuccessCnt, SyncPostNonBlockCnt as VERIFY
 *      |-> TotalASyncSuccessPostCnt = SUM(ASyncPostSuccessCnt) in each _TC3_MAX_N_ASYNC_THREADS
 *      |-> TotalSyncPostSuccessCnt = SUM(SyncPostSuccessCnt) in each _TC3_MAX_M_SYNC_THREADS
 *      |-> TotalPostSuccessCnt = TotalASyncSuccessPostCnt + TotalSyncPostSuccessCnt
 *      |-> TotalPostSuccessCnt = CbKeepAliveCnt + CbSleep9USCnt
 *  6) call IOC_unsubEVT(TEST_KEEPALIVE and TEST_SLEEP_9US) as CLEANUP
 *
 */

#define _TC3_MAX_N_ASYNC_THREADS 16
#define _TC3_MAX_M_SYNC_THREADS 16
#define _TC3_MAX_NN_EVENTS 1000000
#define _TC3_MAX_MM_EVENTS 1000000

typedef struct {
    ULONG_T CbKeepAliveCnt;
    ULONG_T CbSleep9USCnt;
} _TC3_CbPrivData_T, *_TC3_CbPrivData_pT;

static IOC_Result_T __TC3_cbProcEvt(IOC_EvtDesc_pT pEvtDesc, void *pCbPrivData) {
    _TC3_CbPrivData_pT pTC3PrivData = (_TC3_CbPrivData_pT)pCbPrivData;

    if (pEvtDesc->EvtID == IOC_EVTID_TEST_KEEPALIVE) {
        pTC3PrivData->CbKeepAliveCnt++;
    } else if (pEvtDesc->EvtID == IOC_EVTID_TEST_SLEEP_9US) {
        usleep(9);
        pTC3PrivData->CbSleep9USCnt++;
    } else {
        EXPECT_FALSE(true) << "Unexpected EvtID: " << pEvtDesc->EvtID;
    }

    return IOC_RESULT_SUCCESS;
}

typedef struct {
    ULONG_T ASyncPostSuccessCnt;
    ULONG_T ASyncPostNonBlockCnt;
} _TC3_ASyncPostStat_T, *_TC3_ASyncPostStat_pT;

static void *__TC3_AsyncPostThread(void *pArg) {
    _TC3_ASyncPostStat_pT pASyncPostStat = (_TC3_ASyncPostStat_pT)pArg;

    IOC_EvtDesc_T EvtDescKeepAlive = {
        .EvtID = IOC_EVTID_TEST_KEEPALIVE,
    };
    IOC_EvtDesc_T EvtDescSleep9US = {
        .EvtID = IOC_EVTID_TEST_SLEEP_9US,
    };

    IOC_Option_defineNonBlock(OptNonBlock);
    IOC_Result_T Result;

    for (ULONG_T i = 0; i < _TC3_MAX_NN_EVENTS; i++) {
        Result = IOC_postEVT_inConlesMode(&EvtDescKeepAlive, &OptNonBlock);
        if (Result == IOC_RESULT_SUCCESS) {
            pASyncPostStat->ASyncPostSuccessCnt++;
        } else if (Result == IOC_RESULT_TOO_MANY_QUEUING_EVTDESC) {
            pASyncPostStat->ASyncPostNonBlockCnt++;
        } else {
            EXPECT_FALSE(true) << "Unexpected Result: " << Result;
        }

        if (i % 10000 == 0) {
            Result = IOC_postEVT_inConlesMode(&EvtDescSleep9US, &OptNonBlock);
            if (Result == IOC_RESULT_SUCCESS) {
                pASyncPostStat->ASyncPostSuccessCnt++;
            } else if (Result == IOC_RESULT_TOO_MANY_QUEUING_EVTDESC) {
                pASyncPostStat->ASyncPostNonBlockCnt++;
            } else {
                EXPECT_FALSE(true) << "Unexpected Result: " << Result;
            }
        }
    }

    return NULL;
}

typedef struct {
    ULONG_T SyncPostSuccessCnt;
    ULONG_T SyncPostNonBlockCnt;
} _TC3_SyncPostStat_T, *_TC3_SyncPostStat_pT;

static void *__TC3_SyncPostThread(void *pArg) {
    _TC3_SyncPostStat_pT pSyncPostStat = (_TC3_SyncPostStat_pT)pArg;

    IOC_EvtDesc_T EvtDescKeepAlive = {
        .EvtID = IOC_EVTID_TEST_KEEPALIVE,
    };
    IOC_EvtDesc_T EvtDescSleep9US = {
        .EvtID = IOC_EVTID_TEST_SLEEP_9US,
    };

    IOC_Option_defineSyncNonBlock(OptSyncNonBlock);
    IOC_Result_T Result;

    for (ULONG_T i = 0; i < _TC3_MAX_MM_EVENTS; i++) {
        Result = IOC_postEVT_inConlesMode(&EvtDescKeepAlive, &OptSyncNonBlock);
        if (Result == IOC_RESULT_SUCCESS) {
            pSyncPostStat->SyncPostSuccessCnt++;
        } else if (Result == IOC_RESULT_TOO_LONG_EMPTYING_EVTDESC_QUEUE) {
            pSyncPostStat->SyncPostNonBlockCnt++;
        } else {
            EXPECT_FALSE(true) << "Unexpected Result: " << Result;
        }

        if (i % 10000 == 0) {
            Result = IOC_postEVT_inConlesMode(&EvtDescSleep9US, &OptSyncNonBlock);
            if (Result == IOC_RESULT_SUCCESS) {
                pSyncPostStat->SyncPostSuccessCnt++;
            } else if (Result == IOC_RESULT_TOO_LONG_EMPTYING_EVTDESC_QUEUE) {
                pSyncPostStat->SyncPostNonBlockCnt++;
            } else {
                EXPECT_FALSE(true) << "Unexpected Result: " << Result;
            }
        }
    }

    return NULL;
}

TEST(UT_ConlesEventConcurrency, verifyHybridNonblock_byAlternatelyCbProcEvtBlockedOrNot_withHighConcurrency) {
    //===SETUP===
    _TC3_CbPrivData_T TC3PrivData = {
        .CbKeepAliveCnt = 0,
        .CbSleep9USCnt = 0,
    };
    IOC_EvtID_T SubEvtIDs[] = {
        IOC_EVTID_TEST_KEEPALIVE,
        IOC_EVTID_TEST_SLEEP_9US,
    };
    IOC_SubEvtArgs_T SubArgs = {
        .CbProcEvt_F = __TC3_cbProcEvt,
        .pCbPrivData = &TC3PrivData,
        .EvtNum = IOC_calcArrayElmtCnt(SubEvtIDs),
        .pEvtIDs = SubEvtIDs,
    };

    IOC_Result_T Result = _IOC_subEVT_inConlesMode(&SubArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);

    //===BEHAVIOR===
    _TC3_ASyncPostStat_T ASyncPostStat[_TC3_MAX_N_ASYNC_THREADS] = {0};
    _TC3_SyncPostStat_T SyncPostStat[_TC3_MAX_M_SYNC_THREADS] = {0};

    pthread_t ASyncThreads[_TC3_MAX_N_ASYNC_THREADS];
    pthread_t SyncThreads[_TC3_MAX_M_SYNC_THREADS];

    int PosixResult;

    for (ULONG_T i = 0; i < _TC3_MAX_N_ASYNC_THREADS; i++) {
        PosixResult = pthread_create(&ASyncThreads[i], NULL, __TC3_AsyncPostThread, &ASyncPostStat[i]);
        ASSERT_EQ(0, PosixResult);
    }

    for (ULONG_T i = 0; i < _TC3_MAX_M_SYNC_THREADS; i++) {
        PosixResult = pthread_create(&SyncThreads[i], NULL, __TC3_SyncPostThread, &SyncPostStat[i]);
        ASSERT_EQ(0, PosixResult);
    }

    for (ULONG_T i = 0; i < _TC3_MAX_N_ASYNC_THREADS; i++) {
        PosixResult = pthread_join(ASyncThreads[i], NULL);
        ASSERT_EQ(0, PosixResult);
    }

    for (ULONG_T i = 0; i < _TC3_MAX_M_SYNC_THREADS; i++) {
        PosixResult = pthread_join(SyncThreads[i], NULL);
        ASSERT_EQ(0, PosixResult);
    }

    IOC_forceProcEVT();  // force all EvtDesc in IOC's EvtDescQueue to be processed.

    //===VERIFY===
    ULONG_T TotalASyncSuccessPostCnt = 0;
    ULONG_T TotalASyncNonBlockPostCnt = 0;
    for (ULONG_T i = 0; i < _TC3_MAX_N_ASYNC_THREADS; i++) {
        ASSERT_NE(0, ASyncPostStat[i].ASyncPostSuccessCnt) << "ASyncPostSuccessCnt is 0 in ASyncThread[" << i << "]";
        TotalASyncSuccessPostCnt += ASyncPostStat[i].ASyncPostSuccessCnt;

        ASSERT_NE(0, ASyncPostStat[i].ASyncPostNonBlockCnt) << "ASyncPostNonBlockCnt is 0 in ASyncThread[" << i << "]";
        TotalASyncNonBlockPostCnt += ASyncPostStat[i].ASyncPostNonBlockCnt;
    }

    ULONG_T TotalSyncSuccessPostCnt = 0;
    ULONG_T TotalSyncNonBlockPostCnt = 0;
    for (ULONG_T i = 0; i < _TC3_MAX_M_SYNC_THREADS; i++) {
        ASSERT_NE(0, SyncPostStat[i].SyncPostSuccessCnt) << "SyncPostSuccessCnt is 0 in SyncThread[" << i << "]";
        TotalSyncSuccessPostCnt += SyncPostStat[i].SyncPostSuccessCnt;

        ASSERT_NE(0, SyncPostStat[i].SyncPostNonBlockCnt) << "SyncPostNonBlockCnt is 0 in SyncThread[" << i << "]";
        TotalSyncNonBlockPostCnt += SyncPostStat[i].SyncPostNonBlockCnt;
    }

    ULONG_T TotalPostSuccessCnt = TotalASyncSuccessPostCnt + TotalSyncSuccessPostCnt;
    ASSERT_EQ(TotalPostSuccessCnt, TC3PrivData.CbKeepAliveCnt + TC3PrivData.CbSleep9USCnt);  // KeyVerifyPoint

    //===CLEANUP===
    IOC_UnsubEvtArgs_T UnsubArgs = {
        .CbProcEvt_F = __TC3_cbProcEvt,
        .pCbPrivData = &TC3PrivData,
    };
    Result = _IOC_unsubEVT_inConlesMode(&UnsubArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);
}