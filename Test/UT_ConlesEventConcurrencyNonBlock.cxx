#include <pthread.h>
#include <sys/_pthread/_pthread_attr_t.h>
#include <sys/_pthread/_pthread_mutex_t.h>

#include "_UT_IOC_Common.h"

/**
 * NonBlock here means EvtPrducer call postEVT WON'T wait for a moment IF:
 *  IOC's EvtDescQueue full in ASync.
 *    OR
 *  IOC's EvtDescQueue is not empty in Sync.
 *
 * RefDoc:
 *  1) README_UseCase.md
 *  2) UT_ConlesEventConcurrencyNonBlock.md
 */

//======BEGIN OF UNIT TESTING DESIGN===============================================================
/**
 * @brief 【User Story】
 *
 *  US-1: AS an EvtProducer calling IOC_postEVT_inConlesMode,
 *    I want to return immediately without waiting for a moment IF:
 *      AutoLink's internal EvtDescQueue in IOC is full in ASyncMode OR is not empty in SyncMode,
 *        SO THAT I can continue my work without blocking.
 *
 */

/**
 * @brief 【Acceptance Criteria】
 *
 * AC-1: GIVEN EvtProducer calling IOC_postEVT_inConlesMode,
 *         WHEN IOC's EvtDescQueue is full in ASyncMode,
 *         THEN EvtProducer can return immediately without waiting for a moment,
 *           AND the posting EvtDesc will never be processed by IOC.
 * AC-2: GIVEN EvtProducer calling IOC_postEVT_inConlesMode,
 *         WHEN IOC's EvtDescQueue is not empty in SyncMode,
 *         THEN EvtProducer can return immediately without waiting for a moment.
 *          AND the posting EvtDesc will never be processed by IOC.
 * AC-3: GIVEN EvtProducer calling IOC_postEVT_inConlesMode,
 *         WHEN IOC's EvtDescQueue is not full in ASyncMode,
 *         THEN EvtProducer can return immediately without waiting for a moment,
 *           AND the posting EvtDesc will be processed by IOC.
 * AC-4: GIVEN EvtProducer calling IOC_postEVT_inConlesMode,
 *         WHEN IOC's EvtDescQueue is empty in SyncMode,
 *         THEN EvtProducer can return immediately without waiting for a moment,
 *           AND the posting EvtDesc will be processed by IOC.
 *
 */

/**
 * @brief 【Test Cases】
 *
 * TC-1: verifyASyncNonblock_byPostOneMoreEVT_whenEvtDescQueueFull
 * TC-2: verifySyncNonblock_byPostOneMoreEVT_whenEvtDescQueueNotEmpty
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
 *      3.1) wait for __TC1_cbProcEvt to be called and block it.
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
  pthread_mutex_t FirstCbEnterMutex;

  // Main lockIT, ..., Cb lockIT, ...untile..., Main lastly postEVT+unlockIT
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
  IOC_Result_T Result = IOC_getCapabilty(&CapDesc);
  ASSERT_EQ(IOC_RESULT_SUCCESS, Result);

  ULONG_T QUEUE_DEPTH = CapDesc.ConlesModeEvent.DepthEvtDescQueue;
  ASSERT_GT(QUEUE_DEPTH, 1);

  //---------------------------------------------------------------------------
  _TC1_PrivData_T TC1PrivData = {
      .KeepAliveCnt = 0,
  };
  IOC_EvtID_T SubEvtIDs[] = {
      IOC_EVTID_TEST_KEEPALIVE,
  };
  IOC_SubEvtArgs_T SubArgs = {
      .CbProcEvt_F = __TC1_cbProcEvt,
      .pCbPrivData = &TC1PrivData,
      .EvtNum      = IOC_calcArrayElmtCnt(SubEvtIDs),
      .pEvtIDs     = SubEvtIDs,
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
  for (ULONG_T i = 0; i < QUEUE_DEPTH - 1; i++) {
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
  ASSERT_EQ(QUEUE_DEPTH, TC1PrivData.KeepAliveCnt);

  IOC_UnsubEvtArgs_T UnsubArgs = {
      .CbProcEvt_F = __TC1_cbProcEvt,
      .pCbPrivData = &TC1PrivData,
  };
  Result = _IOC_unsubEVT_inConlesMode(&UnsubArgs);
  ASSERT_EQ(IOC_RESULT_SUCCESS, Result);
}