#include "_UT_IOC_Common.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Summary of ConlesEventMisuse
 *  1) verifyNoEvtConsumer_byNotSubEvtButPostEvtDirectly
 *  2) verifyNoEvtConsumer_byUnsubEvtWithFakeUnsubArgs
 *  3) verifyNoEvtConsumer_bySubEvtOnceThenUnsubEvtTwice
 *  4) verifyConflictEvtConsumer_bySubSameFakeEvtSubArgsTwice
 */

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * @[Name]: verifyNoEvtConsumer_byNotSubEvtButPostEvtDirectly
 * @[Purpose]: accord [SPECv2-z.1], verify the behavior of no event consumer when post event directly will return
 * NO_EvtConsumer.
 * @[Steps]:
 *   1. ObjA call postEVT(TEST_KEEPALIVE) directly.
 * @[Expect]: postEVT(TEST_KEEPALIVE) will return IOC_RESULT_NO_EvtConsumer.
 * @[Notes]:
 */
TEST(UT_ConlesEventMisuse, Case01_verifyNoEvtConsumer_byNotSubEvtButPostEvtDirectly) {
  //===SETUP===

  //===BEHAVIOR===
  IOC_EvtDesc_T ObjA_EvtDesc = {.EvtID = IOC_EVTID_TEST_KEEPALIVE};
  IOC_Result_T Result = IOC_postEVT_inConlesMode(&ObjA_EvtDesc, NULL);

  //===VERIFY===
  ASSERT_EQ(IOC_RESULT_NO_EvtConsumer, Result);  // KeyVerifyPoint

  //===CLEANUP===
}

/**
 * @[Name]: verifyNoEvtConsumer_byUnsubEvtWithFakeUnsubArgs
 * @[Purpose]: accord [SPECv2-z.1], verify the behavior of no event consumer then unsubEVT directly will return NO_EvtConsumer.
 * @[Steps]:
 *   1. ObjA call unsubEVT with FakeUnsubArgs directly.
 * @[Expect]: unsubEVT will return IOC_RESULT_NO_EvtConsumer.
 * @[Notes]:
 */
TEST(UT_ConlesEventMisuse, Case02_verifyNoEvtConsumer_byUnsubEvtWithFakeUnsubArgs) {
  //===SETUP===

  //===BEHAVIOR===
  IOC_UnsubEvtArgs_T ObjA_UnsubEvtArgs = {.CbProcEvt_F = NULL, .pCbPriv = NULL};
  IOC_Result_T Result = IOC_unsubEVT_inConlesMode(&ObjA_UnsubEvtArgs);

  //===VERIFY===
  ASSERT_EQ(IOC_RESULT_NO_EvtConsumer, Result);  // KeyVerifyPoint

  //===CLEANUP===
}

/**
 * @[Name]: verifyNoEvtConsumer_bySubEvtOnceThenUnsubEvtTwice
 * @[Purpose]: accord [SPECv2-z.1], verify the behavior of subEVT once then unsubEVT twice will return NO_EvtConsumer.
 * @[Steps]:
 *   1. ObjA call subEVT with FakeSubArgs once.
 *   2. ObjA call unsubEVT once.
 *   3. ObjA call unsubEVT again.
 * @[Expect]: unsubEVT will return IOC_RESULT_NO_EvtConsumer at the second time.
 * @[Notes]:
 */
TEST(UT_ConlesEventMisuse, Case03_verifyNoEvtConsumer_bySubEvtOnceThenUnsubEvtTwice) {
#define _Case03_FakeCbProcEvt_F ((IOC_CbProcEvt_F)0x20040301UL)
#define _Case03_FakeCbPrivData ((void *)0x20040302UL)
  //===SETUP===
  IOC_SubEvtArgs_T ObjA_SubEvtArgs = {
      .CbProcEvt_F = _Case03_FakeCbProcEvt_F, .pCbPrivData = _Case03_FakeCbPrivData, .EvtNum = 0, .pEvtIDs = NULL};
  IOC_Result_T Result = IOC_subEVT_inConlesMode(&ObjA_SubEvtArgs);
  ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // CheckPoint

  IOC_UnsubEvtArgs_T ObjA_UnsubEvtArgs = {.CbProcEvt_F = _Case03_FakeCbProcEvt_F, .pCbPriv = _Case03_FakeCbPrivData};
  Result = IOC_unsubEVT_inConlesMode(&ObjA_UnsubEvtArgs);
  ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // CheckPoint

  //===BEHAVIOR===
  Result = IOC_unsubEVT_inConlesMode(&ObjA_UnsubEvtArgs);

  //===VERIFY===
  ASSERT_EQ(IOC_RESULT_NO_EvtConsumer, Result);  // KeyVerifyPoint

  //===CLEANUP===
}

/**
 * @[Name]: verifyConflictEvtConsumer_bySubSameFakeEvtSubArgsTwice
 * @[Purpose]: accord [SPECv2-z.2], verify the behavior of conflict event consumer when sub same event twice will return
 * CONFLICT_EvtConsumer.
 * @[Steps]:
 *   1. ObjA call subEVT with FakeSubArgs once.
 *   2. ObjA call subEVT with FakeSubArgs again.
 * @[Expect]: subEVT will return IOC_RESULT_CONFLICT_EvtConsumer at the second time.
 * @[Notes]:
 */
TEST(UT_ConlesEventMisuse, Case04_verifyConflictEvtConsumer_bySubSameFakeEvtSubArgsTwice) {
#define _Case04_FakeCbProcEvt_F ((IOC_CbProcEvt_F)0x20240303UL)
#define _Case04_FakeCbPrivData ((void *)0x20240304UL)
  //===SETUP===
  IOC_SubEvtArgs_T ObjA_SubEvtArgs = {
      .CbProcEvt_F = _Case04_FakeCbProcEvt_F, .pCbPrivData = _Case04_FakeCbPrivData, .EvtNum = 0, .pEvtIDs = NULL};
  IOC_Result_T Result = IOC_subEVT_inConlesMode(&ObjA_SubEvtArgs);
  ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // CheckPoint

  //===BEHAVIOR===
  Result = IOC_subEVT_inConlesMode(&ObjA_SubEvtArgs);

  //===VERIFY===
  ASSERT_EQ(IOC_RESULT_CONFLICT_EvtConsumer, Result);  // KeyVerifyPoint

  //===CLEANUP===
  IOC_UnsubEvtArgs_T ObjA_UnsubEvtArgs = {.CbProcEvt_F = _Case04_FakeCbProcEvt_F, .pCbPriv = _Case04_FakeCbPrivData};
  Result = IOC_unsubEVT_inConlesMode(&ObjA_UnsubEvtArgs);
  ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // CheckPoint
}
