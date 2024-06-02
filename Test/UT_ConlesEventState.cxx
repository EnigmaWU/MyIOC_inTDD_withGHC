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
 * Based on the Concept of Conles and the state of EVT,
 *  by defination of IOC_LinkState_T in IOC_Types.h,
 *  by defination of IOC_getLinkState in IOC.h,
 *  by considering UT design aspect in _UT_IOC_Common.h.
 * Design UTs to verify every State and SubState of Event in ConlesMode,
 *  by design combination behaviors of IOC_[sub/unsub/post]EVT.
 *
 * List of UTs in summary:
 *  - Case01_verifyLinkStateReadyIdle_byDoNothing
 *
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
  ASSERT_EQ(IOC_LinkSubState_ReadyIdle, linkState);  // KeyVerifyPoint

  //===CLEANUP===
  // NOP
}