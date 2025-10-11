///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF OVERVIEW OF THIS UNIT TESTING FILE===============================================
/**
 * @brief
 *  Verify boundary/edge conditions of IOC Service APIs (online/offline, accept/connect, inspect, broadcast).
 *
 *-------------------------------------------------------------------------------------------------
 *++Context
 *  This file complements Typical tests with Boundary coverage per CaTDD:
 *   - Invalid/null parameters, non-existing resources
 *   - Unsupported operations by flag/capability
 *   - Buffer/timeout semantics (non-block vs immediate)
 *  Scope focuses on Service Layer APIs around: IOC_onlineService, IOC_offlineService,
 *  IOC_connectService, IOC_acceptClient, IOC_closeLink, IOC_broadcastEVT, IOC_getServiceLinkIDs.
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
 *   ⚪ TC: DISABLED_verifyGetServiceLinkIDs_bySmallBuffer_expectBufferTooSmall
 *
 *  [EXTENSIONS]
 *   ⚪ TC: DISABLED_verifyPostEVT_byNoSubscriber_expectNoEventConsumer
 *   ⚪ TC: DISABLED_verifyTimeoutSemantics_byZeroVsNonBlock_expectDistinctResults
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

//=== Placeholders (disabled) for deeper boundaries to extend later ===
// Duplicate subscribe/unsubscribe handling, buffer-too-small on getServiceLinkIDs, timeout semantics, etc.
TEST(UT_ServiceBoundary, DISABLED_verifyGetServiceLinkIDs_bySmallBuffer_expectBufferTooSmall) {
    GTEST_SKIP() << "TODO: Populate multiple accepted links, then assert BUFFER_TOO_SMALL with partial results.";
}

TEST(UT_ServiceBoundary, DISABLED_verifyPostEVT_byNoSubscriber_expectNoEventConsumer) {
    GTEST_SKIP() << "TODO: Establish link without subscription and verify NO_EVENT_CONSUMER.";
}

TEST(UT_ServiceBoundary, DISABLED_verifyTimeoutSemantics_byZeroVsNonBlock_expectDistinctResults) {
    GTEST_SKIP() << "TODO: Cover immediate timeout vs true non-block for DAT/EVT polling APIs.";
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF TODO/IMPLEMENTATION TRACKING SECTION===========================================
// RED/IMPLEMENTED (to be enabled as features mature):
//  - [@US-2/AC-2] verifyGetServiceLinkIDs_bySmallBuffer_expectBufferTooSmall
//  - Post with no subscribers returns NO_EVENT_CONSUMER across roles
//  - Timeout semantics: Zero-timeout vs NonBlock consistency on send/recv/pull
///////////////////////////////////////////////////////////////////////////////////////////////////
