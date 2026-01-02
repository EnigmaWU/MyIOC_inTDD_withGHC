///////////////////////////////////////////////////////////////////////////////////////////////////
// UT_DataConcurrencyTCP.cxx - Data API Concurrency Testing (TCP Protocol)
//
// PURPOSE:
//   Verify thread-safety and synchronization of IOC Data APIs (sendDAT/recvDAT/flushDAT)
//   using TCP protocol for network communication.
//   Focuses on TCP-specific race conditions, socket thread-safety, and network-related concurrency.
//
// CATDD METHODOLOGY:
//   This file follows Comment-alive Test-Driven Development (CaTDD):
//   - Phase 2: DESIGN - Comprehensive test design in comments
//   - Phase 3: IMPLEMENTATION - TDD Red→Green cycle
//
// PRIORITY CLASSIFICATION:
//   P2: Design-Oriented → Concurrency
//   PROMOTED TO P1 LEVEL due to high risk score:
//     - Impact: 3 (Network deadlock/corruption in distributed systems)
//     - Likelihood: 3 (TCP common in production multi-process apps)
//     - Uncertainty: 2 (Complex socket I/O threading)
//     - Score: 18 → Critical priority
//
// PROTOCOL COVERAGE:
//   - This file: TCP (network communication)
//   - See UT_DataConcurrency.cxx for FIFO local process
//
// TCP-SPECIFIC CONCERNS:
//   - Socket send/recv thread-safety (SIGPIPE, ECONNRESET)
//   - Partial write/read handling under concurrency
//   - Connection state races (accept/close/send concurrent)
//   - Network buffer vs IOC buffer synchronization
//   - Async I/O completion thread coordination
//
// RELATIONSHIPS:
//   - Depends on: Source/IOC_Data.c, Source/_IOC_SrvProtoTCP.c
//   - Related tests: UT_DataConcurrency.cxx (FIFO variant)
//   - Production code: Include/IOC/IOC_DatAPI.h
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <array>
#include <atomic>
#include <chrono>
#include <cstring>
#include <mutex>
#include <thread>
#include <vector>

#include "_UT_IOC_Common.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF OVERVIEW OF THIS UNIT TESTING FILE===============================================
/**
 * @brief
 *   [WHAT] This file verifies IOC Data API concurrency and thread safety for TCP protocol.
 *   [WHERE] in the IOC Data subsystem for network-based data streaming.
 *   [WHY] to ensure no deadlocks or race conditions occur during multi-threaded TCP operations.
 *
 * SCOPE:
 *   - In scope:
 *     • TCP-specific concurrency: socket thread-safety, partial I/O
 *     • Network connection state races (accept/send/close)
 *     • TCP buffer management under multi-threading
 *     • SIGPIPE handling in concurrent write scenarios
 *     • Connection failure propagation across threads
 *     • All scenarios from UT_DataConcurrency.cxx adapted for TCP
 *   - Out of scope:
 *     • FIFO protocol (see UT_DataConcurrency.cxx)
 *     • Network simulation/packet loss (see UT_DataFaultTCP.cxx)
 *
 * TCP-ONLY CONCURRENCY CHALLENGES:
 *   1. Socket FD Thread-Safety: Multiple threads writing to same TCP socket
 *   2. Partial Write Resume: Concurrent threads must not corrupt partial write state
 *   3. Accept/Send Race: Server accepting new connections while sending to existing
 *   4. SIGPIPE Safety: Signal handling during concurrent socket operations
 *   5. Connection State: Disconnect propagation to all concurrent I/O threads
 */
//======>END OF OVERVIEW OF THIS UNIT TESTING FILE=================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF FREELY DRAFTED IDEAS=============================================================
/**
 * BRAINSTORMING: Raw TCP concurrency ideas before structuring
 * (CaTDD Step 2: Freely draft without format constraints)
 *
 * What if scenarios for TCP concurrency:
 *  • What if 5 threads write to same socket at once? → send() serialization critical
 *  • What if socket closes mid-write from another thread? → EPIPE/SIGPIPE handling
 *  • What if partial write occurs, can other threads interfere? → Write buffer integrity
 *  • What if accept() races with broadcast send? → New connection state sync
 *  • What if SIGPIPE kills process during concurrent send? → Signal masking required
 *  • What if TCP buffer full, blocking write vs non-blocking? → Thread starvation risk
 *  • What if client disconnects while server callback active? → Callback safety
 *  • What if recv() timeout varies across threads? → Timeout independence
 *  • What if large message split across multiple send() calls? → Atomicity guarantee
 *  • What if connection breaks during multi-thread operation? → Error propagation
 *
 * TCP-specific edge cases:
 *  • SIGPIPE signal handling (SIG_IGN vs EPIPE return)
 *  • Partial send() return values under contention
 *  • Socket buffer exhaustion with multiple writers
 *  • Connection close during in-flight send/recv
 *  • IPv4 vs IPv6 socket differences
 *  • TCP_NODELAY vs Nagle algorithm interaction
 *
 * Gotchas to verify:
 *  • Socket file descriptor thread-safety (OS level)
 *  • send()/recv() system call atomicity
 *  • IOC buffer vs kernel socket buffer synchronization
 *  • Connection state machine thread safety
 *  • Network latency impact on concurrent operations
 */
//======>END OF FREELY DRAFTED IDEAS===============================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF UNIT TESTING DESIGN==============================================================

/**************************************************************************************************
 * 📋 TCP-SPECIFIC CONCURRENCY TESTING DESIGN
 *
 * COVERAGE MATRIX: Extends base concurrency matrix with TCP-specific scenarios
 *
 * ┌───────────┬──────────┬────────────┬──────────────┬─────────────────────────────┐
 * │ Protocol  │ Pattern  │ Concurr.   │ API Op       │ TCP-Specific User Story     │
 * ├───────────┼──────────┼────────────┼──────────────┼─────────────────────────────┤
 * │ TCP       │ P1       │ Multi-T    │ sendDAT      │ US-T1: Socket write safety  │
 * │ TCP       │ P2       │ Multi-T    │ sendDAT      │ US-T2: Broadcast w/ backlog │
 * │ TCP       │ P1       │ Accept+I/O │ accept+send  │ US-T3: Accept race          │
 * │ TCP       │ Both     │ Disconnect │ send+close   │ US-T4: Disconnect propag.   │
 * │ TCP       │ Both     │ Partial-I/O│ sendDAT      │ US-T5: Partial write safe   │
 * │ TCP       │ Both     │ SIGPIPE    │ sendDAT      │ US-T6: Signal safety        │
 * └───────────┴──────────┴────────────┴──────────────┴─────────────────────────────┘
 *
 * ADDITIONAL USER STORIES (TCP-SPECIFIC):
 *
 *  US-T1: AS a multi-threaded TCP sender,
 *         I WANT multiple threads to call IOC_sendDAT on same TCP socket safely,
 *         SO THAT concurrent sends don't cause EPIPE or data corruption.
 *
 *  US-T2: AS a TCP broadcast server,
 *         I WANT to handle TCP send buffer full on slow clients without blocking fast clients,
 *         SO THAT network backpressure is per-client isolated.
 *
 *  US-T3: AS a TCP server,
 *         I WANT to accept new connections while sending to existing clients concurrently,
 *         SO THAT new client acceptance doesn't stall active data transfers.
 *
 *  US-T4: AS a TCP application,
 *         I WANT peer disconnect to propagate to all concurrent I/O threads gracefully,
 *         SO THAT threads get proper LINK_BROKEN without crashes.
 *
 *  US-T5: AS a sender dealing with TCP partial writes,
 *         I WANT concurrent threads to handle partial send() returns safely,
 *         SO THAT resumption logic doesn't corrupt data from other threads.
 *
 *  US-T6: AS a TCP sender with broken pipe scenarios,
 *         I WANT SIGPIPE signals handled safely during concurrent writes,
 *         SO THAT write failures don't crash my application.
 *************************************************************************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF FAST-FAIL TCP CONCURRENCY SIX====================================================
/**
 * TCP-SPECIFIC FAST-FAIL SIX (run before full suite)
 *
 * FAST-FAIL TCP-SIX:
 *
 * 1. **Single-Thread TCP Baseline**: Verify basic TCP send/recv works
 *    - Test: Simple client-server exchange over TCP
 *    - Purpose: Prove TCP protocol layer functional
 *    - Fail indicator: Connection failure, data loss
 *
 * 2. **Two-Thread TCP Send Race**: Detect socket write concurrency
 *    - Test: 2 threads sending to same TCP socket
 *    - Purpose: Expose send() serialization issues
 *    - Fail indicator: EPIPE, data corruption, deadlock
 *
 * 3. **TCP Disconnect During Send**: Verify error propagation
 *    - Test: Disconnect while thread in IOC_sendDAT
 *    - Purpose: Catch improper ECONNRESET handling
 *    - Fail indicator: Crash, hang, no error returned
 *
 * 4. **SIGPIPE Safety Smoke**: Catch signal handling bug early
 *    - Test: Write to broken TCP connection, expect no crash
 *    - Purpose: Verify SIGPIPE masked or handled
 *    - Fail indicator: Process killed by SIGPIPE
 *
 * 5. **TCP Accept+Send Concurrent**: Basic race between accept/I/O
 *    - Test: 1 thread accepting, 1 thread sending to existing client
 *    - Purpose: Detect connection state races
 *    - Fail indicator: Accept blocks send, or vice versa
 *
 * 6. **TCP Partial Write Smoke**: Handle short send() return
 *    - Test: Large payload likely to trigger partial send()
 *    - Purpose: Verify resume logic not broken
 *    - Fail indicator: Truncated data, corruption
 */
//======>END OF FAST-FAIL TCP CONCURRENCY SIX======================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF USER STORY=======================================================================
// See matrix above for TCP-specific User Stories US-T1 through US-T6
// All US-1 through US-9 from UT_DataConcurrency.cxx also apply to TCP with network nuances
//======>END OF USER STORY=========================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//=======>BEGIN OF ACCEPTANCE CRITERIA=============================================================
/**
 * TCP-SPECIFIC ACCEPTANCE CRITERIA
 *
 * [@US-T1] TCP socket write thread-safety
 *  AC-T1: GIVEN 10 threads calling IOC_sendDAT on same TCP LinkID concurrently,
 *          WHEN each thread sends 100 chunks over network,
 *          THEN TCP socket write operations properly serialized,
 *           AND no EPIPE or SIGPIPE signals,
 *           AND all data delivered intact to receiver.
 *
 *  AC-T2: GIVEN concurrent sends during TCP send buffer full,
 *          WHEN kernel returns EAGAIN/EWOULDBLOCK,
 *          THEN IOC properly retries per-thread,
 *           AND no thread corrupts another's retry state,
 *           AND all sends eventually succeed.
 *
 * [@US-T2] TCP broadcast with backpressure
 *  AC-T3: GIVEN service broadcasting to 5 clients, 1 client has slow TCP receive,
 *          WHEN slow client's TCP buffer fills,
 *          THEN slow client's send blocks/backpressure only affects that client,
 *           AND 4 fast clients continue receiving without delay,
 *           AND no system-wide stall.
 *
 * [@US-T3] Accept/Send race conditions
 *  AC-T4: GIVEN server sending to 3 existing TCP clients from worker threads,
 *          WHEN main thread accepts 2 new client connections concurrently,
 *          THEN accept operations don't interfere with ongoing sends,
 *           AND new client connection state properly initialized,
 *           AND existing client sends complete successfully.
 *
 * [@US-T4] Disconnect propagation
 *  AC-T5: GIVEN 4 threads concurrently sending on same TCP LinkID,
 *          WHEN peer abruptly disconnects (ECONNRESET),
 *          THEN all 4 threads receive IOC_RESULT_LINK_BROKEN,
 *           AND no crashes or hangs,
 *           AND connection state cleaned up safely.
 *
 * [@US-T5] Partial write safety
 *  AC-T6: GIVEN concurrent sends when TCP send() returns partial write (< requested),
 *          WHEN multiple threads need to resume partial writes,
 *          THEN each thread's resumption state isolated,
 *           AND no data corruption from interleaved partial writes,
 *           AND full payloads eventually transmitted.
 *
 * [@US-T6] SIGPIPE signal safety
 *  AC-T7: GIVEN concurrent TCP sends with SIGPIPE possible,
 *          WHEN peer closes connection causing broken pipe,
 *          THEN SIGPIPE properly handled (ignored or caught),
 *           AND sending threads get IOC_RESULT_LINK_BROKEN error,
 *           AND application doesn't crash from signal.
 *
 * NOTE: All AC-1 through AC-X from UT_DataConcurrency.cxx also apply to TCP,
 *       tested here with network transport instead of FIFO.
 */
//=======>END OF ACCEPTANCE CRITERIA================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF TEST CASES=======================================================================
/**
 * TCP-SPECIFIC TEST CASES
 *
 * STATUS TRACKING:
 *  ⚪ TODO/PLANNED:      Designed but not implemented yet
 *  🔴 RED/IMPLEMENTED:   Test written and failing (need prod code)
 *  🟢 GREEN/PASSED:      Test written and passing
 *  ⚠️  ISSUES:           Known problem needing attention
 *
 *===================================================================================================
 * [@AC-T1,US-T1] TCP socket write thread-safety
 *===================================================================================================
 *  ⚪ TC-T1: verifyTCPSocketWriteSafety_by10ThreadsSameSocket_expectNoSIGPIPE
 *      @[Purpose]: Critical - verify concurrent writes to same TCP socket don't cause signals
 *      @[Brief]: 10 threads × IOC_sendDAT on same TCP LinkID, verify serialization
 *      @[Protocol]: TCP
 *
 *  ⚪ TC-T2: verifyTCPPartialWriteRetry_byConcurrentEAGAIN_expectIsolatedRetry
 *      @[Purpose]: Test partial write handling under concurrency
 *      @[Brief]: Simulate TCP buffer full, verify each thread's retry state isolated
 *      @[Protocol]: TCP
 *
 *===================================================================================================
 * [@AC-T3,US-T2] TCP broadcast backpressure isolation
 *===================================================================================================
 *  ⚪ TC-T3: verifyTCPBroadcastBackpressure_bySlowClient_expectFastNotBlocked
 *      @[Purpose]: Ensure slow TCP client doesn't block fast clients
 *      @[Brief]: 5 clients, 1 slow (small recv buffer), verify 4 fast continue
 *      @[Protocol]: TCP
 *
 *===================================================================================================
 * [@AC-T4,US-T3] TCP accept/send race
 *===================================================================================================
 *  ⚪ TC-T4: verifyTCPAcceptSendRace_byConcurrentAcceptAndSend_expectNoInterference
 *      @[Purpose]: Validate accept doesn't interfere with concurrent sends
 *      @[Brief]: 3 senders active while 2 new clients connect
 *      @[Protocol]: TCP
 *
 *===================================================================================================
 * [@AC-T5,US-T4] TCP disconnect propagation
 *===================================================================================================
 *  ⚪ TC-T5: verifyTCPDisconnectPropagation_by4ConcurrentSenders_expectAllGetLinkBroken
 *      @[Purpose]: Test graceful error propagation on peer disconnect
 *      @[Brief]: 4 threads sending when peer closes, verify all get LINK_BROKEN
 *      @[Protocol]: TCP
 *
 *===================================================================================================
 * [@AC-T6,US-T5] TCP partial write isolation
 *===================================================================================================
 *  ⚪ TC-T6: verifyTCPPartialWriteIsolation_byConcurrentPartialWrites_expectNoCorruption
 *      @[Purpose]: Ensure partial write resume doesn't corrupt concurrent threads
 *      @[Brief]: Force partial writes, verify each thread's state isolated
 *      @[Protocol]: TCP
 *
 *===================================================================================================
 * [@AC-T7,US-T6] SIGPIPE safety
 *===================================================================================================
 *  ⚪ TC-T7: verifyTCPSIGPIPESafety_byBrokenPipeDuringConcurrentSend_expectNoCrash
 *      @[Purpose]: Critical - verify SIGPIPE doesn't crash application
 *      @[Brief]: Peer closes during concurrent sends, verify signal handled
 *      @[Protocol]: TCP
 *
 *===================================================================================================
 * TCP ADAPTATIONS OF BASE CONCURRENCY TESTS
 *===================================================================================================
 *  ⚪ TC-T8: verifyP1MultiClientSendTCP_by10Clients500Chunks_expectAll5000Received
 *      @[Purpose]: Pattern-1 multi-client send over TCP network
 *      @[Brief]: Same as FIFO TC-1 but with TCP transport
 *      @[Protocol]: TCP
 *
 *  ⚪ TC-T9: verifyP2ServiceBroadcastTCP_by8Clients1000Chunks_expectAllReceive
 *      @[Purpose]: Pattern-2 broadcast over TCP with network delays
 *      @[Brief]: Same as FIFO TC-4 but with TCP transport
 *      @[Protocol]: TCP
 *
 *  ⚪ TC-T10: verifyMultiLinkIsolationTCP_by5Links4ThreadsEach_expectFullIsolation
 *      @[Purpose]: Multi-link isolation with TCP sockets
 *      @[Brief]: Same as FIFO TC-11 but with TCP connections
 *      @[Protocol]: TCP
 *
 *  ⚪ TC-T11: verifyCallbackReentrantTCP_bySendFromCallbackSameLink_expectNoDeadlock
 *      @[Purpose]: CRITICAL deadlock test over TCP
 *      @[Brief]: Same as FIFO TC-15 but with TCP socket locks
 *      @[Protocol]: TCP
 */
//======>END OF TEST CASES=========================================================================
//======>END OF UNIT TESTING DESIGN================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF TODO/IMPLEMENTATION TRACKING SECTION=============================================
// 🔴 TCP CONCURRENCY IMPLEMENTATION STATUS - Organized by Priority
//
// STATUS LEGEND:
//   ⚪ TODO/PLANNED:      Designed but not implemented
//   🔴 RED/IMPLEMENTED:   Test written and failing
//   🟢 GREEN/PASSED:      Test passing
//
//=================================================================================================
// 🥇 CRITICAL PRIORITY – TCP-Specific Must-Have
//=================================================================================================
//   ⚪ [@AC-1,US-T6] TC-T7: verifyTCPSIGPIPESafety (MOST CRITICAL - can crash process)
//   ⚪ [@AC-1,US-T4] TC-T5: verifyTCPDisconnectPropagation (error handling)
//   ⚪ [@AC-1,US-T1] TC-T1: verifyTCPSocketWriteSafety (basic thread-safety)
//
//=================================================================================================
// 🥈 HIGH PRIORITY – TCP Protocol Correctness
//=================================================================================================
//   ⚪ [@AC-T6,US-T5] TC-T6: verifyTCPPartialWriteIsolation
//   ⚪ [@AC-T2,US-T1] TC-T2: verifyTCPPartialWriteRetry
//   ⚪ [@AC-T4,US-T3] TC-T4: verifyTCPAcceptSendRace
//
//=================================================================================================
// 🥉 MEDIUM PRIORITY – TCP Quality & Edge Cases
//=================================================================================================
//   ⚪ [@AC-T3,US-T2] TC-T3: verifyTCPBroadcastBackpressure
//   ⚪ TC-T8: TCP adaptation of P1 multi-client
//   ⚪ TC-T9: TCP adaptation of P2 broadcast
//   ⚪ TC-T10: TCP adaptation of multi-link isolation
//   ⚪ TC-T11: TCP adaptation of callback deadlock
//
//=================================================================================================
// IMPLEMENTATION ROADMAP:
//   Week 1: TC-T7 (SIGPIPE safety) + TC-T1 (basic socket write) + TC-T5 (disconnect)
//   Week 2: TC-T6, TC-T2 (partial write handling)
//   Week 3: TC-T4 (accept race) + TC-T3 (backpressure)
//   Week 4: TCP adaptations (TC-T8 through TC-T11)
//=================================================================================================
//======>END OF TODO/IMPLEMENTATION TRACKING SECTION===============================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF TEST HELPER UTILITIES============================================================
/**
 * TCP CONCURRENCY TEST INFRASTRUCTURE:
 *
 * TCP-Specific Utilities:
 *  • TCPConnectionInfo: Per-connection state tracking
 *    - ServerPort, ClientFd, ServerFd for identification
 *    - ConnectionState: CONNECTED, DISCONNECTED, ERROR
 *    - BytesSent/Received counters
 *
 *  • TCPConcurrencyCounters: Extends ConcurrencyCounters with TCP metrics
 *    - SIGPIPECount: Tracks broken pipe signals
 *    - ECONNRESETCount: Connection reset errors
 *    - PartialWriteCount: Partial send() occurrences
 *    - RetryCount: Number of send/recv retries
 *
 *  • SimulateTCPBackpressure(): Helper to fill TCP send buffer
 *    - Sets small SO_RCVBUF on client side
 *    - Triggers EAGAIN/EWOULDBLOCK for testing
 *
 *  • SIGPIPEHandler(): Signal handler for SIGPIPE testing
 *    - Counts SIGPIPE occurrences without crashing
 *    - Atomic increment for thread-safety
 *
 * Shared Utilities (from UT_DataConcurrency.cxx):
 *  • DataChunk, ComputeChecksum: Payload integrity
 *  • ConcurrencyCounters: Basic atomic counters
 *
 * Future Utilities (TODO):
 *  • TCPDeadlockDetector: Timeout with socket state dump
 *  • NetworkLatencySimulator: Add artificial delays
 *  • PartialWriteForcer: Controllable short send() returns
 */
//======>END OF TEST HELPER UTILITIES==============================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF TODO/IMPLEMENTATION TRACKING SECTION=============================================
// 🔴 IMPLEMENTATION STATUS TRACKING - TCP Protocol
//
//=================================================================================================
// 🥇 HIGH PRIORITY – TCP-Critical Concurrency
//=================================================================================================
//   ⚪ [@AC-T1,US-T1] TC-T1: verifyTCPSocketWriteSafety_by10ThreadsSameSocket_expectNoSIGPIPE – CRITICAL
//   ⚪ [@AC-T7,US-T6] TC-T7: verifyTCPSIGPIPESafety_byBrokenPipeDuringConcurrentSend_expectNoCrash – CRITICAL
//   ⚪ [@AC-T5,US-T4] TC-T5: verifyTCPDisconnectPropagation_by4ConcurrentSenders_expectAllGetLinkBroken
//   ⚪ [TCP-Adapt] TC-T11: verifyCallbackReentrantTCP_bySendFromCallbackSameLink_expectNoDeadlock
//
//=================================================================================================
// 🥈 MEDIUM PRIORITY – TCP Network Behavior
//=================================================================================================
//   ⚪ [@AC-T3,US-T2] TC-T3: verifyTCPBroadcastBackpressure_bySlowClient_expectFastNotBlocked
//   ⚪ [@AC-T4,US-T3] TC-T4: verifyTCPAcceptSendRace_byConcurrentAcceptAndSend_expectNoInterference
//   ⚪ [TCP-Adapt] TC-T8: verifyP1MultiClientSendTCP_by10Clients500Chunks_expectAll5000Received
//   ⚪ [TCP-Adapt] TC-T9: verifyP2ServiceBroadcastTCP_by8Clients1000Chunks_expectAllReceive
//
//=================================================================================================
// 🥉 LOW PRIORITY – TCP Edge Cases
//=================================================================================================
//   ⚪ [@AC-T2,US-T1] TC-T2: verifyTCPPartialWriteRetry_byConcurrentEAGAIN_expectIsolatedRetry
//   ⚪ [@AC-T6,US-T5] TC-T6: verifyTCPPartialWriteIsolation_byConcurrentPartialWrites_expectNoCorruption
//
///////////////////////////////////////////////////////////////////////////////////////////////////
//======>END OF TODO/IMPLEMENTATION TRACKING SECTION===============================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======BEGIN OF UNIT TESTING IMPLEMENTATION=======================================================

// TCP-specific test context
namespace {

// TCP connection helper
struct TCPConnectionInfo {
    std::string ServerIP;
    uint16_t ServerPort;
    IOC_SrvID_T SrvID;
    std::vector<IOC_LinkID_T> ClientLinkIDs;
    std::atomic<bool> ServerActive{true};
};

// Counters for TCP tests
struct TCPConcurrencyCounters {
    std::atomic<uint32_t> SuccessfulSends{0};
    std::atomic<uint32_t> FailedSends{0};
    std::atomic<uint32_t> SIGPIPEDetected{0};
    std::atomic<uint32_t> LinkBrokenErrors{0};
    std::atomic<uint32_t> PartialWrites{0};
    std::atomic<uint32_t> BytesSent{0};
    std::atomic<uint32_t> BytesReceived{0};
};

}  // namespace

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF: [@AC-T1,US-T1] TC-T1: TCP Socket Write Safety==================================

/**
 * @[Name]: verifyTCPSocketWriteSafety_by10ThreadsSameSocket_expectNoSIGPIPE
 * @[Purpose]: CRITICAL - verify concurrent writes to same TCP socket don't cause SIGPIPE
 * @[Steps]:
 *   1) 🔧 SETUP: Create TCP server with DatReceiver service
 *   2) 🔧 SETUP: Create 1 DatSender client connected over TCP
 *   3) 🎯 BEHAVIOR: Launch 10 threads all calling IOC_sendDAT on same TCP LinkID
 *   4) 🎯 BEHAVIOR: Each thread sends 100 chunks (1KB each)
 *   5) ✅ VERIFY: No SIGPIPE signals generated
 *   6) ✅ VERIFY: All 1000 chunks received by server
 *   7) ✅ VERIFY: TCP socket write operations properly serialized
 *   8) 🧹 CLEANUP: Disconnect, offline service
 * @[Expect]: Zero SIGPIPE, all data received intact
 * @[Protocol]: TCP
 * @[Risk]: CRITICAL - SIGPIPE can crash application if not handled
 */
TEST(UT_DataConcurrencyTCP, verifyTCPSocketWriteSafety_by10ThreadsSameSocket_expectNoSIGPIPE) {
    //===SETUP===
    printf("🔧 SETUP: TCP socket write safety - 10 threads → same socket\n");

    // TODO: Implement critical TCP socket thread-safety test
    // 1. Setup TCP server (localhost:port) with DatReceiver
    // 2. Create single DatSender client connecting over TCP
    // 3. Launch 10 threads sharing same TCP LinkID
    // 4. Install SIGPIPE handler to detect if signal occurs
    // 5. Each thread sends 100 chunks concurrently
    // 6. Verify zero SIGPIPE signals
    // 7. Verify all 1000 chunks received

    GTEST_SKIP() << "⚪ TODO: Implement CRITICAL TCP socket write safety test";
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF: [@AC-T7,US-T6] TC-T7: SIGPIPE Safety===========================================

/**
 * @[Name]: verifyTCPSIGPIPESafety_byBrokenPipeDuringConcurrentSend_expectNoCrash
 * @[Purpose]: CRITICAL - verify SIGPIPE during concurrent sends doesn't crash app
 * @[Steps]:
 *   1) 🔧 SETUP: Create TCP connection, start 5 concurrent senders
 *   2) 🎯 BEHAVIOR: Abruptly close peer connection during active sends
 *   3) 🎯 BEHAVIOR: Monitor for SIGPIPE signal
 *   4) ✅ VERIFY: SIGPIPE handled (ignored or caught), no crash
 *   5) ✅ VERIFY: All sender threads get IOC_RESULT_LINK_BROKEN error
 *   6) 🧹 CLEANUP: Join threads, cleanup
 * @[Expect]: No crash, graceful error to all threads
 * @[Protocol]: TCP
 * @[Risk]: CRITICAL - Default SIGPIPE terminates process
 */
TEST(UT_DataConcurrencyTCP, verifyTCPSIGPIPESafety_byBrokenPipeDuringConcurrentSend_expectNoCrash) {
    //===SETUP===
    printf("🔧 SETUP: SIGPIPE safety test - broken pipe during concurrent sends\n");

    // TODO: Implement critical SIGPIPE safety test
    // 1. Setup TCP connection
    // 2. Launch 5 sender threads
    // 3. After senders active, abruptly close peer (shutdown socket)
    // 4. Install signal handler to detect SIGPIPE
    // 5. Verify application doesn't terminate
    // 6. Verify all threads get proper error codes

    GTEST_SKIP() << "⚪ TODO: Implement CRITICAL SIGPIPE safety test";
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF: [@AC-T5,US-T4] TC-T5: Disconnect Propagation===================================

/**
 * @[Name]: verifyTCPDisconnectPropagation_by4ConcurrentSenders_expectAllGetLinkBroken
 * @[Purpose]: Test graceful disconnect error propagation to all concurrent threads
 * @[Steps]:
 *   1) 🔧 SETUP: Create TCP connection with 4 sender threads active
 *   2) 🎯 BEHAVIOR: Peer closes connection (ECONNRESET scenario)
 *   3) ✅ VERIFY: All 4 threads receive IOC_RESULT_LINK_BROKEN
 *   4) ✅ VERIFY: No crashes, no hangs
 *   5) ✅ VERIFY: Connection state properly cleaned up
 *   6) 🧹 CLEANUP: Join threads
 * @[Expect]: All threads get LINK_BROKEN, system stable
 * @[Protocol]: TCP
 */
TEST(UT_DataConcurrencyTCP, verifyTCPDisconnectPropagation_by4ConcurrentSenders_expectAllGetLinkBroken) {
    //===SETUP===
    printf("🔧 SETUP: TCP disconnect propagation - 4 concurrent senders\n");

    // TODO: Implement disconnect propagation test
    // 1. Setup TCP connection
    // 2. Launch 4 threads continuously sending
    // 3. Simulate peer disconnect (close socket from receiver side)
    // 4. Verify all sender threads detect disconnection
    // 5. Verify proper error code (IOC_RESULT_LINK_BROKEN)

    GTEST_SKIP() << "⚪ TODO: Implement TCP disconnect propagation test";
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF: TCP Adaptations of Base Tests=================================================

/**
 * @[Name]: verifyP1MultiClientSendTCP_by10Clients500Chunks_expectAll5000Received
 * @[Purpose]: Pattern-1 multi-client concurrent send over TCP network
 * @[Brief]: TCP adaptation of FIFO TC-1, tests network transport nuances
 * @[Protocol]: TCP
 */
TEST(UT_DataConcurrencyTCP, verifyP1MultiClientSendTCP_by10Clients500Chunks_expectAll5000Received) {
    //===SETUP===
    printf("🔧 SETUP: Pattern-1 TCP - 10 concurrent clients over network\n");

    // TODO: Implement TCP variant of multi-client send
    // 1. TCP server at localhost:port
    // 2. 10 TCP clients connecting
    // 3. Each client sends 500 chunks over TCP
    // 4. Server receives all 5000 via callback
    // 5. Verify network transport doesn't introduce issues

    GTEST_SKIP() << "⚪ TODO: Implement Pattern-1 TCP multi-client send test";
}

TEST(UT_DataConcurrencyTCP, verifyP2ServiceBroadcastTCP_by8Clients1000Chunks_expectAllReceive) {
    GTEST_SKIP() << "⚪ TODO: Implement Pattern-2 TCP broadcast test";
}

TEST(UT_DataConcurrencyTCP, verifyMultiLinkIsolationTCP_by5Links4ThreadsEach_expectFullIsolation) {
    GTEST_SKIP() << "⚪ TODO: Implement TCP multi-link isolation test";
}

TEST(UT_DataConcurrencyTCP, verifyCallbackReentrantTCP_bySendFromCallbackSameLink_expectNoDeadlock) {
    GTEST_SKIP() << "⚪ TODO: Implement TCP callback re-entrancy deadlock test";
}

TEST(UT_DataConcurrencyTCP, verifyTCPBroadcastBackpressure_bySlowClient_expectFastNotBlocked) {
    GTEST_SKIP() << "⚪ TODO: Implement TCP backpressure isolation test";
}

TEST(UT_DataConcurrencyTCP, verifyTCPAcceptSendRace_byConcurrentAcceptAndSend_expectNoInterference) {
    GTEST_SKIP() << "⚪ TODO: Implement TCP accept/send race test";
}

TEST(UT_DataConcurrencyTCP, verifyTCPPartialWriteRetry_byConcurrentEAGAIN_expectIsolatedRetry) {
    GTEST_SKIP() << "⚪ TODO: Implement TCP partial write retry test";
}

TEST(UT_DataConcurrencyTCP, verifyTCPPartialWriteIsolation_byConcurrentPartialWrites_expectNoCorruption) {
    GTEST_SKIP() << "⚪ TODO: Implement TCP partial write isolation test";
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>END OF UNIT TESTING IMPLEMENTATION========================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF TODO/IMPLEMENTATION TRACKING SECTION=============================================
// 🔴 TCP CONCURRENCY IMPLEMENTATION STATUS - TDD Red→Green Methodology
//
// PURPOSE:
//   Track TCP-specific concurrency test implementation with focus on network nuances.
//   Ensure comprehensive coverage of TCP socket thread-safety scenarios.
//
// STATUS LEGEND:
//   ⚪ TODO/PLANNED:      Designed but not implemented yet
//   🔴 RED/FAILING:       Test written, failing (need production code fix)
//   🟢 GREEN/PASSED:      Test written and passing
//   ⚠️  ISSUES:           Known problem needing attention
//   🚫 BLOCKED:          Cannot proceed due to dependency
//
// PRIORITY LEVELS:
//   🥇 CRITICAL:  Must-have, can crash process (SIGPIPE, deadlock)
//   🥈 HIGH:      Important for correctness (thread-safety, error handling)
//   🥉 MEDIUM:    Quality assurance (edge cases, TCP adaptations)
//
// WORKFLOW:
//   1. Implement Fast-Fail TCP-Six first (smoke tests)
//   2. Complete CRITICAL priority (TC-T7, TC-T5, TC-T1)
//   3. Move to HIGH priority (partial write, accept race)
//   4. Add MEDIUM priority (TCP adaptations from FIFO tests)
//   5. Mark status: ⚪ TODO → 🔴 RED → 🟢 GREEN
//
//===================================================================================================
// 🎯 FAST-FAIL TCP-SIX - TCP Socket Smoke Tests (Run First)
//===================================================================================================
//   ⚪ FF-TCP-1: Single-Thread TCP Baseline
//        - Description: Simple client-server exchange over TCP
//        - Category: Smoke Test
//        - Estimated effort: 30 min
//        - Depends on: None
//        - Verification: Connection success, data exchange works
//
//   ⚪ FF-TCP-2: Two-Thread TCP Send Race
//        - Description: 2 threads sending to same TCP socket
//        - Category: Smoke Test
//        - Estimated effort: 1 hour
//        - Depends on: FF-TCP-1 GREEN
//        - Tools: ThreadSanitizer
//        - Verification: No EPIPE, data intact
//
//   ⚪ FF-TCP-3: TCP Disconnect During Send
//        - Description: Disconnect while thread in sendDAT
//        - Category: Smoke Test
//        - Estimated effort: 1 hour
//        - Depends on: FF-TCP-1 GREEN
//        - Verification: LINK_BROKEN returned, no crash
//
//   ⚪ FF-TCP-4: SIGPIPE Safety Smoke
//        - Description: Write to broken connection, expect no crash
//        - Category: Smoke Test (CRITICAL safety)
//        - Estimated effort: 1.5 hours
//        - Depends on: FF-TCP-3 GREEN
//        - Tools: Signal handler test, strace
//        - Verification: Process not killed by SIGPIPE
//
//   ⚪ FF-TCP-5: TCP Accept+Send Concurrent
//        - Description: 1 thread accepting, 1 sending to existing
//        - Category: Smoke Test
//        - Estimated effort: 1.5 hours
//        - Depends on: FF-TCP-1, FF-TCP-2 GREEN
//        - Verification: No blocking, both succeed
//
//   ⚪ FF-TCP-6: TCP Partial Write Smoke
//        - Description: Large payload likely to trigger partial send()
//        - Category: Smoke Test
//        - Estimated effort: 1 hour
//        - Depends on: FF-TCP-1 GREEN
//        - Verification: Full data delivered despite partial writes
//
// 🚪 GATE: Fast-Fail TCP-Six must be GREEN before main TCP tests
//
//===================================================================================================
// 🥇 CRITICAL PRIORITY - Can Crash Process (Production Blockers)
//===================================================================================================
//   ⚪ [@AC-T7,US-T6] TC-T7: verifyTCPSIGPIPESafety_byBrokenPipeDuringConcurrentSend_expectNoCrash
//        - Description: MOST CRITICAL - SIGPIPE crash prevention
//        - Category: Signal safety (process survival)
//        - Priority: MOST CRITICAL
//        - Estimated effort: 4 hours
//        - Depends on: FF-TCP-4 GREEN
//        - Tools: Signal handler, strace monitoring, concurrent senders
//        - Verification: Process survives SIGPIPE, returns EPIPE error
//        - Risk: Can crash entire application if not handled
//        - Implementation notes:
//          * Install signal(SIGPIPE, SIG_IGN) or use MSG_NOSIGNAL
//          * Peer close during concurrent send()
//          * Verify all sender threads get EPIPE/LINK_BROKEN
//
//   ⚪ [@AC-T5,US-T4] TC-T5: verifyTCPDisconnectPropagation_by4ConcurrentSenders_expectAllGetLinkBroken
//        - Description: Disconnect error propagation to all threads
//        - Category: Error handling (graceful degradation)
//        - Priority: CRITICAL
//        - Estimated effort: 3 hours
//        - Depends on: FF-TCP-3 GREEN
//        - Tools: Thread synchronization, error counter
//        - Verification: All 4 threads receive LINK_BROKEN, no hangs
//        - Implementation notes:
//          * Peer abrupt close (ECONNRESET simulation)
//          * Connection state broadcast to all I/O threads
//          * No use-after-free on connection cleanup
//
//   ⚪ [@AC-T1,US-T1] TC-T1: verifyTCPSocketWriteSafety_by10Threads100Chunks_expectSerialized
//        - Description: Basic TCP socket write thread-safety
//        - Category: Thread-safety (fundamental correctness)
//        - Priority: CRITICAL
//        - Estimated effort: 4 hours
//        - Depends on: FF-TCP-2 GREEN
//        - Tools: ThreadSanitizer, data integrity checker
//        - Verification: All 1000 chunks delivered intact, no corruption
//        - Implementation notes:
//          * Mutex/lock around socket write() calls
//          * Handle SO_SNDBUF full (EAGAIN)
//          * Verify send() serialization
//
// 🚪 GATE: CRITICAL tests GREEN required for production deployment
//
//===================================================================================================
// 🥈 HIGH PRIORITY - Protocol Correctness (Quality Assurance)
//===================================================================================================
//   ⚪ [@AC-T6,US-T5] TC-T6: verifyTCPPartialWriteIsolation_byConcurrentPartialWrites_expectNoCorruption
//        - Description: Partial write resume doesn't corrupt other threads
//        - Category: Partial I/O handling
//        - Estimated effort: 5 hours
//        - Depends on: TC-T1 GREEN, FF-TCP-6 GREEN
//        - Tools: Network simulator (limit send buffer), state tracker
//        - Verification: Thread-local partial state, no cross-contamination
//        - Implementation notes:
//          * send() returns < requested bytes
//          * Each thread tracks own partial write position
//          * Retry logic thread-local, not global
//
//   ⚪ [@AC-T2,US-T1] TC-T2: verifyTCPPartialWriteRetry_byConcurrentEAGAIN_expectIsolatedRetry
//        - Description: EAGAIN handling under concurrency
//        - Category: Non-blocking I/O
//        - Estimated effort: 4 hours
//        - Depends on: TC-T1 GREEN
//        - Tools: Small SO_SNDBUF, high send rate
//        - Verification: Retry state per-thread, all sends eventually succeed
//
//   ⚪ [@AC-T4,US-T3] TC-T4: verifyTCPAcceptSendRace_byConcurrentAcceptAndSend_expectNoInterference
//        - Description: Accept doesn't block sends, and vice versa
//        - Category: Connection state races
//        - Estimated effort: 4 hours
//        - Depends on: TC-T1 GREEN, FF-TCP-5 GREEN
//        - Tools: Accept queue monitor, send latency tracker
//        - Verification: Accept and send independent, no mutual blocking
//        - Implementation notes:
//          * Accept queue mutex separate from send path
//          * New connection initialization non-blocking
//          * Existing client I/O continues during accept
//
//   ⚪ [@AC-T3,US-T2] TC-T3: verifyTCPBroadcastBackpressure_bySlowClient_expectFastNotBlocked
//        - Description: Per-client backpressure isolation
//        - Category: Flow control
//        - Estimated effort: 5 hours
//        - Depends on: TC-T1 GREEN
//        - Tools: Slow receiver simulation (small SO_RCVBUF)
//        - Verification: 4 fast clients unaffected by 1 slow client
//        - Implementation notes:
//          * Per-client send buffers
//          * Async I/O or thread-per-client
//          * Slow client timeout or disconnect
//
// 🚪 GATE: HIGH priority for production-grade TCP implementation
//
//===================================================================================================
// 🥉 MEDIUM PRIORITY - TCP Adaptations & Edge Cases
//===================================================================================================
//   ⚪ TC-T8: verifyP1MultiClientSendTCP_by10Clients500Chunks_expectAll5000Received
//        - Description: Pattern-1 multi-client over TCP (adaptation from FIFO)
//        - Category: TCP adaptation (architectural validation)
//        - Estimated effort: 2 hours
//        - Depends on: TC-T1 GREEN, UT_DataConcurrency TC-1 GREEN
//        - Verification: Same as FIFO TC-1 but with TCP transport
//
//   ⚪ TC-T9: verifyP2ServiceBroadcastTCP_by8Clients1000Chunks_expectAllReceive
//        - Description: Pattern-2 broadcast over TCP (adaptation from FIFO)
//        - Category: TCP adaptation
//        - Estimated effort: 2 hours
//        - Depends on: TC-T1 GREEN, UT_DataConcurrency TC-4 GREEN
//        - Verification: 8 clients × 1000 chunks = 8000 receptions
//
//   ⚪ TC-T10: verifyMultiLinkIsolationTCP_by5Links4ThreadsEach_expectFullIsolation
//        - Description: Multi-link isolation with TCP sockets (adaptation)
//        - Category: TCP adaptation (isolation)
//        - Estimated effort: 3 hours
//        - Depends on: TC-T1 GREEN, UT_DataConcurrency TC-11 GREEN
//        - Verification: 5 TCP connections, no cross-contamination
//
//   ⚪ TC-T11: verifyCallbackReentrantTCP_bySendFromCallbackSameLink_expectNoDeadlock
//        - Description: Callback deadlock test over TCP (adaptation)
//        - Category: TCP adaptation (deadlock)
//        - Estimated effort: 3 hours
//        - Depends on: TC-T1 GREEN, UT_DataConcurrency TC-15 GREEN
//        - Verification: No deadlock with TCP socket locks
//
//===================================================================================================
// 📊 PROGRESS SUMMARY
//===================================================================================================
// Fast-Fail TCP-Six:  0/6  GREEN (⚪⚪⚪⚪⚪⚪)
// CRITICAL Priority:  0/3  GREEN (⚪⚪⚪)
// HIGH Priority:      0/4  GREEN (⚪⚪⚪⚪)
// MEDIUM Priority:    0/4  GREEN (⚪⚪⚪⚪)
// Total TCP Tests:    0/17 GREEN
//
// Next Action: Implement Fast-Fail TCP-Six → CRITICAL tests (TC-T7, TC-T5, TC-T1)
//
//===================================================================================================
// 🛠️ TCP IMPLEMENTATION ROADMAP (4-Week Plan)
//===================================================================================================
// Week 1: TCP Infrastructure + Fast-Fail TCP-Six
//   - Days 1-2: TCP test infrastructure (TCPConnectionInfo, signal handler)
//   - Days 3-5: Implement & validate Fast-Fail TCP-Six
//   - Goal: All TCP smoke tests GREEN
//
// Week 2: CRITICAL Priority Tests (TC-T7, TC-T5, TC-T1)
//   - Days 1-2: TC-T7 SIGPIPE safety (MOST CRITICAL)
//   - Day 3: TC-T5 disconnect propagation
//   - Days 4-5: TC-T1 socket write thread-safety
//   - Goal: Production blockers resolved
//
// Week 3: HIGH Priority Tests (TC-T6, TC-T2, TC-T4, TC-T3)
//   - Days 1-2: Partial write handling (TC-T6, TC-T2)
//   - Day 3: Accept/send race (TC-T4)
//   - Days 4-5: Broadcast backpressure (TC-T3)
//   - Goal: TCP protocol correctness validated
//
// Week 4: MEDIUM Priority (TCP Adaptations) + Integration
//   - Days 1-3: Implement TC-T8 through TC-T11
//   - Days 4-5: Full suite with ThreadSanitizer, integration testing
//   - Goal: Complete TCP concurrency coverage
//
//===================================================================================================
// 🔧 TCP-SPECIFIC IMPLEMENTATION NOTES
//===================================================================================================
// 1. SIGPIPE Handling (TC-T7):
//    ✓ Install signal(SIGPIPE, SIG_IGN) in IOC initialization
//    ✓ OR use MSG_NOSIGNAL flag in send()
//    ✓ Convert EPIPE to IOC_RESULT_LINK_BROKEN
//
// 2. Socket Thread-Safety (TC-T1):
//    ✓ Mutex around socket write() system call
//    ✓ Per-connection lock (not global lock)
//    ✓ Consider lock-free queue for send requests
//
// 3. Partial Write Handling (TC-T6, TC-T2):
//    ✓ Thread-local partial write state
//    ✓ Resume from offset on next call
//    ✓ Atomic completion detection
//
// 4. Connection State (TC-T5):
//    ✓ Atomic connection state flag
//    ✓ Broadcast disconnect event to all I/O threads
//    ✓ Graceful cleanup without use-after-free
//
// 5. Testing Tools:
//    ✓ ThreadSanitizer: -fsanitize=thread
//    ✓ strace: Monitor socket syscalls
//    ✓ tcpdump: Network packet analysis
//    ✓ iptables: Simulate network conditions
//
//===================================================================================================
// ✅ COMPLETED TESTS (for reference, remove after stable)
//===================================================================================================
// (None yet - all TCP tests in TODO state)
//
//======>END OF TODO/IMPLEMENTATION TRACKING SECTION===============================================
