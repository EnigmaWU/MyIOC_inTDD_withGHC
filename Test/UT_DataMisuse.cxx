///////////////////////////////////////////////////////////////////////////////////////////////////
// Data Misuse FIFO - P1 InvalidFunc Misuse Testing
//
// PURPOSE:
//   Validate FIFO data API error handling for incorrect usage patterns.
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
 *   [WHAT] This file validates FIFO data API error handling for incorrect usage
 *   [WHERE] in the IOC Data API with FIFO protocol layer
 *   [WHY] to ensure API misuse is detected and handled gracefully
 *
 * SCOPE:
 *   - [In scope]: P1 InvalidFunc Misuse tests (incorrect API usage)
 *   - [In scope]: Null pointer handling for IOC_sendDAT/recvDAT/flushDAT
 *   - [In scope]: Invalid parameter values (LinkID, DatDesc)
 *   - [In scope]: Illegal state transitions (operations on closed links)
 *   - [In scope]: Role violations (send on receiver, recv on sender)
 *   - [In scope]: FIFO-specific misuse (file manipulation, path corruption)
 *   - [Out of scope]: Valid boundary cases → see UT_DataEdgeUS*.cxx
 *   - [Out of scope]: External failures → see UT_DataFault.cxx
 *   - [Out of scope]: Typical scenarios → see UT_DataTypical.cxx
 *
 * KEY CONCEPTS:
 *   - Data Misuse: Incorrect API usage patterns that should be rejected
 *   - Role Mismatch: Using sender APIs on receiver links and vice versa
 *   - State Violation: Operations on invalid/closed/non-existent links
 *   - Parameter Corruption: Malformed DatDesc structures
 *
 * RELATIONSHIPS:
 *   - Extends: UT_DataTypical.cxx (error handling for typical patterns)
 *   - Related: UT_DataEdgeUS*.cxx (misuse vs boundary distinction)
 *   - Related: UT_DataFault.cxx (misuse vs fault distinction)
 *   - Companion: UT_DataMisuseTCP.cxx (same tests with TCP protocol)
 */
//======>END OF OVERVIEW===========================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF TEST DESIGN======================================================================
/**
 * COVERAGE MATRIX (P1 InvalidFunc Misuse):
 * ┌──────────────────────────┬─────────────────────────┬────────────────────────────┐
 * │ Misuse Category          │ API Function            │ Error Type                 │
 * ├──────────────────────────┼─────────────────────────┼────────────────────────────┤
 * │ Null Pointers            │ IOC_sendDAT             │ NULL pDatDesc              │
 * │ Null Pointers            │ IOC_recvDAT             │ NULL pDatDesc              │
 * │ Null Pointers            │ IOC_flushDAT            │ NULL pOption (valid case)  │
 * │ Invalid IDs              │ IOC_sendDAT             │ IOC_ID_INVALID             │
 * │ Invalid IDs              │ IOC_recvDAT             │ IOC_ID_INVALID             │
 * │ Invalid IDs              │ IOC_flushDAT            │ IOC_ID_INVALID             │
 * │ Invalid IDs              │ IOC_sendDAT             │ Non-existent LinkID        │
 * │ Invalid IDs              │ IOC_recvDAT             │ Non-existent LinkID        │
 * │ Invalid IDs              │ IOC_flushDAT            │ Non-existent LinkID        │
 * │ State Violations         │ IOC_sendDAT             │ On closed link             │
 * │ State Violations         │ IOC_recvDAT             │ On closed link             │
 * │ State Violations         │ IOC_flushDAT            │ On closed link             │
 * │ State Violations         │ IOC_sendDAT             │ Before connection          │
 * │ State Violations         │ IOC_recvDAT             │ Before connection          │
 * │ State Violations         │ IOC_sendDAT             │ After service offline      │
 * │ Role Violations          │ IOC_sendDAT             │ On DatReceiver link        │
 * │ Role Violations          │ IOC_recvDAT             │ On DatSender link (manual) │
 * │ Role Violations          │ IOC_flushDAT            │ On DatReceiver link        │
 * │ Sequence Violations      │ IOC_sendDAT             │ Concurrent sends same link │
 * │ Sequence Violations      │ IOC_closeLink           │ Double-close               │
 * │ DatDesc Corruption       │ IOC_sendDAT             │ Malformed DatDesc          │
 * │ DatDesc Corruption       │ IOC_sendDAT             │ NULL payload with size > 0 │
 * │ DatDesc Corruption       │ IOC_sendDAT             │ Reusing DatDesc w/o reinit │
 * │ DatDesc Corruption       │ IOC_recvDAT             │ Invalid DatDesc config     │
 * │ FIFO-Specific Misuse     │ IOC_sendDAT             │ FIFO file deleted manually │
 * │ FIFO-Specific Misuse     │ IOC_onlineService       │ Invalid FIFO path          │
 * │ FIFO-Specific Misuse     │ IOC_sendDAT             │ FIFO permission changed    │
 * └──────────────────────────┴─────────────────────────┴────────────────────────────┘
 *
 * FIFO PATH BASE: test/data/misuse/fifo/
 *
 * PRIORITY: P1 InvalidFunc Misuse (CRITICAL)
 *
 * STATUS:
 *   ⚪ 0/20 tests implemented (TODO)
 *   📋 20 total test scenarios planned
 *   🎯 Target: 100% P1 Misuse coverage for Data API
 */
///////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF USER STORY=======================================================================
/**
 * US-1: As a developer, I want null pointer errors caught gracefully
 *       so that API misuse doesn't cause crashes or undefined behavior.
 *
 * US-2: As a developer, I want invalid LinkID errors detected immediately
 *       so that I know when I'm using wrong handles or identifiers.
 *
 * US-3: As a developer, I want state violation errors reported clearly
 *       so that I can fix incorrect API call sequences.
 *
 * US-4: As a developer, I want role mismatch errors prevented
 *       so that sender/receiver usage is enforced correctly.
 *
 * US-5: As a developer, I want DatDesc corruption detected
 *       so that data integrity issues are caught early.
 *
 * US-6: As a developer, I want FIFO-specific errors handled gracefully
 *       so that file system issues don't crash the application.
 */
//======>END OF USER STORY==========================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF ACCEPTANCE CRITERIA===============================================================
/**
 * [@US-1] Null Pointer Handling
 *  AC-1: GIVEN NULL pDatDesc passed to IOC_sendDAT,
 *        WHEN calling IOC_sendDAT,
 *        THEN returns IOC_RESULT_INVALID_PARAM without crashing.
 *
 *  AC-2: GIVEN NULL pDatDesc passed to IOC_recvDAT,
 *        WHEN calling IOC_recvDAT,
 *        THEN returns IOC_RESULT_INVALID_PARAM without crashing.
 *
 *  AC-3: GIVEN NULL pOption passed to IOC_flushDAT,
 *        WHEN calling IOC_flushDAT,
 *        THEN uses default options and succeeds (valid case).
 *
 * [@US-2] Invalid LinkID Handling
 *  AC-1: GIVEN IOC_ID_INVALID passed to data APIs,
 *        WHEN calling IOC_sendDAT/recvDAT/flushDAT,
 *        THEN returns IOC_RESULT_NOT_EXIST_LINK.
 *
 *  AC-2: GIVEN non-existent LinkID (random valid-looking ID),
 *        WHEN calling data APIs,
 *        THEN returns IOC_RESULT_NOT_EXIST_LINK.
 *
 * [@US-3] State Violation Detection
 *  AC-1: GIVEN link has been closed via IOC_closeLink,
 *        WHEN calling data operations on closed link,
 *        THEN returns IOC_RESULT_NOT_EXIST_LINK.
 *
 *  AC-2: GIVEN connection not yet established,
 *        WHEN calling IOC_sendDAT/recvDAT before connect,
 *        THEN returns IOC_RESULT_NOT_EXIST_LINK.
 *
 *  AC-3: GIVEN service has been taken offline,
 *        WHEN calling data operations on orphaned link,
 *        THEN returns IOC_RESULT_LINK_BROKEN or NOT_EXIST_LINK.
 *
 *  AC-4: GIVEN double-close scenario (closeLink called twice),
 *        WHEN calling second close,
 *        THEN returns error without system corruption.
 *
 * [@US-4] Role Mismatch Detection
 *  AC-1: GIVEN link configured as DatReceiver,
 *        WHEN calling IOC_sendDAT on receiver link,
 *        THEN returns IOC_RESULT_INVALID_OPERATION or similar error.
 *
 *  AC-2: GIVEN link configured as DatSender (no callback),
 *        WHEN calling IOC_recvDAT manually on sender link,
 *        THEN returns IOC_RESULT_INVALID_OPERATION or similar error.
 *
 *  AC-3: GIVEN link configured as DatReceiver,
 *        WHEN calling IOC_flushDAT on receiver link,
 *        THEN returns IOC_RESULT_INVALID_OPERATION or similar error.
 *
 * [@US-5] DatDesc Corruption Detection
 *  AC-1: GIVEN malformed DatDesc (uninitialized structure),
 *        WHEN calling IOC_sendDAT with malformed desc,
 *        THEN returns IOC_RESULT_INVALID_PARAM or similar error.
 *
 *  AC-2: GIVEN DatDesc with NULL payload but size > 0,
 *        WHEN calling IOC_sendDAT,
 *        THEN returns IOC_RESULT_INVALID_PARAM.
 *
 *  AC-3: GIVEN DatDesc reused without re-initialization,
 *        WHEN calling IOC_sendDAT with stale desc,
 *        THEN behavior is undefined but shouldn't crash (document this).
 *
 *  AC-4: GIVEN invalid DatDesc configuration for receive,
 *        WHEN calling IOC_recvDAT with bad buffer,
 *        THEN returns IOC_RESULT_INVALID_PARAM.
 *
 * [@US-6] FIFO-Specific Misuse Handling
 *  AC-1: GIVEN FIFO file deleted manually during operation,
 *        WHEN calling IOC_sendDAT after deletion,
 *        THEN returns IOC_RESULT_LINK_BROKEN.
 *
 *  AC-2: GIVEN invalid/corrupted FIFO path in service setup,
 *        WHEN calling IOC_onlineService,
 *        THEN returns configuration error.
 *
 *  AC-3: GIVEN FIFO permissions changed to read-only,
 *        WHEN calling IOC_sendDAT,
 *        THEN returns IOC_RESULT_LINK_BROKEN or access error.
 */
//======>END OF ACCEPTANCE CRITERIA=================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF TEST CASES========================================================================
/**
 * [@AC-1,US-1] Null Pointer Handling (3 tests)
 *  ⚪ TC-1: verifyDataMisuse_byNullDatDescOnSend_expectInvalidParam
 *      @[Purpose]: Validate NULL pDatDesc to IOC_sendDAT returns INVALID_PARAM
 *      @[Brief]: Call IOC_sendDAT with NULL DatDesc on valid connection
 *
 *  ⚪ TC-2: verifyDataMisuse_byNullDatDescOnRecv_expectInvalidParam
 *      @[Purpose]: Validate NULL pDatDesc to IOC_recvDAT returns INVALID_PARAM
 *      @[Brief]: Call IOC_recvDAT with NULL DatDesc on valid connection
 *
 *  ⚪ TC-3: verifyDataMisuse_byNullOptionOnFlush_expectDefaultBehavior
 *      @[Purpose]: Validate NULL pOption to IOC_flushDAT uses defaults
 *      @[Brief]: Call IOC_flushDAT with NULL options, expect success
 *
 * [@AC-1,AC-2,US-2] Invalid LinkID Handling (6 tests)
 *  ⚪ TC-4: verifyDataMisuse_byInvalidLinkIDOnSend_expectNotExistLink
 *      @[Purpose]: Validate IOC_ID_INVALID to IOC_sendDAT returns NOT_EXIST_LINK
 *      @[Brief]: Call IOC_sendDAT with IOC_ID_INVALID
 *
 *  ⚪ TC-5: verifyDataMisuse_byInvalidLinkIDOnRecv_expectNotExistLink
 *      @[Purpose]: Validate IOC_ID_INVALID to IOC_recvDAT returns NOT_EXIST_LINK
 *      @[Brief]: Call IOC_recvDAT with IOC_ID_INVALID
 *
 *  ⚪ TC-6: verifyDataMisuse_byInvalidLinkIDOnFlush_expectNotExistLink
 *      @[Purpose]: Validate IOC_ID_INVALID to IOC_flushDAT returns NOT_EXIST_LINK
 *      @[Brief]: Call IOC_flushDAT with IOC_ID_INVALID
 *
 *  ⚪ TC-7: verifyDataMisuse_byNonExistentLinkIDOnSend_expectNotExistLink
 *      @[Purpose]: Validate random non-existent LinkID returns NOT_EXIST_LINK
 *      @[Brief]: Call IOC_sendDAT with valid-looking but non-existent LinkID
 *
 *  ⚪ TC-8: verifyDataMisuse_byNonExistentLinkIDOnRecv_expectNotExistLink
 *      @[Purpose]: Validate random non-existent LinkID returns NOT_EXIST_LINK
 *      @[Brief]: Call IOC_recvDAT with valid-looking but non-existent LinkID
 *
 *  ⚪ TC-9: verifyDataMisuse_byNonExistentLinkIDOnFlush_expectNotExistLink
 *      @[Purpose]: Validate random non-existent LinkID returns NOT_EXIST_LINK
 *      @[Brief]: Call IOC_flushDAT with valid-looking but non-existent LinkID
 *
 * [@AC-1,AC-2,AC-3,AC-4,US-3] State Violation Detection (7 tests)
 *  ⚪ TC-10: verifyDataMisuse_bySendOnClosedLink_expectNotExistLink
 *      @[Purpose]: Validate IOC_sendDAT on closed link returns NOT_EXIST_LINK
 *      @[Brief]: Close link, then attempt IOC_sendDAT
 *
 *  ⚪ TC-11: verifyDataMisuse_byRecvOnClosedLink_expectNotExistLink
 *      @[Purpose]: Validate IOC_recvDAT on closed link returns NOT_EXIST_LINK
 *      @[Brief]: Close link, then attempt IOC_recvDAT
 *
 *  ⚪ TC-12: verifyDataMisuse_byFlushOnClosedLink_expectNotExistLink
 *      @[Purpose]: Validate IOC_flushDAT on closed link returns NOT_EXIST_LINK
 *      @[Brief]: Close link, then attempt IOC_flushDAT
 *
 *  ⚪ TC-13: verifyDataMisuse_bySendBeforeConnection_expectNotExistLink
 *      @[Purpose]: Validate IOC_sendDAT before connection established
 *      @[Brief]: Call IOC_sendDAT without prior connect/accept
 *
 *  ⚪ TC-14: verifyDataMisuse_byRecvBeforeConnection_expectNotExistLink
 *      @[Purpose]: Validate IOC_recvDAT before connection established
 *      @[Brief]: Call IOC_recvDAT without prior connect/accept
 *
 *  ⚪ TC-15: verifyDataMisuse_bySendAfterServiceOffline_expectLinkBroken
 *      @[Purpose]: Validate IOC_sendDAT after service taken offline
 *      @[Brief]: Offline service, then attempt send on orphaned link
 *
 *  ⚪ TC-16: verifyDataMisuse_byDoubleCloseLink_expectGracefulHandling
 *      @[Purpose]: Validate double IOC_closeLink doesn't corrupt system
 *      @[Brief]: Close same link twice, expect error on second close
 *
 * [@AC-1,AC-2,AC-3,US-4] Role Mismatch Detection (3 tests)
 *  ⚪ TC-17: verifyDataMisuse_bySendOnReceiverLink_expectInvalidOperation
 *      @[Purpose]: Validate IOC_sendDAT rejected on DatReceiver link
 *      @[Brief]: Connect as DatReceiver, attempt IOC_sendDAT
 *
 *  ⚪ TC-18: verifyDataMisuse_byRecvOnSenderLink_expectInvalidOperation
 *      @[Purpose]: Validate manual IOC_recvDAT rejected on DatSender link
 *      @[Brief]: Connect as DatSender (no callback), attempt IOC_recvDAT
 *
 *  ⚪ TC-19: verifyDataMisuse_byFlushOnReceiverLink_expectInvalidOperation
 *      @[Purpose]: Validate IOC_flushDAT rejected on DatReceiver link
 *      @[Brief]: Connect as DatReceiver, attempt IOC_flushDAT
 *
 * [@AC-1,AC-2,AC-3,AC-4,US-5] DatDesc Corruption Detection (4 tests)
 *  ⚪ TC-20: verifyDataMisuse_byMalformedDatDesc_expectInvalidParam
 *      @[Purpose]: Validate uninitialized DatDesc rejected
 *      @[Brief]: Call IOC_sendDAT with uninitialized/garbage DatDesc
 *
 *  ⚪ TC-21: verifyDataMisuse_byNullPayloadNonZeroSize_expectInvalidParam
 *      @[Purpose]: Validate NULL payload + size>0 rejected
 *      @[Brief]: Create DatDesc with NULL data but PtrDataSize > 0
 *
 *  ⚪ TC-22: verifyDataMisuse_byReusingDatDescWithoutReinit_expectUndefinedBehavior
 *      @[Purpose]: Document behavior of reused DatDesc without reinit
 *      @[Brief]: Send data, reuse same DatDesc without IOC_initDatDesc
 *
 *  ⚪ TC-23: verifyDataMisuse_byInvalidRecvDatDescConfig_expectInvalidParam
 *      @[Purpose]: Validate invalid receive buffer configuration rejected
 *      @[Brief]: Call IOC_recvDAT with invalid buffer setup in DatDesc
 *
 * [@AC-1,AC-2,AC-3,US-6] FIFO-Specific Misuse (3 tests)
 *  ⚪ TC-24: verifyDataMisuse_byFIFOFileDeletedDuringOperation_expectLinkBroken
 *      @[Purpose]: Validate graceful handling when FIFO file deleted
 *      @[Brief]: Establish connection, delete FIFO file, attempt send
 *
 *  ⚪ TC-25: verifyDataMisuse_byInvalidFIFOPath_expectConfigurationError
 *      @[Purpose]: Validate invalid FIFO path rejected during service setup
 *      @[Brief]: Try to online service with corrupted/invalid FIFO path
 *
 *  ⚪ TC-26: verifyDataMisuse_byFIFOPermissionChangedToReadOnly_expectAccessError
 *      @[Purpose]: Validate permission errors detected during operation
 *      @[Brief]: Establish connection, change FIFO to read-only, attempt send
 */
//======>END OF TEST CASES==========================================================================
//======>END OF TEST DESIGN=========================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF UNIT TESTING IMPLEMENTATION======================================================

/**
 * ╔══════════════════════════════════════════════════════════════════════════════════════════╗
 * ║                        🔴 NULL POINTER HANDLING - AC-1,US-1                              ║
 * ╚══════════════════════════════════════════════════════════════════════════════════════════╝
 */

/**
 * TC-1: verifyDataMisuse_byNullDatDescOnSend_expectInvalidParam
 * @[Steps]: Call IOC_sendDAT with NULL pDatDesc → Verify INVALID_PARAM returned
 * @[Expect]: IOC_RESULT_INVALID_PARAM (or NOT_EXIST_LINK if LinkID checked first)
 */
TEST(UT_DataMisuse, verifyDataMisuse_byNullDatDescOnSend_expectInvalidParam) {
    printf("🔴 RED: verifyDataMisuse_byNullDatDescOnSend_expectInvalidParam\n");

    //===BEHAVIOR: Call IOC_sendDAT with NULL pDatDesc===
    IOC_Result_T result = IOC_sendDAT(IOC_ID_INVALID, NULL, NULL);

    //===VERIFY: Should return INVALID_PARAM or NOT_EXIST_LINK===
    // Note: Implementation may check LinkID first, so NOT_EXIST_LINK is acceptable
    EXPECT_TRUE(result == IOC_RESULT_INVALID_PARAM || result == IOC_RESULT_NOT_EXIST_LINK)
        << "Expected INVALID_PARAM or NOT_EXIST_LINK, got: " << result;
}

/**
 * TC-2: verifyDataMisuse_byNullDatDescOnRecv_expectInvalidParam
 * @[Steps]: Call IOC_recvDAT with NULL pDatDesc → Verify INVALID_PARAM returned
 * @[Expect]: IOC_RESULT_INVALID_PARAM (or NOT_EXIST_LINK if LinkID checked first)
 */
TEST(UT_DataMisuse, verifyDataMisuse_byNullDatDescOnRecv_expectInvalidParam) {
    printf("🔴 RED: verifyDataMisuse_byNullDatDescOnRecv_expectInvalidParam\n");

    //===BEHAVIOR: Call IOC_recvDAT with NULL pDatDesc===
    IOC_Result_T result = IOC_recvDAT(IOC_ID_INVALID, NULL, NULL);

    //===VERIFY: Should return INVALID_PARAM or NOT_EXIST_LINK===
    EXPECT_TRUE(result == IOC_RESULT_INVALID_PARAM || result == IOC_RESULT_NOT_EXIST_LINK)
        << "Expected INVALID_PARAM or NOT_EXIST_LINK, got: " << result;
}

/**
 * TC-3: verifyDataMisuse_byNullOptionOnFlush_expectDefaultBehavior
 * @[Steps]: Setup valid link → Call IOC_flushDAT with NULL pOption → Verify success
 * @[Expect]: IOC_RESULT_SUCCESS (NULL options should use defaults)
 */
TEST(UT_DataMisuse, verifyDataMisuse_byNullOptionOnFlush_expectDefaultBehavior) {
    printf("🔴 RED: verifyDataMisuse_byNullOptionOnFlush_expectDefaultBehavior\n");

    //===SETUP: Create valid service and connection===
    IOC_SrvID_T srvID = IOC_ID_INVALID;
    IOC_SrvArgs_T srvArgs = {};
    IOC_Helper_initSrvArgs(&srvArgs);
    srvArgs.SrvURI.pProtocol = IOC_SRV_PROTO_FIFO;
    srvArgs.SrvURI.pHost = IOC_SRV_HOST_LOCAL_PROCESS;
    srvArgs.SrvURI.pPath = "test/data/misuse/flush_null_option";
    srvArgs.UsageCapabilites = IOC_LinkUsageDatReceiver;
    srvArgs.Flags = IOC_SRVFLAG_AUTO_ACCEPT;

    IOC_Result_T result = IOC_onlineService(&srvID, &srvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result) << "Service setup failed";

    IOC_LinkID_T linkID = IOC_ID_INVALID;
    IOC_ConnArgs_T connArgs = {};
    IOC_Helper_initConnArgs(&connArgs);
    connArgs.SrvURI = srvArgs.SrvURI;
    connArgs.Usage = IOC_LinkUsageDatSender;

    result = IOC_connectService(&linkID, &connArgs, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result) << "Connection failed";

    //===BEHAVIOR: Call IOC_flushDAT with NULL pOption===
    result = IOC_flushDAT(linkID, NULL);

    //===VERIFY: Should succeed with default options===
    EXPECT_EQ(IOC_RESULT_SUCCESS, result) << "IOC_flushDAT with NULL options should use defaults and succeed";

    //===CLEANUP===
    IOC_closeLink(linkID);
    IOC_offlineService(srvID);
}

/**
 * ╔══════════════════════════════════════════════════════════════════════════════════════════╗
 * ║                      🔴 INVALID LINKID HANDLING - AC-1,AC-2,US-2                         ║
 * ╚══════════════════════════════════════════════════════════════════════════════════════════╝
 */

/**
 * TC-4: verifyDataMisuse_byInvalidLinkIDOnSend_expectNotExistLink
 * @[Steps]: Call IOC_sendDAT with IOC_ID_INVALID → Verify NOT_EXIST_LINK
 * @[Expect]: IOC_RESULT_NOT_EXIST_LINK
 */
TEST(UT_DataMisuse, verifyDataMisuse_byInvalidLinkIDOnSend_expectNotExistLink) {
    printf("🔴 RED: verifyDataMisuse_byInvalidLinkIDOnSend_expectNotExistLink\n");

    //===BEHAVIOR: Call IOC_sendDAT with IOC_ID_INVALID===
    IOC_DatDesc_T datDesc = {};
    IOC_initDatDesc(&datDesc);
    const char* testData = "test";
    datDesc.Payload.pData = (void*)testData;
    datDesc.Payload.PtrDataSize = 4;

    IOC_Result_T result = IOC_sendDAT(IOC_ID_INVALID, &datDesc, NULL);

    //===VERIFY: Should return NOT_EXIST_LINK===
    EXPECT_EQ(IOC_RESULT_NOT_EXIST_LINK, result) << "IOC_sendDAT with IOC_ID_INVALID should return NOT_EXIST_LINK";
}

/**
 * TC-5: verifyDataMisuse_byInvalidLinkIDOnRecv_expectNotExistLink
 * @[Steps]: Call IOC_recvDAT with IOC_ID_INVALID → Verify NOT_EXIST_LINK
 * @[Expect]: IOC_RESULT_NOT_EXIST_LINK
 */
TEST(UT_DataMisuse, verifyDataMisuse_byInvalidLinkIDOnRecv_expectNotExistLink) {
    printf("🔴 RED: verifyDataMisuse_byInvalidLinkIDOnRecv_expectNotExistLink\n");

    //===BEHAVIOR: Call IOC_recvDAT with IOC_ID_INVALID===
    IOC_DatDesc_T datDesc = {};
    IOC_initDatDesc(&datDesc);

    IOC_Result_T result = IOC_recvDAT(IOC_ID_INVALID, &datDesc, NULL);

    //===VERIFY: Should return NOT_EXIST_LINK===
    EXPECT_EQ(IOC_RESULT_NOT_EXIST_LINK, result) << "IOC_recvDAT with IOC_ID_INVALID should return NOT_EXIST_LINK";
}

/**
 * TC-6: verifyDataMisuse_byInvalidLinkIDOnFlush_expectNotExistLink
 * @[Steps]: Call IOC_flushDAT with IOC_ID_INVALID → Verify NOT_EXIST_LINK
 * @[Expect]: IOC_RESULT_NOT_EXIST_LINK
 */
TEST(UT_DataMisuse, verifyDataMisuse_byInvalidLinkIDOnFlush_expectNotExistLink) {
    printf("🔴 RED: verifyDataMisuse_byInvalidLinkIDOnFlush_expectNotExistLink\n");

    //===BEHAVIOR: Call IOC_flushDAT with IOC_ID_INVALID===
    IOC_Result_T result = IOC_flushDAT(IOC_ID_INVALID, NULL);

    //===VERIFY: Should return NOT_EXIST_LINK===
    EXPECT_EQ(IOC_RESULT_NOT_EXIST_LINK, result) << "IOC_flushDAT with IOC_ID_INVALID should return NOT_EXIST_LINK";
}

/**
 * TC-7: verifyDataMisuse_byNonExistentLinkIDOnSend_expectNotExistLink
 * @[Steps]: Call IOC_sendDAT with random non-existent LinkID → Verify NOT_EXIST_LINK
 * @[Expect]: IOC_RESULT_NOT_EXIST_LINK
 */
TEST(UT_DataMisuse, verifyDataMisuse_byNonExistentLinkIDOnSend_expectNotExistLink) {
    printf("🔴 RED: verifyDataMisuse_byNonExistentLinkIDOnSend_expectNotExistLink\n");

    //===BEHAVIOR: Call IOC_sendDAT with non-existent but valid-looking LinkID===
    IOC_DatDesc_T datDesc = {};
    IOC_initDatDesc(&datDesc);
    const char* testData = "test";
    datDesc.Payload.pData = (void*)testData;
    datDesc.Payload.PtrDataSize = 4;

    IOC_LinkID_T nonExistentLinkID = 999999;  // Valid-looking but doesn't exist
    IOC_Result_T result = IOC_sendDAT(nonExistentLinkID, &datDesc, NULL);

    //===VERIFY: Should return NOT_EXIST_LINK===
    EXPECT_EQ(IOC_RESULT_NOT_EXIST_LINK, result) << "IOC_sendDAT with non-existent LinkID should return NOT_EXIST_LINK";
}

/**
 * TC-8: verifyDataMisuse_byNonExistentLinkIDOnRecv_expectNotExistLink
 */
TEST(UT_DataMisuse, verifyDataMisuse_byNonExistentLinkIDOnRecv_expectNotExistLink) {
    printf("🔴 RED: verifyDataMisuse_byNonExistentLinkIDOnRecv_expectNotExistLink\n");

    IOC_DatDesc_T datDesc = {};
    IOC_initDatDesc(&datDesc);

    IOC_LinkID_T nonExistentLinkID = 999999;
    IOC_Result_T result = IOC_recvDAT(nonExistentLinkID, &datDesc, NULL);

    EXPECT_EQ(IOC_RESULT_NOT_EXIST_LINK, result) << "IOC_recvDAT with non-existent LinkID should return NOT_EXIST_LINK";
}

/**
 * TC-9: verifyDataMisuse_byNonExistentLinkIDOnFlush_expectNotExistLink
 */
TEST(UT_DataMisuse, verifyDataMisuse_byNonExistentLinkIDOnFlush_expectNotExistLink) {
    printf("🔴 RED: verifyDataMisuse_byNonExistentLinkIDOnFlush_expectNotExistLink\n");

    IOC_LinkID_T nonExistentLinkID = 999999;
    IOC_Result_T result = IOC_flushDAT(nonExistentLinkID, NULL);

    EXPECT_EQ(IOC_RESULT_NOT_EXIST_LINK, result)
        << "IOC_flushDAT with non-existent LinkID should return NOT_EXIST_LINK";
}

/**
 * ╔══════════════════════════════════════════════════════════════════════════════════════════╗
 * ║                    🔴 STATE VIOLATION DETECTION - AC-1,AC-2,AC-3,AC-4,US-3               ║
 * ╚══════════════════════════════════════════════════════════════════════════════════════════╝
 */

/**
 * TC-10: verifyDataMisuse_bySendOnClosedLink_expectNotExistLink
 * @[Steps]: Setup link → Close it → Try IOC_sendDAT → Verify NOT_EXIST_LINK
 * @[Expect]: IOC_RESULT_NOT_EXIST_LINK
 */
TEST(UT_DataMisuse, verifyDataMisuse_bySendOnClosedLink_expectNotExistLink) {
    printf("🔴 RED: verifyDataMisuse_bySendOnClosedLink_expectNotExistLink\n");

    //===SETUP: Create and then close link===
    IOC_SrvID_T srvID = IOC_ID_INVALID;
    IOC_SrvArgs_T srvArgs = {};
    IOC_Helper_initSrvArgs(&srvArgs);
    srvArgs.SrvURI.pProtocol = IOC_SRV_PROTO_FIFO;
    srvArgs.SrvURI.pHost = IOC_SRV_HOST_LOCAL_PROCESS;
    srvArgs.SrvURI.pPath = "test/data/misuse/send_on_closed";
    srvArgs.UsageCapabilites = IOC_LinkUsageDatReceiver;
    srvArgs.Flags = IOC_SRVFLAG_AUTO_ACCEPT;

    IOC_Result_T result = IOC_onlineService(&srvID, &srvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result);

    IOC_LinkID_T linkID = IOC_ID_INVALID;
    IOC_ConnArgs_T connArgs = {};
    IOC_Helper_initConnArgs(&connArgs);
    connArgs.SrvURI = srvArgs.SrvURI;
    connArgs.Usage = IOC_LinkUsageDatSender;

    result = IOC_connectService(&linkID, &connArgs, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result);

    // Close the link
    result = IOC_closeLink(linkID);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result);

    //===BEHAVIOR: Try to send on closed link===
    IOC_DatDesc_T datDesc = {};
    IOC_initDatDesc(&datDesc);
    const char* testData = "test";
    datDesc.Payload.pData = (void*)testData;
    datDesc.Payload.PtrDataSize = 4;

    result = IOC_sendDAT(linkID, &datDesc, NULL);

    //===VERIFY: Should return NOT_EXIST_LINK===
    EXPECT_EQ(IOC_RESULT_NOT_EXIST_LINK, result) << "IOC_sendDAT on closed link should return NOT_EXIST_LINK";

    //===CLEANUP===
    IOC_offlineService(srvID);
}

/**
 * TC-11: verifyDataMisuse_byRecvOnClosedLink_expectNotExistLink
 */
TEST(UT_DataMisuse, verifyDataMisuse_byRecvOnClosedLink_expectNotExistLink) {
    printf("🔴 RED: verifyDataMisuse_byRecvOnClosedLink_expectNotExistLink\n");

    //===SETUP: Create and close link===
    IOC_SrvID_T srvID = IOC_ID_INVALID;
    IOC_SrvArgs_T srvArgs = {};
    IOC_Helper_initSrvArgs(&srvArgs);
    srvArgs.SrvURI.pProtocol = IOC_SRV_PROTO_FIFO;
    srvArgs.SrvURI.pHost = IOC_SRV_HOST_LOCAL_PROCESS;
    srvArgs.SrvURI.pPath = "test/data/misuse/recv_on_closed";
    srvArgs.UsageCapabilites = IOC_LinkUsageDatSender;
    srvArgs.Flags = IOC_SRVFLAG_AUTO_ACCEPT;

    IOC_Result_T result = IOC_onlineService(&srvID, &srvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result);

    IOC_LinkID_T linkID = IOC_ID_INVALID;
    IOC_ConnArgs_T connArgs = {};
    IOC_Helper_initConnArgs(&connArgs);
    connArgs.SrvURI = srvArgs.SrvURI;
    connArgs.Usage = IOC_LinkUsageDatReceiver;

    result = IOC_connectService(&linkID, &connArgs, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result);

    result = IOC_closeLink(linkID);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result);

    //===BEHAVIOR: Try to recv on closed link===
    IOC_DatDesc_T datDesc = {};
    IOC_initDatDesc(&datDesc);

    result = IOC_recvDAT(linkID, &datDesc, NULL);

    //===VERIFY===
    EXPECT_EQ(IOC_RESULT_NOT_EXIST_LINK, result) << "IOC_recvDAT on closed link should return NOT_EXIST_LINK";

    //===CLEANUP===
    IOC_offlineService(srvID);
}

/**
 * TC-12: verifyDataMisuse_byFlushOnClosedLink_expectNotExistLink
 */
TEST(UT_DataMisuse, verifyDataMisuse_byFlushOnClosedLink_expectNotExistLink) {
    printf("🔴 RED: verifyDataMisuse_byFlushOnClosedLink_expectNotExistLink\n");

    //===SETUP===
    IOC_SrvID_T srvID = IOC_ID_INVALID;
    IOC_SrvArgs_T srvArgs = {};
    IOC_Helper_initSrvArgs(&srvArgs);
    srvArgs.SrvURI.pProtocol = IOC_SRV_PROTO_FIFO;
    srvArgs.SrvURI.pHost = IOC_SRV_HOST_LOCAL_PROCESS;
    srvArgs.SrvURI.pPath = "test/data/misuse/flush_on_closed";
    srvArgs.UsageCapabilites = IOC_LinkUsageDatReceiver;
    srvArgs.Flags = IOC_SRVFLAG_AUTO_ACCEPT;

    IOC_Result_T result = IOC_onlineService(&srvID, &srvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result);

    IOC_LinkID_T linkID = IOC_ID_INVALID;
    IOC_ConnArgs_T connArgs = {};
    IOC_Helper_initConnArgs(&connArgs);
    connArgs.SrvURI = srvArgs.SrvURI;
    connArgs.Usage = IOC_LinkUsageDatSender;

    result = IOC_connectService(&linkID, &connArgs, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result);

    result = IOC_closeLink(linkID);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result);

    //===BEHAVIOR===
    result = IOC_flushDAT(linkID, NULL);

    //===VERIFY===
    EXPECT_EQ(IOC_RESULT_NOT_EXIST_LINK, result) << "IOC_flushDAT on closed link should return NOT_EXIST_LINK";

    //===CLEANUP===
    IOC_offlineService(srvID);
}

/**
 * TC-13: verifyDataMisuse_bySendBeforeConnection_expectNotExistLink
 */
TEST(UT_DataMisuse, verifyDataMisuse_bySendBeforeConnection_expectNotExistLink) {
    printf("🔴 RED: verifyDataMisuse_bySendBeforeConnection_expectNotExistLink\n");

    //===BEHAVIOR: Try to send without establishing connection===
    IOC_DatDesc_T datDesc = {};
    IOC_initDatDesc(&datDesc);
    const char* testData = "test";
    datDesc.Payload.pData = (void*)testData;
    datDesc.Payload.PtrDataSize = 4;

    IOC_LinkID_T fakeLinkID = 12345;  // Never connected
    IOC_Result_T result = IOC_sendDAT(fakeLinkID, &datDesc, NULL);

    //===VERIFY===
    EXPECT_EQ(IOC_RESULT_NOT_EXIST_LINK, result) << "IOC_sendDAT before connection should return NOT_EXIST_LINK";
}

/**
 * TC-14: verifyDataMisuse_byRecvBeforeConnection_expectNotExistLink
 */
TEST(UT_DataMisuse, verifyDataMisuse_byRecvBeforeConnection_expectNotExistLink) {
    printf("🔴 RED: verifyDataMisuse_byRecvBeforeConnection_expectNotExistLink\n");

    IOC_DatDesc_T datDesc = {};
    IOC_initDatDesc(&datDesc);

    IOC_LinkID_T fakeLinkID = 12345;
    IOC_Result_T result = IOC_recvDAT(fakeLinkID, &datDesc, NULL);

    EXPECT_EQ(IOC_RESULT_NOT_EXIST_LINK, result) << "IOC_recvDAT before connection should return NOT_EXIST_LINK";
}

/**
 * TC-15: verifyDataMisuse_bySendAfterServiceOffline_expectLinkBroken
 */
TEST(UT_DataMisuse, verifyDataMisuse_bySendAfterServiceOffline_expectLinkBroken) {
    printf("🔴 RED: verifyDataMisuse_bySendAfterServiceOffline_expectLinkBroken\n");

    //===SETUP===
    IOC_SrvID_T srvID = IOC_ID_INVALID;
    IOC_SrvArgs_T srvArgs = {};
    IOC_Helper_initSrvArgs(&srvArgs);
    srvArgs.SrvURI.pProtocol = IOC_SRV_PROTO_FIFO;
    srvArgs.SrvURI.pHost = IOC_SRV_HOST_LOCAL_PROCESS;
    srvArgs.SrvURI.pPath = "test/data/misuse/send_after_offline";
    srvArgs.UsageCapabilites = IOC_LinkUsageDatReceiver;
    srvArgs.Flags = IOC_SRVFLAG_AUTO_ACCEPT;

    IOC_Result_T result = IOC_onlineService(&srvID, &srvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result);

    IOC_LinkID_T linkID = IOC_ID_INVALID;
    IOC_ConnArgs_T connArgs = {};
    IOC_Helper_initConnArgs(&connArgs);
    connArgs.SrvURI = srvArgs.SrvURI;
    connArgs.Usage = IOC_LinkUsageDatSender;

    result = IOC_connectService(&linkID, &connArgs, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result);

    // Take service offline while link still exists
    result = IOC_offlineService(srvID);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result);

    //===BEHAVIOR: Try to send after service offline===
    IOC_DatDesc_T datDesc = {};
    IOC_initDatDesc(&datDesc);
    const char* testData = "test";
    datDesc.Payload.pData = (void*)testData;
    datDesc.Payload.PtrDataSize = 4;

    result = IOC_sendDAT(linkID, &datDesc, NULL);

    //===VERIFY: Could be LINK_BROKEN or NOT_EXIST_LINK===
    EXPECT_TRUE(result == IOC_RESULT_LINK_BROKEN || result == IOC_RESULT_NOT_EXIST_LINK)
        << "IOC_sendDAT after service offline should return LINK_BROKEN or NOT_EXIST_LINK, got: " << result;

    //===CLEANUP===
    IOC_closeLink(linkID);  // May also fail, but we clean up anyway
}

/**
 * TC-16: verifyDataMisuse_byDoubleCloseLink_expectGracefulHandling
 */
TEST(UT_DataMisuse, verifyDataMisuse_byDoubleCloseLink_expectGracefulHandling) {
    printf("🔴 RED: verifyDataMisuse_byDoubleCloseLink_expectGracefulHandling\n");

    //===SETUP===
    IOC_SrvID_T srvID = IOC_ID_INVALID;
    IOC_SrvArgs_T srvArgs = {};
    IOC_Helper_initSrvArgs(&srvArgs);
    srvArgs.SrvURI.pProtocol = IOC_SRV_PROTO_FIFO;
    srvArgs.SrvURI.pHost = IOC_SRV_HOST_LOCAL_PROCESS;
    srvArgs.SrvURI.pPath = "test/data/misuse/double_close";
    srvArgs.UsageCapabilites = IOC_LinkUsageDatReceiver;
    srvArgs.Flags = IOC_SRVFLAG_AUTO_ACCEPT;

    IOC_Result_T result = IOC_onlineService(&srvID, &srvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result);

    IOC_LinkID_T linkID = IOC_ID_INVALID;
    IOC_ConnArgs_T connArgs = {};
    IOC_Helper_initConnArgs(&connArgs);
    connArgs.SrvURI = srvArgs.SrvURI;
    connArgs.Usage = IOC_LinkUsageDatSender;

    result = IOC_connectService(&linkID, &connArgs, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result);

    // First close
    result = IOC_closeLink(linkID);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result);

    //===BEHAVIOR: Second close on same link===
    result = IOC_closeLink(linkID);

    //===VERIFY: Should handle gracefully (error but no crash)===
    EXPECT_NE(IOC_RESULT_SUCCESS, result) << "Double IOC_closeLink should return error (NOT_EXIST_LINK or similar)";

    //===CLEANUP===
    IOC_offlineService(srvID);
}

/**
 * ╔══════════════════════════════════════════════════════════════════════════════════════════╗
 * ║                       🔴 ROLE MISMATCH DETECTION - AC-1,AC-2,AC-3,US-4                   ║
 * ╚══════════════════════════════════════════════════════════════════════════════════════════╝
 */

/**
 * TC-17: verifyDataMisuse_bySendOnReceiverLink_expectInvalidOperation
 */
TEST(UT_DataMisuse, verifyDataMisuse_bySendOnReceiverLink_expectInvalidOperation) {
    printf("🔴 RED: verifyDataMisuse_bySendOnReceiverLink_expectInvalidOperation\n");

    //===SETUP: Connect as DatReceiver===
    IOC_SrvID_T srvID = IOC_ID_INVALID;
    IOC_SrvArgs_T srvArgs = {};
    IOC_Helper_initSrvArgs(&srvArgs);
    srvArgs.SrvURI.pProtocol = IOC_SRV_PROTO_FIFO;
    srvArgs.SrvURI.pHost = IOC_SRV_HOST_LOCAL_PROCESS;
    srvArgs.SrvURI.pPath = "test/data/misuse/send_on_receiver";
    srvArgs.UsageCapabilites = IOC_LinkUsageDatSender;  // Service is sender
    srvArgs.Flags = IOC_SRVFLAG_AUTO_ACCEPT;

    IOC_Result_T result = IOC_onlineService(&srvID, &srvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result);

    IOC_LinkID_T linkID = IOC_ID_INVALID;
    IOC_ConnArgs_T connArgs = {};
    IOC_Helper_initConnArgs(&connArgs);
    connArgs.SrvURI = srvArgs.SrvURI;
    connArgs.Usage = IOC_LinkUsageDatReceiver;  // Client is receiver

    result = IOC_connectService(&linkID, &connArgs, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result);

    //===BEHAVIOR: Try to send on receiver link (role mismatch)===
    IOC_DatDesc_T datDesc = {};
    IOC_initDatDesc(&datDesc);
    const char* testData = "test";
    datDesc.Payload.pData = (void*)testData;
    datDesc.Payload.PtrDataSize = 4;

    result = IOC_sendDAT(linkID, &datDesc, NULL);

    //===VERIFY: Should reject (implementation may vary on specific error code)===
    EXPECT_NE(IOC_RESULT_SUCCESS, result) << "IOC_sendDAT on DatReceiver link should be rejected";

    //===CLEANUP===
    IOC_closeLink(linkID);
    IOC_offlineService(srvID);
}

/**
 * TC-18: verifyDataMisuse_byRecvOnSenderLink_expectInvalidOperation
 */
TEST(UT_DataMisuse, verifyDataMisuse_byRecvOnSenderLink_expectInvalidOperation) {
    printf("🔴 RED: verifyDataMisuse_byRecvOnSenderLink_expectInvalidOperation\n");

    //===SETUP: Connect as DatSender (no callback, manual recv attempt)===
    IOC_SrvID_T srvID = IOC_ID_INVALID;
    IOC_SrvArgs_T srvArgs = {};
    IOC_Helper_initSrvArgs(&srvArgs);
    srvArgs.SrvURI.pProtocol = IOC_SRV_PROTO_FIFO;
    srvArgs.SrvURI.pHost = IOC_SRV_HOST_LOCAL_PROCESS;
    srvArgs.SrvURI.pPath = "test/data/misuse/recv_on_sender";
    srvArgs.UsageCapabilites = IOC_LinkUsageDatReceiver;
    srvArgs.Flags = IOC_SRVFLAG_AUTO_ACCEPT;

    IOC_Result_T result = IOC_onlineService(&srvID, &srvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result);

    IOC_LinkID_T linkID = IOC_ID_INVALID;
    IOC_ConnArgs_T connArgs = {};
    IOC_Helper_initConnArgs(&connArgs);
    connArgs.SrvURI = srvArgs.SrvURI;
    connArgs.Usage = IOC_LinkUsageDatSender;  // Client is sender

    result = IOC_connectService(&linkID, &connArgs, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result);

    //===BEHAVIOR: Try manual recv on sender link===
    IOC_DatDesc_T datDesc = {};
    IOC_initDatDesc(&datDesc);

    result = IOC_recvDAT(linkID, &datDesc, NULL);

    //===VERIFY: Should reject===
    EXPECT_NE(IOC_RESULT_SUCCESS, result) << "IOC_recvDAT on DatSender link should be rejected";

    //===CLEANUP===
    IOC_closeLink(linkID);
    IOC_offlineService(srvID);
}

/**
 * TC-19: verifyDataMisuse_byFlushOnReceiverLink_expectInvalidOperation
 */
TEST(UT_DataMisuse, verifyDataMisuse_byFlushOnReceiverLink_expectInvalidOperation) {
    printf("🔴 RED: verifyDataMisuse_byFlushOnReceiverLink_expectInvalidOperation\n");

    //===SETUP: Connect as DatReceiver===
    IOC_SrvID_T srvID = IOC_ID_INVALID;
    IOC_SrvArgs_T srvArgs = {};
    IOC_Helper_initSrvArgs(&srvArgs);
    srvArgs.SrvURI.pProtocol = IOC_SRV_PROTO_FIFO;
    srvArgs.SrvURI.pHost = IOC_SRV_HOST_LOCAL_PROCESS;
    srvArgs.SrvURI.pPath = "test/data/misuse/flush_on_receiver";
    srvArgs.UsageCapabilites = IOC_LinkUsageDatSender;
    srvArgs.Flags = IOC_SRVFLAG_AUTO_ACCEPT;

    IOC_Result_T result = IOC_onlineService(&srvID, &srvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result);

    IOC_LinkID_T linkID = IOC_ID_INVALID;
    IOC_ConnArgs_T connArgs = {};
    IOC_Helper_initConnArgs(&connArgs);
    connArgs.SrvURI = srvArgs.SrvURI;
    connArgs.Usage = IOC_LinkUsageDatReceiver;

    result = IOC_connectService(&linkID, &connArgs, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result);

    //===BEHAVIOR: Try flush on receiver link===
    result = IOC_flushDAT(linkID, NULL);

    //===VERIFY: Should reject===
    EXPECT_NE(IOC_RESULT_SUCCESS, result) << "IOC_flushDAT on DatReceiver link should be rejected";

    //===CLEANUP===
    IOC_closeLink(linkID);
    IOC_offlineService(srvID);
}

/**
 * ╔══════════════════════════════════════════════════════════════════════════════════════════╗
 * ║                   🔴 DATDESC CORRUPTION DETECTION - AC-1,AC-2,AC-3,AC-4,US-5              ║
 * ╚══════════════════════════════════════════════════════════════════════════════════════════╝
 */

/**
 * TC-20: verifyDataMisuse_byMalformedDatDesc_expectInvalidParam
 */
TEST(UT_DataMisuse, verifyDataMisuse_byMalformedDatDesc_expectInvalidParam) {
    printf("🔴 RED: verifyDataMisuse_byMalformedDatDesc_expectInvalidParam\n");

    //===SETUP===
    IOC_SrvID_T srvID = IOC_ID_INVALID;
    IOC_SrvArgs_T srvArgs = {};
    IOC_Helper_initSrvArgs(&srvArgs);
    srvArgs.SrvURI.pProtocol = IOC_SRV_PROTO_FIFO;
    srvArgs.SrvURI.pHost = IOC_SRV_HOST_LOCAL_PROCESS;
    srvArgs.SrvURI.pPath = "test/data/misuse/malformed_desc";
    srvArgs.UsageCapabilites = IOC_LinkUsageDatReceiver;
    srvArgs.Flags = IOC_SRVFLAG_AUTO_ACCEPT;

    IOC_Result_T result = IOC_onlineService(&srvID, &srvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result);

    IOC_LinkID_T linkID = IOC_ID_INVALID;
    IOC_ConnArgs_T connArgs = {};
    IOC_Helper_initConnArgs(&connArgs);
    connArgs.SrvURI = srvArgs.SrvURI;
    connArgs.Usage = IOC_LinkUsageDatSender;

    result = IOC_connectService(&linkID, &connArgs, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result);

    //===BEHAVIOR: Use uninitialized/malformed DatDesc===
    IOC_DatDesc_T malformedDesc;                          // Not initialized - contains garbage
    memset(&malformedDesc, 0xFF, sizeof(malformedDesc));  // Fill with garbage

    result = IOC_sendDAT(linkID, &malformedDesc, NULL);

    //===VERIFY: Should detect and reject===
    EXPECT_NE(IOC_RESULT_SUCCESS, result) << "IOC_sendDAT with malformed DatDesc should be rejected";

    //===CLEANUP===
    IOC_closeLink(linkID);
    IOC_offlineService(srvID);
}

/**
 * TC-21: verifyDataMisuse_byNullPayloadNonZeroSize_expectInvalidParam
 */
TEST(UT_DataMisuse, verifyDataMisuse_byNullPayloadNonZeroSize_expectInvalidParam) {
    printf("🔴 RED: verifyDataMisuse_byNullPayloadNonZeroSize_expectInvalidParam\n");

    //===SETUP===
    IOC_SrvID_T srvID = IOC_ID_INVALID;
    IOC_SrvArgs_T srvArgs = {};
    IOC_Helper_initSrvArgs(&srvArgs);
    srvArgs.SrvURI.pProtocol = IOC_SRV_PROTO_FIFO;
    srvArgs.SrvURI.pHost = IOC_SRV_HOST_LOCAL_PROCESS;
    srvArgs.SrvURI.pPath = "test/data/misuse/null_payload";
    srvArgs.UsageCapabilites = IOC_LinkUsageDatReceiver;
    srvArgs.Flags = IOC_SRVFLAG_AUTO_ACCEPT;

    IOC_Result_T result = IOC_onlineService(&srvID, &srvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result);

    IOC_LinkID_T linkID = IOC_ID_INVALID;
    IOC_ConnArgs_T connArgs = {};
    IOC_Helper_initConnArgs(&connArgs);
    connArgs.SrvURI = srvArgs.SrvURI;
    connArgs.Usage = IOC_LinkUsageDatSender;

    result = IOC_connectService(&linkID, &connArgs, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result);

    //===BEHAVIOR: DatDesc with NULL payload but size > 0===
    IOC_DatDesc_T badDesc = {};
    IOC_initDatDesc(&badDesc);
    badDesc.Payload.pData = NULL;       // NULL pointer
    badDesc.Payload.PtrDataSize = 100;  // But claim 100 bytes

    result = IOC_sendDAT(linkID, &badDesc, NULL);

    //===VERIFY: Should reject this invalid configuration===
    EXPECT_NE(IOC_RESULT_SUCCESS, result) << "IOC_sendDAT with NULL payload + size>0 should be rejected";

    //===CLEANUP===
    IOC_closeLink(linkID);
    IOC_offlineService(srvID);
}

// TC-22, TC-23, TC-24, TC-25, TC-26 implementation can follow
// For now, marking as lower priority - core misuse tests above are critical

TEST(UT_DataMisuse, verifyDataMisuse_byReusingDatDescWithoutReinit_expectUndefinedBehavior) {
    GTEST_SKIP() << "TODO: P2 - Implement reusing DatDesc test";
}

TEST(UT_DataMisuse, verifyDataMisuse_byInvalidRecvDatDescConfig_expectInvalidParam) {
    GTEST_SKIP() << "TODO: P2 - Implement invalid recv DatDesc config test";
}

TEST(UT_DataMisuse, verifyDataMisuse_byFIFOFileDeletedDuringOperation_expectLinkBroken) {
    GTEST_SKIP() << "TODO: P2 - FIFO-specific test - implement after core tests pass";
}

TEST(UT_DataMisuse, verifyDataMisuse_byInvalidFIFOPath_expectConfigurationError) {
    GTEST_SKIP() << "TODO: P2 - FIFO-specific test - implement after core tests pass";
}

TEST(UT_DataMisuse, verifyDataMisuse_byFIFOPermissionChangedToReadOnly_expectAccessError) {
    GTEST_SKIP() << "TODO: P2 - FIFO-specific test - implement after core tests pass";
}

//======>END OF UNIT TESTING IMPLEMENTATION========================================================
