///////////////////////////////////////////////////////////////////////////////////////////////////
// Command Typical TCP (TCP protocol) — UT skeleton
//
// PURPOSE:
//   Verify TCP protocol layer integration with command execution patterns.
//   This test suite validates that IOC command APIs work correctly over network sockets
//   with the same semantics as FIFO (in-memory) transport but with TCP-specific considerations.
//
// TDD WORKFLOW:
//   Design → Draft → Structure → Test (RED) → Code (GREEN) → Refactor → Repeat
//
// REFERENCE: LLM/CaTDD_DesignPrompt.md for full methodology
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <chrono>
#include <thread>

#include "_UT_IOC_Common.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF OVERVIEW OF THIS UNIT TESTING FILE===============================================
/**
 * @brief
 *   [WHAT] This file verifies TCP-based connection-oriented command execution (Conet)
 *   [WHERE] in the IOC Command API with TCP protocol layer (_IOC_SrvProtoTCP.c)
 *   [WHY] to ensure reliable P2P command request-response patterns over network sockets.
 *
 * SCOPE:
 *   - [In scope]: TCP protocol command execution, socket lifecycle, network transport validation
 *   - [In scope]: Same command patterns as FIFO but over TCP (IOC_SRV_PROTO_TCP)
 *   - [In scope]: TCP-specific concerns: port binding, connection failures, network timing
 *   - [Out of scope]: Broadcast commands (see UT_ServiceBroadcast.cxx)
 *   - [Out of scope]: FIFO/memory-based transport (see UT_CommandTypical.cxx)
 *   - [Out of scope]: Auto-accept patterns (see UT_CommandTypicalAutoAccept.cxx)
 *   - [Out of scope]: Cross-process/multi-machine testing (integration test scope)
 *
 * KEY CONCEPTS:
 *   - TCP Protocol Layer: Socket-based transport (bind, listen, accept, connect)
 *   - Protocol Abstraction: Same IOC APIs (IOC_execCMD), different transport layer
 *   - Message Framing: TCPMessageHeader_T + IOC_CmdDesc_T protocol structure
 *   - Background Receiver: pthread-based async message handling thread
 *   - Service Roles: CmdExecutor (processes commands) vs CmdInitiator (sends commands)
 *   - Port Management: Each test uses unique port to avoid conflicts (base: 18080)
 *
 * KEY DIFFERENCES FROM UT_CommandTypical.cxx (FIFO):
 *   - Protocol: IOC_SRV_PROTO_TCP vs IOC_SRV_PROTO_FIFO
 *   - Transport: Network sockets vs in-process memory queues
 *   - URI format: tcp://localhost:18080/service vs fifo://local-process/service
 *   - Timing: Network latency vs immediate in-memory
 *   - Lifecycle: Socket connect/bind/listen vs direct FIFO connection
 *   - Concurrency: Background receiver thread required for TCP
 *
 * RELATIONSHIPS:
 *   - Depends on: IOC Command API (IOC_execCMD, IOC_acceptClient, IOC_connectService)
 *   - Depends on: TCP protocol layer implementation (_IOC_SrvProtoTCP.c)
 *   - Related tests: UT_CommandTypical.cxx (FIFO-based reference patterns)
 *   - Related tests: UT_CommandTypicalAutoAccept.cxx (auto-accept extension patterns)
 *   - Production code: Source/_IOC_SrvProtoTCP.c, Source/IOC_Command.c
 */
//======>END OF OVERVIEW OF THIS UNIT TESTING FILE=================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF UNIT TESTING DESIGN==============================================================
/**
 * Design focus:
 *  - TCP protocol layer validation with command execution patterns
 *  - Socket-based command transport vs memory-based FIFO transport
 *  - Network-specific scenarios: port binding, connection management, timeouts
 *  - Same command API patterns as UT_CommandTypical.cxx but over TCP
 *  - TCP receiver thread functionality and command message framing
 *
 * Test progression:
 *  - Basic TCP command execution (CmdExecutor with callback)
 *  - Multiple command types over TCP (PING, ECHO, CALC)
 *  - Multi-client TCP connections with command isolation
 *  - TCP command timeouts and timing constraints
 *  - Reversed roles: service as CmdInitiator over TCP
 *  - TCP-specific error scenarios: port conflicts, connection failures
 *
 * TCP Protocol Specifics:
 *  - Port management: Using different ports for different tests to avoid conflicts
 *  - Connection lifecycle: TCP socket connect/accept vs FIFO direct connection
 *  - Message framing: TCPMessageHeader_T + IOC_CmdDesc_T protocol
 *  - Background receiver: pthread-based receiver thread for async message handling
 */
///////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF USER STORY=======================================================================
/**
 * US-1: As a service developer, I want TCP-based command executor capability
 *       so that clients can send commands over network sockets with reliable transport.
 *
 * US-2: As a service developer, I want TCP-based command initiator capability
 *       so that service can send commands to remote clients over network.
 *
 * US-3: As a system integrator, I want TCP command execution to handle network-specific concerns
 *       so that command flows work reliably over socket transport layer.
 *
 * US-4: As a developer, I want TCP protocol to support same command patterns as FIFO
 *       so that I can switch protocols without changing application logic.
 */
//======>END OF USER STORY==========================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//=======>BEGIN OF ACCEPTANCE CRITERIA==============================================================
/**
 * [@US-1] TCP-Based Command Executor Service
 *  AC-1: GIVEN a service with IOC_SRV_PROTO_TCP and CmdExecutor capability,
 *         WHEN client connects via TCP and sends command,
 *         THEN service callback processes command over socket and returns result.
 *  AC-2: GIVEN TCP service supporting multiple command types,
 *         WHEN client sends different commands over same TCP connection,
 *         THEN each command executes correctly with proper message framing.
 *  AC-3: GIVEN multiple clients connected to TCP service,
 *         WHEN clients send commands over separate TCP sockets,
 *         THEN each command is processed independently without socket interference.
 *  AC-4: GIVEN TCP command with timeout constraints,
 *         WHEN command execution takes expected time over socket,
 *         THEN command completes within timeout considering network latency.
 *
 * [@US-2] TCP-Based Command Initiator Service
 *  AC-1: GIVEN a TCP service as CmdInitiator and client as CmdExecutor,
 *         WHEN service sends command over TCP to client,
 *         THEN client callback processes command and service receives result over socket.
 *  AC-2: GIVEN TCP service orchestrating multiple clients,
 *         WHEN service sends different commands to different TCP clients,
 *         THEN each client executes its command over independent TCP connections.
 *
 * [@US-3] TCP Network-Specific Scenarios
 *  AC-1: GIVEN TCP service binding to specific port,
 *         WHEN service comes online,
 *         THEN TCP socket binds successfully and listens on configured port.
 *  AC-2: GIVEN TCP service with active connections,
 *         WHEN connection is closed or fails,
 *         THEN error handling works correctly without affecting other connections.
 *  AC-3: GIVEN TCP command with network timeout,
 *         WHEN network delay occurs,
 *         THEN timeout mechanisms work correctly for TCP transport.
 *
 * [@US-4] Protocol Layer Abstraction
 *  AC-1: GIVEN same command execution code,
 *         WHEN protocol changes from FIFO to TCP (or vice versa),
 *         THEN command patterns work identically at application level.
 *  AC-2: GIVEN command API usage patterns,
 *         WHEN using TCP protocol vs FIFO protocol,
 *         THEN only SrvURI protocol field differs, behavior remains consistent.
 */
//=======>END OF ACCEPTANCE CRITERIA================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF TEST CASES=======================================================================
/**************************************************************************************************
 * @brief 【TCP Command Test Cases】
 *
 * ORGANIZATION STRATEGIES:
 *  - By Protocol Layer: TCP-specific validation vs API-level behavior
 *  - By Test Category: Typical → Network-Specific → Error → Performance
 *  - By Coverage Matrix: Same command patterns as FIFO but over TCP
 *  - By Priority: Basic TCP commands first, complex scenarios second
 *
 * STATUS TRACKING: ⚪ = Planned/TODO，🔴 = Implemented/RED, 🟢 = Passed/GREEN, ⚠️ = Issues
 *
 * ✅ FRAMEWORK STATUS: Command APIs (IOC_execCMD) work with protocol delegation.
 *    TCP protocol layer (_IOC_SrvProtoTCP.c) implemented with socket-based transport.
 *
 * PORT ALLOCATION STRATEGY:
 *  - Use different ports for each test to avoid conflicts
 *  - Base port: 18080 (18080, 18081, 18082, ...)
 *  - Tests run sequentially by default (Google Test), parallel execution needs port isolation
 *
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * 📋 [US-1]: TCP Service as CmdExecutor (Client→Server Command Patterns)
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 *
 * [@AC-1,US-1] Basic TCP command execution with callback processing
 *  ⚪ TC-1: verifyTcpServiceAsCmdExecutor_bySingleClient_expectSynchronousResponse
 *      @[Purpose]: Validate fundamental TCP CmdExecutor→callback execution from client initiator
 *      @[Brief]: TCP service accepts client over socket, processes PING command via callback,
 *                returns PONG response over TCP connection
 *      @[Protocol]: tcp://localhost:18080/CmdTypicalTCP_SingleClient
 *      @[Status]: TODO - Implement TCP-based PING command execution
 *      @[Steps]:
 *          1. Online TCP service (CmdExecutor, port 18080) with PING command support
 *          2. Client connects via TCP to service
 *          3. Service accepts TCP connection
 *          4. Client sends PING command over TCP socket
 *          5. Service callback processes PING, returns PONG
 *          6. Verify PONG response received by client over TCP
 *          7. Cleanup TCP connections and service
 *
 * [@AC-2,US-1] Multiple command type support over TCP
 *  ⚪ TC-1: verifyTcpServiceAsCmdExecutor_byMultipleCommandTypes_expectProperExecution
 *      @[Purpose]: Ensure TCP service can handle different command types with message framing
 *      @[Brief]: Tests PING (no payload), ECHO (text), CALC (numeric) commands over TCP
 *      @[Protocol]: tcp://localhost:18081/CmdTypicalTCP_MultiTypes
 *      @[Status]: TODO - Implement multi-type command execution over TCP
 *      @[Steps]:
 *          1. Online TCP service (port 18081) supporting PING, ECHO, CALC commands
 *          2. Client connects and accepts TCP link
 *          3. Send PING command, verify PONG response
 *          4. Send ECHO command with text payload, verify echo response
 *          5. Send CALC command with numeric payload, verify calculation result
 *          6. Verify all commands execute correctly with proper TCP message framing
 *          7. Cleanup
 *
 * [@AC-3,US-1] Multi-client TCP isolation and concurrent command processing
 *  ⚪ TC-1: verifyTcpServiceAsCmdExecutor_byMultipleClients_expectIsolatedExecution
 *      @[Purpose]: Validate command isolation between multiple TCP clients without socket interference
 *      @[Brief]: 3 clients connect via separate TCP sockets, send unique ECHO commands concurrently,
 *                verify response isolation across TCP connections
 *      @[Protocol]: tcp://localhost:18082/CmdTypicalTCP_MultiClient
 *      @[Status]: TODO - Implement multi-client TCP command isolation
 *      @[Steps]:
 *          1. Online TCP service (port 18082) with ECHO command support
 *          2. Start 3 client threads, each connects via TCP
 *          3. Service accepts 3 TCP connections
 *          4. Each client sends unique ECHO command over its TCP socket concurrently
 *          5. Verify each client receives correct response without cross-talk
 *          6. Verify total command count = 3
 *          7. Cleanup all TCP connections
 *
 * [@AC-4,US-1] TCP command timeout and timing constraint validation
 *  ⚪ TC-1: verifyTcpServiceAsCmdExecutor_byTimeoutConstraints_expectProperTiming
 *      @[Purpose]: Validate command timeout behavior over TCP transport
 *      @[Brief]: Test DELAY command with timeouts over TCP, verify completion and timeout scenarios
 *      @[Protocol]: tcp://localhost:18083/CmdTypicalTCP_Timeout
 *      @[Status]: TODO - Implement TCP command timeout validation
 *      @[Steps]:
 *          1. Online TCP service (port 18083) with DELAY command support
 *          2. Client connects via TCP
 *          3. Send DELAY command with short delay (< timeout)
 *          4. Verify command completes successfully over TCP
 *          5. Send DELAY command with long delay (> timeout)
 *          6. Verify timeout behavior over TCP socket
 *          7. Cleanup
 *
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * 📋 [US-2]: TCP Service as CmdInitiator (Server→Client Command Patterns)
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 *
 * [@AC-1,US-2] Reversed TCP command flow from service to client
 *  ⚪ TC-1: verifyTcpServiceAsCmdInitiator_bySingleClient_expectClientExecution
 *      @[Purpose]: Validate reversed command flow from service to client over TCP
 *      @[Brief]: TCP service sends PING to client over socket, client processes via callback,
 *                service gets PONG response over TCP
 *      @[Protocol]: tcp://localhost:18084/CmdTypicalTCP_Reversed
 *      @[Status]: TODO - Implement reversed TCP command flow
 *      @[Steps]:
 *          1. Online TCP service (CmdInitiator, port 18084)
 *          2. Client connects with CmdExecutor usage
 *          3. Service accepts TCP connection
 *          4. Service sends PING command to client over TCP
 *          5. Client callback processes PING, returns PONG
 *          6. Service receives PONG response over TCP socket
 *          7. Cleanup
 *
 * [@AC-2,US-2] TCP service orchestrating multiple client operations
 *  ⚪ TC-1: verifyTcpServiceAsCmdInitiator_byMultipleClients_expectOrchestration
 *      @[Purpose]: Validate service orchestrating commands across multiple TCP clients
 *      @[Brief]: Service sends different commands to different clients over separate TCP sockets
 *      @[Protocol]: tcp://localhost:18085/CmdTypicalTCP_Orchestrate
 *      @[Status]: TODO - Implement TCP multi-client orchestration
 *      @[Steps]:
 *          1. Online TCP service (CmdInitiator, port 18085)
 *          2. Multiple clients connect with CmdExecutor usage
 *          3. Service accepts multiple TCP connections
 *          4. Service sends different commands to different TCP clients
 *          5. Each client processes command independently over its TCP socket
 *          6. Service collects results from all TCP connections
 *          7. Cleanup
 *
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * 📋 [US-3]: TCP Network-Specific Scenarios
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 *
 * [@AC-1,US-3] TCP service port binding validation
 *  ⚪ TC-1: verifyTcpServicePortBinding_byOnlineService_expectSuccessfulBind
 *      @[Purpose]: Validate TCP service successfully binds to configured port
 *      @[Brief]: Online TCP service, verify socket binds and listens on specified port
 *      @[Protocol]: tcp://localhost:18086/CmdTypicalTCP_PortBind
 *      @[Status]: TODO - Implement TCP port binding validation
 *      @[Steps]:
 *          1. Verify port 18086 is available
 *          2. Online TCP service on port 18086
 *          3. Verify service listening on port (netstat/lsof check or connect attempt)
 *          4. Offline service
 *          5. Verify port is released
 *
 * [@AC-2,US-3] TCP connection failure handling
 *  ⚪ TC-1: verifyTcpConnectionFailure_byClosedSocket_expectGracefulError
 *      @[Purpose]: Validate error handling when TCP connection fails or closes
 *      @[Brief]: Test command execution when TCP socket closes unexpectedly
 *      @[Protocol]: tcp://localhost:18087/CmdTypicalTCP_ConnFail
 *      @[Status]: TODO - Implement TCP connection failure scenarios
 *      @[Steps]:
 *          1. Online TCP service (port 18087)
 *          2. Client connects
 *          3. Service accepts connection
 *          4. Close TCP socket prematurely (simulated network failure)
 *          5. Attempt command execution
 *          6. Verify graceful error handling (IOC_RESULT_BUG or appropriate error)
 *          7. Verify other connections unaffected
 *          8. Cleanup
 *
 * [@AC-3,US-3] TCP network timeout scenarios
 *  ⚪ TC-1: verifyTcpNetworkTimeout_bySlowResponse_expectTimeoutBehavior
 *      @[Purpose]: Validate timeout mechanisms work correctly over TCP transport
 *      @[Brief]: Test command timeout with simulated network delay
 *      @[Protocol]: tcp://localhost:18088/CmdTypicalTCP_NetTimeout
 *      @[Status]: TODO - Implement TCP network timeout scenarios
 *      @[Steps]:
 *          1. Online TCP service (port 18088)
 *          2. Client connects
 *          3. Send command with short timeout
 *          4. Simulate delay in command processing (> timeout)
 *          5. Verify timeout error occurs
 *          6. Verify socket state remains valid for subsequent commands
 *          7. Cleanup
 *
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * 📋 [US-4]: Protocol Layer Abstraction Validation
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 *
 * [@AC-1,US-4] Same code, different protocol behavior
 *  ⚪ TC-1: verifyProtocolAbstraction_byTcpVsFifo_expectIdenticalBehavior
 *      @[Purpose]: Validate command patterns work identically at API level for TCP vs FIFO
 *      @[Brief]: Run same command sequence with TCP vs FIFO, verify identical results
 *      @[Protocol]: tcp://localhost:18089/AbstractionTest vs fifo://local-process/AbstractionTest
 *      @[Status]: TODO - Implement protocol abstraction comparison
 *      @[Steps]:
 *          1. Define common command test sequence (PING, ECHO, CALC)
 *          2. Run sequence with TCP service (port 18089)
 *          3. Run same sequence with FIFO service
 *          4. Verify identical API-level behavior and results
 *          5. Document differences (if any) at protocol level only
 *          6. Cleanup
 *
 * [@AC-2,US-4] Protocol URI field as only difference
 *  ⚪ TC-1: verifyProtocolUri_byDifferentProtocols_expectOnlyUriDifference
 *      @[Purpose]: Validate only SrvURI.pProtocol differs between TCP and FIFO usage
 *      @[Brief]: Compare service setup code for TCP vs FIFO, verify minimal differences
 *      @[Protocol]: N/A (code inspection validation)
 *      @[Status]: TODO - Verify code-level protocol abstraction
 *      @[Steps]:
 *          1. Create service setup helper accepting protocol as parameter
 *          2. Test with IOC_SRV_PROTO_TCP
 *          3. Test with IOC_SRV_PROTO_FIFO
 *          4. Verify rest of code identical (CmdUsageArgs, callbacks, etc.)
 *          5. Document protocol-agnostic patterns
 *
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * 📋 Additional TCP-Specific Considerations (Future Extensions)
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 *
 * [FUTURE] TCP Port Conflict Scenarios
 *  - Test port already in use (IOC_onlineService should fail gracefully)
 *  - Test port permission issues (< 1024 without root)
 *  - Test dynamic port allocation (port = 0 scenario)
 *
 * [FUTURE] TCP Message Framing Validation
 *  - Test TCPMessageHeader_T integrity over network
 *  - Test large payload scenarios with TCP socket buffering
 *  - Test fragmented message scenarios
 *
 * [FUTURE] TCP Background Receiver Thread
 *  - Test receiver thread lifecycle (startup, running, shutdown)
 *  - Test thread-safe command response handling
 *  - Test concurrent sends/receives over same TCP socket
 *
 * [FUTURE] Cross-Process TCP Testing
 *  - Create separate client/server executables
 *  - Test real network communication (not localhost)
 *  - Test with actual network latency and unreliability
 *
 * [FUTURE] TCP Security Considerations
 *  - Test with authentication mechanisms (if added)
 *  - Test with TLS/SSL encryption (if added)
 *  - Test with firewall rules affecting localhost
 */
//======>END OF TEST CASES=========================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF TEST HELPER FUNCTIONS============================================================

// Command execution callback private data structure (same as UT_CommandTypical.cxx)
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
static IOC_Result_T __CmdTcpTypical_ExecutorCb(IOC_LinkID_T LinkID, IOC_CmdDesc_pT pCmdDesc, void *pCbPriv) {
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
    } else if (CmdID == IOC_CMDID_TEST_ECHO) {
        // ECHO command: echo back input payload
        void *inputData = IOC_CmdDesc_getInData(pCmdDesc);
        ULONG_T inputSize = IOC_CmdDesc_getInDataSize(pCmdDesc);
        ExecResult = IOC_CmdDesc_setOutPayload(pCmdDesc, inputData, inputSize);
        memcpy(pPrivData->LastResponseData, inputData, inputSize);
        pPrivData->LastResponseSize = inputSize;
    } else if (CmdID == IOC_CMDID_TEST_CALC) {
        // CALC command: perform calculation (input + 1)
        void *inputData = IOC_CmdDesc_getInData(pCmdDesc);
        if (IOC_CmdDesc_getInDataSize(pCmdDesc) == sizeof(int)) {
            int inputValue = *(int *)inputData;
            int result = inputValue + 1;
            ExecResult = IOC_CmdDesc_setOutPayload(pCmdDesc, &result, sizeof(result));
            memcpy(pPrivData->LastResponseData, &result, sizeof(result));
            pPrivData->LastResponseSize = sizeof(result);
        } else {
            ExecResult = IOC_RESULT_INVALID_PARAM;
        }
    } else if (CmdID == IOC_CMDID_TEST_DELAY) {
        // DELAY command: simulate processing delay
        void *inputData = IOC_CmdDesc_getInData(pCmdDesc);
        if (IOC_CmdDesc_getInDataSize(pCmdDesc) == sizeof(int)) {
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

// [@AC-1,US-1] TC-1: verifyTcpServiceAsCmdExecutor_bySingleClient_expectSynchronousResponse
TEST(UT_TcpCommandTypical, verifyTcpServiceAsCmdExecutor_bySingleClient_expectSynchronousResponse) {
    // ═══════════════════════════════════════════════════════════════════════════════════
    // ARRANGE: Setup TCP service as CmdExecutor
    // ═══════════════════════════════════════════════════════════════════════════════════
    constexpr uint16_t TEST_PORT = 18080;

    __CmdExecPriv_T srvExecPriv = {};

    IOC_SrvURI_T srvURI = {
        .pProtocol = IOC_SRV_PROTO_TCP, .pHost = "localhost", .Port = TEST_PORT, .pPath = "CmdTypicalTCP_SingleClient"};

    static IOC_CmdID_T supportedCmdIDs[] = {IOC_CMDID_TEST_PING};
    IOC_CmdUsageArgs_T cmdUsageArgs = {.CbExecCmd_F = __CmdTcpTypical_ExecutorCb,
                                       .pCbPrivData = &srvExecPriv,
                                       .CmdNum = 1,
                                       .pCmdIDs = supportedCmdIDs};

    IOC_SrvArgs_T srvArgs = {.SrvURI = srvURI,
                             .Flags = IOC_SRVFLAG_NONE,
                             .UsageCapabilites = IOC_LinkUsageCmdExecutor,
                             .UsageArgs = {.pCmd = &cmdUsageArgs}};

    IOC_SrvID_T srvID = IOC_ID_INVALID;
    IOC_LinkID_T srvLinkID = IOC_ID_INVALID;
    IOC_LinkID_T cliLinkID = IOC_ID_INVALID;

    // ═══════════════════════════════════════════════════════════════════════════════════
    // ACT: Establish TCP connection and execute command
    // ═══════════════════════════════════════════════════════════════════════════════════

    // Step 1: Online TCP service
    ASSERT_EQ(IOC_RESULT_SUCCESS, IOC_onlineService(&srvID, &srvArgs));
    ASSERT_NE(IOC_ID_INVALID, srvID);

    // Step 2: Client connects via TCP (in separate thread to avoid blocking)
    IOC_ConnArgs_T connArgs = {.SrvURI = srvURI, .Usage = IOC_LinkUsageCmdInitiator};
    std::thread cliThread([&] {
        ASSERT_EQ(IOC_RESULT_SUCCESS, IOC_connectService(&cliLinkID, &connArgs, NULL));
        ASSERT_NE(IOC_ID_INVALID, cliLinkID);
    });

    // Step 3: Service accepts TCP connection
    ASSERT_EQ(IOC_RESULT_SUCCESS, IOC_acceptClient(srvID, &srvLinkID, NULL));
    ASSERT_NE(IOC_ID_INVALID, srvLinkID);

    cliThread.join();  // Wait for client connection to complete

    // Step 4: Execute PING command over TCP
    IOC_CmdDesc_T cmdDesc = {};  // Zero-initialize all fields
    cmdDesc.CmdID = IOC_CMDID_TEST_PING;
    cmdDesc.Status = IOC_CMD_STATUS_INITIALIZED;
    cmdDesc.TimeoutMs = 5000;  // 5 second timeout for network transport

    ASSERT_EQ(IOC_RESULT_SUCCESS, IOC_execCMD(cliLinkID, &cmdDesc, NULL));

    // ═══════════════════════════════════════════════════════════════════════════════════
    // ASSERT: Verify command execution and response
    // ═══════════════════════════════════════════════════════════════════════════════════

    // Verify server-side command execution
    ASSERT_TRUE(srvExecPriv.CommandReceived.load()) << "Server should have received command";
    ASSERT_EQ(1, srvExecPriv.CommandCount.load()) << "Server should have processed 1 command";
    ASSERT_EQ(IOC_CMDID_TEST_PING, srvExecPriv.LastCmdID);
    ASSERT_EQ(IOC_RESULT_SUCCESS, srvExecPriv.LastResult);

    // Verify client-side response data
    void *responseData = IOC_CmdDesc_getOutData(&cmdDesc);
    ULONG_T responseLen = IOC_CmdDesc_getOutDataLen(&cmdDesc);

    ASSERT_NE(nullptr, responseData) << "Response data should not be null";
    ASSERT_EQ(4, responseLen) << "PONG response should be 4 bytes";
    ASSERT_STREQ("PONG", static_cast<char *>(responseData));

    // ═══════════════════════════════════════════════════════════════════════════════════
    // CLEANUP: Release resources
    // ═══════════════════════════════════════════════════════════════════════════════════
    IOC_CmdDesc_cleanup(&cmdDesc);  // Free payload memory before CmdDesc goes out of scope

    if (cliLinkID != IOC_ID_INVALID) IOC_closeLink(cliLinkID);
    if (srvLinkID != IOC_ID_INVALID) IOC_closeLink(srvLinkID);
    if (srvID != IOC_ID_INVALID) IOC_offlineService(srvID);
}

// [@AC-2,US-1] TC-1: verifyTcpServiceAsCmdExecutor_byMultipleCommandTypes_expectProperExecution
TEST(UT_TcpCommandTypical, verifyTcpServiceAsCmdExecutor_byMultipleCommandTypes_expectProperExecution) {
    // TODO: Implement multiple command types over TCP
    // 1. Online TCP service on port 18081
    // 2. Test PING, ECHO, CALC commands sequentially
    // 3. Verify all command types work correctly over TCP
    // 4. Cleanup
    GTEST_SKIP() << "TCP multi-type commands not yet implemented - skeleton only";
}

// [@AC-3,US-1] TC-1: verifyTcpServiceAsCmdExecutor_byMultipleClients_expectIsolatedExecution
TEST(UT_TcpCommandTypical, verifyTcpServiceAsCmdExecutor_byMultipleClients_expectIsolatedExecution) {
    // TODO: Implement multi-client TCP command isolation
    // 1. Online TCP service on port 18082
    // 2. 3 clients connect concurrently
    // 3. Each sends unique ECHO command
    // 4. Verify response isolation
    // 5. Cleanup
    GTEST_SKIP() << "TCP multi-client commands not yet implemented - skeleton only";
}

// [@AC-4,US-1] TC-1: verifyTcpServiceAsCmdExecutor_byTimeoutConstraints_expectProperTiming
TEST(UT_TcpCommandTypical, verifyTcpServiceAsCmdExecutor_byTimeoutConstraints_expectProperTiming) {
    // TODO: Implement TCP command timeout validation
    // 1. Online TCP service on port 18083
    // 2. Test DELAY command with various timeouts
    // 3. Verify timeout behavior over TCP
    // 4. Cleanup
    GTEST_SKIP() << "TCP command timeout not yet implemented - skeleton only";
}

// [@AC-1,US-2] TC-1: verifyTcpServiceAsCmdInitiator_bySingleClient_expectClientExecution
TEST(UT_TcpCommandTypical, verifyTcpServiceAsCmdInitiator_bySingleClient_expectClientExecution) {
    // TODO: Implement reversed TCP command flow
    // 1. Online TCP service (CmdInitiator) on port 18084
    // 2. Client connects with CmdExecutor usage
    // 3. Service sends PING to client
    // 4. Verify PONG response received by service
    // 5. Cleanup
    GTEST_SKIP() << "TCP reversed command flow not yet implemented - skeleton only";
}

// [@AC-2,US-2] TC-1: verifyTcpServiceAsCmdInitiator_byMultipleClients_expectOrchestration
TEST(UT_TcpCommandTypical, verifyTcpServiceAsCmdInitiator_byMultipleClients_expectOrchestration) {
    // TODO: Implement TCP multi-client orchestration
    // 1. Online TCP service (CmdInitiator) on port 18085
    // 2. Multiple clients connect
    // 3. Service sends different commands to different clients
    // 4. Verify orchestration works correctly
    // 5. Cleanup
    GTEST_SKIP() << "TCP multi-client orchestration not yet implemented - skeleton only";
}

// [@AC-1,US-3] TC-1: verifyTcpServicePortBinding_byOnlineService_expectSuccessfulBind
TEST(UT_TcpCommandTypical, verifyTcpServicePortBinding_byOnlineService_expectSuccessfulBind) {
    // TODO: Implement TCP port binding validation
    // 1. Verify port 18086 available
    // 2. Online service on port 18086
    // 3. Verify listening
    // 4. Offline service
    // 5. Verify port released
    GTEST_SKIP() << "TCP port binding validation not yet implemented - skeleton only";
}

// [@AC-2,US-3] TC-1: verifyTcpConnectionFailure_byClosedSocket_expectGracefulError
TEST(UT_TcpCommandTypical, verifyTcpConnectionFailure_byClosedSocket_expectGracefulError) {
    // TODO: Implement TCP connection failure scenarios
    // 1. Setup TCP service and client
    // 2. Close socket unexpectedly
    // 3. Verify graceful error handling
    // 4. Cleanup
    GTEST_SKIP() << "TCP connection failure handling not yet implemented - skeleton only";
}

// [@AC-3,US-3] TC-1: verifyTcpNetworkTimeout_bySlowResponse_expectTimeoutBehavior
TEST(UT_TcpCommandTypical, verifyTcpNetworkTimeout_bySlowResponse_expectTimeoutBehavior) {
    // TODO: Implement TCP network timeout scenarios
    // 1. Setup TCP service on port 18088
    // 2. Send command with short timeout
    // 3. Simulate network delay
    // 4. Verify timeout behavior
    // 5. Cleanup
    GTEST_SKIP() << "TCP network timeout not yet implemented - skeleton only";
}

// [@AC-1,US-4] TC-1: verifyProtocolAbstraction_byTcpVsFifo_expectIdenticalBehavior
TEST(UT_TcpCommandTypical, verifyProtocolAbstraction_byTcpVsFifo_expectIdenticalBehavior) {
    // TODO: Implement protocol abstraction comparison
    // 1. Define common command test sequence
    // 2. Run with TCP (port 18089)
    // 3. Run with FIFO
    // 4. Compare results
    // 5. Cleanup
    GTEST_SKIP() << "Protocol abstraction validation not yet implemented - skeleton only";
}

//======>END OF TEST IMPLEMENTATIONS===============================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF TEST NOTES AND IMPLEMENTATION GUIDANCE===========================================
/**
 * 🔧 IMPLEMENTATION GUIDANCE:
 *
 * 1. PORT MANAGEMENT:
 *    - Each test uses unique port to avoid conflicts
 *    - Base port 18080, increment for each test
 *    - Consider using port 0 (dynamic allocation) for future flexibility
 *
 * 2. TCP-SPECIFIC SETUP:
 *    - IOC_SrvURI_T with .pProtocol = IOC_SRV_PROTO_TCP
 *    - URI format: "tcp://localhost:{port}/{service_name}"
 *    - Port number specified in SrvURI.Port field
 *
 * 3. PROTOCOL LAYER TESTING:
 *    - Most tests mirror UT_CommandTypical.cxx patterns
 *    - Key differences: TCP socket lifecycle, network timing
 *    - Same command API (IOC_execCMD), different transport layer
 *
 * 4. ERROR HANDLING:
 *    - Network-specific errors: connection refused, timeout, socket closed
 *    - Port conflicts: IOC_onlineService should fail gracefully
 *    - Resource cleanup: ensure TCP sockets closed properly
 *
 * 5. DEBUGGING TIPS:
 *    - Use netstat/lsof to verify port binding
 *    - Check TCP receiver thread startup (_IOC_SrvProtoTCP.c)
 *    - Validate message framing (TCPMessageHeader_T)
 *    - Monitor socket states with SO_ERROR socket option
 *
 * 6. PERFORMANCE CONSIDERATIONS:
 *    - TCP has higher latency than FIFO (network stack overhead)
 *    - Consider adjusting timeouts for network transport
 *    - Socket buffering may affect timing-sensitive tests
 *
 * 7. CROSS-REFERENCE:
 *    - UT_CommandTypical.cxx: FIFO-based command patterns (main reference)
 *    - UT_CommandTypicalAutoAccept.cxx: Auto-accept patterns (future TCP extension)
 *    - _IOC_SrvProtoTCP.c: TCP protocol implementation details
 *    - _IOC_SrvProtoFifo.c: FIFO protocol for comparison
 *
 * 🚀 GETTING STARTED:
 *    1. Start with TC-1 (basic PING command over TCP)
 *    2. Copy IOC_SrvURI_T setup from FIFO tests
 *    3. Change .pProtocol to IOC_SRV_PROTO_TCP
 *    4. Set .Port = 18080
 *    5. Run test, debug TCP protocol layer if needed
 *    6. Expand to other test cases progressively
 */
//======>END OF TEST NOTES AND IMPLEMENTATION GUIDANCE=============================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF TODO/IMPLEMENTATION TRACKING SECTION=============================================
// 🔴 IMPLEMENTATION STATUS TRACKING - Organized by Priority and Category
//
// PURPOSE:
//   Track test implementation progress using TDD Red→Green methodology.
//   Maintain visibility of what's done, in progress, and planned.
//
// STATUS LEGEND:
//   ⚪ TODO/PLANNED:      Designed but not implemented yet.
//   🔴 RED/FAILING:       Test written, but production code is missing or incorrect.
//   🟢 GREEN/PASSED:      Test written and passing.
//   ⚠️  ISSUES:           Known problem needing attention.
//   🚫 BLOCKED:          Cannot proceed due to a dependency.
//
// PRIORITY LEVELS:
//   P1 🥇 FUNCTIONAL:     Must complete before P2 (ValidFunc + InvalidFunc).
//   P2 🥈 DESIGN-ORIENTED: Test after P1 (State, Capability, Concurrency).
//   P3 🥉 QUALITY-ORIENTED: Test for quality attributes (Performance, Robust, etc.).
//   P4 🎯 ADDONS:          Optional (Demo, Examples).
//
// WORKFLOW:
//   1. Complete all P1 tests (this is the gate before P2).
//   2. Move to P2 tests based on design complexity.
//   3. Add P3 tests for specific quality requirements.
//   4. Mark status as you go: ⚪ TODO → 🔴 RED → 🟢 GREEN.
//
//===================================================================================================
// P1 🥇 FUNCTIONAL TESTING – ValidFunc (Typical + Boundary)
//===================================================================================================
//
// [@US-1] TCP Service as CmdExecutor - ValidFunc/Typical
//
//   🟢 [@AC-1,US-1] TC-1: verifyTcpServiceAsCmdExecutor_bySingleClient_expectSynchronousResponse
//        - Description: Basic TCP command execution (PING over socket)
//        - Category: Typical (ValidFunc)
//        - Protocol: tcp://localhost:18080/CmdTypicalTCP_SingleClient
//        - Status: 🟢 GREEN - Test passing (TCP protocol layer fixed for command response)
//        - Actual effort: ~3 hours (TCP setup + OUT payload transmission fix)
//        - Dependencies: TCP protocol layer working, receiver thread functional
//        - Notes: Fixed _IOC_SrvProtoTCP.c to send/receive OUT payload data separately
//
//   ⚪ [@AC-2,US-1] TC-1: verifyTcpServiceAsCmdExecutor_byMultipleCommandTypes_expectProperExecution
//        - Description: Multiple command types over TCP (PING, ECHO, CALC)
//        - Category: Typical (ValidFunc)
//        - Protocol: tcp://localhost:18081/CmdTypicalTCP_MultiTypes
//        - Estimated effort: 1 hour
//        - Dependencies: TC-1 passing
//
//   ⚪ [@AC-3,US-1] TC-1: verifyTcpServiceAsCmdExecutor_byMultipleClients_expectIsolatedExecution
//        - Description: Multi-client TCP command isolation
//        - Category: Typical (ValidFunc)
//        - Protocol: tcp://localhost:18082/CmdTypicalTCP_MultiClient
//        - Estimated effort: 2 hours
//        - Dependencies: TC-1 passing, concurrent testing setup
//
// [@US-1] TCP Service as CmdExecutor - ValidFunc/Boundary
//
//   ⚪ [@AC-4,US-1] TC-1: verifyTcpServiceAsCmdExecutor_byTimeoutConstraints_expectProperTiming
//        - Description: TCP command timeout validation
//        - Category: Boundary (ValidFunc)
//        - Protocol: tcp://localhost:18083/CmdTypicalTCP_Timeout
//        - Estimated effort: 1.5 hours
//        - Dependencies: DELAY command support
//
// [@US-2] TCP Service as CmdInitiator - ValidFunc/Typical
//
//   ⚪ [@AC-1,US-2] TC-1: verifyTcpServiceAsCmdInitiator_bySingleClient_expectClientExecution
//        - Description: Reversed TCP command flow (service→client)
//        - Category: Typical (ValidFunc)
//        - Protocol: tcp://localhost:18084/CmdTypicalTCP_Reversed
//        - Estimated effort: 1.5 hours
//        - Dependencies: P1 ValidFunc/Typical complete for US-1
//
//   ⚪ [@AC-2,US-2] TC-1: verifyTcpServiceAsCmdInitiator_byMultipleClients_expectOrchestration
//        - Description: Multi-client TCP orchestration
//        - Category: Typical (ValidFunc)
//        - Protocol: tcp://localhost:18085/CmdTypicalTCP_Orchestrate
//        - Estimated effort: 1.5 hours
//        - Dependencies: Previous US-2 TC passing
//
//===================================================================================================
// P1 🥇 FUNCTIONAL TESTING – InvalidFunc (Fault)
//===================================================================================================
//
// [@US-3] Network-Specific Error Handling - InvalidFunc/Fault
//
//   ⚪ [@AC-2,US-3] TC-1: verifyTcpConnectionFailure_byClosedSocket_expectGracefulError
//        - Description: TCP connection failure handling
//        - Category: Fault (InvalidFunc)
//        - Protocol: tcp://localhost:18087/CmdTypicalTCP_ConnFail
//        - Estimated effort: 2 hours
//        - Dependencies: All ValidFunc tests passing
//        - Notes: Test socket close, verify graceful degradation
//
//   ⚪ [@AC-3,US-3] TC-1: verifyTcpNetworkTimeout_bySlowResponse_expectTimeoutBehavior
//        - Description: TCP network timeout scenarios
//        - Category: Fault (InvalidFunc)
//        - Protocol: tcp://localhost:18088/CmdTypicalTCP_NetTimeout
//        - Estimated effort: 1.5 hours
//        - Dependencies: Timeout boundary tests passing
//
// 🚪 GATE P1: All P1 tests must be GREEN before proceeding to P2.
//   ✅ All ValidFunc tests GREEN (Typical + Boundary)
//   ✅ All InvalidFunc tests GREEN (Fault)
//   ✅ TCP protocol layer stable
//   ✅ No critical network-related bugs
//
//===================================================================================================
// P2 🥈 DESIGN-ORIENTED TESTING – State, Concurrency
//===================================================================================================
//
// [@US-3] Network-Specific Scenarios - Design/State
//
//   ⚪ [@AC-1,US-3] TC-1: verifyTcpServicePortBinding_byOnlineService_expectSuccessfulBind
//        - Description: TCP port binding validation
//        - Category: State
//        - Protocol: tcp://localhost:18086/CmdTypicalTCP_PortBind
//        - Estimated effort: 1 hour
//        - Dependencies: P1 complete
//        - Notes: Verify socket state transitions (bind→listen→accept)
//
//===================================================================================================
// P3 🥉 QUALITY-ORIENTED TESTING – Compatibility
//===================================================================================================
//
// [@US-4] Protocol Abstraction - Quality/Compatibility
//
//   ⚪ [@AC-1,US-4] TC-1: verifyProtocolAbstraction_byTcpVsFifo_expectIdenticalBehavior
//        - Description: TCP vs FIFO behavior comparison
//        - Category: Compatibility
//        - Protocol: tcp://localhost:18089/AbstractionTest + fifo://local-process/AbstractionTest
//        - Estimated effort: 2 hours
//        - Dependencies: P1 complete, UT_CommandTypical.cxx FIFO tests as reference
//        - Notes: Validate protocol-agnostic API patterns
//
//   ⚪ [@AC-2,US-4] TC-1: verifyProtocolUri_byDifferentProtocols_expectOnlyUriDifference
//        - Description: Protocol URI as only difference validation
//        - Category: Compatibility
//        - Protocol: N/A (code pattern validation)
//        - Estimated effort: 1 hour
//        - Dependencies: P1 complete
//        - Notes: Create protocol-agnostic service setup helper
//
// 🚪 GATE P3: Quality attributes validated, production ready.
//
//===================================================================================================
// ✅ COMPLETED TESTS (for reference, can be removed after stable)
//===================================================================================================
//
//   [None yet - all tests in PLANNED state]
//
///////////////////////////////////////////////////////////////////////////////////////////////////
//======>END OF TODO/IMPLEMENTATION TRACKING SECTION===============================================

// END OF FILE
