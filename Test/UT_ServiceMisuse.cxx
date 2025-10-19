///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF OVERVIEW OF THIS UNIT TESTING FILE===============================================
/**
 * @brief
 *  Exercise misuse and fault scenarios around IOC Service APIs to ensure robust error handling.
 *
 *-------------------------------------------------------------------------------------------------
 *++Context
 *  Complements Typical and Boundary suites by validating how the Service layer behaves under
 *  mis-sequenced calls, repeated operations, capability mismatches, and resource leaks.
 *  These tests intentionally violate usage contracts to confirm defensive programming and clear diagnostics.
 *
 *  KEY DISTINCTION FROM BOUNDARY TESTS:
 *   - Boundary: Wrong inputs (NULL, invalid values) with CORRECT usage patterns
 *   - Misuse: Wrong usage patterns (wrong sequence, repeated ops, incompatible capabilities)
 *
 *  COVERAGE AREAS:
 *   - Lifecycle misuse: Double online/offline, operations in wrong sequence
 *   - Capability misuse: Incompatible usage types, manual ops on automatic services
 *   - State misuse: Operations on closed/offline resources
 *   - Resource containment: Leak prevention during misuse scenarios
 */
//======>END OF OVERVIEW OF THIS UNIT TESTING FILE=================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF UNIT TESTING DESIGN==============================================================
/**
 * 📋 TEST CASE DESIGN ASPECTS/CATEGORIES
 *  Priority: Typical → Boundary → Misuse → Fault → Performance → Concurrency → Others
 *  Principle: Improve Value • Avoid Lost • Balance Skill vs Cost
 */
/**
 * US-1 (Misuse): As a service maintainer, I want repeated lifecycle calls (double online/offline)
 *  to return explicit errors so accidental retries do not corrupt state.
 *
 *  AC-1: GIVEN service already onlined, WHEN IOC_onlineService called again with same args,
 *         THEN return IOC_RESULT_ALREADY_EXIST_SERVICE (or equivalent).
 *  AC-2: GIVEN service already offline, WHEN IOC_offlineService invoked twice,
 *         THEN return IOC_RESULT_NOT_EXIST_SERVICE.
 */
/**
 * US-2 (Misuse): As a service maintainer, I need invalid sequencing (accept before online,
 *  close link twice, connect after offline) to surface predictable codes.
 *
 *  AC-1: GIVEN service never onlined, WHEN IOC_acceptClient called, THEN return IOC_RESULT_NOT_EXIST_SERVICE.
 *  AC-2: GIVEN link already closed, WHEN IOC_closeLink invoked again, THEN return IOC_RESULT_NOT_EXIST_LINK.
 *  AC-3: GIVEN service offline, WHEN IOC_connectService executed, THEN return IOC_RESULT_NOT_EXIST_SERVICE.
 */
/**
 * US-3 (Fault Containment): As an operator, I want resource leaks avoided when misuse occurs,
 *  so failed operations still clean up temporary allocations.
 *
 *  AC-1: GIVEN online failure, WHEN partial service object allocated, THEN internal list remains balanced.
 *  AC-2: GIVEN repeated accept attempts, WHEN queue is empty, THEN no dangling client handles persist.
 */
/**
 * US-4 (Misuse): As a service developer, I want manual accept on AUTO_ACCEPT services to be rejected,
 *  so I don't accidentally interfere with automatic link management.
 *
 *  AC-1: GIVEN service with IOC_SRVFLAG_AUTO_ACCEPT, WHEN calling IOC_acceptClient manually,
 *         THEN return error indicating manual accept is not supported.
 */
/**
 * US-5 (Misuse): As a client developer, I want connection attempts with incompatible capabilities
 *  to fail clearly, so I can fix my configuration.
 *
 *  AC-1: GIVEN service with specific UsageCapabilities, WHEN client connects with incompatible Usage,
 *         THEN return IOC_RESULT_INCOMPATIBLE_USAGE.
 */
/**
 * US-6 (Misuse): As a link user, I want operations on links after service offline to fail predictably,
 *  so I know the service is unavailable.
 *
 *  AC-1: GIVEN service offline and links closed, WHEN attempting operations on those links,
 *         THEN return IOC_RESULT_NOT_EXIST_LINK or IOC_RESULT_LINK_CLOSED.
 */
/**
 * TEST CASES — ORGANIZATION & STATUS
 *  - By Category: Lifecycle misuse → Sequencing misuse → Capability misuse → Resource assurance
 *  - STATUS LEGEND: ⚪ Planned/TODO, 🔴 Implemented/RED, 🟢 Passed/GREEN, ⚠️ Issues
 *
 *  [@US-1/AC-1]
 *   🟢 TC: verifyOnlineService_byRepeatedCall_expectConflictSrvArgs
 *
 *  [@US-1/AC-2]
 *   🟢 TC: verifyOfflineService_byDoubleCall_expectNotExistService
 *
 *  [@US-2/AC-1]
 *   🟢 TC: verifyAcceptClient_beforeOnline_expectNotExistService
 *
 *  [@US-2/AC-2]
 *   🟢 TC: verifyCloseLink_byDoubleClose_expectNotExistLink
 *
 *  [@US-2/AC-3]
 *   🟢 TC: verifyConnectService_afterOffline_expectNotExistService
 *
 *  [@US-3/AC-1]
 *   ⚪ TC: DISABLED_verifyOnlineService_byFailedAlloc_expectNoLeakIndicators
 *
 *  [@US-3/AC-2]
 *   ⚪ TC: DISABLED_verifyAcceptClient_onEmptyQueue_expectNoDanglingLink
 *
 *  [@US-4/AC-1]
 *   ⚠️ TC: DISABLED_verifyAcceptClient_onAutoAcceptService_expectNotSupportManualAccept
 *           (Implementation limitation: manual accept not rejected on AUTO_ACCEPT services)
 *
 *  [@US-5/AC-1]
 *   🔴 TC: verifyConnectService_byIncompatibleUsage_expectIncompatibleUsage
 *
 *  [@US-6/AC-1]
 *   🔴 TC: verifyPostEVT_afterServiceOffline_expectLinkClosedOrNotExist
 */
//======>END OF UNIT TESTING DESIGN================================================================
//======BEGIN OF UNIT TESTING IMPLEMENTATION=======================================================
#include "_UT_IOC_Common.h"

// Notes:
// - Enable each test once supporting hooks (fault injection or leak inspection) are prepared.
// - Keep assertions focused (≤3) to spotlight key misuse signals.

//=== US-1/AC-1 ===
/**
 * @[Name]: verifyOnlineService_byRepeatedCall_expectConflictSrvArgs
 * @[Purpose]: Guard against duplicate online attempts reusing the same service arguments.
 * @[Brief]: Online once, retry with identical URI/capabilities, expect IOC_RESULT_CONFLICT_SRVARGS.
 * @[Steps]:
 *   1) 🔧 Build minimal FIFO service args and online the service.
 *   2) 🎯 Call IOC_onlineService again with identical args.
 *   3) ✅ Verify duplicate attempt returns IOC_RESULT_CONFLICT_SRVARGS.
 *   4) 🧹 Offline original service (and any accidental duplicate) to reset state.
 * @[Expect]: Retry call is rejected with conflict while original service remains intact.
 * @[Status]: GREEN �
 */
TEST(UT_ServiceMisuse, verifyOnlineService_byRepeatedCall_expectConflictSrvArgs) {
    // GIVEN: a service already onlined with specific arguments
    IOC_SrvArgs_T args{};
    args.SrvURI = {.pProtocol = IOC_SRV_PROTO_FIFO, .pHost = IOC_SRV_HOST_LOCAL_PROCESS, .pPath = "misuse-reonline"};
    args.UsageCapabilites = IOC_LinkUsageEvtProducer;
    args.Flags = IOC_SRVFLAG_NONE;

    IOC_SrvID_T firstSrv = IOC_ID_INVALID;
    ASSERT_EQ(IOC_RESULT_SUCCESS, IOC_onlineService(&firstSrv, &args));
    ASSERT_NE(IOC_ID_INVALID, firstSrv);

    // WHEN: attempting to online the same service again with identical arguments
    IOC_SrvID_T duplicateSrv = IOC_ID_INVALID;
    IOC_Result_T retryResult = IOC_onlineService(&duplicateSrv, &args);

    // THEN: expect the API to reject the duplicate with a conflict code
    VERIFY_KEYPOINT_EQ(IOC_RESULT_CONFLICT_SRVARGS, retryResult, "KP1: duplicate online returns conflict");

    // CLEANUP: ensure the original service is properly taken offline
    ASSERT_EQ(IOC_RESULT_SUCCESS, IOC_offlineService(firstSrv));
    if (duplicateSrv != IOC_ID_INVALID) {
        IOC_offlineService(duplicateSrv);
    }
}

//=== US-1/AC-2 ===
/**
 * @[Name]: verifyOfflineService_byDoubleCall_expectNotExistService
 * @[Purpose]: Ensure repeated offline calls surface NOT_EXIST_SERVICE instead of silently succeeding.
 * @[Brief]: Online/offline once successfully, call offline again to confirm NOT_EXIST_SERVICE is returned.
 * @[Steps]:
 *   1) 🔧 Online a minimal FIFO service.
 *   2) 🔧 Offline the service successfully.
 *   3) 🎯 Call IOC_offlineService again on the same SrvID.
 *   4) ✅ Verify the second call yields IOC_RESULT_NOT_EXIST_SERVICE.
 * @[Expect]: Second offline attempt reports NOT_EXIST_SERVICE without side effects.
 * @[Status]: GREEN �
 */
TEST(UT_ServiceMisuse, verifyOfflineService_byDoubleCall_expectNotExistService) {
    IOC_SrvArgs_T args{};
    args.SrvURI = {
        .pProtocol = IOC_SRV_PROTO_FIFO,
        .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
        .pPath = "misuse-double-offline"
    };
    args.UsageCapabilites = IOC_LinkUsageEvtProducer;
    args.Flags = IOC_SRVFLAG_NONE;

    IOC_SrvID_T srvID = IOC_ID_INVALID;
    ASSERT_EQ(IOC_RESULT_SUCCESS, IOC_onlineService(&srvID, &args));
    ASSERT_NE(IOC_ID_INVALID, srvID);

    ASSERT_EQ(IOC_RESULT_SUCCESS, IOC_offlineService(srvID));

    IOC_Result_T secondResult = IOC_offlineService(srvID);
    VERIFY_KEYPOINT_EQ(IOC_RESULT_NOT_EXIST_SERVICE, secondResult,
        "KP1: double offline returns NOT_EXIST_SERVICE");
}

//=== US-2/AC-1 ===
/**
 * @[Name]: verifyAcceptClient_beforeOnline_expectNotExistService
 * @[Purpose]: Ensure accept before online hints caller about missing service.
 * @[Status]: ⚪ Planned
 */
TEST(UT_ServiceMisuse, verifyAcceptClient_beforeOnline_expectNotExistService) {
    // GIVEN: no service is onlined
    IOC_LinkID_T linkID = IOC_ID_INVALID;

    // WHEN: attempting to accept a client on a non-existent service
    IOC_Result_T result = IOC_acceptClient(IOC_ID_INVALID, &linkID, NULL);

    // THEN: expect NOT_EXIST_SERVICE
    VERIFY_KEYPOINT_EQ(IOC_RESULT_NOT_EXIST_SERVICE, result,
                       "KP1: accept before online returns NOT_EXIST_SERVICE");
}

//=== US-2/AC-2 ===
/**
 * @[Name]: verifyCloseLink_byDoubleClose_expectNotExistLink
 * @[Purpose]: Detect repeated close operations on the same link.
 * @[Status]: ⚪ Planned
 */
TEST(UT_ServiceMisuse, verifyCloseLink_byDoubleClose_expectNotExistLink) {
    // GIVEN: a simple service with AUTO_ACCEPT to establish a connection easily
    IOC_SrvArgs_T srvArgs{};
    srvArgs.SrvURI = {.pProtocol = IOC_SRV_PROTO_FIFO, .pHost = IOC_SRV_HOST_LOCAL_PROCESS, .pPath = "misuse-double-close"};
    srvArgs.UsageCapabilites = IOC_LinkUsageEvtProducer;
    srvArgs.Flags = (IOC_SrvFlags_T)(IOC_SRVFLAG_AUTO_ACCEPT);

    IOC_SrvID_T srvID = IOC_ID_INVALID;
    ASSERT_EQ(IOC_RESULT_SUCCESS, IOC_onlineService(&srvID, &srvArgs));
    ASSERT_NE(IOC_ID_INVALID, srvID);

    // AND: a client connects to the service
    IOC_ConnArgs_T connArgs{};
    connArgs.SrvURI = srvArgs.SrvURI;
    connArgs.Usage = IOC_LinkUsageEvtConsumer;
    IOC_LinkID_T cliLinkID = IOC_ID_INVALID;
    ASSERT_EQ(IOC_RESULT_SUCCESS, IOC_connectService(&cliLinkID, &connArgs, NULL));
    ASSERT_NE(IOC_ID_INVALID, cliLinkID);

    // WHEN: closing the link twice
    ASSERT_EQ(IOC_RESULT_SUCCESS, IOC_closeLink(cliLinkID));
    IOC_Result_T secondClose = IOC_closeLink(cliLinkID);

    // THEN: the second close should report NOT_EXIST_LINK
    VERIFY_KEYPOINT_EQ(IOC_RESULT_NOT_EXIST_LINK, secondClose,
                       "KP1: double close returns NOT_EXIST_LINK");

    // CLEANUP
    ASSERT_EQ(IOC_RESULT_SUCCESS, IOC_offlineService(srvID));
}

//=== US-2/AC-3 ===
/**
 * @[Name]: verifyConnectService_afterOffline_expectNotExistService
 * @[Purpose]: Validate connect attempts after explicit offline receive NOT_EXIST_SERVICE.
 * @[Status]: ⚪ Planned
 */
TEST(UT_ServiceMisuse, verifyConnectService_afterOffline_expectNotExistService) {
    // GIVEN: a service that was online and then taken offline
    IOC_SrvArgs_T srvArgs{};
    srvArgs.SrvURI = {.pProtocol = IOC_SRV_PROTO_FIFO, .pHost = IOC_SRV_HOST_LOCAL_PROCESS, .pPath = "misuse-connect-after-offline"};
    srvArgs.UsageCapabilites = IOC_LinkUsageEvtProducer;
    srvArgs.Flags = IOC_SRVFLAG_NONE;

    IOC_SrvID_T srvID = IOC_ID_INVALID;
    ASSERT_EQ(IOC_RESULT_SUCCESS, IOC_onlineService(&srvID, &srvArgs));
    ASSERT_NE(IOC_ID_INVALID, srvID);
    ASSERT_EQ(IOC_RESULT_SUCCESS, IOC_offlineService(srvID));

    // WHEN: attempting to connect to the now-offline service
    IOC_ConnArgs_T connArgs{};
    connArgs.SrvURI = srvArgs.SrvURI;
    connArgs.Usage = IOC_LinkUsageEvtConsumer;
    IOC_LinkID_T linkID = IOC_ID_INVALID;
    IOC_Result_T result = IOC_connectService(&linkID, &connArgs, NULL);

    // THEN: expect NOT_EXIST_SERVICE
    VERIFY_KEYPOINT_EQ(IOC_RESULT_NOT_EXIST_SERVICE, result,
                       "KP1: connect after offline returns NOT_EXIST_SERVICE");
}

//=== US-3/AC-1 ===
/**
 * @[Name]: verifyOnlineService_byFailedAlloc_expectNoLeakIndicators
 * @[Purpose]: Future fault-injection hook to ensure allocator rollbacks leave no residue.
 * @[Status]: ⚪ Planned
 */
TEST(UT_ServiceMisuse, DISABLED_verifyOnlineService_byFailedAlloc_expectNoLeakIndicators) {
    GTEST_SKIP() << "TODO: Inject allocation failure, confirm diagnostics and no leak.";
}

//=== US-3/AC-2 ===
/**
 * @[Name]: verifyAcceptClient_onEmptyQueue_expectNoDanglingLink
 * @[Purpose]: Ensure repeated accepts on empty queue don't leak handles.
 * @[Status]: ⚪ Planned
 */
TEST(UT_ServiceMisuse, DISABLED_verifyAcceptClient_onEmptyQueue_expectNoDanglingLink) {
    GTEST_SKIP() << "TODO: Repeated accept with empty queue, ensure no phantom links remain.";
}

//=== US-4/AC-1 ===
/**
 * @[Name]: verifyAcceptClient_onAutoAcceptService_expectNotSupportManualAccept
 * @[Purpose]: Prevent manual accept on AUTO_ACCEPT services to avoid interfering with automatic link management.
 * @[Brief]: Online service with AUTO_ACCEPT flag, attempt manual acceptClient, expect rejection.
 * @[Steps]:
 *   1) 🔧 Online service with IOC_SRVFLAG_AUTO_ACCEPT.
 *   2) 🎯 Call IOC_acceptClient manually on the AUTO_ACCEPT service.
 *   3) ✅ Verify the call returns IOC_RESULT_NOT_SUPPORT_MANUAL_ACCEPT immediately.
 *   4) 🧹 Offline service to cleanup.
 * @[Expect]: Manual accept attempt is rejected immediately since AUTO_ACCEPT manages links automatically.
 * @[Status]: GREEN 🟢
 */
TEST(UT_ServiceMisuse, verifyAcceptClient_onAutoAcceptService_expectNotSupportManualAccept) {
    // GIVEN: a service with AUTO_ACCEPT flag enabled
    IOC_SrvArgs_T args{};
    args.SrvURI = {
        .pProtocol = IOC_SRV_PROTO_FIFO,
        .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
        .pPath = "misuse-manual-accept-on-auto"
    };
    args.UsageCapabilites = IOC_LinkUsageEvtProducer;
    args.Flags = IOC_SRVFLAG_AUTO_ACCEPT;

    IOC_SrvID_T srvID = IOC_ID_INVALID;
    ASSERT_EQ(IOC_RESULT_SUCCESS, IOC_onlineService(&srvID, &args));
    ASSERT_NE(IOC_ID_INVALID, srvID);

    // WHEN: attempting to manually accept a client on AUTO_ACCEPT service
    IOC_LinkID_T linkID = IOC_ID_INVALID;
    IOC_Result_T result = IOC_acceptClient(srvID, &linkID, NULL);

    // THEN: expect the operation to be rejected immediately
    VERIFY_KEYPOINT_EQ(IOC_RESULT_NOT_SUPPORT_MANUAL_ACCEPT, result,
                       "KP1: manual accept on AUTO_ACCEPT service should be rejected immediately");
    EXPECT_EQ(IOC_ID_INVALID, linkID) << "No link should be created on rejected accept";

    // CLEANUP
    ASSERT_EQ(IOC_RESULT_SUCCESS, IOC_offlineService(srvID));
}

//=== US-5/AC-1 ===
/**
 * @[Name]: verifyConnectService_byIncompatibleUsage_expectIncompatibleUsage
 * @[Purpose]: DISCOVER that incompatible usage connections are currently allowed (SHOULD BE REJECTED).
 * @[Brief]: Online service as EvtProducer, client connects as EvtProducer (not complementary).
 * @[Steps]:
 *   1) 🔧 Online service with UsageCapabilites = IOC_LinkUsageEvtProducer.
 *   2) 🎯 Client attempts to connect with Usage = IOC_LinkUsageEvtProducer (same, not complementary).
 *   3) 🐛 DISCOVER: Current implementation ALLOWS connection but sets link role to "Undefined".
 * @[Expect]: FUTURE: Connection should be REJECTED with IOC_RESULT_INCOMPATIBLE_USAGE.
 *           CURRENT: Connection SUCCEEDS with ServiceLinkRole=Undefined (BUG DISCOVERED).
 * @[Status]: 🔴 RED - Test INTENTIONALLY FAILS to highlight implementation bug
 * @[TODO]: Fix IOC_connectService to validate usage compatibility BEFORE creating link.
 *          Should reject when ServiceCap and ClientUsage are not complementary.
 */
TEST(UT_ServiceMisuse, verifyConnectService_byIncompatibleUsage_expectIncompatibleUsage) {
    // GIVEN: service configured as EvtProducer
    IOC_SrvArgs_T srvArgs{};
    srvArgs.SrvURI = {
        .pProtocol = IOC_SRV_PROTO_FIFO,
        .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
        .pPath = "misuse-incompatible-usage"
    };
    srvArgs.UsageCapabilites = IOC_LinkUsageEvtProducer;  // Service produces events
    srvArgs.Flags = IOC_SRVFLAG_AUTO_ACCEPT;

    IOC_SrvID_T srvID = IOC_ID_INVALID;
    ASSERT_EQ(IOC_RESULT_SUCCESS, IOC_onlineService(&srvID, &srvArgs));
    ASSERT_NE(IOC_ID_INVALID, srvID);

    // WHEN: client attempts to connect as EvtProducer (incompatible - should be Consumer)
    IOC_ConnArgs_T connArgs{};
    connArgs.SrvURI = srvArgs.SrvURI;
    connArgs.Usage = IOC_LinkUsageEvtProducer;  // WRONG! Should be EvtConsumer

    IOC_LinkID_T linkID = IOC_ID_INVALID;
    IOC_Result_T result = IOC_connectService(&linkID, &connArgs, NULL);

    // THEN: EXPECTATION - connection should be rejected
    // REALITY - current implementation allows it but creates undefined role link
    printf("  🐛 BUG DISCOVERED: Incompatible usage connection ALLOWED (should be REJECTED)\n");
    printf("     ServiceCap=EvtProducer + ClientUsage=EvtProducer → Link created with role=Undefined\n");
    printf("     Expected: IOC_RESULT_INCOMPATIBLE_USAGE\n");
    printf("     Actual: IOC_RESULT_SUCCESS with invalid link\n");
    
    // This test INTENTIONALLY FAILS to highlight the bug
    VERIFY_KEYPOINT_NE(IOC_RESULT_SUCCESS, result,
                       "KP1: incompatible usage connection SHOULD BE rejected (currently BUG: allowed)");
    
    // If we got here with SUCCESS, close the invalid link
    if (result == IOC_RESULT_SUCCESS && linkID != IOC_ID_INVALID) {
        printf("  🧹 Cleaning up improperly created link (LinkID=%u)\n", linkID);
        IOC_closeLink(linkID);
    }

    // CLEANUP
    ASSERT_EQ(IOC_RESULT_SUCCESS, IOC_offlineService(srvID));
}

//=== US-6/AC-1 ===
/**
 * @[Name]: verifyPostEVT_afterServiceOffline_expectLinkClosedOrNotExist
 * @[Purpose]: Validate that operations on links fail predictably after service goes offline.
 * @[Brief]: Establish link, offline service (auto-closes links), attempt to use link, expect failure.
 * @[Steps]:
 *   1) 🔧 Online service and establish a client link.
 *   2) 🔧 Take service offline (should automatically close all accepted links).
 *   3) 🎯 Attempt to post event using the now-closed link.
 *   4) ✅ Verify operation fails with IOC_RESULT_NOT_EXIST_LINK or similar.
 * @[Expect]: Link operations fail after service offline with clear error code.
 * @[Status]: RED 🔴
 */
TEST(UT_ServiceMisuse, verifyPostEVT_afterServiceOffline_expectLinkClosedOrNotExist) {
    // GIVEN: service with established client connection
    IOC_SrvArgs_T srvArgs{};
    srvArgs.SrvURI = {
        .pProtocol = IOC_SRV_PROTO_FIFO,
        .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
        .pPath = "misuse-post-after-offline"
    };
    srvArgs.UsageCapabilites = IOC_LinkUsageEvtProducer;
    srvArgs.Flags = IOC_SRVFLAG_AUTO_ACCEPT;

    IOC_SrvID_T srvID = IOC_ID_INVALID;
    ASSERT_EQ(IOC_RESULT_SUCCESS, IOC_onlineService(&srvID, &srvArgs));
    ASSERT_NE(IOC_ID_INVALID, srvID);

    // Client connects
    IOC_ConnArgs_T connArgs{};
    connArgs.SrvURI = srvArgs.SrvURI;
    connArgs.Usage = IOC_LinkUsageEvtConsumer;
    IOC_LinkID_T linkID = IOC_ID_INVALID;
    ASSERT_EQ(IOC_RESULT_SUCCESS, IOC_connectService(&linkID, &connArgs, NULL));
    ASSERT_NE(IOC_ID_INVALID, linkID);

    // WHEN: service goes offline (automatically closes all links)
    ASSERT_EQ(IOC_RESULT_SUCCESS, IOC_offlineService(srvID));

    // THEN: attempting to use the closed link should fail
    IOC_EvtDesc_T evt{};
    evt.EvtID = IOC_EVTID_TEST_KEEPALIVE;
    IOC_Result_T result = IOC_postEVT(linkID, &evt, NULL);

    VERIFY_KEYPOINT_NE(IOC_RESULT_SUCCESS, result,
                       "KP1: posting on closed link after service offline should fail");
    // Expected errors: IOC_RESULT_NOT_EXIST_LINK, IOC_RESULT_LINK_CLOSED, or similar

    // NOTE: Link is already closed by offlineService, no explicit cleanup needed
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF TODO/IMPLEMENTATION TRACKING SECTION===========================================
// Planned Enhancements:
//  - Fault injection harness for service allocator rollbacks
//  - Link leak audit helpers (reuse IOC diagnostics or add test hooks)
//  - Extend misuse coverage to broadcast vs. non-broadcast client roles
//  - Additional capability misuse: DatSender/Receiver, CmdInitiator/Executor incompatibilities
//  - Test getServiceLinkIDs on manual-accept services (boundary vs misuse categorization TBD)
///////////////////////////////////////////////////////////////////////////////////////////////////
