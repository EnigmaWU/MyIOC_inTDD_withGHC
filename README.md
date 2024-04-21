# About::MyIOC_inTDD_withGHC

* This is a practice module named Inter-Object-Communication(a.k.a IOC) and has functions as its name
  * in Test-Driven Development(a.k.a TDD) as Dev-Approach
    * with GitHub Copilot(a.k.a GHC) as Dev-Facilites.

# Requirement and Analysis

* [TODO: UseCase](./README_UseCase.md)              (BeforeAnalysis)//
* [TODO: UserStories](./README_UserStories.md)      (DoingAnalysis)//RefBook: <<User Story Applied>>
* [TODO: Specification](./README_Specification.md)  (AfterAnalysis)//RefPDF: https://github.com/Orthant/IEEE/blob/master/29148-2011.pdf

# Architecure and Design

* [ArchDesign](./README_ArchDesign.md)
* [TODO: ModuleDesign](./README_ModuleDesign.md)
* [TODO: VerifyDesign](./README_VerifyDesign.md)


# Specifications(a.k.a SPEC)

* 【v1】Initial project framework.
  * a）Dirs: Include,Source,Test
  * b）Build: Default TargetUsage=DiagASAN, depend on GTest, enable F5 debug.
  * c）Manifests: LIB::libMyIOC.a, UTEXE::Test/*.cxx

* 【v2】IOC_Event: ~~postEVT,subEVT/cbEVT~~,waitEVT
  * a）Scope: inter-thread, connectless mode(a.k.a ConlesMode);
  * b）Option: ASync\<DFT\>/Sync, MayBlock/NonBlock\<DFT>\/Timeout;
    * Perf: Sync>>ASync, NonBlock>>MayBlock
  * c）Abilities:
    * i)Support ~~1:1/1:N/N:1/N:M~~ post event in ConlesMode;
    * ii)Support ~~customize event ID~~ payload data;
    * iii)~~Support postEVT in cbEVT~~;
  * z) IF...THEN...
    * 1)~~IF no ObjXYZ subEVT, THEN return NO_EvtConsumer when postEVT, return NO_EvtConsumer when unsbuEVT.~~
    * 2)~~IF too many subEVT, THEN return TOO_MANY_EvtConsumer. IF subEVT twice or more, THEN return CONFLICT_EvtConsumer.~~
    * 3)IF subEVT MAX EvtConsumer and unsubEVT all, THEN this is a repeatable progress as robustness. ALSO multiply thread subEVT&unsubEVT still works as robustness.
    * 4)~~IF EvtProducer postEVT(TEST_HELLO_A or TEST_HELLO_B) alternatively, ObjA as EvtConsumer subEVT(TEST_HELLO_A), ObjB as EvtConsumer subEVT(TEST_HELLO_B), THEN ObjA's CbProcEvt will callbacked only with TEST_HELLO_A, ObjB is same exactly with ObjA.~~
    * 5)~~IF ObjA postEVT(TEST_SLEEP_10MS) every 10ms, postEVT(TEST_SLEEP_100MS) every 100ms in a single thread; ObjB subEVT(TEST_SLEEP_10MS) and do sleep 10ms in its CbProcEvt, ObjC subEVT(TEST_SLEEP_100MS) and do sleep 100ms in its CbProcEvt; THEN ObjA will(capable) post 100xTEST_SLEEP_10MS and 10xTEST_SLEE_100MS every second.~~
      * RefMore: EVT::FSM::Conles
    * 6)IF ObjA postEVT(TEST_SLEEP_10MS) every 10ms, ObjB subEVT(TEST_SLEEP_10MS) but do sleep 999ms in its CbProcEvt, THEN ObjA postEVT with ASync/NonBlock by default, but may postEVT in Sync or MayBlock or Timeout.
    * 7)~~IF ObjA subEVT(TEST_SLEEP_99MS), ObjB postEVT(TEST_SLEEP_99MS) with Sync option ON, and ObjA update its SyncFlagValue to TRUE after usleep(99000), THEN ObjB's postEVT will cost >99ms and get ObjA's SyncFlagValue is TRUE immediately after postEVT return.~~
    * 8)IF ObjA's CbProcEvt using too many CPU cycles, ObjB posted too many events, THEN ObjB's postEVT will get TOO_MANY_EVENTS by default, or blocked if OptID=MayBlock is ON, or block a while then get TIMEOUT if OptID=Timeout/withMS is SET.
    * 9) IF ObjA postEVT to ObjB as fast as possible, in ObjB's CbProcEvt_F post another twice or more event to ObjC/x2/x4/..., THEN ObjA/... get TOO_MANY_QUEUING_EVTDESC by default in NonBlock or block until return SUCCESS if MayBlock option on.

* 【vN】Pending ideas...
  * Doc/ScrnCapRecs, clang-format/-tidy
  * Scope: inter-process/-chip/-machine, connectful/connectless mode
  * IOC_Command: execCMD,subCMD/cbCMD,waitCMD/ackCMD
  * IOC_Data: sendDAT,recvDAT
  * Notes: In order to ..., as a ..., I want to ...
  * UserStory = As a「X=Who/Role」，I want 「Y=What/Func」，So that 「Z=Why/Value」
  * ...
