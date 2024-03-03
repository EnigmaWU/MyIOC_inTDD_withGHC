#include "_UT_IOC_Common.h"

/**
 * @[Name]: verifyTooManyEvtCosmer_bySubEvtOneMoreThanMaxEvtCosmer
 * @[Purpose]: accord [SPECv2-z.2], verify the behavior of too many event consumer call subEVT will return TOO_MANY_EVTCOSMER.
 * @[Steps]:
 *   1. Get the max EvtCosmer number by IOC_getCapabilty(CAPID=CONLES_MODE_EVENT).
 *   2. Call subEVT with FakeSubEvtArgs reach max EvtCosmer number.
 *        |-> CbProcEvt_F and CbPrivData is fake valued with number index.
 *   3. Call subEVT with max EvtCosmer number + 1.
 *   4. Call unsubEVT with max EvtCosmer number.
 * @[Expect]: subEVT with max EvtCosmer number + 1 will return IOC_RESULT_TOO_MANY_EVTCOSMER.
 * @[Notes]:
 */

TEST(UT_ConlesEventCapability, Case01_verifyTooManyEvtCosmer_bySubEvtOneMoreThanMaxEvtCosmer) {
  //===SETUP===
  IOC_CapabiltyDescription_T CapDesc = {.CapID = IOC_CAPID_CONLES_MODE_EVENT};
  IOC_Result_T Result = IOC_getCapabilty(&CapDesc);
  ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // CheckPoint

#define _Case01_FakeCbProcEvtBase ((IOC_CbProcEvt_F)0x20240303UL)
#define _Case01_FakeCbPrivDataBase ((void *)0x30304202UL)
  for (uint16_t i = 0; i < CapDesc.ConlesModeEvent.MaxEvtCosmer; i++) {
    IOC_SubEvtArgs_T ObjA_SubEvtArgs = {
        .CbProcEvt_F = (IOC_CbProcEvt_F)((ULONG_T)_Case01_FakeCbProcEvtBase + i),
        .pCbPrivData = (void *)((uintptr_t)_Case01_FakeCbPrivDataBase + i),
        .EvtNum = 0,
        .pEvtIDs = NULL,
    };
    Result = IOC_subEVT_inConlesMode(&ObjA_SubEvtArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // CheckPoint
  }

  //===BEHAVIOR===
  IOC_SubEvtArgs_T ObjA_SubEvtArgs = {
      .CbProcEvt_F = (IOC_CbProcEvt_F)((ULONG_T)_Case01_FakeCbProcEvtBase + CapDesc.ConlesModeEvent.MaxEvtCosmer),
      .pCbPrivData = (void *)((uintptr_t)_Case01_FakeCbPrivDataBase + CapDesc.ConlesModeEvent.MaxEvtCosmer),
      .EvtNum = 0,
      .pEvtIDs = NULL,
  };
  Result = IOC_subEVT_inConlesMode(&ObjA_SubEvtArgs);

  //===VERIFY===
  ASSERT_EQ(IOC_RESULT_TOO_MANY_EVTCOSMER, Result);  // KeyVerifyPoint

  //===CLEANUP===
  for (uint16_t i = 0; i < CapDesc.ConlesModeEvent.MaxEvtCosmer; i++) {
    IOC_UnsubEvtArgs_T ObjA_UnsubEvtArgs = {
        .CbProcEvt_F = (IOC_CbProcEvt_F)((ULONG_T)_Case01_FakeCbProcEvtBase + i),
        .pCbPriv = (void *)((uintptr_t)_Case01_FakeCbPrivDataBase + i),
    };
    Result = IOC_unsubEVT_inConlesMode(&ObjA_UnsubEvtArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // CheckPoint
  }
}
