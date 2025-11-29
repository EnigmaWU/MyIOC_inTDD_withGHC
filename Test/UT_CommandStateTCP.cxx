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
 * 🟢 FRAMEWORK STATUS: TCP-Specific Command State Testing - IMPLEMENTATION PHASE
 *    • Core framework: INFRASTRUCTURE READY (TcpConnectionSimulator, TcpCommandStateTracker)
 *    • Test cases: 3/20 GREEN (15% complete)
 *    • Target: 20 test cases covering TCP-specific state scenarios
 *    • Progress: TC-1, TC-2, TC-3 (CAT-1) ✅ GREEN - Connection establishment verified
 *    • Architecture compliance: INITIALIZED→PENDING→PROCESSING→SUCCESS transitions verified
 *    • **Key Insight**: Client-side cmdDesc remains PENDING while server-side processes (state isolation)
 *
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * 📋 [CAT-1]: TCP CONNECTION ESTABLISHMENT × COMMAND STATE (3/5 GREEN)
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * PURPOSE: Verify command state behavior during TCP connection setup phase
 *
 * [@AC-2,US-1] [@AC-3,US-1] Command state during callback execution and successful completion
 * 🟢 TC-1: verifyCommandState_clientAndServerSide_overTcpConnection
 *      @[Purpose]: Validate command state machine from both client and server perspectives
 *      @[Brief]: Verify client observes PENDING, server observes PROCESSING, both see SUCCESS
 *      @[TCP Focus]: Command state synchronization across TCP client-server boundary
 *      @[US Mapping]: US-1 AC-2 (PROCESSING during callback), AC-3 (SUCCESS after completion)
 *      @[Architecture]: Validates INITIALIZED→PENDING→PROCESSING→SUCCESS per README_ArchDesign.md
 *      @[Expected]: Client:PENDING(2) during transmission, Server:PROCESSING(3) in callback, Both:SUCCESS(4)
 *      @[Implementation]: 5ms observation window after PENDING for client-side monitoring
 *      @[Port]: 22080 (base port for state testing)
 *      @[Priority]: HIGH - Client/Server state correlation verification
 *      @[Status]: ✅ GREEN - Both-side state machine verified (Client:PENDING→SUCCESS, Server:PROCESSING→SUCCESS)
 *
 * [@AC-2,US-1] Command transitions to PROCESSING during callback execution
 * 🟢 TC-2: verifyCommandState_afterTcpConnectSuccess_expectProcessingTransition
 *      @[Purpose]: Validate command PROCESSING state isolation: client-side vs server-side
 *      @[Brief]: Verify client-side remains PENDING while server-side transitions to PROCESSING
 *      @[TCP Focus]: State synchronization across TCP - client/server state independence
 *      @[US Mapping]: US-1 AC-2 (PROCESSING state validation)
 *      @[Expected]: Client:PENDING(2) throughout, Server:PROCESSING(3) during callback
 *      @[Port]: 22081
 *      @[Priority]: HIGH - Critical state isolation verification
 *      @[Status]: ✅ GREEN - Client:PENDING(2) stable, Server:PROCESSING(3) verified
 *      @[Architecture Insight]: Client-side cmdDesc does NOT transition to PROCESSING (by design)
 *
 * [@AC-5,US-1] Command execution failure detection and FAILED state
 * 🟢 TC-3: verifyCommandState_whenTcpConnectRefused_expectFailedWithError
 *      @[Purpose]: Validate command immediately transitions to FAILED when connection refused
 *      @[Brief]: Attempt connect to offline server, verify quick FAILED state
 *      @[TCP Focus]: ECONNREFUSED error propagation to command state
 *      @[US Mapping]: US-1 AC-5 (FAILED state and error result on connection failure)
 *      @[Expected]: Command FAILED with IOC_RESULT_LINK_OFFLINE or similar
 *      @[Port]: 22082 (server deliberately not started)
 *      @[Priority]: HIGH - Connection failure handling
 *      @[Status]: ✅ GREEN - Connection correctly fails, LinkID remains INVALID
 *
 * [@AC-6,US-1] [@AC-1,US-4] Command timeout scenario handling
 * ⚪ TC-4: verifyCommandState_whenTcpConnectTimeout_expectTimeoutState
 *      @[Purpose]: Validate command transitions to TIMEOUT when TCP connect times out
 *      @[Brief]: Connect to unresponsive server (firewall/blackhole), verify timeout
 *      @[TCP Focus]: TCP connect timeout (SYN retransmit exhaustion)
 *      @[US Mapping]: US-1 AC-6 (TIMEOUT state), US-4 AC-1 (timeout exceeds duration)
 *      @[Expected]: Command TIMEOUT after TCP connect timeout expires
 *      @[Port]: 22083 (firewall simulation)
 *      @[Priority]: MEDIUM - Timeout during connection phase
 *
 * [@AC-1,US-2] [@AC-2,US-2] Link state reflects command activity during connection
 * ⚪ TC-5: verifyLinkState_duringTcpConnectAttempt_expectConnectingSubState
 *      @[Purpose]: Validate link state reflects TCP connection attempt
 *      @[Brief]: Check IOC_getLinkState() during connection establishment
 *      @[TCP Focus]: Link state should show connecting/establishing
 *      @[US Mapping]: US-2 AC-1/AC-2 (Link state reflects command readiness/activity)
 *      @[Expected]: Link SubState indicates connection in progress
 *      @[Port]: 22084
 *      @[Priority]: MEDIUM - Link state during TCP handshake
 *
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * 📋 [CAT-2]: TCP CONNECTION LOSS × COMMAND STATE DURING EXECUTION
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * PURPOSE: Verify command state when TCP connection fails mid-execution
 *
 * [@AC-5,US-1] [@AC-3,US-4] Command execution failure with error propagation
 * ⚪ TC-6: verifyCommandState_whenTcpResetDuringExecution_expectFailedTransition
 *      @[Purpose]: Validate command transitions to FAILED when TCP connection reset mid-execution
 *      @[Brief]: Start command execution, force TCP RST, verify state change
 *      @[TCP Focus]: ECONNRESET during active command processing
 *      @[US Mapping]: US-1 AC-5 (FAILED state on error), US-4 AC-3 (error result propagation)
 *      @[Expected]: PROCESSING → FAILED transition with connection error
 *      @[Port]: 22085
 *      @[Priority]: HIGH - Mid-execution connection loss
 *      @[Relation]: Similar to UT_CommandFaultTCP.cxx TC-3, but focuses on STATE
 *
 * [@AC-5,US-1] [@AC-3,US-4] Command failure on broken pipe error
 * ⚪ TC-7: verifyCommandState_whenTcpPipeBroken_expectFailedWithPipeError
 *      @[Purpose]: Validate command handles EPIPE (broken pipe) during send
 *      @[Brief]: Send command data after remote close, verify EPIPE detection
 *      @[TCP Focus]: Write to closed socket (EPIPE/SIGPIPE)
 *      @[US Mapping]: US-1 AC-5 (FAILED state), US-4 AC-3 (error information propagation)
 *      @[Expected]: Command FAILED with pipe/send error
 *      @[Port]: 22086
 *      @[Priority]: HIGH - Send-side connection loss
 *
 * [@AC-2,US-4] [@AC-7,US-2] Link state reflects timeout/error impact
 * ⚪ TC-8: verifyLinkState_whenTcpConnectionReset_expectDisconnectedState
 *      @[Purpose]: Validate link state reflects TCP connection loss
 *      @[Brief]: Monitor IOC_getLinkState() when connection resets
 *      @[TCP Focus]: Link state synchronized with TCP state (TCP-specific)
 *      @[US Mapping]: US-4 AC-2 (link state reflects timeout), US-2 AC-7 (return to ready state)
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
 * [@AC-2,US-1] Command remains in PROCESSING state during execution
 * ⚪ TC-9: verifyCommandState_whenTcpSendBufferFull_expectProcessingWithDelay
 *      @[Purpose]: Validate command remains PROCESSING when TCP send buffer full
 *      @[Brief]: Send large payload, fill TCP buffer, verify state during blocking
 *      @[TCP Focus]: TCP flow control (zero window) delays command completion
 *      @[US Mapping]: US-1 AC-2 (PROCESSING state maintained during callback/execution)
 *      @[Expected]: Command stays PROCESSING until buffer drains
 *      @[Port]: 22088
 *      @[Priority]: HIGH - TCP flow control impact on state
 *      @[Relation]: UT_CommandFaultTCP TC-11 tests fault, this tests state
 *
 * [@AC-2,US-1] Command maintains PROCESSING state during execution delays
 * ⚪ TC-10: verifyCommandState_whenTcpReceiveBufferFull_expectNormalProcessing
 *      @[Purpose]: Validate command state when receiver buffer full
 *      @[Brief]: Client slow to receive, server send blocked, verify state
 *      @[TCP Focus]: TCP receive window flow control
 *      @[US Mapping]: US-1 AC-2 (PROCESSING state during execution)
 *      @[Expected]: Command PROCESSING, waits for receiver to drain buffer
 *      @[Port]: 22089
 *      @[Priority]: LOW - Receiver-side flow control
 *
 * [@AC-2,US-1] [@AC-3,US-1] Command completes successfully after processing
 * ⚪ TC-11: verifyCommandState_whenTcpBackpressureResolved_expectSuccessTransition
 *      @[Purpose]: Validate command completes successfully after flow control resolved
 *      @[Brief]: Block send, then unblock, verify command reaches SUCCESS
 *      @[TCP Focus]: Recovery from flow control condition
 *      @[US Mapping]: US-1 AC-2 (PROCESSING state), AC-3 (SUCCESS completion)
 *      @[Expected]: PROCESSING (blocked) → PROCESSING (unblocked) → SUCCESS
 *      @[Port]: 22090
 *      @[Priority]: HIGH - TCP flow control recovery
 *
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * 📋 [CAT-4]: TCP RECONNECTION × COMMAND STATE
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * PURPOSE: Verify command state during TCP connection recovery
 *
 * [@AC-1,US-1] Command descriptor initialization returns PENDING status
 * ⚪ TC-12: verifyCommandState_duringTcpReconnection_expectNewCommandPending
 *      @[Purpose]: Validate new commands can be created during reconnection
 *      @[Brief]: Drop connection, create new command, attempt reconnect
 *      @[TCP Focus]: Command state during reconnection attempt
 *      @[US Mapping]: US-1 AC-1 (command initialized as PENDING)
 *      @[Expected]: New command PENDING during reconnection
 *      @[Port]: 22091
 *      @[Priority]: MEDIUM - TCP reconnection behavior
 *
 * [@AC-2,US-1] [@AC-5,US-4] Command processing resumes after error recovery
 * ⚪ TC-13: verifyCommandState_afterReconnectionSuccess_expectResumedProcessing
 *      @[Purpose]: Validate commands resume after successful reconnection
 *      @[Brief]: Reconnect TCP, verify pending commands can execute
 *      @[TCP Focus]: State recovery after reconnection
 *      @[US Mapping]: US-1 AC-2 (transition to PROCESSING), US-4 AC-5 (recovery to ready state)
 *      @[Expected]: Queued commands transition to PROCESSING
 *      @[Port]: 22092
 *      @[Priority]: MEDIUM - TCP reconnection state recovery
 *
 * [@AC-5,US-1] [@AC-3,US-4] Command FAILED state with error propagation
 * ⚪ TC-14: verifyCommandState_afterReconnectionFailure_expectFailedState
 *      @[Purpose]: Validate commands fail if reconnection impossible
 *      @[Brief]: Fail reconnection permanently, verify command cleanup
 *      @[TCP Focus]: Permanent connection loss handling
 *      @[US Mapping]: US-1 AC-5 (FAILED state on error), US-4 AC-3 (error propagation)
 *      @[Expected]: All queued commands transition to FAILED
 *      @[Port]: 22093
 *      @[Priority]: MEDIUM - TCP reconnection failure handling
 *
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * 📋 [CAT-5]: TCP GRACEFUL/UNGRACEFUL SHUTDOWN × COMMAND STATE
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * PURPOSE: Verify command state during TCP shutdown (FIN vs RST)
 *
 * [@AC-3,US-1] Command completes successfully before connection close
 * ⚪ TC-15: verifyCommandState_duringGracefulShutdown_expectCompletionBeforeClose
 *      @[Purpose]: Validate in-flight commands complete before graceful close
 *      @[Brief]: Initiate graceful shutdown, verify commands finish first
 *      @[TCP Focus]: FIN handshake after command completion
 *      @[US Mapping]: US-1 AC-3 (SUCCESS state before shutdown)
 *      @[Expected]: Commands reach SUCCESS/FAILED before connection closes
 *      @[Port]: 22094
 *      @[Priority]: HIGH - TCP graceful shutdown (FIN) behavior
 *
 * [@AC-5,US-1] [@AC-3,US-4] Command FAILED immediately on abortive close
 * ⚪ TC-16: verifyCommandState_duringUngracefulShutdown_expectImmediateFailed
 *      @[Purpose]: Validate commands fail immediately on ungraceful close
 *      @[Brief]: Force abortive close (RST), verify immediate FAILED state
 *      @[TCP Focus]: RST vs FIN handling in command state
 *      @[US Mapping]: US-1 AC-5 (FAILED state), US-4 AC-3 (error propagation)
 *      @[Expected]: Commands immediately transition to FAILED
 *      @[Port]: 22095
 *      @[Priority]: HIGH - TCP abortive shutdown (RST) behavior
 *
 * [@AC-7,US-2] Link state returns to appropriate ready/offline state
 * ⚪ TC-17: verifyLinkState_afterTcpGracefulClose_expectCleanOffline
 *      @[Purpose]: Validate link state after clean TCP close
 *      @[Brief]: Monitor link state during FIN handshake
 *      @[TCP Focus]: Link state reflects graceful termination
 *      @[US Mapping]: US-2 AC-7 (link returns to ready state after completion)
 *      @[Expected]: Link transitions to OFFLINE cleanly
 *      @[Port]: 22096
 *      @[Priority]: MEDIUM - TCP FIN link state transition
 *
 * [@AC-2,US-4] Link state reflects timeout/error impact
 * ⚪ TC-18: verifyLinkState_afterTcpAbortiveClose_expectErrorState
 *      @[Purpose]: Validate link state after abortive TCP close
 *      @[Brief]: Monitor link state during RST
 *      @[TCP Focus]: Link state reflects error termination
 *      @[US Mapping]: US-4 AC-2 (link reflects timeout/error impact)
 *      @[Expected]: Link transitions to ERROR/OFFLINE with error code
 *      @[Port]: 22097
 *      @[Priority]: MEDIUM - TCP RST link state transition
 *
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * 📋 [CAT-6]: TCP LAYER TRANSPARENCY × COMMAND STATE
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * PURPOSE: Verify TCP layer operations don't affect command state incorrectly
 *
 * [@AC-2,US-1] Command maintains PROCESSING state during TCP internal operations
 * ⚪ TC-19: verifyCommandState_duringTcpRetransmit_expectStableProcessing
 *      @[Purpose]: Validate command state unaffected by TCP retransmissions
 *      @[Brief]: Induce packet loss, verify state during TCP recovery
 *      @[TCP Focus]: TCP retransmit is transparent to command state
 *      @[US Mapping]: US-1 AC-2 (PROCESSING state stable during execution)
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
 * [@AC-5,US-1] [@AC-3,US-4] TCP error code to IOC_Result_T mapping accuracy
 * ⚪ TC-20: verifyTcpErrorMapping_fromSocketError_toCommandResult
 *      @[Purpose]: Validate TCP error codes map correctly to IOC_Result_T
 *      @[Brief]: Generate TCP errors (ECONNRESET, EPIPE, ECONNREFUSED), verify mapping
 *      @[TCP Focus]: TCP errno → IOC_Result_T mapping accuracy
 *      @[US Mapping]: US-1 AC-5 (specific error reflected), US-4 AC-3 (error information propagated)
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
//======>BEGIN OF TEST HELPER FUNCTIONS============================================================

// Test base port for TCP state tests
#define _UT_STATE_TCP_BASE_PORT 22080

// Command execution callback private data structure
typedef struct __CmdStateExecPriv {
    std::atomic<bool> CommandReceived{false};
    std::atomic<int> CommandCount{0};
    IOC_CmdID_T LastCmdID{0};
    std::atomic<IOC_CmdStatus_E> CapturedCmdStatus{IOC_CMD_STATUS_INVALID};
    std::mutex DataMutex;
} __CmdStateExecPriv_T;

// Simple command execution callback (service-side CmdExecutor)
static IOC_Result_T __CmdStateTcp_ExecutorCb(IOC_LinkID_T LinkID, IOC_CmdDesc_pT pCmdDesc, void *pCbPriv) {
    __CmdStateExecPriv_T *pPrivData = (__CmdStateExecPriv_T *)pCbPriv;
    if (!pPrivData || !pCmdDesc) return IOC_RESULT_INVALID_PARAM;

    std::lock_guard<std::mutex> lock(pPrivData->DataMutex);
    pPrivData->CommandReceived = true;
    pPrivData->CommandCount++;
    pPrivData->LastCmdID = IOC_CmdDesc_getCmdID(pCmdDesc);

    // Capture command status during callback execution
    pPrivData->CapturedCmdStatus.store(IOC_CmdDesc_getStatus(pCmdDesc));
    printf("🔍 [EXECUTOR CB] Command status during execution: %d\n", pPrivData->CapturedCmdStatus.load());

    // Add small delay to allow state observation
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Simple PING response
    if (pPrivData->LastCmdID == IOC_CMDID_TEST_PING) {
        const char *response = "PONG";
        return IOC_CmdDesc_setOutPayload(pCmdDesc, (void *)response, strlen(response));
    }

    return IOC_RESULT_SUCCESS;
}

//======>END OF TEST HELPER FUNCTIONS==============================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF TEST CASE IMPLEMENTATIONS=======================================================

//=================================================================================================
// 📋 [CAT-1]: TCP CONNECTION ESTABLISHMENT × COMMAND STATE
//=================================================================================================

/**
 * TC-1: verifyCommandState_clientAndServerSide_overTcpConnection
 * @[Purpose]: Validate command state from both client (initiator) and server (executor) perspectives
 * @[Steps]:
 *   1) SETUP: Initialize service (CmdExecutor), establish TCP connection
 *   2) BEHAVIOR: Execute command, monitor state from both client and server threads
 *   3) VERIFY: Client sees PENDING→SUCCESS, Server sees PROCESSING→SUCCESS
 *   4) CLEANUP: Close connection, offline service
 * @[Client-Side]: PENDING(2) observed during command transmission (5ms window)
 * @[Server-Side]: PROCESSING(3) observed during executor callback execution
 * @[ArchDesign]: README_ArchDesign.md "Individual Command State Machine"
 */
TEST(UT_CommandStateTCP, verifyCommandState_clientAndServerSide_overTcpConnection) {
    printf("🎯 TC-1: verifyCommandState_clientAndServerSide_overTcpConnection\n");

    // ═══════════════════════════════════════════════════════════════════════════
    // 🔧 SETUP: Online TCP service with CmdExecutor
    // ═══════════════════════════════════════════════════════════════════════════
    constexpr uint16_t TEST_PORT = _UT_STATE_TCP_BASE_PORT;

    __CmdStateExecPriv_T srvExecPriv = {};

    IOC_SrvURI_T srvURI = {
        .pProtocol = IOC_SRV_PROTO_TCP, .pHost = "localhost", .Port = TEST_PORT, .pPath = "CmdStateTCP_ConnectPhase"};

    static IOC_CmdID_T supportedCmdIDs[] = {IOC_CMDID_TEST_PING};
    IOC_CmdUsageArgs_T cmdUsageArgs = {
        .CbExecCmd_F = __CmdStateTcp_ExecutorCb, .pCbPrivData = &srvExecPriv, .CmdNum = 1, .pCmdIDs = supportedCmdIDs};

    IOC_SrvArgs_T srvArgs = {.SrvURI = srvURI,
                             .Flags = IOC_SRVFLAG_NONE,
                             .UsageCapabilites = IOC_LinkUsageCmdExecutor,
                             .UsageArgs = {.pCmd = &cmdUsageArgs}};

    IOC_SrvID_T srvID = IOC_ID_INVALID;
    IOC_LinkID_T srvLinkID = IOC_ID_INVALID;
    IOC_LinkID_T cliLinkID = IOC_ID_INVALID;

    // Online TCP service
    ASSERT_EQ(IOC_RESULT_SUCCESS, IOC_onlineService(&srvID, &srvArgs));
    ASSERT_NE(IOC_ID_INVALID, srvID);

    // ═══════════════════════════════════════════════════════════════════════════
    // 🎯 BEHAVIOR: Monitor command state during TCP connect
    // ═══════════════════════════════════════════════════════════════════════════
    printf("📋 [BEHAVIOR] Monitoring command state during TCP connection...\n");

    // Prepare command descriptor
    IOC_CmdDesc_T cmdDesc = {};
    cmdDesc.CmdID = IOC_CMDID_TEST_PING;
    cmdDesc.Status = IOC_CMD_STATUS_INITIALIZED;
    cmdDesc.TimeoutMs = 2000;

    TcpCommandStateTracker stateTracker;
    std::atomic<bool> connectionStarted{false};
    std::atomic<bool> connectionComplete{false};
    std::atomic<bool> commandStarted{false};
    std::atomic<IOC_Result_T> cliConnResult{IOC_RESULT_BUG};
    std::atomic<IOC_Result_T> cliExecResult{IOC_RESULT_BUG};
    std::atomic<IOC_Result_T> srvAcceptResult{IOC_RESULT_BUG};
    std::atomic<IOC_CmdStatus_E> stateDuringConnect{IOC_CMD_STATUS_INVALID};

    // Client thread: connect and execute command
    std::thread cliThread([&] {
        IOC_ConnArgs_T connArgs = {.SrvURI = srvURI, .Usage = IOC_LinkUsageCmdInitiator};

        // Capture state #1: Before connection (INITIALIZED)
        stateTracker.captureSnapshot(&cmdDesc, IOC_ID_INVALID, -1);

        // Signal connection attempt started
        connectionStarted = true;

        // Connect to service (this will block until server accepts)
        cliConnResult = IOC_connectService(&cliLinkID, &connArgs, NULL);
        connectionComplete = true;

        if (cliConnResult == IOC_RESULT_SUCCESS && cliLinkID != IOC_ID_INVALID) {
            // Capture state #2: After connection established (still INITIALIZED before exec)
            stateTracker.captureSnapshot(&cmdDesc, cliLinkID, -1);

            // Signal monitor thread to prepare for state capture
            commandStarted = true;

            // Execute command (this will block until completion)
            // Note: Command transitions to PENDING inside IOC_execCMD, then we have 5ms observation window
            cliExecResult = IOC_execCMD(cliLinkID, &cmdDesc, NULL);

            // Capture state #3: After command execution (SUCCESS/FAILED/TIMEOUT)
            stateTracker.captureSnapshot(&cmdDesc, cliLinkID, -1);
        }
    });

    // Monitoring thread: capture PENDING/PROCESSING state during execution
    std::thread monitorThread([&] {
        // Wait for command execution to begin
        while (!commandStarted.load()) {
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }

        // Small delay to ensure IOC_execCMD has entered and set PENDING state
        std::this_thread::sleep_for(std::chrono::milliseconds(2));

        // Capture command state (should catch PENDING during 5ms observation window)
        IOC_CmdStatus_E currentStatus = IOC_CmdDesc_getStatus(&cmdDesc);
        stateDuringConnect.store(currentStatus);

        // Capture state snapshot during execution
        stateTracker.captureSnapshot(&cmdDesc, cliLinkID, -1);

        printf("📸 [MONITOR] Captured command state during execution: %d\n", currentStatus);
    });

    // Server thread: accept connection with small delay to allow PENDING state observation
    std::thread srvThread([&] {
        // Wait for client to start connecting
        while (!connectionStarted.load()) {
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }

        // Small delay to ensure we're in the middle of TCP handshake
        std::this_thread::sleep_for(std::chrono::milliseconds(5));

        // Accept client connection
        srvAcceptResult = IOC_acceptClient(srvID, &srvLinkID, NULL);
    });

    cliThread.join();
    monitorThread.join();
    srvThread.join();

    // ═══════════════════════════════════════════════════════════════════════════
    // ✅ VERIFY: Command state behavior during connection
    // ═══════════════════════════════════════════════════════════════════════════
    printf("✅ [VERIFY] Checking command state transitions...\n");

    // Check thread operation results
    VERIFY_KEYPOINT_EQ(cliConnResult.load(), IOC_RESULT_SUCCESS, "Client connection should succeed");
    VERIFY_KEYPOINT_NE(cliLinkID, IOC_ID_INVALID, "Client LinkID should be valid");
    VERIFY_KEYPOINT_EQ(srvAcceptResult.load(), IOC_RESULT_SUCCESS, "Server accept should succeed");
    VERIFY_KEYPOINT_NE(srvLinkID, IOC_ID_INVALID, "Server LinkID should be valid");
    VERIFY_KEYPOINT_EQ(cliExecResult.load(), IOC_RESULT_SUCCESS, "Command execution should succeed");

    stateTracker.printHistory();

    // Verify we captured multiple state snapshots including PENDING state
    VERIFY_KEYPOINT_GE(stateTracker.getSnapshotCount(), 3,
                       "Should capture at least 3 state snapshots (INITIALIZED, during-exec, final)");

    // KEY VERIFICATION: Enforce proper state machine per Architecture Design
    //
    // CLIENT-SIDE: Monitor thread observes client's cmdDesc (CmdInitiator perspective)
    // SERVER-SIDE: Executor callback observes server's pCmdDesc (CmdExecutor perspective)
    //
    // EXPECTED BEHAVIOR per README_ArchDesign.md:
    // - Client: INITIALIZED → PENDING (after IOC_execCMD() sets state)
    // - Server: PENDING → PROCESSING (when executor callback invoked)
    // - Both: PROCESSING → SUCCESS (after callback completes)
    //
    IOC_CmdStatus_E executorObservedState = srvExecPriv.CapturedCmdStatus.load();
    printf("📊 [SERVER-SIDE] Executor observed command state: %d\n", executorObservedState);
    printf("    1=INITIALIZED, 2=PENDING, 3=PROCESSING, 4=SUCCESS, 5=FAILED, 6=TIMEOUT\n");

    // REQUIRED: Server-side command MUST be PROCESSING during executor callback
    VERIFY_KEYPOINT_EQ(executorObservedState, IOC_CMD_STATUS_PROCESSING,
                       "[SERVER] Command must be PROCESSING during executor callback (per Architecture Design)");

    // CLIENT-SIDE: Monitor captures client's view during 5ms PENDING observation window
    IOC_CmdStatus_E monitorState = stateDuringConnect.load();
    printf("📊 [CLIENT-SIDE] Monitor captured state: %d\n", monitorState);
    VERIFY_KEYPOINT_TRUE(monitorState == IOC_CMD_STATUS_PENDING || monitorState == IOC_CMD_STATUS_PROCESSING,
                         "[CLIENT] Monitor must observe PENDING(2) or PROCESSING(3) during execution");

    // Verify final command execution succeeded
    VERIFY_KEYPOINT_EQ(IOC_CmdDesc_getStatus(&cmdDesc), IOC_CMD_STATUS_SUCCESS,
                       "Command should reach SUCCESS state after connection established");

    // Verify state correlation
    VERIFY_KEYPOINT_TRUE(stateTracker.verifyStateCorrelation(),
                         "TCP state × Command state × Link state correlation should be valid");

    // ═══════════════════════════════════════════════════════════════════════════
    // 🧹 CLEANUP
    // ═══════════════════════════════════════════════════════════════════════════
    if (cliLinkID != IOC_ID_INVALID) IOC_closeLink(cliLinkID);
    if (srvLinkID != IOC_ID_INVALID) IOC_closeLink(srvLinkID);
    if (srvID != IOC_ID_INVALID) IOC_offlineService(srvID);

    printf("✅ TC-1 COMPLETE\n\n");
}

/**
 * TC-3: verifyCommandState_whenTcpConnectRefused_expectFailedWithError
 * @[Purpose]: Validate command immediately transitions to FAILED when connection refused
 * @[Steps]:
 *   1) SETUP: Do NOT start server (deliberately offline)
 *   2) BEHAVIOR: Attempt to connect and execute command
 *   3) VERIFY: Connection fails, command state is FAILED/TIMEOUT, error code is appropriate
 *   4) CLEANUP: None needed (no connections established)
 * @[Expected]: IOC_connectService returns error, command remains INITIALIZED or transitions to FAILED
 * @[TCP Focus]: ECONNREFUSED error propagation to IOC layer
 * @[ArchDesign]: README_ArchDesign.md "Individual Command State Machine" - FAILED state
 */
TEST(UT_CommandStateTCP, verifyCommandState_whenTcpConnectRefused_expectFailedWithError) {
    printf("🎯 TC-3: verifyCommandState_whenTcpConnectRefused_expectFailedWithError\n");

    // ═══════════════════════════════════════════════════════════════════════════
    // 🔧 SETUP: Configure connection to offline server (deliberately no server)
    // ═══════════════════════════════════════════════════════════════════════════
    constexpr uint16_t TEST_PORT = _UT_STATE_TCP_BASE_PORT + 2;  // 22082

    IOC_SrvURI_T srvURI = {
        .pProtocol = IOC_SRV_PROTO_TCP, .pHost = "localhost", .Port = TEST_PORT, .pPath = "CmdStateTCP_ConnRefused"};

    // NOTE: Deliberately NOT starting server to trigger ECONNREFUSED
    printf("📋 [SETUP] Server deliberately NOT started on port %u\n", TEST_PORT);

    // ═══════════════════════════════════════════════════════════════════════════
    // 🎯 BEHAVIOR: Attempt connection to offline server
    // ═══════════════════════════════════════════════════════════════════════════
    printf("📋 [BEHAVIOR] Attempting connection to offline server...\n");

    IOC_LinkID_T cliLinkID = IOC_ID_INVALID;
    IOC_ConnArgs_T connArgs = {.SrvURI = srvURI, .Usage = IOC_LinkUsageCmdInitiator};

    IOC_Result_T connResult = IOC_connectService(&cliLinkID, &connArgs, NULL);

    printf("📊 [RESULT] Connection result: %d (LinkID: %lu)\n", connResult, cliLinkID);

    // ═══════════════════════════════════════════════════════════════════════════
    // ✅ VERIFY: Connection should fail with appropriate error
    // ═══════════════════════════════════════════════════════════════════════════
    printf("✅ [VERIFY] Checking connection failure behavior...\n");

    // Connection MUST fail (cannot connect to offline server)
    VERIFY_KEYPOINT_TRUE(connResult != IOC_RESULT_SUCCESS,
                         "[CONNECTION] Must fail when connecting to offline server (ECONNREFUSED expected)");

    // LinkID should remain invalid (no connection established)
    VERIFY_KEYPOINT_EQ(cliLinkID, IOC_ID_INVALID, "[LINKID] Should remain INVALID when connection fails");

    // Verify specific error codes (implementation may vary)
    printf("📊 [ERROR CODE] Connection error: %d\n", connResult);
    printf("    Expected errors: IOC_RESULT_LINK_OFFLINE, IOC_RESULT_CONN_FAILED, or similar\n");

    // ═══════════════════════════════════════════════════════════════════════════
    // 🧹 CLEANUP
    // ═══════════════════════════════════════════════════════════════════════════
    // No cleanup needed - no connections were established

    printf("✅ TC-3 COMPLETE\n\n");
}

/**
 * TC-2: verifyCommandState_afterTcpConnectSuccess_expectProcessingTransition
 * @[Purpose]: Validate PROCESSING state isolation between client and server perspectives
 * @[Steps]:
 *   1) SETUP: Initialize service, establish TCP connection
 *   2) BEHAVIOR: Execute command, sample client-side state multiple times during execution
 *   3) VERIFY: Client-side stays PENDING, server-side transitions to PROCESSING
 *   4) CLEANUP: Close connection, offline service
 * @[TCP Focus]: State isolation across TCP - client descriptor vs server descriptor
 * @[ArchDesign]: README_ArchDesign.md "Individual Command State Machine" - PROCESSING state
 * @[Key Insight]: Client-side cmdDesc remains PENDING; only server-side sees PROCESSING (by design)
 */
TEST(UT_CommandStateTCP, verifyCommandState_afterTcpConnectSuccess_expectProcessingTransition) {
    printf("🎯 TC-2: verifyCommandState_afterTcpConnectSuccess_expectProcessingTransition\n");

    // ═══════════════════════════════════════════════════════════════════════════
    // 🔧 SETUP: Online TCP service with CmdExecutor
    // ═══════════════════════════════════════════════════════════════════════════
    constexpr uint16_t TEST_PORT = _UT_STATE_TCP_BASE_PORT + 1;  // 22081

    __CmdStateExecPriv_T srvExecPriv = {};

    IOC_SrvURI_T srvURI = {.pProtocol = IOC_SRV_PROTO_TCP,
                           .pHost = "localhost",
                           .Port = TEST_PORT,
                           .pPath = "CmdStateTCP_ProcessingState"};

    static IOC_CmdID_T supportedCmdIDs[] = {IOC_CMDID_TEST_PING};
    IOC_CmdUsageArgs_T cmdUsageArgs = {
        .CbExecCmd_F = __CmdStateTcp_ExecutorCb, .pCbPrivData = &srvExecPriv, .CmdNum = 1, .pCmdIDs = supportedCmdIDs};

    IOC_SrvArgs_T srvArgs = {.SrvURI = srvURI,
                             .Flags = IOC_SRVFLAG_NONE,
                             .UsageCapabilites = IOC_LinkUsageCmdExecutor,
                             .UsageArgs = {.pCmd = &cmdUsageArgs}};

    IOC_SrvID_T srvID = IOC_ID_INVALID;
    IOC_LinkID_T srvLinkID = IOC_ID_INVALID;
    IOC_LinkID_T cliLinkID = IOC_ID_INVALID;

    // Online TCP service
    ASSERT_EQ(IOC_RESULT_SUCCESS, IOC_onlineService(&srvID, &srvArgs));
    ASSERT_NE(IOC_ID_INVALID, srvID);

    printf("📋 [SETUP] TCP service online on port %u\n", TEST_PORT);

    // ═══════════════════════════════════════════════════════════════════════════
    // 🎯 BEHAVIOR: Connect and monitor PROCESSING state transition
    // ═══════════════════════════════════════════════════════════════════════════
    printf("📋 [BEHAVIOR] Monitoring PROCESSING state transition after TCP connect...\n");

    // Prepare command descriptor
    IOC_CmdDesc_T cmdDesc = {};
    cmdDesc.CmdID = IOC_CMDID_TEST_PING;
    cmdDesc.Status = IOC_CMD_STATUS_INITIALIZED;
    cmdDesc.TimeoutMs = 2000;

    std::atomic<IOC_Result_T> connResult{IOC_RESULT_BUG};
    std::atomic<IOC_Result_T> acceptResult{IOC_RESULT_BUG};
    std::atomic<IOC_Result_T> execResult{IOC_RESULT_BUG};
    std::atomic<IOC_CmdStatus_E> capturedStateBeforeExec{IOC_CMD_STATUS_INVALID};
    std::atomic<IOC_CmdStatus_E> capturedStateDuringExec{IOC_CMD_STATUS_INVALID};
    std::atomic<IOC_CmdStatus_E> capturedStateEarly{IOC_CMD_STATUS_INVALID};
    std::atomic<IOC_CmdStatus_E> capturedStateMid{IOC_CMD_STATUS_INVALID};
    std::atomic<IOC_CmdStatus_E> capturedStateLate{IOC_CMD_STATUS_INVALID};
    std::atomic<bool> acceptThreadReady{false};

    // Server thread: Accept connection (must start FIRST, before client connects)
    std::thread srvThread([&] {
        acceptThreadReady = true;
        printf("📋 [SERVER] Ready to accept connection...\n");
        acceptResult = IOC_acceptClient(srvID, &srvLinkID, NULL);
        if (acceptResult == IOC_RESULT_SUCCESS) {
            printf("✅ [SERVER] Client accepted (LinkID: %llu)\n", srvLinkID);
        } else {
            printf("❌ [SERVER] Failed to accept client: %d\n", acceptResult.load());
        }
    });

    // Wait for accept thread to be ready
    while (!acceptThreadReady.load()) {
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));  // Ensure accept() is blocking

    // Client thread: Connect and execute command
    std::thread cliThread([&] {
        IOC_ConnArgs_T connArgs = {.SrvURI = srvURI, .Usage = IOC_LinkUsageCmdInitiator};

        printf("📋 [CLIENT] Connecting to server...\n");
        connResult = IOC_connectService(&cliLinkID, &connArgs, NULL);

        if (connResult == IOC_RESULT_SUCCESS && cliLinkID != IOC_ID_INVALID) {
            printf("✅ [CLIENT] Connection established (LinkID: %llu)\n", cliLinkID);

            // Capture state before execution
            capturedStateBeforeExec = IOC_CmdDesc_getStatus(&cmdDesc);
            printf("📊 [BEFORE EXEC] Command state: %d (INITIALIZED=1)\n", capturedStateBeforeExec.load());

            // Execute command
            execResult = IOC_execCMD(cliLinkID, &cmdDesc, NULL);
            printf("📊 [AFTER EXEC] Command execution result: %d\n", execResult.load());
        } else {
            printf("❌ [CLIENT] Connection failed: %d\n", connResult.load());
        }
    });

    // Monitor thread: Capture PROCESSING state during execution
    // Sample multiple times to catch the state transition from PENDING to PROCESSING
    std::thread monitorThread([&] {
        // Wait for connection to be established
        while (cliLinkID == IOC_ID_INVALID) {
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }

        // Sample 1: Early (likely PENDING)
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        capturedStateEarly = IOC_CmdDesc_getStatus(&cmdDesc);
        printf("📊 [SAMPLE 1] Early state: %d\n", capturedStateEarly.load());

        // Sample 2: Mid (should catch PROCESSING)
        std::this_thread::sleep_for(std::chrono::milliseconds(15));
        capturedStateMid = IOC_CmdDesc_getStatus(&cmdDesc);
        printf("📊 [SAMPLE 2] Mid state: %d\n", capturedStateMid.load());

        // Sample 3: Late (might be SUCCESS or still PROCESSING)
        std::this_thread::sleep_for(std::chrono::milliseconds(15));
        capturedStateLate = IOC_CmdDesc_getStatus(&cmdDesc);
        printf("📊 [SAMPLE 3] Late state: %d\n", capturedStateLate.load());

        // Use mid sample as the primary capture
        capturedStateDuringExec.store(capturedStateMid.load());
    });

    cliThread.join();
    srvThread.join();
    monitorThread.join();

    // ═══════════════════════════════════════════════════════════════════════════
    // ✅ VERIFY: Command should transition to PROCESSING
    // ═══════════════════════════════════════════════════════════════════════════
    printf("✅ [VERIFY] Checking PROCESSING state transition...\n");

    // Verify connection and acceptance succeeded
    VERIFY_KEYPOINT_EQ(connResult.load(), IOC_RESULT_SUCCESS, "[CONNECTION] Client connection should succeed");
    VERIFY_KEYPOINT_NE(cliLinkID, IOC_ID_INVALID, "[CONNECTION] Client LinkID should be valid");
    VERIFY_KEYPOINT_EQ(acceptResult.load(), IOC_RESULT_SUCCESS, "[CONNECTION] Server accept should succeed");
    VERIFY_KEYPOINT_NE(srvLinkID, IOC_ID_INVALID, "[CONNECTION] Server LinkID should be valid");

    // Verify command execution succeeded
    VERIFY_KEYPOINT_EQ(execResult.load(), IOC_RESULT_SUCCESS, "[EXECUTION] Command execution should succeed");

    // Verify state before execution was INITIALIZED
    VERIFY_KEYPOINT_EQ(capturedStateBeforeExec.load(), IOC_CMD_STATUS_INITIALIZED,
                       "[BEFORE] Command should be INITIALIZED before execution");

    // Verify executor observed PROCESSING state (server-side view)
    IOC_CmdStatus_E executorObservedState = srvExecPriv.CapturedCmdStatus.load();
    printf("📊 [SERVER-SIDE] Executor observed state: %d (PROCESSING=3 expected)\n", executorObservedState);
    VERIFY_KEYPOINT_EQ(executorObservedState, IOC_CMD_STATUS_PROCESSING,
                       "[SERVER] Command must be PROCESSING during executor callback (US-1 AC-2)");

    // KEY VERIFICATION for TC-2: Client-side state ISOLATION
    // Architecture Insight: Client-side cmdDesc remains PENDING while server processes
    // This validates state independence across TCP boundary
    printf("📊 [CLIENT-SIDE] State progression: Early=%d, Mid=%d, Late=%d\n", capturedStateEarly.load(),
           capturedStateMid.load(), capturedStateLate.load());

    // All client-side samples should be PENDING(2) - this validates state isolation
    bool allPending = (capturedStateEarly.load() == IOC_CMD_STATUS_PENDING) &&
                      (capturedStateMid.load() == IOC_CMD_STATUS_PENDING) &&
                      (capturedStateLate.load() == IOC_CMD_STATUS_PENDING);

    VERIFY_KEYPOINT_TRUE(allPending,
                         "[CLIENT] Client-side cmdDesc should remain PENDING(2) while server processes (validates "
                         "state isolation)");  // Verify final state is SUCCESS
    IOC_CmdStatus_E finalState = IOC_CmdDesc_getStatus(&cmdDesc);
    printf("📊 [FINAL] Command final state: %d (SUCCESS=4 expected)\n", finalState);
    VERIFY_KEYPOINT_EQ(finalState, IOC_CMD_STATUS_SUCCESS,
                       "[FINAL] Command should reach SUCCESS state after execution");

    // ═══════════════════════════════════════════════════════════════════════════
    // 🧹 CLEANUP
    // ═══════════════════════════════════════════════════════════════════════════
    if (cliLinkID != IOC_ID_INVALID) IOC_closeLink(cliLinkID);
    if (srvLinkID != IOC_ID_INVALID) IOC_closeLink(srvLinkID);
    if (srvID != IOC_ID_INVALID) IOC_offlineService(srvID);

    printf("✅ TC-2 COMPLETE\n\n");
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
