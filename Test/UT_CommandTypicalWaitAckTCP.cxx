///////////////////////////////////////////////////////////////////////////////////////////////////
// Command Typical WaitAck TCP (TCP protocol) — UT skeleton
//
// PURPOSE:
//   Verify TCP protocol layer integration with Polling Command patterns (Wait/Ack).
//   This test suite validates that IOC_waitCMD and IOC_ackCMD work correctly over network sockets,
//   enabling manual command detection and response control (vs automatic callbacks).
//   It covers both Server-as-Executor and Client-as-Executor roles.
//
// USAGE:
//   1. Validates TCP Polling (IOC_waitCMD) for both Server and Client roles.
//   2. Validates TCP Manual Ack (IOC_ackCMD) for delayed/async processing.
//   3. Validates TCP Timeout handling for polling operations.
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
#include <condition_variable>
#include <future>
#include <mutex>
#include <thread>
#include <vector>

#include "_UT_IOC_Common.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF OVERVIEW OF THIS UNIT TESTING FILE===============================================
/**
 * @brief
 *   [WHAT] This file verifies TCP-based Polling Command Execution (Wait/Ack)
 *   [WHERE] in the IOC Command API with TCP protocol layer (_IOC_SrvProtoTCP.c)
 *   [WHY] to ensure reliable manual command processing over network sockets.
 *
 * SCOPE:
 *   - [In scope]: TCP service using IOC_waitCMD (Polling)
 *   - [In scope]: TCP client using IOC_waitCMD (Polling)
 *   - [In scope]: TCP service/client using IOC_ackCMD (Manual Response)
 *   - [In scope]: TCP-specific concerns: blocking behavior, network timeouts
 *   - [Out of scope]: Callback-based execution (see UT_CommandTypicalTCP.cxx)
 *   - [Out of scope]: FIFO transport (see UT_CommandTypicalWaitAck.cxx)
 *
 * KEY CONCEPTS:
 *   - Polling: Executor thread blocks on IOC_waitCMD waiting for TCP data
 *   - Manual Ack: Executor explicitly sends response via IOC_ackCMD
 *   - Delayed Response: Executor can hold the command and ack later (async processing)
 *   - TCP Blocking: IOC_waitCMD behavior on TCP sockets (blocking vs non-blocking)
 *   - Role Symmetry: Both Server and Client can act as CmdExecutor (Polling) or CmdInitiator
 *
 * RELATIONSHIPS:
 *   - Depends on: IOC_SrvProtoTCP, IOC_CmdAPI
 *   - Related tests: UT_CommandTypicalTCP.cxx (Callback), UT_CommandTypicalWaitAck.cxx (FIFO)
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
 *   P1 🥇 FUNCTIONAL:     Must complete before P2 (ValidFunc + InvalidFunc)
 *   P2 🥈 DESIGN-ORIENTED: Test after P1 (State, Capability, Concurrency)
 *   P3 🥉 QUALITY-ORIENTED: Test for quality attributes (Performance, Robust, etc.)
 *
 * DEFAULT TEST ORDER:
 *   P1: Typical → Boundary → Misuse → Fault
 *   P2: State → Capability → Concurrency
 *   P3: Performance → Robust → Compatibility → Configuration
 *
 **************************************************************************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF USER STORY=======================================================================
/**
 * US-1: As a service developer, I want to implement the **Polling Command Pattern** (Wait+Ack)
 *       over TCP, so that I can process requests on my own thread/schedule without using callbacks,
 *       while maintaining the standard Request-Response semantics.
 *
 * US-2: As a service developer, I want to support **Delayed Acknowledgment** over TCP,
 *       so that I can perform long-running tasks asynchronously (unblocking the service thread)
 *       while the client remains blocked waiting for the response (SYNC semantics).
 *
 * US-3: As a system integrator, I want **Reliable Timeout Handling** for TCP polling,
 *       so that my service threads do not hang indefinitely when waiting for network commands,
 *       allowing for graceful shutdown or periodic maintenance tasks.
 *
 * US-4: As a system architect, I want **Symmetric Polling Support** (Client as Executor),
 *       so that clients can also serve as command processors (e.g., for server-initiated control)
 *       using the same Wait+Ack pattern over TCP.
 */
//======>END OF USER STORY==========================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//=======>BEGIN OF ACCEPTANCE CRITERIA==============================================================
/**
 * [@US-1] TCP Polling Command Pattern (Wait + Ack)
 *  AC-1: GIVEN a TCP service configured for Polling Mode (no CbExecCmd_F),
 *         WHEN a client sends a command via IOC_execCMD,
 *         THEN the service's call to IOC_waitCMD returns successfully with the command details.
 *  AC-2: GIVEN a command received via IOC_waitCMD,
 *         WHEN the service calls IOC_ackCMD with the result,
 *         THEN the client's IOC_execCMD returns successfully with the correct result data.
 *
 * [@US-2] TCP Delayed Acknowledgment (Async Processing)
 *  AC-1: GIVEN a TCP command received via IOC_waitCMD,
 *         WHEN the service delays the IOC_ackCMD call (simulating async work),
 *         THEN the client's IOC_execCMD remains blocked (waiting) during the delay.
 *  AC-2: GIVEN the delayed processing completes,
 *         WHEN the service finally calls IOC_ackCMD,
 *         THEN the client receives the response and unblocks immediately.
 *
 * [@US-3] TCP Polling Timeouts
 *  AC-1: GIVEN a TCP service calling IOC_waitCMD with a specific timeout (e.g., 100ms),
 *         WHEN no data arrives on the socket within that period,
 *         THEN IOC_waitCMD returns IOC_RESULT_TIMEOUT (or IOC_RESULT_WAIT_CMD_TIMEOUT).
 *  AC-2: GIVEN a timeout occurs,
 *         WHEN the service calls IOC_waitCMD again,
 *         THEN it can still receive subsequent commands (socket remains valid).
 *
 * [@US-4] Client as Polling Executor (Symmetric Role)
 *  AC-1: GIVEN a TCP client configured as CmdExecutor (Polling Mode),
 *         WHEN the server (CmdInitiator) sends a command via IOC_execCMD,
 *         THEN the client's call to IOC_waitCMD returns successfully with the command.
 *  AC-2: GIVEN the client processes the command,
 *         WHEN the client calls IOC_ackCMD,
 *         THEN the server's IOC_execCMD returns successfully with the result.
 */
//=======>END OF ACCEPTANCE CRITERIA================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF TEST CASES=======================================================================
/**************************************************************************************************
 * @brief 【TCP WaitAck Test Cases】
 *
 * ORGANIZATION STRATEGIES:
 *  - By Feature: Basic Polling, Delayed Ack, Timeout, Symmetric Role
 *  - By Protocol: TCP specific validation
 *
 * STATUS TRACKING: ⚪ = Planned/TODO，🔴 = Implemented/RED, 🟢 = Passed/GREEN, ⚠️ = Issues
 *
 * PORT ALLOCATION STRATEGY:
 *  - Range: 18200 - 18299
 *
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * 📋 [US-1] & [US-2]: TCP Polling & Manual Ack (Server as Executor)
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 *
 * [@AC-1,US-1] Basic TCP Polling Pattern
 *  🟢 TC-1: verifyTcpServicePolling_bySingleClient_expectWaitAckPattern
 *      @[Purpose]: Validate basic IOC_waitCMD + IOC_ackCMD pattern over TCP (Server Executor)
 *      @[Brief]: Service(TCP+Polling) → Client connects/sends PING → Service waits/acks PONG
 *      @[Protocol]: tcp://localhost:18200/WaitAckTCP_Basic
 *
 * [@AC-2,US-2] Delayed TCP Response
 *  🟢 TC-1: verifyTcpServiceAsyncProcessing_byDelayedAck_expectControlledTiming
 *      @[Purpose]: Validate that TCP connection stays open during delayed processing
 *      @[Brief]: Service receives command → Sleeps 500ms → Acks. Client waits and succeeds.
 *      @[Protocol]: tcp://localhost:18201/WaitAckTCP_Delayed
 *
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * 📋 [US-3]: TCP Polling Timeouts
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 *
 * [@AC-1,US-3] TCP Polling Timeout
 *  🟢 TC-1: verifyTcpServicePollingTimeout_byEmptyQueue_expectTimeoutHandling
 *      @[Purpose]: Validate IOC_waitCMD timeout behavior on TCP socket
 *      @[Brief]: Service polls with 100ms timeout → No data sent → Verify timeout return
 *      @[Protocol]: tcp://localhost:18202/WaitAckTCP_Timeout
 *
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * 📋 [US-4]: Symmetric Polling (Client as Executor)
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 *
 * [@AC-1,US-4] Client Polling Pattern
 *  ⚪ TC-1: verifyTcpClientPolling_byServerInitiator_expectWaitAckPattern
 *      @[Purpose]: Validate IOC_waitCMD on Client side (Client as Executor)
 *      @[Brief]: Server(Initiator) → Client(Executor+Polling) → Server sends CMD → Client waits/acks
 *      @[Protocol]: tcp://localhost:18203/WaitAckTCP_ClientExec
 *      @[Steps]:
 *          1. Start TCP service (CmdInitiator) on port 18203
 *          2. Client connects (CmdExecutor, Polling)
 *          3. Server accepts connection
 *          4. Client thread calls IOC_waitCMD
 *          5. Server sends CMD via IOC_execCMD
 *          6. Client receives CMD, calls IOC_ackCMD
 *          7. Server receives result
 */
//======>END OF TEST CASES=========================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF TODO/IMPLEMENTATION TRACKING SECTION=============================================
// 🔴 IMPLEMENTATION STATUS TRACKING
//
// STATUS LEGEND:
//   ⚪ TODO/PLANNED:      Designed but not implemented yet.
//   🔴 RED/FAILING:       Test written, but production code is missing or incorrect.
//   🟢 GREEN/PASSED:      Test written and passing.
//
// PRIORITY LEVELS:
//   P1 🥇 FUNCTIONAL:     Basic Polling (TC-1), Symmetric Polling (TC-4)
//   P2 🥈 TIMING:         Delayed Ack (TC-2)
//   P3 🥉 ROBUSTNESS:     Timeout (TC-3)
//
// TRACKING:
//   🟢 [@AC-1,US-1] TC-1: verifyTcpServicePolling_bySingleClient_expectWaitAckPattern
//   🟢 [@AC-2,US-2] TC-1: verifyTcpServiceAsyncProcessing_byDelayedAck_expectControlledTiming
//   🟢 [@AC-1,US-3] TC-1: verifyTcpServicePollingTimeout_byEmptyQueue_expectTimeoutHandling
//   ⚪ [@AC-1,US-4] TC-1: verifyTcpClientPolling_byServerInitiator_expectWaitAckPattern
//
//======>END OF TODO/IMPLEMENTATION TRACKING SECTION===============================================

///////////////////////////////////////////////////////////////////////////////////////////////////
// Helper: Command Definitions
///////////////////////////////////////////////////////////////////////////////////////////////////
#define TEST_CMDID_PING 100
#define TEST_CMDID_DELAY 101

///////////////////////////////////////////////////////////////////////////////////////////////////
// Test Case Implementation
///////////////////////////////////////////////////////////////////////////////////////////////////

// [@AC-1,US-1] TC-1: verifyTcpServicePolling_bySingleClient_expectWaitAckPattern
TEST(UT_CommandTypicalWaitAckTCP, verifyTcpServicePolling_bySingleClient_expectWaitAckPattern) {
    // 1. Setup Service (Polling Mode - No Callback)
    const uint16_t PORT = 18200;
    IOC_SrvURI_T srvURI = {
        .pProtocol = IOC_SRV_PROTO_TCP, .pHost = "0.0.0.0", .Port = PORT, .pPath = "WaitAckTCP_Basic"};

    IOC_SrvID_T srvID;
    IOC_SrvArgs_T srvArgs = {0};
    srvArgs.SrvURI = srvURI;
    srvArgs.UsageCapabilites = IOC_LinkUsageCmdExecutor;
    // Note: No UsageArgs.pCmd needed for polling mode (or minimal init)

    ASSERT_EQ(IOC_onlineService(&srvID, &srvArgs), IOC_RESULT_SUCCESS);

    // 2. Setup Client
    IOC_LinkID_T cliLinkID;
    IOC_ConnArgs_T connArgs = {0};
    connArgs.SrvURI = srvURI;  // Connect to same URI
    connArgs.Usage = IOC_LinkUsageCmdInitiator;

    ASSERT_EQ(IOC_connectService(&cliLinkID, &connArgs, NULL), IOC_RESULT_SUCCESS);

    // 3. Server Accepts Client (Manual Accept for control)
    IOC_LinkID_T srvLinkID;
    // Retry accept loop
    IOC_Result_T acceptRes = IOC_RESULT_BUG;
    for (int i = 0; i < 20; i++) {
        acceptRes = IOC_acceptClient(srvID, &srvLinkID, NULL);
        if (acceptRes == IOC_RESULT_SUCCESS) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    ASSERT_EQ(acceptRes, IOC_RESULT_SUCCESS);

    // 4. Client Sends Command (Async Thread to avoid blocking main test)
    std::future<IOC_Result_T> clientFuture = std::async(std::launch::async, [&]() {
        IOC_CmdDesc_T cmdDesc;
        IOC_CmdDesc_initVar(&cmdDesc);
        cmdDesc.CmdID = TEST_CMDID_PING;
        // Send PING
        return IOC_execCMD(cliLinkID, &cmdDesc, NULL);
    });

    // 5. Server Waits for Command
    IOC_CmdDesc_T recvCmd;
    IOC_CmdDesc_initVar(&recvCmd);

    // Wait with timeout to prevent hanging test
    IOC_Options_T waitOpt = {IOC_OPTID_TIMEOUT};
    waitOpt.Payload.TimeoutUS = 1000000;  // 1 second

    IOC_Result_T waitRes = IOC_waitCMD(srvLinkID, &recvCmd, &waitOpt);
    ASSERT_EQ(waitRes, IOC_RESULT_SUCCESS);
    ASSERT_EQ(recvCmd.CmdID, TEST_CMDID_PING);

    // 6. Server Acks Command
    recvCmd.Result = IOC_RESULT_SUCCESS;  // Set success result
    IOC_Result_T ackRes = IOC_ackCMD(srvLinkID, &recvCmd, NULL);
    ASSERT_EQ(ackRes, IOC_RESULT_SUCCESS);

    // 7. Verify Client Result
    ASSERT_EQ(clientFuture.get(), IOC_RESULT_SUCCESS);

    // Cleanup
    IOC_offlineService(srvID);
    // IOC_disconnectService(cliLinkID); // If API exists
}

// [@AC-1,US-2] TC-1: verifyTcpServiceAsyncProcessing_byDelayedAck_expectControlledTiming
TEST(UT_CommandTypicalWaitAckTCP, verifyTcpServiceAsyncProcessing_byDelayedAck_expectControlledTiming) {
    // 1. Setup Service
    const uint16_t PORT = 18201;
    IOC_SrvURI_T srvURI = {
        .pProtocol = IOC_SRV_PROTO_TCP, .pHost = "0.0.0.0", .Port = PORT, .pPath = "WaitAckTCP_Delayed"};

    IOC_SrvID_T srvID;
    IOC_SrvArgs_T srvArgs = {0};
    srvArgs.SrvURI = srvURI;
    srvArgs.UsageCapabilites = IOC_LinkUsageCmdExecutor;
    ASSERT_EQ(IOC_onlineService(&srvID, &srvArgs), IOC_RESULT_SUCCESS);

    // 2. Setup Client
    IOC_LinkID_T cliLinkID;
    IOC_ConnArgs_T connArgs = {0};
    connArgs.SrvURI = srvURI;
    connArgs.Usage = IOC_LinkUsageCmdInitiator;
    ASSERT_EQ(IOC_connectService(&cliLinkID, &connArgs, NULL), IOC_RESULT_SUCCESS);

    // 3. Server Accepts
    IOC_LinkID_T srvLinkID;
    IOC_Result_T acceptRes = IOC_RESULT_BUG;
    for (int i = 0; i < 20; i++) {
        acceptRes = IOC_acceptClient(srvID, &srvLinkID, NULL);
        if (acceptRes == IOC_RESULT_SUCCESS) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    ASSERT_EQ(acceptRes, IOC_RESULT_SUCCESS);

    // 4. Client Sends Command
    auto start = std::chrono::steady_clock::now();
    std::future<IOC_Result_T> clientFuture = std::async(std::launch::async, [&]() {
        IOC_CmdDesc_T cmdDesc;
        IOC_CmdDesc_initVar(&cmdDesc);
        cmdDesc.CmdID = TEST_CMDID_DELAY;
        return IOC_execCMD(cliLinkID, &cmdDesc, NULL);
    });

    // 5. Server Waits
    IOC_CmdDesc_T recvCmd;
    IOC_CmdDesc_initVar(&recvCmd);
    ASSERT_EQ(IOC_waitCMD(srvLinkID, &recvCmd, NULL), IOC_RESULT_SUCCESS);
    ASSERT_EQ(recvCmd.CmdID, TEST_CMDID_DELAY);

    // 6. Server Delays (Simulate Async Work)
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // 7. Server Acks
    recvCmd.Result = IOC_RESULT_SUCCESS;
    ASSERT_EQ(IOC_ackCMD(srvLinkID, &recvCmd, NULL), IOC_RESULT_SUCCESS);

    // 8. Verify Timing
    IOC_Result_T cliRes = clientFuture.get();
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    ASSERT_EQ(cliRes, IOC_RESULT_SUCCESS);
    EXPECT_GE(duration, 500) << "Client should have waited for at least the delay period";

    IOC_offlineService(srvID);
}

// [@AC-1,US-3] TC-1: verifyTcpServicePollingTimeout_byEmptyQueue_expectTimeoutHandling
TEST(UT_CommandTypicalWaitAckTCP, verifyTcpServicePollingTimeout_byEmptyQueue_expectTimeoutHandling) {
    // 1. Setup Service
    const uint16_t PORT = 18202;
    IOC_SrvURI_T srvURI = {
        .pProtocol = IOC_SRV_PROTO_TCP, .pHost = "0.0.0.0", .Port = PORT, .pPath = "WaitAckTCP_Timeout"};

    IOC_SrvID_T srvID;
    IOC_SrvArgs_T srvArgs = {0};
    srvArgs.SrvURI = srvURI;
    srvArgs.UsageCapabilites = IOC_LinkUsageCmdExecutor;
    ASSERT_EQ(IOC_onlineService(&srvID, &srvArgs), IOC_RESULT_SUCCESS);

    // 2. Setup Client (Connect but send nothing)
    IOC_LinkID_T cliLinkID;
    IOC_ConnArgs_T connArgs = {0};
    connArgs.SrvURI = srvURI;
    connArgs.Usage = IOC_LinkUsageCmdInitiator;
    ASSERT_EQ(IOC_connectService(&cliLinkID, &connArgs, NULL), IOC_RESULT_SUCCESS);

    // 3. Server Accepts
    IOC_LinkID_T srvLinkID;
    IOC_Result_T acceptRes = IOC_RESULT_BUG;
    for (int i = 0; i < 20; i++) {
        acceptRes = IOC_acceptClient(srvID, &srvLinkID, NULL);
        if (acceptRes == IOC_RESULT_SUCCESS) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    ASSERT_EQ(acceptRes, IOC_RESULT_SUCCESS);

    // 4. Server Waits with Timeout
    IOC_CmdDesc_T recvCmd;
    IOC_CmdDesc_initVar(&recvCmd);

    IOC_Options_T opt = {IOC_OPTID_TIMEOUT};
    opt.Payload.TimeoutUS = 100000;  // 100ms

    IOC_Result_T waitRes = IOC_waitCMD(srvLinkID, &recvCmd, &opt);
    ASSERT_EQ(waitRes, IOC_RESULT_TIMEOUT);

    IOC_offlineService(srvID);
}

// [@AC-1,US-4] TC-1: verifyTcpClientPolling_byServerInitiator_expectWaitAckPattern
TEST(UT_CommandTypicalWaitAckTCP, verifyTcpClientPolling_byServerInitiator_expectWaitAckPattern) {
    // 1. Setup Server (Initiator)
    const uint16_t PORT = 18203;
    IOC_SrvURI_T srvURI = {
        .pProtocol = IOC_SRV_PROTO_TCP, .pHost = "0.0.0.0", .Port = PORT, .pPath = "WaitAckTCP_ClientExec"};

    IOC_SrvID_T srvID;
    IOC_SrvArgs_T srvArgs = {0};
    srvArgs.SrvURI = srvURI;
    srvArgs.UsageCapabilites = IOC_LinkUsageCmdInitiator;  // Server initiates commands

    ASSERT_EQ(IOC_onlineService(&srvID, &srvArgs), IOC_RESULT_SUCCESS);

    // 2. Setup Client (Executor - Polling)
    IOC_LinkID_T cliLinkID;
    IOC_ConnArgs_T connArgs = {0};
    connArgs.SrvURI = srvURI;
    connArgs.Usage = IOC_LinkUsageCmdExecutor;  // Client executes commands

    ASSERT_EQ(IOC_connectService(&cliLinkID, &connArgs, NULL), IOC_RESULT_SUCCESS);

    // 3. Server Accepts
    IOC_LinkID_T srvLinkID;
    IOC_Result_T acceptRes = IOC_RESULT_BUG;
    for (int i = 0; i < 20; i++) {
        acceptRes = IOC_acceptClient(srvID, &srvLinkID, NULL);
        if (acceptRes == IOC_RESULT_SUCCESS) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    ASSERT_EQ(acceptRes, IOC_RESULT_SUCCESS);

    // 4. Client Polling Loop (Async)
    std::future<IOC_Result_T> clientFuture = std::async(std::launch::async, [&]() {
        IOC_CmdDesc_T recvCmd;
        IOC_CmdDesc_initVar(&recvCmd);

        // Wait for command from Server
        IOC_Result_T res = IOC_waitCMD(cliLinkID, &recvCmd, NULL);
        if (res != IOC_RESULT_SUCCESS) return res;

        if (recvCmd.CmdID != TEST_CMDID_PING) return IOC_RESULT_INVALID_PARAM;

        // Ack the command
        recvCmd.Result = IOC_RESULT_SUCCESS;
        return IOC_ackCMD(cliLinkID, &recvCmd, NULL);
    });

    // 5. Server Sends Command
    // Give client a moment to enter waitCMD (optional, but good for stability)
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    IOC_CmdDesc_T cmdDesc;
    IOC_CmdDesc_initVar(&cmdDesc);
    cmdDesc.CmdID = TEST_CMDID_PING;

    IOC_Result_T execRes = IOC_execCMD(srvLinkID, &cmdDesc, NULL);
    ASSERT_EQ(execRes, IOC_RESULT_SUCCESS);

    // 6. Verify Client Success
    ASSERT_EQ(clientFuture.get(), IOC_RESULT_SUCCESS);

    IOC_offlineService(srvID);
}