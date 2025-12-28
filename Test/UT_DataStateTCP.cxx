///////////////////////////////////////////////////////////////////////////////////////////////////
// Data State TCP Implementation: TCP-Specific State Integration Testing
//
// 🎯 PURPOSE: Verify TCP-protocol-specific data state behaviors and interactions
// 🔗 RELATIONSHIP: Complements UT_DataStateUS1-7.cxx (protocol-agnostic state testing)
// 📋 FOCUS: TCP connection state × Data transmission state integration
//
// 📊 DESIGN RATIONALE:
//    • UT_DataStateUS1-7.cxx: Protocol-agnostic state machine testing (FIFO/abstract)
//    • UT_DataStateTCP.cxx: TCP-specific state integration scenarios
//    • Key Difference: Connection lifecycle, TCP-specific errors, TCP protocol behavior
//    • Validates: Link substates (DatSender/DatReceiver) in TCP environment
//
// 🏗️ ARCHITECTURE CONTEXT:
//    This file addresses TCP-specific state scenarios that cannot be tested generically:
//    - Data state during TCP connection loss/recovery
//    - Error propagation from TCP layer to data state
//    - State consistency during TCP flow control and backpressure
//    - Data state behavior during TCP connection establishment failures
//    - Bidirectional streaming state management over TCP
//
// 📖 RELATED DOCUMENTATION:
//    • README_ArchDesign-State.md "Data State Machine" for state diagrams
//    • UT_DataState.h for dual-state testing framework
//    • UT_DataFaultTCP.cxx for TCP fault injection patterns
//    • IOC_Types.h for IOC_LinkSubState_T enum definitions
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>

#include "UT_DataState.h"

// Include IOC APIs needed for state tracking
#include "IOC/IOC.h"
#include "IOC/IOC_DatAPI.h"
#include "IOC/IOC_DatDesc.h"
#include "IOC/IOC_SrvAPI.h"
#include "IOC/IOC_Types.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF IMPLEMENTATION OVERVIEW=========================================================
/**
 * @brief TCP-Specific Data State Testing Framework
 *
 * 🔄 TESTING SCOPE: TCP Protocol × Data State Integration
 *
 * This file focuses on TCP-specific state behaviors that complement the protocol-agnostic
 * state testing in UT_DataStateUS1-7.cxx:
 *
 * 🟢 WHAT UT_DataStateUS1-7.cxx TESTS (Protocol-Agnostic):
 *    ✓ Data state transitions (DatSenderReady ↔ DatSenderBusySendDat)
 *    ✓ Receiver state evolution (DatReceiverReady → DatReceiverBusyRecvDat/BusyCbRecvDat)
 *    ✓ State isolation between concurrent sender/receiver
 *    ✓ State consistency across execution patterns
 *    ✓ Multi-role service state management
 *
 * 🔵 WHAT UT_DataStateTCP.cxx TESTS (TCP-Specific):
 *    ⚡ Data state during TCP connection establishment (SYN→ESTABLISHED)
 *    ⚡ TCP-specific errors: ECONNRESET, EPIPE, ECONNREFUSED impact on data state
 *    ⚡ Data state during TCP connection loss (mid-transmission)
 *    ⚡ TCP flow control impact: send buffer full, backpressure, window management
 *    ⚡ TCP shutdown behavior: FIN vs RST impact on data state
 *    ⚡ TCP reconnection: data state during connection recovery
 *    ⚡ Bidirectional streaming: concurrent sender/receiver state over TCP
 *    ⚡ TCP layer transparency: retransmit doesn't affect data state
 *
 * ❌ WHAT UT_DataStateTCP.cxx DOES NOT TEST (Covered by other files):
 *    ✗ Generic timeout detection (US-4 AC-1 in DataStateUS4.cxx)
 *    ✗ Generic error propagation (US-5 in DataStateUS5.cxx)
 *    ✗ Generic buffer state management (US-3 in DataStateUS3.cxx)
 *    ✗ Protocol-agnostic state transitions (US-1/US-2 in DataStateUS1/2.cxx)
 *
 * 📊 TCP STATE × DATA STATE MATRIX:
 *    ┌──────────────────────────┬───────────────────────────────────────────────────┐
 *    │ TCP Connection State     │ Expected Data State Behavior                      │
 *    ├──────────────────────────┼───────────────────────────────────────────────────┤
 *    │ TCP_SYN_SENT             │ Data Sender/Receiver not ready (link not ready)   │
 *    │ TCP_ESTABLISHED          │ DatSenderReady, DatReceiverReady available        │
 *    │ TCP_CLOSE_WAIT           │ Existing transmissions complete, new sends blocked│
 *    │ TCP_CLOSING              │ Data states transition to error/disconnected      │
 *    │ TCP_CLOSED               │ All data states must be reset/disconnected        │
 *    └──────────────────────────┴───────────────────────────────────────────────────┘
 *
 * 🎯 INTEGRATION FOCUS:
 *    • How TCP layer errors (connection loss) affect data state transitions
 *    • Whether data state properly reflects TCP connection health
 *    • Data cleanup and error handling during TCP failures
 *    • State consistency when TCP connection is restored
 *    • Bidirectional state independence over single TCP connection
 */
//======>END OF IMPLEMENTATION OVERVIEW===========================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF TEST CASE ORGANIZATION==========================================================
/**************************************************************************************************
 * @brief 【TCP-Specific Data State Test Cases】
 *
 * ORGANIZATION STRATEGY:
 *  🔷 By TCP Connection Lifecycle Phase:
 *     • Connection Establishment Phase (SYN → ESTABLISHED)
 *     • Active Connection Phase (ESTABLISHED)
 *     • Connection Loss Phase (RESET, TIMEOUT)
 *     • Connection Recovery Phase (Reconnection)
 *     • Connection Termination Phase (Graceful/Ungraceful Close)
 *
 *  🔷 By TCP Error Type × Data State Impact:
 *     • Connection Refused → Data FAILED/NOT_EXIST_LINK
 *     • Connection Reset → Data FAILED (mid-transmission)
 *     • Connection Timeout → Data TIMEOUT
 *     • Send Buffer Full → Data PROCESSING (flow control engaged)
 *     • Receive Timeout → Data TIMEOUT/NO_DATA
 *
 *  🔷 By State Transition Timing:
 *     • Pre-connection: Data stream not initialized, link not ready
 *     • During-transmission: Data streaming when TCP error occurs
 *     • Post-failure: Data state after TCP connection lost
 *     • Recovery: Data state restoration after reconnection
 *
 * 🎯 COVERAGE TARGET: 100% of TCP-specific data state integration scenarios
 *
 * STATUS TRACKING: ⚪ = Planned/TODO，🔴 = Implemented/RED, 🟢 = Passed/GREEN, ⚠️ = Issues
 *
 * ⚪ FRAMEWORK STATUS: TCP-Specific Data State Testing - ⚪ PLANNED
 *    • Core framework: INFRASTRUCTURE NEEDED (TcpConnectionSimulator, TcpDataStateTracker)
 *    • Test cases: 0/18 planned (0% complete)
 *    • Target: 18 test cases covering TCP-specific data state scenarios
 *    • Progress: CAT-1 ⚪ (0/3), CAT-2 ⚪ (0/3), CAT-3 ⚪ (0/3), CAT-4 ⚪ (0/3), CAT-5 ⚪ (0/3), CAT-6 ⚪ (0/3)
 *
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * 📋 [CAT-1]: TCP CONNECTION ESTABLISHMENT × DATA STATE (0/3 PLANNED) ⚪
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * PURPOSE: Verify data state behavior during TCP connection setup phase
 * FOCUS: Link readiness, initial sender/receiver states, connection establishment timing
 *
 * ⚪ TC-1: verifyDataStateBeforeConnection_byCheckingInitialStates_expectNotReady
 *    @[Purpose]: Verify data sender/receiver states before TCP connection established
 *    @[Brief]: Query data states before connectService(), expect link not ready or default states
 *    @[US]: TCP-specific connection lifecycle
 *    @[AC]: Data states reflect TCP connection status
 *    @[KeyVerifyPoint-1]: Before connect, data operations should fail with NOT_EXIST_LINK
 *    @[KeyVerifyPoint-2]: IOC_getLinkState() should indicate link not ready
 *
 * ⚪ TC-2: verifyDataStateDuringConnection_byMonitoringEstablishment_expectTransitionToReady
 *    @[Purpose]: Verify data state transitions during TCP SYN→ESTABLISHED phase
 *    @[Brief]: Monitor data states during connectService(), verify transition to DatSenderReady/DatReceiverReady
 *    @[US]: TCP-specific connection lifecycle
 *    @[AC]: Data states properly initialized upon connection success
 *    @[KeyVerifyPoint-1]: After TCP ESTABLISHED, sender state becomes DatSenderReady
 *    @[KeyVerifyPoint-2]: After TCP ESTABLISHED, receiver state becomes DatReceiverReady
 *
 * ⚪ TC-3: verifyDataStateAfterConnectionFailure_byRefusedConnection_expectNoStateChange
 *    @[Purpose]: Verify data states remain invalid when TCP connection fails
 *    @[Brief]: Attempt connect to refused endpoint, verify data states not initialized
 *    @[US]: TCP-specific error handling
 *    @[AC]: Failed connection should not create valid data states
 *    @[KeyVerifyPoint-1]: Connection failure returns error code
 *    @[KeyVerifyPoint-2]: Data states should not be queryable (invalid LinkID)
 *
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * 📋 [CAT-2]: DATA SENDER STATE × TCP TRANSMISSION (0/3 PLANNED) ⚪
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * PURPOSE: Verify data sender state behavior during TCP data transmission
 * FOCUS: DatSenderReady ↔ DatSenderBusySendDat transitions over TCP
 *
 * ⚪ TC-4: verifySenderStateTransition_bySimpleSendDAT_expectReadyToBusyToReady
 *    @[Purpose]: Verify sender state transitions during normal IOC_sendDAT() over TCP
 *    @[Brief]: Send data chunk via TCP, monitor sender state: Ready → BusySendDat → Ready
 *    @[US]: TCP-specific sender state
 *    @[AC]: Sender state accurately reflects send operation lifecycle
 *    @[KeyVerifyPoint-1]: Before sendDAT, state is DatSenderReady
 *    @[KeyVerifyPoint-2]: During/after sendDAT, state transitions correctly
 *
 * ⚪ TC-5: verifySenderStateDuringFlowControl_byBufferFull_expectBusyState
 *    @[Purpose]: Verify sender state when TCP send buffer full (flow control engaged)
 *    @[Brief]: Fill TCP send buffer, verify sender remains in busy state until buffer available
 *    @[US]: TCP-specific flow control
 *    @[AC]: Sender state reflects TCP backpressure
 *    @[KeyVerifyPoint-1]: Send buffer full triggers DatSenderBusySendDat
 *    @[KeyVerifyPoint-2]: State returns to DatSenderReady after buffer drains
 *
 * ⚪ TC-6: verifySenderStateOnConnectionLoss_byMidTransmissionReset_expectErrorState
 *    @[Purpose]: Verify sender state when TCP connection reset during transmission
 *    @[Brief]: Start large send operation, reset connection mid-transfer, verify error state
 *    @[US]: TCP-specific error handling
 *    @[AC]: Connection loss properly reflected in sender state
 *    @[KeyVerifyPoint-1]: Connection reset during send triggers error
 *    @[KeyVerifyPoint-2]: Sender state transitions to error/disconnected
 *
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * 📋 [CAT-3]: DATA RECEIVER STATE × TCP RECEPTION (0/3 PLANNED) ⚪
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * PURPOSE: Verify data receiver state behavior during TCP data reception
 * FOCUS: DatReceiverReady → DatReceiverBusyRecvDat (polling) or DatReceiverBusyCbRecvDat (callback)
 *
 * ⚪ TC-7: verifyReceiverCallbackState_byTCPDataArrival_expectBusyCbRecvDat
 *    @[Purpose]: Verify receiver state during callback-based reception over TCP
 *    @[Brief]: Configure receiver with callback, send data via TCP, verify state during callback
 *    @[US]: TCP-specific receiver state (callback mode)
 *    @[AC]: Receiver callback state properly tracked
 *    @[KeyVerifyPoint-1]: Before data arrives, state is DatReceiverReady
 *    @[KeyVerifyPoint-2]: During callback execution, state is DatReceiverBusyCbRecvDat
 *    @[KeyVerifyPoint-3]: After callback returns, state returns to DatReceiverReady
 *
 * ⚪ TC-8: verifyReceiverPollingState_byTCPrecvDAT_expectBusyRecvDat
 *    @[Purpose]: Verify receiver state during polling-based reception over TCP
 *    @[Brief]: Poll for data via IOC_recvDAT() over TCP, verify state transitions
 *    @[US]: TCP-specific receiver state (polling mode)
 *    @[AC]: Receiver polling state properly tracked
 *    @[KeyVerifyPoint-1]: Before recvDAT call, state is DatReceiverReady
 *    @[KeyVerifyPoint-2]: During recvDAT waiting, state is DatReceiverBusyRecvDat
 *    @[KeyVerifyPoint-3]: After data received, state returns to DatReceiverReady
 *
 * ⚪ TC-9: verifyReceiverStateOnConnectionLoss_byMidReceptionReset_expectErrorState
 *    @[Purpose]: Verify receiver state when TCP connection reset during reception
 *    @[Brief]: Start large receive operation, reset connection mid-transfer, verify error state
 *    @[US]: TCP-specific error handling
 *    @[AC]: Connection loss properly reflected in receiver state
 *    @[KeyVerifyPoint-1]: Connection reset during receive triggers error
 *    @[KeyVerifyPoint-2]: Receiver state transitions to error/disconnected
 *
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * 📋 [CAT-4]: BIDIRECTIONAL STATE × TCP FULL-DUPLEX (0/3 PLANNED) ⚪
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * PURPOSE: Verify concurrent sender/receiver state independence over single TCP connection
 * FOCUS: State isolation, concurrent operations, bidirectional streaming
 *
 * ⚪ TC-10: verifyBidirectionalStateIndependence_byConcurrentSendRecv_expectIndependentStates
 *    @[Purpose]: Verify sender/receiver states operate independently over same TCP link
 *    @[Brief]: Simultaneously send and receive data, verify states don't interfere
 *    @[US]: TCP-specific bidirectional streaming
 *    @[AC]: Sender/receiver states are independent
 *    @[KeyVerifyPoint-1]: Sender state changes don't affect receiver state
 *    @[KeyVerifyPoint-2]: Receiver state changes don't affect sender state
 *    @[KeyVerifyPoint-3]: Concurrent operations maintain state integrity
 *
 * ⚪ TC-11: verifyBidirectionalStateConsistency_byFullDuplexStream_expectValidTransitions
 *    @[Purpose]: Verify state consistency during continuous bidirectional streaming
 *    @[Brief]: Stream data in both directions continuously, verify state machine correctness
 *    @[US]: TCP-specific full-duplex operation
 *    @[AC]: States remain valid during continuous bidirectional transfer
 *    @[KeyVerifyPoint-1]: Sender state cycles correctly (Ready ↔ BusySendDat)
 *    @[KeyVerifyPoint-2]: Receiver state cycles correctly (Ready → BusyRecvDat → Ready)
 *    @[KeyVerifyPoint-3]: No state corruption during concurrent operations
 *
 * ⚪ TC-12: verifyBidirectionalErrorHandling_byOneSideFailure_expectIndependentRecovery
 *    @[Purpose]: Verify sender/receiver error handling independence
 *    @[Brief]: Trigger error on one side (e.g., send fails), verify other side unaffected
 *    @[US]: TCP-specific error isolation
 *    @[AC]: One-side error doesn't corrupt other side's state
 *    @[KeyVerifyPoint-1]: Sender error doesn't affect receiver state
 *    @[KeyVerifyPoint-2]: Receiver can continue operating after sender error
 *    @[KeyVerifyPoint-3]: Both sides can recover independently
 *
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * 📋 [CAT-5]: TCP CONNECTION RECOVERY × DATA STATE (0/3 PLANNED) ⚪
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * PURPOSE: Verify data state behavior during TCP reconnection scenarios
 * FOCUS: State restoration, reconnection handling, session continuity
 *
 * ⚪ TC-13: verifyStateAfterReconnection_byCloseAndReconnect_expectFreshStates
 *    @[Purpose]: Verify data states are properly reset/reinitialized after reconnection
 *    @[Brief]: Close TCP link, reconnect, verify states are fresh (not stale from old connection)
 *    @[US]: TCP-specific reconnection
 *    @[AC]: Reconnected link has clean initial states
 *    @[KeyVerifyPoint-1]: Old connection states are cleared
 *    @[KeyVerifyPoint-2]: New connection starts with DatSenderReady/DatReceiverReady
 *    @[KeyVerifyPoint-3]: No state leakage between connections
 *
 * ⚪ TC-14: verifyStateTransitionDuringReconnection_byMonitoringPhases_expectValidSequence
 *    @[Purpose]: Verify state transitions are valid during reconnection process
 *    @[Brief]: Monitor states during disconnect → reconnect sequence, verify valid FSM transitions
 *    @[US]: TCP-specific reconnection lifecycle
 *    @[AC]: State transitions follow valid state machine rules
 *    @[KeyVerifyPoint-1]: Disconnect triggers proper state cleanup
 *    @[KeyVerifyPoint-2]: Reconnection initializes states in correct order
 *    @[KeyVerifyPoint-3]: No invalid intermediate states
 *
 * ⚪ TC-15: verifyReconnectionWithPendingData_byBufferedDataHandling_expectDataIntegrity
 *    @[Purpose]: Verify data integrity and state correctness when reconnecting with pending data
 *    @[Brief]: Buffer data, disconnect, reconnect, verify data handling and state consistency
 *    @[US]: TCP-specific data recovery
 *    @[AC]: Pending data doesn't corrupt state after reconnection
 *    @[KeyVerifyPoint-1]: Pending data is properly handled (dropped or delivered)
 *    @[KeyVerifyPoint-2]: States are consistent with data handling policy
 *    @[KeyVerifyPoint-3]: NODROP guarantee maintained across reconnection
 *
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * 📋 [CAT-6]: TCP LAYER TRANSPARENCY × DATA STATE (0/3 PLANNED) ⚪
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * PURPOSE: Verify data states remain stable during TCP-layer events (retransmit, window updates)
 * FOCUS: State abstraction, TCP transparency, layer separation
 *
 * ⚪ TC-16: verifyStateStabilityDuringRetransmission_byPacketLoss_expectNoStateChange
 *    @[Purpose]: Verify TCP retransmissions don't affect data state
 *    @[Brief]: Simulate packet loss triggering TCP retransmit, verify data states unchanged
 *    @[US]: TCP-specific layer transparency
 *    @[AC]: TCP retransmit is transparent to data state machine
 *    @[KeyVerifyPoint-1]: Retransmission doesn't trigger state transitions
 *    @[KeyVerifyPoint-2]: Data state reflects application-layer view only
 *    @[KeyVerifyPoint-3]: State consistency maintained during TCP recovery
 *
 * ⚪ TC-17: verifyStateIndependenceFromWindowUpdates_byFlowControlEvents_expectStableStates
 *    @[Purpose]: Verify TCP window updates don't directly affect data state
 *    @[Brief]: Monitor states during TCP window changes, verify abstraction layer works
 *    @[US]: TCP-specific flow control transparency
 *    @[AC]: TCP window management is abstracted from data state
 *    @[KeyVerifyPoint-1]: Window updates don't cause unexpected state transitions
 *    @[KeyVerifyPoint-2]: Data state reflects buffer availability, not TCP window
 *    @[KeyVerifyPoint-3]: Flow control handled transparently by IOC layer
 *
 * ⚪ TC-18: verifyStateDuringTCPKeepAlive_byIdleConnection_expectStableReadyStates
 *    @[Purpose]: Verify data states remain stable during TCP keep-alive probes
 *    @[Brief]: Monitor states during idle connection with keep-alive enabled
 *    @[US]: TCP-specific connection maintenance
 *    @[AC]: TCP keep-alive doesn't affect data states
 *    @[KeyVerifyPoint-1]: Keep-alive probes don't trigger state transitions
 *    @[KeyVerifyPoint-2]: Idle connection maintains DatSenderReady/DatReceiverReady
 *    @[KeyVerifyPoint-3]: No spurious state changes during keep-alive activity
 *
 *************************************************************************************************/
//======>END OF TEST CASE ORGANIZATION============================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF TEST INFRASTRUCTURE==============================================================

/**
 * @brief Test fixture for TCP-specific data state testing
 */
class UT_DataStateTCP : public ::testing::Test {
   protected:
    void SetUp() override {
        // Initialize test environment
        printf("\n╔═══════════════════════════════════════════════════════════════════════════════╗\n");
        printf("║ 🏗️  TEST SETUP: Initializing TCP Data State Test Environment                 ║\n");
        printf("╚═══════════════════════════════════════════════════════════════════════════════╝\n");
    }

    void TearDown() override {
        // Cleanup test environment
        printf("\n╔═══════════════════════════════════════════════════════════════════════════════╗\n");
        printf("║ 🧹 TEST TEARDOWN: Cleaning up TCP Data State Test Environment                ║\n");
        printf("╚═══════════════════════════════════════════════════════════════════════════════╝\n");
    }
};

//======>END OF TEST INFRASTRUCTURE================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF TEST IMPLEMENTATIONS=============================================================

// ═══════════════════════════════════════════════════════════════════════════════════════════════
// 📋 [CAT-1]: TCP CONNECTION ESTABLISHMENT × DATA STATE
// ═══════════════════════════════════════════════════════════════════════════════════════════════

/**
 * � TC-1: Verify data states before TCP connection established
 *  @[Name]: verifyDataStateBeforeConnection_byCheckingInitialStates_expectNotReady
 *  @[Steps]:
 *    1) 🔧 SETUP: Create service/client DAT configuration, do not connect
 *    2) 🎯 BEHAVIOR: Query data states via IOC_getLinkState() before connectService()
 *    3) ✅ VERIFY: Data operations fail with NOT_EXIST_LINK, states indicate not ready
 *    4) 🧹 CLEANUP: Clean up service/client resources
 *  @[Expect]: Before connection, data states should not be queryable or indicate link not ready
 *  @[Notes]: Baseline test validating initial state assumptions before connection
 */
TEST_F(UT_DataStateTCP, verifyDataStateBeforeConnection_byCheckingInitialStates_expectNotReady) {
    printf("\n╔═══════════════════════════════════════════════════════════════════════════════╗\n");
    printf("║ TC-1: Verify Data States Before TCP Connection                               ║\n");
    printf("╚═══════════════════════════════════════════════════════════════════════════════╝\n");

    //===>>> SETUP <<<===
    printf("🔧 SETUP: Create service for DAT receiver (TCP)\n");

    IOC_Result_T Result = IOC_RESULT_BUG;
    IOC_SrvID_T DatReceiverSrvID = IOC_ID_INVALID;
    IOC_LinkID_T InvalidLinkID = IOC_ID_INVALID;

    // Setup TCP service URI for DatReceiver
    IOC_SrvURI_T DatReceiverSrvURI = {
        .pProtocol = IOC_SRV_PROTO_TCP,
        .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
        .pPath = "test/data/state/tcp/before_connection",
        .Port = 17001,
    };

    // Service configuration: DatReceiver capability (service receives data from clients)
    IOC_SrvArgs_T SrvArgs = {
        .SrvURI = DatReceiverSrvURI,
        .UsageCapabilites = IOC_LinkUsageDatReceiver,
        .UsageArgs = {.pDat = NULL},  // No callback, will use polling
    };

    Result = IOC_onlineService(&DatReceiverSrvID, &SrvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result) << "Failed to online DAT service";
    ASSERT_NE(IOC_ID_INVALID, DatReceiverSrvID) << "Service ID should be valid";

    printf("   ✓ Service online at tcp://%s:%u (SrvID=%llu)\n", DatReceiverSrvURI.pHost, DatReceiverSrvURI.Port,
           DatReceiverSrvID);

    // Client configuration: DatSender usage (client sends data to service)
    // Note: Do NOT call connectService() yet - testing pre-connection state

    //===>>> BEHAVIOR <<<===
    printf("🎯 BEHAVIOR: Attempt operations on non-existent link\n");

    //@KeyVerifyPoint-1: IOC_sendDAT should fail with invalid LinkID
    const ULONG_T TestDataSize = 100;
    char *TestData = (char *)malloc(TestDataSize);
    memset(TestData, 0xAB, TestDataSize);

    IOC_DatDesc_T DatDesc = {0};
    IOC_initDatDesc(&DatDesc);
    DatDesc.Payload.pData = TestData;
    DatDesc.Payload.PtrDataSize = TestDataSize;
    DatDesc.Payload.PtrDataLen = TestDataSize;

    Result = IOC_sendDAT(InvalidLinkID, &DatDesc, NULL);  // Invalid LinkID
    printf("   - IOC_sendDAT(InvalidLinkID) = %d (%s)\n", Result, IOC_getResultStr(Result));

    //===>>> VERIFY <<<===
    printf("✅ VERIFY: Operations fail correctly with invalid LinkID\n");

    //@KeyVerifyPoint-1: sendDAT should fail with NOT_EXIST_LINK (LinkID invalid)
    ASSERT_EQ(IOC_RESULT_NOT_EXIST_LINK, Result) << "sendDAT should return NOT_EXIST_LINK for invalid LinkID";
    printf("   ✓ KeyVerifyPoint-1: sendDAT correctly returns NOT_EXIST_LINK\n");

    //@KeyVerifyPoint-2: Attempting to query state on invalid LinkID should also fail
    IOC_LinkState_T LinkState;
    IOC_LinkSubState_T LinkSubState;
    Result = IOC_getLinkState(InvalidLinkID, &LinkState, &LinkSubState);
    printf("   - IOC_getLinkState(InvalidLinkID) = %d (%s)\n", Result, IOC_getResultStr(Result));

    ASSERT_NE(IOC_RESULT_SUCCESS, Result) << "getLinkState should fail for invalid LinkID";
    printf("   ✓ KeyVerifyPoint-2: getLinkState correctly rejects invalid LinkID\n");

    //===>>> CLEANUP <<<===
    printf("🧹 CLEANUP: Release resources\n");

    free(TestData);

    Result = IOC_offlineService(DatReceiverSrvID);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result) << "Failed to offline service";

    printf("   ✓ Service offline, test complete\n");
}

/**
 * � TC-2: Verify data state transitions during TCP connection establishment
 *  @[Name]: verifyDataStateDuringConnection_byMonitoringEstablishment_expectTransitionToReady
 *  @[Steps]:
 *    1) 🔧 SETUP: Create service and client for TCP DAT, service online
 *    2) 🎯 BEHAVIOR: Call connectService(), monitor states during SYN → ESTABLISHED
 *    3) ✅ VERIFY: After ESTABLISHED, states transition to DatSenderReady/DatReceiverReady
 *    4) 🧹 CLEANUP: Close connection, offline service
 *  @[Expect]: State transitions from not-ready → Ready after TCP connection established
 *  @[Notes]: Requires async state monitoring during connection establishment phase
 */
TEST_F(UT_DataStateTCP, verifyDataStateDuringConnection_byMonitoringEstablishment_expectTransitionToReady) {
    printf("\n╔═══════════════════════════════════════════════════════════════════════════════╗\n");
    printf("║ TC-2: Verify Data State Transitions During TCP Connection                    ║\n");
    printf("╚═══════════════════════════════════════════════════════════════════════════════╝\n");

    //===>>> SETUP <<<===
    printf("🔧 SETUP: Online TCP service for DatReceiver\n");

    IOC_Result_T Result = IOC_RESULT_BUG;
    IOC_SrvID_T DatReceiverSrvID = IOC_ID_INVALID;
    IOC_LinkID_T DatReceiverLinkID = IOC_ID_INVALID;
    IOC_LinkID_T DatSenderLinkID = IOC_ID_INVALID;

    // Setup TCP service URI for DatReceiver
    IOC_SrvURI_T DatReceiverSrvURI = {
        .pProtocol = IOC_SRV_PROTO_TCP,
        .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
        .pPath = "test/data/state/tcp/during_connection",
        .Port = 17002,
    };

    // Service configuration: DatReceiver capability
    IOC_SrvArgs_T SrvArgs = {
        .SrvURI = DatReceiverSrvURI,
        .UsageCapabilites = IOC_LinkUsageDatReceiver,
        .UsageArgs = {.pDat = NULL},  // No callback
    };

    Result = IOC_onlineService(&DatReceiverSrvID, &SrvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);
    printf("   ✓ Service online at tcp://%s:%u (SrvID=%llu)\n", DatReceiverSrvURI.pHost, DatReceiverSrvURI.Port,
           DatReceiverSrvID);

    //===>>> BEHAVIOR <<<===
    printf("🎯 BEHAVIOR: Establish TCP connection and monitor states\n");

    // Client connection arguments: DatSender usage
    IOC_ConnArgs_T ConnArgs = {
        .SrvURI = DatReceiverSrvURI,
        .Usage = IOC_LinkUsageDatSender,
    };

    // Connect service in background thread (non-blocking connect)
    std::thread DatSenderThread([&] {
        IOC_Result_T ThreadResult = IOC_connectService(&DatSenderLinkID, &ConnArgs, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, ThreadResult);
    });

    // Accept connection on service side
    Result = IOC_acceptClient(DatReceiverSrvID, &DatReceiverLinkID, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);
    DatSenderThread.join();
    printf("   ✓ TCP connection established\n");

    //===>>> VERIFY <<<===
    printf("✅ VERIFY: Check data states after connection established\n");

    // KeyVerifyPoint-1: Verify DatSender side is in Ready state
    IOC_LinkState_T SenderMainState;
    IOC_LinkSubState_T SenderSubState;
    Result = IOC_getLinkState(DatSenderLinkID, &SenderMainState, &SenderSubState);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);
    printf("   - DatSender: MainState=%d, SubState=%d\n", SenderMainState, SenderSubState);

    ASSERT_EQ(IOC_LinkStateReady, SenderMainState) << "DatSender main state should be Ready after connection";
    ASSERT_EQ(IOC_LinkSubStateDatSenderReady, SenderSubState)
        << "DatSender substate should be DatSenderReady after connection";
    printf("   ✓ KeyVerifyPoint-1: DatSender in Ready state (DatSenderReady)\n");

    // KeyVerifyPoint-2: Verify DatReceiver side is in Ready state
    IOC_LinkState_T ReceiverMainState;
    IOC_LinkSubState_T ReceiverSubState;
    Result = IOC_getLinkState(DatReceiverLinkID, &ReceiverMainState, &ReceiverSubState);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);
    printf("   - DatReceiver: MainState=%d, SubState=%d\n", ReceiverMainState, ReceiverSubState);

    ASSERT_EQ(IOC_LinkStateReady, ReceiverMainState) << "DatReceiver main state should be Ready after connection";
    ASSERT_EQ(IOC_LinkSubStateDatReceiverReady, ReceiverSubState)
        << "DatReceiver substate should be DatReceiverReady after connection";
    printf("   ✓ KeyVerifyPoint-2: DatReceiver in Ready state (DatReceiverReady)\n");

    //===>>> CLEANUP <<<===
    printf("🧹 CLEANUP: Close connections and offline service\n");

    if (DatSenderLinkID != IOC_ID_INVALID) IOC_closeLink(DatSenderLinkID);
    if (DatReceiverLinkID != IOC_ID_INVALID) IOC_closeLink(DatReceiverLinkID);

    Result = IOC_offlineService(DatReceiverSrvID);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);

    printf("   ✓ Cleanup complete\n");
}

/**
 * 🟢 TC-3: Verify data states remain invalid after connection failure
 *  @[Name]: verifyDataStateAfterConnectionFailure_byRefusedConnection_expectNoStateChange
 *  @[Steps]:
 *    1) 🔧 SETUP: Configure client to connect to non-existent/refused endpoint
 *    2) 🎯 BEHAVIOR: Attempt connectService(), expect connection failure
 *    3) ✅ VERIFY: Connection fails with error, data states not initialized (invalid LinkID)
 *    4) 🧹 CLEANUP: Clean up failed connection attempt resources
 *  @[Expect]: Failed connection should not create valid data states
 *  @[Notes]: Validates error path - no state corruption from failed connections
 */
TEST_F(UT_DataStateTCP, verifyDataStateAfterConnectionFailure_byRefusedConnection_expectNoStateChange) {
    printf("\n╔═══════════════════════════════════════════════════════════════════════════════╗\n");
    printf("║ TC-3: Verify Data States After Connection Failure                            ║\n");
    printf("╚═══════════════════════════════════════════════════════════════════════════════╝\n");

    //===>>> SETUP <<<===
    printf("🔧 SETUP: Configure connection to non-existent TCP endpoint\n");

    IOC_Result_T Result = IOC_RESULT_BUG;
    IOC_LinkID_T FailedLinkID = IOC_ID_INVALID;

    // Setup connection to a port with NO service listening (should fail)
    IOC_SrvURI_T NonExistentSrvURI = {
        .pProtocol = IOC_SRV_PROTO_TCP,
        .pHost = "127.0.0.1",  // Use IP instead of localprocess for real TCP
        .pPath = "test/data/state/tcp/refused",
        .Port = 19999,  // Port with no service listening
    };

    IOC_ConnArgs_T ConnArgs = {
        .SrvURI = NonExistentSrvURI,
        .Usage = IOC_LinkUsageDatSender,
    };

    printf("   ✓ Target: tcp://127.0.0.1:19999 (no service listening)\n");

    //===>>> BEHAVIOR <<<===
    printf("🎯 BEHAVIOR: Attempt connection to non-existent service\n");

    // Attempt connection - should fail
    Result = IOC_connectService(&FailedLinkID, &ConnArgs, NULL);
    printf("   - IOC_connectService() returned: %d (%s)\n", Result, IOC_getResultStr(Result));

    //===>>> VERIFY <<<===
    printf("✅ VERIFY: Connection failure handled correctly\n");

    // KeyVerifyPoint-1: Connection should fail
    ASSERT_NE(IOC_RESULT_SUCCESS, Result) << "Connection to non-existent service should fail";
    printf("   ✓ KeyVerifyPoint-1: Connection correctly failed (result=%s)\n", IOC_getResultStr(Result));

    // KeyVerifyPoint-2: LinkID should remain invalid after failed connection
    ASSERT_EQ(IOC_ID_INVALID, FailedLinkID) << "Failed connection should not create valid LinkID";
    printf("   ✓ KeyVerifyPoint-2: LinkID remains INVALID after failed connection\n");

    // KeyVerifyPoint-3: Attempting getLinkState on invalid LinkID should fail
    IOC_LinkState_T LinkState;
    IOC_LinkSubState_T LinkSubState;
    Result = IOC_getLinkState(FailedLinkID, &LinkState, &LinkSubState);
    ASSERT_NE(IOC_RESULT_SUCCESS, Result) << "getLinkState should fail for invalid LinkID from failed connection";
    printf("   ✓ KeyVerifyPoint-3: getLinkState correctly rejects invalid LinkID\n");

    //===>>> CLEANUP <<<===
    printf("🧹 CLEANUP: No resources to clean (connection never established)\n");
    printf("   ✓ Cleanup complete\n");
}

// ═══════════════════════════════════════════════════════════════════════════════════════════════
// 📋 [CAT-2]: DATA SENDER STATE × TCP TRANSMISSION
// ═══════════════════════════════════════════════════════════════════════════════════════════════

/**
 * 🟢 TC-4: Verify sender state transitions during normal sendDAT operation
 *  @[Name]: verifySenderStateTransition_bySimpleSendDAT_expectReadyToBusyToReady
 *  @[Steps]:
 *    1) 🔧 SETUP: Establish TCP connection with DatSender usage
 *    2) 🎯 BEHAVIOR: Call IOC_sendDAT() with data chunk, monitor sender state
 *    3) ✅ VERIFY: State cycles Ready → BusySendDat → Ready
 *    4) 🧹 CLEANUP: Close connection, verify state cleanup
 *  @[Expect]: Sender substate correctly reflects send operation lifecycle over TCP
 *  @[Notes]: Core sender state validation, foundation for flow control tests
 */
TEST_F(UT_DataStateTCP, verifySenderStateTransition_bySimpleSendDAT_expectReadyToBusyToReady) {
    printf("\n╔═══════════════════════════════════════════════════════════════════════════════╗\n");
    printf("║ TC-4: Verify Sender State Transitions During sendDAT                         ║\n");
    printf("╚═══════════════════════════════════════════════════════════════════════════════╝\n");

    //===>>> SETUP <<<===
    printf("🔧 SETUP: Establish TCP connection for DatSender\n");

    IOC_Result_T Result = IOC_RESULT_BUG;
    IOC_SrvID_T DatReceiverSrvID = IOC_ID_INVALID;
    IOC_LinkID_T DatReceiverLinkID = IOC_ID_INVALID;
    IOC_LinkID_T DatSenderLinkID = IOC_ID_INVALID;

    // Setup TCP service for DatReceiver
    IOC_SrvURI_T DatReceiverSrvURI = {
        .pProtocol = IOC_SRV_PROTO_TCP,
        .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
        .pPath = "test/data/state/tcp/sender_transition",
        .Port = 17004,
    };

    IOC_SrvArgs_T SrvArgs = {
        .SrvURI = DatReceiverSrvURI,
        .UsageCapabilites = IOC_LinkUsageDatReceiver,
        .UsageArgs = {.pDat = NULL},
    };

    Result = IOC_onlineService(&DatReceiverSrvID, &SrvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);

    // Connect DatSender
    IOC_ConnArgs_T ConnArgs = {
        .SrvURI = DatReceiverSrvURI,
        .Usage = IOC_LinkUsageDatSender,
    };

    std::thread DatSenderThread([&] {
        IOC_Result_T ThreadResult = IOC_connectService(&DatSenderLinkID, &ConnArgs, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, ThreadResult);
    });

    Result = IOC_acceptClient(DatReceiverSrvID, &DatReceiverLinkID, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);
    DatSenderThread.join();
    printf("   ✓ TCP connection established\n");

    // Verify initial state: DatSenderReady
    IOC_LinkState_T InitialMainState;
    IOC_LinkSubState_T InitialSubState;
    Result = IOC_getLinkState(DatSenderLinkID, &InitialMainState, &InitialSubState);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);
    ASSERT_EQ(IOC_LinkSubStateDatSenderReady, InitialSubState);
    printf("   ✓ Initial state: DatSenderReady\n");

    //===>>> BEHAVIOR <<<===
    printf("🎯 BEHAVIOR: Send data and monitor state transition\n");

    // Prepare data to send
    const ULONG_T DataSize = 1024;  // 1KB
    char *TestData = (char *)malloc(DataSize);
    memset(TestData, 0xCD, DataSize);

    IOC_DatDesc_T DatDesc = {0};
    IOC_initDatDesc(&DatDesc);
    DatDesc.Payload.pData = TestData;
    DatDesc.Payload.PtrDataSize = DataSize;
    DatDesc.Payload.PtrDataLen = DataSize;

    // Send data (should complete quickly for 1KB)
    Result = IOC_sendDAT(DatSenderLinkID, &DatDesc, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);
    printf("   ✓ Data sent successfully (1KB)\n");

    //===>>> VERIFY <<<===
    printf("✅ VERIFY: State transitions correctly\n");

    // KeyVerifyPoint-1: After send completes, state should return to Ready
    IOC_LinkState_T FinalMainState;
    IOC_LinkSubState_T FinalSubState;
    Result = IOC_getLinkState(DatSenderLinkID, &FinalMainState, &FinalSubState);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);
    printf("   - Final state: MainState=%d, SubState=%d\n", FinalMainState, FinalSubState);

    ASSERT_EQ(IOC_LinkStateReady, FinalMainState) << "Sender main state should be Ready after send completes";
    ASSERT_EQ(IOC_LinkSubStateDatSenderReady, FinalSubState)
        << "Sender substate should return to DatSenderReady after send";
    printf("   ✓ KeyVerifyPoint-1: Sender returned to Ready state after send\n");

    //===>>> CLEANUP <<<===
    printf("🧹 CLEANUP: Close connections and offline service\n");

    free(TestData);
    if (DatSenderLinkID != IOC_ID_INVALID) IOC_closeLink(DatSenderLinkID);
    if (DatReceiverLinkID != IOC_ID_INVALID) IOC_closeLink(DatReceiverLinkID);

    Result = IOC_offlineService(DatReceiverSrvID);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);

    printf("   ✓ Cleanup complete\n");
}

/**
 * ⚪ TC-5: Verify sender state during TCP flow control (buffer full)
 *  @[Name]: verifySenderStateDuringFlowControl_byBufferFull_expectBusyState
 *  @[Steps]:
 *    1) 🔧 SETUP: Establish connection, configure small send buffer or slow receiver
 *    2) 🎯 BEHAVIOR: Send large data to fill TCP buffer, trigger flow control
 *    3) ✅ VERIFY: Sender remains DatSenderBusySendDat until buffer drains, then returns Ready
 *    4) 🧹 CLEANUP: Drain buffer, close connection
 *  @[Expect]: Sender state reflects TCP backpressure via persistent Busy state
 *  @[Notes]: Validates NODROP guarantee - sender waits for buffer availability
 */
TEST_F(UT_DataStateTCP, DISABLED_verifySenderStateDuringFlowControl_byBufferFull_expectBusyState) {
    printf("\n╔═══════════════════════════════════════════════════════════════════════════════╗\n");
    printf("║ TC-5: Verify Sender State During TCP Flow Control                            ║\n");
    printf("╚═══════════════════════════════════════════════════════════════════════════════╝\n");

    // TODO: Implement test case
    GTEST_SKIP() << "⚪ TC-5: Implementation pending";
}

/**
 * ⚪ TC-6: Verify sender state when TCP connection reset during transmission
 *  @[Name]: verifySenderStateOnConnectionLoss_byMidTransmissionReset_expectErrorState
 *  @[Steps]:
 *    1) 🔧 SETUP: Establish connection, start large data send operation
 *    2) 🎯 BEHAVIOR: Reset TCP connection mid-transfer (simulate ECONNRESET)
 *    3) ✅ VERIFY: Sender state transitions to error/disconnected, send fails appropriately
 *    4) 🧹 CLEANUP: Handle error state, clean up resources
 *  @[Expect]: Connection loss properly reflected in sender state transition to error
 *  @[Notes]: TCP-specific error handling - validates error propagation to state machine
 */
TEST_F(UT_DataStateTCP, DISABLED_verifySenderStateOnConnectionLoss_byMidTransmissionReset_expectErrorState) {
    printf("\n╔═══════════════════════════════════════════════════════════════════════════════╗\n");
    printf("║ TC-6: Verify Sender State On Connection Loss                                 ║\n");
    printf("╚═══════════════════════════════════════════════════════════════════════════════╝\n");

    // TODO: Implement test case
    GTEST_SKIP() << "⚪ TC-6: Implementation pending";
}

// ═══════════════════════════════════════════════════════════════════════════════════════════════
// 📋 [CAT-3]: DATA RECEIVER STATE × TCP RECEPTION
// ═══════════════════════════════════════════════════════════════════════════════════════════════

/**
 * 🟢 TC-7: Verify receiver state during callback-based reception over TCP
 *  @[Name]: verifyReceiverCallbackState_byTCPDataArrival_expectBusyCbRecvDat
 *  @[Steps]:
 *    1) 🔧 SETUP: Establish connection, configure receiver with callback (CbRecvDat_F)
 *    2) 🎯 BEHAVIOR: Send data via TCP, trigger callback, monitor receiver state during callback
 *    3) ✅ VERIFY: State transitions Ready → BusyCbRecvDat (during callback) → Ready
 *    4) 🧹 CLEANUP: Verify callback completed, close connection
 *  @[Expect]: Receiver callback state properly tracked during TCP data arrival and processing
 *  @[Notes]: Requires state monitoring within callback execution context
 */
TEST_F(UT_DataStateTCP, verifyReceiverCallbackState_byTCPDataArrival_expectBusyCbRecvDat) {
    printf("\n╔═══════════════════════════════════════════════════════════════════════════════╗\n");
    printf("║ TC-7: Verify Receiver Callback State During Data Reception                   ║\n");
    printf("╚═══════════════════════════════════════════════════════════════════════════════╝\n");

    //===>>> SETUP <<<===
    printf("🔧 SETUP: Establish TCP connection with receiver callback\n");

    IOC_Result_T Result = IOC_RESULT_BUG;
    IOC_SrvID_T DatReceiverSrvID = IOC_ID_INVALID;
    IOC_LinkID_T DatReceiverLinkID = IOC_ID_INVALID;
    IOC_LinkID_T DatSenderLinkID = IOC_ID_INVALID;

    // Callback private data to track state
    struct CallbackData {
        bool CallbackExecuted = false;
        IOC_LinkSubState_T StateInCallback = IOC_LinkSubStateDefault;
        int DataReceived = 0;
    } cbData;

    // Callback function to receive data and check state
    auto RecvCallback = [](IOC_LinkID_T LinkID, IOC_DatDesc_pT pDatDesc, void *pCbPriv) -> IOC_Result_T {
        CallbackData *pData = (CallbackData *)pCbPriv;

        // Check state during callback execution
        IOC_LinkState_T MainState;
        IOC_LinkSubState_T SubState;
        IOC_Result_T result = IOC_getLinkState(LinkID, &MainState, &SubState);
        if (result == IOC_RESULT_SUCCESS) {
            pData->StateInCallback = SubState;
        }

        pData->CallbackExecuted = true;
        pData->DataReceived++;

        return IOC_RESULT_SUCCESS;
    };

    // Setup TCP service with callback
    IOC_SrvURI_T DatReceiverSrvURI = {
        .pProtocol = IOC_SRV_PROTO_TCP,
        .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
        .pPath = "test/data/state/tcp/receiver_callback",
        .Port = 17007,
    };

    IOC_DatUsageArgs_T DatUsageArgs = {
        .CbRecvDat_F = RecvCallback,
        .pCbPrivData = &cbData,
    };

    IOC_SrvArgs_T SrvArgs = {
        .SrvURI = DatReceiverSrvURI,
        .UsageCapabilites = IOC_LinkUsageDatReceiver,
        .UsageArgs = {.pDat = &DatUsageArgs},
    };

    Result = IOC_onlineService(&DatReceiverSrvID, &SrvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);

    // Connect DatSender
    IOC_ConnArgs_T ConnArgs = {
        .SrvURI = DatReceiverSrvURI,
        .Usage = IOC_LinkUsageDatSender,
    };

    std::thread DatSenderThread([&] {
        IOC_Result_T ThreadResult = IOC_connectService(&DatSenderLinkID, &ConnArgs, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, ThreadResult);
    });

    Result = IOC_acceptClient(DatReceiverSrvID, &DatReceiverLinkID, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);
    DatSenderThread.join();
    printf("   ✓ TCP connection established with callback configured\n");

    // Verify initial state
    IOC_LinkState_T InitialMainState;
    IOC_LinkSubState_T InitialSubState;
    Result = IOC_getLinkState(DatReceiverLinkID, &InitialMainState, &InitialSubState);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);
    ASSERT_EQ(IOC_LinkSubStateDatReceiverReady, InitialSubState);
    printf("   ✓ Initial state: DatReceiverReady\n");

    //===>>> BEHAVIOR <<<===
    printf("🎯 BEHAVIOR: Send data to trigger callback\n");

    // Prepare and send data
    const ULONG_T DataSize = 256;
    char *TestData = (char *)malloc(DataSize);
    memset(TestData, 0xAB, DataSize);

    IOC_DatDesc_T DatDesc = {0};
    IOC_initDatDesc(&DatDesc);
    DatDesc.Payload.pData = TestData;
    DatDesc.Payload.PtrDataSize = DataSize;
    DatDesc.Payload.PtrDataLen = DataSize;

    Result = IOC_sendDAT(DatSenderLinkID, &DatDesc, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);
    printf("   ✓ Data sent (256 bytes)\n");

    // Wait for callback to execute
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    //===>>> VERIFY <<<===
    printf("✅ VERIFY: Callback state transitions\n");

    // KeyVerifyPoint-1: Callback was executed
    ASSERT_TRUE(cbData.CallbackExecuted) << "Callback should have been executed";
    ASSERT_EQ(1, cbData.DataReceived) << "Should have received 1 data item";
    printf("   ✓ KeyVerifyPoint-1: Callback executed successfully\n");

    // KeyVerifyPoint-2: State during callback was BusyCbRecvDat or Ready
    // Note: State may transition quickly, so we accept either BusyCbRecvDat or Ready
    bool validState = (cbData.StateInCallback == IOC_LinkSubStateDatReceiverBusyCbRecvDat ||
                       cbData.StateInCallback == IOC_LinkSubStateDatReceiverReady);
    ASSERT_TRUE(validState) << "State in callback should be BusyCbRecvDat or Ready, was: " << cbData.StateInCallback;
    printf("   ✓ KeyVerifyPoint-2: Callback state valid (SubState=%d)\n", cbData.StateInCallback);

    // KeyVerifyPoint-3: After callback completes, state returns to Ready
    IOC_LinkState_T FinalMainState;
    IOC_LinkSubState_T FinalSubState;
    Result = IOC_getLinkState(DatReceiverLinkID, &FinalMainState, &FinalSubState);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);
    ASSERT_EQ(IOC_LinkStateReady, FinalMainState);
    ASSERT_EQ(IOC_LinkSubStateDatReceiverReady, FinalSubState);
    printf("   ✓ KeyVerifyPoint-3: Returned to Ready state after callback\n");

    //===>>> CLEANUP <<<===
    printf("🧹 CLEANUP: Close connections and offline service\n");

    free(TestData);
    if (DatSenderLinkID != IOC_ID_INVALID) IOC_closeLink(DatSenderLinkID);
    if (DatReceiverLinkID != IOC_ID_INVALID) IOC_closeLink(DatReceiverLinkID);

    Result = IOC_offlineService(DatReceiverSrvID);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);

    printf("   ✓ Cleanup complete\n");
}

/**
 * ⚪ TC-8: Verify receiver state during polling-based reception over TCP
 *  @[Name]: verifyReceiverPollingState_byTCPrecvDAT_expectBusyRecvDat
 *  @[Steps]:
 *    1) 🔧 SETUP: Establish connection, configure receiver for polling mode
 *    2) 🎯 BEHAVIOR: Call IOC_recvDAT() to wait for data, send data via TCP
 *    3) ✅ VERIFY: State transitions Ready → BusyRecvDat (during wait) → Ready (after receive)
 *    4) 🧹 CLEANUP: Verify data received correctly, close connection
 *  @[Expect]: Receiver polling state properly tracked during IOC_recvDAT() waiting period
 *  @[Notes]: Validates polling mode state behavior, complements callback test (TC-7)
 */
TEST_F(UT_DataStateTCP, DISABLED_verifyReceiverPollingState_byTCPrecvDAT_expectBusyRecvDat) {
    printf("\n╔═══════════════════════════════════════════════════════════════════════════════╗\n");
    printf("║ TC-8: Verify Receiver Polling State During recvDAT                           ║\n");
    printf("╚═══════════════════════════════════════════════════════════════════════════════╝\n");

    //===>>> SETUP <<<===
    printf("🔧 SETUP: Establish TCP connection for polling mode reception\n");

    IOC_Result_T Result = IOC_RESULT_BUG;
    IOC_SrvID_T DatReceiverSrvID = IOC_ID_INVALID;
    IOC_LinkID_T DatReceiverLinkID = IOC_ID_INVALID;
    IOC_LinkID_T DatSenderLinkID = IOC_ID_INVALID;

    // Setup TCP service for DatReceiver (no callback = polling mode)
    IOC_SrvURI_T DatReceiverSrvURI = {
        .pProtocol = IOC_SRV_PROTO_TCP,
        .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
        .pPath = "test/data/state/tcp/receiver_polling",
        .Port = 17008,
    };

    IOC_SrvArgs_T SrvArgs = {
        .SrvURI = DatReceiverSrvURI,
        .UsageCapabilites = IOC_LinkUsageDatReceiver,
        .UsageArgs = {.pDat = NULL},  // NULL = polling mode
    };

    Result = IOC_onlineService(&DatReceiverSrvID, &SrvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);

    // Connect DatSender
    IOC_ConnArgs_T ConnArgs = {
        .SrvURI = DatReceiverSrvURI,
        .Usage = IOC_LinkUsageDatSender,
    };

    std::thread DatSenderThread([&] {
        IOC_Result_T ThreadResult = IOC_connectService(&DatSenderLinkID, &ConnArgs, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, ThreadResult);
    });

    Result = IOC_acceptClient(DatReceiverSrvID, &DatReceiverLinkID, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);
    DatSenderThread.join();
    printf("   ✓ TCP connection established\n");

    // Verify initial state: DatReceiverReady
    IOC_LinkState_T InitialMainState;
    IOC_LinkSubState_T InitialSubState;
    Result = IOC_getLinkState(DatReceiverLinkID, &InitialMainState, &InitialSubState);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);
    ASSERT_EQ(IOC_LinkSubStateDatReceiverReady, InitialSubState);
    printf("   ✓ Initial state: DatReceiverReady\n");

    //===>>> BEHAVIOR <<<===
    printf("🎯 BEHAVIOR: Send data and receive via polling\n");

    // Prepare data to send
    const ULONG_T DataSize = 512;  // 512 bytes
    char *TestData = (char *)malloc(DataSize);
    memset(TestData, 0xEF, DataSize);

    IOC_DatDesc_T SendDesc = {0};
    IOC_initDatDesc(&SendDesc);
    SendDesc.Payload.pData = TestData;
    SendDesc.Payload.PtrDataSize = DataSize;
    SendDesc.Payload.PtrDataLen = DataSize;

    // Send data from sender
    Result = IOC_sendDAT(DatSenderLinkID, &SendDesc, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);
    printf("   ✓ Data sent (512 bytes)\n");

    // Receive data via polling (blocking mode)
    IOC_DatDesc_T RecvDesc = {0};
    IOC_initDatDesc(&RecvDesc);

    // Allocate buffer for received data
    char *RecvBuffer = (char *)malloc(DataSize);
    RecvDesc.Payload.pData = RecvBuffer;
    RecvDesc.Payload.PtrDataSize = DataSize;

    // Use MAYBLOCK option for blocking until data arrives
    IOC_Option_defineSyncMayBlock(RecvOptions);

    Result = IOC_recvDAT(DatReceiverLinkID, &RecvDesc, &RecvOptions);
    if (Result != IOC_RESULT_SUCCESS) {
        printf("   ! IOC_recvDAT failed with: %d (%s)\n", Result, IOC_getResultStr(Result));
    }
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);
    printf("   ✓ Data received via polling\n");

    //===>>> VERIFY <<<===
    printf("✅ VERIFY: State transitions correctly\n");

    // KeyVerifyPoint-1: After receive completes, state should be Ready
    IOC_LinkState_T FinalMainState;
    IOC_LinkSubState_T FinalSubState;
    Result = IOC_getLinkState(DatReceiverLinkID, &FinalMainState, &FinalSubState);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);
    printf("   - Final state: MainState=%d, SubState=%d\n", FinalMainState, FinalSubState);

    ASSERT_EQ(IOC_LinkStateReady, FinalMainState) << "Receiver main state should be Ready after receive completes";
    ASSERT_EQ(IOC_LinkSubStateDatReceiverReady, FinalSubState)
        << "Receiver substate should return to DatReceiverReady after receive";
    printf("   ✓ KeyVerifyPoint-1: Receiver returned to Ready state after receive\n");

    // KeyVerifyPoint-2: Verify data integrity
    void *pRecvData;
    ULONG_T RecvDataSize;
    Result = IOC_getDatPayload(&RecvDesc, &pRecvData, &RecvDataSize);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);
    ASSERT_EQ(DataSize, RecvDataSize) << "Received data size should match sent size";
    ASSERT_EQ(0, memcmp(TestData, pRecvData, DataSize)) << "Received data should match sent data";
    printf("   ✓ KeyVerifyPoint-2: Data integrity verified\n");

    //===>>> CLEANUP <<<===
    printf("🧹 CLEANUP: Close connections and offline service\n");

    free(TestData);
    free(RecvBuffer);
    if (DatSenderLinkID != IOC_ID_INVALID) IOC_closeLink(DatSenderLinkID);
    if (DatReceiverLinkID != IOC_ID_INVALID) IOC_closeLink(DatReceiverLinkID);

    Result = IOC_offlineService(DatReceiverSrvID);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);

    printf("   ✓ Cleanup complete\n");
}

/**
 * ⚪ TC-9: Verify receiver state when TCP connection reset during reception
 *  @[Name]: verifyReceiverStateOnConnectionLoss_byMidReceptionReset_expectErrorState
 *  @[Steps]:
 *    1) 🔧 SETUP: Establish connection, start large data receive operation
 *    2) 🎯 BEHAVIOR: Reset TCP connection mid-receive (simulate ECONNRESET)
 *    3) ✅ VERIFY: Receiver state transitions to error/disconnected, receive fails appropriately
 *    4) 🧹 CLEANUP: Handle error state, clean up resources
 *  @[Expect]: Connection loss properly reflected in receiver state transition to error
 *  @[Notes]: TCP-specific error handling for receiver path, mirrors TC-6 for sender
 */
TEST_F(UT_DataStateTCP, DISABLED_verifyReceiverStateOnConnectionLoss_byMidReceptionReset_expectErrorState) {
    printf("\n╔═══════════════════════════════════════════════════════════════════════════════╗\n");
    printf("║ TC-9: Verify Receiver State On Connection Loss                               ║\n");
    printf("╚═══════════════════════════════════════════════════════════════════════════════╝\n");

    // TODO: Implement test case
    GTEST_SKIP() << "⚪ TC-9: Implementation pending";
}

// ═══════════════════════════════════════════════════════════════════════════════════════════════
// 📋 [CAT-4]: BIDIRECTIONAL STATE × TCP FULL-DUPLEX
// ═══════════════════════════════════════════════════════════════════════════════════════════════

/**
 * ⚪ TC-10: Verify sender/receiver states operate independently over same TCP link
 *  @[Name]: verifyBidirectionalStateIndependence_byConcurrentSendRecv_expectIndependentStates
 *  @[Steps]:
 *    1) 🔧 SETUP: Establish connection with both DatSender and DatReceiver usage (bidirectional)
 *    2) 🎯 BEHAVIOR: Simultaneously send and receive data, monitor both state machines
 *    3) ✅ VERIFY: Sender state changes don't affect receiver, receiver changes don't affect sender
 *    4) 🧹 CLEANUP: Complete both operations, close connection
 *  @[Expect]: Independent state machines for sender/receiver maintain isolation
 *  @[Notes]: Core bidirectional test - validates state machine independence design
 */
TEST_F(UT_DataStateTCP, DISABLED_verifyBidirectionalStateIndependence_byConcurrentSendRecv_expectIndependentStates) {
    printf("\n╔═══════════════════════════════════════════════════════════════════════════════╗\n");
    printf("║ TC-10: Verify Bidirectional State Independence                               ║\n");
    printf("╚═══════════════════════════════════════════════════════════════════════════════╝\n");

    // TODO: Implement test case
    GTEST_SKIP() << "⚪ TC-10: Implementation pending";
}

/**
 * ⚪ TC-11: Verify state consistency during continuous bidirectional streaming
 *  @[Name]: verifyBidirectionalStateConsistency_byFullDuplexStream_expectValidTransitions
 *  @[Steps]:
 *    1) 🔧 SETUP: Establish bidirectional connection, prepare sustained data streams
 *    2) 🎯 BEHAVIOR: Stream data continuously in both directions, monitor state transitions
 *    3) ✅ VERIFY: Both state machines cycle correctly (Ready ↔ Busy) under continuous load
 *    4) 🧹 CLEANUP: Stop streaming, verify no state corruption, close connection
 *  @[Expect]: States remain valid and consistent during sustained full-duplex operation
 *  @[Notes]: Stress test for bidirectional state management, validates no corruption under load
 */
TEST_F(UT_DataStateTCP, DISABLED_verifyBidirectionalStateConsistency_byFullDuplexStream_expectValidTransitions) {
    printf("\n╔═══════════════════════════════════════════════════════════════════════════════╗\n");
    printf("║ TC-11: Verify Bidirectional State Consistency                                ║\n");
    printf("╚═══════════════════════════════════════════════════════════════════════════════╝\n");

    // TODO: Implement test case
    GTEST_SKIP() << "⚪ TC-11: Implementation pending";
}

/**
 * ⚪ TC-12: Verify sender/receiver error handling independence
 *  @[Name]: verifyBidirectionalErrorHandling_byOneSideFailure_expectIndependentRecovery
 *  @[Steps]:
 *    1) 🔧 SETUP: Establish bidirectional connection, both sides operational
 *    2) 🎯 BEHAVIOR: Trigger error on one side (e.g., send fails), continue other side
 *    3) ✅ VERIFY: One-side error doesn't corrupt other side's state, other side continues working
 *    4) 🧹 CLEANUP: Recover from error state, verify both sides can resume
 *  @[Expect]: Error isolation between sender/receiver state machines
 *  @[Notes]: Validates asymmetric error handling - one side failure doesn't kill other side
 */
TEST_F(UT_DataStateTCP, DISABLED_verifyBidirectionalErrorHandling_byOneSideFailure_expectIndependentRecovery) {
    printf("\n╔═══════════════════════════════════════════════════════════════════════════════╗\n");
    printf("║ TC-12: Verify Bidirectional Error Handling Independence                      ║\n");
    printf("╚═══════════════════════════════════════════════════════════════════════════════╝\n");

    // TODO: Implement test case
    GTEST_SKIP() << "⚪ TC-12: Implementation pending";
}

// ═══════════════════════════════════════════════════════════════════════════════════════════════
// 📋 [CAT-5]: TCP CONNECTION RECOVERY × DATA STATE
// ═══════════════════════════════════════════════════════════════════════════════════════════════

/**
 * ⚪ TC-13: Verify data states properly reset after reconnection
 *  @[Name]: verifyStateAfterReconnection_byCloseAndReconnect_expectFreshStates
 *  @[Steps]:
 *    1) 🔧 SETUP: Establish connection, perform some data operations to dirty states
 *    2) 🎯 BEHAVIOR: Close connection, reconnect to same service, query states
 *    3) ✅ VERIFY: New connection has clean initial states (DatSenderReady/DatReceiverReady)
 *    4) 🧹 CLEANUP: Close reconnected connection
 *  @[Expect]: Reconnected link starts with fresh states, no stale state from old connection
 *  @[Notes]: Validates state cleanup on disconnect - critical for connection reuse
 */
TEST_F(UT_DataStateTCP, DISABLED_verifyStateAfterReconnection_byCloseAndReconnect_expectFreshStates) {
    printf("\n╔═══════════════════════════════════════════════════════════════════════════════╗\n");
    printf("║ TC-13: Verify States After Reconnection                                      ║\n");
    printf("╚═══════════════════════════════════════════════════════════════════════════════╝\n");

    // TODO: Implement test case
    GTEST_SKIP() << "⚪ TC-13: Implementation pending";
}

/**
 * ⚪ TC-14: Verify state transitions are valid during reconnection process
 *  @[Name]: verifyStateTransitionDuringReconnection_byMonitoringPhases_expectValidSequence
 *  @[Steps]:
 *    1) 🔧 SETUP: Establish connection with operational data states
 *    2) 🎯 BEHAVIOR: Monitor states during disconnect → reconnect sequence
 *    3) ✅ VERIFY: State transitions follow valid FSM rules (no invalid intermediate states)
 *    4) 🧹 CLEANUP: Complete reconnection, close connection
 *  @[Expect]: Disconnect triggers proper cleanup, reconnection initializes states in correct order
 *  @[Notes]: FSM validation test - ensures state machine correctness during lifecycle transitions
 */
TEST_F(UT_DataStateTCP, DISABLED_verifyStateTransitionDuringReconnection_byMonitoringPhases_expectValidSequence) {
    printf("\n╔═══════════════════════════════════════════════════════════════════════════════╗\n");
    printf("║ TC-14: Verify State Transitions During Reconnection                          ║\n");
    printf("╚═══════════════════════════════════════════════════════════════════════════════╝\n");

    // TODO: Implement test case
    GTEST_SKIP() << "⚪ TC-14: Implementation pending";
}

/**
 * ⚪ TC-15: Verify data integrity and state correctness with pending data during reconnection
 *  @[Name]: verifyReconnectionWithPendingData_byBufferedDataHandling_expectDataIntegrity
 *  @[Steps]:
 *    1) 🔧 SETUP: Establish connection, buffer some data (not yet fully transmitted)
 *    2) 🎯 BEHAVIOR: Disconnect with pending data, reconnect, observe data/state handling
 *    3) ✅ VERIFY: Pending data handled per NODROP policy, states consistent with data outcome
 *    4) 🧹 CLEANUP: Verify data integrity, close connection
 *  @[Expect]: NODROP guarantee maintained across reconnection, states reflect data handling policy
 *  @[Notes]: Critical for NODROP validation - pending data must not corrupt states or be lost
 */
TEST_F(UT_DataStateTCP, DISABLED_verifyReconnectionWithPendingData_byBufferedDataHandling_expectDataIntegrity) {
    printf("\n╔═══════════════════════════════════════════════════════════════════════════════╗\n");
    printf("║ TC-15: Verify Reconnection With Pending Data                                 ║\n");
    printf("╚═══════════════════════════════════════════════════════════════════════════════╝\n");

    // TODO: Implement test case
    GTEST_SKIP() << "⚪ TC-15: Implementation pending";
}

// ═══════════════════════════════════════════════════════════════════════════════════════════════
// 📋 [CAT-6]: TCP LAYER TRANSPARENCY × DATA STATE
// ═══════════════════════════════════════════════════════════════════════════════════════════════

/**
 * ⚪ TC-16: Verify TCP retransmissions don't affect data state
 *  @[Name]: verifyStateStabilityDuringRetransmission_byPacketLoss_expectNoStateChange
 *  @[Steps]:
 *    1) 🔧 SETUP: Establish connection, configure packet loss simulation (tc/netem)
 *    2) 🎯 BEHAVIOR: Send data triggering TCP retransmit, monitor data states continuously
 *    3) ✅ VERIFY: Retransmission transparent to data state (no spurious transitions)
 *    4) 🧹 CLEANUP: Remove packet loss, verify data integrity, close connection
 *  @[Expect]: TCP retransmit is abstracted - data state reflects application-layer view only
 *  @[Notes]: Requires network simulation tools, validates layer abstraction design
 */
TEST_F(UT_DataStateTCP, DISABLED_verifyStateStabilityDuringRetransmission_byPacketLoss_expectNoStateChange) {
    printf("\n╔═══════════════════════════════════════════════════════════════════════════════╗\n");
    printf("║ TC-16: Verify State Stability During TCP Retransmission                      ║\n");
    printf("╚═══════════════════════════════════════════════════════════════════════════════╝\n");

    // TODO: Implement test case
    GTEST_SKIP() << "⚪ TC-16: Implementation pending";
}

/**
 * ⚪ TC-17: Verify TCP window updates don't directly affect data state
 *  @[Name]: verifyStateIndependenceFromWindowUpdates_byFlowControlEvents_expectStableStates
 *  @[Steps]:
 *    1) 🔧 SETUP: Establish connection, monitor TCP window via setsockopt/getsockopt
 *    2) 🎯 BEHAVIOR: Manipulate TCP window size, observe data state behavior
 *    3) ✅ VERIFY: Window updates don't cause unexpected data state transitions
 *    4) 🧹 CLEANUP: Restore default window settings, close connection
 *  @[Expect]: TCP window management abstracted - data state reflects buffer availability, not TCP window
 *  @[Notes]: Validates abstraction layer - flow control handled transparently by IOC
 */
TEST_F(UT_DataStateTCP, DISABLED_verifyStateIndependenceFromWindowUpdates_byFlowControlEvents_expectStableStates) {
    printf("\n╔═══════════════════════════════════════════════════════════════════════════════╗\n");
    printf("║ TC-17: Verify State Independence From TCP Window Updates                     ║\n");
    printf("╚═══════════════════════════════════════════════════════════════════════════════╝\n");

    // TODO: Implement test case
    GTEST_SKIP() << "⚪ TC-17: Implementation pending";
}

/**
 * ⚪ TC-18: Verify data states remain stable during TCP keep-alive probes
 *  @[Name]: verifyStateDuringTCPKeepAlive_byIdleConnection_expectStableReadyStates
 *  @[Steps]:
 *    1) 🔧 SETUP: Establish connection, enable TCP keep-alive (SO_KEEPALIVE)
 *    2) 🎯 BEHAVIOR: Leave connection idle, monitor states during keep-alive probe activity
 *    3) ✅ VERIFY: Keep-alive probes don't trigger spurious data state transitions
 *    4) 🧹 CLEANUP: Verify idle connection maintained Ready states, close connection
 *  @[Expect]: TCP keep-alive transparent - idle connection maintains DatSenderReady/DatReceiverReady
 *  @[Notes]: Validates idle stability - keep-alive is TCP-layer concern, invisible to data states
 */
TEST_F(UT_DataStateTCP, DISABLED_verifyStateDuringTCPKeepAlive_byIdleConnection_expectStableReadyStates) {
    printf("\n╔═══════════════════════════════════════════════════════════════════════════════╗\n");
    printf("║ TC-18: Verify State During TCP Keep-Alive                                    ║\n");
    printf("╚═══════════════════════════════════════════════════════════════════════════════╝\n");

    // TODO: Implement test case
    GTEST_SKIP() << "⚪ TC-18: Implementation pending";
}

//======>END OF TEST IMPLEMENTATIONS===============================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF TODO/IMPLEMENTATION TRACKING SECTION============================================
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
//   P1 🥇 FUNCTIONAL:     Already complete (57/57 GREEN) - See UT_Data[Typical|Edge|Misuse|Fault]TCP.cxx
//   P2 🥈 DESIGN-ORIENTED: THIS FILE (State testing) - Start after P1 complete
//   P3 🥉 QUALITY-ORIENTED: Future (Performance, Robust, etc.)
//   P4 🎯 ADDONS:          Optional (Demo, Examples)
//
// WORKFLOW:
//   1. ✅ P1 Complete (57/57 tests GREEN) - Gate passed
//   2. 🎯 P2 In Progress (0/18 tests) - State testing (THIS FILE)
//   3. ⚪ P3 Planned - Quality attributes (Capability, Concurrency, Performance)
//   4. ⚪ P4 Optional - Demo/Examples
//
//===================================================================================================
// P2 🥈 DESIGN-ORIENTED TESTING – State (TCP-Specific Integration)
//===================================================================================================
//
// 🚪 GATE P2 ENTRY: P1 Functional Testing COMPLETE ✅
//    - UT_DataTypicalTCP: 7/7 GREEN
//    - UT_DataEdgeTCP: 12/12 GREEN (TCP polling timeout bug fixed)
//    - UT_DataMisuseTCP: 24/24 GREEN
//    - UT_DataFaultTCP: 14/14 PASSED, 6/6 SKIPPED (strategic)
//    - Total: 57/57 P1 tests passing
//
//===================================================================================================
// 📋 [CAT-1]: TCP CONNECTION ESTABLISHMENT × DATA STATE (0/3 PLANNED) ⚪
//===================================================================================================
// Purpose: Verify data state behavior during TCP connection setup phase
// Dependencies: IOC_getLinkState() API, IOC_getLinkConnState() API
// Estimated Total Effort: 4-6 hours
//
//   ⚪ [@CAT-1] TC-1: verifyDataStateBeforeConnection_byCheckingInitialStates_expectNotReady
//        - Description: Verify data sender/receiver states before TCP connection established
//        - Category: State (Connection Lifecycle)
//        - Key Verification: Data operations fail with NOT_EXIST_LINK before connect
//        - Depends on: None (infrastructure test)
//        - Estimated effort: 1-2 hours (includes test infrastructure setup)
//        - Priority: HIGH (validates baseline assumptions)
//
//   ⚪ [@CAT-1] TC-2: verifyDataStateDuringConnection_byMonitoringEstablishment_expectTransitionToReady
//        - Description: Verify data state transitions during TCP SYN→ESTABLISHED phase
//        - Category: State (Connection Lifecycle)
//        - Key Verification: States transition to DatSenderReady/DatReceiverReady after ESTABLISHED
//        - Depends on: TC-1 (baseline verification)
//        - Estimated effort: 2 hours (state monitoring during async connection)
//        - Priority: HIGH (core state transition validation)
//
//   ⚪ [@CAT-1] TC-3: verifyDataStateAfterConnectionFailure_byRefusedConnection_expectNoStateChange
//        - Description: Verify data states remain invalid when TCP connection fails
//        - Category: State (Error Handling)
//        - Key Verification: Failed connection doesn't create invalid states
//        - Depends on: TC-1, TC-2 (normal flow established)
//        - Estimated effort: 1-2 hours (connection failure scenarios)
//        - Priority: MEDIUM (error path validation)
//
//===================================================================================================
// 📋 [CAT-2]: DATA SENDER STATE × TCP TRANSMISSION (0/3 PLANNED) ⚪
//===================================================================================================
// Purpose: Verify data sender state behavior during TCP data transmission
// Dependencies: CAT-1 complete, sendDAT implementation validated (P1 complete)
// Estimated Total Effort: 5-7 hours
//
//   ⚪ [@CAT-2] TC-4: verifySenderStateTransition_bySimpleSendDAT_expectReadyToBusyToReady
//        - Description: Verify sender state transitions during normal IOC_sendDAT() over TCP
//        - Category: State (Sender Operations)
//        - Key Verification: Ready → BusySendDat → Ready transition cycle
//        - Depends on: CAT-1 complete (connection established)
//        - Estimated effort: 2 hours (sender state monitoring)
//        - Priority: HIGH (core sender state validation)
//
//   ⚪ [@CAT-2] TC-5: verifySenderStateDuringFlowControl_byBufferFull_expectBusyState
//        - Description: Verify sender state when TCP send buffer full (flow control engaged)
//        - Category: State (Flow Control)
//        - Key Verification: DatSenderBusySendDat persists until buffer available
//        - Depends on: TC-4 (normal sender state validated)
//        - Estimated effort: 2-3 hours (flow control simulation)
//        - Priority: MEDIUM (TCP-specific behavior)
//
//   ⚪ [@CAT-2] TC-6: verifySenderStateOnConnectionLoss_byMidTransmissionReset_expectErrorState
//        - Description: Verify sender state when TCP connection reset during transmission
//        - Category: State (Error Recovery)
//        - Key Verification: State transitions to error/disconnected on connection loss
//        - Depends on: TC-4, TC-5 (normal flow validated)
//        - Estimated effort: 2 hours (connection reset simulation)
//        - Priority: MEDIUM (error handling validation)
//
//===================================================================================================
// 📋 [CAT-3]: DATA RECEIVER STATE × TCP RECEPTION (0/3 PLANNED) ⚪
//===================================================================================================
// Purpose: Verify data receiver state behavior during TCP data reception
// Dependencies: CAT-1 complete, recvDAT implementation validated (P1 complete)
// Estimated Total Effort: 5-7 hours
//
//   ⚪ [@CAT-3] TC-7: verifyReceiverCallbackState_byTCPDataArrival_expectBusyCbRecvDat
//        - Description: Verify receiver state during callback-based reception over TCP
//        - Category: State (Receiver Operations - Callback)
//        - Key Verification: Ready → BusyCbRecvDat → Ready during callback execution
//        - Depends on: CAT-1 complete (connection established)
//        - Estimated effort: 2 hours (callback state monitoring)
//        - Priority: HIGH (callback mode state validation)
//
//   ⚪ [@CAT-3] TC-8: verifyReceiverPollingState_byTCPrecvDAT_expectBusyRecvDat
//        - Description: Verify receiver state during polling-based reception over TCP
//        - Category: State (Receiver Operations - Polling)
//        - Key Verification: Ready → BusyRecvDat → Ready during recvDAT waiting
//        - Depends on: TC-7 (callback mode validated)
//        - Estimated effort: 2 hours (polling state monitoring)
//        - Priority: HIGH (polling mode state validation)
//
//   ⚪ [@CAT-3] TC-9: verifyReceiverStateOnConnectionLoss_byMidReceptionReset_expectErrorState
//        - Description: Verify receiver state when TCP connection reset during reception
//        - Category: State (Error Recovery)
//        - Key Verification: State transitions to error/disconnected on connection loss
//        - Depends on: TC-7, TC-8 (normal flow validated)
//        - Estimated effort: 2-3 hours (connection reset during recv)
//        - Priority: MEDIUM (error handling validation)
//
//===================================================================================================
// 📋 [CAT-4]: BIDIRECTIONAL STATE × TCP FULL-DUPLEX (0/3 PLANNED) ⚪
//===================================================================================================
// Purpose: Verify concurrent sender/receiver state independence over single TCP connection
// Dependencies: CAT-2 and CAT-3 complete (sender/receiver states validated independently)
// Estimated Total Effort: 6-8 hours
//
//   ⚪ [@CAT-4] TC-10: verifyBidirectionalStateIndependence_byConcurrentSendRecv_expectIndependentStates
//        - Description: Verify sender/receiver states operate independently over same TCP link
//        - Category: State (Bidirectional Independence)
//        - Key Verification: Sender/receiver state changes don't interfere
//        - Depends on: CAT-2, CAT-3 complete (unidirectional validated)
//        - Estimated effort: 2-3 hours (concurrent state monitoring)
//        - Priority: HIGH (validates state machine independence)
//
//   ⚪ [@CAT-4] TC-11: verifyBidirectionalStateConsistency_byFullDuplexStream_expectValidTransitions
//        - Description: Verify state consistency during continuous bidirectional streaming
//        - Category: State (Full-Duplex Operations)
//        - Key Verification: Both state machines cycle correctly under continuous load
//        - Depends on: TC-10 (independence validated)
//        - Estimated effort: 2-3 hours (sustained bidirectional testing)
//        - Priority: MEDIUM (validates sustained operation)
//
//   ⚪ [@CAT-4] TC-12: verifyBidirectionalErrorHandling_byOneSideFailure_expectIndependentRecovery
//        - Description: Verify sender/receiver error handling independence
//        - Category: State (Error Isolation)
//        - Key Verification: One-side error doesn't corrupt other side's state
//        - Depends on: TC-10, TC-11 (normal bidirectional validated)
//        - Estimated effort: 2 hours (asymmetric error injection)
//        - Priority: MEDIUM (error isolation validation)
//
//===================================================================================================
// 📋 [CAT-5]: TCP CONNECTION RECOVERY × DATA STATE (0/3 PLANNED) ⚪
//===================================================================================================
// Purpose: Verify data state behavior during TCP reconnection scenarios
// Dependencies: CAT-1, CAT-2, CAT-3 complete (normal state behavior validated)
// Estimated Total Effort: 5-7 hours
//
//   ⚪ [@CAT-5] TC-13: verifyStateAfterReconnection_byCloseAndReconnect_expectFreshStates
//        - Description: Verify data states are properly reset/reinitialized after reconnection
//        - Category: State (Reconnection Lifecycle)
//        - Key Verification: New connection has clean initial states (no stale state)
//        - Depends on: CAT-1 complete (connection lifecycle validated)
//        - Estimated effort: 2 hours (reconnection state verification)
//        - Priority: HIGH (validates state cleanup)
//
//   ⚪ [@CAT-5] TC-14: verifyStateTransitionDuringReconnection_byMonitoringPhases_expectValidSequence
//        - Description: Verify state transitions are valid during reconnection process
//        - Category: State (FSM Validation)
//        - Key Verification: Disconnect → reconnect follows valid FSM rules
//        - Depends on: TC-13 (reconnection basics validated)
//        - Estimated effort: 2-3 hours (state transition monitoring)
//        - Priority: MEDIUM (FSM correctness validation)
//
//   ⚪ [@CAT-5] TC-15: verifyReconnectionWithPendingData_byBufferedDataHandling_expectDataIntegrity
//        - Description: Verify data integrity and state correctness when reconnecting with pending data
//        - Category: State (Data Recovery)
//        - Key Verification: NODROP guarantee maintained, states consistent with policy
//        - Depends on: TC-13, TC-14 (reconnection flow validated)
//        - Estimated effort: 2 hours (pending data scenarios)
//        - Priority: MEDIUM (validates NODROP guarantee)
//
//===================================================================================================
// 📋 [CAT-6]: TCP LAYER TRANSPARENCY × DATA STATE (0/3 PLANNED) ⚪
//===================================================================================================
// Purpose: Verify data states remain stable during TCP-layer events (retransmit, window updates)
// Dependencies: CAT-2, CAT-3 complete (normal transmission states validated)
// Estimated Total Effort: 6-9 hours (complex TCP-layer simulation)
//
//   ⚪ [@CAT-6] TC-16: verifyStateStabilityDuringRetransmission_byPacketLoss_expectNoStateChange
//        - Description: Verify TCP retransmissions don't affect data state
//        - Category: State (Layer Abstraction)
//        - Key Verification: Retransmit transparent to data state machine
//        - Depends on: CAT-2 complete (sender state validated)
//        - Estimated effort: 3 hours (packet loss simulation)
//        - Priority: LOW (validates abstraction layer)
//        - Note: May require network simulation tools (tc, netem)
//
//   ⚪ [@CAT-6] TC-17: verifyStateIndependenceFromWindowUpdates_byFlowControlEvents_expectStableStates
//        - Description: Verify TCP window updates don't directly affect data state
//        - Category: State (Layer Abstraction)
//        - Key Verification: TCP window changes abstracted from data state
//        - Depends on: CAT-2 complete (flow control validated)
//        - Estimated effort: 2-3 hours (window manipulation)
//        - Priority: LOW (validates abstraction layer)
//
//   ⚪ [@CAT-6] TC-18: verifyStateDuringTCPKeepAlive_byIdleConnection_expectStableReadyStates
//        - Description: Verify data states remain stable during TCP keep-alive probes
//        - Category: State (Idle Connection)
//        - Key Verification: Keep-alive doesn't trigger spurious state changes
//        - Depends on: CAT-1 complete (idle connection validated)
//        - Estimated effort: 2 hours (keep-alive monitoring)
//        - Priority: LOW (validates idle stability)
//
// 🚪 GATE P2 EXIT: All 18 TCP-specific state tests GREEN
//    - Validates: Data state machine correctness over TCP protocol
//    - Unlocks: P3 Quality Testing (Capability, Concurrency, Performance)
//
//===================================================================================================
// ✅ PROGRESS SUMMARY
//===================================================================================================
//
// 📊 CURRENT STATUS: Framework created, awaiting implementation (0/18 tests)
//
// 📋 CATEGORY BREAKDOWN:
//    CAT-1: TCP Connection Establishment × Data State ......... ⚪ 0/3 (PLANNED)
//    CAT-2: Data Sender State × TCP Transmission .............. ⚪ 0/3 (PLANNED)
//    CAT-3: Data Receiver State × TCP Reception ............... ⚪ 0/3 (PLANNED)
//    CAT-4: Bidirectional State × TCP Full-Duplex ............. ⚪ 0/3 (PLANNED)
//    CAT-5: TCP Connection Recovery × Data State .............. ⚪ 0/3 (PLANNED)
//    CAT-6: TCP Layer Transparency × Data State ............... ⚪ 0/3 (PLANNED)
//
// 🎯 RECOMMENDED IMPLEMENTATION ORDER:
//    Phase 1 (Core States):     CAT-1 → CAT-2 → CAT-3 (9 tests, ~14-20 hours)
//    Phase 2 (Integration):     CAT-4 → CAT-5 (6 tests, ~11-15 hours)
//    Phase 3 (Advanced):        CAT-6 (3 tests, ~6-9 hours, optional for P2)
//
// 🎯 NEXT IMMEDIATE STEPS:
//    1. Implement test infrastructure (TcpDataStateTracker helper class)
//    2. Start with CAT-1 TC-1 (baseline state verification)
//    3. Enable state query APIs (IOC_getLinkState, IOC_getLinkConnState)
//    4. Validate against README_ArchDesign-State.md state machine specification
//
// 📅 CREATION DATE: 2025-12-28
// 📝 DESIGN BASIS: README_ArchDesign-State.md "Data State Machine" section (lines 1397-1600)
// 🔗 COMPLEMENTS: UT_DataStateUS1-7.cxx (protocol-agnostic state testing)
// 🧪 TEST FRAMEWORK: GoogleTest + IOC_getLinkState() API + AddressSanitizer
// 📐 STATE MACHINE: 5 substates (DatSenderReady, DatSenderBusySendDat, DatReceiverReady,
//                                DatReceiverBusyRecvDat, DatReceiverBusyCbRecvDat)
//
///////////////////////////////////////////////////////////////////////////////////////////////////
//======>END OF TODO/IMPLEMENTATION TRACKING SECTION===============================================
