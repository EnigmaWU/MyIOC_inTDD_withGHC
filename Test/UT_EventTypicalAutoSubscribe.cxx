///////////////////////////////////////////////////////////////////////////////////////////////////
// Event Typical Auto-Subscribe (Conet) — UT skeleton
//
// Intent:
//   Verify auto-subscribe behavior for event-enabled services and clients via UsageArgs.pEvt.
//   Covers both client-side (ConnArgs) and service-side (SrvArgs) auto-subscription mechanisms.
//   Focus on connection-oriented (Conet) event flows with automatic subscription setup.
///////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF OVERVIEW OF THIS UNIT TESTING FILE===============================================
/**
 * @brief Verify auto-subscribe behavior for typical event flows (Conet):
 *  - Client-side auto-subscribe: IOC_connectService + ConnArgs.UsageArgs.pEvt → automatic IOC_subEVT
 *  - Service-side auto-subscribe: IOC_acceptClient + SrvArgs.UsageArgs.pEvt → automatic IOC_subEVT
 *  - Covers both EvtProducer and EvtConsumer service roles with auto-subscription capabilities.
 *
 *-------------------------------------------------------------------------------------------------
 * Key concepts:
 *  - Auto-subscribe reduces manual IOC_subEVT calls and ensures subscription consistency.
 *  - Mirrors DAT/CMD auto-wiring pattern via UsageArgs for unified API experience.
 *  - Maintains backward compatibility: UsageArgs.pEvt == NULL retains manual subscription requirement.
 *  - Focus on connection-oriented (Conet) flows; connection-less (Conles) is separate.
 *
 * API Contract:
 *  CLIENT-SIDE: If ConnArgs.Usage == IOC_LinkUsageEvtConsumer && ConnArgs.UsageArgs.pEvt != NULL,
 *               then IOC_connectService MUST call IOC_subEVT(LinkID, UsageArgs.pEvt) after connect.
 *  SERVICE-SIDE: If SrvArgs.UsageCapabilites has EvtConsumer && SrvArgs.UsageArgs.pEvt != NULL,
 *                then IOC_acceptClient MUST call IOC_subEVT(AcceptedLinkID, UsageArgs.pEvt) after accept.
 */
//======>END OF OVERVIEW OF THIS UNIT TESTING FILE=================================================

#include "_UT_IOC_Common.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF UNIT TESTING DESIGN==============================================================
/**
 * Design focus:
 *  - Client-side auto-subscribe (ConnArgs.UsageArgs.pEvt) for EvtConsumer connections.
 *  - Service-side auto-subscribe (SrvArgs.UsageArgs.pEvt) for EvtConsumer services.
 *  - Error handling and cleanup when auto-subscribe fails.
 *  - Backward compatibility with manual subscription workflows.
 *  - Multi-client isolation and per-link event delivery with auto-subscribe.
 *
 * Out of scope:
 *  - Broadcast event auto-subscribe (tested in UT_EventBroadcast).
 *  - Connection-less (Conles) auto-subscribe patterns.
 *  - DAT/CMD auto-wiring interactions (separate capabilities).
 */
//======>END OF UNIT TESTING DESIGN================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF USER STORY=======================================================================
/**
 * US-1: As a client EvtConsumer, I want to connect and auto-subscribe to events in one step
 *       so that I can reduce boilerplate code and avoid missing subscription calls.
 *
 * US-2: As a service EvtConsumer, I want to auto-subscribe to client events upon accept
 *       so that server-side event handling is automatically configured per connection.
 *
 * US-3: As a developer working with multiple IOC capabilities (Events, Data, Commands),
 *       I want Event auto-subscribe to follow the same UsageArgs pattern as DAT/CMD auto-wiring
 *       so that I can apply consistent knowledge across all IOC features and reduce learning curve.
 *
 * US-4: As a system integrator, I want auto-subscribe failures to prevent connection establishment
 *       so that partially configured links don't cause runtime issues.
 */
//======>END OF USER STORY==========================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//=======>BEGIN OF ACCEPTANCE CRITERIA==============================================================
/**
 * [@US-1] Client-side Auto-Subscribe
 *  AC-1: GIVEN ConnArgs.Usage == EvtConsumer AND ConnArgs.UsageArgs.pEvt != NULL,
 *         WHEN IOC_connectService is called,
 *         THEN connection succeeds AND auto-subscribe occurs AND events are delivered.
 *  AC-2: GIVEN ConnArgs.UsageArgs.pEvt == NULL,
 *         WHEN IOC_connectService is called,
 *         THEN connection succeeds AND no auto-subscribe occurs AND manual IOC_subEVT is required.
 *  AC-3: GIVEN multiple clients with different UsageArgs.pEvt configurations,
 *         WHEN each connects with auto-subscribe,
 *         THEN each receives only its own subscribed events (isolation).
 *  AC-4: GIVEN auto-subscribe fails during IOC_connectService,
 *         WHEN connection is attempted,
 *         THEN IOC_connectService returns error AND link is cleaned up AND no partial state.
 *
 * [@US-2] Service-side Auto-Subscribe
 *  AC-1: GIVEN SrvArgs.UsageCapabilites has EvtConsumer AND SrvArgs.UsageArgs.pEvt != NULL,
 *         WHEN IOC_acceptClient is called,
 *         THEN accept succeeds AND auto-subscribe occurs AND client events are received.
 *  AC-2: GIVEN SrvArgs.UsageArgs.pEvt == NULL,
 *         WHEN IOC_acceptClient is called,
 *         THEN accept succeeds AND no auto-subscribe occurs AND manual IOC_subEVT is required.
 *  AC-3: GIVEN service accepts multiple clients with auto-subscribe,
 *         WHEN each client posts events,
 *         THEN service receives events from all clients with proper link isolation.
 *  AC-4: GIVEN auto-subscribe fails during IOC_acceptClient,
 *         WHEN accept is attempted,
 *         THEN IOC_acceptClient returns error AND link is cleaned up AND no partial state.
 *
 * [@US-3] API Consistency and Pattern Unification
 *  AC-1: GIVEN a developer familiar with DAT auto-wiring (SrvArgs.UsageArgs.pDat),
 *         WHEN they use Event auto-subscribe (SrvArgs.UsageArgs.pEvt),
 *         THEN the configuration pattern, error handling, and lifecycle should be identical.
 *  AC-2: GIVEN both client and service use UsageArgs.pEvt for auto-subscribe,
 *         WHEN auto-subscribe occurs,
 *         THEN both follow the same error codes, cleanup behavior, and state management as DAT/CMD.
 *  AC-3: GIVEN a service with mixed capabilities (EvtProducer + DatReceiver + CmdExecutor),
 *         WHEN clients connect with different Usage types,
 *         THEN each UsageArgs (pEvt, pDat, pCmd) works independently with consistent patterns.
 *  AC-4: GIVEN NULL UsageArgs across different capabilities,
 *         WHEN connections are established,
 *         THEN all capabilities (EVT, DAT, CMD) consistently require manual setup when UsageArgs is NULL.
 *
 * [@US-4] Error Handling and Robustness
 *  AC-1: GIVEN invalid event IDs in UsageArgs.pEvt,
 *         WHEN auto-subscribe is attempted,
 *         THEN connection/accept fails with appropriate error code.
 *  AC-2: GIVEN service shutdown during auto-subscribe,
 *         WHEN auto-subscribe is in progress,
 *         THEN operation fails gracefully without resource leaks.
 */
//=======>END OF ACCEPTANCE CRITERIA================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF TEST CASES=======================================================================
/**
 * [@AC-1,US-1] TC-1: Client Auto-Subscribe Success
 *  Test: verifyClientAutoSubscribe_byConnArgsUsageArgsEvt_expectDelivered
 *  Purpose: Validate basic client-side auto-subscribe with event delivery.
 *  Steps:
 *    1) Online service (EvtProducer capability).
 *    2) Prepare ConnArgs with Usage=EvtConsumer and UsageArgs.pEvt set.
 *    3) Call IOC_connectService; expect success and automatic subscription.
 *    4) Service posts event; verify client callback receives it.
 *
 * [@AC-2,US-1] TC-1: No Auto-Subscribe When UsageArgs.pEvt is NULL
 *  Test: verifyNoAutoSubscribe_byNullUsageArgsEvt_expectManualRequired
 *  Purpose: Ensure backward compatibility when UsageArgs.pEvt is not set.
 *  Steps:
 *    1) Online service (EvtProducer capability).
 *    2) Connect client with Usage=EvtConsumer but UsageArgs.pEvt=NULL.
 *    3) Service posts event; verify no delivery (no subscription).
 *    4) Manually call IOC_subEVT; verify event delivery works.
 *
 * [@AC-3,US-1] TC-1: Multi-Client Auto-Subscribe Isolation
 *  Test: verifyMultiClientAutoSubscribe_byDifferentEvtIDs_expectIsolation
 *  Purpose: Ensure per-client isolation with different auto-subscribe configurations.
 *  Steps:
 *    1) Online service (EvtProducer capability).
 *    2) Connect N clients, each with different event IDs in UsageArgs.pEvt.
 *    3) Service posts multiple event types.
 *    4) Verify each client receives only its subscribed events.
 *
 * [@AC-4,US-1] TC-1: Auto-Subscribe Failure Cleanup
 *  Test: verifyAutoSubscribeFailure_byInvalidEvtIDs_expectConnectionFails
 *  Purpose: Validate cleanup when auto-subscribe fails during connect.
 *  Steps:
 *    1) Online service (EvtProducer capability).
 *    2) Prepare ConnArgs with invalid event IDs in UsageArgs.pEvt.
 *    3) Call IOC_connectService; expect failure.
 *    4) Verify no link created, no resources leaked.
 *
 * [@AC-1,US-2] TC-1: Service Auto-Subscribe Success
 *  Test: verifyServiceAutoSubscribe_bySrvArgsUsageArgsEvt_expectClientEvtReceived
 *  Purpose: Validate service-side auto-subscribe when accepting clients.
 *  Steps:
 *    1) Online service with EvtConsumer capability and SrvArgs.UsageArgs.pEvt set.
 *    2) Client connects as EvtProducer.
 *    3) Call IOC_acceptClient; expect success and automatic subscription.
 *    4) Client posts event; verify service callback receives it.
 *
 * [@AC-2,US-2] TC-1: No Service Auto-Subscribe When UsageArgs.pEvt is NULL
 *  Test: verifyNoServiceAutoSubscribe_byNullSrvUsageArgsEvt_expectManualRequired
 *  Purpose: Ensure service-side backward compatibility.
 *  Steps:
 *    1) Online service with EvtConsumer capability but SrvArgs.UsageArgs.pEvt=NULL.
 *    2) Accept client connection.
 *    3) Client posts event; verify no delivery (no subscription).
 *    4) Manually call IOC_subEVT on accepted link; verify event delivery works.
 *
 * [@AC-3,US-2] TC-1: Service Multi-Client Auto-Subscribe
 *  Test: verifyServiceMultiClientAutoSubscribe_byMultipleAccepts_expectAllEvtReceived
 *  Purpose: Validate service receives events from multiple auto-subscribed clients.
 *  Steps:
 *    1) Online service with EvtConsumer capability and SrvArgs.UsageArgs.pEvt set.
 *    2) Accept N client connections with auto-subscribe.
 *    3) Each client posts unique events.
 *    4) Verify service receives all events with proper link identification.
 *
 * [@AC-1,US-3] TC-1: Event Auto-Subscribe Follows DAT Pattern
 *  Test: verifyEvtAutoSubscribePattern_matchesDatAutoWiring_expectConsistentAPI
 *  Purpose: Ensure Event auto-subscribe follows the exact same API pattern as DAT auto-wiring.
 *  Steps:
 *    1) Compare SrvArgs.UsageArgs.pDat setup with SrvArgs.UsageArgs.pEvt setup.
 *    2) Verify both use identical configuration approach (callback + private data + capability-specific args).
 *    3) Verify both return identical error codes for similar failure scenarios.
 *    4) Confirm both have same lifecycle (setup → auto-wire → cleanup).
 *
 * [@AC-2,US-3] TC-1: Consistent Error Handling Across Capabilities
 *  Test: verifyConsistentErrorHandling_acrossEvtDatCmd_expectSameErrorCodes
 *  Purpose: Validate that Event auto-subscribe uses same error patterns as DAT/CMD.
 *  Steps:
 *    1) Test invalid UsageArgs scenarios for EVT, DAT, and CMD capabilities.
 *    2) Verify all return same error codes (e.g., IOC_RESULT_INVALID_PARAM).
 *    3) Verify all perform same cleanup actions on failure.
 *    4) Confirm all leave system in same clean state after error.
 *
 * [@AC-3,US-3] TC-1: Mixed Capability Independence
 *  Test: verifyMixedCapabilityIndependence_byMultipleUsageArgs_expectIsolatedBehavior
 *  Purpose: Ensure different UsageArgs work independently but consistently.
 *  Steps:
 *    1) Online service with EvtProducer + DatReceiver + CmdExecutor capabilities.
 *    2) Set up SrvArgs.UsageArgs.pEvt, pDat, and pCmd simultaneously.
 *    3) Connect clients with different Usage types.
 *    4) Verify each auto-wiring works independently without interference.
 *
 * [@AC-4,US-3] TC-1: Consistent NULL UsageArgs Behavior
 *  Test: verifyNullUsageArgsConsistency_acrossAllCapabilities_expectUniformManualSetup
 *  Purpose: Ensure NULL UsageArgs behavior is consistent across EVT, DAT, CMD.
 *  Steps:
 *    1) Online service with mixed capabilities but all UsageArgs set to NULL.
 *    2) Connect clients for each capability type.
 *    3) Verify all connections succeed but require manual setup.
 *    4) Verify manual setup APIs work consistently for all capabilities.
 */
//======>END OF TEST CASES=========================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF TEST CASES (both client-side and service-side auto-subscribe are GREEN)

// [@AC-1,US-1] TC-1: Client Auto-Subscribe Success
/**
 * Test: verifyClientAutoSubscribe_byConnArgsUsageArgsEvt_expectDelivered
 * Purpose: Validate basic client-side auto-subscribe with event delivery.
 * Steps:
 *   1) Online service (EvtProducer capability).
 *   2) Prepare ConnArgs with Usage=EvtConsumer and UsageArgs.pEvt set.
 *   3) Call IOC_connectService; expect success and automatic subscription.
 *   4) Service posts event; verify client callback receives it.
 * Status: GREEN (client-side auto-subscribe is implemented and working).
 */
TEST(UT_ConetEventTypical, verifyClientAutoSubscribe_byConnArgsUsageArgsEvt_expectDelivered) {
    IOC_Result_T ResultValue = IOC_RESULT_BUG;

    // 📋 Test data setup
    IOC_SrvURI_T SrvURI = {.pProtocol = IOC_SRV_PROTO_FIFO,
                           .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
                           .pPath = (const char*)"EvtAutoSubscribe_ClientTest"};

    // 🔧 Setup service as EvtProducer (will send events to clients)
    IOC_SrvArgs_T SrvArgs = {.SrvURI = SrvURI, .Flags = IOC_SRVFLAG_NONE, .UsageCapabilites = IOC_LinkUsageEvtProducer};
    IOC_SrvID_T SrvID = IOC_ID_INVALID;
    ResultValue = IOC_onlineService(&SrvID, &SrvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue) << "Service should come online successfully";
    ASSERT_NE(IOC_ID_INVALID, SrvID) << "Service ID should be valid";

    // 🎯 Setup client event reception tracking
    struct {
        std::atomic<bool> EventReceived{false};
        std::atomic<int> ReceivedCount{0};
        IOC_EvtID_T ExpectedEvtID = IOC_EVTID_TEST_KEEPALIVE;
        IOC_EvtID_T ReceivedEvtID = 0;
        ULONG_T ReceivedEvtValue = 0;
    } ClientEventData;

    // Client event callback function
    auto ClientEventCallback = [](const IOC_EvtDesc_pT pEvtDesc, void* pPrivData) -> IOC_Result_T {
        auto* pEventData = static_cast<decltype(ClientEventData)*>(pPrivData);
        pEventData->EventReceived = true;
        pEventData->ReceivedCount++;
        pEventData->ReceivedEvtID = pEvtDesc->EvtID;
        pEventData->ReceivedEvtValue = pEvtDesc->EvtValue;
        printf("📨 Client received event: EvtID=%" PRIu64 ", EvtValue=%lu\n", pEvtDesc->EvtID, pEvtDesc->EvtValue);
        return IOC_RESULT_SUCCESS;
    };

    // 🚀 CRITICAL: Setup client with auto-subscribe via ConnArgs.UsageArgs.pEvt
    // This is the core functionality being tested - auto-subscribe should happen during IOC_connectService
    IOC_EvtID_T SubscribeEvtIDs[] = {IOC_EVTID_TEST_KEEPALIVE};
    IOC_EvtUsageArgs_T ClientEvtArgs = {
        .CbProcEvt_F = ClientEventCallback, .pCbPrivData = &ClientEventData, .EvtNum = 1, .pEvtIDs = SubscribeEvtIDs};

    IOC_ConnArgs_T ConnArgs = {
        .SrvURI = SrvURI,
        .Usage = IOC_LinkUsageEvtConsumer  // Client as event consumer
    };
    ConnArgs.UsageArgs.pEvt = &ClientEvtArgs;  // 🎯 AUTO-SUBSCRIBE: This should trigger automatic IOC_subEVT

    // Connect client in a separate thread to avoid blocking before accept
    IOC_LinkID_T CliLinkID = IOC_ID_INVALID;
    std::atomic<bool> ClientConnected{false};
    std::atomic<bool> AutoSubscribeExpected{false};

    std::thread ClientThread([&] {
        printf("🔗 Client connecting with auto-subscribe...\n");
        IOC_Result_T ThreadResult = IOC_connectService(&CliLinkID, &ConnArgs, NULL);

        // 🎯 EXPECTED BEHAVIOR: IOC_connectService should automatically call IOC_subEVT internally
        // when ConnArgs.Usage == IOC_LinkUsageEvtConsumer && ConnArgs.UsageArgs.pEvt != NULL
        ASSERT_EQ(IOC_RESULT_SUCCESS, ThreadResult) << "Client connection with auto-subscribe should succeed";
        ASSERT_NE(IOC_ID_INVALID, CliLinkID) << "Client link ID should be valid";

        ClientConnected = true;
        AutoSubscribeExpected = true;  // We expect auto-subscribe to have occurred
        printf("✅ Client connected with LinkID=%" PRIu64 " (auto-subscribe expected)\n", CliLinkID);
    });

    // Accept the client on service side
    IOC_LinkID_T SrvLinkID = IOC_ID_INVALID;
    ResultValue = IOC_acceptClient(SrvID, &SrvLinkID, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue) << "Service should accept client successfully";
    ASSERT_NE(IOC_ID_INVALID, SrvLinkID) << "Service link ID should be valid";

    // Wait for client connection to complete
    ClientThread.join();
    ASSERT_TRUE(ClientConnected.load()) << "Client should be connected";

    // 🎯 CRITICAL TEST: Verify auto-subscribe worked by posting event and checking delivery
    // If auto-subscribe worked, the client should receive this event WITHOUT manual IOC_subEVT
    printf("📤 Service posting event to test auto-subscribe...\n");
    IOC_EvtDesc_T EventToSend = {};
    EventToSend.EvtID = IOC_EVTID_TEST_KEEPALIVE;
    EventToSend.EvtValue = 12345;  // Test value

    ResultValue = IOC_postEVT(SrvLinkID, &EventToSend, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue) << "Service should post event successfully";

    // Wait for event delivery (auto-subscribe should have made this possible)
    printf("⏳ Waiting for auto-subscribed event delivery...\n");
    bool EventDelivered = false;
    for (int i = 0; i < 100 && !ClientEventData.EventReceived.load(); ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    EventDelivered = ClientEventData.EventReceived.load();

    // 🎯 KEY ASSERTION: This should PASS because client-side auto-subscribe is already implemented
    // IOC_connectService automatically calls IOC_subEVT when ConnArgs.UsageArgs.pEvt is set
    ASSERT_TRUE(EventDelivered)
        << "CLIENT AUTO-SUBSCRIBE FAILED: Event not received - IOC_connectService should auto-subscribe "
        << "when ConnArgs.UsageArgs.pEvt was set. Auto-subscribe feature may be broken.";

    ASSERT_EQ(1, ClientEventData.ReceivedCount.load()) << "Client should receive exactly one event via auto-subscribe";

    ASSERT_EQ(IOC_EVTID_TEST_KEEPALIVE, ClientEventData.ReceivedEvtID)
        << "Received event ID should match sent event ID";

    ASSERT_EQ(12345U, ClientEventData.ReceivedEvtValue) << "Received event value should match sent event value";

    printf("✅ AUTO-SUBSCRIBE SUCCESS: Client received event via automatic subscription\n");

    // Cleanup
    if (CliLinkID != IOC_ID_INVALID) {
        IOC_closeLink(CliLinkID);
    }
    if (SrvLinkID != IOC_ID_INVALID) {
        IOC_closeLink(SrvLinkID);
    }
    if (SrvID != IOC_ID_INVALID) {
        IOC_offlineService(SrvID);
    }
}

// [@AC-2,US-1] TC-1: No Auto-Subscribe When UsageArgs.pEvt is NULL
/**
 * Test: verifyNoAutoSubscribe_byNullUsageArgsEvt_expectManualRequired
 * Purpose: Ensure backward compatibility when UsageArgs.pEvt is not set.
 * Steps:
 *   1) Online service (EvtProducer capability).
 *   2) Connect client with Usage=EvtConsumer but UsageArgs.pEvt=NULL.
 *   3) Service posts event; verify no delivery (no subscription).
 *   4) Manually call IOC_subEVT; verify event delivery works.
 * Status: READY (can be implemented since client-side auto-subscribe is working).
 */
TEST(UT_ConetEventTypical, verifyNoAutoSubscribe_byNullUsageArgsEvt_expectManualRequired) {
    GTEST_SKIP() << "AUTO-SUBSCRIBE: Baseline manual subscription behavior validation ready to implement";
}

// [@AC-3,US-1] TC-1: Multi-Client Auto-Subscribe Isolation
/**
 * Test: verifyMultiClientAutoSubscribe_byDifferentEvtIDs_expectIsolation
 * Purpose: Ensure per-client isolation with different auto-subscribe configurations.
 * Steps:
 *   1) Online service (EvtProducer capability).
 *   2) Connect N clients, each with different event IDs in UsageArgs.pEvt.
 *   3) Service posts multiple event types.
 *   4) Verify each client receives only its subscribed events.
 * Status: READY (can be implemented since client-side auto-subscribe is working).
 */
TEST(UT_ConetEventTypical, verifyMultiClientAutoSubscribe_byDifferentEvtIDs_expectIsolation) {
    GTEST_SKIP() << "AUTO-SUBSCRIBE: Multi-client auto-subscribe isolation testing ready to implement";
}

// [@AC-4,US-1] TC-1: Auto-Subscribe Failure Cleanup
/**
 * Test: verifyAutoSubscribeFailure_byInvalidEvtIDs_expectConnectionFails
 * Purpose: Validate cleanup when auto-subscribe fails during connect.
 * Steps:
 *   1) Online service (EvtProducer capability).
 *   2) Prepare ConnArgs with invalid event IDs in UsageArgs.pEvt.
 *   3) Call IOC_connectService; expect failure.
 *   4) Verify no link created, no resources leaked.
 * Status: READY (can be implemented since client-side auto-subscribe is working).
 */
TEST(UT_ConetEventTypical, verifyAutoSubscribeFailure_byInvalidEvtIDs_expectConnectionFails) {
    GTEST_SKIP() << "AUTO-SUBSCRIBE: Error handling and cleanup validation ready to implement";
}

// [@AC-1,US-2] TC-1: Service Auto-Subscribe Success
/**
 * Test: verifyServiceAutoSubscribe_bySrvArgsUsageArgsEvt_expectClientEvtReceived
 * Purpose: Validate service-side auto-subscribe when accepting clients.
 * Steps:
 *   1) Online service with EvtConsumer capability and SrvArgs.UsageArgs.pEvt set.
 *   2) Client connects as EvtProducer.
 *   3) Call IOC_acceptClient; expect success and automatic subscription.
 *   4) Client posts event; verify service callback receives it.
 * Status: GREEN (service-side auto-subscribe is implemented and working).
 */
TEST(UT_ConetEventTypical, verifyServiceAutoSubscribe_bySrvArgsUsageArgsEvt_expectClientEvtReceived) {
    IOC_Result_T ResultValue = IOC_RESULT_BUG;

    // 📋 Test data setup for service as event consumer
    IOC_SrvURI_T SrvURI = {.pProtocol = IOC_SRV_PROTO_FIFO,
                           .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
                           .pPath = (const char*)"EvtAutoSubscribe_ServiceTest"};

    // 🎯 Setup service event reception tracking
    struct {
        std::atomic<bool> EventReceived{false};
        std::atomic<int> ReceivedCount{0};
        IOC_EvtID_T ExpectedEvtID = IOC_EVTID_TEST_KEEPALIVE;
        IOC_EvtID_T ReceivedEvtID = 0;
        ULONG_T ReceivedEvtValue = 0;
    } ServiceEventData;

    // Service event callback function
    auto ServiceEventCallback = [](const IOC_EvtDesc_pT pEvtDesc, void* pPrivData) -> IOC_Result_T {
        auto* pEventData = static_cast<decltype(ServiceEventData)*>(pPrivData);
        pEventData->EventReceived = true;
        pEventData->ReceivedCount++;
        pEventData->ReceivedEvtID = pEvtDesc->EvtID;
        pEventData->ReceivedEvtValue = pEvtDesc->EvtValue;
        printf("📨 Service received event: EvtID=%" PRIu64 ", EvtValue=%lu\n", pEvtDesc->EvtID, pEvtDesc->EvtValue);
        return IOC_RESULT_SUCCESS;
    };

    // 🚀 CRITICAL: Setup service with auto-subscribe via SrvArgs.UsageArgs.pEvt
    IOC_EvtID_T SubscribeEvtIDs[] = {IOC_EVTID_TEST_KEEPALIVE};
    IOC_EvtUsageArgs_T ServiceEvtArgs = {
        .CbProcEvt_F = ServiceEventCallback, .pCbPrivData = &ServiceEventData, .EvtNum = 1, .pEvtIDs = SubscribeEvtIDs};

    // 🔧 Setup service as EvtConsumer with auto-subscribe configuration
    IOC_SrvArgs_T SrvArgs = {.SrvURI = SrvURI, .Flags = IOC_SRVFLAG_NONE, .UsageCapabilites = IOC_LinkUsageEvtConsumer};
    SrvArgs.UsageArgs.pEvt =
        &ServiceEvtArgs;  // 🎯 AUTO-SUBSCRIBE: This should trigger automatic IOC_subEVT during accept

    IOC_SrvID_T SrvID = IOC_ID_INVALID;
    ResultValue = IOC_onlineService(&SrvID, &SrvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue) << "Service should come online successfully";
    ASSERT_NE(IOC_ID_INVALID, SrvID) << "Service ID should be valid";

    // Accept client connection in a separate thread
    IOC_LinkID_T SrvLinkID = IOC_ID_INVALID;
    std::atomic<bool> ClientAccepted{false};
    std::atomic<bool> AutoSubscribeExpected{false};

    std::thread ServiceThread([&] {
        printf("📞 Service waiting to accept client...\n");
        IOC_Result_T ThreadResult = IOC_acceptClient(SrvID, &SrvLinkID, NULL);

        // 🎯 EXPECTED BEHAVIOR: IOC_acceptClient should automatically call IOC_subEVT internally
        // when SrvArgs.UsageCapabilites has EvtConsumer && SrvArgs.UsageArgs.pEvt != NULL
        ASSERT_EQ(IOC_RESULT_SUCCESS, ThreadResult) << "Service should accept client with auto-subscribe";
        ASSERT_NE(IOC_ID_INVALID, SrvLinkID) << "Service link ID should be valid";

        ClientAccepted = true;
        AutoSubscribeExpected = true;  // We expect auto-subscribe to have occurred
        printf("✅ Service accepted client with LinkID=%" PRIu64 " (auto-subscribe expected)\n", SrvLinkID);
    });

    // Give service thread time to start waiting
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Connect as client EvtProducer
    IOC_ConnArgs_T ConnArgs = {
        .SrvURI = SrvURI,
        .Usage = IOC_LinkUsageEvtProducer  // Client as event producer
    };

    IOC_LinkID_T CliLinkID = IOC_ID_INVALID;
    printf("🔗 Client connecting as event producer...\n");
    ResultValue = IOC_connectService(&CliLinkID, &ConnArgs, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue) << "Client connection should succeed";
    ASSERT_NE(IOC_ID_INVALID, CliLinkID) << "Client link ID should be valid";
    printf("✅ Client connected with LinkID=%" PRIu64 "\n", CliLinkID);

    // Wait for accept to complete
    ServiceThread.join();
    ASSERT_TRUE(ClientAccepted) << "Service should have accepted the client";

    // Client posts event to test auto-subscribe
    IOC_EvtDesc_T EvtDesc = {.EvtID = IOC_EVTID_TEST_KEEPALIVE, .EvtValue = 67890};
    printf("📤 Client posting event to test service auto-subscribe...\n");
    ResultValue = IOC_postEVT(CliLinkID, &EvtDesc, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue) << "Client should be able to post event";

    // Wait for event delivery via auto-subscribe
    printf("⏳ Waiting for auto-subscribed event delivery to service...\n");
    auto EventDelivered = [&ServiceEventData]() { return ServiceEventData.EventReceived.load(); };

    bool DeliverySuccess = false;
    for (int i = 0; i < 100 && !DeliverySuccess; i++) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        DeliverySuccess = EventDelivered();
    }

    // 🎯 KEY ASSERTION: This should now PASS because service-side auto-subscribe is implemented
    // IOC_acceptClient automatically calls IOC_subEVT when SrvArgs.UsageArgs.pEvt is set
    ASSERT_TRUE(DeliverySuccess)
        << "SERVICE AUTO-SUBSCRIBE FAILED: Event not received - IOC_acceptClient should auto-subscribe "
        << "when SrvArgs.UsageArgs.pEvt was set. Service-side auto-subscribe implementation verified.";

    ASSERT_EQ(1, ServiceEventData.ReceivedCount.load())
        << "Service should receive exactly one event via auto-subscribe";

    ASSERT_EQ(IOC_EVTID_TEST_KEEPALIVE, ServiceEventData.ReceivedEvtID)
        << "Received event ID should match sent event ID";

    ASSERT_EQ(67890U, ServiceEventData.ReceivedEvtValue) << "Received event value should match sent event value";

    printf("✅ AUTO-SUBSCRIBE SUCCESS: Service received event via automatic subscription\n");

    // Cleanup
    if (CliLinkID != IOC_ID_INVALID) {
        IOC_closeLink(CliLinkID);
    }
    if (SrvLinkID != IOC_ID_INVALID) {
        IOC_closeLink(SrvLinkID);
    }
    if (SrvID != IOC_ID_INVALID) {
        IOC_offlineService(SrvID);
    }
}

// [@AC-2,US-2] TC-1: No Service Auto-Subscribe When UsageArgs.pEvt is NULL
/**
 * Test: verifyNoServiceAutoSubscribe_byNullSrvUsageArgsEvt_expectManualRequired
 * Purpose: Ensure service-side backward compatibility.
 * Steps:
 *   1) Online service with EvtConsumer capability but SrvArgs.UsageArgs.pEvt=NULL.
 *   2) Accept client connection.
 *   3) Client posts event; verify no delivery (no subscription).
 *   4) Manually call IOC_subEVT on accepted link; verify event delivery works.
 * Status: RED (service-side auto-subscribe baseline validation pending).
 */
TEST(UT_ConetEventTypical, verifyNoServiceAutoSubscribe_byNullSrvUsageArgsEvt_expectManualRequired) {
    GTEST_SKIP() << "AUTO-SUBSCRIBE: Service-side baseline manual subscription behavior validation pending";
}

// [@AC-3,US-2] TC-1: Service Multi-Client Auto-Subscribe
/**
 * Test: verifyServiceMultiClientAutoSubscribe_byMultipleAccepts_expectAllEvtReceived
 * Purpose: Validate service receives events from multiple auto-subscribed clients.
 * Steps:
 *   1) Online service with EvtConsumer capability and SrvArgs.UsageArgs.pEvt set.
 *   2) Accept N client connections with auto-subscribe.
 *   3) Each client posts unique events.
 *   4) Verify service receives all events with proper link identification.
 * Status: RED (service-side multi-client auto-subscribe not implemented).
 */
TEST(UT_ConetEventTypical, verifyServiceMultiClientAutoSubscribe_byMultipleAccepts_expectAllEvtReceived) {
    GTEST_SKIP() << "AUTO-SUBSCRIBE: Service-side multi-client auto-subscribe testing pending implementation";
}

// [@AC-1,US-3] TC-1: Event Auto-Subscribe Follows DAT Pattern
/**
 * Test: verifyEvtAutoSubscribePattern_matchesDatAutoWiring_expectConsistentAPI
 * Purpose: Ensure Event auto-subscribe follows the exact same API pattern as DAT auto-wiring.
 * Steps:
 *   1) Compare SrvArgs.UsageArgs.pDat setup with SrvArgs.UsageArgs.pEvt setup.
 *   2) Verify both use identical configuration approach (callback + private data + capability-specific args).
 *   3) Verify both return identical error codes for similar failure scenarios.
 *   4) Confirm both have same lifecycle (setup → auto-wire → cleanup).
 * Status: RED (pattern consistency validation pending auto-subscribe implementation).
 */
TEST(UT_ConetEventTypical, verifyEvtAutoSubscribePattern_matchesDatAutoWiring_expectConsistentAPI) {
    GTEST_SKIP() << "API-CONSISTENCY: Event auto-subscribe API pattern validation pending implementation";
}

// [@AC-2,US-3] TC-1: Consistent Error Handling Across Capabilities
/**
 * Test: verifyConsistentErrorHandling_acrossEvtDatCmd_expectSameErrorCodes
 * Purpose: Validate that Event auto-subscribe uses same error patterns as DAT/CMD.
 * Steps:
 *   1) Test invalid UsageArgs scenarios for EVT, DAT, and CMD capabilities.
 *   2) Verify all return same error codes (e.g., IOC_RESULT_INVALID_PARAM).
 *   3) Verify all perform same cleanup actions on failure.
 *   4) Confirm all leave system in same clean state after error.
 * Status: RED (cross-capability error handling consistency pending implementation).
 */
TEST(UT_ConetEventTypical, verifyConsistentErrorHandling_acrossEvtDatCmd_expectSameErrorCodes) {
    GTEST_SKIP() << "API-CONSISTENCY: Cross-capability error handling validation pending implementation";
}

// [@AC-3,US-3] TC-1: Mixed Capability Independence
/**
 * Test: verifyMixedCapabilityIndependence_byMultipleUsageArgs_expectIsolatedBehavior
 * Purpose: Ensure different UsageArgs work independently but consistently.
 * Steps:
 *   1) Online service with EvtProducer + DatReceiver + CmdExecutor capabilities.
 *   2) Set up SrvArgs.UsageArgs.pEvt, pDat, and pCmd simultaneously.
 *   3) Connect clients with different Usage types.
 *   4) Verify each auto-wiring works independently without interference.
 * Status: RED (mixed capability independence validation pending implementation).
 */
TEST(UT_ConetEventTypical, verifyMixedCapabilityIndependence_byMultipleUsageArgs_expectIsolatedBehavior) {
    GTEST_SKIP() << "API-CONSISTENCY: Mixed capability independence validation pending implementation";
}

// [@AC-4,US-3] TC-1: Consistent NULL UsageArgs Behavior
/**
 * Test: verifyNullUsageArgsConsistency_acrossAllCapabilities_expectUniformManualSetup
 * Purpose: Ensure NULL UsageArgs behavior is consistent across EVT, DAT, CMD.
 * Steps:
 *   1) Online service with mixed capabilities but all UsageArgs set to NULL.
 *   2) Connect clients for each capability type.
 *   3) Verify all connections succeed but require manual setup.
 *   4) Verify manual setup APIs work consistently for all capabilities.
 * Status: RED (NULL UsageArgs consistency validation pending implementation).
 */
TEST(UT_ConetEventTypical, verifyNullUsageArgsConsistency_acrossAllCapabilities_expectUniformManualSetup) {
    GTEST_SKIP() << "API-CONSISTENCY: NULL UsageArgs behavior consistency validation pending implementation";
}

//======>END OF TEST CASES=========================================================================
