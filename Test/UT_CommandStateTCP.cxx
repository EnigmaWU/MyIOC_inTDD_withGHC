///////////////////////////////////////////////////////////////////////////////////////////////////
// Command State TCP Implementation: TCP-Specific State Integration Testing
//
// 🎯 PURPOSE: Verify TCP-protocol-specific command state behaviors and interactions
// 🔗 RELATIONSHIP: Complements UT_CommandStateUS1-5.cxx (protocol-agnostic state testing)
// 📋 FOCUS: TCP connection state × Command execution state integration
//
// 📊 DESIGN RATIONALE:
//    • UT_CommandStateUS1-5.cxx: Protocol-agnostic state machine testing
//    • UT_CommandStateTCP.cxx: TCP-specific state integration scenarios
//    • Key Difference: Connection lifecycle, TCP-specific errors, TCP protocol behavior
//    • US-4 covers generic timeout/error (protocol-agnostic)
//    • TCP file covers TCP-specific errors (ECONNRESET, EPIPE, flow control)
//
// 🏗️ ARCHITECTURE CONTEXT:
//    This file addresses TCP-specific state scenarios that cannot be tested generically:
//    - Command state during TCP connection loss/recovery
//    - Error propagation from TCP layer to command state
//    - State consistency during TCP flow control and backpressure
//    - Command state behavior during TCP connection establishment failures
//
// 📖 RELATED DOCUMENTATION:
//    • See README_ArchDesign.md "CMD::Conet" for TCP connection state diagrams
//    • See UT_CommandState.h for dual-state testing framework
//    • See UT_CommandFaultTCP.cxx for TCP fault injection patterns
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <unistd.h>

#include <chrono>
#include <thread>
#include <vector>

#include "UT_CommandState.h"

// Include IOC APIs needed for state tracking
#include "IOC/IOC.h"
#include "IOC/IOC_CmdAPI.h"
#include "IOC/IOC_CmdDesc.h"
#include "IOC/IOC_SrvAPI.h"
#include "IOC/IOC_Types.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF IMPLEMENTATION OVERVIEW=========================================================
/**
 * @brief TCP-Specific Command State Testing Framework
 *
 * 🔄 TESTING SCOPE: TCP Protocol × Command State Integration
 *
 * This file focuses on TCP-specific state behaviors that complement the protocol-agnostic
 * state testing in UT_CommandStateUS1-5.cxx:
 *
 * 🟢 WHAT UT_CommandStateUS1-5.cxx TESTS (Protocol-Agnostic):
 *    ✓ Command state transitions (PENDING → PROCESSING → SUCCESS/FAILED/TIMEOUT)
 *    ✓ Link state evolution during command execution
 *    ✓ State isolation between concurrent commands
 *    ✓ State consistency across execution patterns
 *    ✓ Multi-role service state management
 *
 * 🔵 WHAT UT_CommandStateTCP.cxx TESTS (TCP-Specific):
 *    ⚡ Command state during TCP connection establishment (SYN→ESTABLISHED)
 *    ⚡ TCP-specific errors: ECONNRESET, EPIPE, ECONNREFUSED
 *    ⚡ Command state during TCP connection loss (mid-execution)
 *    ⚡ TCP flow control impact: send buffer full, backpressure, window management
 *    ⚡ TCP shutdown behavior: FIN vs RST impact on command state
 *    ⚡ TCP reconnection: command state during connection recovery
 *    ⚡ TCP layer transparency: retransmit doesn't affect command state
 *
 * ❌ WHAT UT_CommandStateTCP.cxx DOES NOT TEST (Covered by US-4):
 *    ✗ Generic timeout detection (US-4 AC-1)
 *    ✗ Generic error propagation (US-4 AC-3)
 *    ✗ Generic link recovery after error (US-4 AC-2)
 *    ✗ Generic mixed success/failure (US-4 AC-4)
 *    ✗ Generic error recovery (US-4 AC-5)
 *
 * 📊 TCP STATE × COMMAND STATE MATRIX:
 *    ┌────────────────────────┬──────────────────────────────────────────────────┐
 *    │ TCP Connection State   │ Expected Command State Behavior                  │
 *    ├────────────────────────┼──────────────────────────────────────────────────┤
 *    │ TCP_SYN_SENT           │ Command PENDING, waiting for connection          │
 *    │ TCP_ESTABLISHED        │ Command can transition to PROCESSING             │
 *    │ TCP_CLOSE_WAIT         │ Existing commands complete, new commands blocked │
 *    │ TCP_CLOSING            │ Commands transition to FAILED/TIMEOUT            │
 *    │ TCP_CLOSED             │ All commands must be FAILED or TIMEOUT           │
 *    └────────────────────────┴──────────────────────────────────────────────────┘
 *
 * 🎯 INTEGRATION FOCUS:
 *    • How TCP layer errors (connection loss) affect command state transitions
 *    • Whether command state properly reflects TCP connection health
 *    • Command cleanup and error handling during TCP failures
 *    • State consistency when TCP connection is restored
 */
//======>END OF IMPLEMENTATION OVERVIEW===========================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF TEST CASE ORGANIZATION==========================================================
/**************************************************************************************************
 * @brief 【TCP-Specific Command State Test Cases】
 *
 * ORGANIZATION STRATEGY:
 *  🔷 By TCP Connection Lifecycle Phase:
 *     • Connection Establishment Phase (SYN → ESTABLISHED)
 *     • Active Connection Phase (ESTABLISHED)
 *     • Connection Loss Phase (RESET, TIMEOUT)
 *     • Connection Recovery Phase (Reconnection)
 *     • Connection Termination Phase (Graceful/Ungraceful Close)
 *
 *  🔷 By TCP Error Type × Command State Impact:
 *     • Connection Refused → Command FAILED
 *     • Connection Reset → Command FAILED (mid-execution)
 *     • Connection Timeout → Command TIMEOUT
 *     • Send Buffer Full → Command PROCESSING (blocked)
 *     • Receive Timeout → Command TIMEOUT
 *
 *  🔷 By State Transition Timing:
 *     • Pre-connection: Command created before TCP connection ready
 *     • During-connection: Command executing when TCP error occurs
 *     • Post-failure: Command state after TCP connection lost
 *
 * 🎯 COVERAGE TARGET: 100% of TCP-specific state integration scenarios
 *
 * STATUS TRACKING: ⚪ = Planned/TODO，🔴 = Implemented/RED, 🟢 = Passed/GREEN, ⚠️ = Issues
 *
 * ⚪ FRAMEWORK STATUS: TCP-Specific Command State Testing - DESIGN PHASE
 *    • Core framework: NOT YET IMPLEMENTED
 *    • Test cases: SKELETON ONLY
 *    • Target: 18 test cases covering TCP-specific state scenarios
 *    • Reduced from 25: Removed 7 tests duplicating US-4 (timeout/generic error)
 *
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * 📋 [CAT-1]: TCP CONNECTION ESTABLISHMENT × COMMAND STATE
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * PURPOSE: Verify command state behavior during TCP connection setup phase
 *
 * ⚪ TC-1: verifyCommandState_duringTcpConnect_expectPendingBeforeEstablished
 *      @[Purpose]: Validate command remains PENDING until TCP connection established
 *      @[Brief]: Create command, initiate TCP connect, verify state during SYN_SENT phase
 *      @[TCP Focus]: Command waits for TCP handshake completion
 *      @[Expected]: Command PENDING while TCP state < ESTABLISHED
 *      @[Port]: 22080 (base port for state testing)
 *      @[Priority]: HIGH - Critical connection phase behavior
 *
 * ⚪ TC-2: verifyCommandState_afterTcpConnectSuccess_expectProcessingTransition
 *      @[Purpose]: Validate command transitions to PROCESSING once TCP connection ready
 *      @[Brief]: Monitor command state transition when TCP moves to ESTABLISHED
 *      @[TCP Focus]: State transition timing aligned with TCP handshake
 *      @[Expected]: PENDING → PROCESSING transition synchronized with TCP ESTABLISHED
 *      @[Port]: 22081
 *      @[Priority]: HIGH - Critical state transition timing
 *
 * ⚪ TC-3: verifyCommandState_whenTcpConnectRefused_expectFailedWithError
 *      @[Purpose]: Validate command immediately transitions to FAILED when connection refused
 *      @[Brief]: Attempt connect to offline server, verify quick FAILED state
 *      @[TCP Focus]: ECONNREFUSED error propagation to command state
 *      @[Expected]: Command FAILED with IOC_RESULT_LINK_OFFLINE or similar
 *      @[Port]: 22082 (server deliberately not started)
 *      @[Priority]: HIGH - Connection failure handling
 *
 * ⚪ TC-4: verifyCommandState_whenTcpConnectTimeout_expectTimeoutState
 *      @[Purpose]: Validate command transitions to TIMEOUT when TCP connect times out
 *      @[Brief]: Connect to unresponsive server (firewall/blackhole), verify timeout
 *      @[TCP Focus]: TCP connect timeout (SYN retransmit exhaustion)
 *      @[Expected]: Command TIMEOUT after TCP connect timeout expires
 *      @[Port]: 22083 (firewall simulation)
 *      @[Priority]: MEDIUM - Timeout during connection phase
 *
 * ⚪ TC-5: verifyLinkState_duringTcpConnectAttempt_expectConnectingSubState
 *      @[Purpose]: Validate link state reflects TCP connection attempt
 *      @[Brief]: Check IOC_getLinkState() during connection establishment
 *      @[TCP Focus]: Link state should show connecting/establishing
 *      @[Expected]: Link SubState indicates connection in progress
 *      @[Port]: 22084
 *      @[Priority]: MEDIUM - Link state during TCP handshake
 *
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * 📋 [CAT-2]: TCP CONNECTION LOSS × COMMAND STATE DURING EXECUTION
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * PURPOSE: Verify command state when TCP connection fails mid-execution
 *
 * ⚪ TC-6: verifyCommandState_whenTcpResetDuringExecution_expectFailedTransition
 *      @[Purpose]: Validate command transitions to FAILED when TCP connection reset mid-execution
 *      @[Brief]: Start command execution, force TCP RST, verify state change
 *      @[TCP Focus]: ECONNRESET during active command processing
 *      @[Expected]: PROCESSING → FAILED transition with connection error
 *      @[Port]: 22085
 *      @[Priority]: HIGH - Mid-execution connection loss
 *      @[Relation]: Similar to UT_CommandFaultTCP.cxx TC-3, but focuses on STATE
 *
 * ⚪ TC-7: verifyCommandState_whenTcpPipeBroken_expectFailedWithPipeError
 *      @[Purpose]: Validate command handles EPIPE (broken pipe) during send
 *      @[Brief]: Send command data after remote close, verify EPIPE detection
 *      @[TCP Focus]: Write to closed socket (EPIPE/SIGPIPE)
 *      @[Expected]: Command FAILED with pipe/send error
 *      @[Port]: 22086
 *      @[Priority]: HIGH - Send-side connection loss
 *
 * ⚪ TC-8: verifyLinkState_whenTcpConnectionReset_expectDisconnectedState
 *      @[Purpose]: Validate link state reflects TCP connection loss
 *      @[Brief]: Monitor IOC_getLinkState() when connection resets
 *      @[TCP Focus]: Link state synchronized with TCP state (TCP-specific)
 *      @[Expected]: Link state transitions to OFFLINE/DISCONNECTED
 *      @[Port]: 22087
 *      @[Priority]: HIGH - TCP connection state correlation
 *      @[Note]: Generic timeout/error recovery covered by US-4
 *
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * 📋 [CAT-3]: TCP FLOW CONTROL × COMMAND STATE
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * PURPOSE: Verify command state under TCP flow control conditions
 *
 * ⚪ TC-11: verifyCommandState_whenTcpSendBufferFull_expectProcessingWithDelay
 *      @[Purpose]: Validate command remains PROCESSING when TCP send buffer full
 *      @[Brief]: Send large payload, fill TCP buffer, verify state during blocking
 *      @[TCP Focus]: TCP flow control (zero window) delays command completion
 *      @[Expected]: Command stays PROCESSING until buffer drains
 *      @[Port]: 22088
 *      @[Priority]: HIGH - TCP flow control impact on state
 *      @[Relation]: UT_CommandFaultTCP TC-11 tests fault, this tests state
 *
 * ⚪ TC-10: verifyCommandState_whenTcpReceiveBufferFull_expectNormalProcessing
 *      @[Purpose]: Validate command state when receiver buffer full
 *      @[Brief]: Client slow to receive, server send blocked, verify state
 *      @[TCP Focus]: TCP receive window flow control
 *      @[Expected]: Command PROCESSING, waits for receiver to drain buffer
 *      @[Port]: 22089
 *      @[Priority]: LOW - Receiver-side flow control
 *
 * ⚪ TC-11: verifyCommandState_whenTcpBackpressureResolved_expectSuccessTransition
 *      @[Purpose]: Validate command completes successfully after flow control resolved
 *      @[Brief]: Block send, then unblock, verify command reaches SUCCESS
 *      @[TCP Focus]: Recovery from flow control condition
 *      @[Expected]: PROCESSING (blocked) → PROCESSING (unblocked) → SUCCESS
 *      @[Port]: 22090
 *      @[Priority]: HIGH - TCP flow control recovery
 *
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * 📋 [CAT-4]: TCP RECONNECTION × COMMAND STATE
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * PURPOSE: Verify command state during TCP connection recovery
 *
 * ⚪ TC-12: verifyCommandState_duringTcpReconnection_expectNewCommandPending
 *      @[Purpose]: Validate new commands can be created during reconnection
 *      @[Brief]: Drop connection, create new command, attempt reconnect
 *      @[TCP Focus]: Command state during reconnection attempt
 *      @[Expected]: New command PENDING during reconnection
 *      @[Port]: 22091
 *      @[Priority]: MEDIUM - TCP reconnection behavior
 *
 * ⚪ TC-13: verifyCommandState_afterReconnectionSuccess_expectResumedProcessing
 *      @[Purpose]: Validate commands resume after successful reconnection
 *      @[Brief]: Reconnect TCP, verify pending commands can execute
 *      @[TCP Focus]: State recovery after reconnection
 *      @[Expected]: Queued commands transition to PROCESSING
 *      @[Port]: 22092
 *      @[Priority]: MEDIUM - TCP reconnection state recovery
 *
 * ⚪ TC-14: verifyCommandState_afterReconnectionFailure_expectFailedState
 *      @[Purpose]: Validate commands fail if reconnection impossible
 *      @[Brief]: Fail reconnection permanently, verify command cleanup
 *      @[TCP Focus]: Permanent connection loss handling
 *      @[Expected]: All queued commands transition to FAILED
 *      @[Port]: 22093
 *      @[Priority]: MEDIUM - TCP reconnection failure handling
 *
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * 📋 [CAT-5]: TCP GRACEFUL/UNGRACEFUL SHUTDOWN × COMMAND STATE
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * PURPOSE: Verify command state during TCP shutdown (FIN vs RST)
 *
 * ⚪ TC-15: verifyCommandState_duringGracefulShutdown_expectCompletionBeforeClose
 *      @[Purpose]: Validate in-flight commands complete before graceful close
 *      @[Brief]: Initiate graceful shutdown, verify commands finish first
 *      @[TCP Focus]: FIN handshake after command completion
 *      @[Expected]: Commands reach SUCCESS/FAILED before connection closes
 *      @[Port]: 22094
 *      @[Priority]: HIGH - TCP graceful shutdown (FIN) behavior
 *
 * ⚪ TC-16: verifyCommandState_duringUngracefulShutdown_expectImmediateFailed
 *      @[Purpose]: Validate commands fail immediately on ungraceful close
 *      @[Brief]: Force abortive close (RST), verify immediate FAILED state
 *      @[TCP Focus]: RST vs FIN handling in command state
 *      @[Expected]: Commands immediately transition to FAILED
 *      @[Port]: 22095
 *      @[Priority]: HIGH - TCP abortive shutdown (RST) behavior
 *
 * ⚪ TC-17: verifyLinkState_afterTcpGracefulClose_expectCleanOffline
 *      @[Purpose]: Validate link state after clean TCP close
 *      @[Brief]: Monitor link state during FIN handshake
 *      @[TCP Focus]: Link state reflects graceful termination
 *      @[Expected]: Link transitions to OFFLINE cleanly
 *      @[Port]: 22096
 *      @[Priority]: MEDIUM - TCP FIN link state transition
 *
 * ⚪ TC-18: verifyLinkState_afterTcpAbortiveClose_expectErrorState
 *      @[Purpose]: Validate link state after abortive TCP close
 *      @[Brief]: Monitor link state during RST
 *      @[TCP Focus]: Link state reflects error termination
 *      @[Expected]: Link transitions to ERROR/OFFLINE with error code
 *      @[Port]: 22097
 *      @[Priority]: MEDIUM - TCP RST link state transition
 *
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * 📋 [CAT-6]: TCP LAYER TRANSPARENCY × COMMAND STATE
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * PURPOSE: Verify TCP layer operations don't affect command state incorrectly
 *
 * ⚪ TC-19: verifyCommandState_duringTcpRetransmit_expectStableProcessing
 *      @[Purpose]: Validate command state unaffected by TCP retransmissions
 *      @[Brief]: Induce packet loss, verify state during TCP recovery
 *      @[TCP Focus]: TCP retransmit is transparent to command state
 *      @[Expected]: Command remains PROCESSING during TCP retransmit
 *      @[Port]: 22098
 *      @[Priority]: LOW - TCP layer transparency
 *      @[Note]: Generic timeout testing covered by US-4 AC-1
 *
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * 📋 [CAT-7]: TCP ERROR CODE MAPPING × COMMAND STATE
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * PURPOSE: Verify TCP-specific error codes map correctly to command state
 *
 * ⚪ TC-20: verifyTcpErrorMapping_fromSocketError_toCommandResult
 *      @[Purpose]: Validate TCP error codes map correctly to IOC_Result_T
 *      @[Brief]: Generate TCP errors (ECONNRESET, EPIPE, ECONNREFUSED), verify mapping
 *      @[TCP Focus]: TCP errno → IOC_Result_T mapping accuracy
 *      @[Expected]: ECONNRESET→IOC_RESULT_CONN_RESET, EPIPE→IOC_RESULT_PIPE_ERROR
 *      @[Port]: 22099
 *      @[Priority]: HIGH - TCP error code accuracy
 *      @[Note]: Generic error propagation covered by US-4 AC-3
//======>END OF TEST CASE ORGANIZATION============================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF TCP-SPECIFIC STATE TESTING INFRASTRUCTURE=======================================

/**
 * @brief TCP Connection State Simulation Helper
 *        Provides controlled TCP connection states for testing
 */
class TcpConnectionSimulator {
   public:
    TcpConnectionSimulator(uint16_t port) : m_port(port), m_serverFd(-1), m_clientFd(-1), m_acceptedFd(-1) {}

    ~TcpConnectionSimulator() { cleanup(); }

    // Simulate server accepting connections
    bool startServer() {
        m_serverFd = socket(AF_INET, SOCK_STREAM, 0);
        if (m_serverFd < 0) return false;

        int opt = 1;
        setsockopt(m_serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        struct sockaddr_in addr = {};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(m_port);

        if (bind(m_serverFd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
            close(m_serverFd);
            m_serverFd = -1;
            return false;
        }

        if (listen(m_serverFd, 5) < 0) {
            close(m_serverFd);
            m_serverFd = -1;
            return false;
        }

        return true;
    }

    // Accept client connection
    bool acceptClient() {
        if (m_serverFd < 0) return false;

        struct sockaddr_in clientAddr = {};
        socklen_t addrLen = sizeof(clientAddr);
        m_acceptedFd = accept(m_serverFd, (struct sockaddr *)&clientAddr, &addrLen);

        return m_acceptedFd >= 0;
    }

    // Simulate client connection attempt
    bool connectClient() {
        m_clientFd = socket(AF_INET, SOCK_STREAM, 0);
        if (m_clientFd < 0) return false;

        struct sockaddr_in addr = {};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = inet_addr("127.0.0.1");
        addr.sin_port = htons(m_port);

        if (connect(m_clientFd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
            close(m_clientFd);
            m_clientFd = -1;
            return false;
        }

        return true;
    }

    // Force TCP reset (abortive close)
    void forceReset() {
        if (m_clientFd >= 0) {
            struct linger sl = {1, 0};  // SO_LINGER with timeout 0 = RST
            setsockopt(m_clientFd, SOL_SOCKET, SO_LINGER, &sl, sizeof(sl));
            close(m_clientFd);
            m_clientFd = -1;
        }
    }

    // Graceful close
    void gracefulClose() {
        if (m_clientFd >= 0) {
            shutdown(m_clientFd, SHUT_RDWR);
            close(m_clientFd);
            m_clientFd = -1;
        }
    }

    void cleanup() {
        if (m_acceptedFd >= 0) {
            close(m_acceptedFd);
            m_acceptedFd = -1;
        }
        if (m_clientFd >= 0) {
            close(m_clientFd);
            m_clientFd = -1;
        }
        if (m_serverFd >= 0) {
            close(m_serverFd);
            m_serverFd = -1;
        }
    }

    int getServerFd() const { return m_serverFd; }
    int getClientFd() const { return m_clientFd; }
    int getAcceptedFd() const { return m_acceptedFd; }

   private:
    uint16_t m_port;
    int m_serverFd;
    int m_clientFd;
    int m_acceptedFd;
}; /**
    * @brief TCP State × Command State Correlation Tracker
    *        Monitors both TCP connection state and command state simultaneously
    */
class TcpCommandStateTracker {
   public:
    struct StateSnapshot {
        std::chrono::steady_clock::time_point timestamp;
        // TCP state
        int tcpState;  // From getsockopt(TCP_INFO)
        bool tcpConnected;
        // Command state
        IOC_CmdStatus_E cmdStatus;
        IOC_Result_T cmdResult;
        // Link state
        IOC_LinkState_T linkMainState;
        IOC_LinkSubState_T linkSubState;
    };

    void captureSnapshot(IOC_CmdDesc_pT pCmdDesc, IOC_LinkID_T linkID, int tcpFd) {
        StateSnapshot snapshot;
        snapshot.timestamp = std::chrono::steady_clock::now();

        // Capture TCP state (simplified - full implementation would use TCP_INFO)
        snapshot.tcpConnected = (tcpFd >= 0);
        snapshot.tcpState = 0;  // TODO: Get actual TCP state via getsockopt

        // Capture command state
        if (pCmdDesc) {
            snapshot.cmdStatus = IOC_CmdDesc_getStatus(pCmdDesc);
            snapshot.cmdResult = IOC_CmdDesc_getResult(pCmdDesc);
        }

        // Capture link state
        IOC_getLinkState(linkID, &snapshot.linkMainState, &snapshot.linkSubState);

        m_history.push_back(snapshot);
    }

    bool verifyStateCorrelation() {
        // Verify that TCP state, command state, and link state are consistent
        // For example: If TCP disconnected, command should be FAILED/TIMEOUT
        for (const auto &snapshot : m_history) {
            if (!snapshot.tcpConnected) {
                if (snapshot.cmdStatus == IOC_CMD_STATUS_PROCESSING) {
                    printf("⚠️  State Correlation Violation: TCP disconnected but command still PROCESSING\n");
                    return false;
                }
            }
        }
        return true;
    }

    void printHistory() {
        printf("\n📊 TCP × Command State History:\n");
        for (size_t i = 0; i < m_history.size(); i++) {
            const auto &s = m_history[i];
            printf("[%zu] TCP:%s Cmd:%d/%d Link:%d/%d\n", i, s.tcpConnected ? "CONN" : "DISC", s.cmdStatus, s.cmdResult,
                   s.linkMainState, s.linkSubState);
        }
    }

    void clear() { m_history.clear(); }
    size_t getSnapshotCount() const { return m_history.size(); }

   private:
    std::vector<StateSnapshot> m_history;
};

//======>END OF TCP-SPECIFIC STATE TESTING INFRASTRUCTURE=========================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF TEST CASE IMPLEMENTATIONS=======================================================

//=================================================================================================
// 📋 [CAT-1]: TCP CONNECTION ESTABLISHMENT × COMMAND STATE
//=================================================================================================

/**
 * TC-1: verifyCommandState_duringTcpConnect_expectPendingBeforeEstablished
 * @[Purpose]: Validate command remains PENDING until TCP connection established
 * @[Steps]:
 *   1) 🔧 SETUP: Initialize service (CmdExecutor), start TCP server
 *   2) 🎯 BEHAVIOR: Client initiates TCP handshake, monitor command state
 *   3) ✅ VERIFY: Command stays PENDING during connection establishment
 *   4) 🧹 CLEANUP: Close connection, offline service
 * @[Expect]: Command PENDING while TCP state < ESTABLISHED
 */
TEST(UT_CommandStateTCP, verifyCommandState_duringTcpConnect_expectPendingBeforeEstablished) {
    printf("🎯 BEHAVIOR: verifyCommandState_duringTcpConnect_expectPendingBeforeEstablished\n");

    // ═══════════════════════════════════════════════════════════════════════════
    // 🔧 SETUP: Online TCP service
    // ═══════════════════════════════════════════════════════════════════════════
    constexpr uint16_t TEST_PORT = 22080;

    // TODO: Create service with CmdExecutor capability
    // TODO: Setup TcpConnectionSimulator
    // TODO: Prepare command descriptor

    // ═══════════════════════════════════════════════════════════════════════════
    // 🎯 BEHAVIOR: Monitor command state during TCP connect
    // ═══════════════════════════════════════════════════════════════════════════

    // TODO: Initiate TCP connection in separate thread
    // TODO: Capture command state snapshots during handshake
    // TODO: Verify command PENDING until connection ESTABLISHED

    // ═══════════════════════════════════════════════════════════════════════════
    // ✅ VERIFY: State correlation
    // ═══════════════════════════════════════════════════════════════════════════

    // TODO: Assert command stayed PENDING during connect phase
    // TODO: Assert command transitioned after ESTABLISHED

    // ═══════════════════════════════════════════════════════════════════════════
    // 🧹 CLEANUP
    // ═══════════════════════════════════════════════════════════════════════════

    // Mark as RED - infrastructure ready, implementation pending
    GTEST_SKIP() << "🔴 RED: Test structure ready, awaiting full implementation";
}

//======>END OF TEST CASE IMPLEMENTATIONS=========================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF IMPLEMENTATION ROADMAP==========================================================
/**
 * 🗺️ IMPLEMENTATION ROADMAP FOR UT_CommandStateTCP.cxx
 *
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * PHASE 1: FOUNDATION (Week 1) - Priority: HIGH
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * ✅ Task 1.1: Design skeleton and test case categorization (COMPLETE - This file)
 * ⚪ Task 1.2: Implement TcpConnectionSimulator helper class
 *    - startServer(), connectClient(), forceReset(), gracefulClose()
 *    - Validate with simple socket programming tests
 *
 * ⚪ Task 1.3: Implement TcpCommandStateTracker helper class
 *    - captureSnapshot() with TCP_INFO support
 *    - verifyStateCorrelation() validation logic
 *    - printHistory() debugging output
 *
 * ⚪ Task 1.4: Create TCP state test fixture base class
 *    - SetUp(): Initialize IOC framework + TCP server
 *    - TearDown(): Cleanup connections and IOC resources
 *    - Helper methods: createCommandOnTcpLink(), waitForState(), etc.
 *
 * MILESTONE 1: Infrastructure ready for test implementation
 *
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * PHASE 2: HIGH-PRIORITY TEST CASES (Week 2-3) - Priority: HIGH
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * ⚪ Task 2.1: Implement CAT-1 (Connection Establishment) - TCs 1-3
 *    - TC-1: Command state during TCP connect (PENDING)
 *    - TC-2: Command state after connect success (PROCESSING)
 *    - TC-3: Command state on connect refused (FAILED)
 *
 * ⚪ Task 2.2: Implement CAT-2 (Connection Loss) - TCs 6-8
 *    - TC-6: Connection reset mid-execution (ECONNRESET)
 *    - TC-7: Broken pipe during send (EPIPE)
 *    - TC-8: Link state after connection reset
 *
 * ⚪ Task 2.3: Implement CAT-5 (Shutdown) - TCs 17-18
 *    - TC-17: Graceful shutdown sequencing
 *    - TC-18: Ungraceful shutdown immediate failure
 *
 * ⚪ Task 2.4: Implement CAT-7 (Error Mapping) - TC-20
 *    - TC-20: TCP errno → IOC_Result_T mapping (ECONNRESET, EPIPE, ECONNREFUSED)
 *
 * MILESTONE 2: Critical path test cases implemented and GREEN
 *
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * PHASE 3: MEDIUM-PRIORITY TEST CASES (Week 4) - Priority: MEDIUM
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * ⚪ Task 3.1: Implement CAT-1 remaining (TCs 4-5)
 *    - TC-4: Connect timeout
 *    - TC-5: Link state during connection
 *
 * ⚪ Task 3.2: Implement CAT-3 (Flow Control) - TCs 9-11
 *    - TC-9: Send buffer full
 *    - TC-10: Receive buffer full
 *    - TC-11: Backpressure resolved
 *
 * ⚪ Task 3.3: Implement CAT-4 (Reconnection) - TCs 12-14
 *    - TC-12: State during reconnection
 *    - TC-13: State after reconnection success
 *    - TC-14: State after reconnection failure
 *
 * ⚪ Task 3.4: Implement CAT-5 remaining (TCs 17-18)
 *    - TC-17: Link state after graceful close
 *    - TC-18: Link state after abortive close
 *
 * MILESTONE 3: 80% test coverage complete
 *
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * PHASE 4: LOW-PRIORITY TEST CASES (Week 5) - Priority: LOW
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * ⚪ Task 4.1: Implement CAT-6 (Transparency) - TC-19
 *    - TC-19: State during TCP retransmit
 *
 * MILESTONE 4: 100% test coverage complete
 *
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * PHASE 5: INTEGRATION & DOCUMENTATION (Week 6) - Priority: HIGH
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * ⚪ Task 5.1: Run full test suite, verify all GREEN
 * ⚪ Task 5.2: Performance profiling (test execution time)
 * ⚪ Task 5.3: Update documentation
 *    - README_ArchDesign.md: Add TCP × Command state integration
 *    - UT_CommandState.h: Document TCP-specific testing approach
 * ⚪ Task 5.4: Code review and cleanup
 * ⚪ Task 5.5: Compare with UT_CommandFaultTCP.cxx, document relationship
 *
 * MILESTONE 5: Production-ready TCP state testing framework
 *
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * 📊 EFFORT ESTIMATION:
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * Phase 1: 5-8 hours   (Infrastructure)
 * Phase 2: 10-14 hours (8 critical test cases)
 * Phase 3: 10-15 hours (8 medium-priority test cases)
 * Phase 4: 2-3 hours   (2 low-priority test cases)
 * Phase 5: 3-5 hours   (Integration & docs)
 * ─────────────────────────────────────────
 * TOTAL:   30-45 hours (~1 week full-time)
 *
 * NOTE: Reduced from 39-55 hours by removing 7 duplicate tests
 *
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * 🎯 SUCCESS CRITERIA:
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * ✓ All 18 test cases implemented and GREEN
 * ✓ 100% coverage of TCP-specific state integration scenarios
 * ✓ Zero state correlation violations detected
 * ✓ Test execution time < 60 seconds (all tests)
 * ✓ No memory leaks (valgrind clean)
 * ✓ Documentation complete and accurate
 * ✓ Clear relationship with UT_CommandFaultTCP.cxx established
 *
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * 🔗 RELATED WORK:
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * • UT_CommandStateUS1.cxx - Individual command state (protocol-agnostic)
 * • UT_CommandStateUS2.cxx - Link command state (protocol-agnostic)
 * • UT_CommandStateUS4.cxx - Timeout and error state (protocol-agnostic)
 * • UT_CommandFaultTCP.cxx - TCP fault scenarios (fault focus, not state focus)
 * • UT_CommandTypicalTCP.cxx - TCP happy-path scenarios
 * • README_ArchDesign.md - State machine diagrams
 */
//======>END OF IMPLEMENTATION ROADMAP============================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF DESIGN NOTES & DECISION LOG=====================================================
/**
 * 🗒️ DESIGN DECISIONS AND RATIONALE
 *
 * ─────────────────────────────────────────────────────────────────────────────────────────────
 * DECISION 1: Why separate UT_CommandStateTCP.cxx from UT_CommandStateUS1-5.cxx?
 * ─────────────────────────────────────────────────────────────────────────────────────────────
 * RATIONALE:
 *  • US1-5 test protocol-agnostic state machine behavior (state transitions, isolation, etc.)
 *  • TCP-specific scenarios require network simulation, connection lifecycle, TCP errors
 *  • Mixing TCP-specific and generic tests would make US1-5 unnecessarily complex
 *  • Separation of concerns: State machine logic vs Transport protocol integration
 *
 * BENEFITS:
 *  ✓ US1-5 remain clean, focused, protocol-independent
 *  ✓ TCP testing can use heavy infrastructure (socket simulation) without affecting US1-5
 *  ✓ Other protocols (FIFO) can follow same pattern with UT_CommandStateFIFO.cxx
 *
 * ─────────────────────────────────────────────────────────────────────────────────────────────
 * DECISION 2: Why 18 test cases (reduced from 25) organized into 7 categories?
 * ─────────────────────────────────────────────────────────────────────────────────────────────
 * RATIONALE:
 *  • TCP connection has distinct lifecycle phases (establish, active, loss, recovery, close)
 *  • Each phase has unique TCP-specific state implications
 *  • 18 TCs provide TCP-specific coverage, avoiding duplication with US-4
 *  • 7 categories map to TCP protocol concerns (not generic error handling)
 *
 * COVERAGE ANALYSIS (UPDATED):
 *  • CAT-1 (5 TCs): Connection establishment - TCP handshake state behavior
 *  • CAT-2 (3 TCs): Connection loss - TCP-specific errors (ECONNRESET, EPIPE) + link state
 *  • CAT-3 (3 TCs): Flow control - TCP buffer management and backpressure
 *  • CAT-4 (3 TCs): Reconnection - TCP connection recovery patterns
 *  • CAT-5 (4 TCs): Shutdown - TCP FIN vs RST behavior
 *  • CAT-6 (1 TC): Transparency - TCP retransmit doesn't affect command state
 *  • CAT-7 (1 TC): Error mapping - TCP errno → IOC_Result_T
 *
 * REMOVED (Duplicate US-4):
 *  ✗ Generic timeout detection (US-4 AC-1 covers this)
 *  ✗ Generic error propagation (US-4 AC-3 covers this)
 *  ✗ Multiple command failure (US-4 AC-4 covers this)
 *  ✗ Generic state correlation (US-4 covers this)
 *  ✗ Timeout hierarchy (US-4 AC-1 covers this)
 *
 * ─────────────────────────────────────────────────────────────────────────────────────────────
 * DECISION 3: Why TcpConnectionSimulator and TcpCommandStateTracker helper classes?
 * ─────────────────────────────────────────────────────────────────────────────────────────────
 * RATIONALE:
 *  • TCP state manipulation (RST, graceful close, buffer full) requires low-level control
 *  • Raw socket programming in each test case = code duplication and error-prone
 *  • Simulator provides clean API: forceReset(), gracefulClose(), etc.
 *  • Tracker enables automated correlation verification (reduces manual checking)
 *
 * DESIGN:
 *  • TcpConnectionSimulator: Focused on TCP connection control
 *  • TcpCommandStateTracker: Focused on state monitoring and correlation
 *  • Separation of concerns: Control vs Observation
 *
 * ─────────────────────────────────────────────────────────────────────────────────────────────
 * DECISION 4: Relationship with UT_CommandFaultTCP.cxx and UT_CommandStateUS4.cxx?
 * ─────────────────────────────────────────────────────────────────────────────────────────────
 * THREE-WAY COMPARISON:
 *  ┌────────────────────┬────────────────────────┬───────────────────────┬─────────────────────────┐
 *  │ Aspect             │ UT_CommandFaultTCP     │ UT_CommandStateUS4    │ UT_CommandStateTCP      │
 *  ├────────────────────┼────────────────────────┼───────────────────────┼─────────────────────────┤
 *  │ Primary Focus      │ FAULT injection        │ GENERIC timeout/error │ TCP-SPECIFIC state      │
 *  │ Test Goal          │ "Fails gracefully?"    │ "Timeout detected?"   │ "TCP state correct?"    │
 *  │ Protocol Scope     │ TCP only               │ Protocol-agnostic     │ TCP-specific            │
 *  │ Error Types        │ Network faults         │ Generic timeouts      │ TCP errno (ECONNRESET)  │
 *  │ State Tracking     │ Final state only       │ Timeout/error states  │ Full TCP state history  │
 *  │ Timeout Testing    │ Detection only         │ Comprehensive         │ TCP-specific (retrans.) │
 *  │ Flow Control       │ Not tested             │ Not tested            │ TCP buffer management   │
 *  │ Connection Lifecycle│ Fault scenarios       │ Not tested            │ Full TCP lifecycle      │
 *  └────────────────────┴────────────────────────┴───────────────────────┴─────────────────────────┘
 *
 * OVERLAP RESOLUTION:
 *  Initially had overlap with US-4 (timeout/error), removed 7 duplicate tests:
 *  ✗ Removed: Generic timeout (TC-8), multiple failures (TC-9), timeout hierarchy (TC-22/23)
 *  ✗ Removed: Generic error propagation (TC-24 parts), state correlation (TC-25)
 *  ✓ Kept: TCP-specific scenarios only
 *
 * COMPLEMENTARY RELATIONSHIP:
 *  • UT_CommandFaultTCP: "System survives TCP failure" (reliability testing)
 *  • UT_CommandStateUS4: "Timeout/error detected correctly" (protocol-agnostic)
 *  • UT_CommandStateTCP: "TCP state reported correctly" (TCP-specific observability)
 *  • Together: Complete command testing (Fault + Generic Error + TCP State)
 *
 * ─────────────────────────────────────────────────────────────────────────────────────────────
 * DECISION 5: Port allocation strategy (22080-22099, reduced from 22080-22104)?
 * ─────────────────────────────────────────────────────────────────────────────────────────────
 * RATIONALE:
 *  • Avoid conflict with UT_CommandFaultTCP (21080-21099) and Typical tests (20xxx)
 *  • 22xxx range clearly indicates "State testing" vs other categories
 *  • Sequential allocation makes tracking easier during debugging
 *  • Each test case gets dedicated port (prevents cross-test interference)
 *
 * ALLOCATION (UPDATED):
 *  • 22080-22084: CAT-1 (Connection Establishment) - 5 TCs
 *  • 22085-22087: CAT-2 (Connection Loss) - 3 TCs
 *  • 22088-22090: CAT-3 (Flow Control) - 3 TCs
 *  • 22091-22093: CAT-4 (Reconnection) - 3 TCs
 *  • 22094-22097: CAT-5 (Shutdown) - 4 TCs
 *  • 22098: CAT-6 (Transparency) - 1 TC
 *  • 22099: CAT-7 (Error Mapping) - 1 TC
 *  • Total: 18 TCs (22080-22099, 20 ports allocated for future expansion)
 *
 * ─────────────────────────────────────────────────────────────────────────────────────────────
 * DECISION 6: Why implement StateSnapshot history tracking?
 * ─────────────────────────────────────────────────────────────────────────────────────────────
 * RATIONALE:
 *  • State testing requires verifying transitions, not just final state
 *  • Need to capture: "Did command go through correct state sequence?"
 *  • Debugging: State history reveals timing issues and race conditions
 *  • Correlation: Can validate TCP state ⟺ Command state ⟺ Link state consistency
 *
 * IMPLEMENTATION:
 *  • Lightweight snapshot structure (timestamp + states)
 *  • Vector-based history (dynamic growth)
 *  • verifyStateCorrelation() automates validation
 *  • printHistory() aids debugging
 *
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * 📝 OPEN QUESTIONS FOR FUTURE CONSIDERATION:
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * Q1: Should we test IPv6 TCP state behavior separately?
 *     Current: IPv4 only (127.0.0.1)
 *     Future: Add IPv6 variant if needed (low priority)
 *
 * Q2: Should we test TCP keepalive impact on command state?
 *     Current: Not covered
 *     Future: May need test case for long-lived commands with keepalive
 *
 * Q3: Should we test TCP_USER_TIMEOUT socket option?
 *     Current: Not covered
 *     Future: May affect command timeout behavior
 *
 * Q4: Should we simulate network partition (vs connection loss)?
 *     Current: Connection loss via RST/FIN
 *     Future: True partition (packets dropped, no RST) may reveal different behavior
 *
 * Q5: Should we test TCP Fast Open impact on command state?
 *     Current: Standard TCP handshake only
 *     Future: TFO may change connection establishment state transitions
 */
//======>END OF DESIGN NOTES & DECISION LOG=======================================================
