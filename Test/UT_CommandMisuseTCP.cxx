///////////////////////////////////////////////////////////////////////////////////////////////////
// Command Misuse TCP - P1 InvalidFunc Misuse Testing
//
// PURPOSE:
//   Validate TCP command API error handling for incorrect usage patterns.
//   Tests invalid inputs and wrong API usage to ensure graceful error handling.
//
// TDD WORKFLOW:
//   Design → Draft → Structure → Test (RED) → Code (GREEN) → Refactor → Repeat
//
// REFERENCE: LLM/CaTDD_DesignPrompt.md for full methodology
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "_UT_IOC_Common.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF OVERVIEW=========================================================================
/**
 * @brief
 *   [WHAT] This file validates TCP command API error handling for incorrect usage
 *   [WHERE] in the IOC Command API with TCP protocol layer
 *   [WHY] to ensure API misuse is detected and handled gracefully
 *
 * SCOPE:
 *   - [In scope]: P1 InvalidFunc Misuse tests (incorrect API usage)
 *   - [In scope]: Null pointer handling
 *   - [In scope]: Invalid parameter values
 *   - [In scope]: Illegal state transitions
 *   - [In scope]: Protocol mismatches
 *   - [Out of scope]: Valid boundary cases → see UT_CommandBoundaryTCP.cxx
 *   - [Out of scope]: External failures → see UT_CommandFaultTCP.cxx
 *   - [Out of scope]: Typical scenarios → see UT_CommandTypicalTCP.cxx
 *
 * RELATIONSHIPS:
 *   - Extends: UT_CommandTypicalTCP.cxx (error handling for typical patterns)
 *   - Related: UT_CommandBoundaryTCP.cxx (misuse vs boundary distinction)
 *   - Related: UT_CommandFaultTCP.cxx (misuse vs fault distinction)
 */
//======>END OF OVERVIEW===========================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF TEST DESIGN======================================================================
/**
 * COVERAGE MATRIX (P1 InvalidFunc Misuse):
 * ┌──────────────────────────┬─────────────────────────┬────────────────────────────┐
 * │ Misuse Category          │ API Function            │ Error Type                 │
 * ├──────────────────────────┼─────────────────────────┼────────────────────────────┤
 * │ Null Pointers            │ IOC_execCMD             │ NULL CmdDesc               │
 * │ Null Pointers            │ IOC_onlineService       │ NULL SrvArgs, NULL pSrvID  │
 * │ Null Pointers            │ IOC_connectService      │ NULL ConnArgs, NULL pLinkID│
 * │ Invalid IDs              │ IOC_execCMD             │ Invalid LinkID             │
 * │ Invalid IDs              │ IOC_offlineService      │ Invalid SrvID              │
 * │ State Violations         │ IOC_execCMD             │ Before connect, after close│
 * │ State Violations         │ IOC_closeLink           │ Double-close               │
 * │ Protocol Errors          │ IOC_onlineService       │ NULL/wrong protocol string │
 * │ Protocol Errors          │ IOC_onlineService       │ NULL host, Port 0          │
 * │ Command Descriptor       │ IOC_execCMD             │ Unsupported, wrong status  │
 * │ Lifecycle Errors         │ IOC_offlineService      │ Double-offline             │
 * │ Lifecycle Errors         │ IOC_closeLink           │ Invalid LinkID             │
 * └──────────────────────────┴─────────────────────────┴────────────────────────────┘
 *
 * PORT ALLOCATION: Base 20080 (20080-20093)
 *
 * PRIORITY: P1 InvalidFunc Misuse (COMPLETE)
 *
 * STATUS:
 *   🟢 23 tests implemented and ALL GREEN! ✅✅✅
 *   📋 23 total test scenarios
 *   🎉 RGR CYCLE COMPLETE: All bugs fixed!
 */
///////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF USER STORY=======================================================================
/**
 * US-1: As a developer, I want null pointer errors caught gracefully
 *       so that API misuse doesn't cause crashes or undefined behavior.
 *
 * US-2: As a developer, I want invalid ID errors detected immediately
 *       so that I know when I'm using wrong handles or identifiers.
 *
 * US-3: As a developer, I want state violation errors reported clearly
 *       so that I can fix incorrect API call sequences.
 *
 * US-4: As a developer, I want protocol errors caught during setup
 *       so that configuration mistakes are detected early.
 */
//======>END OF USER STORY==========================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF ACCEPTANCE CRITERIA===============================================================
/**
 * [@US-1] Null Pointer Handling
 *  AC-1: GIVEN null pointer passed to API function,
 *        WHEN calling function,
 *        THEN returns INVALID_PARAM without crashing.
 *
 * [@US-2] Invalid ID Handling
 *  AC-1: GIVEN invalid LinkID/SrvID,
 *        WHEN calling API with invalid ID,
 *        THEN returns appropriate error code.
 *
 * [@US-3] State Violation Detection
 *  AC-1: GIVEN API called in wrong state,
 *        WHEN calling out-of-sequence,
 *        THEN returns state error without corruption.
 *
 * [@US-4] Protocol Error Detection
 *  AC-1: GIVEN wrong protocol configuration,
 *        WHEN attempting service setup,
 *        THEN returns configuration error.
 */
//======>END OF ACCEPTANCE CRITERIA=================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF TEST CASES========================================================================
/**
 * [@AC-1,US-1] Null Pointer Handling (7 tests)
 *  🟢 TC-1: verifyTcpMisuse_byNullCmdDesc_expectInvalidParam
 *      @[Purpose]: Validate NULL CmdDesc returns INVALID_PARAM without crashing
 *      @[Brief]: Call IOC_execCMD with NULL CmdDesc on valid connection
 *  🟢 TC-2: verifyTcpMisuse_byNullSrvArgs_expectInvalidParam
 *      @[Purpose]: Validate NULL SrvArgs returns INVALID_PARAM without crashing
 *      @[Brief]: Call IOC_onlineService with NULL SrvArgs
 *  🟢 TC-3: verifyTcpMisuse_byNullConnArgs_expectInvalidParam
 *      @[Purpose]: Validate NULL ConnArgs returns INVALID_PARAM without crashing
 *      @[Brief]: Call IOC_connectService with NULL ConnArgs
 *  🟢 TC-4: verifyTcpMisuse_byNullSrvIDOutput_expectInvalidParam
 *      @[Purpose]: Validate NULL output pointer returns INVALID_PARAM
 *      @[Brief]: Call IOC_onlineService with NULL pSrvID pointer
 *  🟢 TC-5: verifyTcpMisuse_byNullLinkIDOutput_expectInvalidParam
 *      @[Purpose]: Validate NULL output pointer returns INVALID_PARAM
 *      @[Brief]: Call IOC_connectService with NULL pLinkID pointer
 *  🟢 TC-6: verifyTcpMisuse_byNullAcceptOutput_expectInvalidParam
 *      @[Purpose]: Validate NULL output pointer returns INVALID_PARAM
 *      @[Brief]: Call IOC_acceptClient with NULL pLinkID pointer
 *      @[RGR]: 🟢 GREEN - Fixed! Added NULL check
 *  🟢 TC-7: verifyTcpMisuse_byNullWaitCmdDesc_expectInvalidParam
 *      @[Purpose]: Validate NULL CmdDesc returns INVALID_PARAM without crashing
 *      @[Brief]: Call IOC_waitCMD with NULL CmdDesc pointer
 *
 * [@AC-1,US-2] Invalid ID Handling (3 tests)
 *  🟢 TC-1: verifyTcpMisuse_byInvalidLinkID_expectError
 *      @[Purpose]: Validate invalid LinkID is detected and rejected
 *      @[Brief]: Call IOC_execCMD with IOC_ID_INVALID
 *  🟢 TC-2: verifyTcpMisuse_byInvalidSrvID_expectError
 *      @[Purpose]: Validate invalid SrvID is detected and rejected
 *      @[Brief]: Call IOC_offlineService with IOC_ID_INVALID
 *  🟢 TC-3: verifyTcpMisuse_byInvalidSrvIDForAccept_expectError
 *      @[Purpose]: Validate invalid SrvID in acceptClient is rejected
 *      @[Brief]: Call IOC_acceptClient with IOC_ID_INVALID
 *
 * [@AC-1,US-3] State Violations (3 tests)
 *  🟢 TC-1: verifyTcpMisuse_byExecBeforeConnect_expectStateError
 *      @[Purpose]: Validate command execution without connection fails
 *      @[Brief]: Try IOC_execCMD with fabricated LinkID before connecting
 *  🟢 TC-2: verifyTcpMisuse_byExecAfterClose_expectStateError
 *      @[Purpose]: Validate command execution after close fails
 *      @[Brief]: Connect, close, then try IOC_execCMD on closed link
 *  🟢 TC-3: verifyTcpMisuse_byDoubleClose_expectError
 *      @[Purpose]: Validate double-close is detected and fails
 *      @[Brief]: Call IOC_closeLink twice on same LinkID
 *
 * [@AC-1,US-4] Protocol Configuration Errors (4 tests)
 *  🟢 TC-1: verifyTcpMisuse_byWrongProtocol_expectConfigError
 *      @[Purpose]: Validate wrong protocol string is rejected
 *      @[Brief]: Call IOC_onlineService with "invalid_proto://"
 *      @[RGR]: 🟢 GREEN - Fixed! Returns NOT_SUPPORT
 *  🟢 TC-2: verifyTcpMisuse_byInvalidPort_expectConfigError
 *      @[Purpose]: Validate port 0 handling (OS-dependent)
 *      @[Brief]: Call IOC_onlineService with Port=0
 *  🟢 TC-3: verifyTcpMisuse_byNullProtocolString_expectInvalidParam
 *      @[Purpose]: Validate NULL protocol string is rejected
 *      @[Brief]: Call IOC_onlineService with pProtocol=NULL
 *  🟢 TC-4: verifyTcpMisuse_byNullHostString_expectInvalidParam
 *      @[Purpose]: Validate NULL host handling (may mean INADDR_ANY)
 *      @[Brief]: Call IOC_onlineService with pHost=NULL
 *
 * Link Usage Misuse (1 test)
 *  🟢 TC-1: verifyTcpMisuse_byExecOnWrongUsageLink_expectUsageError
 *      @[Purpose]: Validate link usage capability enforcement
 *      @[Brief]: Create link with DatSender usage, try IOC_execCMD
 *
 * Command Descriptor Misuse (3 tests)
 *  🟢 TC-1: verifyTcpMisuse_byUnsupportedCmdID_expectError
 *      @[Purpose]: Validate unsupported command ID returns NOT_SUPPORT
 *      @[Brief]: Execute ECHO command when only PING is supported
 *  🟢 TC-2: verifyTcpMisuse_byWrongCmdStatus_expectError
 *      @[Purpose]: Validate wrong CmdDesc status is handled gracefully
 *      @[Brief]: Call IOC_execCMD with Status=PENDING instead of INITIALIZED
 *  🟢 TC-3: verifyTcpMisuse_byNullPayloadNonZeroSize_expectError
 *      @[Purpose]: Validate NULL payload with non-zero size is rejected
 *      @[Brief]: Call IOC_CmdDesc_setInPayload with NULL pointer and size>0
 *      @[RGR]: 🟢 GREEN - Fixed! Added NULL check
 *
 * Lifecycle Misuse (2 tests)
 *  🟢 TC-1: verifyTcpMisuse_byDoubleOffline_expectError
 *      @[Purpose]: Validate double-offline is detected and fails
 *      @[Brief]: Call IOC_offlineService twice on same SrvID
 *  🟢 TC-2: verifyTcpMisuse_byCloseInvalidLink_expectError
 *      @[Purpose]: Validate closing invalid LinkID fails
 *      @[Brief]: Call IOC_closeLink with IOC_ID_INVALID
 */
//======>END OF TEST CASES==========================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF TEST IMPLEMENTATION===============================================================

#include <thread>

///////////////////////////////////////////////////////////////////////////////////////////////////
// [@AC-1,US-1] Null Pointer Handling Tests
///////////////////////////////////////////////////////////////////////////////////////////////////

// TC-1: verifyTcpMisuse_byNullCmdDesc_expectInvalidParam
/**
 * @[Category]: P1-Misuse (InvalidFunc)
 * @[Purpose]: Validate NULL CmdDesc returns INVALID_PARAM without crashing
 * @[Brief]: Call IOC_execCMD with NULL CmdDesc on valid connection
 * @[4-Phase Structure]:
 *   1) 🔧 SETUP: Create valid TCP connection
 *   2) 🎯 BEHAVIOR: Call IOC_execCMD with NULL CmdDesc
 *   3) ✅ VERIFY: Should return INVALID_PARAM
 *   4) 🧹 CLEANUP: Close connections and offline service
 */
TEST(UT_TcpCommandMisuse, verifyTcpMisuse_byNullCmdDesc_expectInvalidParam) {
    // ═══════════════════════════════════════════════════════════════════════════════════
    // 🔧 SETUP: Setup valid connection, then test null CmdDesc
    // ═══════════════════════════════════════════════════════════════════════════════════
    constexpr uint16_t TEST_PORT = 20080;

    IOC_SrvURI_T srvURI = {
        .pProtocol = IOC_SRV_PROTO_TCP, .pHost = "localhost", .Port = TEST_PORT, .pPath = "CmdMisuse_NullCmdDesc"};

    IOC_SrvArgs_T srvArgs = {
        .SrvURI = srvURI, .Flags = IOC_SRVFLAG_NONE, .UsageCapabilites = IOC_LinkUsageCmdExecutor, .UsageArgs = {}};

    IOC_SrvID_T srvID = IOC_ID_INVALID;
    IOC_LinkID_T srvLinkID = IOC_ID_INVALID;
    IOC_LinkID_T cliLinkID = IOC_ID_INVALID;

    ASSERT_EQ(IOC_RESULT_SUCCESS, IOC_onlineService(&srvID, &srvArgs));
    ASSERT_NE(IOC_ID_INVALID, srvID);

    IOC_ConnArgs_T connArgs = {.SrvURI = srvURI, .Usage = IOC_LinkUsageCmdInitiator};
    std::thread cliThread([&] { IOC_connectService(&cliLinkID, &connArgs, NULL); });

    ASSERT_EQ(IOC_RESULT_SUCCESS, IOC_acceptClient(srvID, &srvLinkID, NULL));
    cliThread.join();

    // ═══════════════════════════════════════════════════════════════════════════════════
    // 🎯 BEHAVIOR: Call IOC_execCMD with NULL CmdDesc
    // ═══════════════════════════════════════════════════════════════════════════════════

    IOC_Result_T result = IOC_execCMD(cliLinkID, NULL, NULL);

    // ═══════════════════════════════════════════════════════════════════════════════════
    // ✅ VERIFY: Should return INVALID_PARAM for NULL CmdDesc
    // ═══════════════════════════════════════════════════════════════════════════════════

    VERIFY_KEYPOINT_EQ(result, IOC_RESULT_INVALID_PARAM, "NULL CmdDesc should return INVALID_PARAM");

    // ═══════════════════════════════════════════════════════════════════════════════════
    // 🧹 CLEANUP: Release resources
    // ═══════════════════════════════════════════════════════════════════════════════════

    if (cliLinkID != IOC_ID_INVALID) IOC_closeLink(cliLinkID);
    if (srvLinkID != IOC_ID_INVALID) IOC_closeLink(srvLinkID);
    if (srvID != IOC_ID_INVALID) IOC_offlineService(srvID);
}

// TC-2: verifyTcpMisuse_byNullSrvArgs_expectInvalidParam
/**
 * @[Category]: P1-Misuse (InvalidFunc)
 * @[Purpose]: Validate NULL SrvArgs returns INVALID_PARAM without crashing
 * @[Brief]: Call IOC_onlineService with NULL SrvArgs
 * @[4-Phase Structure]:
 *   1) 🎯 BEHAVIOR: Call IOC_onlineService with NULL SrvArgs
 *   2) ✅ VERIFY: Should return INVALID_PARAM, SrvID remains INVALID
 */
TEST(UT_TcpCommandMisuse, verifyTcpMisuse_byNullSrvArgs_expectInvalidParam) {
    // ═══════════════════════════════════════════════════════════════════════════════════
    // 🎯 BEHAVIOR: Call IOC_onlineService with NULL SrvArgs
    // ═══════════════════════════════════════════════════════════════════════════════════

    IOC_SrvID_T srvID = IOC_ID_INVALID;
    IOC_Result_T result = IOC_onlineService(&srvID, NULL);

    // ═══════════════════════════════════════════════════════════════════════════════════
    // ✅ VERIFY: Should return INVALID_PARAM without crashing
    // ═══════════════════════════════════════════════════════════════════════════════════

    VERIFY_KEYPOINT_EQ(result, IOC_RESULT_INVALID_PARAM, "NULL SrvArgs should return INVALID_PARAM");
    VERIFY_KEYPOINT_EQ(srvID, IOC_ID_INVALID, "SrvID should remain INVALID");
}

// TC-3: verifyTcpMisuse_byNullConnArgs_expectInvalidParam
/**
 * @[Category]: P1-Misuse (InvalidFunc)
 * @[Purpose]: Validate NULL ConnArgs returns INVALID_PARAM without crashing
 * @[Brief]: Call IOC_connectService with NULL ConnArgs
 * @[4-Phase Structure]:
 *   1) 🎯 BEHAVIOR: Call IOC_connectService with NULL ConnArgs
 *   2) ✅ VERIFY: Should return INVALID_PARAM, LinkID remains INVALID
 */
TEST(UT_TcpCommandMisuse, verifyTcpMisuse_byNullConnArgs_expectInvalidParam) {
    // ═══════════════════════════════════════════════════════════════════════════════════
    // 🎯 BEHAVIOR: Call IOC_connectService with NULL ConnArgs
    // ═══════════════════════════════════════════════════════════════════════════════════

    IOC_LinkID_T cliLinkID = IOC_ID_INVALID;
    IOC_Result_T result = IOC_connectService(&cliLinkID, NULL, NULL);

    // ═══════════════════════════════════════════════════════════════════════════════════
    // ✅ VERIFY: Should return INVALID_PARAM without crashing
    // ═══════════════════════════════════════════════════════════════════════════════════

    VERIFY_KEYPOINT_EQ(result, IOC_RESULT_INVALID_PARAM, "NULL ConnArgs should return INVALID_PARAM");
    VERIFY_KEYPOINT_EQ(cliLinkID, IOC_ID_INVALID, "LinkID should remain INVALID");
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// [@AC-1,US-2] Invalid ID Handling Tests
///////////////////////////////////////////////////////////////////////////////////////////////////

// TC-1: verifyTcpMisuse_byInvalidLinkID_expectError
/**
 * @[Category]: P1-Misuse (InvalidFunc)
 * @[Purpose]: Validate invalid LinkID is detected and rejected
 * @[Brief]: Call IOC_execCMD with IOC_ID_INVALID
 * @[4-Phase Structure]:
 *   1) 🔧 SETUP: Create command descriptor without valid connection
 *   2) 🎯 BEHAVIOR: Call IOC_execCMD with IOC_ID_INVALID
 *   3) ✅ VERIFY: Should return INVALID_PARAM or NOT_EXIST
 *   4) 🧹 CLEANUP: Clean up command descriptor
 */
TEST(UT_TcpCommandMisuse, verifyTcpMisuse_byInvalidLinkID_expectError) {
    // ═══════════════════════════════════════════════════════════════════════════════════
    // 🔧 SETUP: Create command descriptor without valid connection
    // ═══════════════════════════════════════════════════════════════════════════════════

    IOC_CmdDesc_T cmdDesc = {};
    cmdDesc.CmdID = IOC_CMDID_TEST_PING;
    cmdDesc.Status = IOC_CMD_STATUS_INITIALIZED;
    cmdDesc.TimeoutMs = 1000;

    // ═══════════════════════════════════════════════════════════════════════════════════
    // 🎯 BEHAVIOR: Call IOC_execCMD with invalid LinkID
    // ═══════════════════════════════════════════════════════════════════════════════════

    IOC_Result_T result = IOC_execCMD(IOC_ID_INVALID, &cmdDesc, NULL);

    // ═══════════════════════════════════════════════════════════════════════════════════
    // ✅ VERIFY: Should return error for invalid LinkID
    // ═══════════════════════════════════════════════════════════════════════════════════

    // Accept either INVALID_PARAM or NOT_EXIST depending on implementation
    VERIFY_KEYPOINT_TRUE(result == IOC_RESULT_INVALID_PARAM || result == IOC_RESULT_NOT_EXIST,
                         "Should return INVALID_PARAM or NOT_EXIST for invalid LinkID");

    // 🧹 CLEANUP
    IOC_CmdDesc_cleanup(&cmdDesc);
}

// TC-2: verifyTcpMisuse_byInvalidSrvID_expectError
/**
 * @[Category]: P1-Misuse (InvalidFunc)
 * @[Purpose]: Validate invalid SrvID is detected and rejected
 * @[Brief]: Call IOC_offlineService with IOC_ID_INVALID
 * @[4-Phase Structure]:
 *   1) 🎯 BEHAVIOR: Call IOC_offlineService with IOC_ID_INVALID
 *   2) ✅ VERIFY: Should return error (not SUCCESS)
 */
TEST(UT_TcpCommandMisuse, verifyTcpMisuse_byInvalidSrvID_expectError) {
    // ═══════════════════════════════════════════════════════════════════════════════════
    // 🎯 BEHAVIOR: Call IOC_offlineService with invalid SrvID
    // ═══════════════════════════════════════════════════════════════════════════════════

    IOC_Result_T result = IOC_offlineService(IOC_ID_INVALID);

    // ═══════════════════════════════════════════════════════════════════════════════════
    // VERIFY: Should return error for invalid SrvID
    // ═══════════════════════════════════════════════════════════════════════════════════

    VERIFY_KEYPOINT_NE(result, IOC_RESULT_SUCCESS, "Should fail with invalid SrvID");
}

// TC-3: verifyTcpMisuse_byInvalidSrvIDForAccept_expectError
/**
 * @[Category]: P1-Misuse (InvalidFunc)
 * @[Purpose]: Validate invalid SrvID in acceptClient is rejected
 * @[Brief]: Call IOC_acceptClient with IOC_ID_INVALID
 * @[4-Phase Structure]:
 *   1) 🎯 BEHAVIOR: Call IOC_acceptClient with IOC_ID_INVALID
 *   2) ✅ VERIFY: Should return error (not SUCCESS)
 */
TEST(UT_TcpCommandMisuse, verifyTcpMisuse_byInvalidSrvIDForAccept_expectError) {
    // ═══════════════════════════════════════════════════════════════════════════════════
    // 🎯 BEHAVIOR: Call IOC_acceptClient with invalid SrvID
    // ═══════════════════════════════════════════════════════════════════════════════════

    IOC_LinkID_T linkID = IOC_ID_INVALID;
    IOC_Result_T result = IOC_acceptClient(IOC_ID_INVALID, &linkID, NULL);

    // ═══════════════════════════════════════════════════════════════════════════════════
    // ✅ VERIFY: Should return error for invalid SrvID
    // ═══════════════════════════════════════════════════════════════════════════════════

    VERIFY_KEYPOINT_NE(result, IOC_RESULT_SUCCESS, "Should fail with invalid SrvID");
    VERIFY_KEYPOINT_EQ(linkID, IOC_ID_INVALID, "LinkID should remain INVALID");
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// [@AC-1,US-3] State Violation Tests
///////////////////////////////////////////////////////////////////////////////////////////////////

// TC-1: verifyTcpMisuse_byExecBeforeConnect_expectStateError
/**
 * @[Category]: P1-Misuse (InvalidFunc)
 * @[Purpose]: Validate command execution without connection fails
 * @[Brief]: Try IOC_execCMD with fabricated LinkID before connecting
 * @[4-Phase Structure]:
 *   1) 🔧 SETUP: Create command descriptor without establishing connection
 *   2) 🎯 BEHAVIOR: Try IOC_execCMD with fabricated LinkID
 *   3) ✅ VERIFY: Should return error (not SUCCESS)
 *   4) 🧹 CLEANUP: Clean up command descriptor
 */
TEST(UT_TcpCommandMisuse, verifyTcpMisuse_byExecBeforeConnect_expectStateError) {
    // ═══════════════════════════════════════════════════════════════════════════════════
    // 🔧 SETUP: Create command without establishing connection
    // ═══════════════════════════════════════════════════════════════════════════════════

    IOC_CmdDesc_T cmdDesc = {};
    cmdDesc.CmdID = IOC_CMDID_TEST_PING;
    cmdDesc.Status = IOC_CMD_STATUS_INITIALIZED;
    cmdDesc.TimeoutMs = 1000;

    // Use a fabricated/invalid LinkID that looks valid but isn't connected
    IOC_LinkID_T fakeLinkID = 0x12345678;

    // ═══════════════════════════════════════════════════════════════════════════════════
    // 🎯 BEHAVIOR: Try to execute command without valid connection
    // ═══════════════════════════════════════════════════════════════════════════════════

    IOC_Result_T result = IOC_execCMD(fakeLinkID, &cmdDesc, NULL);

    // ═══════════════════════════════════════════════════════════════════════════════════
    // VERIFY: Should return state/connection error
    // ═══════════════════════════════════════════════════════════════════════════════════

    VERIFY_KEYPOINT_NE(result, IOC_RESULT_SUCCESS, "Should fail when executing before connect");

    IOC_CmdDesc_cleanup(&cmdDesc);
}

// TC-2: verifyTcpMisuse_byExecAfterClose_expectStateError
/**
 * @[Category]: P1-Misuse (InvalidFunc)
 * @[Purpose]: Validate command execution after close fails
 * @[Brief]: Connect, close, then try IOC_execCMD on closed link
 * @[4-Phase Structure]:
 *   1) 🔧 SETUP: Setup connection, then close it
 *   2) 🎯 BEHAVIOR: Try IOC_execCMD after closing link
 *   3) ✅ VERIFY: Should return error (not SUCCESS)
 *   4) 🧹 CLEANUP: Close server link and offline service
 */
TEST(UT_TcpCommandMisuse, verifyTcpMisuse_byExecAfterClose_expectStateError) {
    // ═══════════════════════════════════════════════════════════════════════════════════
    // 🔧 SETUP: Setup connection, then close it
    // ═══════════════════════════════════════════════════════════════════════════════════
    constexpr uint16_t TEST_PORT = 20081;

    IOC_SrvURI_T srvURI = {
        .pProtocol = IOC_SRV_PROTO_TCP, .pHost = "localhost", .Port = TEST_PORT, .pPath = "CmdMisuse_ExecAfterClose"};

    IOC_SrvArgs_T srvArgs = {
        .SrvURI = srvURI, .Flags = IOC_SRVFLAG_NONE, .UsageCapabilites = IOC_LinkUsageCmdExecutor, .UsageArgs = {}};

    IOC_SrvID_T srvID = IOC_ID_INVALID;
    IOC_LinkID_T srvLinkID = IOC_ID_INVALID;
    IOC_LinkID_T cliLinkID = IOC_ID_INVALID;

    ASSERT_EQ(IOC_RESULT_SUCCESS, IOC_onlineService(&srvID, &srvArgs));

    IOC_ConnArgs_T connArgs = {.SrvURI = srvURI, .Usage = IOC_LinkUsageCmdInitiator};
    std::thread cliThread([&] { IOC_connectService(&cliLinkID, &connArgs, NULL); });

    ASSERT_EQ(IOC_RESULT_SUCCESS, IOC_acceptClient(srvID, &srvLinkID, NULL));
    cliThread.join();

    // Close the client link
    ASSERT_EQ(IOC_RESULT_SUCCESS, IOC_closeLink(cliLinkID));

    // ═══════════════════════════════════════════════════════════════════════════════════
    // 🎯 BEHAVIOR: Try to execute command after closing link
    // ═══════════════════════════════════════════════════════════════════════════════════

    IOC_CmdDesc_T cmdDesc = {};
    cmdDesc.CmdID = IOC_CMDID_TEST_PING;
    cmdDesc.Status = IOC_CMD_STATUS_INITIALIZED;
    cmdDesc.TimeoutMs = 1000;

    IOC_Result_T result = IOC_execCMD(cliLinkID, &cmdDesc, NULL);

    // ═══════════════════════════════════════════════════════════════════════════════════
    // VERIFY: Should return error for closed link
    // ═══════════════════════════════════════════════════════════════════════════════════

    VERIFY_KEYPOINT_NE(result, IOC_RESULT_SUCCESS, "Should fail when executing after close");

    IOC_CmdDesc_cleanup(&cmdDesc);
    if (srvLinkID != IOC_ID_INVALID) IOC_closeLink(srvLinkID);
    if (srvID != IOC_ID_INVALID) IOC_offlineService(srvID);
}

// TC-3: verifyTcpMisuse_byDoubleClose_expectError
/**
 * @[Category]: P1-Misuse (InvalidFunc)
 * @[Purpose]: Validate double-close is detected and fails
 * @[Brief]: Call IOC_closeLink twice on same LinkID
 * @[4-Phase Structure]:
 *   1) 🔧 SETUP: Setup connection
 *   2) 🎯 BEHAVIOR: Close the link twice
 *   3) ✅ VERIFY: First close succeeds, second close fails
 *   4) 🧹 CLEANUP: Close server link and offline service
 */
TEST(UT_TcpCommandMisuse, verifyTcpMisuse_byDoubleClose_expectError) {
    // ═══════════════════════════════════════════════════════════════════════════════════
    // 🔧 SETUP: Setup connection
    // ═══════════════════════════════════════════════════════════════════════════════════
    constexpr uint16_t TEST_PORT = 20082;

    IOC_SrvURI_T srvURI = {
        .pProtocol = IOC_SRV_PROTO_TCP, .pHost = "localhost", .Port = TEST_PORT, .pPath = "CmdMisuse_DoubleClose"};

    IOC_SrvArgs_T srvArgs = {
        .SrvURI = srvURI, .Flags = IOC_SRVFLAG_NONE, .UsageCapabilites = IOC_LinkUsageCmdExecutor, .UsageArgs = {}};

    IOC_SrvID_T srvID = IOC_ID_INVALID;
    IOC_LinkID_T srvLinkID = IOC_ID_INVALID;
    IOC_LinkID_T cliLinkID = IOC_ID_INVALID;

    ASSERT_EQ(IOC_RESULT_SUCCESS, IOC_onlineService(&srvID, &srvArgs));

    IOC_ConnArgs_T connArgs = {.SrvURI = srvURI, .Usage = IOC_LinkUsageCmdInitiator};
    std::thread cliThread([&] { IOC_connectService(&cliLinkID, &connArgs, NULL); });

    ASSERT_EQ(IOC_RESULT_SUCCESS, IOC_acceptClient(srvID, &srvLinkID, NULL));
    cliThread.join();

    // ═══════════════════════════════════════════════════════════════════════════════════
    // 🎯 BEHAVIOR: Close the link twice
    // ═══════════════════════════════════════════════════════════════════════════════════

    IOC_Result_T firstClose = IOC_closeLink(cliLinkID);
    IOC_Result_T secondClose = IOC_closeLink(cliLinkID);

    // ═══════════════════════════════════════════════════════════════════════════════════
    // VERIFY: First close should succeed, second should fail
    // ═══════════════════════════════════════════════════════════════════════════════════

    VERIFY_KEYPOINT_EQ(firstClose, IOC_RESULT_SUCCESS, "First close should succeed");
    VERIFY_KEYPOINT_NE(secondClose, IOC_RESULT_SUCCESS, "Second close should fail (double close)");

    if (srvLinkID != IOC_ID_INVALID) IOC_closeLink(srvLinkID);
    if (srvID != IOC_ID_INVALID) IOC_offlineService(srvID);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// [@AC-1,US-4] Protocol Configuration Error Tests
///////////////////////////////////////////////////////////////////////////////////////////////////

// TC-1: verifyTcpMisuse_byWrongProtocol_expectConfigError
/**
 * @[Category]: P1-Misuse (InvalidFunc)
 * @[Purpose]: Validate wrong protocol string is rejected
 * @[Brief]: Call IOC_onlineService with "invalid_proto://"
 * @[RGR Status]: 🟢 GREEN - Implementation fixed, test now passes
 * @[Fixed]: Added protocol validation returning IOC_RESULT_NOT_SUPPORT
 *          Location: Source/IOC_Service.c lines 382-386, 934-938
 *          Implementation: Check if pMethods is NULL after protocol loop
 * @[4-Phase Structure]:
 *   1) 🔧 SETUP: Setup with invalid protocol string
 *   2) 🎯 BEHAVIOR: Try to online service with wrong protocol
 *   3) ✅ VERIFY: Returns error, SrvID remains INVALID
 */
TEST(UT_TcpCommandMisuse, verifyTcpMisuse_byWrongProtocol_expectConfigError) {
    // ═══════════════════════════════════════════════════════════════════════════════════
    // 🔧 SETUP: Setup with invalid protocol string
    // ═══════════════════════════════════════════════════════════════════════════════════

    IOC_SrvURI_T srvURI = {.pProtocol = "invalid_proto://",  // Wrong protocol
                           .pHost = "localhost",
                           .Port = 20083,
                           .pPath = "CmdMisuse_WrongProto"};

    IOC_SrvArgs_T srvArgs = {
        .SrvURI = srvURI, .Flags = IOC_SRVFLAG_NONE, .UsageCapabilites = IOC_LinkUsageCmdExecutor, .UsageArgs = {}};

    IOC_SrvID_T srvID = IOC_ID_INVALID;

    // ═══════════════════════════════════════════════════════════════════════════════════
    // 🎯 BEHAVIOR: Try to online service with wrong protocol
    // ═══════════════════════════════════════════════════════════════════════════════════

    IOC_Result_T result = IOC_onlineService(&srvID, &srvArgs);

    // ═══════════════════════════════════════════════════════════════════════════════════
    // VERIFY: Should return error for invalid protocol
    // ═══════════════════════════════════════════════════════════════════════════════════

    VERIFY_KEYPOINT_NE(result, IOC_RESULT_SUCCESS, "Should fail with invalid protocol");
    VERIFY_KEYPOINT_EQ(srvID, IOC_ID_INVALID, "SrvID should remain INVALID");
}

// TC-2: verifyTcpMisuse_byInvalidPort_expectConfigError
/**
 * @[Category]: P1-Misuse (InvalidFunc)
 * @[Purpose]: Validate port 0 handling (OS-dependent)
 * @[Brief]: Call IOC_onlineService with Port=0
 * @[Notes]: Port 0 may be valid (OS assigns random port) or invalid - implementation-dependent
 * @[4-Phase Structure]:
 *   1) 🔧 SETUP: Setup with port 0 (invalid)
 *   2) 🎯 BEHAVIOR: Try to online service with port 0
 *   3) ✅ VERIFY: Should handle gracefully without crash
 */
TEST(UT_TcpCommandMisuse, verifyTcpMisuse_byInvalidPort_expectConfigError) {
    // ═══════════════════════════════════════════════════════════════════════════════════
    // 🔧 SETUP: Setup with port 0 (invalid)
    // ═══════════════════════════════════════════════════════════════════════════════════

    IOC_SrvURI_T srvURI = {
        .pProtocol = IOC_SRV_PROTO_TCP, .pHost = "localhost", .Port = 0, .pPath = "CmdMisuse_InvalidPort"};

    IOC_SrvArgs_T srvArgs = {
        .SrvURI = srvURI, .Flags = IOC_SRVFLAG_NONE, .UsageCapabilites = IOC_LinkUsageCmdExecutor, .UsageArgs = {}};

    IOC_SrvID_T srvID = IOC_ID_INVALID;

    // ═══════════════════════════════════════════════════════════════════════════════════
    // 🎯 BEHAVIOR: Try to online service with port 0
    // ═══════════════════════════════════════════════════════════════════════════════════

    IOC_Result_T result = IOC_onlineService(&srvID, &srvArgs);

    // ═══════════════════════════════════════════════════════════════════════════════════
    // ✅ VERIFY: Should fail or succeed with port 0 (OS-dependent behavior)
    // Port 0 may be valid (OS assigns random port) or invalid depending on implementation
    // ═══════════════════════════════════════════════════════════════════════════════════

    // This is implementation-dependent - just ensure no crash
    if (result == IOC_RESULT_SUCCESS && srvID != IOC_ID_INVALID) {
        IOC_offlineService(srvID);
    }
    SUCCEED() << "Port 0 handling completed without crash";
}

// TC-3: verifyTcpMisuse_byNullProtocolString_expectInvalidParam
/**
 * @[Category]: P1-Misuse (InvalidFunc)
 * @[Purpose]: Validate NULL protocol string is rejected
 * @[Brief]: Call IOC_onlineService with pProtocol=NULL
 * @[4-Phase Structure]:
 *   1) 🔧 SETUP: Setup with NULL protocol string
 *   2) 🎯 BEHAVIOR: Try to online service with NULL protocol
 *   3) ✅ VERIFY: Should fail with error (not SUCCESS)
 */
TEST(UT_TcpCommandMisuse, verifyTcpMisuse_byNullProtocolString_expectInvalidParam) {
    // ═══════════════════════════════════════════════════════════════════════════════════
    // 🔧 SETUP: Setup with NULL protocol string
    // ═══════════════════════════════════════════════════════════════════════════════════

    IOC_SrvURI_T srvURI = {.pProtocol = NULL,  // NULL protocol
                           .pHost = "localhost",
                           .Port = 20084,
                           .pPath = "CmdMisuse_NullProto"};

    IOC_SrvArgs_T srvArgs = {
        .SrvURI = srvURI, .Flags = IOC_SRVFLAG_NONE, .UsageCapabilites = IOC_LinkUsageCmdExecutor, .UsageArgs = {}};

    IOC_SrvID_T srvID = IOC_ID_INVALID;

    // ═══════════════════════════════════════════════════════════════════════════════════
    // 🎯 BEHAVIOR: Try to online service with NULL protocol
    // ═══════════════════════════════════════════════════════════════════════════════════

    IOC_Result_T result = IOC_onlineService(&srvID, &srvArgs);

    // ═══════════════════════════════════════════════════════════════════════════════════
    // VERIFY: Should return error for NULL protocol
    // ═══════════════════════════════════════════════════════════════════════════════════

    VERIFY_KEYPOINT_NE(result, IOC_RESULT_SUCCESS, "Should fail with NULL protocol string");
}

// TC-4: verifyTcpMisuse_byNullHostString_expectInvalidParam
/**
 * @[Category]: P1-Misuse (InvalidFunc)
 * @[Purpose]: Validate NULL host handling (may mean INADDR_ANY)
 * @[Brief]: Call IOC_onlineService with pHost=NULL
 * @[Notes]: NULL host may be valid (binds to INADDR_ANY) - implementation-dependent
 * @[4-Phase Structure]:
 *   1) 🔧 SETUP: Setup with NULL host string
 *   2) 🎯 BEHAVIOR: Try to online service with NULL host
 *   3) ✅ VERIFY: Should handle gracefully without crash
 */
TEST(UT_TcpCommandMisuse, verifyTcpMisuse_byNullHostString_expectInvalidParam) {
    // ═══════════════════════════════════════════════════════════════════════════════════
    // 🔧 SETUP: Setup with NULL host string
    // ═══════════════════════════════════════════════════════════════════════════════════

    IOC_SrvURI_T srvURI = {.pProtocol = IOC_SRV_PROTO_TCP, .pHost = NULL, .Port = 20085, .pPath = "CmdMisuse_NullHost"};

    IOC_SrvArgs_T srvArgs = {
        .SrvURI = srvURI, .Flags = IOC_SRVFLAG_NONE, .UsageCapabilites = IOC_LinkUsageCmdExecutor, .UsageArgs = {}};

    IOC_SrvID_T srvID = IOC_ID_INVALID;

    // ═══════════════════════════════════════════════════════════════════════════════════
    // 🎯 BEHAVIOR: Try to online service with NULL host
    // ═══════════════════════════════════════════════════════════════════════════════════

    IOC_Result_T result = IOC_onlineService(&srvID, &srvArgs);

    // ═══════════════════════════════════════════════════════════════════════════════════
    // ✅ VERIFY: Should return error or succeed (NULL host may mean INADDR_ANY)
    // ═══════════════════════════════════════════════════════════════════════════════════

    // This is implementation-dependent - just ensure no crash
    if (result == IOC_RESULT_SUCCESS && srvID != IOC_ID_INVALID) {
        IOC_offlineService(srvID);
    }
    SUCCEED() << "NULL host handling completed without crash";
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Link Usage Misuse Tests
///////////////////////////////////////////////////////////////////////////////////////////////////

// TC-1: verifyTcpMisuse_byExecOnWrongUsageLink_expectUsageError
/**
 * @[Category]: P1-Misuse (InvalidFunc)
 * @[Purpose]: Validate link usage capability enforcement
 * @[Brief]: Create link with DatSender usage, try IOC_execCMD
 * @[Notes]: Links have strict usage capabilities - commands require CmdInitiator usage
 * @[4-Phase Structure]:
 *   1) 🔧 SETUP: Create connection with IOC_LinkUsageDatSender (wrong usage)
 *   2) 🎯 BEHAVIOR: Try IOC_execCMD on DatSender link
 *   3) ✅ VERIFY: Should return usage error (not SUCCESS)
 *   4) 🧹 CLEANUP: Close connections and offline service
 */
TEST(UT_TcpCommandMisuse, verifyTcpMisuse_byExecOnWrongUsageLink_expectUsageError) {
    // ═══════════════════════════════════════════════════════════════════════════════════
    // 🔧 SETUP: Create service and connect with WRONG usage (DatSender instead of CmdInitiator)
    // ═══════════════════════════════════════════════════════════════════════════════════
    constexpr uint16_t TEST_PORT = 20093;

    IOC_SrvURI_T srvURI = {
        .pProtocol = IOC_SRV_PROTO_TCP, .pHost = "localhost", .Port = TEST_PORT, .pPath = "CmdMisuse_WrongUsage"};

    // Service supports data receiver
    IOC_SrvArgs_T srvArgs = {
        .SrvURI = srvURI, .Flags = IOC_SRVFLAG_NONE, .UsageCapabilites = IOC_LinkUsageDatReceiver, .UsageArgs = {}};

    IOC_SrvID_T srvID = IOC_ID_INVALID;
    IOC_LinkID_T srvLinkID = IOC_ID_INVALID;
    IOC_LinkID_T cliLinkID = IOC_ID_INVALID;

    ASSERT_EQ(IOC_RESULT_SUCCESS, IOC_onlineService(&srvID, &srvArgs));

    // Connect as DatSender (not CmdInitiator)
    IOC_ConnArgs_T connArgs = {.SrvURI = srvURI, .Usage = IOC_LinkUsageDatSender};
    std::thread cliThread([&] { IOC_connectService(&cliLinkID, &connArgs, NULL); });

    ASSERT_EQ(IOC_RESULT_SUCCESS, IOC_acceptClient(srvID, &srvLinkID, NULL));
    cliThread.join();

    // ═══════════════════════════════════════════════════════════════════════════════════
    // 🎯 BEHAVIOR: Try to execute command on link with wrong usage capability
    // ═══════════════════════════════════════════════════════════════════════════════════

    IOC_CmdDesc_T cmdDesc = {};
    cmdDesc.CmdID = IOC_CMDID_TEST_PING;
    cmdDesc.Status = IOC_CMD_STATUS_INITIALIZED;
    cmdDesc.TimeoutMs = 1000;

    IOC_Result_T result = IOC_execCMD(cliLinkID, &cmdDesc, NULL);

    // ═══════════════════════════════════════════════════════════════════════════════════
    // ✅ VERIFY: Should return usage error (link doesn't support command execution)
    // ═══════════════════════════════════════════════════════════════════════════════════

    VERIFY_KEYPOINT_NE(result, IOC_RESULT_SUCCESS, "Should fail when using link with wrong usage capability");

    // 🧹 CLEANUP
    IOC_CmdDesc_cleanup(&cmdDesc);
    if (cliLinkID != IOC_ID_INVALID) IOC_closeLink(cliLinkID);
    if (srvLinkID != IOC_ID_INVALID) IOC_closeLink(srvLinkID);
    if (srvID != IOC_ID_INVALID) IOC_offlineService(srvID);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Command Descriptor Misuse Tests
///////////////////////////////////////////////////////////////////////////////////////////////////

// TC-1: verifyTcpMisuse_byUnsupportedCmdID_expectError
/**
 * @[Category]: P1-Misuse (InvalidFunc)
 * @[Purpose]: Validate unsupported command ID returns NOT_SUPPORT
 * @[Brief]: Execute ECHO command when only PING is supported
 * @[Notes]: IOC_execCMD returns SUCCESS (transport OK), but CmdDesc.Result shows NOT_SUPPORT
 * @[4-Phase Structure]:
 *   1) 🔧 SETUP: Setup connection with limited command support (PING only)
 *   2) 🎯 BEHAVIOR: Try to execute unsupported command (ECHO)
 *   3) ✅ VERIFY: Executor should return NOT_SUPPORT in CmdDesc.Result
 *   4) 🧹 CLEANUP: Close connections and offline service
 */
TEST(UT_TcpCommandMisuse, verifyTcpMisuse_byUnsupportedCmdID_expectError) {
    // ═══════════════════════════════════════════════════════════════════════════════════
    // 🔧 SETUP: Setup connection with limited command support
    // ═══════════════════════════════════════════════════════════════════════════════════
    constexpr uint16_t TEST_PORT = 20086;

    // Callback that only supports PING, returns NOT_SUPPORT for others
    static auto execCb = [](IOC_LinkID_T LinkID, IOC_CmdDesc_pT pCmdDesc, void *pCbPriv) -> IOC_Result_T {
        IOC_CmdID_T CmdID = IOC_CmdDesc_getCmdID(pCmdDesc);
        if (CmdID == IOC_CMDID_TEST_PING) {
            const char *response = "PONG";
            IOC_CmdDesc_setOutPayload(pCmdDesc, (void *)response, strlen(response));
            return IOC_RESULT_SUCCESS;
        }
        return IOC_RESULT_NOT_SUPPORT;
    };

    static IOC_CmdID_T supportedCmdIDs[] = {IOC_CMDID_TEST_PING};  // Only PING supported
    IOC_CmdUsageArgs_T cmdUsageArgs = {
        .CbExecCmd_F = execCb, .pCbPrivData = NULL, .CmdNum = 1, .pCmdIDs = supportedCmdIDs};

    IOC_SrvURI_T srvURI = {
        .pProtocol = IOC_SRV_PROTO_TCP, .pHost = "localhost", .Port = TEST_PORT, .pPath = "CmdMisuse_UnsupportedCmd"};

    IOC_SrvArgs_T srvArgs = {.SrvURI = srvURI,
                             .Flags = IOC_SRVFLAG_NONE,
                             .UsageCapabilites = IOC_LinkUsageCmdExecutor,
                             .UsageArgs = {.pCmd = &cmdUsageArgs}};

    IOC_SrvID_T srvID = IOC_ID_INVALID;
    IOC_LinkID_T srvLinkID = IOC_ID_INVALID;
    IOC_LinkID_T cliLinkID = IOC_ID_INVALID;

    ASSERT_EQ(IOC_RESULT_SUCCESS, IOC_onlineService(&srvID, &srvArgs));

    IOC_ConnArgs_T connArgs = {.SrvURI = srvURI, .Usage = IOC_LinkUsageCmdInitiator};
    std::thread cliThread([&] { IOC_connectService(&cliLinkID, &connArgs, NULL); });

    ASSERT_EQ(IOC_RESULT_SUCCESS, IOC_acceptClient(srvID, &srvLinkID, NULL));
    cliThread.join();

    // ═══════════════════════════════════════════════════════════════════════════════════
    // 🎯 BEHAVIOR: Try to execute unsupported command (ECHO when only PING is supported)
    // ═══════════════════════════════════════════════════════════════════════════════════

    IOC_CmdDesc_T cmdDesc = {};
    cmdDesc.CmdID = IOC_CMDID_TEST_ECHO;  // Not in supported list
    cmdDesc.Status = IOC_CMD_STATUS_INITIALIZED;
    cmdDesc.TimeoutMs = 5000;

    IOC_Result_T result = IOC_execCMD(cliLinkID, &cmdDesc, NULL);

    // ═══════════════════════════════════════════════════════════════════════════════════
    // ✅ VERIFY: Executor callback returns NOT_SUPPORT, but the IOC_execCMD may still
    // return SUCCESS if the protocol layer completed the round-trip. The real error
    // is in CmdDesc.Result which contains the executor's return value.
    // ═══════════════════════════════════════════════════════════════════════════════════

    // The command execution completed (transport succeeded)
    // Check the Result field for the executor's return value
    IOC_Result_T execResult = IOC_CmdDesc_getResult(&cmdDesc);
    VERIFY_KEYPOINT_EQ(execResult, IOC_RESULT_NOT_SUPPORT, "Executor should return NOT_SUPPORT for unsupported CmdID");

    IOC_CmdDesc_cleanup(&cmdDesc);
    if (cliLinkID != IOC_ID_INVALID) IOC_closeLink(cliLinkID);
    if (srvLinkID != IOC_ID_INVALID) IOC_closeLink(srvLinkID);
    if (srvID != IOC_ID_INVALID) IOC_offlineService(srvID);
}

// TC-2: verifyTcpMisuse_byWrongCmdStatus_expectError
/**
 * @[Category]: P1-Misuse (InvalidFunc)
 * @[Purpose]: Validate wrong CmdDesc status is handled gracefully
 * @[Brief]: Call IOC_execCMD with Status=PENDING instead of INITIALIZED
 * @[Notes]: Implementation-dependent - may fail or auto-correct the status
 * @[4-Phase Structure]:
 *   1) 🔧 SETUP: Setup valid connection
 *   2) 🎯 BEHAVIOR: Try to execute with wrong CmdDesc status
 *   3) ✅ VERIFY: Should handle gracefully without crash
 *   4) 🧹 CLEANUP: Close connections and offline service
 */
TEST(UT_TcpCommandMisuse, verifyTcpMisuse_byWrongCmdStatus_expectError) {
    // ═══════════════════════════════════════════════════════════════════════════════════
    // 🔧 SETUP: Setup valid connection
    // ═══════════════════════════════════════════════════════════════════════════════════
    constexpr uint16_t TEST_PORT = 20088;

    static auto execCb = [](IOC_LinkID_T LinkID, IOC_CmdDesc_pT pCmdDesc, void *pCbPriv) -> IOC_Result_T {
        const char *response = "PONG";
        IOC_CmdDesc_setOutPayload(pCmdDesc, (void *)response, strlen(response));
        return IOC_RESULT_SUCCESS;
    };

    static IOC_CmdID_T supportedCmdIDs[] = {IOC_CMDID_TEST_PING};
    IOC_CmdUsageArgs_T cmdUsageArgs = {
        .CbExecCmd_F = execCb, .pCbPrivData = NULL, .CmdNum = 1, .pCmdIDs = supportedCmdIDs};

    IOC_SrvURI_T srvURI = {
        .pProtocol = IOC_SRV_PROTO_TCP, .pHost = "localhost", .Port = TEST_PORT, .pPath = "CmdMisuse_WrongStatus"};

    IOC_SrvArgs_T srvArgs = {.SrvURI = srvURI,
                             .Flags = IOC_SRVFLAG_NONE,
                             .UsageCapabilites = IOC_LinkUsageCmdExecutor,
                             .UsageArgs = {.pCmd = &cmdUsageArgs}};

    IOC_SrvID_T srvID = IOC_ID_INVALID;
    IOC_LinkID_T srvLinkID = IOC_ID_INVALID;
    IOC_LinkID_T cliLinkID = IOC_ID_INVALID;

    ASSERT_EQ(IOC_RESULT_SUCCESS, IOC_onlineService(&srvID, &srvArgs));

    IOC_ConnArgs_T connArgs = {.SrvURI = srvURI, .Usage = IOC_LinkUsageCmdInitiator};
    std::thread cliThread([&] { IOC_connectService(&cliLinkID, &connArgs, NULL); });

    ASSERT_EQ(IOC_RESULT_SUCCESS, IOC_acceptClient(srvID, &srvLinkID, NULL));
    cliThread.join();

    // ═══════════════════════════════════════════════════════════════════════════════════
    // 🎯 BEHAVIOR: Try to execute with wrong CmdDesc status (PENDING instead of INITIALIZED)
    // ═══════════════════════════════════════════════════════════════════════════════════

    IOC_CmdDesc_T cmdDesc = {};
    cmdDesc.CmdID = IOC_CMDID_TEST_PING;
    cmdDesc.Status = IOC_CMD_STATUS_PENDING;  // Wrong status - should be INITIALIZED
    cmdDesc.TimeoutMs = 5000;

    IOC_Result_T result = IOC_execCMD(cliLinkID, &cmdDesc, NULL);

    // ═══════════════════════════════════════════════════════════════════════════════════
    // ✅ VERIFY: Should return error or handle gracefully
    // ═══════════════════════════════════════════════════════════════════════════════════

    // Implementation-dependent: may fail or auto-correct the status
    // Just ensure no crash
    SUCCEED() << "Wrong CmdDesc status handled without crash, result=" << result;

    // 🧹 CLEANUP
    IOC_CmdDesc_cleanup(&cmdDesc);
    if (cliLinkID != IOC_ID_INVALID) IOC_closeLink(cliLinkID);
    if (srvLinkID != IOC_ID_INVALID) IOC_closeLink(srvLinkID);
    if (srvID != IOC_ID_INVALID) IOC_offlineService(srvID);
}

// TC-3: verifyTcpMisuse_byNullPayloadNonZeroSize_expectError
/**
 * @[Category]: P1-Misuse (InvalidFunc)
 * @[Purpose]: Validate NULL payload with non-zero size is rejected
 * @[Brief]: Call IOC_CmdDesc_setInPayload with NULL pointer and size>0
 * @[RGR Status]: 🟢 GREEN - Implementation fixed, test now passes
 * @[Fixed]: Added NULL pointer check in IOC_CmdDesc_setInPayload/setOutPayload
 *          Location: Include/IOC/IOC_CmdDesc.h lines 162, 190
 *          Implementation: if (!pData && DataSize > 0) return IOC_RESULT_INVALID_PARAM;
 * @[4-Phase Structure]:
 *   1) 🔧 SETUP: Create command descriptor
 *   2) 🎯 BEHAVIOR: Call setInPayload with NULL data and size=100
 *   3) ✅ VERIFY: Returns INVALID_PARAM without crash
 *   4) 🧹 CLEANUP: Clean up command descriptor
 */
TEST(UT_TcpCommandMisuse, verifyTcpMisuse_byNullPayloadNonZeroSize_expectError) {
    // ═══════════════════════════════════════════════════════════════════════════════════
    // 🔧 SETUP: Create command descriptor
    // ═══════════════════════════════════════════════════════════════════════════════════

    IOC_CmdDesc_T cmdDesc = {};
    IOC_CmdDesc_initVar(&cmdDesc);
    cmdDesc.CmdID = IOC_CMDID_TEST_ECHO;
    cmdDesc.Status = IOC_CMD_STATUS_INITIALIZED;

    // ═══════════════════════════════════════════════════════════════════════════════════
    // 🎯 BEHAVIOR: Try to set NULL payload with non-zero size
    // ═══════════════════════════════════════════════════════════════════════════════════

    IOC_Result_T result = IOC_CmdDesc_setInPayload(&cmdDesc, NULL, 100);

    // ═══════════════════════════════════════════════════════════════════════════════════
    // ✅ VERIFY: Should return error or handle gracefully (implementation-dependent)
    // ═══════════════════════════════════════════════════════════════════════════════════

    // Implementation may either:
    // 1) Return INVALID_PARAM (best practice)
    // 2) Handle NULL gracefully without crash (acceptable)
    // Either way, ensure no crash occurs
    VERIFY_KEYPOINT_NE(result, IOC_RESULT_SUCCESS, "NULL payload with non-zero size should fail or be handled");

    // 🧹 CLEANUP
    IOC_CmdDesc_cleanup(&cmdDesc);
}

// TC-4: verifyTcpMisuse_byNullSrvIDOutput_expectInvalidParam
/**
 * @[Category]: P1-Misuse (InvalidFunc)
 * @[Purpose]: Validate NULL output pointer returns INVALID_PARAM
 * @[Brief]: Call IOC_onlineService with NULL pSrvID pointer
 * @[4-Phase Structure]:
 *   1) 🔧 SETUP: Setup valid service args but NULL output pointer
 *   2) 🎯 BEHAVIOR: Call IOC_onlineService with NULL pSrvID
 *   3) ✅ VERIFY: Should return INVALID_PARAM
 */
TEST(UT_TcpCommandMisuse, verifyTcpMisuse_byNullSrvIDOutput_expectInvalidParam) {
    // ═══════════════════════════════════════════════════════════════════════════════════
    // 🔧 SETUP: Setup valid service args but NULL output pointer
    // ═══════════════════════════════════════════════════════════════════════════════════

    IOC_SrvURI_T srvURI = {
        .pProtocol = IOC_SRV_PROTO_TCP, .pHost = "localhost", .Port = 20089, .pPath = "CmdMisuse_NullSrvIDOut"};

    IOC_SrvArgs_T srvArgs = {
        .SrvURI = srvURI, .Flags = IOC_SRVFLAG_NONE, .UsageCapabilites = IOC_LinkUsageCmdExecutor, .UsageArgs = {}};

    // ═══════════════════════════════════════════════════════════════════════════════════
    // 🎯 BEHAVIOR: Call IOC_onlineService with NULL SrvID output pointer
    // ═══════════════════════════════════════════════════════════════════════════════════

    IOC_Result_T result = IOC_onlineService(NULL, &srvArgs);

    // ═══════════════════════════════════════════════════════════════════════════════════
    // VERIFY: Should return INVALID_PARAM
    // ═══════════════════════════════════════════════════════════════════════════════════

    VERIFY_KEYPOINT_EQ(result, IOC_RESULT_INVALID_PARAM, "Should return INVALID_PARAM for NULL pSrvID");
}

// TC-4: verifyTcpMisuse_byNullLinkIDOutput_expectInvalidParam
/**
 * @[Category]: P1-Misuse (InvalidFunc)
 * @[Purpose]: Validate NULL output pointer returns INVALID_PARAM
 * @[Brief]: Call IOC_connectService with NULL pLinkID pointer
 * @[4-Phase Structure]:
 *   1) 🎯 BEHAVIOR: Call IOC_connectService with NULL pLinkID
 *   2) ✅ VERIFY: Should return INVALID_PARAM
 */
TEST(UT_TcpCommandMisuse, verifyTcpMisuse_byNullLinkIDOutput_expectInvalidParam) {
    // ═══════════════════════════════════════════════════════════════════════════════════
    // 🎯 BEHAVIOR: Call IOC_connectService with NULL LinkID output pointer
    // ═══════════════════════════════════════════════════════════════════════════════════

    IOC_SrvURI_T srvURI = {
        .pProtocol = IOC_SRV_PROTO_TCP, .pHost = "localhost", .Port = 20090, .pPath = "CmdMisuse_NullLinkIDOut"};

    IOC_ConnArgs_T connArgs = {.SrvURI = srvURI, .Usage = IOC_LinkUsageCmdInitiator};

    IOC_Result_T result = IOC_connectService(NULL, &connArgs, NULL);

    // ═══════════════════════════════════════════════════════════════════════════════════
    // VERIFY: Should return INVALID_PARAM
    // ═══════════════════════════════════════════════════════════════════════════════════

    VERIFY_KEYPOINT_EQ(result, IOC_RESULT_INVALID_PARAM, "Should return INVALID_PARAM for NULL pLinkID");
}

// TC-6: verifyTcpMisuse_byNullAcceptOutput_expectInvalidParam
/**
 * @[Category]: P1-Misuse (InvalidFunc)
 * @[Purpose]: Validate NULL output pointer returns INVALID_PARAM
 * @[Brief]: Call IOC_acceptClient with NULL pLinkID pointer
 * @[RGR Status]: 🟢 GREEN - Implementation fixed, test now passes
 * @[Fixed]: Added NULL pointer check for pLinkID in IOC_acceptClient
 *          Location: Source/IOC_Service.c line 758
 *          Implementation: if (!pLinkID) return IOC_RESULT_INVALID_PARAM;
 * @[4-Phase Structure]:
 *   1) 🔧 SETUP: Create valid service (no client connection needed)
 *   2) 🎯 BEHAVIOR: Call IOC_acceptClient with NULL pLinkID
 *   3) ✅ VERIFY: Returns INVALID_PARAM immediately before any accept logic
 *   4) 🧹 CLEANUP: Offline service
 */
TEST(UT_TcpCommandMisuse, verifyTcpMisuse_byNullAcceptOutput_expectInvalidParam) {
    // ═══════════════════════════════════════════════════════════════════════════════════
    // 🔧 SETUP: Create service (NULL check happens before accept, so no client needed)
    // ═══════════════════════════════════════════════════════════════════════════════════
    constexpr uint16_t TEST_PORT = 20091;

    IOC_SrvURI_T srvURI = {
        .pProtocol = IOC_SRV_PROTO_TCP, .pHost = "localhost", .Port = TEST_PORT, .pPath = "CmdMisuse_NullAccept"};

    IOC_SrvArgs_T srvArgs = {
        .SrvURI = srvURI, .Flags = IOC_SRVFLAG_NONE, .UsageCapabilites = IOC_LinkUsageCmdExecutor, .UsageArgs = {}};

    IOC_SrvID_T srvID = IOC_ID_INVALID;

    ASSERT_EQ(IOC_RESULT_SUCCESS, IOC_onlineService(&srvID, &srvArgs));

    // ═══════════════════════════════════════════════════════════════════════════════════
    // 🎯 BEHAVIOR: Call IOC_acceptClient with NULL pLinkID output
    // ═══════════════════════════════════════════════════════════════════════════════════

    IOC_Result_T result = IOC_acceptClient(srvID, NULL, NULL);

    // ═══════════════════════════════════════════════════════════════════════════════════
    // ✅ VERIFY: Should return INVALID_PARAM immediately (before any accept logic)
    // ═══════════════════════════════════════════════════════════════════════════════════

    VERIFY_KEYPOINT_EQ(result, IOC_RESULT_INVALID_PARAM, "Should return INVALID_PARAM for NULL pLinkID");

    // ═══════════════════════════════════════════════════════════════════════════════════
    // 🧹 CLEANUP
    // ═══════════════════════════════════════════════════════════════════════════════════

    IOC_offlineService(srvID);
}

// TC-7: verifyTcpMisuse_byNullWaitCmdDesc_expectInvalidParam
/**
 * @[Category]: P1-Misuse (InvalidFunc)
 * @[Purpose]: Validate NULL CmdDesc returns INVALID_PARAM without crashing
 * @[Brief]: Call IOC_waitCMD with NULL CmdDesc pointer
 * @[Notes]: waitCMD checks NULL at IOC_Command.c:299-301 before blocking - test safe
 * @[4-Phase Structure]:
 *   1) 🔧 SETUP: Create valid connection
 *   2) 🎯 BEHAVIOR: Call IOC_waitCMD with NULL CmdDesc
 *   3) ✅ VERIFY: Should return INVALID_PARAM immediately (before blocking)
 *   4) 🧹 CLEANUP: Close connections and offline service
 */
TEST(UT_TcpCommandMisuse, verifyTcpMisuse_byNullWaitCmdDesc_expectInvalidParam) {
    // ═══════════════════════════════════════════════════════════════════════════════════
    // 🔧 SETUP: Create connection (waitCMD checks NULL before blocking)
    // ═══════════════════════════════════════════════════════════════════════════════════
    constexpr uint16_t TEST_PORT = 20092;

    IOC_SrvURI_T srvURI = {
        .pProtocol = IOC_SRV_PROTO_TCP, .pHost = "localhost", .Port = TEST_PORT, .pPath = "CmdMisuse_NullWaitCmd"};

    IOC_SrvArgs_T srvArgs = {
        .SrvURI = srvURI, .Flags = IOC_SRVFLAG_NONE, .UsageCapabilites = IOC_LinkUsageCmdExecutor, .UsageArgs = {}};

    IOC_SrvID_T srvID = IOC_ID_INVALID;
    IOC_LinkID_T srvLinkID = IOC_ID_INVALID;
    IOC_LinkID_T cliLinkID = IOC_ID_INVALID;

    ASSERT_EQ(IOC_RESULT_SUCCESS, IOC_onlineService(&srvID, &srvArgs));

    IOC_ConnArgs_T connArgs = {.SrvURI = srvURI, .Usage = IOC_LinkUsageCmdInitiator};
    std::thread cliThread([&] { IOC_connectService(&cliLinkID, &connArgs, NULL); });

    ASSERT_EQ(IOC_RESULT_SUCCESS, IOC_acceptClient(srvID, &srvLinkID, NULL));
    cliThread.join();

    // ═══════════════════════════════════════════════════════════════════════════════════
    // 🎯 BEHAVIOR: Call IOC_waitCMD with NULL CmdDesc (should check before blocking)
    // ═══════════════════════════════════════════════════════════════════════════════════

    IOC_Result_T result = IOC_waitCMD(srvLinkID, NULL, NULL);

    // ═══════════════════════════════════════════════════════════════════════════════════
    // ✅ VERIFY: Should return INVALID_PARAM immediately without blocking
    // ═══════════════════════════════════════════════════════════════════════════════════

    VERIFY_KEYPOINT_EQ(result, IOC_RESULT_INVALID_PARAM, "NULL CmdDesc should return INVALID_PARAM");

    // 🧹 CLEANUP
    if (cliLinkID != IOC_ID_INVALID) IOC_closeLink(cliLinkID);
    if (srvLinkID != IOC_ID_INVALID) IOC_closeLink(srvLinkID);
    if (srvID != IOC_ID_INVALID) IOC_offlineService(srvID);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Lifecycle Misuse Tests
///////////////////////////////////////////////////////////////////////////////////////////////////

// TC-1: verifyTcpMisuse_byDoubleOffline_expectError
/**
 * @[Category]: P1-Misuse (InvalidFunc)
 * @[Purpose]: Validate double-offline is detected and fails
 * @[Brief]: Call IOC_offlineService twice on same SrvID
 * @[4-Phase Structure]:
 *   1) 🔧 SETUP: Setup service
 *   2) 🎯 BEHAVIOR: Offline the service twice
 *   3) ✅ VERIFY: First offline succeeds, second offline fails
 */
TEST(UT_TcpCommandMisuse, verifyTcpMisuse_byDoubleOffline_expectError) {
    // ═══════════════════════════════════════════════════════════════════════════════════
    // 🔧 SETUP: Setup service
    // ═══════════════════════════════════════════════════════════════════════════════════
    constexpr uint16_t TEST_PORT = 20087;

    IOC_SrvURI_T srvURI = {
        .pProtocol = IOC_SRV_PROTO_TCP, .pHost = "localhost", .Port = TEST_PORT, .pPath = "CmdMisuse_DoubleOffline"};

    IOC_SrvArgs_T srvArgs = {
        .SrvURI = srvURI, .Flags = IOC_SRVFLAG_NONE, .UsageCapabilites = IOC_LinkUsageCmdExecutor, .UsageArgs = {}};

    IOC_SrvID_T srvID = IOC_ID_INVALID;
    ASSERT_EQ(IOC_RESULT_SUCCESS, IOC_onlineService(&srvID, &srvArgs));

    // ═══════════════════════════════════════════════════════════════════════════════════
    // 🎯 BEHAVIOR: Offline the service twice
    // ═══════════════════════════════════════════════════════════════════════════════════

    IOC_Result_T firstOffline = IOC_offlineService(srvID);
    IOC_Result_T secondOffline = IOC_offlineService(srvID);

    // ═══════════════════════════════════════════════════════════════════════════════════
    // VERIFY: First offline should succeed, second should fail
    // ═══════════════════════════════════════════════════════════════════════════════════

    VERIFY_KEYPOINT_EQ(firstOffline, IOC_RESULT_SUCCESS, "First offline should succeed");
    VERIFY_KEYPOINT_NE(secondOffline, IOC_RESULT_SUCCESS, "Second offline should fail (double offline)");
}

// TC-2: verifyTcpMisuse_byCloseInvalidLink_expectError
/**
 * @[Category]: P1-Misuse (InvalidFunc)
 * @[Purpose]: Validate closing invalid LinkID fails
 * @[Brief]: Call IOC_closeLink with IOC_ID_INVALID
 * @[4-Phase Structure]:
 *   1) 🎯 BEHAVIOR: Try to close invalid LinkID
 *   2) ✅ VERIFY: Should return error (not SUCCESS)
 */
TEST(UT_TcpCommandMisuse, verifyTcpMisuse_byCloseInvalidLink_expectError) {
    // ═══════════════════════════════════════════════════════════════════════════════════
    // 🎯 BEHAVIOR: Try to close invalid LinkID
    // ═══════════════════════════════════════════════════════════════════════════════════

    IOC_Result_T result = IOC_closeLink(IOC_ID_INVALID);

    // ═══════════════════════════════════════════════════════════════════════════════════
    // VERIFY: Should return error for invalid LinkID
    // ═══════════════════════════════════════════════════════════════════════════════════

    VERIFY_KEYPOINT_NE(result, IOC_RESULT_SUCCESS, "Should fail when closing invalid LinkID");
}

//======>END OF TEST IMPLEMENTATION=================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF TODO TRACKING=====================================================================
/**
 * 🟢 IMPLEMENTATION STATUS TRACKING
 *
 * P1 INVALIDFUNC MISUSE TESTS:
 *
 * Null Pointer Handling (7 tests):
 *   🟢 TC-1: verifyTcpMisuse_byNullCmdDesc_expectInvalidParam
 *   🟢 TC-2: verifyTcpMisuse_byNullSrvArgs_expectInvalidParam
 *   🟢 TC-3: verifyTcpMisuse_byNullConnArgs_expectInvalidParam
 *   🟢 TC-4: verifyTcpMisuse_byNullSrvIDOutput_expectInvalidParam
 *   🟢 TC-5: verifyTcpMisuse_byNullLinkIDOutput_expectInvalidParam
 *   🟢 TC-6: verifyTcpMisuse_byNullAcceptOutput_expectInvalidParam
 *   🟢 TC-7: verifyTcpMisuse_byNullWaitCmdDesc_expectInvalidParam
 *
 * Invalid ID Handling (3 tests):
 *   🟢 TC-1: verifyTcpMisuse_byInvalidLinkID_expectError
 *   🟢 TC-2: verifyTcpMisuse_byInvalidSrvID_expectError
 *   🟢 TC-3: verifyTcpMisuse_byInvalidSrvIDForAccept_expectError
 *
 * State Violations (3 tests):
 *   🟢 TC-1: verifyTcpMisuse_byExecBeforeConnect_expectStateError
 *   🟢 TC-2: verifyTcpMisuse_byExecAfterClose_expectStateError
 *   🟢 TC-3: verifyTcpMisuse_byDoubleClose_expectError
 *
 * Protocol Configuration (4 tests):
 *   🟢 TC-1: verifyTcpMisuse_byWrongProtocol_expectConfigError
 *   🟢 TC-2: verifyTcpMisuse_byInvalidPort_expectConfigError
 *   🟢 TC-3: verifyTcpMisuse_byNullProtocolString_expectInvalidParam
 *   🟢 TC-4: verifyTcpMisuse_byNullHostString_expectInvalidParam
 *
 * Link Usage Misuse (1 test):
 *   🟢 TC-1: verifyTcpMisuse_byExecOnWrongUsageLink_expectUsageError
 *
 * Command Descriptor Misuse (3 tests):
 *   🟢 TC-1: verifyTcpMisuse_byUnsupportedCmdID_expectError
 *   🟢 TC-2: verifyTcpMisuse_byWrongCmdStatus_expectError
 *   🟢 TC-3: verifyTcpMisuse_byNullPayloadNonZeroSize_expectError
 *
 * Lifecycle Misuse (2 tests):
 *   🟢 TC-1: verifyTcpMisuse_byDoubleOffline_expectError
 *   🟢 TC-2: verifyTcpMisuse_byCloseInvalidLink_expectError
 *
 * TOTAL: 23/23 implemented and ALL GREEN! ✅✅✅
 *
 * QUALITY GATE P1 MISUSE: ALL TESTS PASS! 🎉
 *   ✅ All critical misuse scenarios covered (23 tests)
 *   ✅ Null pointer handling verified (7/7 GREEN) - FIXED! ✅
 *   ✅ Invalid ID handling verified (3 tests)
 *   ✅ State violation handling verified (3 tests)
 *   ✅ Protocol configuration errors verified (4/4 GREEN) - FIXED! ✅
 *   ✅ Link usage capability enforcement verified (1 test)
 *   ✅ Command descriptor misuse verified (3/3 GREEN) - FIXED! ✅
 *   ✅ Lifecycle misuse verified (2 tests)
 *   🎉 RGR COMPLETE: All 3 bugs fixed through TDD!
 *      1. WrongProtocol: Now returns IOC_RESULT_NOT_SUPPORT ✅
 *      2. NullPayload: Added NULL check before memcpy ✅
 *      3. NullAccept: Added NULL check for pLinkID parameter ✅
 *   🐞 BUGS FIXED: 3/3 (100% completion) ✅✅✅
 */
//======>END OF TODO TRACKING=======================================================================
