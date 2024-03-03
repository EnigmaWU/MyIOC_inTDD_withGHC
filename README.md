# About::MyIOC_inTDD_withGHC

* This is a practice module named Inter-Object-Communication(a.k.a IOC) and has functions as its name
  * in Test-Driven Development(a.k.a TDD) as Dev-Approach
    * with GitHub Copilot(a.k.a GHC) as Dev-Facilites.

# Definitions

* Event Producer(a.k.a EvtPrduer) who generate/trigge events.
  * EvtPrduer will post event to IOC by IOC_postEVT API.
* Event Consumer(a.k.a EvtCosmer) who process events.
  * EvtCosmer will subscribe or unsubscribe event to IOC by IOC_subEVT or IOC_unsubEVT API.
* Module Manager(a.k.a ModMgr) who is a manager role such as platform manager, call IOC's MGR_APIs with arguments by product requirements to initModule, or deinitModule before module exit.
* Module User(a.k.a ModUsr) who is EvtPrduer or EvtCosmer call IOC's USR_APIs.

# Specifications(a.k.a SPEC)

* 【v1】Initial project framework.
  * a）Dirs: Include,Source,Test
  * b）Build: Default TargetUsage=DiagASAN, depend on GTest, enable F5 debug.
  * c）Manifests: LIB::libMyIOC.a, UTEXE::Test/*.cxx

* 【v2】IOC_Event: postEVT,subEVT/cbEVT,waitEVT
  * a）Scope: inter-thread, connectless mode(a.k.a ConlesMode);
  * b）Option: ASync\<DFT\>/Sync, MayBlock/NonBlock\<DFT>\/Timeout;
    * Perf: Sync>>ASync, NonBlock>>MayBlock
  * c）Abilities:
    * i)Support ~~1:1/1:N~~/N:1/N:M post event in ConlesMode;
    * ii)Support ~~customize event ID~~ payload data;
    * iii)Support postEVT in cbEVT;
  * z) IF...THEN...
    * 1)~~IF no ObjXYZ subEVT, THEN return NO_EVTCOSMER when postEVT, return NO_EVTCOSMER when unsbuEVT.~~
    * 2)~~IF too many subEVT, THEN return TOO_MANY_EVTCOSMER. IF subEVT twice or more, THEN return CONFLICT_EVTCOSMER.~~
    * 3)IF subEVT MAX EvtCosmer and unsubEVT all, THEN this is a repeatable progress as robustness. ALSO multiply thread subEVT&unsubEVT still works as robustness.

* 【vN】Pending ideas...
  * Doc/ScrnCapRecs, clang-format/-tidy
  * Scope: inter-process/-chip/-machine, connectful/connectless mode
  * IOC_Command: execCMD,subCMD/cbCMD,waitCMD/ackCMD
  * IOC_Data: sendDAT,recvDAT
  * Notes: In order to ..., as a ..., I want to ...
  * ...
