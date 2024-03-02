#include "_UT_IOC_Common.h"

/**
 * @[Name]: verifyNoEvtCosmer_byNotSubEvtButPostEvtDirectly
 * @[Purpose]: accord [SPECv2-z.1], verify the behavior of no event consumer when post event directly will return NO_EVTCOSMER.
 * @[Steps]:
 *   1. ObjA call postEVT(TEST_KEEPALIVE) directly.
 * @[Expect]: postEVT(TEST_KEEPALIVE) will return IOC_RESULT_NO_EVTCOSMER.
 * @[Notes]:
 */
TEST(UT_ConlesEventMisuse, Case01_verifyNoEvtCosmer_byNotSubEvtButPostEvtDirectly) {
  //===SETUP===

  //===BEHAVIOR===
  IOC_EvtDesc_T ObjA_EvtDesc = {.EvtID = IOC_EVTID_TEST_KEEPALIVE};
  IOC_Result_T Result = IOC_postEVT_inConlesMode(&ObjA_EvtDesc, NULL);

  //===VERIFY===
  ASSERT_EQ(IOC_RESULT_NO_EVTCOSMER, Result);  // KeyVerifyPoint

  //===CLEANUP===
}
