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
 *  reference defination of IOC_LinkState_T in IOC_Types.h,
 *  reference defination of IOC_getLinkState in IOC.h,
 *  reference considering UT design aspect in _UT_IOC_Common.h.
 * Design UTs to verify every State and SubState of Event in ConlesMode,
 *  by design combination behaviors of IOC_[sub/unsub/post]EVT.
 *
 * List of UTs in summary:
 *  - Case01_verifyLinkStateReadyIdle_byDoNothing
 *  - Case02_verifyLinkStateReadyIdleOrLocked_bySubUnsubEvtConcurrently
 *  - Case03_verifyLinkStateBusyProcing_byPostEVT_ofTestSleep99msEvt
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
  IOC_LinkState_T linkState       = IOC_LinkStateUndefined;
  IOC_LinkSubState_T linkSubState = IOC_LinkSubStateUndefined;

  IOC_Result_T result = IOC_getLinkState(IOC_CONLES_MODE_AUTO_LINK_ID, &linkState, &linkSubState);
  ASSERT_EQ(IOC_RESULT_SUCCESS, result);  // VerifyPoint

  //===VERIFY===
  ASSERT_EQ(IOC_LinkStateReady, linkState);          // KeyVerifyPoint
  ASSERT_EQ(IOC_LinkSubState_ReadyIdle, linkSubState);  // KeyVerifyPoint

  //===CLEANUP===
  // NOP
}

/**
 * @[Name]: Case02_verifyLinkStateReadyIdleOrLocked_bySubUnsubEvtConcurrently
 * @[Purpose]: By LinkState definition in README_ArchDesign::State::EVT::Conles and IOC_Types.h::IOC_LinkState_T,
 *    verify Link's main state is LinkStateReady and sub state is LinkStateReadyIdle or LinkStateReadyLocked,
 *    when subEVT+unsubEVT in multi threads currently.
 * @[Steps]:
 *    1. Create _Case02_MAX_THREAD_NUM threads with thread body named _Case02_subUnsubEvtThread as SETUP
 *      |-> each thread has a ThreadID argument from 1 to _Case02_MAX_THREAD_NUM
 *    2. In Each thread do subEVT+unsubEVT of _Case02_MAX_SUBUNSUB_CNT in loop as BEHAVIOR
 *      |-> call subEVT with SubEvtArgs set with CbProcEvt_F=pCbPrivData=ThreadID and EvtIDs=IOC_EVTID_TEST_KEEPALIVE
 *      |-> call unsubEVT with UnsubEvtArgs same with SubEvtArgs's CbProcEvt_F and pCbPrivData.
 *      |-> RefAPI: IOC_subEVT_inConlesMode, IOC_unsubEVT_inConlesMode in IOC.h
 *      |-> RefType: IOC_SubEvtArgs_T, IOC_UnsubEvtArgs_T in IOC_Types.h
 *    3. In main thread call IOC_getLinkState to get the LinkState and LinkSubState continuously as BEHAVIOR
 *      a)-> check LinkState is LinkStateReady and sub state is LinkStateReadyIdle or LinkStateReadyLocked as VERIFY
 *      |-> account the getting of LinkState in LinkStateCnt as BEHAVIOR
 *      |-> account the getting of LinkSubState in LinkSubStateIdleCnt or LinkSubStateLockedCnt as BEHAVIOR
 *    4. Verify the _Case02_LinkStateCnt>0 and _Case02_LinkSubStateCnt>0 as VERIFY
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

TEST(UT_ConlesEventState, Case02_verifyLinkStateReadyIdleOrLocked_bySubUnsubEvtConcurrently) {
  //===SETUP===
  std::thread threads[_Case02_MAX_THREAD_NUM];
  for (long i = 0; i < _Case02_MAX_THREAD_NUM; i++) {
    threads[i] = std::thread(_Case02_subUnsubEvtThread, i + 1);
  }

  //===BEHAVIOR===
  uint32_t LinkStateCnt          = 0;
  uint32_t LinkSubStateIdleCnt   = 0;
  uint32_t LinkSubStateLockedCnt = 0;
  uint32_t GetLinkStateCnt       = _Case02_MAX_SUBUNSUB_CNT * _Case02_MAX_THREAD_NUM;
  for (uint32_t i = 0; i < GetLinkStateCnt; i++) {
    IOC_LinkState_T linkState       = IOC_LinkStateUndefined;
    IOC_LinkSubState_T linkSubState = IOC_LinkSubStateUndefined;

    IOC_Result_T result = IOC_getLinkState(IOC_CONLES_MODE_AUTO_LINK_ID, &linkState, &linkSubState);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result);  // VerifyPoint
    // linkState MUST be IOC_LinkStateReady
    ASSERT_EQ(IOC_LinkStateReady, linkState);  // VerifyPoint
    // linkSubState MUST be IOC_LinkSubState_ReadyIdle or IOC_LinkSubState_ReadyLocked
    ASSERT_TRUE(linkSubState == IOC_LinkSubState_ReadyIdle || linkSubState == IOC_LinkSubState_ReadyLocked);  // VerifyPoint

    if (linkState == IOC_LinkStateReady) {
      LinkStateCnt++;
    }
    if (linkSubState == IOC_LinkSubState_ReadyIdle) {
      LinkSubStateIdleCnt++;
    }
    if (linkSubState == IOC_LinkSubState_ReadyLocked) {
      LinkSubStateLockedCnt++;
    }
  }

  //===VERIFY===
  ASSERT_GT(LinkStateCnt, 0);           // KeyVerifyPoint
  ASSERT_GT(LinkSubStateIdleCnt, 0);    // KeyVerifyPoint
  ASSERT_GT(LinkSubStateLockedCnt, 0);  // KeyVerifyPoint

  // GetLinkStateCnt MUST be equal to LinkStateCnt or (LinkSubStateIdleCnt + LinkSubStateLockedCnt)
  ASSERT_EQ(GetLinkStateCnt, LinkStateCnt);                                 // KeyVerifyPoint
  ASSERT_EQ(GetLinkStateCnt, LinkSubStateIdleCnt + LinkSubStateLockedCnt);  // KeyVerifyPoint

  //===CLEANUP===
  for (long i = 0; i < _Case02_MAX_THREAD_NUM; i++) {
    threads[i].join();
  }
}
