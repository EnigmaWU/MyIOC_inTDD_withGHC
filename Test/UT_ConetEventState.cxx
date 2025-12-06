/**************************************************************************************************
 * @file UT_ConetEventState.cxx
 * @brief Unit tests for Link Event State patterns in ConetMode (connection-oriented mode)
 * @date 2025-11-30
 *
 * CaTDD Implementation - Following LLM/CaTDD_DesignPrompt.md methodology
 *
 * DESIGN PHASE: Complete ✅
 * IMPLEMENTATION PHASE: Tests written but BLOCKED ⚠️
 * STATUS: Need to resolve EventCallbackHelper LinkID tracking issue
 **************************************************************************************************/

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>

#include "IOC/IOC.h"
#include "IOC/IOC_EvtAPI.h"
#include "IOC/IOC_SrvAPI.h"
#include "IOC/IOC_Types.h"
#include "_UT_IOC_Common.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF OVERVIEW OF THIS UNIT TESTING FILE===============================================
/**
 * @brief
 *   [WHAT] This file verifies Link Operation State (Level 2) behavior during ConetMode event operations
 *   [WHERE] in the IOC Event API (ConetMode - connection-oriented)
 *   [WHY] to ensure link states correctly reflect Ready/Busy transitions during event operations
 *
 * SCOPE:
 *   - In scope: ConetMode event state (postEVT, subEVT, unsubEVT, callback execution)
 *   - In scope: Level 2 (Operation State): Ready ⟷ BusyCbProcEvt/BusySubEvt/BusyUnsubEvt
 *   - In scope: Verification that Level 3 (SubState) is ALWAYS Default (architectural constraint)
 *   - Out of scope: Level 1 (Connection State) - covered in UT_LinkConnStateTCP.cxx
 *   - Out of scope: ConlesMode events - covered in UT_ConlesEventState.cxx
 *
 * KEY CONCEPTS:
 *   - ConetMode: Connection-oriented event delivery using explicit LinkID
 *   - LinkID: Obtained from IOC_connectService() or IOC_acceptClient()
 *   - Event Operations: IOC_postEVT(LinkID, ...), IOC_subEVT(LinkID, ...)
 *   - Fire-and-forget: Event posting is asynchronous, link stays Ready
 *   - NO EVT SubStates: Unlike CMD/DAT, events don't use Level 3 substates (always Default)
 *
 * RELATIONSHIPS:
 *   - Depends on: IOC_onlineService, IOC_connectService, IOC_acceptClient, IOC_postEVT, IOC_subEVT
 *   - Related tests: UT_ConlesEventState.cxx (ConlesMode), UT_LinkStateOperation.cxx (protocol-agnostic)
 *   - Production code: Source/IOC_Event.c, Source/_IOC_ConetEvent.c
 *   - Architecture doc: README_ArchDesign-State.md (3-level state hierarchy)
 */
//======>END OF OVERVIEW OF THIS UNIT TESTING FILE=================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF UNIT TESTING DESIGN==============================================================

/**************************************************************************************************
 * 📋 COVERAGE STRATEGY: ConetMode Event State Testing
 *
 * DIMENSIONS:
 *   Dimension 1: Event Operation (Post, Subscribe, Unsubscribe, Callback)
 *   Dimension 2: Link State (Ready, BusyCbProcEvt, BusySubEvt, BusyUnsubEvt)
 *   Dimension 3: Mode Comparison (ConetMode vs ConlesMode)
 *
 * COVERAGE MATRIX:
 * ┌──────────────────┬────────────────┬──────────────┬────────────────────────────────┐
 * │ Operation        │ Expected State │ Mode         │ User Story                     │
 * ├──────────────────┼────────────────┼──────────────┼────────────────────────────────┤
 * │ Post Event       │ Ready          │ ConetMode    │ US-1: Fire-and-forget post     │
 * │ Subscribe Event  │ Ready          │ ConetMode    │ US-2: Subscription management  │
 * │ Callback Execute │ BusyCbProcEvt  │ ConetMode    │ US-3: State during callback    │
 * │ All Operations   │ SubState=0     │ ConetMode    │ US-4: No EVT substates         │
 * │ Post Comparison  │ Ready          │ Both         │ US-5: Mode pattern comparison  │
 * └──────────────────┴────────────────┴──────────────┴────────────────────────────────┘
 *
 * PRIORITY: P2 🥈 DESIGN-ORIENTED (State Testing)
 *   - State transitions are architectural core for event operations
 *   - Tests validate FSM correctness for ConetMode
 **************************************************************************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF USER STORY=======================================================================
/**
 * US-1: As an event system developer,
 *       I want IOC_postEVT to be fire-and-forget (link stays Ready),
 *       So that event producers don't block on delivery.
 *
 * US-2: As an event consumer implementer,
 *       I want IOC_subEVT/unsubEVT to transition through proper states,
 *       So that I can monitor subscription lifecycle.
 *
 * US-3: As a callback function implementer,
 *       I want to know link state during callback execution (BusyCbProcEvt),
 *       So that I can detect reentrant calls or debugging.
 *
 * US-4: As an architecture validator,
 *       I want to verify EVT operations never use Level 3 substates,
 *       So that design decision "NO EVT SubStates" is enforced.
 *
 * US-5: As a system integrator,
 *       I want to understand ConetMode vs ConlesMode state patterns,
 *       So that I can choose the right mode for my use case.
 */
//======>END OF USER STORY=========================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//=======>BEGIN OF ACCEPTANCE CRITERIA=============================================================
/**
 * [@US-1] Fire-and-forget event posting
 *  AC-1: GIVEN a ConetMode link in Ready state,
 *         WHEN IOC_postEVT is called with valid event,
 *         THEN link remains in Ready state immediately after post,
 *          AND SubState remains Default (0).
 *
 * [@US-2] Subscription state management
 *  AC-1: GIVEN a ConetMode link in Ready state,
 *         WHEN IOC_subEVT is called,
 *         THEN link returns to Ready state after subscription completes,
 *          AND SubState remains Default (0).
 *  AC-2: GIVEN a ConetMode link with active subscription,
 *         WHEN IOC_unsubEVT is called,
 *         THEN link returns to Ready state after unsubscription completes,
 *          AND SubState remains Default (0).
 *
 * [@US-3] State during callback execution
 *  AC-1: GIVEN a ConetMode link with subscription and registered callback,
 *         WHEN event triggers callback execution,
 *         THEN link shows BusyCbProcEvt state during callback,
 *          AND SubState remains Default (0),
 *          AND link returns to Ready after callback completes.
 *
 * [@US-4] Architectural constraint: No EVT substates
 *  AC-1: GIVEN various EVT operations (post, subscribe, unsubscribe, callback),
 *         WHEN state is queried at any point,
 *         THEN SubState is ALWAYS Default (0),
 *          AND this holds for all operations regardless of timing.
 *
 * [@US-5] ConetMode vs ConlesMode comparison
 *  AC-1: GIVEN equivalent operations in both modes,
 *         WHEN comparing state patterns,
 *         THEN both modes show similar state behavior (Ready for fire-and-forget),
 *          AND both modes use SubState=Default (0),
 *          AND key difference is LinkID (explicit vs AutoLinkID).
 */
//=======>END OF ACCEPTANCE CRITERIA===============================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF TEST CASES=======================================================================
/**
 * ═════════════════════════════════════════════════════════════════════════════════════════════
 * 📋 [CATEGORY: State] ConetMode Event Operation State Patterns
 * ═════════════════════════════════════════════════════════════════════════════════════════════
 *
 * [@AC-1,US-1] Fire-and-forget event posting
 *  ⚠️ TC-1: verifyEventState_postEvtViaLink_expectReadyWithDefaultSubstate
 *      @[Purpose]: Validate fire-and-forget semantics - link stays Ready after post
 *      @[Brief]: Post event via ConetMode link, verify Ready state immediately after
 *      @[Status]: BLOCKED - Need to run to verify GREEN status
 *
 * [@AC-1,US-2] Subscription state management
 *  ⚠️ TC-2: verifyEventState_subscriptionViaLink_expectLinkStateOnly
 *      @[Purpose]: Validate link state during subscribe/unsubscribe lifecycle
 *      @[Brief]: Subscribe and unsubscribe, verify Ready state after each operation
 *      @[Status]: BLOCKED - Need to run to verify GREEN status
 *
 * [@AC-1,US-3] State during callback execution
 *  🚫 TC-3: verifyEventState_callbackExecution_expectBusyCbProcEvt
 *      @[Purpose]: Validate link shows BusyCbProcEvt during callback
 *      @[Brief]: Trigger callback with blocking, query state during execution
 *      @[Status]: BLOCKED - EventCallbackHelper cannot track LinkID!
 *      @[ISSUE]: pEvtDesc doesn't have PostedByLinkID field
 *      @[SOLUTION NEEDED]: Pass LinkID to helper via setup or redesign test
 *
 * [@AC-1,US-4] No EVT substates verification
 *  ⚠️ TC-4: verifyEventState_noEVTSubstates_expectDefault
 *      @[Purpose]: Comprehensive verification that SubState is ALWAYS Default
 *      @[Brief]: Perform all EVT operations, capture state snapshots, verify SubState=0
 *      @[Status]: BLOCKED - Need to run to verify GREEN status
 *
 * ═════════════════════════════════════════════════════════════════════════════════════════════
 * 📋 [CATEGORY: Comparison] ConetMode vs ConlesMode State Patterns
 * ═════════════════════════════════════════════════════════════════════════════════════════════
 *
 * [@AC-1,US-5] Post pattern comparison
 *  ⚠️ TC-5: compareEventState_postPatterns_expectSimilarBehavior
 *      @[Purpose]: Verify both modes use fire-and-forget with Ready state
 *      @[Brief]: Post via ConetMode and ConlesMode, compare state patterns
 *      @[Status]: BLOCKED - Need to run to verify GREEN status
 *
 * [@AC-1,US-5] Subscription model comparison
 *  ⚠️ TC-6: compareEventState_subscriptionModels_expectDifferences
 *      @[Purpose]: Highlight LinkID vs AutoLinkID difference
 *      @[Brief]: Subscribe in both modes, verify same state but different IDs
 *      @[Status]: BLOCKED - Need to run to verify GREEN status
 *
 * [@AC-1,US-5] State tracking comparison
 *  ⚠️ TC-7: compareEventState_stateTracking_expectMainStatesOnly
 *      @[Purpose]: Verify both modes use only Level 2 states (no Level 3)
 *      @[Brief]: Query state in both modes, verify SubState=Default
 *      @[Status]: BLOCKED - Need to run to verify GREEN status
 *
 * [@AC-1,US-5] Architecture compliance comparison
 *  ⚠️ TC-8: verifyArchitectureCompliance_noEVTSubstates_expectConsistent
 *      @[Purpose]: Comprehensive verification of "NO EVT SubStates" design decision
 *      @[Brief]: Perform all operations in both modes, verify SubState=0 throughout
 *      @[Status]: BLOCKED - Need to run to verify GREEN status
 */
//======>END OF TEST CASES=========================================================================
//======>END OF UNIT TESTING DESIGN================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF TODO/IMPLEMENTATION TRACKING SECTION=============================================
// 🔴 IMPLEMENTATION STATUS TRACKING
//
// STATUS LEGEND:
//   🟢 GREEN/PASSED:      Test written and passing
//   ⚠️  BLOCKED/ISSUES:   Test written but cannot verify (need to run or fix issue)
//   🚫 CRITICAL_BLOCK:    Fundamental issue preventing test execution
//
//===================================================================================================
// P2 🥈 DESIGN-ORIENTED TESTING – State Patterns
//===================================================================================================
//
//   ⚠️ [@AC-1,US-1] TC-1: verifyEventState_postEvtViaLink_expectReadyWithDefaultSubstate
//        - Status: Test implemented, compiles, need to RUN to verify GREEN
//        - Category: State (ConetMode fire-and-forget)
//        - Estimated: Should pass immediately (simple state query)
//
//   ⚠️ [@AC-1,US-2] TC-2: verifyEventState_subscriptionViaLink_expectLinkStateOnly
//        - Status: Test implemented, compiles, need to RUN to verify GREEN
//        - Category: State (subscription lifecycle)
//        - Estimated: Should pass immediately
//
//   🚫 [@AC-1,US-3] TC-3: verifyEventState_callbackExecution_expectBusyCbProcEvt
//        - Status: CRITICAL BLOCK - EventCallbackHelper cannot track state!
//        - Category: State (callback execution)
//        - ISSUE: Callback receives pEvtDesc but no LinkID
//        - ISSUE: Cannot call IOC_getLinkState(linkID, ...) without LinkID
//        - SOLUTION OPTIONS:
//          A) Pass LinkID via helper constructor/setup method (RECOMMENDED)
//          B) Store LinkID in test fixture shared state
//          C) Accept limitation - verify state from OUTSIDE callback only
//        - Estimated: 30min to fix + verify
//
//   ⚠️ [@AC-1,US-4] TC-4: verifyEventState_noEVTSubstates_expectDefault
//        - Status: Test implemented, compiles, need to RUN to verify GREEN
//        - Category: State (architectural compliance)
//        - Estimated: Should pass immediately
//
//===================================================================================================
// P2 🥈 DESIGN-ORIENTED TESTING – Mode Comparison
//===================================================================================================
//
//   ⚠️ [@AC-1,US-5] TC-5: compareEventState_postPatterns_expectSimilarBehavior
//        - Status: Test implemented, compiles, need to RUN to verify GREEN
//        - Category: Comparison (ConetMode vs ConlesMode)
//        - Estimated: Should pass immediately
//
//   ⚠️ [@AC-1,US-5] TC-6: compareEventState_subscriptionModels_expectDifferences
//        - Status: Test implemented, compiles, need to RUN to verify GREEN
//        - Category: Comparison (subscription patterns)
//        - Estimated: Should pass immediately
//
//   ⚠️ [@AC-1,US-5] TC-7: compareEventState_stateTracking_expectMainStatesOnly
//        - Status: Test implemented, compiles, need to RUN to verify GREEN
//        - Category: Comparison (state tracking)
//        - Estimated: Should pass immediately
//
//   ⚠️ [@AC-1,US-5] TC-8: verifyArchitectureCompliance_noEVTSubstates_expectConsistent
//        - Status: Test implemented, compiles, need to RUN to verify GREEN
//        - Category: Comparison (comprehensive verification)
//        - Estimated: Should pass immediately
//
//===================================================================================================
// 🚪 IMMEDIATE ACTIONS REQUIRED
//===================================================================================================
//
//   1. FIX TC-3 EventCallbackHelper LinkID tracking issue (CRITICAL)
//   2. RUN all 8 tests to verify GREEN status
//   3. Update status markers based on results
//   4. Move to GREEN/PASSED once all tests pass
//
///////////////////////////////////////////////////////////////////////////////////////////////////
//======>END OF TODO/IMPLEMENTATION TRACKING SECTION===============================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======BEGIN OF UNIT TESTING IMPLEMENTATION=======================================================

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>

#include "IOC/IOC.h"
#include "IOC/IOC_EvtAPI.h"
#include "IOC/IOC_SrvAPI.h"
#include "IOC/IOC_Types.h"
#include "_UT_IOC_Common.h"

/**************************************************************************************************
 * @brief 【ConetMode Event State Test Infrastructure】
 *
 * Helper utilities for monitoring event state in ConetMode:
 *  • StateSnapshot: Captures link state at a point in time
 *  • EventCallbackHelper: Tracks callback execution and state during callbacks
 *  • StateMonitor: Polls link state during async operations
 **************************************************************************************************/

// Structure to capture link state snapshot
struct EventStateSnapshot {
    IOC_LinkState_T MainState;
    IOC_LinkSubState_T SubState;
    IOC_Result_T QueryResult;
    std::chrono::steady_clock::time_point Timestamp;
};

// Helper to capture current link state
static EventStateSnapshot CaptureEventState(IOC_LinkID_T linkID) {
    EventStateSnapshot snapshot;
    snapshot.QueryResult = IOC_getLinkState(linkID, &snapshot.MainState, &snapshot.SubState);
    snapshot.Timestamp = std::chrono::steady_clock::now();
    return snapshot;
}

// Callback helper to track state during event processing
class EventCallbackHelper {
   public:
    IOC_LinkID_T TrackedLinkID = IOC_ID_INVALID;  // LinkID to monitor during callback
    std::atomic<int> CallbackCount{0};
    std::atomic<IOC_LinkState_T> StateInCallback{IOC_LinkStateUndefined};
    std::atomic<IOC_LinkSubState_T> SubStateInCallback{IOC_LinkSubStateDefault};
    std::mutex Mutex;
    std::condition_variable CV;
    std::atomic<bool> CallbackStarted{false};
    std::atomic<bool> AllowCallbackToProceed{false};

    // Set LinkID to track - call before subscription
    void SetTrackedLink(IOC_LinkID_T linkID) { TrackedLinkID = linkID; }

    static IOC_Result_T StaticCallback(IOC_EvtDesc_pT pEvtDesc, void *pCbPrivData) {
        auto *helper = static_cast<EventCallbackHelper *>(pCbPrivData);
        return helper->OnCallback(pEvtDesc);
    }

   private:
    IOC_Result_T OnCallback(IOC_EvtDesc_pT pEvtDesc) {
        CallbackCount++;

        // Capture state during callback execution (if LinkID was set)
        if (TrackedLinkID != IOC_ID_INVALID) {
            IOC_getLinkState(TrackedLinkID, (IOC_LinkState_pT)&StateInCallback,
                             (IOC_LinkSubState_pT)&SubStateInCallback);
        }

        // Signal that callback started
        CallbackStarted = true;
        CV.notify_all();

        // Wait for test to allow callback to proceed (for blocking tests)
        std::unique_lock<std::mutex> lock(Mutex);
        CV.wait(lock, [this] { return AllowCallbackToProceed.load(); });
        return IOC_RESULT_SUCCESS;
    }
};

/**************************************************************************************************
 * @brief 【CAT-1: ConetMode Event State Patterns】
 *
 * Verify link operation state behavior during ConetMode event operations:
 *  • TC-1: Post event via link → verify Ready state (fire-and-forget)
 *  • TC-2: Subscribe event via link → verify state during subscription
 *  • TC-3: Callback execution → verify BusyCbProcEvt state
 *  • TC-4: No EVT substates → verify SubState always Default(0)
 **************************************************************************************************/

/**
 * @test TC1_verifyEventState_postEvtViaLink_expectReadyWithDefaultSubstate
 * @brief Verify link remains in Ready state during event post (fire-and-forget)
 *
 * @architecture Event post in ConetMode is fire-and-forget:
 *   - Link posts event to queue
 *   - Link returns to Ready immediately (no waiting)
 *   - SubState remains Default (0) - no EVT substates
 *
 * @steps
 *   1. Setup TCP service and client link
 *   2. Query initial state → expect Ready + Default substate
 *   3. Post event via link
 *   4. Query state immediately after post → expect Ready + Default substate
 *   5. Cleanup
 */
TEST(UT_ConetEventState_Patterns, TC1_verifyEventState_postEvtViaLink_expectReadyWithDefaultSubstate) {
    //===SETUP: Capture accepted link via callback===
    struct AutoAcceptCtx {
        std::atomic<IOC_LinkID_T> AcceptedLinkID{IOC_ID_INVALID};
    } ctx;

    auto onAccepted = [](IOC_SrvID_T, IOC_LinkID_T linkID, void *pPriv) {
        auto *pCtx = static_cast<AutoAcceptCtx *>(pPriv);
        pCtx->AcceptedLinkID = linkID;
        printf("🎯 Auto-accept callback: accepted LinkID=%" PRIu64 "\n", linkID);
    };

    IOC_SrvURI_T srvURI = {.pProtocol = IOC_SRV_PROTO_FIFO,
                           .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
                           .pPath = (const char *)"ConetEvtState_TC1"};
    IOC_SrvArgs_T srvArgs = {.SrvURI = srvURI,
                             .Flags = IOC_SRVFLAG_AUTO_ACCEPT,
                             .UsageCapabilites = IOC_LinkUsageEvtProducer,
                             .OnAutoAccepted_F = onAccepted,
                             .pSrvPriv = &ctx};
    IOC_SrvID_T srvID = IOC_ID_INVALID;
    IOC_Result_T result = IOC_onlineService(&srvID, &srvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result);

    //===SETUP: Client connects with auto-subscribe (will trigger auto-accept)===
    EventCallbackHelper helper;
    helper.AllowCallbackToProceed = true;  // Allow callback to proceed immediately (non-blocking test)

    IOC_EvtID_T evtIDs[] = {IOC_EVTID_TEST_KEEPALIVE};
    IOC_EvtUsageArgs_T evtArgs = {
        .CbProcEvt_F = EventCallbackHelper::StaticCallback, .pCbPrivData = &helper, .EvtNum = 1, .pEvtIDs = evtIDs};

    IOC_ConnArgs_T connArgs = {.SrvURI = srvURI, .Usage = IOC_LinkUsageEvtConsumer};
    connArgs.UsageArgs.pEvt = &evtArgs;  // Auto-subscribe on client side
    IOC_LinkID_T cliLinkID = IOC_ID_INVALID;
    result = IOC_connectService(&cliLinkID, &connArgs, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result);

    // Wait for auto-accept
    for (int i = 0; i < 100 && ctx.AcceptedLinkID == IOC_ID_INVALID; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    IOC_LinkID_T srvLinkID = ctx.AcceptedLinkID.load();
    ASSERT_NE(IOC_ID_INVALID, srvLinkID) << "Auto-accept should provide server link ID";

    printf("🔍 TC-1: cliLinkID=%" PRIu64 ", srvLinkID=%" PRIu64 "\n", cliLinkID, srvLinkID);

    //===VERIFY: Initial state===
    EventStateSnapshot initial = CaptureEventState(srvLinkID);
    ASSERT_EQ(IOC_RESULT_SUCCESS, initial.QueryResult);
    EXPECT_EQ(IOC_LinkStateReady, initial.MainState);
    EXPECT_EQ(IOC_LinkSubStateDefault, initial.SubState);

    //===BEHAVIOR: Post event from service side===
    IOC_EvtDesc_T evtDesc = {};
    evtDesc.EvtID = IOC_EVTID_TEST_KEEPALIVE;
    evtDesc.EvtValue = 12345;

    result = IOC_postEVT(srvLinkID, &evtDesc, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result) << "Post from service link should succeed";

    //===VERIFY: State after post===
    EventStateSnapshot afterPost = CaptureEventState(srvLinkID);
    ASSERT_EQ(IOC_RESULT_SUCCESS, afterPost.QueryResult);
    EXPECT_EQ(IOC_LinkStateReady, afterPost.MainState);
    EXPECT_EQ(IOC_LinkSubStateDefault, afterPost.SubState);

    //===CLEANUP===
    IOC_closeLink(cliLinkID);
    IOC_offlineService(srvID);
}

/**
 * @test TC2_verifyEventState_subscriptionViaLink_expectLinkStateOnly
 * @brief Verify link state during event subscription (BusySubEvt during sub, Ready after)
 *
 * @architecture Event subscription in ConetMode:
 *   - During IOC_subEVT: Link may show BusySubEvt
 *   - After subscription: Link returns to Ready
 *   - SubState remains Default (0) throughout
 *
 * @steps
 *   1. Setup TCP service and client link
 *   2. Subscribe to event
 *   3. Query state after subscription → expect Ready + Default substate
 *   4. Unsubscribe from event
 *   5. Query state after unsubscription → expect Ready + Default substate
 *   6. Cleanup
 */
TEST(UT_ConetEventState_Patterns, TC2_verifyEventState_subscriptionViaLink_expectLinkStateOnly) {
    //===SETUP: Create service as event producer===
    IOC_SrvURI_T srvURI = {.pProtocol = IOC_SRV_PROTO_FIFO,
                           .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
                           .pPath = (const char *)"ConetEvtState_TC2"};
    IOC_SrvArgs_T srvArgs = {.SrvURI = srvURI, .Flags = IOC_SRVFLAG_NONE, .UsageCapabilites = IOC_LinkUsageEvtProducer};
    IOC_SrvID_T srvID = IOC_ID_INVALID;
    IOC_Result_T result = IOC_onlineService(&srvID, &srvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result);
    ASSERT_NE(IOC_ID_INVALID, srvID);

    //===SETUP: Client connects as event subscriber===
    IOC_ConnArgs_T connArgs = {.SrvURI = srvURI, .Usage = IOC_LinkUsageEvtConsumer};
    IOC_LinkID_T cliLinkID = IOC_ID_INVALID;

    std::atomic<bool> clientConnected{false};
    std::thread clientThread([&] {
        IOC_Result_T connResult = IOC_connectService(&cliLinkID, &connArgs, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, connResult);
        ASSERT_NE(IOC_ID_INVALID, cliLinkID);
        clientConnected = true;
    });

    IOC_LinkID_T srvLinkID = IOC_ID_INVALID;
    result = IOC_acceptClient(srvID, &srvLinkID, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result);
    ASSERT_NE(IOC_ID_INVALID, srvLinkID);

    clientThread.join();
    ASSERT_TRUE(clientConnected.load());
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    //===BEHAVIOR: Subscribe to event using actual API signature===
    EventCallbackHelper helper;
    IOC_EvtID_T evtIDs[] = {IOC_EVTID_TEST_KEEPALIVE};
    IOC_SubEvtArgs_T subArgs = {};
    subArgs.EvtNum = 1;
    subArgs.pEvtIDs = evtIDs;
    subArgs.CbProcEvt_F = EventCallbackHelper::StaticCallback;
    subArgs.pCbPrivData = &helper;

    // Actual signature: IOC_subEVT(LinkID, pSubEvtArgs)
    result = IOC_subEVT(cliLinkID, &subArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result);

    //===VERIFY: State after subscription - Ready with Default substate===
    EventStateSnapshot afterSub = CaptureEventState(cliLinkID);
    ASSERT_EQ(IOC_RESULT_SUCCESS, afterSub.QueryResult);
    EXPECT_EQ(IOC_LinkStateReady, afterSub.MainState) << "After subscription, link should be Ready";
    EXPECT_EQ(IOC_LinkSubStateDefault, afterSub.SubState) << "EVT operations have NO substates";

    //===BEHAVIOR: Unsubscribe from event===
    IOC_UnsubEvtArgs_T unsubArgs = {};
    unsubArgs.CbProcEvt_F = EventCallbackHelper::StaticCallback;
    unsubArgs.pCbPrivData = &helper;

    // Actual signature: IOC_unsubEVT(LinkID, pUnsubEvtArgs)
    result = IOC_unsubEVT(cliLinkID, &unsubArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result);

    //===VERIFY: State after unsubscription - Ready with Default substate===
    EventStateSnapshot afterUnsub = CaptureEventState(cliLinkID);
    ASSERT_EQ(IOC_RESULT_SUCCESS, afterUnsub.QueryResult);
    EXPECT_EQ(IOC_LinkStateReady, afterUnsub.MainState) << "After unsubscription, link should be Ready";
    EXPECT_EQ(IOC_LinkSubStateDefault, afterUnsub.SubState) << "EVT substates always Default";

    //===CLEANUP===
    if (cliLinkID != IOC_ID_INVALID) {
        IOC_closeLink(cliLinkID);
    }
    if (srvLinkID != IOC_ID_INVALID) {
        IOC_closeLink(srvLinkID);
    }
    if (srvID != IOC_ID_INVALID) {
        IOC_offlineService(srvID);
    }
}

/**
 * @test TC3_verifyEventState_callbackExecution_expectBusyCbProcEvt
 * @brief Verify link shows BusyCbProcEvt during event callback execution
 *
 * @architecture During callback processing:
 *   - Link main state: BusyCbProcEvt
 *   - Link substate: Default (0) - no EVT substates
 *   - After callback: Returns to Ready
 *
 * @steps
 *   1. Setup TCP service and two links (poster + subscriber)
 *   2. Subscribe to event with blocking callback
 *   3. Post event from another link
 *   4. Wait for callback to start
 *   5. Query state during callback → expect BusyCbProcEvt
 *   6. Allow callback to complete
 *   7. Query state after callback → expect Ready
 *   8. Cleanup
 */
TEST(UT_ConetEventState_Patterns, TC3_verifyEventState_callbackExecution_expectBusyCbProcEvt) {
    //===SETUP: Create service as event producer (supports sending to subscribers)===
    IOC_SrvURI_T srvURI = {.pProtocol = IOC_SRV_PROTO_FIFO,
                           .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
                           .pPath = (const char *)"ConetEvtState_TC3"};
    IOC_SrvArgs_T srvArgs = {.SrvURI = srvURI,
                             .Flags = IOC_SRVFLAG_NONE,
                             .UsageCapabilites = IOC_LinkUsageEvtProducer};  // Service will SEND events
    IOC_SrvID_T srvID = IOC_ID_INVALID;
    IOC_Result_T result = IOC_onlineService(&srvID, &srvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result);
    ASSERT_NE(IOC_ID_INVALID, srvID);

    //===SETUP: Client connects as subscriber===
    IOC_ConnArgs_T subscriberConnArgs = {.SrvURI = srvURI, .Usage = IOC_LinkUsageEvtConsumer};
    IOC_LinkID_T subscriberLinkID = IOC_ID_INVALID;

    std::atomic<bool> subscriberConnected{false};
    std::thread subscriberThread([&] {
        IOC_Result_T connResult = IOC_connectService(&subscriberLinkID, &subscriberConnArgs, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, connResult);
        ASSERT_NE(IOC_ID_INVALID, subscriberLinkID);
        subscriberConnected = true;
    });

    IOC_LinkID_T srvLink1ID = IOC_ID_INVALID;
    result = IOC_acceptClient(srvID, &srvLink1ID, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result);

    subscriberThread.join();
    ASSERT_TRUE(subscriberConnected.load());
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    //===BEHAVIOR: Subscribe with blocking callback===
    EventCallbackHelper helper;
    helper.SetTrackedLink(subscriberLinkID);  // Tell helper which link to monitor
    IOC_EvtID_T evtIDs[] = {IOC_EVTID_TEST_KEEPALIVE};
    IOC_SubEvtArgs_T subArgs = {};
    subArgs.EvtNum = 1;
    subArgs.pEvtIDs = evtIDs;
    subArgs.CbProcEvt_F = EventCallbackHelper::StaticCallback;
    subArgs.pCbPrivData = &helper;

    result = IOC_subEVT(subscriberLinkID, &subArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result);

    //===BEHAVIOR: Service posts event (will trigger subscriber's callback)===
    IOC_EvtDesc_T evtDesc = {};
    evtDesc.EvtID = IOC_EVTID_TEST_KEEPALIVE;
    evtDesc.EvtValue = 99999;

    // Service posts via its link to subscriber
    result = IOC_postEVT(srvLink1ID, &evtDesc, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result);

    //===WAIT: For callback to start===
    {
        std::unique_lock<std::mutex> lock(helper.Mutex);
        helper.CV.wait_for(lock, std::chrono::milliseconds(500), [&helper] { return helper.CallbackStarted.load(); });
    }
    ASSERT_TRUE(helper.CallbackStarted.load()) << "Callback should have started";

    //===VERIFY: State during callback - BusyCbProcEvt===
    EventStateSnapshot duringCallback = CaptureEventState(subscriberLinkID);
    ASSERT_EQ(IOC_RESULT_SUCCESS, duringCallback.QueryResult);
    EXPECT_EQ(IOC_LinkStateBusyCbProcEvt, duringCallback.MainState)
        << "During callback, link should show BusyCbProcEvt";
    EXPECT_EQ(IOC_LinkSubStateDefault, duringCallback.SubState) << "EVT substates always Default";

    // Also verify state captured inside callback
    EXPECT_EQ(IOC_LinkStateBusyCbProcEvt, helper.StateInCallback.load())
        << "State queried inside callback should be BusyCbProcEvt";
    EXPECT_EQ(IOC_LinkSubStateDefault, helper.SubStateInCallback.load()) << "SubState inside callback is Default";

    //===BEHAVIOR: Allow callback to complete===
    helper.AllowCallbackToProceed = true;
    helper.CV.notify_all();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    //===VERIFY: State after callback - Ready===
    EventStateSnapshot afterCallback = CaptureEventState(subscriberLinkID);
    ASSERT_EQ(IOC_RESULT_SUCCESS, afterCallback.QueryResult);
    EXPECT_EQ(IOC_LinkStateReady, afterCallback.MainState) << "After callback, link should return to Ready";
    EXPECT_EQ(IOC_LinkSubStateDefault, afterCallback.SubState) << "EVT substates always Default";

    //===CLEANUP===
    IOC_UnsubEvtArgs_T unsubArgs = {};
    unsubArgs.CbProcEvt_F = EventCallbackHelper::StaticCallback;
    unsubArgs.pCbPrivData = &helper;
    IOC_unsubEVT(subscriberLinkID, &unsubArgs);

    if (subscriberLinkID != IOC_ID_INVALID) {
        IOC_closeLink(subscriberLinkID);
    }
    if (srvLink1ID != IOC_ID_INVALID) {
        IOC_closeLink(srvLink1ID);
    }
    if (srvID != IOC_ID_INVALID) {
        IOC_offlineService(srvID);
    }
}

/**
 * @test TC4_verifyEventState_noEVTSubstates_expectDefault
 * @brief Architectural verification: EVT operations NEVER use Level 3 substates
 *
 * @architecture "Why No EVT SubStates":
 *   - EVT operations are fire-and-forget or queue-based
 *   - No multi-step protocol like CMD (request-response) or DAT (send-ack)
 *   - SubState always remains Default (0)
 *   - Only main state changes: Ready ↔ Busy{CbProcEvt|SubEvt|UnsubEvt}
 *
 * @steps
 *   1. Setup service and link
 *   2. Perform various EVT operations (sub, post, unsub)
 *   3. Query state throughout operations
 *   4. Verify SubState ALWAYS remains Default (0)
 *   5. Cleanup
 */
TEST(UT_ConetEventState_Patterns, TC4_verifyEventState_noEVTSubstates_expectDefault) {
    //===SETUP: Create service as event producer===
    IOC_SrvURI_T srvURI = {.pProtocol = IOC_SRV_PROTO_FIFO,
                           .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
                           .pPath = (const char *)"ConetEvtState_TC4"};
    IOC_SrvArgs_T srvArgs = {.SrvURI = srvURI,
                             .Flags = IOC_SRVFLAG_NONE,
                             .UsageCapabilites = IOC_LinkUsageEvtProducer};  // Service will SEND events
    IOC_SrvID_T srvID = IOC_ID_INVALID;
    IOC_Result_T result = IOC_onlineService(&srvID, &srvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result);
    ASSERT_NE(IOC_ID_INVALID, srvID);

    //===SETUP: Client connects as consumer===
    IOC_ConnArgs_T connArgs = {.SrvURI = srvURI, .Usage = IOC_LinkUsageEvtConsumer};  // Client will RECEIVE events
    IOC_LinkID_T cliLinkID = IOC_ID_INVALID;

    std::atomic<bool> clientConnected{false};
    std::thread clientThread([&] {
        IOC_Result_T connResult = IOC_connectService(&cliLinkID, &connArgs, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, connResult);
        ASSERT_NE(IOC_ID_INVALID, cliLinkID);
        clientConnected = true;
    });

    IOC_LinkID_T srvLinkID = IOC_ID_INVALID;
    result = IOC_acceptClient(srvID, &srvLinkID, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result);

    clientThread.join();
    ASSERT_TRUE(clientConnected.load());
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    std::vector<EventStateSnapshot> snapshots;

    //===VERIFY: Initial state - SubState is Default===
    snapshots.push_back(CaptureEventState(srvLinkID));  // Query service link that will post

    //===BEHAVIOR: Subscribe to event===
    EventCallbackHelper helper;
    helper.AllowCallbackToProceed = true;  // Non-blocking callback
    IOC_EvtID_T evtIDs[] = {IOC_EVTID_TEST_KEEPALIVE};
    IOC_SubEvtArgs_T subArgs = {};
    subArgs.EvtNum = 1;
    subArgs.pEvtIDs = evtIDs;
    subArgs.CbProcEvt_F = EventCallbackHelper::StaticCallback;
    subArgs.pCbPrivData = &helper;
    result = IOC_subEVT(cliLinkID, &subArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result);

    snapshots.push_back(CaptureEventState(srvLinkID));

    //===BEHAVIOR: Service posts event===
    IOC_EvtDesc_T evtDesc = {};
    evtDesc.EvtID = IOC_EVTID_TEST_KEEPALIVE;
    evtDesc.EvtValue = 77777;
    result = IOC_postEVT(srvLinkID, &evtDesc, NULL);  // Service posts to client
    ASSERT_EQ(IOC_RESULT_SUCCESS, result);

    snapshots.push_back(CaptureEventState(srvLinkID));
    std::this_thread::sleep_for(std::chrono::milliseconds(100));  // Allow callback
    snapshots.push_back(CaptureEventState(srvLinkID));

    //===BEHAVIOR: Unsubscribe from event===
    IOC_UnsubEvtArgs_T unsubArgs = {};
    unsubArgs.CbProcEvt_F = EventCallbackHelper::StaticCallback;
    unsubArgs.pCbPrivData = &helper;
    result = IOC_unsubEVT(cliLinkID, &unsubArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result);

    snapshots.push_back(CaptureEventState(srvLinkID));

    //===VERIFY: SubState ALWAYS Default throughout all operations===
    for (size_t i = 0; i < snapshots.size(); i++) {
        EXPECT_EQ(IOC_LinkSubStateDefault, snapshots[i].SubState)
            << "Architectural requirement: EVT operations have NO Level 3 substates (snapshot " << i << ")";
    }

    //===CLEANUP===
    if (cliLinkID != IOC_ID_INVALID) {
        IOC_closeLink(cliLinkID);
    }
    if (srvLinkID != IOC_ID_INVALID) {
        IOC_closeLink(srvLinkID);
    }
    if (srvID != IOC_ID_INVALID) {
        IOC_offlineService(srvID);
    }
}

/**************************************************************************************************
 * @brief 【CAT-2: ConetMode vs ConlesMode Comparison】
 *
 * Compare event state patterns between ConetMode and ConlesMode:
 *  • TC-5: Post patterns - both fire-and-forget
 *  • TC-6: Subscription models - link-based vs auto-link
 *  • TC-7: State tracking - both use main states only
 *  • TC-8: Architecture compliance - no EVT substates in either mode
 **************************************************************************************************/

/**
 * @test TC5_compareEventState_postPatterns_expectSimilarBehavior
 * @brief Compare event post state behavior: ConetMode vs ConlesMode
 *
 * @architecture Both modes use fire-and-forget for event post:
 *   - ConetMode: Post via specific link, link stays Ready
 *   - ConlesMode: Post via auto-link, auto-link stays Ready
 *   - Both: SubState always Default (0)
 *
 * @steps
 *   1. Setup ConetMode link
 *   2. Post event via ConetMode → verify Ready state
 *   3. Post event via ConlesMode → verify Ready state
 *   4. Compare state patterns
 *   5. Cleanup
 */
TEST(UT_ConetEventState_Comparison, TC5_compareEventState_postPatterns_expectSimilarBehavior) {
    //===SETUP: ConetMode link - Service as producer===
    IOC_SrvURI_T srvURI = {.pProtocol = IOC_SRV_PROTO_FIFO,
                           .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
                           .pPath = (const char *)"ConetEvtState_TC5"};
    IOC_SrvArgs_T srvArgs = {.SrvURI = srvURI, .Flags = IOC_SRVFLAG_NONE, .UsageCapabilites = IOC_LinkUsageEvtProducer};
    IOC_SrvID_T srvID = IOC_ID_INVALID;
    IOC_Result_T result = IOC_onlineService(&srvID, &srvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result);

    IOC_ConnArgs_T connArgs = {.SrvURI = srvURI, .Usage = IOC_LinkUsageEvtConsumer};
    IOC_LinkID_T conetLinkID = IOC_ID_INVALID;

    std::atomic<bool> connected{false};
    std::thread connThread([&] {
        IOC_Result_T connResult = IOC_connectService(&conetLinkID, &connArgs, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, connResult);
        connected = true;
    });

    IOC_LinkID_T srvLinkID = IOC_ID_INVALID;
    result = IOC_acceptClient(srvID, &srvLinkID, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result);

    connThread.join();
    ASSERT_TRUE(connected.load());
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    //===BEHAVIOR: Service posts via ConetMode (explicit LinkID)===
    IOC_EvtDesc_T evtDescConet = {};
    evtDescConet.EvtID = IOC_EVTID_TEST_KEEPALIVE;
    evtDescConet.EvtValue = 11111;
    result = IOC_postEVT(srvLinkID, &evtDescConet, NULL);  // Service posts to client
    ASSERT_EQ(IOC_RESULT_SUCCESS, result);

    EventStateSnapshot conetAfterPost = CaptureEventState(srvLinkID);  // Query service link

    //===BEHAVIOR: Post via ConlesMode (AUTO_LINK_ID)===
    IOC_EvtDesc_T evtDescConles = {};
    evtDescConles.EvtID = IOC_EVTID_TEST_KEEPALIVE;
    evtDescConles.EvtValue = 22222;
    result = IOC_postEVT(IOC_CONLES_MODE_AUTO_LINK_ID, &evtDescConles, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result);

    EventStateSnapshot conlesAfterPost = CaptureEventState(IOC_CONLES_MODE_AUTO_LINK_ID);

    //===VERIFY: Both show Ready + Default substate===
    ASSERT_EQ(IOC_RESULT_SUCCESS, conetAfterPost.QueryResult);
    EXPECT_EQ(IOC_LinkStateReady, conetAfterPost.MainState) << "ConetMode: Post is fire-and-forget, link stays Ready";
    EXPECT_EQ(IOC_LinkSubStateDefault, conetAfterPost.SubState) << "ConetMode: No EVT substates";

    ASSERT_EQ(IOC_RESULT_SUCCESS, conlesAfterPost.QueryResult);
    EXPECT_EQ(IOC_LinkStateReady, conlesAfterPost.MainState)
        << "ConlesMode: Post is fire-and-forget, auto-link stays Ready";
    EXPECT_EQ(IOC_LinkSubStateDefault, conlesAfterPost.SubState) << "ConlesMode: No EVT substates";

    // Both modes exhibit same state pattern for post
    EXPECT_EQ(conetAfterPost.MainState, conlesAfterPost.MainState) << "Post pattern consistent across modes";

    //===CLEANUP===
    if (conetLinkID != IOC_ID_INVALID) {
        IOC_closeLink(conetLinkID);
    }
    if (srvLinkID != IOC_ID_INVALID) {
        IOC_closeLink(srvLinkID);
    }
    if (srvID != IOC_ID_INVALID) {
        IOC_offlineService(srvID);
    }
}

/**
 * @test TC6_compareEventState_subscriptionModels_expectDifferences
 * @brief Compare subscription state patterns: ConetMode (link-based) vs ConlesMode (auto-link)
 *
 * @architecture Subscription models differ:
 *   - ConetMode: Explicit link subscription, link tracks state
 *   - ConlesMode: Auto-link subscription, simplified management
 *   - Both: SubState always Default (0)
 *
 * @steps
 *   1. Setup ConetMode link
 *   2. Subscribe via ConetMode → verify link state
 *   3. Subscribe via ConlesMode → verify auto-link state
 *   4. Compare subscription patterns
 *   5. Cleanup
 */
TEST(UT_ConetEventState_Comparison, TC6_compareEventState_subscriptionModels_expectDifferences) {
    //===SETUP: ConetMode link===
    IOC_SrvURI_T srvURI = {.pProtocol = IOC_SRV_PROTO_FIFO,
                           .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
                           .pPath = (const char *)"ConetEvtState_TC6"};
    IOC_SrvArgs_T srvArgs = {.SrvURI = srvURI, .Flags = IOC_SRVFLAG_NONE, .UsageCapabilites = IOC_LinkUsageEvtProducer};
    IOC_SrvID_T srvID = IOC_ID_INVALID;
    IOC_Result_T result = IOC_onlineService(&srvID, &srvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result);

    IOC_ConnArgs_T connArgs = {.SrvURI = srvURI, .Usage = IOC_LinkUsageEvtConsumer};
    IOC_LinkID_T conetLinkID = IOC_ID_INVALID;

    std::atomic<bool> connected{false};
    std::thread connThread([&] {
        IOC_Result_T connResult = IOC_connectService(&conetLinkID, &connArgs, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, connResult);
        connected = true;
    });

    IOC_LinkID_T srvLinkID = IOC_ID_INVALID;
    result = IOC_acceptClient(srvID, &srvLinkID, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result);
    connThread.join();
    ASSERT_TRUE(connected.load());
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    //===BEHAVIOR: Subscribe via ConetMode===
    EventCallbackHelper conetHelper;
    IOC_EvtID_T evtIDs[] = {IOC_EVTID_TEST_KEEPALIVE};
    IOC_SubEvtArgs_T conetSubArgs = {};
    conetSubArgs.EvtNum = 1;
    conetSubArgs.pEvtIDs = evtIDs;
    conetSubArgs.CbProcEvt_F = EventCallbackHelper::StaticCallback;
    conetSubArgs.pCbPrivData = &conetHelper;
    result = IOC_subEVT(conetLinkID, &conetSubArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result);

    EventStateSnapshot conetAfterSub = CaptureEventState(conetLinkID);

    //===BEHAVIOR: Subscribe via ConlesMode===
    EventCallbackHelper conlesHelper;
    IOC_SubEvtArgs_T conlesSubArgs = {};
    conlesSubArgs.EvtNum = 1;
    conlesSubArgs.pEvtIDs = evtIDs;
    conlesSubArgs.CbProcEvt_F = EventCallbackHelper::StaticCallback;
    conlesSubArgs.pCbPrivData = &conlesHelper;
    result = IOC_subEVT(IOC_CONLES_MODE_AUTO_LINK_ID, &conlesSubArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result);

    EventStateSnapshot conlesAfterSub = CaptureEventState(IOC_CONLES_MODE_AUTO_LINK_ID);

    //===VERIFY: Both show Ready state after subscription===
    ASSERT_EQ(IOC_RESULT_SUCCESS, conetAfterSub.QueryResult);
    EXPECT_EQ(IOC_LinkStateReady, conetAfterSub.MainState) << "ConetMode: Link Ready after subscription";
    EXPECT_EQ(IOC_LinkSubStateDefault, conetAfterSub.SubState) << "ConetMode: No EVT substates";

    ASSERT_EQ(IOC_RESULT_SUCCESS, conlesAfterSub.QueryResult);
    EXPECT_EQ(IOC_LinkStateReady, conlesAfterSub.MainState) << "ConlesMode: Auto-link Ready after subscription";
    EXPECT_EQ(IOC_LinkSubStateDefault, conlesAfterSub.SubState) << "ConlesMode: No EVT substates";

    // Key difference: ConetMode uses explicit link, ConlesMode uses auto-link
    EXPECT_NE(conetLinkID, IOC_CONLES_MODE_AUTO_LINK_ID) << "Different link models";

    //===CLEANUP===
    IOC_UnsubEvtArgs_T conetUnsubArgs = {};
    conetUnsubArgs.CbProcEvt_F = EventCallbackHelper::StaticCallback;
    conetUnsubArgs.pCbPrivData = &conetHelper;
    IOC_unsubEVT(conetLinkID, &conetUnsubArgs);

    IOC_UnsubEvtArgs_T conlesUnsubArgs = {};
    conlesUnsubArgs.CbProcEvt_F = EventCallbackHelper::StaticCallback;
    conlesUnsubArgs.pCbPrivData = &conlesHelper;
    IOC_unsubEVT(IOC_CONLES_MODE_AUTO_LINK_ID, &conlesUnsubArgs);

    if (conetLinkID != IOC_ID_INVALID) {
        IOC_closeLink(conetLinkID);
    }
    if (srvLinkID != IOC_ID_INVALID) {
        IOC_closeLink(srvLinkID);
    }
    if (srvID != IOC_ID_INVALID) {
        IOC_offlineService(srvID);
    }
}

/**
 * @test TC7_compareEventState_stateTracking_expectMainStatesOnly
 * @brief Verify both modes use only main states (Level 2), no substates (Level 3)
 *
 * @architecture State tracking consistency:
 *   - Both modes: Track Ready, BusyCbProcEvt, BusySubEvt, BusyUnsubEvt
 *   - Both modes: SubState always Default (0)
 *   - Difference: ConetMode has Level 1 (connection state), ConlesMode doesn't
 *
 * @steps
 *   1. Setup both modes
 *   2. Perform operations in both modes
 *   3. Verify both use main states only
 *   4. Verify both have SubState = Default
 *   5. Cleanup
 */
TEST(UT_ConetEventState_Comparison, TC7_compareEventState_stateTracking_expectMainStatesOnly) {
    //===SETUP: ConetMode link - Service as producer===
    IOC_SrvURI_T srvURI = {.pProtocol = IOC_SRV_PROTO_FIFO,
                           .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
                           .pPath = (const char *)"ConetEvtState_TC7"};
    IOC_SrvArgs_T srvArgs = {.SrvURI = srvURI,
                             .Flags = IOC_SRVFLAG_NONE,
                             .UsageCapabilites = IOC_LinkUsageEvtProducer};  // Service sends events
    IOC_SrvID_T srvID = IOC_ID_INVALID;
    IOC_Result_T result = IOC_onlineService(&srvID, &srvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result);

    IOC_ConnArgs_T connArgs = {.SrvURI = srvURI, .Usage = IOC_LinkUsageEvtConsumer};  // Client receives events
    IOC_LinkID_T conetLinkID = IOC_ID_INVALID;
    std::atomic<bool> connected{false};
    std::thread connThread([&] {
        IOC_Result_T connResult = IOC_connectService(&conetLinkID, &connArgs, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, connResult);
        connected = true;
    });

    IOC_LinkID_T srvLinkID = IOC_ID_INVALID;
    result = IOC_acceptClient(srvID, &srvLinkID, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result);
    connThread.join();
    ASSERT_TRUE(connected.load());
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    //===BEHAVIOR: Query both modes===
    EventStateSnapshot conetState = CaptureEventState(srvLinkID);  // Query service link
    EventStateSnapshot conlesState = CaptureEventState(IOC_CONLES_MODE_AUTO_LINK_ID);

    //===VERIFY: Both modes use main states only, no substates===
    ASSERT_EQ(IOC_RESULT_SUCCESS, conetState.QueryResult);
    ASSERT_EQ(IOC_RESULT_SUCCESS, conlesState.QueryResult);

    // Both should be in Ready state initially
    EXPECT_EQ(IOC_LinkStateReady, conetState.MainState);
    EXPECT_EQ(IOC_LinkStateReady, conlesState.MainState);

    // Both should have Default substate (no EVT substates)
    EXPECT_EQ(IOC_LinkSubStateDefault, conetState.SubState) << "ConetMode: No Level 3 substates for EVT";
    EXPECT_EQ(IOC_LinkSubStateDefault, conlesState.SubState) << "ConlesMode: No Level 3 substates for EVT";

    // Architecture compliance: Both modes use same main state enum
    EXPECT_EQ(conetState.MainState, conlesState.MainState) << "Both modes use same operation state tracking";

    //===CLEANUP===
    if (conetLinkID != IOC_ID_INVALID) {
        IOC_closeLink(conetLinkID);
    }
    if (srvLinkID != IOC_ID_INVALID) {
        IOC_closeLink(srvLinkID);
    }
    if (srvID != IOC_ID_INVALID) {
        IOC_offlineService(srvID);
    }
}

/**
 * @test TC8_verifyArchitectureCompliance_noEVTSubstates_expectConsistent
 * @brief Comprehensive architectural verification: No EVT substates in either mode
 *
 * @architecture "Why No EVT SubStates" verification:
 *   - EVT operations are fundamentally different from CMD/DAT
 *   - No multi-step handshake protocol
 *   - Fire-and-forget or queue-based semantics
 *   - SubState provides no additional value for EVT
 *   - Architecture decision: EVT uses Level 2 only, no Level 3
 *
 * @steps
 *   1. Test various EVT operations in both modes
 *   2. Continuously monitor SubState
 *   3. Verify SubState NEVER changes from Default
 *   4. Document architectural compliance
 *   5. Cleanup
 */
TEST(UT_ConetEventState_Comparison, TC8_verifyArchitectureCompliance_noEVTSubstates_expectConsistent) {
    //===SETUP: ConetMode - Service as producer===
    IOC_SrvURI_T srvURI = {.pProtocol = IOC_SRV_PROTO_FIFO,
                           .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
                           .pPath = (const char *)"ConetEvtState_TC8"};
    IOC_SrvArgs_T srvArgs = {.SrvURI = srvURI,
                             .Flags = IOC_SRVFLAG_NONE,
                             .UsageCapabilites = IOC_LinkUsageEvtProducer};  // Service sends events
    IOC_SrvID_T srvID = IOC_ID_INVALID;
    IOC_Result_T result = IOC_onlineService(&srvID, &srvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result);

    IOC_ConnArgs_T connArgs = {.SrvURI = srvURI, .Usage = IOC_LinkUsageEvtConsumer};  // Client receives events
    IOC_LinkID_T conetLinkID = IOC_ID_INVALID;
    std::atomic<bool> connected{false};
    std::thread connThread([&] {
        IOC_Result_T connResult = IOC_connectService(&conetLinkID, &connArgs, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, connResult);
        connected = true;
    });

    IOC_LinkID_T srvLinkID = IOC_ID_INVALID;
    result = IOC_acceptClient(srvID, &srvLinkID, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result);
    connThread.join();
    ASSERT_TRUE(connected.load());
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    std::vector<EventStateSnapshot> conetSnapshots;
    std::vector<EventStateSnapshot> conlesSnapshots;

    //===BEHAVIOR: Perform comprehensive EVT operations===
    EventCallbackHelper helper;
    helper.AllowCallbackToProceed = true;

    // Initial state
    conetSnapshots.push_back(CaptureEventState(srvLinkID));  // Query service link
    conlesSnapshots.push_back(CaptureEventState(IOC_CONLES_MODE_AUTO_LINK_ID));

    // Subscribe operations
    IOC_EvtID_T evtIDs[] = {IOC_EVTID_TEST_KEEPALIVE};
    IOC_SubEvtArgs_T subArgsConet = {
        .EvtNum = 1, .pEvtIDs = evtIDs, .CbProcEvt_F = EventCallbackHelper::StaticCallback, .pCbPrivData = &helper};
    IOC_subEVT(conetLinkID, &subArgsConet);

    IOC_SubEvtArgs_T subArgsConles = {
        .EvtNum = 1, .pEvtIDs = evtIDs, .CbProcEvt_F = EventCallbackHelper::StaticCallback, .pCbPrivData = &helper};
    IOC_subEVT(IOC_CONLES_MODE_AUTO_LINK_ID, &subArgsConles);

    conetSnapshots.push_back(CaptureEventState(srvLinkID));
    conlesSnapshots.push_back(CaptureEventState(IOC_CONLES_MODE_AUTO_LINK_ID));

    // Post operations - Service posts
    IOC_EvtDesc_T evtDescConet = {.EvtID = IOC_EVTID_TEST_KEEPALIVE, .EvtValue = 88888};
    IOC_postEVT(srvLinkID, &evtDescConet, NULL);  // Service posts to client
    IOC_EvtDesc_T evtDescConles = {.EvtID = IOC_EVTID_TEST_KEEPALIVE, .EvtValue = 99999};
    IOC_postEVT(IOC_CONLES_MODE_AUTO_LINK_ID, &evtDescConles, NULL);

    conetSnapshots.push_back(CaptureEventState(srvLinkID));
    conlesSnapshots.push_back(CaptureEventState(IOC_CONLES_MODE_AUTO_LINK_ID));

    std::this_thread::sleep_for(std::chrono::milliseconds(100));  // Allow callbacks

    conetSnapshots.push_back(CaptureEventState(srvLinkID));
    conlesSnapshots.push_back(CaptureEventState(IOC_CONLES_MODE_AUTO_LINK_ID));

    // Unsubscribe operations
    IOC_UnsubEvtArgs_T unsubArgsConet = {.CbProcEvt_F = EventCallbackHelper::StaticCallback, .pCbPrivData = &helper};
    IOC_unsubEVT(conetLinkID, &unsubArgsConet);

    IOC_UnsubEvtArgs_T unsubArgsConles = {.CbProcEvt_F = EventCallbackHelper::StaticCallback, .pCbPrivData = &helper};
    IOC_unsubEVT(IOC_CONLES_MODE_AUTO_LINK_ID, &unsubArgsConles);

    conetSnapshots.push_back(CaptureEventState(conetLinkID));
    conlesSnapshots.push_back(CaptureEventState(IOC_CONLES_MODE_AUTO_LINK_ID));

    //===VERIFY: SubState ALWAYS Default in BOTH modes across ALL operations===
    for (size_t i = 0; i < conetSnapshots.size(); i++) {
        EXPECT_EQ(IOC_LinkSubStateDefault, conetSnapshots[i].SubState)
            << "ConetMode: SubState must be Default at snapshot " << i;
    }

    for (size_t i = 0; i < conlesSnapshots.size(); i++) {
        EXPECT_EQ(IOC_LinkSubStateDefault, conlesSnapshots[i].SubState)
            << "ConlesMode: SubState must be Default at snapshot " << i;
    }

    //===ARCHITECTURAL COMPLIANCE VERIFICATION===
    // This test confirms the design decision documented in README_ArchDesign-State.md:
    // "Why No EVT SubStates": EVT operations are fundamentally fire-and-forget or queue-based,
    // without the multi-step handshake protocols of CMD/DAT. Therefore, Level 3 (SubState)
    // tracking provides no additional value and is architecturally excluded for EVT.

    //===CLEANUP===
    if (conetLinkID != IOC_ID_INVALID) {
        IOC_closeLink(conetLinkID);
    }
    if (srvLinkID != IOC_ID_INVALID) {
        IOC_closeLink(srvLinkID);
    }
    if (srvID != IOC_ID_INVALID) {
        IOC_offlineService(srvID);
    }
}

/**************************************************************************************************
 * END OF TEST FILE
 **************************************************************************************************/
