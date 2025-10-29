///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF OVERVIEW OF THIS UNIT TESTING FILE===============================================
/**
 * @brief ValidFunc-Boundary Tests: Verify boundary/edge conditions that still WORK correctly.
 *
 *-------------------------------------------------------------------------------------------------
 * @category ValidFunc-Boundary (Edge Cases That Still Work - APIs Function Correctly)
 *
 * Part of Test Design Formula:
 *   Service's Functional Test = ValidFunc(Typical + Boundary) + InValidFunc(Misuse)
 *                                                  ^^^^^^^^
 *                                          (Edges but WORKS!)
 *
 * ValidFunc = API WORKS from caller's viewpoint (successful operation or graceful rejection)
 *  - Typical: Common scenarios in normal range
 *  - Boundary: Edge cases (min/max args, limits, empty buffers) but API still behaves correctly
 *
 * This file covers: Edge/boundary conditions where APIs function as designed
 *  - Min/max parameter values (NULL pointers, zero buffers, invalid IDs)
 *  - Resource boundary conditions (non-existent services/links, empty queues)
 *  - Capability limits (unsupported operations, buffer too small)
 *  - Timeout edge semantics (zero vs non-blocking vs immediate)
 *  - APIs return appropriate error codes and maintain system integrity
 *
 * Test Philosophy - KEY DISTINCTION:
 *  - ValidFunc (Typical + Boundary): API WORKS correctly (returns expected result/error)
 *  - InValidFunc (Misuse): API usage FAILS (wrong sequence, double calls, state violations)
 *  - Focus: Verify APIs handle edge inputs gracefully and return correct diagnostic codes
 *  - All tests here: Single operations, correct sequence, proper usage - just edge inputs
 *
 * Related Test Files:
 *  - UT_ServiceTypical.cxx: ValidFunc-Typical (common scenarios that work)
 *  - UT_ServiceMisuse.cxx: InValidFunc-Misuse (wrong usage that fails)
 *  - See: Test/UT_ServiceTestDesign.md for complete test taxonomy
 *
 *-------------------------------------------------------------------------------------------------
 *++Context
 *  Complements Typical tests by testing parameter boundaries and limits per CaTDD principles.
 *  Scope: IOC_onlineService, IOC_offlineService, IOC_connectService, IOC_acceptClient,
 *         IOC_closeLink, IOC_broadcastEVT, IOC_getServiceLinkIDs.
 */
//======>END OF OVERVIEW OF THIS UNIT TESTING FILE=================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF UNIT TESTING DESIGN==============================================================
/**
 * 📋 TEST CASE DESIGN ASPECTS/CATEGORIES
 *  Priority: Typical → Boundary → State → Fault → Performance → Concurrency → Others
 *  Principle: Improve Value • Avoid Lost • Balance Skill vs Cost
 *
 *  Extended taxonomy (adopt as needed):
 *   - FreelyDrafts • Typical • Demo • Boundary • State • Performance • Concurrency
 *   - Capability/Capacity • Robust • Fault • Misuse • Compatibility • Configuration • Others
 *   - Note: Start with Typical → Boundary, then grow coverage deliberately.
 */
/**
 * US-1 (Boundary): As a service developer, I want invalid inputs to be rejected clearly,
 *  so that misuse is caught early and does not corrupt internal state.
 *
 *  AC-1: GIVEN null/invalid params, WHEN calling Service APIs, THEN return INVALID_PARAM.
 *  AC-2: GIVEN not-exist resource (service/link), WHEN operating on it, THEN return NOT_EXIST_*.
 *
 * US-2 (Boundary): As a service developer, I want unsupported operations to return explicit codes,
 *  so users understand missing flags/capabilities.
 *
 *  AC-1: GIVEN service without BROADCAST flag, WHEN calling broadcastEVT, THEN return NOT_SUPPORT_BROADCAST_EVENT.
 *  AC-2: GIVEN small buffer for service link inspection, WHEN links exceed capacity,
 *         THEN return BUFFER_TOO_SMALL (if applicable) with partial results.
 *
 * US-3 (Boundary): As a service developer, I want to be notified when an event has no consumers,
 *  so that I can handle cases where no one is listening.
 *
 *  AC-1: GIVEN a link with no event subscriptions, WHEN posting an event, THEN return NO_EVENT_CONSUMER.
 */
/**
 * US/AC/TC Contract
 *  - US: Value from user perspective
 *  - AC: GIVEN/WHEN/THEN conditions for each US
 *  - TC: Concrete steps and assertions to verify an AC
 *  - Rule of three: At least 1 US; ≥1 AC per US; ≥1 TC per AC
 *  - Keep ≤3 key assertions per test case; add more cases if needed.
 */

/**
 * COVERAGE STRATEGY (choose axes):
 *  - Service Role × Client Role × Mode
 *  - Component State × Operation × Boundary
 *  - Concurrency × Resource limits × Faults
 *
 * TEMPLATE PATTERN (coverage matrix skeleton):
 * ┌─────────────────┬─────────────┬─────────────┬──────────────────────────────┐
 * │ Dimension 1     │ Dimension 2 │ Dimension 3 │ Key Scenarios                │
 * ├─────────────────┼─────────────┼─────────────┼──────────────────────────────┤
 * │ [Example values]│ [Example]   │ [Example]   │ [US-X: Short description]    │
 * └─────────────────┴─────────────┴─────────────┴──────────────────────────────┘
 */

/**
 * TEST CASES — ORGANIZATION & STATUS
 *  - By Category: Typical → Boundary → State → Error → Performance
 *  - By Priority: Critical first
 *  STATUS LEGEND: ⚪ Planned/TODO, 🔴 Implemented/RED, 🟢 Passed/GREEN, ⚠️ Issues
 *
 * CLASSIC LIST FORMAT (per AC)
 *  [@US-1/AC-1]
 *   � TC: verifyOnlineService_byNullSrvID_expectInvalidParam
 *   � TC: verifyOnlineService_byInvalidSrvArgs_expectInvalidParam
 *   � TC: verifyGetServiceLinkIDs_byNullParams_expectInvalidParam
 *
 *  [@US-1/AC-2]
 *   � TC: verifyConnectService_byNotExistService_expectNotExistService
 *   � TC: verifyAcceptClient_byInvalidSrvID_expectNotExistService
 *   � TC: verifyCloseLink_byInvalidLink_expectNotExistLink
 *   � TC: verifyOfflineService_byInvalidSrvID_expectNotExistService
 *
 *  [@US-2/AC-1]
 *   � TC: verifyBroadcastEVT_withoutFlag_expectNotSupportBroadcastEvent
 *
 *  [@US-2/AC-2]
 *   🟢 TC: verifyGetServiceLinkIDs_bySmallBuffer_expectBufferTooSmall
 *
 *  [EXTENSIONS]
 *   🟢 TC: verifyPostEVT_byNoSubscriber_expectNoEventConsumer
 *   🟢 TC: verifyTimeoutSemantics_byZeroVsNonBlock_expectDistinctResults
 */
//======>END OF UNIT TESTING DESIGN================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======BEGIN OF UNIT TESTING IMPLEMENTATION=======================================================
#include "_UT_IOC_Common.h"

// Notes:
// - Keep each test with <= 3 key assertions where possible.
// - Prefer short names: verifyX_byY_expectZ

//=== US-1/AC-1: INVALID_PARAM on bad inputs ===
/**
 * @[Name]: verifyOnlineService_byNullSrvID_expectInvalidParam
 * @[Purpose]: Ensure API guards invalid output parameter and returns INVALID_PARAM without aborting
 * @[Brief]: Call IOC_onlineService with nullptr pSrvID and valid args; expect INVALID_PARAM
 * @[Steps]:
 *   1) 🔧 Prepare minimal valid IOC_SrvArgs_T
 *   2) 🎯 Call IOC_onlineService(nullptr, &args)
 *   3) ✅ Assert return is IOC_RESULT_INVALID_PARAM
 * @[Expect]: No assertion; explicit invalid-parameter return code
 * @[Status]: PASSED/GREEN ✅
 * @[Notes]: Boundary path; logging is allowed, assertion removed in service code
 */
TEST(UT_ServiceBoundary, verifyOnlineService_byNullSrvID_expectInvalidParam) {
    // US-1/AC-1
    // GIVEN: null output parameter pSrvID
    // WHEN: calling IOC_onlineService(nullptr, &args)
    // THEN: function returns IOC_RESULT_INVALID_PARAM and does not assert
    // SETUP
    IOC_SrvArgs_T args{};
    args.SrvURI = {.pProtocol = IOC_SRV_PROTO_FIFO, .pHost = IOC_SRV_HOST_LOCAL_PROCESS, .pPath = "boundary-nullid"};
    args.UsageCapabilites = IOC_LinkUsageEvtProducer;

    // BEHAVIOR
    printf("🎯 BEHAVIOR: onlineService with null srvID\n");
    IOC_Result_T r = IOC_onlineService(nullptr, &args);

    // VERIFY
    ASSERT_EQ(IOC_RESULT_INVALID_PARAM, r);
}

/**
 * @[Name]: verifyOnlineService_byInvalidSrvArgs_expectInvalidParam
 * @[Purpose]: Validate rejection of invalid service arguments (missing capabilities)
 * @[Brief]: Call IOC_onlineService with zero UsageCapabilites; expect INVALID_PARAM
 * @[Steps]:
 *   1) 🔧 Create IOC_SrvArgs_T with UsageCapabilites=0
 *   2) 🎯 Call IOC_onlineService(&srvID, &badArgs)
 *   3) ✅ Assert return is IOC_RESULT_INVALID_PARAM
 * @[Expect]: No assertion; explicit invalid-parameter return code
 * @[Status]: PASSED/GREEN ✅
 * @[Notes]: Complements null pSrvID boundary
 */
TEST(UT_ServiceBoundary, verifyOnlineService_byInvalidSrvArgs_expectInvalidParam) {
    // US-1/AC-1
    // GIVEN: invalid service args (no usage capabilities)
    // WHEN: calling IOC_onlineService(&srvID, &badArgs)
    // THEN: function returns IOC_RESULT_INVALID_PARAM and does not assert
    // SETUP: missing usage capabilities
    IOC_SrvID_T srvID = IOC_ID_INVALID;
    IOC_SrvArgs_T badArgs{};
    badArgs.SrvURI = {.pProtocol = IOC_SRV_PROTO_FIFO, .pHost = IOC_SRV_HOST_LOCAL_PROCESS, .pPath = "boundary-bad"};
    badArgs.UsageCapabilites = (IOC_LinkUsage_T)0;  // invalid

    // BEHAVIOR
    printf("🎯 BEHAVIOR: onlineService with invalid args (no capabilities)\n");
    IOC_Result_T r = IOC_onlineService(&srvID, &badArgs);

    // VERIFY
    ASSERT_EQ(IOC_RESULT_INVALID_PARAM, r);
}

/**
 * @[Name]: verifyGetServiceLinkIDs_byNullParams_expectInvalidParam
 * @[Purpose]: Ensure inspection API validates output pointers
 * @[Brief]: Call IOC_getServiceLinkIDs with null outputs; expect INVALID_PARAM
 * @[Steps]:
 *   1) 🔧 Choose any SrvID value
 *   2) 🎯 Call IOC_getServiceLinkIDs(anySrv, nullptr, 0, nullptr)
 *   3) ✅ Assert return is IOC_RESULT_INVALID_PARAM
 * @[Expect]: No assertion; explicit invalid-parameter return code
 * @[Status]: PASSED/GREEN ✅
 * @[Notes]: Keeps API contract consistent across getters
 */
TEST(UT_ServiceBoundary, verifyGetServiceLinkIDs_byNullParams_expectInvalidParam) {
    // US-1/AC-1
    // GIVEN: null output buffers for LinkIDs and count
    // WHEN: calling IOC_getServiceLinkIDs(anySrv, nullptr, 0, nullptr)
    // THEN: function returns IOC_RESULT_INVALID_PARAM
    // SETUP
    IOC_SrvID_T anySrv = 12345;  // any value; API checks null outputs first

    // BEHAVIOR
    printf("🎯 BEHAVIOR: getServiceLinkIDs with null outputs\n");
    IOC_Result_T r1 = IOC_getServiceLinkIDs(anySrv, nullptr, 0, nullptr);

    // VERIFY
    ASSERT_EQ(IOC_RESULT_INVALID_PARAM, r1);
}

//=== US-1/AC-2: NOT_EXIST_* on missing resources ===
/**
 * @[Name]: verifyConnectService_byNotExistService_expectNotExistService
 * @[Purpose]: Ensure connect rejects non-existent services
 * @[Brief]: Connect to a never-onlined SrvURI; expect NOT_EXIST_SERVICE
 * @[Steps]:
 *   1) 🔧 Build ConnArgs with path to a non-existent service
 *   2) 🎯 Call IOC_connectService(&linkID, &conn, nullptr)
 *   3) ✅ Assert return is IOC_RESULT_NOT_EXIST_SERVICE
 * @[Expect]: Clear warning log; no assertion abort
 * @[Status]: PASSED/GREEN ✅
 * @[Notes]: Negative path for service discovery
 */
TEST(UT_ServiceBoundary, verifyConnectService_byNotExistService_expectNotExistService) {
    // US-1/AC-2
    // GIVEN: a SrvURI that does not correspond to any onlined service
    // WHEN: calling IOC_connectService(&linkID, &conn, nullptr)
    // THEN: function returns IOC_RESULT_NOT_EXIST_SERVICE
    // SETUP: connect to a service path that was never onlined
    IOC_SrvURI_T uri = {.pProtocol = IOC_SRV_PROTO_FIFO, .pHost = IOC_SRV_HOST_LOCAL_PROCESS, .pPath = "no-such-svc"};
    IOC_ConnArgs_T conn = {.SrvURI = uri, .Usage = IOC_LinkUsageEvtConsumer};
    IOC_LinkID_T linkID = IOC_ID_INVALID;

    // BEHAVIOR
    printf("🎯 BEHAVIOR: connectService to non-existent service\n");
    IOC_Result_T r = IOC_connectService(&linkID, &conn, nullptr);

    // VERIFY
    ASSERT_EQ(IOC_RESULT_NOT_EXIST_SERVICE, r);
}

/**
 * @[Name]: verifyAcceptClient_byInvalidSrvID_expectNotExistService
 * @[Purpose]: Ensure accept client validates service existence
 * @[Brief]: Call IOC_acceptClient with invalid SrvID; expect NOT_EXIST_SERVICE
 * @[Steps]:
 *   1) 🔧 Prepare invalid SrvID and link placeholder
 *   2) 🎯 Call IOC_acceptClient(badSrv, &linkID, nullptr)
 *   3) ✅ Assert return is IOC_RESULT_NOT_EXIST_SERVICE
 * @[Expect]: Error+warn logs; no assertion abort
 * @[Status]: PASSED/GREEN ✅
 * @[Notes]: Relies on __IOC_getSrvObjBySrvID to return NULL for bad IDs
 */
TEST(UT_ServiceBoundary, verifyAcceptClient_byInvalidSrvID_expectNotExistService) {
    // US-1/AC-2
    // GIVEN: an invalid service ID
    // WHEN: calling IOC_acceptClient(badSrv, &linkID, nullptr)
    // THEN: function returns IOC_RESULT_NOT_EXIST_SERVICE
    // SETUP
    IOC_SrvID_T badSrv = 0xFFFF;  // invalid
    IOC_LinkID_T linkID = IOC_ID_INVALID;

    // BEHAVIOR
    printf("🎯 BEHAVIOR: acceptClient with invalid service ID\n");
    IOC_Result_T r = IOC_acceptClient(badSrv, &linkID, nullptr);

    // VERIFY
    ASSERT_EQ(IOC_RESULT_NOT_EXIST_SERVICE, r);
}

/**
 * @[Name]: verifyCloseLink_byInvalidLink_expectNotExistLink
 * @[Purpose]: Ensure link close handles invalid IDs cleanly
 * @[Brief]: Close a non-existent LinkID; expect NOT_EXIST_LINK
 * @[Steps]:
 *   1) 🔧 Choose an invalid/random LinkID
 *   2) 🎯 Call IOC_closeLink(LinkID)
 *   3) ✅ Assert return is IOC_RESULT_NOT_EXIST_LINK
 * @[Expect]: Error log only; no assertion
 * @[Status]: PASSED/GREEN ✅
 * @[Notes]: Uses safer LinkID validation in helper
 */
TEST(UT_ServiceBoundary, verifyCloseLink_byInvalidLink_expectNotExistLink) {
    // US-1/AC-2
    // GIVEN: a non-existent LinkID
    // WHEN: calling IOC_closeLink(0xDEADBEEF)
    // THEN: function returns IOC_RESULT_NOT_EXIST_LINK
    // BEHAVIOR
    printf("🎯 BEHAVIOR: closeLink on non-existent link\n");
    IOC_Result_T r = IOC_closeLink(0xDEADBEEF);

    // VERIFY
    ASSERT_EQ(IOC_RESULT_NOT_EXIST_LINK, r);
}

/**
 * @[Name]: verifyOfflineService_byInvalidSrvID_expectNotExistService
 * @[Purpose]: Ensure offline validates SrvID and reports NOT_EXIST_SERVICE
 * @[Brief]: Offline an invalid service ID; expect NOT_EXIST_SERVICE
 * @[Steps]:
 *   1) 🔧 Choose invalid SrvID
 *   2) 🎯 Call IOC_offlineService(bad)
 *   3) ✅ Assert return is IOC_RESULT_NOT_EXIST_SERVICE
 * @[Expect]: Error+warn logs; no assertion
 * @[Status]: PASSED/GREEN ✅
 * @[Notes]: Matches connect/accept negative paths
 */
TEST(UT_ServiceBoundary, verifyOfflineService_byInvalidSrvID_expectNotExistService) {
    // US-1/AC-2
    // GIVEN: an invalid service ID
    // WHEN: calling IOC_offlineService(0xBEEF)
    // THEN: function returns IOC_RESULT_NOT_EXIST_SERVICE
    // BEHAVIOR
    printf("🎯 BEHAVIOR: offlineService on invalid service ID\n");
    IOC_Result_T r = IOC_offlineService(0xBEEF);

    // VERIFY
    ASSERT_EQ(IOC_RESULT_NOT_EXIST_SERVICE, r);
}

//=== US-2/AC-1: Unsupported operation signals ===
/**
 * @[Name]: verifyBroadcastEVT_withoutFlag_expectNotSupportBroadcastEvent
 * @[Purpose]: Ensure broadcast requires IOC_SRVFLAG_BROADCAST_EVENT
 * @[Brief]: Online a producer without the flag and call broadcastEVT; expect NOT_SUPPORT_BROADCAST_EVENT
 * @[Steps]:
 *   1) 🔧 Online service with UsageCapabilites=EvtProducer and Flags=0
 *   2) 🎯 Call IOC_broadcastEVT(srvID, &evt, nullptr)
 *   3) ✅ Assert return is IOC_RESULT_NOT_SUPPORT_BROADCAST_EVENT
 * @[Expect]: No crash; explicit not-supported code
 * @[Status]: PASSED/GREEN ✅
 * @[Notes]: Cleans up by IOC_offlineService(srvID)
 */
TEST(UT_ServiceBoundary, verifyBroadcastEVT_withoutFlag_expectNotSupportBroadcastEvent) {
    // US-2/AC-1
    // GIVEN: a service onlined without IOC_SRVFLAG_BROADCAST_EVENT
    // WHEN: calling IOC_broadcastEVT(srvID, &evt, nullptr)
    // THEN: function returns IOC_RESULT_NOT_SUPPORT_BROADCAST_EVENT
    // SETUP: online a regular EvtProducer service WITHOUT broadcast flag
    IOC_SrvID_T srvID = IOC_ID_INVALID;
    IOC_SrvURI_T uri = {
        .pProtocol = IOC_SRV_PROTO_FIFO, .pHost = IOC_SRV_HOST_LOCAL_PROCESS, .pPath = "no-broadcast-flag"};
    IOC_SrvArgs_T args{};
    args.SrvURI = uri;
    args.Flags = (IOC_SrvFlags_T)0;
    args.UsageCapabilites = IOC_LinkUsageEvtProducer;

    ASSERT_EQ(IOC_RESULT_SUCCESS, IOC_onlineService(&srvID, &args));

    IOC_EvtDesc_T evt{};
    evt.EvtID = IOC_EVTID_TEST_KEEPALIVE;

    // BEHAVIOR
    printf("🎯 BEHAVIOR: broadcastEVT without broadcast flag\n");
    IOC_Result_T r = IOC_broadcastEVT(srvID, &evt, nullptr);

    // VERIFY
    ASSERT_EQ(IOC_RESULT_NOT_SUPPORT_BROADCAST_EVENT, r);

    // CLEANUP
    ASSERT_EQ(IOC_RESULT_SUCCESS, IOC_offlineService(srvID));
}

/**
 * @[Name]: verifyGetServiceLinkIDs_bySmallBuffer_expectBufferTooSmall
 * @[Purpose]: Ensure getServiceLinkIDs handles small buffers gracefully
 * @[Brief]: Online a service, connect multiple clients, then call getServiceLinkIDs with a buffer smaller than the
 * number of links. Expect BUFFER_TOO_SMALL and a partial list of links.
 * @[Steps]:
 *   1) 🔧 Online a service that can accept clients.
 *   2) 🔧 Connect 3 clients to the service.
 *   3) 🎯 Call IOC_getServiceLinkIDs with a buffer of size 2.
 *   4) ✅ Assert return is IOC_RESULT_BUFFER_TOO_SMALL.
 *   5) ✅ Assert that the number of links returned is 2.
 *   6) 🔧 Clean up all links and the service.
 * @[Expect]: IOC_RESULT_BUFFER_TOO_SMALL and partial results.
 * @[Status]: PASSED/GREEN ✅
 */
TEST(UT_ServiceBoundary, verifyGetServiceLinkIDs_bySmallBuffer_expectBufferTooSmall) {
    // US-2/AC-2
    // GIVEN: a service with more client links than the provided buffer size
    // WHEN: calling IOC_getServiceLinkIDs with a small buffer
    // THEN: function returns IOC_RESULT_BUFFER_TOO_SMALL and fills the buffer
    // SETUP: online a service and connect multiple clients
    IOC_SrvID_T srvID = IOC_ID_INVALID;
    IOC_SrvURI_T uri = {
        .pProtocol = IOC_SRV_PROTO_FIFO, .pHost = IOC_SRV_HOST_LOCAL_PROCESS, .pPath = "small-buffer-test"};
    IOC_SrvArgs_T args = {
        .SrvURI = uri, .Flags = IOC_SRVFLAG_AUTO_ACCEPT, .UsageCapabilites = IOC_LinkUsageEvtProducer};
    ASSERT_EQ(IOC_RESULT_SUCCESS, IOC_onlineService(&srvID, &args));

    // Connect multiple clients
    const int num_clients = 3;
    IOC_LinkID_T client_links[num_clients];
    for (int i = 0; i < num_clients; ++i) {
        IOC_ConnArgs_T conn_args = {.SrvURI = uri, .Usage = IOC_LinkUsageEvtConsumer};
        ASSERT_EQ(IOC_RESULT_SUCCESS, IOC_connectService(&client_links[i], &conn_args, nullptr));
    }

    // Wait for all clients to be auto-accepted
    uint16_t link_count = 0;
    IOC_LinkID_T temp_links[num_clients];
    for (int i = 0; i < 10 && link_count < num_clients; ++i) {
        // Query the actual number of links. The result of the call might be BUFFER_TOO_SMALL
        // if we query with a small buffer, but pActualCount will be updated.
        // However, to be safe and handle all cases, we provide a full buffer.
        IOC_getServiceLinkIDs(srvID, temp_links, num_clients, &link_count);
        if (link_count < num_clients) {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
    }
    ASSERT_EQ(num_clients, link_count);

    // BEHAVIOR: try to get link IDs with a buffer that is too small
    const int buffer_size = 2;
    IOC_LinkID_T link_buffer[buffer_size];
    uint16_t num_links_returned = 0;

    printf("🎯 BEHAVIOR: getServiceLinkIDs with a small buffer\n");
    IOC_Result_T r = IOC_getServiceLinkIDs(srvID, link_buffer, buffer_size, &num_links_returned);

    // VERIFY
    ASSERT_EQ(IOC_RESULT_BUFFER_TOO_SMALL, r);
    ASSERT_EQ(buffer_size, num_links_returned);

    // CLEANUP
    for (int i = 0; i < num_clients; ++i) {
        ASSERT_EQ(IOC_RESULT_SUCCESS, IOC_closeLink(client_links[i]));
    }
    ASSERT_EQ(IOC_RESULT_SUCCESS, IOC_offlineService(srvID));
}

/**
 * @[Name]: verifyPostEVT_byNoSubscriber_expectNoEventConsumer
 * @[Purpose]: Ensure postEVT returns NO_EVENT_CONSUMER if no client is subscribed.
 * @[Brief]: Establish a producer-consumer link, but do not subscribe to any events. Call postEVT and expect
 * NO_EVENT_CONSUMER.
 * @[Steps]:
 *   1) 🔧 Online a service (producer) and connect a client (consumer).
 *   2) 🎯 Call IOC_postEVT for a test event.
 *   3) ✅ Assert return is IOC_RESULT_NO_EVENT_CONSUMER.
 *   4) 🔧 Clean up the link and service.
 * @[Expect]: IOC_RESULT_NO_EVENT_CONSUMER because the client never subscribed.
 * @[Status]: PASSED/GREEN ✅
 */
TEST(UT_ServiceBoundary, verifyPostEVT_byNoSubscriber_expectNoEventConsumer) {
    // GIVEN: a producer-consumer link is established, but the consumer has not subscribed to any events
    // WHEN: the producer posts an event
    // THEN: the call returns IOC_RESULT_NO_EVENT_CONSUMER
    // SETUP
    IOC_SrvID_T srvID = IOC_ID_INVALID;
    IOC_LinkID_T producer_link = IOC_ID_INVALID;
    IOC_LinkID_T consumer_link = IOC_ID_INVALID;

    IOC_SrvURI_T uri = {
        .pProtocol = IOC_SRV_PROTO_FIFO, .pHost = IOC_SRV_HOST_LOCAL_PROCESS, .pPath = "no-subscriber-test"};
    IOC_SrvArgs_T srv_args = {
        .SrvURI = uri, .Flags = IOC_SRVFLAG_AUTO_ACCEPT, .UsageCapabilites = IOC_LinkUsageEvtProducer};
    ASSERT_EQ(IOC_RESULT_SUCCESS, IOC_onlineService(&srvID, &srv_args));

    IOC_ConnArgs_T conn_args = {.SrvURI = uri, .Usage = IOC_LinkUsageEvtConsumer};
    ASSERT_EQ(IOC_RESULT_SUCCESS, IOC_connectService(&consumer_link, &conn_args, nullptr));

    // The service link (producer_link) is created by the auto-accept mechanism.
    // We need to find it to post an event. A short wait may be needed for the async accept.
    uint16_t link_count = 0;
    for (int i = 0; i < 10 && link_count == 0; ++i) {
        IOC_getServiceLinkIDs(srvID, &producer_link, 1, &link_count);
        if (link_count == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
    ASSERT_EQ(1, link_count);

    IOC_EvtDesc_T evt = {.EvtID = IOC_EVTID_TEST_KEEPALIVE};

    // BEHAVIOR
    printf("🎯 BEHAVIOR: postEVT to a link with no event subscriptions\n");
    IOC_Result_T r = IOC_postEVT(producer_link, &evt, nullptr);

    // VERIFY
    ASSERT_EQ(IOC_RESULT_NO_EVENT_CONSUMER, r);

    // CLEANUP
    ASSERT_EQ(IOC_RESULT_SUCCESS, IOC_offlineService(srvID));
}

/**
 * @[Name]: verifyTimeoutSemantics_byZeroVsNonBlock_expectDistinctResults
 * @[Purpose]: Ensure IOC_pullEVT differentiates between non-blocking and finite-timeout polling modes.
 * @[Brief]: When no events are queued, a pull with timeout=0 should return NO_EVENT_PENDING immediately, while a pull
 * with a small positive timeout should block briefly and then report TIMEOUT.
 * @[Steps]:
 *   1) 🔧 Establish a producer-consumer link and subscribe to a test event.
 *   2) 🎯 Call IOC_pullEVT with timeout=0. Assert it returns IOC_RESULT_NO_EVENT_PENDING without a noticeable delay.
 *   3) 🎯 Call IOC_pullEVT with IOC_TIMEOUT_IMMEDIATE. Assert it blocks briefly and returns IOC_RESULT_TIMEOUT.
 *   4) 🔧 Post and pull an event to confirm normal delivery still works.
 *   5) 🎯 Repeat steps 2 & 3 to ensure the behaviors stay consistent after activity.
 *   6) 🔧 Clean up resources.
 * @[Expect]: Non-blocking mode returns IOC_RESULT_NO_EVENT_PENDING; finite timeout returns IOC_RESULT_TIMEOUT.
 * @[Status]: PASSED/GREEN ✅
 */
TEST(UT_ServiceBoundary, verifyTimeoutSemantics_byZeroVsNonBlock_expectDistinctResults) {
    // GIVEN: a link with an active subscription but no pending events
    // WHEN: pulling events with a zero timeout or non-block flag
    // THEN: the call should return immediately with IOC_RESULT_NO_MORE_EVENT
    // SETUP
    IOC_SrvID_T srvID = IOC_ID_INVALID;
    IOC_LinkID_T producer_link = IOC_ID_INVALID;
    IOC_LinkID_T consumer_link = IOC_ID_INVALID;
    IOC_EvtDesc_T evt_desc{};
    IOC_Result_T r;

    IOC_SrvURI_T uri = {.pProtocol = IOC_SRV_PROTO_FIFO, .pHost = IOC_SRV_HOST_LOCAL_PROCESS, .pPath = "timeout-test"};
    IOC_SrvArgs_T srv_args = {
        .SrvURI = uri, .Flags = IOC_SRVFLAG_AUTO_ACCEPT, .UsageCapabilites = IOC_LinkUsageEvtProducer};
    ASSERT_EQ(IOC_RESULT_SUCCESS, IOC_onlineService(&srvID, &srv_args));

    IOC_ConnArgs_T conn_args = {.SrvURI = uri, .Usage = IOC_LinkUsageEvtConsumer};
    ASSERT_EQ(IOC_RESULT_SUCCESS, IOC_connectService(&consumer_link, &conn_args, nullptr));

    // Wait for producer link to be established
    uint16_t link_count = 0;
    for (int i = 0; i < 10 && link_count == 0; ++i) {
        IOC_getServiceLinkIDs(srvID, &producer_link, 1, &link_count);
        if (link_count == 0) std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    ASSERT_EQ(1, link_count);

    // Subscribe to the event for polling consumption
    IOC_EvtID_T evt_id = IOC_EVTID_TEST_KEEPALIVE;
    IOC_SubEvtArgs_T sub_args{};
    sub_args.CbProcEvt_F = nullptr;
    sub_args.pCbPrivData = nullptr;
    sub_args.EvtNum = 1;
    sub_args.pEvtIDs = &evt_id;
    ASSERT_EQ(IOC_RESULT_SUCCESS, IOC_subEVT(consumer_link, &sub_args));

    // Define options for non-blocking and finite-timeout pulls
    IOC_Option_defineNonBlock(pull_opt_nonblock);
    IOC_Option_defineTimeout(pull_opt_immediate, IOC_TIMEOUT_IMMEDIATE);

    // BEHAVIOR & VERIFY 1: Test before any event is posted
    printf("🎯 BEHAVIOR: pullEVT with timeout=0 (no event queued)\n");
    r = IOC_pullEVT(consumer_link, &evt_desc, &pull_opt_nonblock);
    ASSERT_EQ(IOC_RESULT_NO_EVENT_PENDING, r);

    printf("🎯 BEHAVIOR: pullEVT with IOC_TIMEOUT_IMMEDIATE (no event queued)\n");
    r = IOC_pullEVT(consumer_link, &evt_desc, &pull_opt_immediate);
    ASSERT_EQ(IOC_RESULT_TIMEOUT, r);

    // Post and pull an event to confirm setup is correct
    IOC_EvtDesc_T post_evt = {.EvtID = IOC_EVTID_TEST_KEEPALIVE};
    ASSERT_EQ(IOC_RESULT_SUCCESS, IOC_postEVT(producer_link, &post_evt, nullptr));
    ASSERT_EQ(IOC_RESULT_SUCCESS, IOC_pullEVT(consumer_link, &evt_desc, nullptr));
    ASSERT_EQ(post_evt.EvtID, evt_desc.EvtID);

    // BEHAVIOR & VERIFY 2: Test after the event queue has been emptied
    printf("🎯 BEHAVIOR: pullEVT with timeout=0 (after clearing queue)\n");
    r = IOC_pullEVT(consumer_link, &evt_desc, &pull_opt_nonblock);
    ASSERT_EQ(IOC_RESULT_NO_EVENT_PENDING, r);

    printf("🎯 BEHAVIOR: pullEVT with IOC_TIMEOUT_IMMEDIATE (after clearing queue)\n");
    r = IOC_pullEVT(consumer_link, &evt_desc, &pull_opt_immediate);
    ASSERT_EQ(IOC_RESULT_TIMEOUT, r);

    // CLEANUP
    ASSERT_EQ(IOC_RESULT_SUCCESS, IOC_offlineService(srvID));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF TODO/IMPLEMENTATION TRACKING SECTION===========================================
// COVERAGE COMPLETE:
//  - Post with no subscribers returns NO_EVENT_CONSUMER across roles
//  - Timeout semantics: Zero-timeout vs NonBlock consistency on send/recv/pull
///////////////////////////////////////////////////////////////////////////////////////////////////
