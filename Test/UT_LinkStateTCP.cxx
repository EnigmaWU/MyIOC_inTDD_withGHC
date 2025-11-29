/**************************************************************************************************
 * @file UT_LinkStateTCP.cxx
 * @brief Unit tests for Link State (US-2) behavior over TCP protocol
 * @date 2025-11-29
 *
 * @purpose Validate IOC link state machine behavior specific to TCP transport protocol.
 *          Tests link main state and substates during TCP connection lifecycle, state
 *          transitions during command/event activity, and correlation with TCP socket state.
 *
 * @architecture_mapping US-2: Link Command State (README_ArchDesign.md)
 *          Link State Machine: LinkStateReady (composite) with role-specific substates
 *          - CmdInitiatorReady ⟷ CmdInitiatorBusyExecCmd
 *          - CmdExecutorReady → BusyWaitCmd → BusyExecCmd → BusyAckCmd → Ready
 *
 * @scope TCP-specific link state testing (US-2 × TCP protocol integration)
 * @related_files
 *   - UT_CommandStateTCP.cxx: Command state (US-1) over TCP
 *   - UT_LinkStateUS2.cxx: Protocol-agnostic link state testing
 *   - README_ArchDesign.md: Link State Machine specifications
 *
 * FRAMEWORK STATUS: ⚪ Link State Testing - DESIGN PHASE
 *    • Test infrastructure: PENDING (TcpLinkStateMonitor, LinkStateValidator)
 *    • Test cases: 0/14 (0% complete)
 *    • Target: 14 test cases covering TCP-specific link state scenarios
 *    • Progress: Design skeleton created, ready for implementation
 **************************************************************************************************/

#include "_UT_IOC_Common.h"
#include "IOC/IOC.h"
#include "IOC/IOC_CmdAPI.h"
#include "IOC/IOC_CmdDesc.h"
#include "IOC/IOC_SrvAPI.h"
#include "IOC/IOC_Types.h"

#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>
#include <vector>

/**************************************************************************************************
 * @brief 【TCP-Specific Link State Test Cases】
 *
 * ORGANIZATION STRATEGY:
 *  🔷 By TCP Connection Lifecycle Phase:
 *     • Connection Establishment Phase (SYN → SYN-ACK → ACK → ESTABLISHED)
 *     • Active Connection Phase (ESTABLISHED with command/event activity)
 *     • Connection Loss Phase (RESET, TIMEOUT, network partition)
 *     • Connection Shutdown Phase (Graceful FIN vs Abortive RST)
 *     • State Correlation (Link state ⟺ TCP socket state ⟺ Command activity)
 *
 *  🔷 By Link State Layer:
 *     • Link Main State: Ready/Busy/Offline/Disconnected
 *     • Link SubState: CmdInitiatorReady, CmdInitiatorBusyExecCmd, CmdExecutorBusyWaitCmd, etc.
 *     • TCP Socket State: LISTEN, SYN_SENT, ESTABLISHED, FIN_WAIT, CLOSE_WAIT, etc.
 *
 *  🔷 By State Transition Trigger:
 *     • Connection events: connect(), accept(), close()
 *     • Command activity: IOC_execCMD() triggering substate transitions
 *     • TCP errors: ECONNRESET, EPIPE, ETIMEDOUT
 *     • Application control: IOC_closeLink(), IOC_offlineService()
 *
 * 🎯 COVERAGE TARGET: 100% of TCP-specific link state integration scenarios
 *
 * STATUS TRACKING: ⚪ = Planned/TODO，🔴 = Implemented/RED, 🟢 = Passed/GREEN, ⚠️ = Issues
 *
 * 🟢 FRAMEWORK STATUS: TCP-Specific Link State Testing - DESIGN PHASE
 *    • Core framework: PENDING (TcpLinkStateMonitor, LinkStateValidator)
 *    • Test cases: 0/14 (0% complete)
 *    • Target: 14 test cases covering TCP-specific link state scenarios
 *    • Progress: Design skeleton established, 4 tests moved from UT_CommandStateTCP.cxx
 *    • Architecture compliance: Link State Machine per README_ArchDesign.md
 *
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * 📋 [CAT-1]: TCP CONNECTION ESTABLISHMENT × LINK STATE (0/3)
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * PURPOSE: Verify link state behavior during TCP connection setup phase
 *
 * [@AC-1,US-2] [@AC-2,US-2] Link state reflects command readiness and activity
 * ⚪ TC-1: verifyLinkState_duringTcpConnectAttempt_expectConnectingSubState
 *      @[Purpose]: Validate link state reflects TCP connection attempt
 *      @[Brief]: Check IOC_getLinkState() during connection establishment
 *      @[TCP Focus]: Link state should show connecting/establishing during TCP handshake
 *      @[US Mapping]: US-2 AC-1 (link ready for commands), AC-2 (reflects command activity)
 *      @[Expected]: Link SubState indicates connection in progress
 *      @[Architecture]: LinkStateReady composite state with CmdInitiator substates
 *      @[Port]: 23080 (base port for link state testing)
 *      @[Priority]: HIGH - Link state during TCP handshake
 *      @[Origin]: Moved from UT_CommandStateTCP.cxx TC-4
 *
 * [@AC-1,US-2] [@AC-7,US-2] Link state transitions to ready after connection success
 * ⚪ TC-2: verifyLinkState_afterTcpConnectSuccess_expectReadyState
 *      @[Purpose]: Validate link transitions to Ready state after TCP connection established
 *      @[Brief]: Monitor link main state and substate after successful connection
 *      @[TCP Focus]: Link state synchronized with TCP ESTABLISHED state
 *      @[US Mapping]: US-2 AC-1 (link ready for commands), AC-7 (ready state after completion)
 *      @[Expected]: Link Main State = Ready, SubState = CmdInitiatorReady or CmdExecutorReady
 *      @[Port]: 23081
 *      @[Priority]: HIGH - Link state after connection establishment
 *
 * [@AC-1,US-2] Link state remains offline/disconnected when connection fails
 * ⚪ TC-3: verifyLinkState_whenTcpConnectRefused_expectOfflineState
 *      @[Purpose]: Validate link remains offline when connection refused (ECONNREFUSED)
 *      @[Brief]: Attempt connect to offline server, verify link state reflects failure
 *      @[TCP Focus]: Link state correctly indicates connection failure
 *      @[US Mapping]: US-2 AC-1 (link not ready when connection fails)
 *      @[Expected]: Link Main State = Offline/Disconnected, connection attempt fails
 *      @[Port]: 23082 (server deliberately not started)
 *      @[Priority]: HIGH - Link state on connection failure
 *
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * 📋 [CAT-2]: ACTIVE CONNECTION × LINK SUBSTATE TRANSITIONS (0/3)
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * PURPOSE: Verify link substates during command/event activity on established TCP connection
 *
 * [@AC-2,US-2] Link substate reflects command activity (CmdInitiator perspective)
 * ⚪ TC-4: verifyLinkSubState_duringCommandExecution_expectBusyExecCmd
 *      @[Purpose]: Validate link substate transitions during command execution
 *      @[Brief]: Execute command, monitor link substate (CmdInitiator side)
 *      @[TCP Focus]: Link substate synchronized with command execution over TCP
 *      @[US Mapping]: US-2 AC-2 (link state reflects command activity)
 *      @[Expected]: CmdInitiatorReady → CmdInitiatorBusyExecCmd → CmdInitiatorReady
 *      @[Architecture]: CmdInitiatorReady ⟷ CmdInitiatorBusyExecCmd (per README_ArchDesign.md)
 *      @[Port]: 23083
 *      @[Priority]: HIGH - Link substate during command execution (initiator side)
 *
 * [@AC-2,US-2] Link substate reflects command reception (CmdExecutor perspective)
 * ⚪ TC-5: verifyLinkSubState_duringCommandReception_expectBusyWaitAndExec
 *      @[Purpose]: Validate link substate transitions during command reception
 *      @[Brief]: Monitor link substate on server side during command processing
 *      @[TCP Focus]: Link substate reflects command lifecycle (executor side)
 *      @[US Mapping]: US-2 AC-2 (link state reflects command activity)
 *      @[Expected]: CmdExecutorReady → BusyWaitCmd → BusyExecCmd → BusyAckCmd → Ready
 *      @[Architecture]: CmdExecutor substate machine (per README_ArchDesign.md)
 *      @[Port]: 23084
 *      @[Priority]: HIGH - Link substate during command processing (executor side)
 *
 * [@AC-7,US-2] Link substate returns to ready after command completion
 * ⚪ TC-6: verifyLinkSubState_afterCommandCompletion_expectReturnToReady
 *      @[Purpose]: Validate link substate returns to Ready after command completes
 *      @[Brief]: Execute multiple commands, verify substate returns to Ready between commands
 *      @[TCP Focus]: Link substate cleanup after command lifecycle
 *      @[US Mapping]: US-2 AC-7 (link returns to ready state after completion)
 *      @[Expected]: Each command cycle: Ready → Busy → Ready (repeatable)
 *      @[Port]: 23085
 *      @[Priority]: MEDIUM - Link substate cleanup and reusability
 *
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * 📋 [CAT-3]: TCP CONNECTION LOSS × LINK STATE (0/3)
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * PURPOSE: Verify link state behavior when TCP connection fails or resets
 *
 * [@AC-2,US-4] [@AC-7,US-2] Link state reflects timeout/error impact
 * ⚪ TC-7: verifyLinkState_whenTcpConnectionReset_expectDisconnectedState
 *      @[Purpose]: Validate link state reflects TCP connection loss (ECONNRESET)
 *      @[Brief]: Monitor IOC_getLinkState() when connection resets
 *      @[TCP Focus]: Link state synchronized with TCP state (RST packet received)
 *      @[US Mapping]: US-4 AC-2 (link state reflects timeout/error), US-2 AC-7 (state transition)
 *      @[Expected]: Link Main State transitions to OFFLINE/DISCONNECTED with error indication
 *      @[Architecture]: Link State Machine error handling
 *      @[Port]: 23086
 *      @[Priority]: HIGH - TCP connection state correlation
 *      @[Origin]: Moved from UT_CommandStateTCP.cxx TC-8
 *
 * [@AC-2,US-4] Link state reflects broken pipe error
 * ⚪ TC-8: verifyLinkState_whenTcpPipeBroken_expectErrorState
 *      @[Purpose]: Validate link state reflects EPIPE (write to closed socket)
 *      @[Brief]: Close remote end, attempt send, verify link state reflects error
 *      @[TCP Focus]: Link state reflects send-side connection loss
 *      @[US Mapping]: US-4 AC-2 (link reflects error impact)
 *      @[Expected]: Link Main State = Disconnected/Error with EPIPE indication
 *      @[Port]: 23087
 *      @[Priority]: HIGH - Send-side connection loss detection
 *
 * [@AC-2,US-4] Link state reflects connection timeout
 * ⚪ TC-9: verifyLinkState_whenTcpConnectionTimeout_expectTimeoutState
 *      @[Purpose]: Validate link state reflects TCP connection timeout
 *      @[Brief]: Simulate network partition, verify link detects timeout
 *      @[TCP Focus]: Link state reflects TCP keepalive timeout or retransmit timeout
 *      @[US Mapping]: US-4 AC-2 (link reflects timeout impact)
 *      @[Expected]: Link Main State = Disconnected/Timeout after timeout period
 *      @[Port]: 23088
 *      @[Priority]: MEDIUM - Connection timeout detection
 *
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * 📋 [CAT-4]: TCP SHUTDOWN × LINK STATE (0/3)
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * PURPOSE: Verify link state during TCP connection shutdown (graceful vs abortive)
 *
 * [@AC-7,US-2] Link state returns to appropriate ready/offline state
 * ⚪ TC-10: verifyLinkState_afterTcpGracefulClose_expectCleanOffline
 *      @[Purpose]: Validate link state after clean TCP close (FIN handshake)
 *      @[Brief]: Monitor link state during graceful shutdown (close() with FIN)
 *      @[TCP Focus]: Link state reflects graceful termination
 *      @[US Mapping]: US-2 AC-7 (link returns to ready/offline state after completion)
 *      @[Expected]: Link transitions to OFFLINE cleanly, no error state
 *      @[Architecture]: Link State Machine clean shutdown path
 *      @[Port]: 23089
 *      @[Priority]: MEDIUM - TCP FIN link state transition
 *      @[Origin]: Moved from UT_CommandStateTCP.cxx TC-17
 *
 * [@AC-2,US-4] Link state reflects timeout/error impact
 * ⚪ TC-11: verifyLinkState_afterTcpAbortiveClose_expectErrorState
 *      @[Purpose]: Validate link state after abortive TCP close (RST)
 *      @[Brief]: Monitor link state during abortive shutdown (SO_LINGER=0 or RST)
 *      @[TCP Focus]: Link state reflects error termination
 *      @[US Mapping]: US-4 AC-2 (link reflects timeout/error impact)
 *      @[Expected]: Link transitions to ERROR/OFFLINE with error indication
 *      @[Architecture]: Link State Machine error shutdown path
 *      @[Port]: 23090
 *      @[Priority]: MEDIUM - TCP RST link state transition
 *      @[Origin]: Moved from UT_CommandStateTCP.cxx TC-18
 *
 * [@AC-7,US-2] Link state transitions correctly during server shutdown
 * ⚪ TC-12: verifyLinkState_duringServerShutdown_expectGracefulOffline
 *      @[Purpose]: Validate link state when server initiates shutdown (IOC_offlineService)
 *      @[Brief]: Monitor client link state when server goes offline
 *      @[TCP Focus]: Link state reflects server-initiated shutdown
 *      @[US Mapping]: US-2 AC-7 (link transitions to offline state)
 *      @[Expected]: Client link detects server shutdown, transitions to OFFLINE
 *      @[Port]: 23091
 *      @[Priority]: MEDIUM - Server-initiated shutdown detection
 *
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * 📋 [CAT-5]: LINK STATE CORRELATION × TCP SOCKET STATE (0/2)
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * PURPOSE: Verify correlation between IOC link state and TCP socket state
 *
 * [@AC-1,US-2] [@AC-2,US-2] Link state accurately reflects TCP socket state
 * ⚪ TC-13: verifyLinkStateCorrelation_withTcpSocketState_expectConsistency
 *      @[Purpose]: Validate IOC link state matches underlying TCP socket state
 *      @[Brief]: Compare IOC_getLinkState() with getsockopt(TCP_INFO) throughout connection lifecycle
 *      @[TCP Focus]: Link state abstraction correctly represents TCP socket state
 *      @[US Mapping]: US-2 AC-1 (link ready), AC-2 (link reflects activity)
 *      @[Expected]: Link state transitions match TCP socket state transitions
 *      @[Architecture]: Link State Machine accurately abstracts transport layer
 *      @[Port]: 23092
 *      @[Priority]: HIGH - State abstraction accuracy verification
 *
 * [@AC-2,US-2] Link substate transitions correlate with command activity
 * ⚪ TC-14: verifyLinkSubStateCorrelation_withCommandActivity_expectConsistency
 *      @[Purpose]: Validate link substate accurately reflects command execution activity
 *      @[Brief]: Monitor link substate during command bursts, verify correlation
 *      @[TCP Focus]: Link substate transitions synchronized with TCP data transfer
 *      @[US Mapping]: US-2 AC-2 (link state reflects command activity)
 *      @[Expected]: Substate transitions align with command execution timing
 *      @[Port]: 23093
 *      @[Priority]: HIGH - Substate synchronization verification
 *
 **************************************************************************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF TCP-SPECIFIC LINK STATE TESTING INFRASTRUCTURE==================================

/**
 * @brief TCP Link State Monitor Helper
 *        Monitors and records link state transitions during TCP operations
 */
class TcpLinkStateMonitor {
   public:
    struct LinkStateSnapshot {
        std::chrono::steady_clock::time_point timestamp;
        IOC_LinkID_T linkID;
        IOC_LinkState_T mainState;
        IOC_LinkSubState_T subState;
        int tcpSocketState;  // From TCP_INFO or getsockopt()
        bool tcpConnected;
    };

    TcpLinkStateMonitor(IOC_LinkID_T linkID) : m_linkID(linkID) {}

    void captureSnapshot() {
        LinkStateSnapshot snapshot = {};
        snapshot.timestamp = std::chrono::steady_clock::now();
        snapshot.linkID = m_linkID;

        // Capture IOC link state
        IOC_getLinkState(m_linkID, &snapshot.mainState, &snapshot.subState);

        // TODO: Capture TCP socket state via getsockopt(TCP_INFO)
        snapshot.tcpSocketState = 0;  // Placeholder
        snapshot.tcpConnected = false;  // Placeholder

        std::lock_guard<std::mutex> lock(m_mutex);
        m_history.push_back(snapshot);
    }

    void printHistory() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        printf("\n📊 Link State History (LinkID: %llu):\n", m_linkID);
        for (size_t i = 0; i < m_history.size(); i++) {
            const auto &snap = m_history[i];
            printf("  [%zu] MainState=%d, SubState=%d, TcpState=%d, TcpConnected=%d\n",
                   i, snap.mainState, snap.subState, snap.tcpSocketState, snap.tcpConnected);
        }
    }

    size_t getSnapshotCount() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_history.size();
    }

    void clear() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_history.clear();
    }

   private:
    IOC_LinkID_T m_linkID;
    mutable std::mutex m_mutex;
    std::vector<LinkStateSnapshot> m_history;
};

/**
 * @brief Link State Validator
 *        Validates link state transitions and correlations
 */
class LinkStateValidator {
   public:
    static bool validateStateTransition(IOC_LinkState_T fromState, IOC_LinkState_T toState) {
        // TODO: Implement state machine validation logic
        // Validate allowed state transitions per README_ArchDesign.md
        return true;  // Placeholder
    }

    static bool validateSubStateTransition(IOC_LinkSubState_T fromSubState, IOC_LinkSubState_T toSubState) {
        // TODO: Implement substate machine validation logic
        // Validate allowed substate transitions per README_ArchDesign.md
        return true;  // Placeholder
    }

    static bool validateStateCorrelation(IOC_LinkState_T linkState, int tcpSocketState) {
        // TODO: Implement state correlation validation
        // Verify IOC link state matches TCP socket state
        return true;  // Placeholder
    }
};

//======>END OF TCP-SPECIFIC LINK STATE TESTING INFRASTRUCTURE====================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF TEST HELPER FUNCTIONS============================================================

// Test base port for TCP link state tests
#define _UT_LINKSTATE_TCP_BASE_PORT 23080

// Link state observation callback private data
typedef struct __LinkStateObserverPriv {
    std::atomic<int> StateChangeCount{0};
    std::atomic<IOC_LinkState_T> LastMainState;
    std::atomic<IOC_LinkSubState_T> LastSubState;
    std::mutex DataMutex;
} __LinkStateObserverPriv_T;

// Simple command execution callback for link state testing
static IOC_Result_T __LinkStateTcp_ExecutorCb(IOC_LinkID_T LinkID, IOC_CmdDesc_pT pCmdDesc, void *pCbPriv) {
    if (!pCmdDesc) return IOC_RESULT_INVALID_PARAM;

    // Simple PING response
    IOC_CmdID_T cmdID = IOC_CmdDesc_getCmdID(pCmdDesc);
    if (cmdID == IOC_CMDID_TEST_PING) {
        const char *response = "PONG";
        return IOC_CmdDesc_setOutPayload(pCmdDesc, (void *)response, strlen(response));
    }

    return IOC_RESULT_SUCCESS;
}

//======>END OF TEST HELPER FUNCTIONS==============================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF TEST CASE IMPLEMENTATIONS=======================================================

//=================================================================================================
// 📋 [CAT-1]: TCP CONNECTION ESTABLISHMENT × LINK STATE
//=================================================================================================

// TEST IMPLEMENTATIONS PENDING - Design skeleton established
// TC-1 through TC-14 will be implemented following CaTDD methodology

//=================================================================================================
// 📋 [CAT-2]: ACTIVE CONNECTION × LINK SUBSTATE TRANSITIONS
//=================================================================================================

// TEST IMPLEMENTATIONS PENDING

//=================================================================================================
// 📋 [CAT-3]: TCP CONNECTION LOSS × LINK STATE
//=================================================================================================

// TEST IMPLEMENTATIONS PENDING

//=================================================================================================
// 📋 [CAT-4]: TCP SHUTDOWN × LINK STATE
//=================================================================================================

// TEST IMPLEMENTATIONS PENDING

//=================================================================================================
// 📋 [CAT-5]: LINK STATE CORRELATION × TCP SOCKET STATE
//=================================================================================================

// TEST IMPLEMENTATIONS PENDING

//======>END OF TEST CASE IMPLEMENTATIONS=========================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF IMPLEMENTATION ROADMAP==========================================================
/**
 * 🗺️ IMPLEMENTATION ROADMAP FOR UT_LinkStateTCP.cxx
 *
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * PHASE 1: FOUNDATION (Week 1) - Priority: HIGH
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * ✅ Task 1.1: Design skeleton and test case categorization (COMPLETE - This file)
 * ⚪ Task 1.2: Implement TcpLinkStateMonitor helper class
 *    - captureSnapshot() with TCP_INFO support
 *    - printHistory() debugging output
 *    - Thread-safe history tracking
 *
 * ⚪ Task 1.3: Implement LinkStateValidator helper class
 *    - validateStateTransition() per README_ArchDesign.md
 *    - validateSubStateTransition() per README_ArchDesign.md
 *    - validateStateCorrelation() for IOC↔TCP state matching
 *
 * ⚪ Task 1.4: Create TCP link state test fixture base class
 *    - SetUp(): Initialize IOC framework + TCP server
 *    - TearDown(): Cleanup connections and IOC resources
 *    - Helper methods: setupTcpConnection(), monitorLinkState(), etc.
 *
 * MILESTONE 1: Infrastructure ready for test implementation
 *
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * PHASE 2: MOVE EXISTING TESTS FROM UT_CommandStateTCP.cxx (Week 1) - Priority: HIGH
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * ⚪ Task 2.1: Move TC-1 (formerly CommandStateTCP TC-4)
 *    - verifyLinkState_duringTcpConnectAttempt_expectConnectingSubState
 *    - Adapt from command state focus to link state focus
 *    - Verify during TCP handshake (SYN → SYN-ACK → ACK)
 *
 * ⚪ Task 2.2: Move TC-7 (formerly CommandStateTCP TC-8)
 *    - verifyLinkState_whenTcpConnectionReset_expectDisconnectedState
 *    - Focus on link state transition, not command state
 *
 * ⚪ Task 2.3: Move TC-10 (formerly CommandStateTCP TC-17)
 *    - verifyLinkState_afterTcpGracefulClose_expectCleanOffline
 *    - Verify FIN handshake reflected in link state
 *
 * ⚪ Task 2.4: Move TC-11 (formerly CommandStateTCP TC-18)
 *    - verifyLinkState_afterTcpAbortiveClose_expectErrorState
 *    - Verify RST reflected in link state
 *
 * MILESTONE 2: 4 existing tests migrated and GREEN
 *
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * PHASE 3: HIGH-PRIORITY NEW TESTS (Week 2) - Priority: HIGH
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * ⚪ Task 3.1: Implement CAT-1 remaining tests (TC-2, TC-3)
 *    - TC-2: Link state after successful connection
 *    - TC-3: Link state when connection refused
 *
 * ⚪ Task 3.2: Implement CAT-2 (Active Connection × Link SubState) - TCs 4-6
 *    - TC-4: CmdInitiator substate during command execution
 *    - TC-5: CmdExecutor substate during command processing
 *    - TC-6: Substate returns to Ready after command
 *
 * ⚪ Task 3.3: Implement CAT-5 (State Correlation) - TCs 13-14
 *    - TC-13: Link state ⟺ TCP socket state correlation
 *    - TC-14: Link substate ⟺ Command activity correlation
 *
 * MILESTONE 3: Core link state behavior verified (10/14 tests GREEN)
 *
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * PHASE 4: MEDIUM-PRIORITY TESTS (Week 3) - Priority: MEDIUM
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * ⚪ Task 4.1: Implement CAT-3 remaining tests (TC-8, TC-9)
 *    - TC-8: Link state on broken pipe (EPIPE)
 *    - TC-9: Link state on connection timeout
 *
 * ⚪ Task 4.2: Implement CAT-4 remaining test (TC-12)
 *    - TC-12: Link state during server shutdown
 *
 * MILESTONE 4: 100% test coverage complete (14/14 tests GREEN)
 *
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * PHASE 5: INTEGRATION & DOCUMENTATION (Week 3) - Priority: HIGH
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * ⚪ Task 5.1: Run full test suite, verify all GREEN
 * ⚪ Task 5.2: Performance profiling (test execution time)
 * ⚪ Task 5.3: Update documentation
 *    - README_ArchDesign.md: Add link state testing notes
 *    - Update UT_CommandStateTCP.cxx: Reference UT_LinkStateTCP.cxx
 * ⚪ Task 5.4: Code review and cleanup
 * ⚪ Task 5.5: Compare with UT_LinkStateUS2.cxx (protocol-agnostic link state tests)
 *
 * MILESTONE 5: Production-ready TCP link state testing framework
 *
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * 📊 EFFORT ESTIMATION:
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * Phase 1: 6-8 hours   (Infrastructure)
 * Phase 2: 6-8 hours   (Move 4 existing tests)
 * Phase 3: 12-16 hours (6 high-priority new tests)
 * Phase 4: 6-8 hours   (3 medium-priority tests)
 * Phase 5: 3-5 hours   (Integration & docs)
 * ─────────────────────────────────────────
 * TOTAL:   33-45 hours (~1 week full-time)
 *
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * 🎯 SUCCESS CRITERIA:
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * ✓ All 14 test cases implemented and GREEN
 * ✓ 100% coverage of TCP-specific link state scenarios
 * ✓ All state transitions validated against README_ArchDesign.md
 * ✓ Zero state correlation violations detected
 * ✓ Test execution time < 45 seconds (all tests)
 * ✓ No memory leaks (valgrind clean)
 * ✓ Documentation complete and accurate
 * ✓ Clear separation: Link State (US-2) vs Command State (US-1) tests
 *
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * 🔗 RELATED WORK:
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * • UT_LinkStateUS2.cxx - Protocol-agnostic link state (US-2)
 * • UT_CommandStateTCP.cxx - Command state (US-1) over TCP
 * • UT_CommandFaultTCP.cxx - TCP fault scenarios
 * • README_ArchDesign.md - Link State Machine specifications
 */
//======>END OF IMPLEMENTATION ROADMAP============================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF DESIGN NOTES & DECISION LOG=====================================================
/**
 * 🗒️ DESIGN DECISIONS AND RATIONALE
 *
 * ─────────────────────────────────────────────────────────────────────────────────────────────
 * DECISION 1: Why separate UT_LinkStateTCP.cxx from UT_CommandStateTCP.cxx?
 * ─────────────────────────────────────────────────────────────────────────────────────────────
 * RATIONALE:
 *  • README_ArchDesign.md clearly separates Link State (US-2) and Command State (US-1)
 *  • Link State: Communication channel status with composite substates
 *  • Command State: Individual command execution lifecycle
 *  • These are independent state machines with different purposes
 *  • Mixing tests violated Single Responsibility Principle
 *
 * ARCHITECTURE EVIDENCE:
 *  • Link State Machine (US-2): LinkStateReady composite with role substates
 *    - CmdInitiatorReady ⟷ CmdInitiatorBusyExecCmd
 *    - CmdExecutorReady → BusyWaitCmd → BusyExecCmd → BusyAckCmd → Ready
 *  • Command State Machine (US-1): INITIALIZED → PENDING → PROCESSING → SUCCESS/FAILED/TIMEOUT
 *  • Independent lifecycles, different transition triggers
 *
 * BENEFITS:
 *  ✓ Clear architectural alignment with README_ArchDesign.md
 *  ✓ Single Responsibility: Each file tests one state machine
 *  ✓ Maintainability: Link state changes don't affect command state tests
 *  ✓ Scalability: Link state testing extends to EVT and DAT protocols
 *  ✓ Documentation: Test structure mirrors architecture design
 *
 * ─────────────────────────────────────────────────────────────────────────────────────────────
 * DECISION 2: Why 14 test cases organized into 5 categories?
 * ─────────────────────────────────────────────────────────────────────────────────────────────
 * RATIONALE:
 *  • TCP connection has distinct lifecycle phases affecting link state
 *  • Link state has two layers: Main state + SubState
 *  • Need comprehensive coverage: Connection, Activity, Loss, Shutdown, Correlation
 *  • 14 TCs provide balanced coverage without redundancy
 *
 * COVERAGE ANALYSIS:
 *  • CAT-1 (3 TCs): Connection Establishment - TCP handshake × link state
 *  • CAT-2 (3 TCs): Active Operations - Link substates during command activity
 *  • CAT-3 (3 TCs): Connection Loss - TCP errors reflected in link state
 *  • CAT-4 (3 TCs): Shutdown - Graceful/Abortive close × link state
 *  • CAT-5 (2 TCs): State Correlation - Link ⟺ TCP ⟺ Command consistency
 *
 * ARCHITECTURE MAPPING:
 *  • CAT-1, CAT-3, CAT-4: Link Main State transitions (Ready/Busy/Offline/Disconnected)
 *  • CAT-2: Link SubState transitions (CmdInitiator/CmdExecutor substates)
 *  • CAT-5: State abstraction accuracy (IOC layer ⟺ TCP layer)
 *
 * ─────────────────────────────────────────────────────────────────────────────────────────────
 * DECISION 3: Why TcpLinkStateMonitor and LinkStateValidator helper classes?
 * ─────────────────────────────────────────────────────────────────────────────────────────────
 * RATIONALE:
 *  • Link state testing requires monitoring state transitions over time
 *  • Need correlation between IOC link state and TCP socket state
 *  • State machine validation requires checking allowed transitions per architecture
 *  • Helper classes reduce code duplication and improve test clarity
 *
 * DESIGN:
 *  • TcpLinkStateMonitor: Records state history with timestamps
 *    - captureSnapshot(): Captures IOC link state + TCP socket state
 *    - printHistory(): Debugging output for state transition sequences
 *    - Thread-safe for multi-threaded test scenarios
 *
 *  • LinkStateValidator: Validates state machine rules
 *    - validateStateTransition(): Checks main state transitions
 *    - validateSubStateTransition(): Checks substate transitions
 *    - validateStateCorrelation(): Checks IOC ⟺ TCP state consistency
 *
 * ─────────────────────────────────────────────────────────────────────────────────────────────
 * DECISION 4: Port allocation strategy (23080-23093)?
 * ─────────────────────────────────────────────────────────────────────────────────────────────
 * RATIONALE:
 *  • Avoid conflict with other test suites:
 *    - 20xxx: Typical tests
 *    - 21xxx: Command fault tests (UT_CommandFaultTCP)
 *    - 22xxx: Command state tests (UT_CommandStateTCP)
 *    - 23xxx: Link state tests (UT_LinkStateTCP) ← NEW
 *  • 23xxx range clearly indicates "Link State testing"
 *  • Sequential allocation (23080-23093) for 14 test cases
 *  • Each test gets dedicated port (prevents cross-test interference)
 *
 * ALLOCATION:
 *  • 23080-23082: CAT-1 (Connection Establishment) - 3 TCs
 *  • 23083-23085: CAT-2 (Active Connection × SubState) - 3 TCs
 *  • 23086-23088: CAT-3 (Connection Loss) - 3 TCs
 *  • 23089-23091: CAT-4 (Shutdown) - 3 TCs
 *  • 23092-23093: CAT-5 (State Correlation) - 2 TCs
 *
 * ─────────────────────────────────────────────────────────────────────────────────────────────
 * DECISION 5: Relationship with UT_LinkStateUS2.cxx (protocol-agnostic link state)?
 * ─────────────────────────────────────────────────────────────────────────────────────────────
 * COMPARISON:
 *  ┌────────────────────┬───────────────────────────┬─────────────────────────────┐
 *  │ Aspect             │ UT_LinkStateUS2.cxx       │ UT_LinkStateTCP.cxx         │
 *  ├────────────────────┼───────────────────────────┼─────────────────────────────┤
 *  │ Primary Focus      │ GENERIC link state        │ TCP-SPECIFIC link state     │
 *  │ Test Goal          │ "State machine correct?"  │ "TCP integration correct?"  │
 *  │ Protocol Scope     │ Protocol-agnostic         │ TCP only                    │
 *  │ State Transitions  │ Abstract transitions      │ TCP-triggered transitions   │
 *  │ Connection Events  │ Generic connect/close     │ TCP handshake, RST, FIN     │
 *  │ Error Scenarios    │ Generic errors            │ ECONNRESET, EPIPE, timeout  │
 *  │ SubState Testing   │ Role-based substates      │ TCP activity × substates    │
 *  │ Correlation        │ Not tested                │ IOC ⟺ TCP socket state      │
 *  └────────────────────┴───────────────────────────┴─────────────────────────────┘
 *
 * COMPLEMENTARY RELATIONSHIP:
 *  • UT_LinkStateUS2: "Link state machine logic correct" (protocol-independent)
 *  • UT_LinkStateTCP: "TCP transport correctly drives link state machine"
 *  • Together: Complete link state testing (Logic + TCP Integration)
 *
 * ─────────────────────────────────────────────────────────────────────────────────────────────
 * DECISION 6: Migration strategy for 4 tests from UT_CommandStateTCP.cxx?
 * ─────────────────────────────────────────────────────────────────────────────────────────────
 * ORIGIN:
 *  • UT_CommandStateTCP.cxx originally had 4 link state tests (misplaced):
 *    - TC-4: verifyLinkState_duringTcpConnectAttempt (CAT-1 in this file)
 *    - TC-8: verifyLinkState_whenTcpConnectionReset (CAT-3 in this file)
 *    - TC-17: verifyLinkState_afterTcpGracefulClose (CAT-4 in this file)
 *    - TC-18: verifyLinkState_afterTcpAbortiveClose (CAT-4 in this file)
 *
 * MIGRATION PLAN:
 *  1. Move test implementations from UT_CommandStateTCP.cxx to this file
 *  2. Adapt test focus: Command state → Link state
 *  3. Update test names and documentation if needed
 *  4. Adjust port allocations (22xxx → 23xxx)
 *  5. Verify tests still pass after migration
 *
 * BENEFITS:
 *  ✓ Preserves existing test designs (no lost work)
 *  ✓ Establishes proper architectural alignment
 *  ✓ Reduces UT_CommandStateTCP.cxx scope (3/15 tests after migration)
 *  ✓ Creates foundation for UT_LinkStateTCP.cxx (4/14 tests migrated)
 *
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * 📝 OPEN QUESTIONS FOR FUTURE CONSIDERATION:
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * Q1: Should we test link state for other protocols (FIFO, UDP)?
 *     Current: TCP only
 *     Future: Create UT_LinkStateFIFO.cxx, UT_LinkStateUDP.cxx if needed
 *
 * Q2: Should we test link state during reconnection/failover?
 *     Current: Basic reconnection covered in CAT-4 (CommandStateTCP)
 *     Future: May need dedicated reconnection × link state tests
 *
 * Q3: Should we test link state with multiple concurrent commands?
 *     Current: Single command execution
 *     Future: May reveal substate transition bugs under load
 *
 * Q4: Should we test link state during event subscription/publication?
 *     Current: Command-focused (CmdInitiator/CmdExecutor substates)
 *     Future: Need EventPublisher/EventSubscriber substate testing
 *
 * Q5: Should we test link state with data transfer operations?
 *     Current: Command-focused
 *     Future: Need DataSender/DataReceiver substate testing
 */
//======>END OF DESIGN NOTES & DECISION LOG=======================================================
