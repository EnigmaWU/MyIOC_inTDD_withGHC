/**
 * @file UT_ConlesEventState.cxx
 * @brief This file contains the UTs to verify State of Event in ConlesMode.
 * @RefMore:
 *     README_ArchDesign @ PRJROOT
 *         |-> Concept
 *             |-> Conet vs Conles
 *             |-> MSG::EVT
 *         |-> State
 *             |-> EVT::Conles
 */

/**
 * @section DesignOfUT ConlesEventState
 * Based on the Concept of Conles and the state of EVT in ConlesMode,
 *  refMore: section 'Category-A' in README_UseCases.md,
 *  refMore: section 'Concept' and 'State' in README_ArchDesign.md,
 *  refMore: IOC_LinkState_T defined in IOC_Types.h,
 *  refMore: IOC_getLinkState and IOC[sub/unsub/post]EVT defined in IOC.h.
 * Design UTs to verify every State and SubState of Event in ConlesMode,
 *  from designed behaviors of IOC_[sub/unsub/post]EVT_inConlesMode,
 *  and by IOC_getLinkState's result as verify.
 *---------------------------------------------------------------------------------------------------------------------
 *===> Begin DesignOfUT of Acceptace Creteria(a.k.a AC) <===
 *
 *===> End DesignOfUT <===
 *---------------------------------------------------------------------------------------------------------------------
 *===> Begin DesignOfTestCase accordint to ACs <===
 *  - Case01_verifyLinkStateReadyIdle_byDoNothing
 *  - Case02_verifyLinkStateBusy_bySubUnsubEvtConcurrently
 *  - Case03_verifyLinkStateBusyCbProcEvt_bySleepWhenCbProcEvt
 *  - Case04_verifyUnsubEvtMayBlock_bySleepWhenCbProcEvt
 *  - Case05_verifySubEvtMayBlock_bySleepWhenCbProcEvt
 *===> End DesignOfTestCase <===
 */

#include "_UT_IOC_Common.h"
/**
 * @section ImplOfUT ConlesEventStateReady
 * @RefTemplate: UT_FreelyDrafts.cxx
 */

/**
 * @[Name]: Case01_verifyLinkStateReadyIdle_byDoNothing
 * @[Purpose]: By LinkState definition in README_ArchDesign::State::EVT::Conles and IOC_Types.h::IOC_LinkState_T,
 *    verify Link's main state is LinkStateReady and sub state is LinkStateReadyIdle upon _initCRuntimeSuccess.
 * @[Steps]:
 *    1. Call IOC_getLinkState to get the LinkState and LinkSubState as BEHAVIOR
 *    2. Verify the LinkState is LinkStateReady and sub state is LinkStateReadyIdle as VERIFY
 * @[Expect]: Step-2 is TRUE.
 * @[Notes]: _initCRuntimeSuccess is system initialize automatically, which means byDoNothing.
 */
TEST(UT_ConlesEventState, Case01_verifyLinkStateReadyIdle_byDoNothing) {
  //===SETUP===
  // NOP

  //===BEHAVIOR===
  IOC_LinkState_T linkState = IOC_LinkStateUndefined;

  IOC_Result_T result = IOC_getLinkState(IOC_CONLES_MODE_AUTO_LINK_ID, &linkState, NULL);
  ASSERT_EQ(IOC_RESULT_SUCCESS, result);  // VerifyPoint

  //===VERIFY===
  ASSERT_EQ(IOC_LinkStateReady, linkState);  // KeyVerifyPoint

  //===CLEANUP===
  // NOP
}

/**
 * @[Name]: Case02_verifyLinkStateBusySubEvtOrUnsubEvt_bySubUnsubEvtConcurrently
 * @[Purpose]: By LinkState definition in README_ArchDesign::State::EVT::Conles and IOC_Types.h::IOC_LinkState_T,
 *    verify Link's main state is LinkStateBusySubEvt when call subEVT and LinkStateBusyUnsubEvt when call unsubEVT.
 *    Here call subEVT and unsubEVT concurrently to verify its LinkState correctness.
 * @[Steps]:
 *    1. Create _Case02_MAX_THREAD_NUM threads with thread body named _Case02_subUnsubEvtThread as SETUP
 *      |-> each thread has a ThreadID argument from 1 to _Case02_MAX_THREAD_NUM
 *    2. In Each thread do subEVT+unsubEVT of _Case02_MAX_SUBUNSUB_CNT in loop as BEHAVIOR
 *      |-> call subEVT with SubEvtArgs set with CbProcEvt_F=pCbPrivData=ThreadID and EvtIDs=IOC_EVTID_TEST_KEEPALIVE
 *      |-> call unsubEVT with UnsubEvtArgs same with SubEvtArgs's CbProcEvt_F and pCbPrivData.
 *      |-> RefAPI: IOC_subEVT_inConlesMode, IOC_unsubEVT_inConlesMode in IOC.h
 *      |-> RefType: IOC_SubEvtArgs_T, IOC_UnsubEvtArgs_T in IOC_Types.h
 *    3. In main thread call IOC_getLinkState to get the LinkState and LinkSubState continuously as BEHAVIOR
 *      a)-> check LinkState is LinkStateBusySubEvt or LinkStateBusyUnsubEvt as VERIFY
 *      |-> account the getting of LinkState in LinkStateBusySubEvtCnt/UnsubEvtCnt as BEHAVIOR
 *    4. Verify the LinkStateBusySubEvtCnt/UnsubEvtCnt MUST >0
 *      |-> LinkStateReadyCnt MAY be 0 or >0
 * @[Expect]: Step-3.a is TRUE, Step-4 is TRUE.
 * @[Notes]:
 *      RefCode: UT_ConlesEventTypical.Case01-07
 */

#define _Case02_MAX_THREAD_NUM 10
#define _Case02_MAX_SUBUNSUB_CNT 1000000

static void _Case02_subUnsubEvtThread(long ThreadID) {
  for (uint32_t i = 0; i < _Case02_MAX_SUBUNSUB_CNT; i++) {
      IOC_EvtID_T EvtIDs[] = {IOC_EVTID_TEST_KEEPALIVE};

      IOC_SubEvtArgs_T subEvtArgs = {
          .CbProcEvt_F = (IOC_CbProcEvt_F)ThreadID,
          .pCbPrivData = (void *)ThreadID,
          .EvtNum      = IOC_calcArrayElmtCnt(EvtIDs),
          .pEvtIDs     = EvtIDs,
      };

      IOC_Result_T result = IOC_subEVT_inConlesMode(&subEvtArgs);
      ASSERT_EQ(IOC_RESULT_SUCCESS, result);  // VerifyPoint

      IOC_UnsubEvtArgs_T unsubEvtArgs = {
          .CbProcEvt_F = (IOC_CbProcEvt_F)ThreadID,
          .pCbPrivData = (void *)ThreadID,
      };
      result = IOC_unsubEVT_inConlesMode(&unsubEvtArgs);
      ASSERT_EQ(IOC_RESULT_SUCCESS, result);  // VerifyPoint
  }
}

TEST(UT_ConlesEventState, Case02_verifyLinkStateBusySubEvtOrUnsubEvt_bySubUnsubEvtConcurrently) {
  //===SETUP===
  std::thread threads[_Case02_MAX_THREAD_NUM];
  for (long i = 0; i < _Case02_MAX_THREAD_NUM; i++) {
    threads[i] = std::thread(_Case02_subUnsubEvtThread, i + 1);
  }

  //===BEHAVIOR===
  uint32_t LinkStateReadyCnt        = 0;
  uint32_t LinkStateBusySubEvtCnt   = 0;
  uint32_t LinkStateBusyUnsubEvtCnt = 0;

  uint32_t GetLinkStateCnt       = _Case02_MAX_SUBUNSUB_CNT * _Case02_MAX_THREAD_NUM;
  for (uint32_t i = 0; i < GetLinkStateCnt; i++) {
    IOC_LinkState_T linkState = IOC_LinkStateUndefined;

    IOC_Result_T result = IOC_getLinkState(IOC_CONLES_MODE_AUTO_LINK_ID, &linkState, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result);  // VerifyPoint
    // linkState MUST be IOC_LinkStateReady

    if (linkState == IOC_LinkStateReady) {
      LinkStateReadyCnt++;
    } else if (linkState == IOC_LinkStateBusySubEvt) {
      LinkStateBusySubEvtCnt++;
    } else if (linkState == IOC_LinkStateBusyUnsubEvt) {
      LinkStateBusyUnsubEvtCnt++;
    }
  }

  //===VERIFY===
  // LinkStateBusySubEvtCnt MUST be greater than 0
  ASSERT_GT(LinkStateBusySubEvtCnt, 0);  // KeyVerifyPoint
  // LinkStateBusyUnsubEvtCnt MUST be greater than 0
  ASSERT_GT(LinkStateBusyUnsubEvtCnt, 0);  // KeyVerifyPoint
  // LinkStateReadyCnt MAY be 0 or greater than 0
  ASSERT_GE(LinkStateReadyCnt, 0);  // KeyVerifyPoint

  //===CLEANUP===
  for (long i = 0; i < _Case02_MAX_THREAD_NUM; i++) {
    threads[i].join();
  }
}

/**
 * @[Name]: Case03_verifyLinkStateBusyCbProcEvt_bySleepWhenCbProcEvt
 * @[Purpose]: According to LinkState definition in README_ArchDesign::State::EVT::Conles
 *      and IOC_LinkState_T/IOC_LinkSubState_T in IOC_Types.h,
 *    verify Link's main state is LinkStateBusyCbProcEvt
 *      by postEVT of TestSleep99msEvt and sync state checking via CbProcEvt_F.
 * @[Steps]:
 *    1. subEVT as SETUP
 *      |-> CbProcEvt_F named _Case03_CbProcEvt_F_TestSleep99msEvt
 *      |-> EvtID is IOC_EVTID_TEST_SLEEP_99MS
 *      |-> RefAPI: IOC_subEVT_inConlesMode in IOC.h
 *      |-> RefType: IOC_SubEvtArgs_T in IOC_Types.h
 *      |-> RefType: IOC_CbProcEvt_F in IOC_Types.h
 *      a) Call IOC_getLinkState to get the LinkState
 *           and make sure LinkState is LinkStateReady as VERIFY
 *    2. postEVT of TestSleep99msEvt as BEHAVIOR
 *      |-> RefAPI: IOC_postEVT_inConlesMode in IOC.h
 *      |-> RefType: IOC_EvtDesc_T in IOC_Types.h
 *    3. Wait SemEnterCbProcEvt as BEHAVIOR
 *      |-> Post SemEnterCbProcEvt from CbProcEvt_F
 *    4. Call IOC_getLinkState to get the LinkState as BEHAVIOR
 *    5. Verify the LinkState is LinkStateBusyCbProcEvt as VERIFY
 *    6. Post SemLeaveCbProcEvt to CbProcEvt_F as BEHAVIOR
 *      |-> Waiting SemLeaveCbProcEvt in CbProcEvt_F after Step-3
 *    7. Sleep 100ms to assume CbProcEvt_F is return as BEHAVIOR
 *    8. unSubEVT as CLEANUP
 *      a) Call IOC_getLinkState to get the LinkState
 *           and make sure LinkState is LinkStateReady as VERIFY
 * @[Expect]:
 *    Step-1.a is TRUE, Step-5 is TRUE, Step-8.a is TRUE.
 * @[Notes]:
 *      RefCode:
 */

typedef struct {
  sem_t *pEnterCbProcEvtSem;
  sem_t *pLeaveCbProcEvtSem;
} _Case03_PrivData_T;

static IOC_Result_T _Case03_CbProcEvt_F_TestSleep99msEvt(const IOC_EvtDesc_pT pEvtDesc, void *pCbPrivData) {
  _Case03_PrivData_T *pPrivData = (_Case03_PrivData_T *)pCbPrivData;
  sem_post(pPrivData->pEnterCbProcEvtSem);

  // Sleep 99ms to simulate the processing of TestSleep99msEvt
  usleep(99 * 1000);

  sem_wait(pPrivData->pLeaveCbProcEvtSem);
  return IOC_RESULT_SUCCESS;
}

TEST(UT_ConlesEventState, Case03_verifyLinkStateBusyCbProcEvt_bySleepWhenCbProcEvt) {
  //===SETUP===
  _Case03_PrivData_T PrivData = {};

  sem_unlink("/EnterCbProcEvtSem");
  PrivData.pEnterCbProcEvtSem = sem_open("/EnterCbProcEvtSem", O_CREAT | O_EXCL, 0644, 0);
  ASSERT_NE(SEM_FAILED, PrivData.pEnterCbProcEvtSem)  // VerifyPoint
      << "errno=" << errno << ", " << strerror(errno);

  sem_unlink("/LeaveCbProcEvtSem");
  PrivData.pLeaveCbProcEvtSem = sem_open("/LeaveCbProcEvtSem", O_CREAT | O_EXCL, 0644, 0);
  ASSERT_NE(SEM_FAILED, PrivData.pLeaveCbProcEvtSem)  // VerifyPoint
      << "errno=" << errno << ", " << strerror(errno);

  IOC_EvtID_T EvtIDs[] = {IOC_EVTID_TEST_SLEEP_99MS};

  IOC_SubEvtArgs_T subEvtArgs = {
      .CbProcEvt_F = _Case03_CbProcEvt_F_TestSleep99msEvt,
      .pCbPrivData = &PrivData,
      .EvtNum      = IOC_calcArrayElmtCnt(EvtIDs),
      .pEvtIDs     = EvtIDs,
  };
  IOC_Result_T Result = IOC_subEVT_inConlesMode(&subEvtArgs);
  ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

  //===BEHAVIOR===
  // Step-1.a
  IOC_LinkState_T LinkState       = IOC_LinkStateUndefined;
  Result                          = IOC_getLinkState(IOC_CONLES_MODE_AUTO_LINK_ID, &LinkState, NULL);
  ASSERT_EQ(IOC_RESULT_SUCCESS, Result);                // KeyVerifyPoint
  ASSERT_EQ(IOC_LinkStateReady, LinkState);             // KeyVerifyPoint

  // Step-2
  IOC_EvtDesc_T EvtDesc = {.EvtID = IOC_EVTID_TEST_SLEEP_99MS};
  Result                = IOC_postEVT_inConlesMode(&EvtDesc, NULL);
  ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

  // Step-3
  int RetPSX = sem_wait(PrivData.pEnterCbProcEvtSem);
  ASSERT_EQ(0, RetPSX)  // VerifyPoint
      << "errno=" << errno << ", " << strerror(errno);

  // Step-4
  Result = IOC_getLinkState(IOC_CONLES_MODE_AUTO_LINK_ID, &LinkState, NULL);
  ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

  //===VERIFY===
  // Step-5
  ASSERT_EQ(IOC_LinkStateBusyCbProcEvt, LinkState);  // KeyVerifyPoint

  // Step-6
  sem_post(PrivData.pLeaveCbProcEvtSem);

  // Step-7
  usleep(200 * 1000);

  //===CLEANUP===
  // Step-8.a
  Result = IOC_getLinkState(IOC_CONLES_MODE_AUTO_LINK_ID, &LinkState, NULL);
  ASSERT_EQ(IOC_RESULT_SUCCESS, Result);                // KeyVerifyPoint
  ASSERT_EQ(IOC_LinkStateReady, LinkState);             // KeyVerifyPoint

  IOC_UnsubEvtArgs_T unsubEvtArgs = {
      .CbProcEvt_F = _Case03_CbProcEvt_F_TestSleep99msEvt,
      .pCbPrivData = &PrivData,
  };
  Result = IOC_unsubEVT_inConlesMode(&unsubEvtArgs);
  ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

  sem_close(PrivData.pEnterCbProcEvtSem);
  sem_close(PrivData.pLeaveCbProcEvtSem);
  sem_unlink("/EnterCbProcEvtSem");
  sem_unlink("/LeaveCbProcEvtSem");
}

/**
 * @[Name]: Case04_verifyUnsubEvtMayBlock_bySleepWhenCbProcEvt
 * @[Purpose]: According to LinkState definition in README_ArchDesign::State::EVT::Conles,
 *    ONLY Link's main state is Ready, unsubEVT may be accpeted by IOC.
 *    SO GIVEN Link is in Busy State,
 *       WHEN call unsubEVT of that Link,
 *       THEN unsubEVT may be blocked.
 * @[Steps]: RefFlow in UT_ConlesEventState.md::FlowChat::Case04
 *    1. EvtConsumer call subEVT as SETUP
 *        |-> subEvtArgs(_Case04_CbProcEvt_F_TestSleep99msEvt) with dynamic allocated private data
 *        a)-> getLinkState to make sure LinkStateReady as a small VERIFY
 *    2. postEVT of TestSleep99msEvt as BEHAVIOR
 *        a)-> wait CbProcEvt_F to be called via EnterCbProcEvtSem in private data
 *              |-> in CbProcEvt_F, ONLY process event TestSleep99ms, simusleep 99ms, then ++Sleep99msEvtCnt
 *        b)-> getLinkState to make sure LinkStateBusyCbProcEvt
 *    3. unsubEVT EvtConsumer as BEHAVIOR
 *        |-> save begin&end time of calling unsubEVT in private data
 *    4. Calculate the time consumption, its delta time MUST be greater than 99ms as VERIFY
 *        |-> Sleep99msEvtCnt in private data MUST be 1 as VERIFY
 *        |-> free the private data immediately after this step as CLEANUP
 * @[Expect]:
 *    Step-4 is TRUE.
 * @[Notes]:
 *    RefCode: TEST(UT_ConlesEventState, Case03_verifyLinkStateBusyCbProcEvt_bySleepWhenCbProcEvt)
 */

typedef struct {
  sem_t *pEnterCbProcEvtSem;
  struct timeval BeginTime, EndTime;  // for unsubEVT time consumption
  uint32_t Sleep99msEvtCnt;
} _Case04_PrivData_T, *_Case04_PrivData_pT;

static IOC_Result_T _Case04_CbProcEvt_F_TestSleep99msEvt(const IOC_EvtDesc_pT pEvtDesc, void *pCbPrivData) {
  _Case04_PrivData_pT pPrivData = (_Case04_PrivData_pT)pCbPrivData;

  switch (pEvtDesc->EvtID) {
    case IOC_EVTID_TEST_SLEEP_99MS:
      sem_post(pPrivData->pEnterCbProcEvtSem);
      // Sleep 99ms to simulate the processing of TestSleep99msEvt
      usleep(99 * 1000);
      pPrivData->Sleep99msEvtCnt++;
      break;
    default:
      EXPECT_FALSE(1) << "Unexpected EvtID=" << pEvtDesc->EvtID;
  }

  return IOC_RESULT_SUCCESS;
}

TEST(UT_ConlesEventState, Case04_verifyUnsubEvtMayBlock_bySleepWhenCbProcEvt) {
  //===SETUP===
  _Case04_PrivData_pT pPrivData = (_Case04_PrivData_pT)malloc(sizeof(_Case04_PrivData_T));
  ASSERT_NE(nullptr, pPrivData);  // VerifyPoint
  // Init private data
  pPrivData->Sleep99msEvtCnt = 0;

  sem_unlink("/EnterCbProcEvtSem");
  pPrivData->pEnterCbProcEvtSem = sem_open("/EnterCbProcEvtSem", O_CREAT | O_EXCL, 0644, 0);
  ASSERT_NE(SEM_FAILED, pPrivData->pEnterCbProcEvtSem)  // VerifyPoint
      << "errno=" << errno << ", " << strerror(errno);

  IOC_EvtID_T EvtIDs[] = {
      IOC_EVTID_TEST_SLEEP_99MS,
  };
  IOC_SubEvtArgs_T SubEvtArgs = {
      .CbProcEvt_F = _Case04_CbProcEvt_F_TestSleep99msEvt,
      .pCbPrivData = pPrivData,
      .EvtNum      = IOC_calcArrayElmtCnt(EvtIDs),
      .pEvtIDs     = EvtIDs,
  };
  IOC_Result_T Result = IOC_subEVT_inConlesMode(&SubEvtArgs);
  ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

  // Step-1.a: getLinkState to make sure LinkStateReady as a small VERIFY
  IOC_LinkState_T LinkState = IOC_LinkStateUndefined;
  Result                    = IOC_getLinkState(IOC_CONLES_MODE_AUTO_LINK_ID, &LinkState, NULL);
  ASSERT_EQ(IOC_RESULT_SUCCESS, Result);                // VerifyPoint
  ASSERT_EQ(IOC_LinkStateReady, LinkState);             // KeyVerifyPoint

  //===BEHAVIOR===
  // Step-2: postEVT of TestSleep99msEvt to simulate the processing of TestSleep99msEvt
  IOC_EvtDesc_T EvtDesc = {
      .EvtID = IOC_EVTID_TEST_SLEEP_99MS,
  };
  Result = IOC_postEVT_inConlesMode(&EvtDesc, NULL);
  ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

  // Step-2.a: wait CbProcEvt_F to be called via EnterCbProcEvtSem in private data
  int RetPSX = sem_wait(pPrivData->pEnterCbProcEvtSem);
  ASSERT_EQ(0, RetPSX)  // VerifyPoint
      << "errno=" << errno << ", " << strerror(errno);

  // Step-2.b: getLinkState to make sure LinkStateBusyCbProcEvt
  Result = IOC_getLinkState(IOC_CONLES_MODE_AUTO_LINK_ID, &LinkState, NULL);
  ASSERT_EQ(IOC_RESULT_SUCCESS, Result);                  // VerifyPoint
  ASSERT_EQ(IOC_LinkStateBusyCbProcEvt, LinkState);       // KeyVerifyPoint

  // Step-3: unsubEVT EvtConsumer which may be blocked as BEHAVIOR
  IOC_UnsubEvtArgs_T UnsubEvtArgs = {
      .CbProcEvt_F = _Case04_CbProcEvt_F_TestSleep99msEvt,
      .pCbPrivData = pPrivData,
  };
  gettimeofday(&pPrivData->BeginTime, NULL);
  Result = IOC_unsubEVT_inConlesMode(&UnsubEvtArgs);
  ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint
  gettimeofday(&pPrivData->EndTime, NULL);

  //===VERIFY===
  // Step-4: Calculate the time consumption, its delta time MUST be >=99ms as VERIFY to indicate unsubEVT is blocked
  uint32_t TimeConsumption = IOC_deltaTimevalInMS(&pPrivData->BeginTime, &pPrivData->EndTime);
  ASSERT_GT(TimeConsumption, 99 - 1)  // KeyVerifyPoint
      << "TimeConsumption=" << TimeConsumption << "ms, Sleep99msEvtCnt=" << pPrivData->Sleep99msEvtCnt;

  ASSERT_EQ(1, pPrivData->Sleep99msEvtCnt);  // KeyVerifyPoint

  //===CLEANUP===
  sem_close(pPrivData->pEnterCbProcEvtSem);
  sem_unlink("/EnterCbProcEvtSem");
  free(pPrivData);
}

/**
 * @[Name]: Case05_verifySubEvtMayBlock_bySleepWhenCbProcEvt
 * @[Purpose]: According to LinkState definition in README_ArchDesign::State::EVT::Conles,
 *    ONLY Link's main state is Ready, subEVT may be accpeted by IOC.
 *    SO GIVEN Link is in Busy State,
 *       WHEN call subEVT of that Link,
 *       THEN subEVT may be blocked.
 * @[Steps]:
 *    0) RefSteps in Case04, Except we have 2xEvtConsumer named No1 and No2
 *        |-> No1 is same with Case04's EvtConsumer
 *        |-> No2 is a new EvtConsumer do subEVT of KeepAliveEvt which may be blocked
 *    X) No1 call subEVT(TestSleep99msEvt) as SETUP
 *        a) No2 waiting for No1 enter into process TestSleep99msEvt
 *        b) getLinkState to make sure LinkStateBusyCbProcEvt as a small VERIFY
 *    Y) No2 call subEVT(KeeaAlive) which may be blocked as VERIFY
 *        |-> because No1 is processing TestSleep99msEvt
 *        a) calculate the time consumption of subEVT(KeeaAlive) as VERIFY
 *    Z) No1 and No2 call unsubEVT as CLEANUP
 * @[Expect]:
 *    Case04's Step-4, corresponding to Case05's Step-Y-a is TRUE.
 * @[Notes]:
 *    RefCode: TEST(UT_ConlesEventState, Case04_verifyUnsubEvtMayBlock_bySleepWhenCbProcEvt)
 *    RefFlow: UT_ConlesEventState.md::FlowChat::Case05
 */

typedef struct {
  sem_t *pEnterCbProcEvtSem;
} _Case05_No1PrivData_T, *_Case05_No1PrivData_pT;

typedef struct {
  struct timeval BeginTime, EndTime;  // for subEVT time consumption
  uint32_t KeepAliveEvtCnt;           // for verifying KeepAliveEvtCnt==0, because no postEVT of KeepAlive
} _Case05_No2PrivData_T, *_Case05_No2PrivData_pT;

static IOC_Result_T _Case05_No1CbProcEvt_F_TestSleep99msEvt(const IOC_EvtDesc_pT pEvtDesc, void *pCbPrivData) {
  _Case05_No1PrivData_pT pPrivData = (_Case05_No1PrivData_pT)pCbPrivData;

  switch (pEvtDesc->EvtID) {
    case IOC_EVTID_TEST_SLEEP_99MS:
      sem_post(pPrivData->pEnterCbProcEvtSem);
      // Sleep 99ms to simulate the processing of TestSleep99msEvt
      usleep(99 * 1000);
      break;
    default:
      EXPECT_FALSE(1) << "Unexpected EvtID=" << pEvtDesc->EvtID;
  }

  return IOC_RESULT_SUCCESS;
}
static IOC_Result_T _Case05_No2CbProcEvt_F_KeepAlive(const IOC_EvtDesc_pT pEvtDesc, void *pCbPrivData) {
  // Get a pointer to the private data for this event
  _Case05_No2PrivData_pT pPrivData = (_Case05_No2PrivData_pT)pCbPrivData;

  switch (pEvtDesc->EvtID) {
    case IOC_EVTID_TEST_KEEPALIVE:
      // Increment the KeepAliveEvtCnt each time this event is received
      pPrivData->KeepAliveEvtCnt++;
      break;
    default:
      EXPECT_FALSE(1) << "Unexpected EvtID=" << pEvtDesc->EvtID;
  }

  return IOC_RESULT_SUCCESS;
}

TEST(UT_ConlesEventState, Case05_verifySubEvtMayBlock_bySleepWhenCbProcEvt) {
  //===SETUP===
  _Case05_No1PrivData_pT pNo1PrivData = (_Case05_No1PrivData_pT)malloc(sizeof(_Case05_No1PrivData_T));
  ASSERT_NE(nullptr, pNo1PrivData);  // VerifyPoint
  // Init private data
  sem_unlink("/EnterCbProcEvtSem");
  pNo1PrivData->pEnterCbProcEvtSem = sem_open("/EnterCbProcEvtSem", O_CREAT | O_EXCL, 0644, 0);
  ASSERT_NE(SEM_FAILED, pNo1PrivData->pEnterCbProcEvtSem)  // VerifyPoint
      << "errno=" << errno << ", " << strerror(errno);

  _Case05_No2PrivData_pT pNo2PrivData = (_Case05_No2PrivData_pT)malloc(sizeof(_Case05_No2PrivData_T));
  ASSERT_NE(nullptr, pNo2PrivData);  // VerifyPoint
  // Init private data
  pNo2PrivData->KeepAliveEvtCnt = 0;

  IOC_EvtID_T No1EvtIDs[] = {
      IOC_EVTID_TEST_SLEEP_99MS,
  };
  IOC_SubEvtArgs_T No1SubEvtArgs = {
      .CbProcEvt_F = _Case05_No1CbProcEvt_F_TestSleep99msEvt,
      .pCbPrivData = pNo1PrivData,
      .EvtNum      = IOC_calcArrayElmtCnt(No1EvtIDs),
      .pEvtIDs     = No1EvtIDs,
  };
  IOC_Result_T Result = IOC_subEVT_inConlesMode(&No1SubEvtArgs);
  ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

  // Step-1.a: getLinkState to make sure LinkStateReady as a small VERIFY
  IOC_LinkState_T LinkState = IOC_LinkStateUndefined;
  Result                    = IOC_getLinkState(IOC_CONLES_MODE_AUTO_LINK_ID, &LinkState, NULL);
  ASSERT_EQ(IOC_RESULT_SUCCESS, Result);     // VerifyPoint
  ASSERT_EQ(IOC_LinkStateReady, LinkState);  // KeyVerifyPoint

  //===BEHAVIOR===
  // Step-2: postEVT of TestSleep99msEvt to simulate the processing of TestSleep99msEvt
  IOC_EvtDesc_T No1EvtDesc = {
      .EvtID = IOC_EVTID_TEST_SLEEP_99MS,
  };
  Result = IOC_postEVT_inConlesMode(&No1EvtDesc, NULL);
  ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

  // Step-2.a: wait CbProcEvt_F to be called via EnterCbProcEvtSem in private data
  int RetPSX = sem_wait(pNo1PrivData->pEnterCbProcEvtSem);
  ASSERT_EQ(0, RetPSX)  // VerifyPoint
      << "errno=" << errno << ", " << strerror(errno);

  // Step-2.b: getLinkState to make sure LinkStateBusyCbProcEvt
  Result = IOC_getLinkState(IOC_CONLES_MODE_AUTO_LINK_ID, &LinkState, NULL);
  ASSERT_EQ(IOC_RESULT_SUCCESS, Result);                  // VerifyPoint
  ASSERT_EQ(IOC_LinkStateBusyCbProcEvt, LinkState);       // KeyVerifyPoint

  // Step-3: subEVT of KeepAliveEvt which may be blocked as BEHAVIOR
  IOC_EvtID_T No2EvtIDs[] = {
      IOC_EVTID_TEST_KEEPALIVE,
  };
  IOC_SubEvtArgs_T No2SubEvtArgs = {
      .CbProcEvt_F = _Case05_No2CbProcEvt_F_KeepAlive,
      .pCbPrivData = pNo2PrivData,
      .EvtNum      = IOC_calcArrayElmtCnt(No2EvtIDs),
      .pEvtIDs     = No2EvtIDs,
  };
  gettimeofday(&pNo2PrivData->BeginTime, NULL);
  Result = IOC_subEVT_inConlesMode(&No2SubEvtArgs);
  gettimeofday(&pNo2PrivData->EndTime, NULL);
  ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

  //===VERIFY===
  // Step-4: Calculate the time consumption, its delta time MUST be >=99ms as VERIFY to indicate subEVT is blocked
  uint32_t TimeConsumption = IOC_deltaTimevalInMS(&pNo2PrivData->BeginTime, &pNo2PrivData->EndTime);
  ASSERT_GT(TimeConsumption, 99 - 1)  // KeyVerifyPoint
      << "TimeConsumption=" << TimeConsumption << "ms, KeepAliveEvtCnt=" << pNo2PrivData->KeepAliveEvtCnt;

  ASSERT_EQ(0, pNo2PrivData->KeepAliveEvtCnt);  // KeyVerifyPoint

  //===CLEANUP===
  // Step-5: unsubEVT EvtConsumer which may be blocked as CLEANUP
  IOC_UnsubEvtArgs_T No1UnsubEvtArgs = {
      .CbProcEvt_F = _Case05_No1CbProcEvt_F_TestSleep99msEvt,
      .pCbPrivData = pNo1PrivData,
  };
  Result = IOC_unsubEVT_inConlesMode(&No1UnsubEvtArgs);
  ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

  IOC_UnsubEvtArgs_T No2UnsubEvtArgs = {
      .CbProcEvt_F = _Case05_No2CbProcEvt_F_KeepAlive,
      .pCbPrivData = pNo2PrivData,
  };
  Result = IOC_unsubEVT_inConlesMode(&No2UnsubEvtArgs);
  ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

  sem_close(pNo1PrivData->pEnterCbProcEvtSem);
  sem_unlink("/EnterCbProcEvtSem");
  free(pNo1PrivData);
  free(pNo2PrivData);
}
