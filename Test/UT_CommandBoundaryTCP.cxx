///////////////////////////////////////////////////////////////////////////////////////////////////
// Command Boundary TCP — Boundary Testing for TCP Protocol
//
// PURPOSE:
//   Verify TCP command execution at edge cases, parameter limits, and boundary conditions
//   to ensure robust behavior at the extremes of valid input ranges.
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

#include <chrono>
#include <thread>

#include "_UT_IOC_Common.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF OVERVIEW OF THIS UNIT TESTING FILE===============================================
/**
 * @brief
 *   [WHAT] This file verifies TCP command execution at boundary conditions and limits
 *   [WHERE] in the IOC Command API with TCP protocol layer (_IOC_SrvProtoTCP.c)
 *   [WHY] to ensure robust behavior at edge cases and parameter extremes.
 *
 * SCOPE:
 *   - [In scope]: Boundary conditions for TCP command execution (timeouts, payload sizes, connection limits)
 *   - [In scope]: Edge cases at min/max valid parameter ranges
 *   - [In scope]: TCP-specific boundaries (port numbers, socket limits, buffer sizes)
 *   - [In scope]: Rapid state transitions (connect/disconnect cycles)
 *   - [Out of scope]: Invalid inputs (see UT_CommandMisuseTCP.cxx)
 *   - [Out of scope]: External fault scenarios (see UT_CommandFaultTCP.cxx)
 *   - [Out of scope]: Typical happy path scenarios (see UT_CommandTypicalTCP.cxx)
 *
 * KEY CONCEPTS:
 *   - Boundary Testing: Test at edges of valid input ranges (min, max, zero, limit-1, limit+0)
 *   - TCP Limits: Socket limits, connection limits, buffer boundaries
 *   - Timeout Boundaries: Zero timeout, minimum timeout, maximum timeout
 *   - Payload Boundaries: Empty payload, maximum payload size
 *   - Port Boundaries: Valid port range edges (1024, 65535)
 *   - Connection Boundaries: Rapid connect/disconnect, connection queue limits
 *
 * RELATIONSHIPS:
 *   - Extends: UT_CommandTypicalTCP.cxx (typical scenarios)
 *   - Complements: UT_CommandMisuseTCP.cxx (invalid inputs), UT_CommandFaultTCP.cxx (faults)
 *   - Depends on: IOC Command API, TCP protocol layer implementation
 *   - Production code: Source/_IOC_SrvProtoTCP.c, Source/IOC_Command.c
 */
//======>END OF OVERVIEW OF THIS UNIT TESTING FILE=================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF UNIT TESTING DESIGN==============================================================
/**************************************************************************************************
 *  TEST CASE DESIGN ASPECTS/CATEGORIES
 *
 * DESIGN PRINCIPLE: IMPROVE VALUE • AVOID LOSS • BALANCE SKILL vs COST
 *
 * PRIORITY FRAMEWORK:
 *   P1 🥇 FUNCTIONAL:     ValidFunc(Typical + Boundary) + InvalidFunc(Misuse + Fault)
 *   P2 🥈 DESIGN-ORIENTED: State, Capability, Concurrency
 *   P3 🥉 QUALITY-ORIENTED: Performance, Robust, Compatibility, Configuration
 *
 * CONTEXT-SPECIFIC ADJUSTMENT:
 *   - File Focus: P1 Boundary (ValidFunc) - edge cases with valid inputs
 *   - Rationale: Boundary conditions often reveal off-by-one errors, buffer issues, timing edge cases
 *   - Risk: High impact (crashes, hangs) if boundaries not handled correctly
 *
 * RISK ASSESSMENT:
 *   US-1/AC-1/TC-1 (Timeout boundaries): Impact=3, Likelihood=3, Uncertainty=2 → Score=18 (High priority)
 *   US-1/AC-2/TC-1 (Payload boundaries): Impact=3, Likelihood=2, Uncertainty=2 → Score=12 (Medium-high)
 *   US-2/AC-1/TC-1 (Connection boundaries): Impact=2, Likelihood=2, Uncertainty=2 → Score=8 (Medium)
 *   US-2/AC-2/TC-1 (Port boundaries): Impact=2, Likelihood=2, Uncertainty=1 → Score=4 (Low-medium)
 *   US-3/AC-1/TC-1 (Buffer boundaries): Impact=3, Likelihood=2, Uncertainty=2 → Score=12 (Medium-high)
 *
 * COVERAGE STRATEGY: Boundary Type × Parameter × Range Extreme
 *
 * COVERAGE MATRIX (Systematic Test Planning):
 * ┌──────────────────────┬──────────────────┬───────────────────┬────────────────────────────┐
 * │ Boundary Type        │ Parameter        │ Range Extreme     │ Key Scenarios              │
 * ├──────────────────────┼──────────────────┼───────────────────┼────────────────────────────┤
 * │ Timing               │ Timeout          │ Zero, Min, Max    │ US-1: Timeout edges        │
 * │ Data Size            │ Payload          │ Empty, Max        │ US-1: Payload sizes        │
 * │ Connection           │ Client Count     │ 1, Max            │ US-2: Connection limits    │
 * │ Network              │ Port Number      │ 1024, 65535       │ US-2: Port range edges     │
 * │ Protocol             │ Buffer Size      │ Full, Partial     │ US-3: Buffer boundaries    │
 * │ State Transition     │ Lifecycle        │ Rapid cycles      │ US-3: State boundaries     │
 * └──────────────────────┴──────────────────┴───────────────────┴────────────────────────────┘
 *
 * QUALITY GATE P1 (Boundary):
 *   ✅ All timeout boundary tests GREEN (zero, min, max timeout)
 *   ✅ All payload boundary tests GREEN (empty, large payload)
 *   ✅ All connection boundary tests GREEN (single, max connections)
 *   ✅ All port boundary tests GREEN (valid range edges)
 *   ✅ All buffer boundary tests GREEN (full, partial buffers)
 *   ✅ No crashes, hangs, or resource leaks at boundaries
 */
///////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF USER STORY=======================================================================
/**
 * US-1: As a command developer, I want TCP command execution to handle timing and data size boundaries
 *       so that commands work correctly at extreme timeout values and payload sizes.
 *
 * US-2: As a system integrator, I want TCP command execution to handle connection and port boundaries
 *       so that the system operates correctly at network resource limits.
 *
 * US-3: As a reliability engineer, I want TCP command execution to handle protocol buffer boundaries
 *       so that message framing works correctly at buffer limits and rapid state transitions.
 */
//======>END OF USER STORY==========================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//=======>BEGIN OF ACCEPTANCE CRITERIA==============================================================
/**
 * [@US-1] Timing and Data Size Boundaries
 *  AC-1: GIVEN a TCP command with timeout boundary values,
 *         WHEN timeout is zero, minimum, or maximum valid value,
 *         THEN command execution handles timeout correctly without hang or crash.
 *  AC-2: GIVEN a TCP command with payload size boundaries,
 *         WHEN payload is empty or at maximum allowed size,
 *         THEN message framing and transmission work correctly.
 *  AC-3: GIVEN a TCP command with rapid repeated execution,
 *         WHEN commands execute back-to-back at minimum intervals,
 *         THEN each command completes correctly without interference.
 *
 * [@US-2] Connection and Port Boundaries
 *  AC-1: GIVEN a TCP service with connection limit boundaries,
 *         WHEN client count reaches maximum supported connections,
 *         THEN service handles connections gracefully up to limit.
 *  AC-2: GIVEN a TCP service with port number boundaries,
 *         WHEN service binds to valid edge ports (1024, high port),
 *         THEN socket binding succeeds for all valid port values.
 *  AC-3: GIVEN a TCP service with rapid connect/disconnect cycles,
 *         WHEN clients connect and disconnect in quick succession,
 *         THEN service maintains stability without resource leaks.
 *
 * [@US-3] Protocol Buffer and State Boundaries
 *  AC-1: GIVEN TCP message framing with buffer boundaries,
 *         WHEN message fills entire buffer or spans multiple buffers,
 *         THEN protocol correctly handles buffer-boundary messages.
 *  AC-2: GIVEN a TCP connection with partial message scenarios,
 *         WHEN message arrives in fragments at boundaries,
 *         THEN receiver correctly assembles complete message.
 *  AC-3: GIVEN a TCP service with concurrent command boundaries,
 *         WHEN maximum concurrent commands execute simultaneously,
 *         THEN service processes all commands without queue overflow.
 */
//=======>END OF ACCEPTANCE CRITERIA================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF TEST CASES=======================================================================
/**************************************************************************************************
 * @brief 【TCP Command Boundary Test Cases】
 *
 * ORGANIZATION: By Boundary Type (Timing → Data → Connection → Protocol)
 * STATUS TRACKING: TODO = Planned/TODO，🔴 = Implemented/RED, GREEN = Passed/GREEN
 *
 * PORT ALLOCATION STRATEGY:
 *  - Base port: 19080 (different from Typical tests to avoid conflicts)
 *  - Range: 19080-19099 for boundary tests
 *
 * ===============================================================================
 * [US-1]: Timing and Data Size Boundaries
 * ===============================================================================
 *
 * [@AC-1,US-1] Timeout boundary handling (MOVED from UT_CommandTypicalTCP.cxx)
 *  GREEN TC-1: verifyTcpCommandTimeout_byBoundaryValues_expectCorrectBehavior
 */
*@[ Purpose ] : Validate timeout handling at boundary values(zero, min, max) * @[ Brief ]
    : Test command execution with zero timeout,
    1ms timeout,
    max timeout *@[ Protocol ] : tcp :  // localhost:19080/CmdBoundaryTCP_Timeout
                                        *@[ Status ]
    : TODO
      -
      Implement timeout boundary validation *@[ Steps ] : *1. Online TCP service with DELAY command support * 2. Test
                                                          case 1:
Zero timeout(immediate timeout expected) * 3. Test case 2:
1ms timeout(very short, may timeout) * 4. Test case 3:
Maximum timeout value(should complete) * 5. Verify each timeout behavior is correct * 6. Cleanup * *
        [ @AC - 2, US - 1 ] Payload size boundaries *TODO TC
    - 1 : verifyTcpCommandPayload_byEmptyPayload_expectSuccess *@[ Purpose ] : Validate command with zero -
                                                                               length payload *@[ Brief ]
    : Send command with no payload data,
    verify execution *@[ Protocol ] : tcp :  // localhost:19081/CmdBoundaryTCP_EmptyPayload
                                             *@[ Status ] : TODO
                                                            -
                                                            Implement empty payload test *@[ Steps ]
    : *1. Online TCP service with command accepting empty payload
      *
      2. Client sends command with empty payload * 3. Verify command executes successfully *
      4. Verify response is correct * 5. Cleanup **TODO TC -
                                                            2
    : verifyTcpCommandPayload_byMaxPayload_expectSuccess *@[ Purpose ]
    : Validate command with maximum allowed payload size *@[ Brief ] : Send command with max payload(e.g., 64KB),
    verify transmission *@[ Protocol ] : tcp :  // localhost:19082/CmdBoundaryTCP_MaxPayload
                                                *@[ Status ]
    : TODO
      -
      Implement max payload test *@[ Steps ] : *1. Online TCP service with ECHO command *
                                               2. Client sends ECHO with maximum payload(64KB) *
                                               3. Verify command transmits and executes *
                                               4. Verify full payload echoed back correctly * 5. Cleanup * *
                                               [ @AC - 3, US - 1 ] Rapid repeated command execution *TODO TC
      -
      1 : verifyTcpCommandRapidExecution_byBackToBackCommands_expectAllComplete *@[ Purpose ]
    : Validate rapid consecutive command execution *@[ Brief ] : Execute 100 commands back
                                                                 -
                                                                 to -
                                                                 back with minimal delay *@[ Protocol ] : tcp
    :  // localhost:19083/CmdBoundaryTCP_Rapid
       *@[ Status ] : TODO
                      -
                      Implement rapid execution test *@[ Steps ]
    : * 1. Online TCP service with PING command * 2. Client executes
      100 PING commands in tight loop * 3. Verify all commands complete successfully * 4. Verify no commands lost
        or duplicated * 5. Cleanup ** == == == == == == == == == == == == == == == == == == == == == == == == == == ==
               == == == == == == == == == == == == == == == == == == == ==
    = *[US - 2] : Connection and Port Boundaries * == == == == == == == == == == == == == == == == == == == == == == ==
      == == == == == == == == == == == == == == == == == == == == == == == ==
    = **[ @AC - 1, US - 2 ] Connection limit boundaries * TODO TC -
      1 : verifyTcpMaxConnections_byLimitedClients_expectAllAccepted *
          @[ Purpose ] : Validate behavior at maximum connection limit * @[ Brief ] : Connect maximum allowed clients,
    verify all accepted *@[ Protocol ] : tcp :  // localhost:19084/CmdBoundaryTCP_MaxConn
                                                *@[ Status ] : TODO
                                                               -
                                                               Implement max connection test *@[ Steps ]
    : *1. Online TCP service
      *
      2. Connect N clients(where N = max supported connections) * 3. Verify all connections accepted successfully *
      4. Execute command from each client * 5. Verify all commands complete * 6. Cleanup all connections * *
      [ @AC - 2, US - 2 ] Port number boundaries *TODO TC -
                                                               1
    : verifyTcpPortBinding_byLowPort_expectSuccess *@[ Purpose ] : Validate binding to lowest valid non
                                                                   -
                                                                   privileged port *@[ Brief ]
    : Bind to port 1024(lowest non - root port) *
      @[ Protocol ] : tcp :  // localhost:1024/CmdBoundaryTCP_LowPort
                             *@[ Status ] : TODO -
                                                                   Implement low port test *@[ Steps ]
    : *1. Online TCP service on port 1024 *
      2. Verify bind succeeds * 3. Client connects and executes command * 4. Verify success *
      5. Cleanup *@[ Notes ] : May require special permissions on some systems **TODO TC
                                                                   -
                                                                   2
    : verifyTcpPortBinding_byHighPort_expectSuccess *@[ Purpose ] : Validate binding to highest valid port *@[ Brief ]
    : Bind to port 65535(max port number) *
      @[ Protocol ] : tcp :  // localhost:65535/CmdBoundaryTCP_HighPort
                             *@[ Status ] : TODO -
                                                                   Implement high port test *@[ Steps ]
    : *1. Online TCP service on port 65535 *
      2. Verify bind succeeds * 3. Client connects and executes command * 4. Verify success * 5. Cleanup * *
      [ @AC - 3, US - 2 ] Rapid connect / disconnect cycles *TODO TC
                                                                   -
                                                                   1
    : verifyTcpRapidCycles_byConnectDisconnect_expectStability *@[ Purpose ]
    : Validate stability during rapid connection state changes *@[ Brief ]
    : Perform 50 connect
      -
      disconnect cycles in quick succession *@[ Protocol ] : tcp :  // localhost:19085/CmdBoundaryTCP_RapidCycles
                                                                    *@[ Status ]
    : TODO
      -
      Implement rapid cycles test *@[ Steps ] : * 1. Online TCP service * 2. Loop 50 times
    : *a.Client connects *b.Execute one command *c.Client disconnects
      immediately * 3. Verify no resource leaks * 4. Verify service remains stable * 5. Cleanup ** ==
        == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == ==
        == == == == == == == == ==
    = *[US - 3] : Protocol Buffer and State Boundaries * == == == == == == == == == == == == == == == == == == == == ==
      == == == == == == == == == == == == == == == == == == == == == == == == == ==
    = **[ @AC - 1, US - 3 ] Buffer boundary message framing * TODO TC -
              1 : verifyTcpBufferBoundary_byExactBufferSize_expectCorrectFraming *
                  @[ Purpose ] : Validate message framing when message exactly fills buffer *
                                 @[ Brief ] : Send message that exactly matches internal buffer size *
                                              @[ Protocol ] : tcp :  // localhost:19086/CmdBoundaryTCP_BufferExact
                                                                     *@[ Status ]
    : TODO -
              Implement buffer boundary test * @[ Steps ] : *1. Determine internal buffer size from implementation *
                                                            2. Online TCP service with ECHO command *
                                                            3. Send payload that exactly fills buffer *
                                                            4. Verify message received
          and echoed correctly * 5. Cleanup * *[ @AC - 2, US - 3 ] Partial message assembly * TODO TC -
                  1 : verifyTcpPartialMessage_byFragmentedTransmission_expectCorrectAssembly *
                      @[ Purpose ] : Validate correct message assembly from fragments *
                                     @[ Brief ] : Simulate fragmented message transmission at boundaries *
                                                  @[ Protocol ] : tcp :  // localhost:19087/CmdBoundaryTCP_Fragment
                                                                         *@[ Status ]
    : TODO -
                  Implement partial message test * @[ Steps ] : *1. Online TCP service *
                                                                2. Send command in multiple
                                                                fragments(header separately, payload separately) *
                                                                3. Verify receiver assembles complete message *
                                                                4. Verify command executes correctly * 5. Cleanup *
                                                                @[ Notes ] : May require raw socket manipulation
      or protocol inspection * *[ @AC - 3, US - 3 ] Concurrent command boundaries * TODO TC -
                 1 : verifyTcpConcurrentCommands_byMaxSimultaneous_expectAllComplete *
                     @[ Purpose ] : Validate behavior at maximum concurrent command limit *
                                    @[ Brief ] : Execute maximum allowed concurrent commands on single connection *
                                                 @[ Protocol ] : tcp :  // localhost:19088/CmdBoundaryTCP_MaxConcurrent
                                                                        *@[ Status ]
    : TODO -
                 Implement max concurrent test * @[ Steps ] : *1. Online TCP service * 2. Client connects *
                                                              3. Fire maximum concurrent commands without waiting *
                                                              4. Verify all commands queue
             and execute * 5. Verify all responses received * 6. Cleanup * /
///////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF COMMON TEST INFRASTRUCTURE=======================================================

// Test base port for boundary tests
#define _UT_BOUNDARY_TCP_BASE_PORT 19080

// Timeout values for boundary testing
#define _UT_BOUNDARY_TIMEOUT_ZERO 0
#define _UT_BOUNDARY_TIMEOUT_MIN 1
#define _UT_BOUNDARY_TIMEOUT_MAX 60000
#define _UT_BOUNDARY_TIMEOUT_TYPICAL 5000

// Payload size boundaries
#define _UT_BOUNDARY_PAYLOAD_EMPTY 0
#define _UT_BOUNDARY_PAYLOAD_SMALL 64
#define _UT_BOUNDARY_PAYLOAD_MEDIUM 1024
#define _UT_BOUNDARY_PAYLOAD_LARGE 65536

// Connection boundaries
#define _UT_BOUNDARY_MAX_CONNECTIONS 10
#define _UT_BOUNDARY_RAPID_CYCLES 50
#define _UT_BOUNDARY_RAPID_COMMANDS 100

                     /**
                      * @brief Callback for testing various command types with boundary conditions
                      */
                     static IOC_Result_T _CbExecCmd_BoundaryTest(IOC_CmdDesc_pT pCmdDesc) {
    if (!pCmdDesc) return IOC_RESULT_BUG;

    IOC_CmdID_T cmdID = IOC_CmdDesc_getCmdID(pCmdDesc);
    ULONG_T payloadLen = IOC_CmdDesc_getPayloadLen(pCmdDesc);
    const void *pPayload = IOC_CmdDesc_getPayload(pCmdDesc);

    // Handle different command types
    if (cmdID == _UT_IOC_CMDID_PING) {
        // PING: No payload, return PONG
        static const char pong[] = "PONG";
        IOC_CmdDesc_setResult(pCmdDesc, pong, sizeof(pong));
        return IOC_RESULT_SUCCESS;
    } else if (cmdID == _UT_IOC_CMDID_ECHO) {
        // ECHO: Return payload as-is
        if (payloadLen > 0 && pPayload) {
            IOC_CmdDesc_setResult(pCmdDesc, pPayload, payloadLen);
        } else {
            // Empty payload case
            static const char empty[] = "";
            IOC_CmdDesc_setResult(pCmdDesc, empty, 1);
        }
        return IOC_RESULT_SUCCESS;
    } else if (cmdID == _UT_IOC_CMDID_DELAY) {
        // DELAY: Sleep for specified milliseconds
        if (payloadLen >= sizeof(ULONG_T)) {
            ULONG_T delayMs = *(ULONG_T *)pPayload;
            std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
        }
        static const char done[] = "DONE";
        IOC_CmdDesc_setResult(pCmdDesc, done, sizeof(done));
        return IOC_RESULT_SUCCESS;
    }

    return IOC_RESULT_CMD_EXEC_FAILED;
}

//======>END OF COMMON TEST INFRASTRUCTURE=========================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF TEST IMPLEMENTATIONS=============================================================

// [@AC-1,US-1] TC-1: verifyTcpCommandTimeout_byBoundaryValues_expectCorrectBehavior
// MOVED from UT_CommandTypicalTCP.cxx: verifyTcpServiceAsCmdExecutor_byTimeoutConstraints_expectProperTiming
TEST(UT_TcpCommandBoundary, verifyTcpCommandTimeout_byBoundaryValues_expectCorrectBehavior) {
    //===SETUP===
    const USHORT_T srvPort = _UT_BOUNDARY_TCP_BASE_PORT;
    const char *srvName = "CmdBoundaryTCP_Timeout";

    // Service setup
    IOC_SrvConfigForCmd_T srvCfg = IOC_SRVCONFIGFORCMD_INIT_VALUE;
    snprintf(srvCfg.SrvURI, sizeof(srvCfg.SrvURI), "tcp://localhost:%u/%s", srvPort, srvName);
    srvCfg.UsageCapabilities = IOC_SRV_CAPABILITY_CMDEXECUTOR;
    srvCfg.Proto = IOC_SRV_PROTO_TCP;
    srvCfg.CbExecCmd_F = _CbExecCmd_BoundaryTest;
    srvCfg.AutoAcceptNewClient = true;

    IOC_LinkID_T srvLink = IOC_ID_INVALID;
    IOC_Result_T result = IOC_onlineService(&srvCfg, NULL, &srvLink);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result) << "Failed to online service";
    ASSERT_NE(IOC_ID_INVALID, srvLink) << "Invalid service link ID";

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Client setup
    IOC_LinkConfigForCmd_T clientCfg = IOC_LINKCONFIGFORCMD_INIT_VALUE;
    snprintf(clientCfg.PeerURI, sizeof(clientCfg.PeerURI), "tcp://localhost:%u/%s", srvPort, srvName);
    clientCfg.LinkUsage = IOC_LINK_USAGE_CMDINITIAOR;
    clientCfg.Proto = IOC_SRV_PROTO_TCP;

    IOC_LinkID_T clientLink = IOC_ID_INVALID;
    result = IOC_connectService(&clientCfg, NULL, &clientLink);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result) << "Failed to connect to service";
    ASSERT_NE(IOC_ID_INVALID, clientLink) << "Invalid client link ID";

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    //===BEHAVIOR===
    printf(" [BOUNDARY] Testing timeout boundary values\n");

    // Test 1: Short delay command that completes within timeout
    printf("  Test 1: Short delay (100ms) with typical timeout (5000ms)\n");
    {
        ULONG_T delayMs = 100;
        IOC_CmdDesc_T cmdDesc = IOC_CMDDESC_INIT_VALUE;
        IOC_CmdDesc_setCmdID(&cmdDesc, _UT_IOC_CMDID_DELAY);
        IOC_CmdDesc_setPayload(&cmdDesc, &delayMs, sizeof(delayMs));
        IOC_CmdDesc_setTimeout(&cmdDesc, _UT_BOUNDARY_TIMEOUT_TYPICAL);

        auto startTime = std::chrono::steady_clock::now();
        result = IOC_execCMD(clientLink, &cmdDesc);
        auto endTime = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

        EXPECT_EQ(IOC_RESULT_SUCCESS, result) << "Short delay command should succeed";
        EXPECT_GE(duration, 100) << "Duration should be at least delay time";
        EXPECT_LT(duration, 1000) << "Duration should complete quickly";
        printf("    ✅ Short delay completed in %ldms\n", duration);
    }

    // Test 2: Command with very short timeout (likely to timeout)
    printf("  Test 2: Delayed response (500ms) with minimum timeout (1ms)\n");
    {
        ULONG_T delayMs = 500;
        IOC_CmdDesc_T cmdDesc = IOC_CMDDESC_INIT_VALUE;
        IOC_CmdDesc_setCmdID(&cmdDesc, _UT_IOC_CMDID_DELAY);
        IOC_CmdDesc_setPayload(&cmdDesc, &delayMs, sizeof(delayMs));
        IOC_CmdDesc_setTimeout(&cmdDesc, _UT_BOUNDARY_TIMEOUT_MIN);

        auto startTime = std::chrono::steady_clock::now();
        result = IOC_execCMD(clientLink, &cmdDesc);
        auto endTime = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

        EXPECT_EQ(IOC_RESULT_TIMEOUT, result) << "Should timeout with 1ms timeout";
        EXPECT_LT(duration, 100) << "Should timeout quickly";
        printf("    ✅ Command timed out as expected in %ldms\n", duration);
    }

    // Test 3: Command with maximum timeout
    printf("  Test 3: Quick command (10ms) with maximum timeout (60000ms)\n");
    {
        ULONG_T delayMs = 10;
        IOC_CmdDesc_T cmdDesc = IOC_CMDDESC_INIT_VALUE;
        IOC_CmdDesc_setCmdID(&cmdDesc, _UT_IOC_CMDID_DELAY);
        IOC_CmdDesc_setPayload(&cmdDesc, &delayMs, sizeof(delayMs));
        IOC_CmdDesc_setTimeout(&cmdDesc, _UT_BOUNDARY_TIMEOUT_MAX);

        auto startTime = std::chrono::steady_clock::now();
        result = IOC_execCMD(clientLink, &cmdDesc);
        auto endTime = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

        EXPECT_EQ(IOC_RESULT_SUCCESS, result) << "Quick command should succeed with max timeout";
        EXPECT_GE(duration, 10) << "Duration should be at least delay time";
        EXPECT_LT(duration, 500) << "Should complete quickly despite large timeout";
        printf("    ✅ Quick command completed in %ldms (max timeout: %dms)\n", duration, _UT_BOUNDARY_TIMEOUT_MAX);
    }

    //===VERIFY===
    printf("✅ [BOUNDARY] All timeout boundary tests passed\n");

    //===CLEANUP===
    IOC_disconnectService(clientLink);
    IOC_offlineService(srvLink);
}

//======>END OF TEST IMPLEMENTATIONS===============================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF TODO/IMPLEMENTATION TRACKING SECTION=============================================
/**
 * 🔴 IMPLEMENTATION STATUS TRACKING - Boundary Testing (P1 ValidFunc)
 *
 * STATUS LEGEND:
 *   TODO TODO/PLANNED:      Designed but not implemented
 *   🔴 RED/IMPLEMENTED:   Test written and failing (need prod code)
 *   GREEN GREEN/PASSED:      Test written and passing
 *
 * PRIORITY LEVELS:
 *   🥇 HIGH:    Critical boundaries (timeouts, payload sizes)
 *   🥈 MEDIUM:  Important limits (connections, ports)
 *   🥉 LOW:     Advanced boundaries (buffers, concurrency)
 *
 *=================================================================================================
 * 🥇 HIGH PRIORITY – Critical Boundaries
 *=================================================================================================
 *   GREEN [@AC-1,US-1] TC-1: verifyTcpCommandTimeout_byBoundaryValues_expectCorrectBehavior
 *        - Status: MOVED from UT_CommandTypicalTCP.cxx and IMPLEMENTED
 *        - Tests: Zero timeout (N/A), min timeout (1ms), max timeout (60s)
 *        - Notes: Covers most critical timing boundary cases
 *
 *   TODO [@AC-2,US-1] TC-1: verifyTcpCommandPayload_byEmptyPayload_expectSuccess
 *   TODO [@AC-2,US-1] TC-2: verifyTcpCommandPayload_byMaxPayload_expectSuccess
 *
 *=================================================================================================
 * 🥈 MEDIUM PRIORITY – Connection and Port Boundaries
 *=================================================================================================
 *   TODO [@AC-3,US-1] TC-1: verifyTcpCommandRapidExecution_byBackToBackCommands_expectAllComplete
 *   TODO [@AC-1,US-2] TC-1: verifyTcpMaxConnections_byLimitedClients_expectAllAccepted
 *   TODO [@AC-2,US-2] TC-1: verifyTcpPortBinding_byLowPort_expectSuccess
 *   TODO [@AC-2,US-2] TC-2: verifyTcpPortBinding_byHighPort_expectSuccess
 *   TODO [@AC-3,US-2] TC-1: verifyTcpRapidCycles_byConnectDisconnect_expectStability
 *
 *=================================================================================================
 * 🥉 LOW PRIORITY – Protocol and Buffer Boundaries
 *=================================================================================================
 *   TODO [@AC-1,US-3] TC-1: verifyTcpBufferBoundary_byExactBufferSize_expectCorrectFraming
 *   TODO [@AC-2,US-3] TC-1: verifyTcpPartialMessage_byFragmentedTransmission_expectCorrectAssembly
 *   TODO [@AC-3,US-3] TC-1: verifyTcpConcurrentCommands_byMaxSimultaneous_expectAllComplete
 *
 *=================================================================================================
 * 📊 SUMMARY
 *=================================================================================================
 *   TOTAL: 11 test cases designed
 *   IMPLEMENTED: 1/11 (9%)
 *   HIGH PRIORITY: 3 tests (1 GREEN, 2 TODO)
 *   MEDIUM PRIORITY: 5 tests (all TODO)
 *   LOW PRIORITY: 3 tests (all TODO)
 *
 *   NEXT STEPS:
 *   1. Implement payload boundary tests (HIGH priority)
 *   2. Implement connection limit tests (MEDIUM priority)
 *   3. Implement protocol buffer tests (LOW priority)
 */
///////////////////////////////////////////////////////////////////////////////////////////////////
//======>END OF TODO/IMPLEMENTATION TRACKING SECTION===============================================

// END OF FILE
