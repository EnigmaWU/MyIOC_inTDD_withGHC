///////////////////////////////////////////////////////////////////////////////////////////////////
// Command Fault TCP — Fault Testing for TCP Protocol
//
// PURPOSE:
//   Verify TCP command execution handles external faults and error conditions gracefully
//   to ensure robust recovery and proper error propagation.
//
// TDD WORKFLOW:
//   Design → Draft → Structure → Test (RED) → Code (GREEN) → Refactor → Repeat
//
// REFERENCE: LLM/CaTDD_DesignPrompt.md for full methodology
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>

#include "_UT_IOC_Common.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF OVERVIEW OF THIS UNIT TESTING FILE===============================================
/**
 * @brief
 *   [WHAT] This file verifies TCP command execution handles external faults and errors
 *   [WHERE] in the IOC Command API with TCP protocol layer (_IOC_SrvProtoTCP.c)
 *   [WHY] to ensure reliable operation under adverse conditions and network failures.
 *
 * SCOPE:
 *   - [In scope]: Network-level faults (connection loss, socket errors, timeouts)
 *   - [In scope]: Resource exhaustion (port conflicts, connection limits exceeded)
 *   - [In scope]: External failures (service offline, host unreachable)
 *   - [In scope]: Error recovery and graceful degradation
 *   - [Out of scope]: Valid inputs at boundaries (see UT_CommandBoundaryTCP.cxx)
 *   - [Out of scope]: API misuse (see UT_CommandMisuseTCP.cxx)
 *   - [Out of scope]: Correct operation (see UT_CommandTypicalTCP.cxx)
 *
 * KEY CONCEPTS:
 *   - Fault Testing: Test system behavior under external failure conditions
 *   - Network Faults: Socket closed, connection lost, network unreachable
 *   - Resource Faults: Port conflicts, connection limit exceeded, out of file descriptors
 *   - Timeout Faults: Network delay, slow response, no response
 *   - Recovery: Graceful degradation, error detection, resource cleanup
 *
 * FAULT CATEGORIES:
 *   1. Connection Faults: Socket closed unexpectedly, connection refused, peer reset
 *   2. Network Faults: Host unreachable, network timeout, packet loss
 *   3. Resource Faults: Port already in use, too many open files, connection queue full
 *   4. Timeout Faults: Command timeout, connect timeout, accept timeout
 *   5. Message Faults: Partial message, corrupted data, unexpected disconnect during transmission
 *
 * RELATIONSHIPS:
 *   - Complements: UT_CommandTypicalTCP.cxx (correct operation)
 *   - Complements: UT_CommandBoundaryTCP.cxx (boundary conditions)
 *   - Complements: UT_CommandMisuseTCP.cxx (API misuse)
 *   - Depends on: IOC Command API error handling, TCP protocol resilience
 *   - Production code: Source/_IOC_SrvProtoTCP.c, Source/IOC_Command.c
 */
//======>END OF OVERVIEW OF THIS UNIT TESTING FILE=================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF UNIT TESTING DESIGN==============================================================
/**************************************************************************************************
 * 📋 TEST CASE DESIGN ASPECTS/CATEGORIES
 *
 * DESIGN PRINCIPLE: IMPROVE VALUE • AVOID LOSS • BALANCE SKILL vs COST
 *
 * PRIORITY FRAMEWORK:
 *   P1 🥇 FUNCTIONAL:     ValidFunc(Typical + Boundary) + InvalidFunc(Misuse + Fault)
 *   P2 🥈 DESIGN-ORIENTED: State, Capability, Concurrency
 *   P3 🥉 QUALITY-ORIENTED: Performance, Robust, Compatibility, Configuration
 *
 * CONTEXT-SPECIFIC ADJUSTMENT:
 *   - File Focus: P1 Fault (InvalidFunc) - external fault scenarios
 *   - Rationale: Reliability under network faults is critical for production use
 *   - Risk: High impact (data loss, hung connections) if not handled properly
 *
 * RISK ASSESSMENT:
 *   US-1/AC-1/TC-1 (Connection failure): Impact=3, Likelihood=3, Uncertainty=1 → Score=9 (High)
 *   US-1/AC-2/TC-1 (Network timeout): Impact=3, Likelihood=3, Uncertainty=1 → Score=9 (High)
 *   US-2/AC-1/TC-1 (Port conflict): Impact=2, Likelihood=2, Uncertainty=1 → Score=4 (Medium)
 *   US-2/AC-2/TC-1 (Connection limit): Impact=2, Likelihood=2, Uncertainty=2 → Score=8 (Medium)
 *   US-3/AC-1/TC-1 (Service offline): Impact=2, Likelihood=2, Uncertainty=1 → Score=4 (Medium)
 *
 * COVERAGE STRATEGY: Fault Type × Detection Point × Recovery Action
 *
 * COVERAGE MATRIX (Systematic Test Planning):
 * ┌──────────────────────┬──────────────────┬───────────────────┬────────────────────────────┐
 * │ Fault Type           │ Detection Point  │ Recovery Action   │ Key Scenarios              │
 * ├──────────────────────┼──────────────────┼───────────────────┼────────────────────────────┤
 * │ Connection Loss      │ During exec      │ Return error      │ US-1: Socket closed        │
 * │ Network Timeout      │ During wait      │ Timeout + cleanup │ US-1: Slow/no response     │
 * │ Port Conflict        │ During online    │ Bind error        │ US-2: Port in use          │
 * │ Resource Exhaustion  │ During connect   │ Limit error       │ US-2: Too many connections │
 * │ Service Unavailable  │ During connect   │ Connect fail      │ US-3: Offline service      │
 * │ Message Corruption   │ During receive   │ Discard + error   │ US-3: Partial message      │
 * └──────────────────────┴──────────────────┴───────────────────┴────────────────────────────┘
 *
 * QUALITY GATE P1 (Fault):
 *   ✅ All connection fault tests detect and report errors properly
 *   ✅ All timeout tests complete within expected time bounds
 *   ✅ All resource fault tests return appropriate error codes
 *   ✅ No resource leaks on any fault path
 *   ✅ No crashes or undefined behavior on any fault
 */
///////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF USER STORY=======================================================================
/**
 * US-1: As a reliability engineer, I want TCP command execution to handle network faults
 *       so that connection failures and timeouts are detected and reported properly.
 *
 * US-2: As a system administrator, I want TCP services to handle resource exhaustion
 *       so that port conflicts and connection limits return clear errors.
 *
 * US-3: As a developer, I want TCP command execution to handle service unavailability
 *       so that offline services and message corruption are handled gracefully.
 */
//======>END OF USER STORY==========================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//=======>BEGIN OF ACCEPTANCE CRITERIA==============================================================
/**
 * [@US-1] Network Fault Handling
 *  AC-1: GIVEN a TCP command execution in progress,
 *         WHEN connection is lost (socket closed unexpectedly),
 *         THEN command execution returns error without hanging or crashing.
 *  AC-2: GIVEN a TCP command with timeout,
 *         WHEN network delay causes response to exceed timeout,
 *         THEN command returns IOC_RESULT_TIMEOUT within expected time.
 *  AC-3: GIVEN a TCP connection during command transmission,
 *         WHEN peer resets connection abruptly,
 *         THEN error is detected and resources cleaned up properly.
 *
 * [@US-2] Resource Exhaustion Handling
 *  AC-1: GIVEN a TCP service attempting to come online,
 *         WHEN port is already in use by another process,
 *         THEN onlineService returns IOC_RESULT_PORT_IN_USE error.
 *  AC-2: GIVEN a TCP service with connection limit,
 *         WHEN client count exceeds maximum connections,
 *         THEN new connections rejected with clear error.
 *  AC-3: GIVEN a system approaching file descriptor limit,
 *         WHEN new TCP connections attempted,
 *         THEN system returns resource exhaustion error gracefully.
 *
 * [@US-3] Service Unavailability Handling
 *  AC-1: GIVEN a client attempting to connect,
 *         WHEN TCP service is offline or unreachable,
 *         THEN connectService returns IOC_RESULT_CONNECTION_FAILED error.
 *  AC-2: GIVEN a TCP message reception,
 *         WHEN partial message received due to disconnect,
 *         THEN receiver detects incomplete message and discards it.
 *  AC-3: GIVEN a TCP service restart scenario,
 *         WHEN service goes offline and online again,
 *         THEN existing connections fail and new connections succeed.
 */
//=======>END OF ACCEPTANCE CRITERIA================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF TEST CASES=======================================================================
/**************************************************************************************************
 * @brief 【TCP Command Fault Test Cases】
 *
 * ORGANIZATION: By Fault Category (Network → Resource → Service)
 * STATUS TRACKING: ⚪ = Planned/TODO，🔴 = Implemented/RED, 🟢 = Passed/GREEN
 *
 * PORT ALLOCATION STRATEGY:
 *  - Base port: 21080 (different from Typical, Boundary, Misuse to avoid conflicts)
 *  - Range: 21080-21099 for fault tests
 *
 */

///////////////////////////////////////////////////////////////////////////////////////////////////
// Test Categories Below:
//
// ==================================================================================================
// 📋 [US-1]: Network Fault Handling
// ==================================================================================================
//
// [@AC-1,US-1] Connection failure during execution (MOVED from UT_CommandTypicalTCP.cxx)
//  🟢 TC-1: verifyTcpFaultConnection_byClosedSocket_expectGracefulError
//      @[Purpose]: Validate error handling when socket closes during command execution
//      @[Brief]: Close socket unexpectedly, attempt command, verify graceful failure
//      @[Protocol]: tcp://localhost:21080/CmdFaultTCP_ConnLoss
//      @[Status]: MOVED from UT_CommandTypicalTCP.cxx, IMPLEMENTED and GREEN
//      @[Steps]:
//          1. Online TCP service and establish client connection
//          2. Close server-side socket to simulate connection loss
//          3. Attempt command execution from client
//          4. Verify returns error (not IOC_RESULT_SUCCESS)
//          5. Verify no hang or crash
//          6. Cleanup
//
// [@AC-2,US-1] Network timeout (MOVED from UT_CommandTypicalTCP.cxx)
//  🟢 TC-1: verifyTcpFaultTimeout_bySlowResponse_expectTimeoutBehavior
//      @[Purpose]: Validate timeout behavior when response is slower than timeout
//      @[Brief]: Send DELAY command that exceeds timeout, verify timeout detection
//      @[Protocol]: tcp://localhost:21081/CmdFaultTCP_Timeout
//      @[Status]: MOVED from UT_CommandTypicalTCP.cxx, IMPLEMENTED and GREEN
//      @[Steps]:
//          1. Online TCP service with DELAY command support
//          2. Client connects
//          3. Send DELAY command with delay greater than timeout
//          4. Verify returns IOC_RESULT_TIMEOUT
//          5. Verify timeout occurs within expected time window
//          6. Cleanup
//
// [@AC-3,US-1] Peer reset during transmission
//  ⚪ TC-1: verifyTcpFaultReset_byPeerReset_expectErrorDetection
//      @[Purpose]: Validate detection of peer connection reset
//      @[Brief]: Simulate RST packet by closing socket with SO_LINGER, verify detection
//      @[Status]: TODO - Implement peer reset test
//
// ==================================================================================================
// 📋 [US-2]: Resource Exhaustion Handling
// ==================================================================================================
//
// [@AC-1,US-2] Port conflict
//  ⚪ TC-1: verifyTcpFaultResource_byPortConflict_expectPortInUseError
//      @[Purpose]: Validate port conflict detection
//      @[Brief]: Bring two services online on same port, expect second to fail
//      @[Status]: TODO - Implement port conflict test
//      @[Steps]:
//          1. Online first TCP service on port 21082
//          2. Attempt to online second service on same port 21082
//          3. Verify second online returns IOC_RESULT_PORT_IN_USE or similar
//          4. Verify first service unaffected
//          5. Cleanup both services
//
// [@AC-2,US-2] Connection limit exceeded
//  ⚪ TC-1: verifyTcpFaultResource_byMaxConnections_expectConnectionRejected
//      @[Purpose]: Validate behavior when connection limit exceeded
//      @[Brief]: Connect max+1 clients, verify last connection rejected
//      @[Status]: TODO - Implement connection limit test
//
// [@AC-3,US-2] File descriptor exhaustion
//  ⚪ TC-1: verifyTcpFaultResource_byFdExhaustion_expectResourceError
//      @[Purpose]: Validate behavior near file descriptor limits
//      @[Brief]: Open many file descriptors, attempt TCP connection, verify error
//      @[Status]: TODO - Implement FD exhaustion test (LOW priority, system-dependent)
//
// ==================================================================================================
// 📋 [US-3]: Service Unavailability Handling
// ==================================================================================================
//
// [@AC-1,US-3] Connect to offline service
//  ⚪ TC-1: verifyTcpFaultUnavailable_byOfflineService_expectConnectionFailed
//      @[Purpose]: Validate connection failure to non-existent service
//      @[Brief]: Attempt connect to port with no listener, verify error
//      @[Status]: TODO - Implement offline service test
//      @[Steps]:
//          1. Verify port 21083 has no listener
//          2. Attempt client connect to port 21083
//          3. Verify connectService returns IOC_RESULT_CONNECTION_FAILED
//          4. Verify no hang (connection timeout works)
//          5. Cleanup
//
// [@AC-2,US-3] Partial message reception
//  ⚪ TC-1: verifyTcpFaultMessage_byPartialMessage_expectDiscard
//      @[Purpose]: Validate handling of incomplete messages
//      @[Brief]: Send partial TCP message (only header), disconnect, verify detection
//      @[Status]: TODO - Implement partial message test (requires protocol inspection)
//
// [@AC-3,US-3] Service restart scenario
//  ⚪ TC-1: verifyTcpFaultRestart_byServiceRestart_expectProperTransition
//      @[Purpose]: Validate behavior during service restart
//      @[Brief]: Establish connection, offline service, bring back online, verify states
//      @[Status]: TODO - Implement service restart test
//
///////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF TEST HELPER FUNCTIONS============================================================

// Test base port for fault tests
#define _UT_FAULT_TCP_BASE_PORT 21080

// Command execution callback private data structure (copied from UT_CommandTypicalTCP.cxx)
typedef struct __CmdExecPriv {
    std::atomic<bool> CommandReceived{false};
    std::atomic<int> CommandCount{0};
    IOC_CmdID_T LastCmdID{0};
    IOC_CmdStatus_E LastStatus{IOC_CMD_STATUS_PENDING};
    IOC_Result_T LastResult{IOC_RESULT_BUG};
    char LastResponseData[512];
    ULONG_T LastResponseSize{0};
    std::mutex DataMutex;
    int ClientIndex{0};  // For multi-client scenarios
} __CmdExecPriv_T;

// Command execution callback function (service-side CmdExecutor)
static IOC_Result_T __CmdTcpFault_ExecutorCb(IOC_LinkID_T LinkID, IOC_CmdDesc_pT pCmdDesc, void *pCbPriv) {
    __CmdExecPriv_T *pPrivData = (__CmdExecPriv_T *)pCbPriv;
    if (!pPrivData || !pCmdDesc) return IOC_RESULT_INVALID_PARAM;

    std::lock_guard<std::mutex> lock(pPrivData->DataMutex);

    pPrivData->CommandReceived = true;
    pPrivData->CommandCount++;
    pPrivData->LastCmdID = IOC_CmdDesc_getCmdID(pCmdDesc);

    // Process different command types
    IOC_CmdID_T CmdID = IOC_CmdDesc_getCmdID(pCmdDesc);
    IOC_Result_T ExecResult = IOC_RESULT_SUCCESS;

    if (CmdID == IOC_CMDID_TEST_PING) {
        // PING command: simple response with "PONG"
        const char *response = "PONG";
        ExecResult = IOC_CmdDesc_setOutPayload(pCmdDesc, (void *)response, strlen(response));
        strcpy(pPrivData->LastResponseData, response);
        pPrivData->LastResponseSize = strlen(response);
    } else if (CmdID == IOC_CMDID_TEST_DELAY) {
        // DELAY command: simulate processing delay
        void *inputData = IOC_CmdDesc_getInData(pCmdDesc);
        if (IOC_CmdDesc_getInDataLen(pCmdDesc) == sizeof(int)) {
            int delayMs = *(int *)inputData;
            std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
            const char *response = "DELAY_COMPLETE";
            ExecResult = IOC_CmdDesc_setOutPayload(pCmdDesc, (void *)response, strlen(response));
            strcpy(pPrivData->LastResponseData, response);
            pPrivData->LastResponseSize = strlen(response);
        } else {
            ExecResult = IOC_RESULT_INVALID_PARAM;
        }
    } else {
        // Unsupported command type
        ExecResult = IOC_RESULT_NOT_SUPPORT;
    }

    pPrivData->LastResult = ExecResult;
    return ExecResult;
}

//======>END OF TEST HELPER FUNCTIONS==============================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF TEST IMPLEMENTATIONS=============================================================

// [@AC-1,US-1] TC-1: verifyTcpFaultConnection_byClosedSocket_expectGracefulError
// MOVED from UT_CommandTypicalTCP.cxx: verifyTcpConnectionFailure_byClosedSocket_expectGracefulError
TEST(UT_TcpCommandFault, verifyTcpFaultConnection_byClosedSocket_expectGracefulError) {
    //===SETUP===
    constexpr uint16_t TEST_PORT = _UT_FAULT_TCP_BASE_PORT;

    __CmdExecPriv_T srvExecPriv = {};

    IOC_SrvURI_T srvURI = {
        .pProtocol = IOC_SRV_PROTO_TCP, .pHost = "localhost", .Port = TEST_PORT, .pPath = "CmdFaultTCP_ConnLoss"};

    static IOC_CmdID_T supportedCmdIDs[] = {IOC_CMDID_TEST_PING};
    IOC_CmdUsageArgs_T cmdUsageArgs = {
        .CbExecCmd_F = __CmdTcpFault_ExecutorCb, .pCbPrivData = &srvExecPriv, .CmdNum = 1, .pCmdIDs = supportedCmdIDs};

    IOC_SrvArgs_T srvArgs = {.SrvURI = srvURI,
                             .Flags = IOC_SRVFLAG_NONE,
                             .UsageCapabilites = IOC_LinkUsageCmdExecutor,
                             .UsageArgs = {.pCmd = &cmdUsageArgs}};

    IOC_SrvID_T srvID = IOC_ID_INVALID;
    IOC_LinkID_T srvLinkID = IOC_ID_INVALID;
    IOC_LinkID_T cliLinkID = IOC_ID_INVALID;

    // Step 1: Online TCP service
    ASSERT_EQ(IOC_RESULT_SUCCESS, IOC_onlineService(&srvID, &srvArgs));
    ASSERT_NE(IOC_ID_INVALID, srvID);

    // Step 2: Client connects
    IOC_ConnArgs_T connArgs = {.SrvURI = srvURI, .Usage = IOC_LinkUsageCmdInitiator};
    std::thread cliThread([&] {
        ASSERT_EQ(IOC_RESULT_SUCCESS, IOC_connectService(&cliLinkID, &connArgs, NULL));
        ASSERT_NE(IOC_ID_INVALID, cliLinkID);
    });

    // Step 3: Service accepts
    ASSERT_EQ(IOC_RESULT_SUCCESS, IOC_acceptClient(srvID, &srvLinkID, NULL));
    ASSERT_NE(IOC_ID_INVALID, srvLinkID);

    cliThread.join();

    //===BEHAVIOR===
    printf("📋 [FAULT] Testing connection failure - closed socket\n");

    // Step 4: Close TCP socket unexpectedly (Simulate network failure)
    // We close the SERVER side link, which closes the socket.
    IOC_closeLink(srvLinkID);
    srvLinkID = IOC_ID_INVALID;

    // Allow some time for TCP FIN/RST to propagate
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Step 5: Attempt command execution from Client
    IOC_CmdDesc_T cmdDesc = {};
    cmdDesc.CmdID = IOC_CMDID_TEST_PING;
    cmdDesc.Status = IOC_CMD_STATUS_INITIALIZED;
    cmdDesc.TimeoutMs = 1000;

    IOC_Result_T result = IOC_execCMD(cliLinkID, &cmdDesc, NULL);

    //===VERIFY===
    EXPECT_NE(IOC_RESULT_SUCCESS, result) << "Command execution should fail on closed connection";
    printf("✅ [FAULT] Connection failure detected gracefully, result=%d\n", result);

    //===CLEANUP===
    IOC_CmdDesc_cleanup(&cmdDesc);
    if (cliLinkID != IOC_ID_INVALID) IOC_closeLink(cliLinkID);
    if (srvID != IOC_ID_INVALID) IOC_offlineService(srvID);
}

// [@AC-2,US-1] TC-1: verifyTcpFaultTimeout_bySlowResponse_expectTimeoutBehavior
// MOVED from UT_CommandTypicalTCP.cxx: verifyTcpNetworkTimeout_bySlowResponse_expectTimeoutBehavior
TEST(UT_TcpCommandFault, verifyTcpFaultTimeout_bySlowResponse_expectTimeoutBehavior) {
    //===SETUP===
    constexpr uint16_t TEST_PORT = _UT_FAULT_TCP_BASE_PORT + 1;

    __CmdExecPriv_T srvExecPriv = {};

    IOC_SrvURI_T srvURI = {
        .pProtocol = IOC_SRV_PROTO_TCP, .pHost = "localhost", .Port = TEST_PORT, .pPath = "CmdFaultTCP_Timeout"};

    static IOC_CmdID_T supportedCmdIDs[] = {IOC_CMDID_TEST_DELAY};
    IOC_CmdUsageArgs_T cmdUsageArgs = {
        .CbExecCmd_F = __CmdTcpFault_ExecutorCb, .pCbPrivData = &srvExecPriv, .CmdNum = 1, .pCmdIDs = supportedCmdIDs};

    IOC_SrvArgs_T srvArgs = {.SrvURI = srvURI,
                             .Flags = IOC_SRVFLAG_NONE,
                             .UsageCapabilites = IOC_LinkUsageCmdExecutor,
                             .UsageArgs = {.pCmd = &cmdUsageArgs}};

    IOC_SrvID_T srvID = IOC_ID_INVALID;
    IOC_LinkID_T srvLinkID = IOC_ID_INVALID;
    IOC_LinkID_T cliLinkID = IOC_ID_INVALID;

    // Step 1: Online TCP service
    ASSERT_EQ(IOC_RESULT_SUCCESS, IOC_onlineService(&srvID, &srvArgs));
    ASSERT_NE(IOC_ID_INVALID, srvID);

    // Step 2: Client connects
    IOC_ConnArgs_T connArgs = {.SrvURI = srvURI, .Usage = IOC_LinkUsageCmdInitiator};
    std::thread cliThread([&] {
        ASSERT_EQ(IOC_RESULT_SUCCESS, IOC_connectService(&cliLinkID, &connArgs, NULL));
        ASSERT_NE(IOC_ID_INVALID, cliLinkID);
    });

    // Step 3: Service accepts
    ASSERT_EQ(IOC_RESULT_SUCCESS, IOC_acceptClient(srvID, &srvLinkID, NULL));
    ASSERT_NE(IOC_ID_INVALID, srvLinkID);

    cliThread.join();

    //===BEHAVIOR===
    printf("📋 [FAULT] Testing network timeout - slow response\n");

    // Send DELAY command with delay exceeding timeout
    // Note: TCP protocol adds ~1000ms overhead to timeout.
    // We set timeout to 100ms, so effective timeout is ~1100ms.
    // We set delay to 2000ms to ensure timeout.
    int delayMs = 2000;
    IOC_CmdDesc_T cmdDesc = {};
    cmdDesc.CmdID = IOC_CMDID_TEST_DELAY;
    cmdDesc.Status = IOC_CMD_STATUS_INITIALIZED;
    cmdDesc.TimeoutMs = 100;
    IOC_CmdDesc_setInPayload(&cmdDesc, &delayMs, sizeof(delayMs));

    auto start = std::chrono::high_resolution_clock::now();
    IOC_Result_T result = IOC_execCMD(cliLinkID, &cmdDesc, NULL);
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    //===VERIFY===
    EXPECT_EQ(IOC_RESULT_TIMEOUT, result) << "Command should timeout due to slow response";
    EXPECT_GE(duration, 1100) << "Duration should reflect timeout + overhead";
    EXPECT_LT(duration, 2500) << "Duration should not exceed delay significantly";
    printf("✅ [FAULT] Timeout detected as expected, duration=%ldms, result=%d\n", duration, result);

    //===CLEANUP===
    IOC_CmdDesc_cleanup(&cmdDesc);
    if (cliLinkID != IOC_ID_INVALID) IOC_closeLink(cliLinkID);
    if (srvLinkID != IOC_ID_INVALID) IOC_closeLink(srvLinkID);
    if (srvID != IOC_ID_INVALID) IOC_offlineService(srvID);
}

//======>END OF TEST IMPLEMENTATIONS===============================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF TODO/IMPLEMENTATION TRACKING SECTION=============================================
/**
 * 🔴 IMPLEMENTATION STATUS TRACKING - Fault Testing (P1 InvalidFunc)
 *
 * STATUS LEGEND:
 *   ⚪ TODO/PLANNED:      Designed but not implemented
 *   🔴 RED/IMPLEMENTED:   Test written and failing (need prod code)
 *   🟢 GREEN/PASSED:      Test written and passing
 *
 * PRIORITY LEVELS:
 *   🥇 HIGH:    Critical faults (connection loss, timeouts causing hangs/data loss)
 *   🥈 MEDIUM:  Important faults (resource exhaustion, service unavailability)
 *   🥉 LOW:     Edge case faults (rare scenarios, system-dependent issues)
 *
 *=================================================================================================
 * 🥇 HIGH PRIORITY – Critical Network Faults
 *=================================================================================================
 *   🟢 [@AC-1,US-1] TC-1: verifyTcpFaultConnection_byClosedSocket_expectGracefulError
 *        - Status: MOVED from UT_CommandTypicalTCP.cxx and IMPLEMENTED
 *        - Tests: Connection loss during command execution
 *        - Result: Graceful error handling validated
 *
 *   🟢 [@AC-2,US-1] TC-1: verifyTcpFaultTimeout_bySlowResponse_expectTimeoutBehavior
 *        - Status: MOVED from UT_CommandTypicalTCP.cxx and IMPLEMENTED
 *        - Tests: Network timeout detection and recovery
 *        - Result: Timeout mechanism working correctly
 *
 *   ⚪ [@AC-3,US-1] TC-1: verifyTcpFaultReset_byPeerReset_expectErrorDetection
 *
 *=================================================================================================
 * 🥈 MEDIUM PRIORITY – Resource Exhaustion and Service Faults
 *=================================================================================================
 *   ⚪ [@AC-1,US-2] TC-1: verifyTcpFaultResource_byPortConflict_expectPortInUseError
 *   ⚪ [@AC-2,US-2] TC-1: verifyTcpFaultResource_byMaxConnections_expectConnectionRejected
 *   ⚪ [@AC-1,US-3] TC-1: verifyTcpFaultUnavailable_byOfflineService_expectConnectionFailed
 *   ⚪ [@AC-3,US-3] TC-1: verifyTcpFaultRestart_byServiceRestart_expectProperTransition
 *
 *=================================================================================================
 * 🥉 LOW PRIORITY – Edge Case Faults
 *=================================================================================================
 *   ⚪ [@AC-3,US-2] TC-1: verifyTcpFaultResource_byFdExhaustion_expectResourceError
 *   ⚪ [@AC-2,US-3] TC-1: verifyTcpFaultMessage_byPartialMessage_expectDiscard
 *
 *=================================================================================================
 * 📊 SUMMARY
 *=================================================================================================
 *   TOTAL: 9 test cases designed
 *   IMPLEMENTED: 2/9 (22% - both HIGH priority tests GREEN)
 *   HIGH PRIORITY: 3 tests (2 GREEN, 1 TODO)
 *   MEDIUM PRIORITY: 4 tests (all TODO)
 *   LOW PRIORITY: 2 tests (all TODO)
 *
 *   MOVED FROM UT_CommandTypicalTCP.cxx:
 *   - [@AC-2,US-3] TC-1: verifyTcpConnectionFailure_byClosedSocket_expectGracefulError
 *   - [@AC-3,US-3] TC-1: verifyTcpNetworkTimeout_bySlowResponse_expectTimeoutBehavior
 *
 *   NEXT STEPS:
 *   1. Implement remaining HIGH priority peer reset test
 *   2. Implement MEDIUM priority resource exhaustion tests
 *   3. Implement LOW priority edge case tests
 *   4. Follow TDD: Write test (RED) → Implement fix (GREEN) → Refactor → Repeat
 */
///////////////////////////////////////////////////////////////////////////////////////////////////
//======>END OF TODO/IMPLEMENTATION TRACKING SECTION===============================================

// END OF FILE
