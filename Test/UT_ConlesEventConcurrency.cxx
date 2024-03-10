#include "_UT_IOC_Common.h"
//===>RefMore: TEMPLATE OF UT CASE in UT_FreelyDrafts.cxx
//===>RefMore: ConsoleEventTypical UT_ConlesEventTypical.cxx
//===>RefMore: ConsoleEventCapabilty UT_ConlesEventCapabilty.cxx
//===>RefMore: SPECv2 in README.md

// Define a test case to verify SPECv2-z.5 in README.md
/** RefMore: ===TEMPLATE OF UT CASE=== in UT_FreelyDrafts.cxx
 * @[Name]: verifyASync_byPostTestSleep9ms99msEvtEvery10msByEvtPrducerInSingleThread
 * @[Purpose]: verify SPECv2-z.5 in README.md
 * @[Steps]:
 *   1) ObjB as EvtCosmer subEVT(TEST_SLEEP_9MS), ObjC as EvtCosmer subEVT(TEST_SLEEP_99MS)
 *   2) ObjA as EvtPrducer postEVT(TEST_SLEEP_9MS) every 10ms and postEVT(TEST_SLEEP_99MS) every 100ms
 *       |-> ObjA in single thread, ObjA run in a usleep(10ms) loop in 100 times.
 *   3) ObjA's PostTestSleep9msEvtCnt is 100 and PostTestSleep99msEvtCnt is 10
 *   4) ObjB's ProcTestSleep9msEvtCnt is 100, ObjC's ProcTestSleep99msEvtCnt is 10
 * @[Expect]: Step3 and Step4 are all true.
 *
 */
// TEST(UT_ConlesEventConcurrency, verifyASync_byPostTestSleep9ms99msEvtEvery10msByEvtPrducerInSingleThread) {}