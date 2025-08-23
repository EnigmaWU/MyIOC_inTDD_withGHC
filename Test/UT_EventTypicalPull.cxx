///////////////////////////////////////////////////////////////////////////////////////////////////
// Event Typical Pull (Polling-based / Conet) — UT
//
// Intent:
// - Focus on polling-based event consumption using IOC_pullEVT API.
// - Validate IOC_pullEVT as an alternative to callback-based IOC_subEVT.
// - Test various polling modes: non-blocking, blocking with timeout, infinite blocking.
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "_UT_IOC_Common.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF OVERVIEW OF THIS UNIT TESTING FILE===============================================
/**
 * @brief Verify polling-based event consumption flows using IOC_pullEVT:
 *  - Service posts events, client pulls events using IOC_pullEVT instead of callbacks.
 *  - Test different polling modes: non-blocking, timeout-based, infinite blocking.
 *  - Ensure compatibility with connection-oriented (Conet) event flows.
 *
 * Key concepts:
 *  - IOC_pullEVT: Polling-based event consumption (alternative to IOC_subEVT callbacks).
 *  - Polling modes: Non-blocking (immediate return), blocking with timeout, infinite blocking.
 *  - Event delivery: First-come-first-served when mixed with IOC_subEVT.
 */
//======>END OF OVERVIEW OF THIS UNIT TESTING FILE=================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF UNIT TESTING DESIGN==============================================================
/**
 * Design focus:
 *  - IOC_pullEVT API validation across different polling modes.
 *  - Performance and behavior under various timeout configurations.
 *  - Interaction with callback-based IOC_subEVT on the same LinkID.
 *  - Resource management and cleanup for polling-based consumers.
 */
///////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF USER STORY=======================================================================
/**
 * US-1: As a service EvtConsumer, I want to pull events using polling mode
 *       so that I can control when and how events are consumed without callbacks.
 *
 * US-2: As a service EvtConsumer, I want configurable polling behavior (blocking/non-blocking)
 *       so that I can adapt event consumption to different performance requirements.
 *
 * US-3: As a service EvtConsumer, I want to mix polling and callback-based consumption
 *       so that I can use the most appropriate method for different event types.
 */
//======>END OF USER STORY==========================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//=======>BEGIN OF ACCEPTANCE CRITERIA==============================================================
/**
 * [@US-1]
 *  AC-1: GIVEN a Conet service (producer) and a client (consumer) with no IOC_subEVT subscription,
 *        WHEN the service posts an event and client calls IOC_pullEVT,
 *        THEN the client successfully receives the event via polling.
 *  AC-2: GIVEN multiple events posted by service,
 *        WHEN client repeatedly calls IOC_pullEVT,
 *        THEN client receives events in FIFO order.
 *
 * [@US-2]
 *  AC-1: GIVEN no events available,
 *        WHEN client calls IOC_pullEVT with non-blocking mode (timeout=0),
 *        THEN IOC_RESULT_NO_EVENT_CONSUMER is returned immediately.
 *  AC-2: GIVEN events posted after a delay,
 *        WHEN client calls IOC_pullEVT with blocking timeout,
 *        THEN client receives event within timeout or gets IOC_RESULT_TIMEOUT.
 *  AC-3: GIVEN events posted after a delay,
 *        WHEN client calls IOC_pullEVT with infinite timeout,
 *        THEN client waits indefinitely until event is received.
 *
 * [@US-3]
 *  AC-1: GIVEN both IOC_subEVT callback and IOC_pullEVT polling on same LinkID,
 *        WHEN service posts events,
 *        THEN each event is delivered to only one consumer (first-come-first-served).
 *  AC-2: GIVEN mixed polling and callback consumers,
 *        WHEN service posts multiple events,
 *        THEN events are distributed fairly between consumers.
 */
//=======>END OF ACCEPTANCE CRITERIA================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF TEST CASES=======================================================================

/**
 * [@AC-1,US-1] TC-1: verifyPullEVT_byBasicPolling_expectEventReceived
 * Test: verifyPullEVT_byBasicPolling_expectEventReceived
 * Purpose: Validate basic IOC_pullEVT functionality for polling-based event consumption.
 * Steps:
 *   1) Online service (EvtProducer); client connects (EvtConsumer) without IOC_subEVT.
 *   2) Service posts an event to the client link.
 *   3) Client calls IOC_pullEVT to retrieve the event.
 *   4) Assert event details match what was posted.
 *
 * [@AC-2,US-1] TC-1: verifyPullEVT_byMultipleEvents_expectFIFOOrder
 * Test: verifyPullEVT_byMultipleEvents_expectFIFOOrder
 * Purpose: Ensure IOC_pullEVT delivers multiple events in FIFO order.
 * Steps:
 *   1) Service posts multiple events with sequential values.
 *   2) Client calls IOC_pullEVT repeatedly to retrieve all events.
 *   3) Assert events are received in the same order as posted.
 *   4) Verify sequence IDs are strictly increasing.
 *
 * [@AC-1,US-2] TC-1: verifyPullEVT_byNonBlockingMode_expectImmediateReturn
 * Test: verifyPullEVT_byNonBlockingMode_expectImmediateReturn
 * Purpose: Validate non-blocking IOC_pullEVT behavior when no events are available.
 * Steps:
 *   1) Client connects without any events posted.
 *   2) Client calls IOC_pullEVT with timeout=0 (non-blocking).
 *   3) Assert IOC_RESULT_NO_EVENT_CONSUMER returned immediately.
 *   4) Measure response time to ensure it's truly non-blocking.
 *
 * [@AC-2,US-2] TC-1: verifyPullEVT_byBlockingTimeout_expectTimeoutBehavior
 * Test: verifyPullEVT_byBlockingTimeout_expectTimeoutBehavior
 * Purpose: Validate IOC_pullEVT timeout behavior in blocking mode.
 * Steps:
 *   1) Client calls IOC_pullEVT with specific timeout value.
 *   2) No events are posted during timeout period.
 *   3) Assert IOC_RESULT_TIMEOUT returned after timeout expires.
 *   4) Verify actual timeout duration matches expected duration.
 *
 * [@AC-3,US-2] TC-1: verifyPullEVT_byInfiniteTimeout_expectEventualSuccess
 * Test: verifyPullEVT_byInfiniteTimeout_expectEventualSuccess
 * Purpose: Validate IOC_pullEVT infinite blocking behavior.
 * Steps:
 *   1) Client calls IOC_pullEVT with infinite timeout in separate thread.
 *   2) Service posts event after a delay.
 *   3) Assert client receives event successfully.
 *   4) Verify client waited until event was available.
 *
 * [@AC-1,US-3] TC-1: verifyPullEVT_withMixedConsumers_expectFirstComeFirstServed
 * Test: verifyPullEVT_withMixedConsumers_expectFirstComeFirstServed
 * Purpose: Validate event distribution when mixing IOC_pullEVT and IOC_subEVT consumers.
 * Steps:
 *   1) Set up both callback-based (IOC_subEVT) and polling-based (IOC_pullEVT) consumers.
 *   2) Service posts events while both consumers are active.
 *   3) Assert each event is delivered to only one consumer.
 *   4) Verify no events are duplicated or lost.
 *
 * [@AC-2,US-3] TC-1: verifyPullEVT_withMultiplePollingConsumers_expectFairDistribution
 * Test: verifyPullEVT_withMultiplePollingConsumers_expectFairDistribution
 * Purpose: Validate fair event distribution among multiple IOC_pullEVT consumers.
 * Steps:
 *   1) Set up multiple clients using IOC_pullEVT on different links.
 *   2) Service posts events to all links.
 *   3) All clients poll simultaneously for events.
 *   4) Assert events are distributed fairly and completely.
 */
//======>END OF TEST CASES=========================================================================

// Test callback for mixed polling/callback scenarios
typedef struct __PullTestContext {
    std::atomic<int> CallbackEventCount{0};
    std::atomic<int> PullEventCount{0};
    std::vector<IOC_EvtID_T> CallbackEvents;
    std::vector<IOC_EvtID_T> PullEvents;
    std::mutex EventMutex;
} __PullTestContext_T;

static IOC_Result_T __PullTest_CallbackHandler(const IOC_EvtDesc_pT pEvtDesc, void *pCbPriv) {
    __PullTestContext_T *pContext = (__PullTestContext_T *)pCbPriv;
    if (!pContext || !pEvtDesc) return IOC_RESULT_INVALID_PARAM;

    std::lock_guard<std::mutex> lock(pContext->EventMutex);
    pContext->CallbackEvents.push_back(IOC_EvtDesc_getEvtID((IOC_EvtDesc_pT)pEvtDesc));
    pContext->CallbackEventCount++;
    return IOC_RESULT_SUCCESS;
}

// [@AC-1,US-1] TC-1: verifyPullEVT_byBasicPolling_expectEventReceived
TEST(UT_ConetEventTypical, verifyPullEVT_byBasicPolling_expectEventReceived) {
    IOC_Result_T R = IOC_RESULT_BUG;

    // Service setup (Conet producer)
    IOC_SrvURI_T SrvURI = {.pProtocol = IOC_SRV_PROTO_FIFO,
                           .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
                           .pPath = (const char *)"EvtPull_BasicPolling"};
    IOC_SrvArgs_T SrvArgs = {.SrvURI = SrvURI, .Flags = IOC_SRVFLAG_NONE, .UsageCapabilites = IOC_LinkUsageEvtProducer};
    IOC_SrvID_T SrvID = IOC_ID_INVALID;
    R = IOC_onlineService(&SrvID, &SrvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, R);

    // Client setup (Conet consumer) - no IOC_subEVT, only IOC_pullEVT
    IOC_ConnArgs_T ConnArgs = {.SrvURI = SrvURI, .Usage = IOC_LinkUsageEvtConsumer};
    IOC_LinkID_T CliLinkID = IOC_ID_INVALID;
    std::thread CliThread([&] {
        IOC_Result_T R_thread = IOC_connectService(&CliLinkID, &ConnArgs, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, R_thread);
        ASSERT_NE(IOC_ID_INVALID, CliLinkID);
    });

    // Accept the client
    IOC_LinkID_T SrvLinkID = IOC_ID_INVALID;
    R = IOC_acceptClient(SrvID, &SrvLinkID, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, R);
    ASSERT_NE(IOC_ID_INVALID, SrvLinkID);

    // Wait for client connection
    if (CliThread.joinable()) CliThread.join();

    // Service posts an event
    IOC_EvtDesc_T PostedEvt = {};
    PostedEvt.EvtID = IOC_EVTID_TEST_KEEPALIVE;
    PostedEvt.EvtValue = 100;
    R = IOC_postEVT(SrvLinkID, &PostedEvt, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, R);

    // Client pulls the event using IOC_pullEVT
    IOC_EvtDesc_T PulledEvt = {};
    R = IOC_pullEVT(CliLinkID, &PulledEvt, NULL);  // Default non-blocking mode
    ASSERT_EQ(IOC_RESULT_SUCCESS, R);

    // Verify event details
    ASSERT_EQ(IOC_EVTID_TEST_KEEPALIVE, IOC_EvtDesc_getEvtID(&PulledEvt));
    ASSERT_EQ((ULONG_T)100, IOC_EvtDesc_getEvtValue(&PulledEvt));

    // Cleanup
    if (CliLinkID != IOC_ID_INVALID) IOC_closeLink(CliLinkID);
    if (SrvLinkID != IOC_ID_INVALID) IOC_closeLink(SrvLinkID);
    if (SrvID != IOC_ID_INVALID) IOC_offlineService(SrvID);
}

// [@AC-2,US-1] TC-1: verifyPullEVT_byMultipleEvents_expectFIFOOrder
TEST(UT_ConetEventTypical, verifyPullEVT_byMultipleEvents_expectFIFOOrder) {
    IOC_Result_T R = IOC_RESULT_BUG;
    const int NumEvents = 5;

    // Service setup
    IOC_SrvURI_T SrvURI = {.pProtocol = IOC_SRV_PROTO_FIFO,
                           .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
                           .pPath = (const char *)"EvtPull_FIFOOrder"};
    IOC_SrvArgs_T SrvArgs = {.SrvURI = SrvURI, .Flags = IOC_SRVFLAG_NONE, .UsageCapabilites = IOC_LinkUsageEvtProducer};
    IOC_SrvID_T SrvID = IOC_ID_INVALID;
    R = IOC_onlineService(&SrvID, &SrvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, R);

    // Client setup
    IOC_ConnArgs_T ConnArgs = {.SrvURI = SrvURI, .Usage = IOC_LinkUsageEvtConsumer};
    IOC_LinkID_T CliLinkID = IOC_ID_INVALID;
    std::thread CliThread([&] {
        IOC_Result_T R_thread = IOC_connectService(&CliLinkID, &ConnArgs, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, R_thread);
    });

    // Accept client
    IOC_LinkID_T SrvLinkID = IOC_ID_INVALID;
    R = IOC_acceptClient(SrvID, &SrvLinkID, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, R);
    if (CliThread.joinable()) CliThread.join();

    // Post multiple events in sequence
    std::vector<ULONG_T> PostedValues;
    for (int i = 0; i < NumEvents; ++i) {
        IOC_EvtDesc_T EvtDesc = {};
        EvtDesc.EvtID = IOC_EVTID_TEST_KEEPALIVE;
        EvtDesc.EvtValue = 200 + i;  // 200, 201, 202, 203, 204
        PostedValues.push_back(EvtDesc.EvtValue);

        R = IOC_postEVT(SrvLinkID, &EvtDesc, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, R);
    }

    // Pull all events and verify FIFO order
    std::vector<ULONG_T> PulledValues;
    std::vector<ULONG_T> PulledSequences;
    for (int i = 0; i < NumEvents; ++i) {
        IOC_EvtDesc_T PulledEvt = {};
        R = IOC_pullEVT(CliLinkID, &PulledEvt, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, R) << "Failed to pull event " << i;

        PulledValues.push_back(IOC_EvtDesc_getEvtValue(&PulledEvt));
        PulledSequences.push_back(IOC_EvtDesc_getSeqID(&PulledEvt));
    }

    // Verify FIFO order
    ASSERT_EQ(PostedValues, PulledValues) << "Events not received in FIFO order";

    // Verify sequence IDs are strictly increasing
    for (int i = 1; i < NumEvents; ++i) {
        ASSERT_LT(PulledSequences[i - 1], PulledSequences[i]) << "Sequence IDs not strictly increasing";
    }

    // Verify no more events available
    IOC_EvtDesc_T ExtraEvt = {};
    R = IOC_pullEVT(CliLinkID, &ExtraEvt, NULL);
    ASSERT_EQ(IOC_RESULT_NO_EVENT_CONSUMER, R) << "Unexpected extra event found";

    // Cleanup
    if (CliLinkID != IOC_ID_INVALID) IOC_closeLink(CliLinkID);
    if (SrvLinkID != IOC_ID_INVALID) IOC_closeLink(SrvLinkID);
    if (SrvID != IOC_ID_INVALID) IOC_offlineService(SrvID);
}

// [@AC-1,US-2] TC-1: verifyPullEVT_byNonBlockingMode_expectImmediateReturn
TEST(UT_ConetEventTypical, verifyPullEVT_byNonBlockingMode_expectImmediateReturn) {
    IOC_Result_T R = IOC_RESULT_BUG;

    // Service setup
    IOC_SrvURI_T SrvURI = {.pProtocol = IOC_SRV_PROTO_FIFO,
                           .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
                           .pPath = (const char *)"EvtPull_NonBlocking"};
    IOC_SrvArgs_T SrvArgs = {.SrvURI = SrvURI, .Flags = IOC_SRVFLAG_NONE, .UsageCapabilites = IOC_LinkUsageEvtProducer};
    IOC_SrvID_T SrvID = IOC_ID_INVALID;
    R = IOC_onlineService(&SrvID, &SrvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, R);

    // Client setup
    IOC_ConnArgs_T ConnArgs = {.SrvURI = SrvURI, .Usage = IOC_LinkUsageEvtConsumer};
    IOC_LinkID_T CliLinkID = IOC_ID_INVALID;
    std::thread CliThread([&] {
        IOC_Result_T R_thread = IOC_connectService(&CliLinkID, &ConnArgs, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, R_thread);
    });

    // Accept client
    IOC_LinkID_T SrvLinkID = IOC_ID_INVALID;
    R = IOC_acceptClient(SrvID, &SrvLinkID, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, R);
    if (CliThread.joinable()) CliThread.join();

    // Try to pull event when none are available (non-blocking mode)
    auto startTime = std::chrono::high_resolution_clock::now();

    IOC_EvtDesc_T PulledEvt = {};
    IOC_Options_T Options[] = {{.IDs = IOC_OPTID_TIMEOUT, .Payload.TimeoutUS = 0}};  // Non-blocking
    R = IOC_pullEVT(CliLinkID, &PulledEvt, Options);

    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

    // Verify immediate return with correct result
    ASSERT_EQ(IOC_RESULT_NO_EVENT_CONSUMER, R) << "Should return NO_EVENT_CONSUMER when no events available";
    ASSERT_LT(duration.count(), 50) << "Non-blocking call took too long: " << duration.count() << "ms";

    // Cleanup
    if (CliLinkID != IOC_ID_INVALID) IOC_closeLink(CliLinkID);
    if (SrvLinkID != IOC_ID_INVALID) IOC_closeLink(SrvLinkID);
    if (SrvID != IOC_ID_INVALID) IOC_offlineService(SrvID);
}

// [@AC-2,US-2] TC-1: verifyPullEVT_byBlockingTimeout_expectTimeoutBehavior
TEST(UT_ConetEventTypical, verifyPullEVT_byBlockingTimeout_expectTimeoutBehavior) {
    IOC_Result_T R = IOC_RESULT_BUG;
    const ULONG_T TimeoutUS = 100000;  // 100ms timeout

    // Service setup
    IOC_SrvURI_T SrvURI = {.pProtocol = IOC_SRV_PROTO_FIFO,
                           .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
                           .pPath = (const char *)"EvtPull_BlockingTimeout"};
    IOC_SrvArgs_T SrvArgs = {.SrvURI = SrvURI, .Flags = IOC_SRVFLAG_NONE, .UsageCapabilites = IOC_LinkUsageEvtProducer};
    IOC_SrvID_T SrvID = IOC_ID_INVALID;
    R = IOC_onlineService(&SrvID, &SrvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, R);

    // Client setup
    IOC_ConnArgs_T ConnArgs = {.SrvURI = SrvURI, .Usage = IOC_LinkUsageEvtConsumer};
    IOC_LinkID_T CliLinkID = IOC_ID_INVALID;
    std::thread CliThread([&] {
        IOC_Result_T R_thread = IOC_connectService(&CliLinkID, &ConnArgs, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, R_thread);
    });

    // Accept client
    IOC_LinkID_T SrvLinkID = IOC_ID_INVALID;
    R = IOC_acceptClient(SrvID, &SrvLinkID, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, R);
    if (CliThread.joinable()) CliThread.join();

    // Try to pull with timeout (no events posted)
    auto startTime = std::chrono::high_resolution_clock::now();

    IOC_EvtDesc_T PulledEvt = {};
    IOC_Options_T Options[] = {{.IDs = IOC_OPTID_TIMEOUT, .Payload.TimeoutUS = TimeoutUS}};
    R = IOC_pullEVT(CliLinkID, &PulledEvt, Options);

    auto endTime = std::chrono::high_resolution_clock::now();
    auto actualDuration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);

    // Verify timeout behavior
    ASSERT_EQ(IOC_RESULT_TIMEOUT, R) << "Should return TIMEOUT when no events within timeout";
    ASSERT_GE(actualDuration.count(), TimeoutUS * 0.8) << "Timeout occurred too early";
    ASSERT_LE(actualDuration.count(), TimeoutUS * 1.5) << "Timeout occurred too late";

    // Cleanup
    if (CliLinkID != IOC_ID_INVALID) IOC_closeLink(CliLinkID);
    if (SrvLinkID != IOC_ID_INVALID) IOC_closeLink(SrvLinkID);
    if (SrvID != IOC_ID_INVALID) IOC_offlineService(SrvID);
}

// [@AC-3,US-2] TC-1: verifyPullEVT_byInfiniteTimeout_expectEventualSuccess
TEST(UT_ConetEventTypical, verifyPullEVT_byInfiniteTimeout_expectEventualSuccess) {
    IOC_Result_T R = IOC_RESULT_BUG;
    const int DelayMS = 50;  // Event will be posted after 50ms

    // Service setup
    IOC_SrvURI_T SrvURI = {.pProtocol = IOC_SRV_PROTO_FIFO,
                           .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
                           .pPath = (const char *)"EvtPull_InfiniteTimeout"};
    IOC_SrvArgs_T SrvArgs = {.SrvURI = SrvURI, .Flags = IOC_SRVFLAG_NONE, .UsageCapabilites = IOC_LinkUsageEvtProducer};
    IOC_SrvID_T SrvID = IOC_ID_INVALID;
    R = IOC_onlineService(&SrvID, &SrvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, R);

    // Client setup
    IOC_ConnArgs_T ConnArgs = {.SrvURI = SrvURI, .Usage = IOC_LinkUsageEvtConsumer};
    IOC_LinkID_T CliLinkID = IOC_ID_INVALID;
    std::thread CliThread([&] {
        IOC_Result_T R_thread = IOC_connectService(&CliLinkID, &ConnArgs, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, R_thread);
    });

    // Accept client
    IOC_LinkID_T SrvLinkID = IOC_ID_INVALID;
    R = IOC_acceptClient(SrvID, &SrvLinkID, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, R);
    if (CliThread.joinable()) CliThread.join();

    // Start thread to post event after delay
    std::atomic<bool> EventPosted{false};
    std::thread PostThread([&] {
        std::this_thread::sleep_for(std::chrono::milliseconds(DelayMS));
        IOC_EvtDesc_T EvtDesc = {};
        EvtDesc.EvtID = IOC_EVTID_TEST_KEEPALIVE;
        EvtDesc.EvtValue = 300;
        IOC_Result_T R_post = IOC_postEVT(SrvLinkID, &EvtDesc, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, R_post);
        EventPosted = true;
    });

    // Pull with infinite timeout - should wait until event is posted
    auto startTime = std::chrono::high_resolution_clock::now();

    IOC_EvtDesc_T PulledEvt = {};
    IOC_Options_T Options[] = {{.IDs = IOC_OPTID_TIMEOUT, .Payload.TimeoutUS = IOC_TIMEOUT_INFINITE}};
    R = IOC_pullEVT(CliLinkID, &PulledEvt, Options);

    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

    // Verify successful event reception
    ASSERT_EQ(IOC_RESULT_SUCCESS, R) << "Should successfully receive event with infinite timeout";
    ASSERT_EQ(IOC_EVTID_TEST_KEEPALIVE, IOC_EvtDesc_getEvtID(&PulledEvt));
    ASSERT_EQ((ULONG_T)300, IOC_EvtDesc_getEvtValue(&PulledEvt));
    ASSERT_TRUE(EventPosted.load()) << "Event should have been posted";
    ASSERT_GE(duration.count(), DelayMS * 0.8) << "Should have waited for event";

    // Cleanup
    if (PostThread.joinable()) PostThread.join();
    if (CliLinkID != IOC_ID_INVALID) IOC_closeLink(CliLinkID);
    if (SrvLinkID != IOC_ID_INVALID) IOC_closeLink(SrvLinkID);
    if (SrvID != IOC_ID_INVALID) IOC_offlineService(SrvID);
}

// [@AC-1,US-3] TC-1: verifyPullEVT_withMixedConsumers_expectFirstComeFirstServed
TEST(UT_ConetEventTypical, verifyPullEVT_withMixedConsumers_expectFirstComeFirstServed) {
    IOC_Result_T R = IOC_RESULT_BUG;

    // Service setup
    IOC_SrvURI_T SrvURI = {.pProtocol = IOC_SRV_PROTO_FIFO,
                           .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
                           .pPath = (const char *)"EvtPull_MixedConsumers"};
    IOC_SrvArgs_T SrvArgs = {.SrvURI = SrvURI, .Flags = IOC_SRVFLAG_NONE, .UsageCapabilites = IOC_LinkUsageEvtProducer};
    IOC_SrvID_T SrvID = IOC_ID_INVALID;
    R = IOC_onlineService(&SrvID, &SrvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, R);

    // Client setup
    IOC_ConnArgs_T ConnArgs = {.SrvURI = SrvURI, .Usage = IOC_LinkUsageEvtConsumer};
    IOC_LinkID_T CliLinkID = IOC_ID_INVALID;
    std::thread CliThread([&] {
        IOC_Result_T R_thread = IOC_connectService(&CliLinkID, &ConnArgs, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, R_thread);
    });

    // Accept client
    IOC_LinkID_T SrvLinkID = IOC_ID_INVALID;
    R = IOC_acceptClient(SrvID, &SrvLinkID, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, R);
    if (CliThread.joinable()) CliThread.join();

    // Set up mixed consumers: callback + polling
    __PullTestContext_T TestContext = {};

    // Set up callback consumer
    static IOC_EvtID_T SubEvtIDs[1] = {IOC_EVTID_TEST_KEEPALIVE};
    IOC_SubEvtArgs_T Sub = {
        .CbProcEvt_F = __PullTest_CallbackHandler, .pCbPrivData = &TestContext, .EvtNum = 1, .pEvtIDs = &SubEvtIDs[0]};
    R = IOC_subEVT(CliLinkID, &Sub);
    ASSERT_EQ(IOC_RESULT_SUCCESS, R);

    // Post multiple events
    const int NumEvents = 10;
    for (int i = 0; i < NumEvents; ++i) {
        IOC_EvtDesc_T EvtDesc = {};
        EvtDesc.EvtID = IOC_EVTID_TEST_KEEPALIVE;
        EvtDesc.EvtValue = 400 + i;
        R = IOC_postEVT(SrvLinkID, &EvtDesc, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, R);

        // Alternate between callback processing and polling
        if (i % 2 == 0) {
            // Let callback handle this event
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        } else {
            // Try to pull this event
            IOC_EvtDesc_T PulledEvt = {};
            IOC_Result_T PullResult = IOC_pullEVT(CliLinkID, &PulledEvt, NULL);
            if (PullResult == IOC_RESULT_SUCCESS) {
                std::lock_guard<std::mutex> lock(TestContext.EventMutex);
                TestContext.PullEvents.push_back(IOC_EvtDesc_getEvtID(&PulledEvt));
                TestContext.PullEventCount++;
            }
        }
    }

    // Wait for callback processing
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Verify that all events were consumed (no duplicates, no losses)
    int TotalConsumed = TestContext.CallbackEventCount.load() + TestContext.PullEventCount.load();
    ASSERT_EQ(NumEvents, TotalConsumed) << "Not all events were consumed";

    // Verify no events are left
    IOC_EvtDesc_T ExtraEvt = {};
    R = IOC_pullEVT(CliLinkID, &ExtraEvt, NULL);
    ASSERT_EQ(IOC_RESULT_NO_EVENT_CONSUMER, R) << "Unexpected remaining events";

    // Cleanup
    if (CliLinkID != IOC_ID_INVALID) IOC_closeLink(CliLinkID);
    if (SrvLinkID != IOC_ID_INVALID) IOC_closeLink(SrvLinkID);
    if (SrvID != IOC_ID_INVALID) IOC_offlineService(SrvID);
}

//======>END OF TEST CASES==========================================================================
