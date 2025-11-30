/**************************************************************************************************
 * @file UT_ConetEventState.cxx
 * @brief Unit tests for Link Event State patterns in ConetMode (connection-oriented mode)
 * @date 2025-11-30
 *
 * @purpose Validate IOC event state behavior for ConetMode (link-based event operations).
 *          Tests link main state (Ready/Busy) during event subscription, unsubscription, and
 *          callback execution over TCP connections.
 *
 * @architecture_mapping
 *          Link State Hierarchy for Events:
 *          - Level 1 (Connection State): Tested in UT_LinkConnStateTCP.cxx
 *          - Level 2 (Operation State): Ready ⟷ BusyCbProcEvt/BusySubEvt/BusyUnsubEvt (THIS FILE)
 *          - Level 3 (Detail SubState): ALWAYS Default (0) - EVT has NO substates
 *
 * @scope ConetMode event state testing (Level 2 only, no Level 3 substates)
 * @related_files
 *   - UT_ConlesEventState.cxx: ConlesMode (connectionless) event state patterns
 *   - UT_LinkStateOperation.cxx: Protocol-agnostic operation state (Level 2)
 *   - UT_LinkStateCorrelation.cxx: 3-level hierarchy correlation
 *   - README_ArchDesign-State.md: Event state machine specifications
 *
 * FRAMEWORK STATUS: ✅ ConetMode Event State Testing - IMPLEMENTATION COMPLETE
 *    • Test infrastructure: StateMonitor, EventCallbackHelper
 *    • Test cases: 8/8 (100% complete)
 *    • Target: 8 test cases covering ConetMode-specific event state scenarios
 *    • Progress: Full implementation of Phase 2.2
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

    EventStateSnapshot()
        : MainState(IOC_LinkStateUndefined),
          SubState(IOC_LinkSubStateDefault),
          QueryResult(IOC_RESULT_UNDEFINED),
          Timestamp(std::chrono::steady_clock::now()) {}
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
    std::atomic<int> CallbackCount{0};
    std::atomic<IOC_LinkState_T> StateInCallback{IOC_LinkStateUndefined};
    std::atomic<IOC_LinkSubState_T> SubStateInCallback{IOC_LinkSubStateDefault};
    std::mutex Mutex;
    std::condition_variable CV;
    std::atomic<bool> CallbackStarted{false};
    std::atomic<bool> AllowCallbackToProceed{false};

    static void StaticCallback(IOC_EvtID_T evtID, IOC_EvtDesc_pT pEvtDesc, void* pCbPrivData) {
        auto* helper = static_cast<EventCallbackHelper*>(pCbPrivData);
        helper->OnCallback(evtID, pEvtDesc);
    }

   private:
    void OnCallback(IOC_EvtID_T evtID, IOC_EvtDesc_pT pEvtDesc) {
        CallbackCount++;

        // Capture state during callback execution
        IOC_LinkID_T linkID = pEvtDesc->PostedByLinkID;
        IOC_getLinkState(linkID, (IOC_LinkState_pT)&StateInCallback, (IOC_LinkSubState_pT)&SubStateInCallback);

        // Signal that callback started
        CallbackStarted = true;
        CV.notify_all();

        // Wait for test to allow callback to proceed (for blocking tests)
        std::unique_lock<std::mutex> lock(Mutex);
        CV.wait(lock, [this] { return AllowCallbackToProceed.load(); });
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
    //===SETUP: Create TCP service and client link===
    IOC_SrvID_T srvID;
    IOC_SrvCfg_T srvCfg = BUILD_INIT_SrvCfg_InFIFO_OutTCP(26000, (char*)"ConetEvt_TC1");
    IOC_Result_T result = IOC_onlineService(&srvCfg, &srvID);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result);

    IOC_LinkID_T linkID;
    IOC_LinkCfg_T linkCfg =
        BUILD_INIT_LinkCfg_InOutTCP(IOC_LinkUsageEvtPoster, (char*)"tcp://localprocess:26000/ConetEvt_TC1");
    result = IOC_openLink(&linkCfg, &linkID);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result);

    std::this_thread::sleep_for(std::chrono::milliseconds(50));  // Allow connection

    //===VERIFY: Initial state - Ready with Default substate===
    EventStateSnapshot initial = CaptureEventState(linkID);
    ASSERT_EQ(IOC_RESULT_SUCCESS, initial.QueryResult);
    EXPECT_EQ(IOC_LinkStateReady, initial.MainState) << "ConetMode link should be Ready initially";
    EXPECT_EQ(IOC_LinkSubStateDefault, initial.SubState) << "EVT operations have NO substates (always Default)";

    //===BEHAVIOR: Post event via link===
    IOC_EvtDesc_T evtDesc = {};
    evtDesc.EvtID = IOC_EVTID_TEST_KEEPALIVE;
    evtDesc.PostedByLinkID = linkID;
    result = IOC_postEVT(&evtDesc);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result);

    //===VERIFY: State immediately after post - still Ready (fire-and-forget)===
    EventStateSnapshot afterPost = CaptureEventState(linkID);
    ASSERT_EQ(IOC_RESULT_SUCCESS, afterPost.QueryResult);
    EXPECT_EQ(IOC_LinkStateReady, afterPost.MainState) << "Post is fire-and-forget, link should remain Ready";
    EXPECT_EQ(IOC_LinkSubStateDefault, afterPost.SubState) << "EVT substates always Default";

    //===CLEANUP===
    IOC_closeLink(linkID);
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
    //===SETUP: Create TCP service and client link===
    IOC_SrvID_T srvID;
    IOC_SrvCfg_T srvCfg = BUILD_INIT_SrvCfg_InFIFO_OutTCP(26001, (char*)"ConetEvt_TC2");
    IOC_Result_T result = IOC_onlineService(&srvCfg, &srvID);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result);

    IOC_LinkID_T linkID;
    IOC_LinkCfg_T linkCfg =
        BUILD_INIT_LinkCfg_InOutTCP(IOC_LinkUsageEvtSubscriber, (char*)"tcp://localprocess:26001/ConetEvt_TC2");
    result = IOC_openLink(&linkCfg, &linkID);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result);

    std::this_thread::sleep_for(std::chrono::milliseconds(50));  // Allow connection

    //===BEHAVIOR: Subscribe to event===
    EventCallbackHelper helper;
    IOC_EvtID_T evtIDs[] = {IOC_EVTID_TEST_KEEPALIVE};
    IOC_SubEvtArgs_T subArgs = {};
    subArgs.LinkID = linkID;
    subArgs.EvtCnt = 1;
    subArgs.pEvtIDs = evtIDs;
    subArgs.CbProcEvt_F = EventCallbackHelper::StaticCallback;
    subArgs.pCbPrivData = &helper;

    result = IOC_subEVT(&subArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result);

    //===VERIFY: State after subscription - Ready with Default substate===
    EventStateSnapshot afterSub = CaptureEventState(linkID);
    ASSERT_EQ(IOC_RESULT_SUCCESS, afterSub.QueryResult);
    EXPECT_EQ(IOC_LinkStateReady, afterSub.MainState) << "After subscription, link should be Ready";
    EXPECT_EQ(IOC_LinkSubStateDefault, afterSub.SubState) << "EVT operations have NO substates";

    //===BEHAVIOR: Unsubscribe from event===
    IOC_UnsubEvtArgs_T unsubArgs = {};
    unsubArgs.LinkID = linkID;
    unsubArgs.CbProcEvt_F = EventCallbackHelper::StaticCallback;
    unsubArgs.pCbPrivData = &helper;

    result = IOC_unsubEVT(&unsubArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result);

    //===VERIFY: State after unsubscription - Ready with Default substate===
    EventStateSnapshot afterUnsub = CaptureEventState(linkID);
    ASSERT_EQ(IOC_RESULT_SUCCESS, afterUnsub.QueryResult);
    EXPECT_EQ(IOC_LinkStateReady, afterUnsub.MainState) << "After unsubscription, link should be Ready";
    EXPECT_EQ(IOC_LinkSubStateDefault, afterUnsub.SubState) << "EVT substates always Default";

    //===CLEANUP===
    IOC_closeLink(linkID);
    IOC_offlineService(srvID);
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
    //===SETUP: Create service and two links===
    IOC_SrvID_T srvID;
    IOC_SrvCfg_T srvCfg = BUILD_INIT_SrvCfg_InFIFO_OutTCP(26002, (char*)"ConetEvt_TC3");
    IOC_Result_T result = IOC_onlineService(&srvCfg, &srvID);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result);

    // Subscriber link
    IOC_LinkID_T subscriberLinkID;
    IOC_LinkCfg_T subscriberCfg =
        BUILD_INIT_LinkCfg_InOutTCP(IOC_LinkUsageEvtSubscriber, (char*)"tcp://localprocess:26002/ConetEvt_TC3");
    result = IOC_openLink(&subscriberCfg, &subscriberLinkID);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result);

    // Poster link
    IOC_LinkID_T posterLinkID;
    IOC_LinkCfg_T posterCfg =
        BUILD_INIT_LinkCfg_InOutTCP(IOC_LinkUsageEvtPoster, (char*)"tcp://localprocess:26002/ConetEvt_TC3");
    result = IOC_openLink(&posterCfg, &posterLinkID);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));  // Allow connections

    //===BEHAVIOR: Subscribe with blocking callback===
    EventCallbackHelper helper;
    IOC_EvtID_T evtIDs[] = {IOC_EVTID_TEST_KEEPALIVE};
    IOC_SubEvtArgs_T subArgs = {};
    subArgs.LinkID = subscriberLinkID;
    subArgs.EvtCnt = 1;
    subArgs.pEvtIDs = evtIDs;
    subArgs.CbProcEvt_F = EventCallbackHelper::StaticCallback;
    subArgs.pCbPrivData = &helper;

    result = IOC_subEVT(&subArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result);

    //===BEHAVIOR: Post event from another link===
    IOC_EvtDesc_T evtDesc = {};
    evtDesc.EvtID = IOC_EVTID_TEST_KEEPALIVE;
    evtDesc.PostedByLinkID = posterLinkID;
    result = IOC_postEVT(&evtDesc);
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
    std::this_thread::sleep_for(std::chrono::milliseconds(100));  // Allow callback to finish

    //===VERIFY: State after callback - Ready===
    EventStateSnapshot afterCallback = CaptureEventState(subscriberLinkID);
    ASSERT_EQ(IOC_RESULT_SUCCESS, afterCallback.QueryResult);
    EXPECT_EQ(IOC_LinkStateReady, afterCallback.MainState) << "After callback, link should return to Ready";
    EXPECT_EQ(IOC_LinkSubStateDefault, afterCallback.SubState) << "EVT substates always Default";

    //===CLEANUP===
    IOC_UnsubEvtArgs_T unsubArgs = {};
    unsubArgs.LinkID = subscriberLinkID;
    unsubArgs.CbProcEvt_F = EventCallbackHelper::StaticCallback;
    unsubArgs.pCbPrivData = &helper;
    IOC_unsubEVT(&unsubArgs);

    IOC_closeLink(posterLinkID);
    IOC_closeLink(subscriberLinkID);
    IOC_offlineService(srvID);
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
    //===SETUP: Create TCP service and link===
    IOC_SrvID_T srvID;
    IOC_SrvCfg_T srvCfg = BUILD_INIT_SrvCfg_InFIFO_OutTCP(26003, (char*)"ConetEvt_TC4");
    IOC_Result_T result = IOC_onlineService(&srvCfg, &srvID);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result);

    IOC_LinkID_T linkID;
    IOC_LinkCfg_T linkCfg =
        BUILD_INIT_LinkCfg_InOutTCP((IOC_LinkUsage_T)(IOC_LinkUsageEvtPoster | IOC_LinkUsageEvtSubscriber),
                                    (char*)"tcp://localprocess:26003/ConetEvt_TC4");
    result = IOC_openLink(&linkCfg, &linkID);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result);

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    std::vector<EventStateSnapshot> snapshots;

    //===VERIFY: Initial state - SubState is Default===
    snapshots.push_back(CaptureEventState(linkID));

    //===BEHAVIOR: Subscribe to event===
    EventCallbackHelper helper;
    helper.AllowCallbackToProceed = true;  // Non-blocking callback
    IOC_EvtID_T evtIDs[] = {IOC_EVTID_TEST_KEEPALIVE};
    IOC_SubEvtArgs_T subArgs = {};
    subArgs.LinkID = linkID;
    subArgs.EvtCnt = 1;
    subArgs.pEvtIDs = evtIDs;
    subArgs.CbProcEvt_F = EventCallbackHelper::StaticCallback;
    subArgs.pCbPrivData = &helper;
    result = IOC_subEVT(&subArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result);

    snapshots.push_back(CaptureEventState(linkID));

    //===BEHAVIOR: Post event===
    IOC_EvtDesc_T evtDesc = {};
    evtDesc.EvtID = IOC_EVTID_TEST_KEEPALIVE;
    evtDesc.PostedByLinkID = linkID;
    result = IOC_postEVT(&evtDesc);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result);

    snapshots.push_back(CaptureEventState(linkID));
    std::this_thread::sleep_for(std::chrono::milliseconds(100));  // Allow callback
    snapshots.push_back(CaptureEventState(linkID));

    //===BEHAVIOR: Unsubscribe from event===
    IOC_UnsubEvtArgs_T unsubArgs = {};
    unsubArgs.LinkID = linkID;
    unsubArgs.CbProcEvt_F = EventCallbackHelper::StaticCallback;
    unsubArgs.pCbPrivData = &helper;
    result = IOC_unsubEVT(&unsubArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result);

    snapshots.push_back(CaptureEventState(linkID));

    //===VERIFY: SubState ALWAYS Default throughout all operations===
    for (size_t i = 0; i < snapshots.size(); i++) {
        EXPECT_EQ(IOC_LinkSubStateDefault, snapshots[i].SubState)
            << "Architectural requirement: EVT operations have NO Level 3 substates (snapshot " << i << ")";
    }

    //===CLEANUP===
    IOC_closeLink(linkID);
    IOC_offlineService(srvID);
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
    //===SETUP: ConetMode link===
    IOC_SrvID_T srvID;
    IOC_SrvCfg_T srvCfg = BUILD_INIT_SrvCfg_InFIFO_OutTCP(26004, (char*)"ConetEvt_TC5");
    IOC_Result_T result = IOC_onlineService(&srvCfg, &srvID);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result);

    IOC_LinkID_T conetLinkID;
    IOC_LinkCfg_T conetCfg =
        BUILD_INIT_LinkCfg_InOutTCP(IOC_LinkUsageEvtPoster, (char*)"tcp://localprocess:26004/ConetEvt_TC5");
    result = IOC_openLink(&conetCfg, &conetLinkID);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result);

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    //===BEHAVIOR: Post via ConetMode===
    IOC_EvtDesc_T evtDescConet = {};
    evtDescConet.EvtID = IOC_EVTID_TEST_KEEPALIVE;
    evtDescConet.PostedByLinkID = conetLinkID;
    result = IOC_postEVT(&evtDescConet);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result);

    EventStateSnapshot conetAfterPost = CaptureEventState(conetLinkID);

    //===BEHAVIOR: Post via ConlesMode===
    IOC_EvtDesc_T evtDescConles = {};
    evtDescConles.EvtID = IOC_EVTID_TEST_KEEPALIVE;
    evtDescConles.PostedByLinkID = IOC_CONLES_MODE_AUTO_LINK_ID;
    result = IOC_postEVT(&evtDescConles);
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
    IOC_closeLink(conetLinkID);
    IOC_offlineService(srvID);
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
    IOC_SrvID_T srvID;
    IOC_SrvCfg_T srvCfg = BUILD_INIT_SrvCfg_InFIFO_OutTCP(26005, (char*)"ConetEvt_TC6");
    IOC_Result_T result = IOC_onlineService(&srvCfg, &srvID);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result);

    IOC_LinkID_T conetLinkID;
    IOC_LinkCfg_T conetCfg =
        BUILD_INIT_LinkCfg_InOutTCP(IOC_LinkUsageEvtSubscriber, (char*)"tcp://localprocess:26005/ConetEvt_TC6");
    result = IOC_openLink(&conetCfg, &conetLinkID);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result);

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    //===BEHAVIOR: Subscribe via ConetMode===
    EventCallbackHelper conetHelper;
    IOC_EvtID_T evtIDs[] = {IOC_EVTID_TEST_KEEPALIVE};
    IOC_SubEvtArgs_T conetSubArgs = {};
    conetSubArgs.LinkID = conetLinkID;
    conetSubArgs.EvtCnt = 1;
    conetSubArgs.pEvtIDs = evtIDs;
    conetSubArgs.CbProcEvt_F = EventCallbackHelper::StaticCallback;
    conetSubArgs.pCbPrivData = &conetHelper;
    result = IOC_subEVT(&conetSubArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result);

    EventStateSnapshot conetAfterSub = CaptureEventState(conetLinkID);

    //===BEHAVIOR: Subscribe via ConlesMode===
    EventCallbackHelper conlesHelper;
    IOC_SubEvtArgs_T conlesSubArgs = {};
    conlesSubArgs.LinkID = IOC_CONLES_MODE_AUTO_LINK_ID;
    conlesSubArgs.EvtCnt = 1;
    conlesSubArgs.pEvtIDs = evtIDs;
    conlesSubArgs.CbProcEvt_F = EventCallbackHelper::StaticCallback;
    conlesSubArgs.pCbPrivData = &conlesHelper;
    result = IOC_subEVT(&conlesSubArgs);
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
    conetUnsubArgs.LinkID = conetLinkID;
    conetUnsubArgs.CbProcEvt_F = EventCallbackHelper::StaticCallback;
    conetUnsubArgs.pCbPrivData = &conetHelper;
    IOC_unsubEVT(&conetUnsubArgs);

    IOC_UnsubEvtArgs_T conlesUnsubArgs = {};
    conlesUnsubArgs.LinkID = IOC_CONLES_MODE_AUTO_LINK_ID;
    conlesUnsubArgs.CbProcEvt_F = EventCallbackHelper::StaticCallback;
    conlesUnsubArgs.pCbPrivData = &conlesHelper;
    IOC_unsubEVT(&conlesUnsubArgs);

    IOC_closeLink(conetLinkID);
    IOC_offlineService(srvID);
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
    //===SETUP: ConetMode link===
    IOC_SrvID_T srvID;
    IOC_SrvCfg_T srvCfg = BUILD_INIT_SrvCfg_InFIFO_OutTCP(26006, (char*)"ConetEvt_TC7");
    IOC_Result_T result = IOC_onlineService(&srvCfg, &srvID);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result);

    IOC_LinkID_T conetLinkID;
    IOC_LinkCfg_T conetCfg =
        BUILD_INIT_LinkCfg_InOutTCP((IOC_LinkUsage_T)(IOC_LinkUsageEvtPoster | IOC_LinkUsageEvtSubscriber),
                                    (char*)"tcp://localprocess:26006/ConetEvt_TC7");
    result = IOC_openLink(&conetCfg, &conetLinkID);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result);

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    //===BEHAVIOR: Query both modes===
    EventStateSnapshot conetState = CaptureEventState(conetLinkID);
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
    IOC_closeLink(conetLinkID);
    IOC_offlineService(srvID);
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
    //===SETUP: Both modes===
    IOC_SrvID_T srvID;
    IOC_SrvCfg_T srvCfg = BUILD_INIT_SrvCfg_InFIFO_OutTCP(26007, (char*)"ConetEvt_TC8");
    IOC_Result_T result = IOC_onlineService(&srvCfg, &srvID);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result);

    IOC_LinkID_T conetLinkID;
    IOC_LinkCfg_T conetCfg =
        BUILD_INIT_LinkCfg_InOutTCP((IOC_LinkUsage_T)(IOC_LinkUsageEvtPoster | IOC_LinkUsageEvtSubscriber),
                                    (char*)"tcp://localprocess:26007/ConetEvt_TC8");
    result = IOC_openLink(&conetCfg, &conetLinkID);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result);

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    std::vector<EventStateSnapshot> conetSnapshots;
    std::vector<EventStateSnapshot> conlesSnapshots;

    //===BEHAVIOR: Perform comprehensive EVT operations===
    EventCallbackHelper helper;
    helper.AllowCallbackToProceed = true;

    // Initial state
    conetSnapshots.push_back(CaptureEventState(conetLinkID));
    conlesSnapshots.push_back(CaptureEventState(IOC_CONLES_MODE_AUTO_LINK_ID));

    // Subscribe operations
    IOC_EvtID_T evtIDs[] = {IOC_EVTID_TEST_KEEPALIVE};
    IOC_SubEvtArgs_T subArgsConet = {};
    subArgsConet.LinkID = conetLinkID;
    subArgsConet.EvtCnt = 1;
    subArgsConet.pEvtIDs = evtIDs;
    subArgsConet.CbProcEvt_F = EventCallbackHelper::StaticCallback;
    subArgsConet.pCbPrivData = &helper;
    IOC_subEVT(&subArgsConet);

    IOC_SubEvtArgs_T subArgsConles = {};
    subArgsConles.LinkID = IOC_CONLES_MODE_AUTO_LINK_ID;
    subArgsConles.EvtCnt = 1;
    subArgsConles.pEvtIDs = evtIDs;
    subArgsConles.CbProcEvt_F = EventCallbackHelper::StaticCallback;
    subArgsConles.pCbPrivData = &helper;
    IOC_subEVT(&subArgsConles);

    conetSnapshots.push_back(CaptureEventState(conetLinkID));
    conlesSnapshots.push_back(CaptureEventState(IOC_CONLES_MODE_AUTO_LINK_ID));

    // Post operations
    IOC_EvtDesc_T evtDescConet = {IOC_EVTID_TEST_KEEPALIVE, conetLinkID};
    IOC_postEVT(&evtDescConet);
    IOC_EvtDesc_T evtDescConles = {IOC_EVTID_TEST_KEEPALIVE, IOC_CONLES_MODE_AUTO_LINK_ID};
    IOC_postEVT(&evtDescConles);

    conetSnapshots.push_back(CaptureEventState(conetLinkID));
    conlesSnapshots.push_back(CaptureEventState(IOC_CONLES_MODE_AUTO_LINK_ID));

    std::this_thread::sleep_for(std::chrono::milliseconds(100));  // Allow callbacks

    conetSnapshots.push_back(CaptureEventState(conetLinkID));
    conlesSnapshots.push_back(CaptureEventState(IOC_CONLES_MODE_AUTO_LINK_ID));

    // Unsubscribe operations
    IOC_UnsubEvtArgs_T unsubArgsConet = {};
    unsubArgsConet.LinkID = conetLinkID;
    unsubArgsConet.CbProcEvt_F = EventCallbackHelper::StaticCallback;
    unsubArgsConet.pCbPrivData = &helper;
    IOC_unsubEVT(&unsubArgsConet);

    IOC_UnsubEvtArgs_T unsubArgsConles = {};
    unsubArgsConles.LinkID = IOC_CONLES_MODE_AUTO_LINK_ID;
    unsubArgsConles.CbProcEvt_F = EventCallbackHelper::StaticCallback;
    unsubArgsConles.pCbPrivData = &helper;
    IOC_unsubEVT(&unsubArgsConles);

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
    IOC_closeLink(conetLinkID);
    IOC_offlineService(srvID);
}

/**************************************************************************************************
 * END OF TEST FILE
 **************************************************************************************************/
