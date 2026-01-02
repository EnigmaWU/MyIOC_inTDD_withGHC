///////////////////////////////////////////////////////////////////////////////////////////////////
// UT_DataConcurrency.cxx - Data API Concurrency Testing (FIFO Protocol)
//
// PURPOSE:
//   Verify thread-safety and synchronization of IOC Data APIs (sendDAT/recvDAT/flushDAT)
//   using FIFO protocol for local process communication.
//   Focuses on race conditions, deadlocks, and concurrent state transitions.
//
// CATDD METHODOLOGY:
//   This file follows Comment-alive Test-Driven Development (CaTDD):
//   - Phase 2: DESIGN - Comprehensive test design in comments
//   - Phase 3: IMPLEMENTATION - TDD Red→Green cycle
//
// PRIORITY CLASSIFICATION:
//   P2: Design-Oriented → Concurrency
//   PROMOTED TO P1 LEVEL due to high risk score:
//     - Impact: 3 (Deadlock/Data Corruption in production apps)
//     - Likelihood: 3 (IOC used in multi-threaded environments)
//     - Uncertainty: 2 (Complex inter-thread coordination)
//     - Score: 18 → Critical priority
//
// PROTOCOL COVERAGE:
//   - This file: FIFO (local process IPC)
//   - See UT_DataConcurrencyTCP.cxx for TCP network protocol
//
// RELATIONSHIPS:
//   - Depends on: Source/IOC_Data.c, Source/_IOC_SrvProtoFifo.c
//   - Related tests: UT_DataConcurrencyTCP.cxx (TCP variant)
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
 *   [WHAT] This file verifies IOC Data API concurrency and thread safety for FIFO protocol.
 *   [WHERE] in the IOC Data subsystem for local process data streaming.
 *   [WHY] to ensure no deadlocks or race conditions occur during multi-threaded data operations.
 *
 * SCOPE:
 *   - In scope:
 *     • Concurrent IOC_sendDAT (same link, different links)
 *     • Concurrent IOC_recvDAT (polling mode, callback mode)
 *     • Concurrent IOC_flushDAT during active sending
 *     • Mixed send/recv/flush operations
 *     • Callback re-entrancy and deadlock prevention
 *     • Both architectural patterns (P1: Svc=Receiver, P2: Svc=Sender)
 *   - Out of scope:
 *     • TCP protocol (see UT_DataConcurrencyTCP.cxx)
 *     • Stress testing (see UT_DataRobustness.cxx)
 *     • Basic functionality (see UT_DataTypical.cxx)
 *
 * KEY CONCEPTS:
 *   - Thread Safety: Multiple threads accessing Data APIs without corruption
 *   - Pattern-1 (P1): Service=DatReceiver, Client=DatSender (data collection server)
 *   - Pattern-2 (P2): Service=DatSender, Client=DatReceiver (broadcast server)
 *   - Deadlock Prevention: No circular dependencies in lock acquisition
 *   - Link Isolation: Operations on different LinkIDs are independent
 */
//======>END OF OVERVIEW OF THIS UNIT TESTING FILE=================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF FREELY DRAFTED IDEAS=============================================================
/**
 * BRAINSTORMING: Raw concurrency test ideas before structuring into US/AC/TC
 * (CaTDD Step 2: Freely draft without format constraints)
 *
 * What if scenarios for FIFO concurrency:
 *  • What if 10 clients hammer same service concurrently? → Pattern-1 multi-client stress
 *  • What if service broadcasts to 8 clients at once? → Pattern-2 broadcast safety
 *  • What if callback calls sendDAT on same link? → CRITICAL deadlock scenario
 *  • What if callback calls sendDAT on different link? → Bi-directional flow test
 *  • What if one link blocks full, do others continue? → Link isolation critical
 *  • What if 5 threads flush same link simultaneously? → Flush serialization
 *  • What if send/recv/flush all happen at once? → Mixed operation race
 *  • What if 8 threads poll same recvDAT? → Receiver coordination
 *  • What if thread A sends while thread B closes link? → Graceful error propagation
 *  • What if nested callbacks A→B→A? → Infinite recursion detection
 *
 * Edge cases to explore:
 *  • Thread count = 1 (baseline), 2 (simple race), 4 (realistic), 16 (stress)
 *  • Link count = 1 (focus), 5 (isolation), 100 (capacity)
 *  • Payload size = 1B (minimal), 1KB (typical), 100KB (large)
 *  • Buffer states = empty, partial, full, overflow
 *  • Timing = simultaneous start, staggered, random
 *
 * Gotchas to verify:
 *  • FIFO shared memory race conditions
 *  • Circular buffer wraparound under concurrency
 *  • Callback execution context safety
 *  • Lock-free vs locked data structures
 *  • Memory barriers and cache coherency
 */
//======>END OF FREELY DRAFTED IDEAS===============================================================

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
 *   P4 🎯 ADDONS:          Optional (Demo, Examples)
 *
 * DEFAULT TEST ORDER:
 *   P1: Typical → Edge → Misuse → Fault
 *   P2: State → Capability → Concurrency
 *   P3: Performance → Robust → Compatibility → Configuration
 *   P4: Demo/Example
 *
 * CONTEXT-SPECIFIC ADJUSTMENTS:
 *   - Highly Concurrent: Promote Concurrency to P1 level (APPLIED HERE)
 *   - Data Streaming: Thread-safety critical for production apps
 *
 * RISK-DRIVEN ADJUSTMENT:
 *   Score = Impact (3) × Likelihood (3) × Uncertainty (2) = 18
 *   → PROMOTED TO P1 LEVEL
 *
 *===================================================================================================
 * PRIORITY-2 PROMOTED TO P1: CONCURRENCY TESTING (Thread Safety)
 *===================================================================================================
 *
 *   🚀 CONCURRENCY: Thread safety and synchronization for Data APIs
 *      - Purpose: Validate concurrent access and find race conditions
 *      - Examples: Multi-thread send, callback deadlock, link isolation
 *      - Status: THIS FILE - FIFO protocol coverage
 *************************************************************************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF FAST-FAIL CONCURRENCY SIX========================================================
/**
 * Run these FIRST to catch common concurrency bugs quickly (before full test suite)
 * Goal: Fail fast on obvious issues, save time on complex scenarios
 *
 * CONCURRENCY FAST-FAIL SIX:
 *
 * 1. **Single-Thread Baseline**: Verify non-concurrent operation works
 *    - Test: Single sender, single receiver, sequential operations
 *    - Purpose: Prove basic functionality before adding concurrency
 *    - Fail indicator: If this fails, fix basic functionality first
 *
 * 2. **Two-Thread Simple Race**: Detect basic race conditions
 *    - Test: 2 threads sending on same link, verify no corruption
 *    - Purpose: Expose unsynchronized shared state access
 *    - Fail indicator: Data corruption, crashes, inconsistent results
 *
 * 3. **Deadlock Timeout Test**: Catch obvious deadlocks (5-second max)
 *    - Test: Callback calls sendDAT on same link, must complete in 5s
 *    - Purpose: Detect circular lock dependencies
 *    - Fail indicator: Test hangs/timeout
 *
 * 4. **Thread Count = CPU Cores**: Realistic concurrency level
 *    - Test: N threads (N=CPU cores) concurrent operations
 *    - Purpose: Real-world multi-core behavior
 *    - Fail indicator: Performance degradation, race conditions
 *
 * 5. **Link Isolation Smoke**: Verify no cross-link contamination
 *    - Test: 3 links, each with unique data, verify separation
 *    - Purpose: Catch global lock or shared state bugs
 *    - Fail indicator: Data mixing between links
 *
 * 6. **Callback Re-entry Smoke**: Simplest echo pattern
 *    - Test: Minimal callback that sends on different link
 *    - Purpose: Baseline callback safety before complex scenarios
 *    - Fail indicator: Deadlock, stack overflow, crash
 *
 * USAGE:
 *   - Run Fast-Fail Six before every major code change
 *   - If any fail, stop and debug before proceeding to full suite
 *   - Green Fast-Fail Six = safe to run comprehensive tests
 */
//======>END OF FAST-FAIL CONCURRENCY SIX==========================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF USER STORY=======================================================================
/**
 * COVERAGE MATRIX: 4D Design Space
 *
 * ┌───────────┬──────────┬────────────┬──────────────┬─────────────────────────────┐
 * │ Protocol  │ Pattern  │ Concurr.   │ API Op       │ User Story                  │
 * ├───────────┼──────────┼────────────┼──────────────┼─────────────────────────────┤
 * │ FIFO      │ P1       │ Multi-T    │ sendDAT      │ US-1: Multi-client send     │
 * │ FIFO      │ P2       │ Multi-T    │ sendDAT      │ US-2: Service broadcast     │
 * │ FIFO      │ P1       │ Multi-T    │ recvDAT      │ US-3: Service multi-recv    │
 * │ FIFO      │ P2       │ Multi-T    │ recvDAT      │ US-4: Client multi-poll     │
 * │ FIFO      │ Both     │ Multi-Link │ sendDAT      │ US-5: Link isolation        │
 * │ FIFO      │ Both     │ Callback   │ CbRecvDat_F  │ US-6: Callback safety       │
 * │ FIFO      │ Both     │ Re-entrant │ Send-from-Cb │ US-7: Deadlock prevention   │
 * │ FIFO      │ Both     │ Mixed      │ Send+Recv    │ US-8: Mixed ops race-free   │
 * │ FIFO      │ Both     │ Multi-T    │ flushDAT     │ US-9: Flush thread safety   │
 * └───────────┴──────────┴────────────┴──────────────┴─────────────────────────────┘
 *
 * Pattern Legend:
 *   P1 = Service as DatReceiver, Client as DatSender (data collection pattern)
 *   P2 = Service as DatSender, Client as DatReceiver (broadcast pattern)
 *
 * USER STORIES:
 *
 *  US-1: AS a multi-client application using Pattern-1 (P1: Svc=Receiver, Client=Sender),
 *        I WANT multiple DatSender clients to call IOC_sendDAT concurrently to the service,
 *        SO THAT each client can stream data independently without blocking others.
 *
 *  US-2: AS a broadcast server using Pattern-2 (P2: Svc=Sender, Client=Receiver),
 *        I WANT service to call IOC_sendDAT concurrently to multiple client connections,
 *        SO THAT I can efficiently push data to all clients without serialization.
 *
 *  US-3: AS a service with Pattern-1 receiving from multiple senders,
 *        I WANT to use multiple threads calling IOC_recvDAT (polling) for different clients,
 *        SO THAT I can scale my data consumption across CPU cores.
 *
 *  US-4: AS a multi-threaded client with Pattern-2 receiving from service,
 *        I WANT to call IOC_recvDAT from multiple threads on same LinkID safely,
 *        SO THAT my receiver threads coordinate without data loss or duplication.
 *
 *  US-5: AS a developer managing multiple data links,
 *        I WANT concurrent IOC_sendDAT/recvDAT calls on different LinkIDs to be isolated,
 *        SO THAT operations on one link don't block or interfere with others.
 *
 *  US-6: AS a DatReceiver using callback mode (CbRecvDat_F),
 *        I WANT main thread IOC_sendDAT to be safe while callback executes,
 *        SO THAT I can continue operations without waiting for callback completion.
 *
 *  US-7: AS a bi-directional data application,
 *        I WANT to safely call IOC_sendDAT from within CbRecvDat_F callback,
 *        SO THAT my request-response pattern doesn't deadlock the system.
 *
 *  US-8: AS a complex application with mixed data operations,
 *        I WANT concurrent send/recv/flush operations on same link to be race-free,
 *        SO THAT my data integrity is maintained under concurrent load.
 *
 *  US-9: AS a sender with flush requirements,
 *        I WANT IOC_flushDAT to be thread-safe during concurrent IOC_sendDAT,
 *        SO THAT I can force transmission from monitoring threads safely.
 */
//======>END OF USER STORY=========================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//=======>BEGIN OF ACCEPTANCE CRITERIA=============================================================
/**
 * ACCEPTANCE CRITERIA define WHAT should be tested (make User Stories testable)
 *
 * FORMAT: GIVEN [initial context], WHEN [trigger/action], THEN [expected outcome]
 *
 *-------------------------------------------------------------------------------------------------
 * [@US-1] Pattern-1: Multi-client concurrent sending to service
 *-------------------------------------------------------------------------------------------------
 *  AC-1: GIVEN DatReceiver service online with FIFO protocol,
 *         WHEN 10 DatSender clients call IOC_sendDAT concurrently (each sends 500 chunks),
 *         THEN all clients succeed without errors,
 *          AND service receives all 5000 chunks via callback,
 *          AND no data corruption occurs (payload integrity verified).
 *
 *  AC-2: GIVEN DatReceiver service with limited buffer capacity,
 *         WHEN multiple clients send concurrently causing buffer pressure,
 *         THEN clients receive proper IOC_RESULT_BUFFER_FULL when appropriate,
 *          AND no data loss occurs after buffer drains,
 *          AND system recovers to normal operation.
 *
 *  AC-3: GIVEN clients sending different payload sizes concurrently (1KB-100KB),
 *         WHEN mixed sizes transmitted on multiple client links,
 *         THEN all payloads received intact,
 *          AND no size corruption or payload mixing between clients.
 *
 *-------------------------------------------------------------------------------------------------
 * [@US-2] Pattern-2: Service concurrent broadcast to multiple clients
 *-------------------------------------------------------------------------------------------------
 *  AC-1: GIVEN DatSender service with 8 DatReceiver clients connected,
 *         WHEN service broadcasts 1000 chunks to all clients from single thread,
 *         THEN all clients receive all chunks via callback,
 *          AND broadcast serialization is thread-safe,
 *          AND payload integrity maintained for all recipients.
 *
 *  AC-2: GIVEN DatSender service with multiple sender threads,
 *         WHEN each thread sends to different client subset concurrently,
 *         THEN link isolation maintained (no cross-client contamination),
 *          AND all sends succeed,
 *          AND clients receive correct data streams.
 *
 *  AC-3: GIVEN service broadcasting under client consumption rate mismatch,
 *         WHEN slow client causes backpressure while others consume normally,
 *         THEN fast clients not blocked by slow client,
 *          AND proper flow control per client,
 *          AND no system-wide deadlock.
 *
 *-------------------------------------------------------------------------------------------------
 * [@US-3] Pattern-1: Service multi-threaded receive
 *-------------------------------------------------------------------------------------------------
 *  AC-1: GIVEN service with 5 client connections,
 *         WHEN service uses 5 threads, each polling IOC_recvDAT on one client link,
 *         THEN perfect link isolation (each thread receives only its link's data),
 *          AND no data mixing between links,
 *          AND all threads make independent progress.
 *
 *  AC-2: GIVEN service thread pool polling multiple clients,
 *         WHEN some links have no data (NO_DATA timeout),
 *         THEN timeout handling thread-safe,
 *          AND threads with available data continue processing,
 *          AND no starvation of any thread.
 *
 *  AC-3: GIVEN high-frequency polling from multiple service threads,
 *         WHEN clients send at different rates,
 *         THEN threads coordinate safely on per-link basis,
 *          AND no deadlock or race conditions,
 *          AND proper round-robin or priority scheduling if implemented.
 *
 *-------------------------------------------------------------------------------------------------
 * [@US-4] Pattern-2: Client multi-threaded polling
 *-------------------------------------------------------------------------------------------------
 *  AC-1: GIVEN client with 8 receiver threads polling same LinkID,
 *         WHEN service sends 1000 chunks,
 *         THEN exactly 1000 chunks received total (no duplication),
 *          AND each chunk received by exactly one thread,
 *          AND no data corruption from thread contention.
 *
 *  AC-2: GIVEN concurrent receivers with timeout on same link,
 *         WHEN some threads timeout while others receive data,
 *         THEN proper NO_DATA handling per thread,
 *          AND no race on subsequent receives,
 *          AND timeout threads don't interfere with receiving threads.
 *
 *  AC-3: GIVEN high thread count (16 threads) polling aggressively,
 *         WHEN service sends slowly (data arrival slower than poll rate),
 *         THEN threads coordinate without spinning excessively,
 *          AND proper synchronization prevents thundering herd,
 *          AND CPU usage reasonable (not 100% spin).
 *
 *-------------------------------------------------------------------------------------------------
 * [@US-5] Multi-link isolation
 *-------------------------------------------------------------------------------------------------
 *  AC-1: GIVEN 5 LinkIDs each with 4 sender threads,
 *         WHEN all 20 threads call IOC_sendDAT concurrently,
 *         THEN complete link isolation (no cross-link data contamination),
 *          AND operations on one link don't affect timing of others,
 *          AND all links achieve expected throughput independently.
 *
 *  AC-2: GIVEN one link blocking in sendDAT (buffer full),
 *         WHEN other links sending normally,
 *         THEN non-blocking links continue unaffected,
 *          AND blocking link's threads properly wait,
 *          AND no system-wide lock contention.
 *
 *  AC-3: GIVEN concurrent flushDAT on 3 different links,
 *         WHEN all flushing simultaneously,
 *         THEN no mutual interference between links,
 *          AND all flush operations succeed independently,
 *          AND proper per-link flush semantics.
 *
 *-------------------------------------------------------------------------------------------------
 * [@US-6] Callback+API thread safety
 *-------------------------------------------------------------------------------------------------
 *  AC-1: GIVEN main thread calling IOC_sendDAT while CbRecvDat_F callback executing,
 *         WHEN both operations concurrent on different links,
 *         THEN both safe with proper synchronization,
 *          AND no race conditions on shared state,
 *          AND callback completion doesn't block main thread unnecessarily.
 *
 *  AC-2: GIVEN callback executing IOC_recvDAT (polling from callback),
 *         WHEN main thread also calls IOC_sendDAT,
 *         THEN proper synchronization prevents corruption,
 *          AND no deadlock from nested API calls,
 *          AND both operations complete successfully.
 *
 *  AC-3: GIVEN callback modifies user context while main reads it,
 *         WHEN concurrent access to shared user data,
 *         THEN IOC doesn't add additional races (user must protect own data),
 *          AND IOC internal state remains consistent,
 *          AND no torn reads of IOC-managed state.
 *
 *-------------------------------------------------------------------------------------------------
 * [@US-7] Deadlock prevention from callback re-entrancy
 *-------------------------------------------------------------------------------------------------
 *  AC-1: GIVEN CbRecvDat_F callback calls IOC_sendDAT on same LinkID,
 *         WHEN callback triggered during receive processing,
 *         THEN no deadlock occurs,
 *          AND either send succeeds or proper error returned,
 *          AND system remains responsive.
 *
 *  AC-2: GIVEN CbRecvDat_F callback calls IOC_sendDAT on different LinkID,
 *         WHEN both links active in bi-directional communication,
 *         THEN proper lock ordering prevents deadlock,
 *          AND bi-directional data flow works correctly,
 *          AND no circular dependency in lock acquisition.
 *
 *  AC-3: GIVEN nested callbacks (A receives → sends to B → B receives → sends to A),
 *         WHEN triggered by initial send,
 *         THEN system detects or allows safe nested execution,
 *          AND no infinite recursion,
 *          AND proper termination of nested call chains.
 *
 *-------------------------------------------------------------------------------------------------
 * [@US-8] Mixed operation race conditions
 *-------------------------------------------------------------------------------------------------
 *  AC-1: GIVEN concurrent sendDAT+recvDAT+flushDAT on same link,
 *         WHEN all three operations executing from different threads,
 *         THEN link state machine remains consistent,
 *          AND no internal corruption,
 *          AND all operations complete with proper results.
 *
 *  AC-2: GIVEN sendDAT in progress when flushDAT called from another thread,
 *         WHEN flush requested during active transmission,
 *         THEN flush either waits for send or queues properly,
 *          AND send completes correctly,
 *          AND flush then executes without data loss.
 *
 *  AC-3: GIVEN recvDAT polling while link closure from another thread,
 *         WHEN concurrent close during receive wait,
 *         THEN receive gets graceful IOC_RESULT_LINK_BROKEN,
 *          AND no crash or hang,
 *          AND link cleanup proceeds safely.
 *
 *-------------------------------------------------------------------------------------------------
 * [@US-9] Flush thread safety
 *-------------------------------------------------------------------------------------------------
 *  AC-1: GIVEN 5 threads calling IOC_flushDAT concurrently on same LinkID,
 *         WHEN all flush simultaneously,
 *         THEN flushes serialize safely,
 *          AND all return success or reasonable error,
 *          AND no internal state corruption.
 *
 *  AC-2: GIVEN flushDAT during high-rate sendDAT from another thread,
 *         WHEN sender churning (100 sends/sec) and flusher requests flush,
 *         THEN flush eventually succeeds without indefinite blocking,
 *          AND sender not starved,
 *          AND proper fairness between send and flush.
 *
 *  AC-3: GIVEN flush from monitoring thread while sender uses timeout,
 *         WHEN both operations have timeout configured,
 *         THEN proper timeout handling per operation,
 *          AND no timeout cascade (one timeout doesn't cause others),
 *          AND operations independently respect their timeouts.
 */
//=======>END OF ACCEPTANCE CRITERIA================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF TEST CASES=======================================================================
/**
 * TEST CASES define HOW to verify each Acceptance Criterion
 *
 * NAMING CONVENTION: verifyBehavior_byCondition_expectResult
 *
 * STATUS TRACKING:
 *  ⚪ TODO/PLANNED:      Designed but not implemented yet
 *  🔴 RED/IMPLEMENTED:   Test written and failing (need prod code)
 *  🟢 GREEN/PASSED:      Test written and passing
 *  ⚠️  ISSUES:           Known problem needing attention
 *
 *===================================================================================================
 * [@AC-1,US-1] Pattern-1: Multi-client concurrent sending
 *===================================================================================================
 *  ⚪ TC-1: verifyP1MultiClientSend_by10Clients500Chunks_expectAll5000Received
 *      @[Purpose]: Validate thread-safety of multiple clients sending to service concurrently
 *      @[Brief]: 10 DatSender clients × 500 chunks = 5000 total, service callback receives all
 *      @[Protocol]: FIFO
 *      @[Pattern]: P1 (Service=DatReceiver, Client=DatSender)
 *
 *  ⚪ TC-2: verifyP1MultiClientSend_byBufferPressure_expectProperFlowControl
 *      @[Purpose]: Test concurrent send behavior under buffer pressure
 *      @[Brief]: Clients send faster than service consumes, verify BUFFER_FULL handling
 *      @[Protocol]: FIFO
 *
 *  ⚪ TC-3: verifyP1MultiClientSend_byMixedPayloadSizes_expectNoCorruption
 *      @[Purpose]: Ensure payload integrity with mixed sizes under concurrency
 *      @[Brief]: Clients send 1KB, 10KB, 100KB concurrently, verify no mixing
 *      @[Protocol]: FIFO
 *
 *===================================================================================================
 * [@AC-1,US-2] Pattern-2: Service broadcast to multiple clients
 *===================================================================================================
 *  ⚪ TC-4: verifyP2ServiceBroadcast_by8Clients1000Chunks_expectAllReceive
 *      @[Purpose]: Validate service broadcasting to multiple clients safely
 *      @[Brief]: Service sends 1000 chunks, 8 clients each receive 1000 via callback
 *      @[Protocol]: FIFO
 *      @[Pattern]: P2 (Service=DatSender, Client=DatReceiver)
 *
 *  ⚪ TC-5: verifyP2ServiceMultiThread_byDifferentClientSubsets_expectLinkIsolation
 *      @[Purpose]: Test service using multiple threads sending to different clients
 *      @[Brief]: 4 service threads, each managing 2 clients, verify isolation
 *      @[Protocol]: FIFO
 *
 *  ⚪ TC-6: verifyP2ServiceBroadcast_bySlowClientBackpressure_expectFastNotBlocked
 *      @[Purpose]: Ensure slow client doesn't block fast clients
 *      @[Brief]: 1 slow client (sleep in callback), 7 fast clients, verify throughput
 *      @[Protocol]: FIFO
 *
 *===================================================================================================
 * [@AC-1,US-3] Pattern-1: Service multi-threaded receive
 *===================================================================================================
 *  ⚪ TC-7: verifyP1ServiceMultiRecv_by5ThreadsPolling_expectPerfectIsolation
 *      @[Purpose]: Validate service threads polling different client links independently
 *      @[Brief]: 5 service threads × 5 client links, verify no data mixing
 *      @[Protocol]: FIFO
 *
 *  ⚪ TC-8: verifyP1ServiceMultiRecv_byMixedDataAvailability_expectProperTimeout
 *      @[Purpose]: Test timeout handling when some links have no data
 *      @[Brief]: 3 links with data, 2 links empty, verify timeout threads don't block others
 *      @[Protocol]: FIFO
 *
 *===================================================================================================
 * [@AC-1,US-4] Pattern-2: Client multi-threaded polling
 *===================================================================================================
 *  ⚪ TC-9: verifyP2ClientMultiPoll_by8Threads1000Chunks_expectNoDuplication
 *      @[Purpose]: Ensure multiple client threads polling same link don't duplicate data
 *      @[Brief]: 8 threads polling, 1000 chunks sent, verify exactly 1000 received
 *      @[Protocol]: FIFO
 *
 *  ⚪ TC-10: verifyP2ClientMultiPoll_byTimeoutContention_expectProperCoordination
 *      @[Purpose]: Test timeout handling with multiple threads on same link
 *      @[Brief]: Mixed timeout/success scenarios, verify thread coordination
 *      @[Protocol]: FIFO
 *
 *===================================================================================================
 * [@AC-1,US-5] Multi-link isolation
 *===================================================================================================
 *  ⚪ TC-11: verifyMultiLinkIsolation_by5Links4ThreadsEach_expectFullIsolation
 *      @[Purpose]: Validate complete independence of operations on different links
 *      @[Brief]: 5 links × 4 threads = 20 concurrent senders, verify no interference
 *      @[Protocol]: FIFO
 *
 *  ⚪ TC-12: verifyMultiLinkIsolation_byOneBlockingOthersActive_expectNoInterference
 *      @[Purpose]: Ensure blocking on one link doesn't affect others
 *      @[Brief]: 1 link with buffer full blocking, 4 other links active
 *      @[Protocol]: FIFO
 *
 *===================================================================================================
 * [@AC-1,US-6] Callback+API thread safety
 *===================================================================================================
 *  ⚪ TC-13: verifyCallbackSafety_byMainSendDuringCallback_expectBothSafe
 *      @[Purpose]: Test main thread send while callback executes
 *      @[Brief]: Main sends to link A while callback from link B executing
 *      @[Protocol]: FIFO
 *
 *  ⚪ TC-14: verifyCallbackSafety_byCallbackRecvMainSend_expectNoDeadlock
 *      @[Purpose]: Callback calling recvDAT while main sends
 *      @[Brief]: Callback polls another link while main thread sends
 *      @[Protocol]: FIFO
 *
 *===================================================================================================
 * [@AC-1,US-7] Deadlock prevention from callback re-entrancy
 *===================================================================================================
 *  ⚪ TC-15: verifyCallbackReentrant_bySendFromCallbackSameLink_expectNoDeadlock
 *      @[Purpose]: Critical deadlock test - send from callback on same link
 *      @[Brief]: CbRecvDat_F calls IOC_sendDAT on same LinkID
 *      @[Protocol]: FIFO
 *
 *  ⚪ TC-16: verifyCallbackReentrant_bySendFromCallbackDiffLink_expectBidirectionalFlow
 *      @[Purpose]: Test bi-directional communication safety
 *      @[Brief]: Link A callback sends to Link B, Link B callback sends to Link A
 *      @[Protocol]: FIFO
 *
 *  ⚪ TC-17: verifyCallbackReentrant_byNestedCallbacks_expectSafeTermination
 *      @[Purpose]: Detect infinite recursion in nested callback chains
 *      @[Brief]: A→B→A callback chain with termination condition
 *      @[Protocol]: FIFO
 *
 *===================================================================================================
 * [@AC-1,US-8] Mixed operation race conditions
 *===================================================================================================
 *  ⚪ TC-18: verifyMixedOps_byConcurrentSendRecvFlush_expectStateMachineConsistent
 *      @[Purpose]: Test all three operations concurrent on same link
 *      @[Brief]: 3 threads: sender, receiver, flusher, verify state consistency
 *      @[Protocol]: FIFO
 *
 *  ⚪ TC-19: verifyMixedOps_byFlushDuringSend_expectProperQueueing
 *      @[Purpose]: Flush called during active send, verify ordering
 *      @[Brief]: Sender active, flusher requests flush, verify data ordering
 *      @[Protocol]: FIFO
 *
 *  ⚪ TC-20: verifyMixedOps_byCloseDuringRecv_expectGracefulError
 *      @[Purpose]: Link closure during concurrent receive
 *      @[Brief]: Receiver polling while closer closes link, verify LINK_BROKEN
 *      @[Protocol]: FIFO
 *
 *===================================================================================================
 * [@AC-1,US-9] Flush thread safety
 *===================================================================================================
 *  ⚪ TC-21: verifyFlushThreadSafe_by5ConcurrentFlushers_expectSerialization
 *      @[Purpose]: Multiple threads flushing same link
 *      @[Brief]: 5 threads call IOC_flushDAT simultaneously, verify serialization
 *      @[Protocol]: FIFO
 *
 *  ⚪ TC-22: verifyFlushThreadSafe_byFlushDuringHighRateSend_expectEventualSuccess
 *      @[Purpose]: Flush doesn't get starved by continuous sending
 *      @[Brief]: Sender at 100/sec, flusher requests flush, verify completion
 *      @[Protocol]: FIFO
 *
 *  ⚪ TC-23: verifyFlushThreadSafe_byTimeoutIndependence_expectProperHandling
 *      @[Purpose]: Flush and send timeouts are independent
 *      @[Brief]: Both operations with timeouts, verify no cascade
 *      @[Protocol]: FIFO
 */
//======>END OF TEST CASES=========================================================================
//======>END OF UNIT TESTING DESIGN================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF TODO/IMPLEMENTATION TRACKING SECTION=============================================
// 🔴 IMPLEMENTATION STATUS TRACKING - Organized by Priority and Category
//
// STATUS LEGEND:
//   ⚪ TODO/PLANNED:      Designed but not implemented
//   🔴 RED/IMPLEMENTED:   Test written and failing (need prod code)
//   🟢 GREEN/PASSED:      Test written and passing
//   ⚠️  ISSUES:           Known problem needing attention
//
// PRIORITY LEVELS:
//   🥇 HIGH:    Must-have for release (Core concurrency scenarios)
//   🥈 MEDIUM:  Important for quality (Edge cases, advanced patterns)
//   🥉 LOW:     Nice-to-have (Comprehensive coverage)
//
//=================================================================================================
// 🥇 HIGH PRIORITY – Core Concurrency Scenarios
//=================================================================================================
//   ⚪ [@AC-1,US-1] TC-1: verifyP1MultiClientSend_by10Clients500Chunks_expectAll5000Received
//   ⚪ [@AC-1,US-2] TC-4: verifyP2ServiceBroadcast_by8Clients1000Chunks_expectAllReceive
//   ⚪ [@AC-1,US-5] TC-11: verifyMultiLinkIsolation_by5Links4ThreadsEach_expectFullIsolation
//   ⚪ [@AC-1,US-7] TC-15: verifyCallbackReentrant_bySendFromCallbackSameLink_expectNoDeadlock – CRITICAL
//   ⚪ [@AC-1,US-7] TC-16: verifyCallbackReentrant_bySendFromCallbackDiffLink_expectBidirectionalFlow
//
//=================================================================================================
// 🥈 MEDIUM PRIORITY – Advanced Patterns & Edge Cases
//=================================================================================================
//   ⚪ [@AC-2,US-1] TC-2: verifyP1MultiClientSend_byBufferPressure_expectProperFlowControl
//   ⚪ [@AC-1,US-3] TC-7: verifyP1ServiceMultiRecv_by5ThreadsPolling_expectPerfectIsolation
//   ⚪ [@AC-1,US-4] TC-9: verifyP2ClientMultiPoll_by8Threads1000Chunks_expectNoDuplication
//   ⚪ [@AC-1,US-6] TC-13: verifyCallbackSafety_byMainSendDuringCallback_expectBothSafe
//   ⚪ [@AC-1,US-8] TC-18: verifyMixedOps_byConcurrentSendRecvFlush_expectStateMachineConsistent
//   ⚪ [@AC-1,US-9] TC-21: verifyFlushThreadSafe_by5ConcurrentFlushers_expectSerialization
//
//=================================================================================================
// 🥉 LOW PRIORITY – Comprehensive Coverage
//=================================================================================================
//   ⚪ [@AC-3,US-1] TC-3: verifyP1MultiClientSend_byMixedPayloadSizes_expectNoCorruption
//   ⚪ [@AC-2,US-3] TC-8: verifyP1ServiceMultiRecv_byMixedDataAvailability_expectProperTimeout
//   ⚪ [@AC-2,US-4] TC-10: verifyP2ClientMultiPoll_byTimeoutContention_expectProperCoordination
//   ⚪ [@AC-3,US-7] TC-17: verifyCallbackReentrant_byNestedCallbacks_expectSafeTermination
//   ⚪ [@AC-2,US-9] TC-22: verifyFlushThreadSafe_byFlushDuringHighRateSend_expectEventualSuccess
//
///////////////////////////////////////////////////////////////////////////////////////////////////
//======>END OF TODO/IMPLEMENTATION TRACKING SECTION===============================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF TEST HELPER UTILITIES============================================================
/**
 * CONCURRENCY TEST INFRASTRUCTURE:
 *
 * Core Utilities:
 *  • ConcurrencyCounters: Atomic counters for thread-safe verification
 *    - ChunksSent/Received, Errors, DeadlockDetected
 *    - Used across all tests for result aggregation
 *
 *  • DataChunk: Payload structure with integrity verification
 *    - SequenceNum: Detect duplication/loss
 *    - ClientID: Detect cross-client contamination
 *    - Checksum: Detect data corruption
 *    - Payload[128]: Actual data buffer
 *
 *  • ComputeChecksum(): Simple XOR checksum for corruption detection
 *    - Fast, deterministic, good enough for testing
 *
 * Pattern-Specific Contexts:
 *  • P1ServiceReceiverContext: Pattern-1 (Service=DatReceiver)
 *    - Aggregates received chunks from multiple senders
 *    - Mutex-protected vector for callback thread safety
 *
 *  • P2ClientReceiverContext: Pattern-2 (Service=DatSender)
 *    - Per-client receiver context (implemented below)
 *    - Verifies broadcast delivery integrity
 *
 * Callback Functions:
 *  • P1_CbRecvDat(): Callback for Pattern-1 service receiving (implemented below)
 *  • P2_CbRecvDat(): Callback for Pattern-2 client receiving [TODO]
 *
 * Future Utilities (TODO):
 *  • ThreadBarrier: Synchronize thread starts (wait for all ready)
 *  • DeadlockDetector: Timeout-based with thread dumps
 *  • MemoryLeakDetector: RAII-based resource tracking
 *  • RaceDetectorHelper: Integration with ThreadSanitizer
 */
//======>END OF TEST HELPER UTILITIES==============================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======BEGIN OF UNIT TESTING IMPLEMENTATION=======================================================

// Thread-safe context helpers for test cases
namespace {

// Atomic counters for verification
struct ConcurrencyCounters {
    std::atomic<uint32_t> ChunksSent{0};
    std::atomic<uint32_t> ChunksReceived{0};
    std::atomic<uint32_t> SendErrors{0};
    std::atomic<uint32_t> RecvErrors{0};
    std::atomic<uint32_t> FlushCount{0};
    std::atomic<bool> DeadlockDetected{false};
};

// Payload verification structure
struct DataChunk {
    uint32_t SequenceNum;
    uint32_t ClientID;
    uint32_t PayloadSize;
    uint8_t Checksum;
    char Payload[128];
};

// Compute simple checksum for data verification
uint8_t ComputeChecksum(const char* pData, size_t Size) {
    uint8_t Sum = 0;
    for (size_t i = 0; i < Size; ++i) {
        Sum ^= pData[i];
    }
    return Sum;
}

// Callback context for Pattern-1 (Service=Receiver)
struct P1ServiceReceiverContext {
    ConcurrencyCounters Counters;
    std::vector<DataChunk> ReceivedChunks;
    std::mutex Mutex;  // Protect ReceivedChunks vector
};

// Callback function for Pattern-1 service (DatReceiver)
IOC_Result_T P1_CbRecvDat(const IOC_DatDesc_pT pDatDesc, void* pCbPrivData) {
    auto* pCtx = static_cast<P1ServiceReceiverContext*>(pCbPrivData);

    // Verify payload integrity
    if (pDatDesc->PayloadSize >= sizeof(DataChunk)) {
        const auto* pChunk = static_cast<const DataChunk*>(pDatDesc->pPayload);
        uint8_t ExpectedChecksum = ComputeChecksum(pChunk->Payload, pChunk->PayloadSize);

        if (pChunk->Checksum == ExpectedChecksum) {
            std::lock_guard<std::mutex> Lock(pCtx->Mutex);
            pCtx->ReceivedChunks.push_back(*pChunk);
            pCtx->Counters.ChunksReceived.fetch_add(1);
        } else {
            pCtx->Counters.RecvErrors.fetch_add(1);
        }
    }

    return IOC_RESULT_SUCCESS;
}

// Callback context for Pattern-2 (Service=Sender, Client=Receiver)
struct P2ClientReceiverContext {
    uint32_t ClientID;
    ConcurrencyCounters Counters;
    std::vector<DataChunk> ReceivedChunks;
    std::mutex Mutex;
};

// Callback function for Pattern-2 client (DatReceiver)
IOC_Result_T P2_CbRecvDat(const IOC_DatDesc_pT pDatDesc, void* pCbPrivData) {
    auto* pCtx = static_cast<P2ClientReceiverContext*>(pCbPrivData);

    if (pDatDesc->PayloadSize >= sizeof(DataChunk)) {
        const auto* pChunk = static_cast<const DataChunk*>(pDatDesc->pPayload);

        std::lock_guard<std::mutex> Lock(pCtx->Mutex);
        pCtx->ReceivedChunks.push_back(*pChunk);
        pCtx->Counters.ChunksReceived.fetch_add(1);
    }

    return IOC_RESULT_SUCCESS;
}

}  // namespace

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF: [@AC-1,US-1] TC-1: Pattern-1 Multi-Client Concurrent Send======================

/**
 * @[Name]: verifyP1MultiClientSend_by10Clients500Chunks_expectAll5000Received
 * @[Purpose]: Validate thread-safety of multiple DatSender clients sending to service concurrently
 * @[Steps]:
 *   1) 🔧 SETUP: Online DatReceiver service with FIFO protocol, callback mode
 *   2) 🔧 SETUP: Create 10 DatSender clients, each connecting to service
 *   3) 🎯 BEHAVIOR: Each client thread sends 500 unique data chunks concurrently
 *   4) ✅ VERIFY: Service callback receives all 5000 chunks with correct payload
 *   5) ✅ VERIFY: No data corruption, no send errors
 *   6) 🧹 CLEANUP: Disconnect clients, offline service
 * @[Expect]: All 5000 chunks received intact, zero errors
 * @[Protocol]: FIFO
 * @[Pattern]: P1 (Service=DatReceiver, Client=DatSender)
 */
TEST(UT_DataConcurrency, verifyP1MultiClientSend_by10Clients500Chunks_expectAll5000Received) {
    //===SETUP===
    printf("🔧 SETUP: Pattern-1 Service=Receiver, 10 concurrent DatSender clients\n");

    // TODO: Implement test
    // 1. IOC_onlineService with UsageCapabilites=DatReceiver, FIFO protocol
    // 2. Create 10 client threads, each IOC_connectService with Usage=DatSender
    // 3. Each thread sends 500 DataChunks with unique sequence numbers
    // 4. Service callback accumulates received chunks
    //
    // KeyVerifyPoint-1: All 5000 chunks received (no loss)
    // KeyVerifyPoint-2: No data corruption (checksum valid)
    // KeyVerifyPoint-3: Zero send errors from all threads

    GTEST_SKIP() << "⚪ TODO: Implement Pattern-1 multi-client concurrent send test";
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF: [@AC-1,US-2] TC-4: Pattern-2 Service Broadcast=================================

/**
 * @[Name]: verifyP2ServiceBroadcast_by8Clients1000Chunks_expectAllReceive
 * @[Purpose]: Validate service broadcasting to multiple DatReceiver clients safely
 * @[Steps]:
 *   1) 🔧 SETUP: Online DatSender service with FIFO protocol
 *   2) 🔧 SETUP: Create 8 DatReceiver clients, each connecting with callback
 *   3) 🎯 BEHAVIOR: Service accepts all clients, then broadcasts 1000 chunks
 *   4) ✅ VERIFY: Each of 8 clients receives all 1000 chunks via callback
 *   5) ✅ VERIFY: Payload integrity for all clients
 *   6) 🧹 CLEANUP: Disconnect clients, offline service
 * @[Expect]: 8 clients × 1000 chunks = 8000 total receptions, zero errors
 * @[Protocol]: FIFO
 * @[Pattern]: P2 (Service=DatSender, Client=DatReceiver)
 */
TEST(UT_DataConcurrency, verifyP2ServiceBroadcast_by8Clients1000Chunks_expectAllReceive) {
    //===SETUP===
    printf("🔧 SETUP: Pattern-2 Service=Sender, 8 DatReceiver clients\n");

    // TODO: Implement test
    // 1. IOC_onlineService with UsageCapabilites=DatSender, FIFO protocol
    // 2. Create 8 clients, each IOC_connectService with Usage=DatReceiver, callback mode
    // 3. Service IOC_acceptClient for all 8
    // 4. Service broadcasts 1000 DataChunks to all clients
    //
    // KeyVerifyPoint-1: Each of 8 clients receives exactly 1000 chunks
    // KeyVerifyPoint-2: Total receptions = 8000 (8 clients × 1000)
    // KeyVerifyPoint-3: Payload integrity for all clients (checksum valid)

    GTEST_SKIP() << "⚪ TODO: Implement Pattern-2 service broadcast test";
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF: [@AC-1,US-5] TC-11: Multi-Link Isolation=======================================

/**
 * @[Name]: verifyMultiLinkIsolation_by5Links4ThreadsEach_expectFullIsolation
 * @[Purpose]: Validate complete independence of concurrent operations on different links
 * @[Steps]:
 *   1) 🔧 SETUP: Create 5 separate data links (Pattern-1)
 *   2) 🎯 BEHAVIOR: Launch 4 sender threads per link (20 threads total)
 *   3) 🎯 BEHAVIOR: Each thread sends 100 chunks to its assigned link
 *   4) ✅ VERIFY: Each link receives exactly 400 chunks (no cross-contamination)
 *   5) ✅ VERIFY: Link timing independent (no mutual blocking)
 *   6) 🧹 CLEANUP: Close all links
 * @[Expect]: Perfect link isolation, 5 links × 400 chunks each
 * @[Protocol]: FIFO
 */
TEST(UT_DataConcurrency, verifyMultiLinkIsolation_by5Links4ThreadsEach_expectFullIsolation) {
    //===SETUP===
    printf("🔧 SETUP: 5 links × 4 threads = 20 concurrent senders\n");

    // TODO: Implement test
    // 1. Create 5 service instances or 5 separate link pairs
    // 2. Launch 20 threads (4 per link)
    // 3. Each thread sends with LinkID-specific markers
    //
    // KeyVerifyPoint-1: Each link receives exactly 400 chunks (4 threads × 100)
    // KeyVerifyPoint-2: No cross-link data contamination (unique markers preserved)
    // KeyVerifyPoint-3: Link timing independence (no mutual blocking)

    GTEST_SKIP() << "⚪ TODO: Implement multi-link isolation test";
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF: [@AC-1,US-7] TC-15: Critical Deadlock Test=====================================

/**
 * @[Name]: verifyCallbackReentrant_bySendFromCallbackSameLink_expectNoDeadlock
 * @[Purpose]: CRITICAL - Test callback calling IOC_sendDAT on same LinkID doesn't deadlock
 * @[Steps]:
 *   1) 🔧 SETUP: Create bi-directional link (both sides can send/receive)
 *   2) 🎯 BEHAVIOR: CbRecvDat_F callback calls IOC_sendDAT on same LinkID
 *   3) ✅ VERIFY: Either send succeeds OR proper error (IOC_RESULT_REENTRANT_CALL?)
 *   4) ✅ VERIFY: No deadlock (test completes within timeout)
 *   5) 🧹 CLEANUP: Close link
 * @[Expect]: No deadlock, system remains responsive
 * @[Protocol]: FIFO
 * @[Risk]: CRITICAL - Common usage pattern, must not deadlock
 */
TEST(UT_DataConcurrency, verifyCallbackReentrant_bySendFromCallbackSameLink_expectNoDeadlock) {
    //===SETUP===
    printf("🔧 SETUP: CRITICAL deadlock test - send from callback on same link\n");

    // TODO: Implement critical test
    // 1. Create link with bidirectional capability
    // 2. Callback implementation calls IOC_sendDAT on same LinkID
    // 3. Use timeout to detect deadlock (e.g., 5 seconds max)
    // 4. Verify callback either completes or returns error

    GTEST_SKIP() << "⚪ TODO: Implement CRITICAL callback re-entrancy deadlock test";
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF: [@AC-1,US-7] TC-16: Bi-directional Flow========================================

/**
 * @[Name]: verifyCallbackReentrant_bySendFromCallbackDiffLink_expectBidirectionalFlow
 * @[Purpose]: Test bi-directional communication with callback-initiated sends
 * @[Steps]:
 *   1) 🔧 SETUP: Create 2 links (A ↔ B bidirectional)
 *   2) 🎯 BEHAVIOR: Link A callback sends to Link B, Link B callback sends to Link A
 *   3) 🎯 BEHAVIOR: Initiate data flow from one side
 *   4) ✅ VERIFY: Bi-directional flow works without deadlock
 *   5) ✅ VERIFY: Proper lock ordering (no circular dependency)
 *   6) 🧹 CLEANUP: Close both links
 * @[Expect]: Successful bi-directional flow, no deadlock
 * @[Protocol]: FIFO
 */
TEST(UT_DataConcurrency, verifyCallbackReentrant_bySendFromCallbackDiffLink_expectBidirectionalFlow) {
    //===SETUP===
    printf("🔧 SETUP: Bi-directional flow with cross-link callback sends\n");

    // TODO: Implement test
    // 1. Setup two separate links with bidirectional capability
    // 2. Link A callback sends response to Link B
    // 3. Link B callback sends response to Link A
    // 4. Trigger ping-pong exchange
    //
    // KeyVerifyPoint-1: Bi-directional flow works without deadlock
    // KeyVerifyPoint-2: Proper lock ordering prevents circular dependency
    // KeyVerifyPoint-3: Ping-pong terminates gracefully (no infinite loop)

    GTEST_SKIP() << "⚪ TODO: Implement bi-directional callback flow test";
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF: Additional Test Placeholders===================================================

// Placeholder tests for remaining test cases (TC-2 through TC-23)
// These follow the same structure as above examples
// Implement based on priority: HIGH → MEDIUM → LOW

TEST(UT_DataConcurrency, verifyP1MultiClientSend_byBufferPressure_expectProperFlowControl) {
    GTEST_SKIP() << "⚪ TODO: Implement buffer pressure flow control test";
}

TEST(UT_DataConcurrency, verifyP1ServiceMultiRecv_by5ThreadsPolling_expectPerfectIsolation) {
    GTEST_SKIP() << "⚪ TODO: Implement service multi-threaded polling test";
}

TEST(UT_DataConcurrency, verifyP2ClientMultiPoll_by8Threads1000Chunks_expectNoDuplication) {
    GTEST_SKIP() << "⚪ TODO: Implement client multi-threaded polling test";
}

TEST(UT_DataConcurrency, verifyMixedOps_byConcurrentSendRecvFlush_expectStateMachineConsistent) {
    GTEST_SKIP() << "⚪ TODO: Implement mixed operations concurrency test";
}

TEST(UT_DataConcurrency, verifyFlushThreadSafe_by5ConcurrentFlushers_expectSerialization) {
    GTEST_SKIP() << "⚪ TODO: Implement concurrent flush test";
}

// Additional test cases (TC-7 through TC-23) follow similar pattern...
// Implement incrementally based on priority tracking section above

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>END OF UNIT TESTING IMPLEMENTATION========================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF TODO/IMPLEMENTATION TRACKING SECTION=============================================
// 🔴 IMPLEMENTATION STATUS TRACKING - TDD Red→Green Methodology
//
// PURPOSE:
//   Track concurrency test implementation progress with clear visibility.
//   Ensure systematic coverage of all identified scenarios.
//
// STATUS LEGEND:
//   ⚪ TODO/PLANNED:      Designed but not implemented yet
//   🔴 RED/FAILING:       Test written, failing (need production code fix)
//   🟢 GREEN/PASSED:      Test written and passing
//   ⚠️  ISSUES:           Known problem needing attention
//   🚫 BLOCKED:          Cannot proceed due to dependency
//
// PRIORITY LEVELS:
//   🥇 HIGH:    Must-have for release (Core concurrency scenarios)
//   🥈 MEDIUM:  Important for quality (Edge cases, advanced patterns)
//   🥉 LOW:     Nice-to-have (Comprehensive coverage)
//
// WORKFLOW:
//   1. Implement Fast-Fail Six first (smoke tests)
//   2. Complete HIGH priority tests (critical concurrency)
//   3. Move to MEDIUM priority (advanced scenarios)
//   4. Add LOW priority tests (comprehensive coverage)
//   5. Mark status: ⚪ TODO → 🔴 RED → 🟢 GREEN
//
//===================================================================================================
// 🎯 FAST-FAIL SIX - Smoke Tests (Run First)
//===================================================================================================
//   ⚪ FF-1: Single-Thread Baseline
//        - Description: Verify non-concurrent operation works
//        - Category: Smoke Test
//        - Estimated effort: 30 min
//        - Depends on: None
//
//   ⚪ FF-2: Two-Thread Simple Race
//        - Description: 2 threads sending on same link
//        - Category: Smoke Test
//        - Estimated effort: 45 min
//        - Depends on: FF-1 GREEN
//
//   ⚪ FF-3: Deadlock Timeout Test
//        - Description: Callback sends on same link, 5s max
//        - Category: Smoke Test
//        - Estimated effort: 1 hour
//        - Depends on: FF-1, FF-2 GREEN
//        - Tools: DeadlockDetector utility
//
//   ⚪ FF-4: Thread Count = CPU Cores
//        - Description: Realistic concurrency level
//        - Category: Smoke Test
//        - Estimated effort: 45 min
//        - Depends on: FF-2 GREEN
//
//   ⚪ FF-5: Link Isolation Smoke
//        - Description: 3 links with unique data
//        - Category: Smoke Test
//        - Estimated effort: 1 hour
//        - Depends on: FF-2 GREEN
//
//   ⚪ FF-6: Callback Re-entry Smoke
//        - Description: Minimal echo pattern on different link
//        - Category: Smoke Test
//        - Estimated effort: 1 hour
//        - Depends on: FF-3 GREEN
//
// 🚪 GATE: Fast-Fail Six must be GREEN before proceeding to main tests
//
//===================================================================================================
// 🥇 HIGH PRIORITY - Core Concurrency Scenarios (Must-Have)
//===================================================================================================
//   ⚪ [@AC-1,US-1] TC-1: verifyP1MultiClientSend_by10Clients500Chunks_expectAll5000Received
//        - Description: Pattern-1 multi-client concurrent send
//        - Category: Multi-thread send (P1 architecture)
//        - Estimated effort: 3 hours
//        - Depends on: FF-2, FF-4 GREEN
//        - Tools: ConcurrencyCounters, DataChunk validation
//        - Verification: 5000 chunks received, zero errors, no corruption
//
//   ⚪ [@AC-1,US-2] TC-4: verifyP2ServiceBroadcast_by8Clients1000Chunks_expectAllReceive
//        - Description: Pattern-2 service broadcast to multiple receivers
//        - Category: Multi-thread receive (P2 architecture)
//        - Estimated effort: 3 hours
//        - Depends on: FF-2, FF-4 GREEN
//        - Tools: P2ClientReceiverContext, broadcast counters
//        - Verification: 8×1000=8000 total receptions, integrity preserved
//
//   ⚪ [@AC-1,US-5] TC-11: verifyMultiLinkIsolation_by5Links4ThreadsEach_expectFullIsolation
//        - Description: Multi-link isolation under concurrency
//        - Category: Link isolation (critical safety property)
//        - Estimated effort: 4 hours
//        - Depends on: TC-1 or TC-4 GREEN, FF-5 GREEN
//        - Tools: Link-specific markers, cross-link contamination detector
//        - Verification: 5 links × 400 chunks each, zero cross-contamination
//
//   ⚪ [@AC-1,US-7] TC-15: verifyCallbackReentrant_bySendFromCallbackSameLink_expectNoDeadlock
//        - Description: CRITICAL - Callback sends on same LinkID
//        - Category: Callback re-entrancy (deadlock prevention)
//        - Priority: MOST CRITICAL
//        - Estimated effort: 5 hours
//        - Depends on: FF-3, FF-6 GREEN
//        - Tools: DeadlockDetector (5s timeout), re-entrancy depth tracker
//        - Verification: Completes within 5s, no deadlock, proper error or success
//        - Risk: Common usage pattern, production blocker if deadlock exists
//
//   ⚪ [@AC-1,US-7] TC-16: verifyCallbackReentrant_bySendFromCallbackDiffLink_expectBidirectionalFlow
//        - Description: Callback sends on different LinkID (routing pattern)
//        - Category: Callback routing (lock ordering)
//        - Estimated effort: 4 hours
//        - Depends on: TC-15 GREEN, FF-6 GREEN
//        - Tools: RoutingCallbackContext, ping-pong termination detector
//        - Verification: Bidirectional flow works, no circular deadlock
//
// 🚪 GATE: HIGH priority must be GREEN before production release
//
//===================================================================================================
// 🥈 MEDIUM PRIORITY - Advanced Scenarios (Quality Assurance)
//===================================================================================================
//   ⚪ [@AC-1,US-3] TC-5: verifyPollingMultiThread_byServiceSide5Pollers_expectRaceConditionFree
//        - Description: Service-side multi-thread polling
//        - Category: Polling concurrency
//        - Estimated effort: 3 hours
//        - Depends on: TC-1 GREEN
//        - Tools: Atomic poll counters, race detector
//
//   ⚪ [@AC-1,US-3] TC-6: verifyPollingMultiThread_byClientSide5Pollers_expectRaceConditionFree
//        - Description: Client-side multi-thread polling
//        - Category: Polling concurrency
//        - Estimated effort: 3 hours
//        - Depends on: TC-4 GREEN
//
//   ⚪ [@AC-1,US-4] TC-7: verifyBufferPressure_bySenderFasterThanReceiver_expectFlowControl
//        - Description: Buffer pressure and flow control
//        - Category: Backpressure handling
//        - Estimated effort: 4 hours
//        - Depends on: TC-1 GREEN
//        - Tools: Buffer fill level monitor, rate limiter
//
//   ⚪ [@AC-1,US-6] TC-13: verifyMixedOps_byConcurrentSendRecvFlush_expectStateMachineConsistent
//        - Description: Mixed operations (send/recv/flush) concurrency
//        - Category: State machine integrity
//        - Estimated effort: 5 hours
//        - Depends on: TC-1, TC-4, TC-20 GREEN
//        - Tools: State transition logger, consistency checker
//
//   ⚪ [@AC-1,US-6] TC-14: verifyFlushThreadSafe_by5ConcurrentFlushers_expectSerialization
//        - Description: Concurrent flush operations
//        - Category: Flush thread-safety
//        - Estimated effort: 3 hours
//        - Depends on: TC-1 GREEN
//
//   ⚪ [@AC-1,US-8] TC-17: verifyCallbackNested_byChainA_B_C_expectStackSafe
//        - Description: Nested callback chains (depth limit)
//        - Category: Callback nesting (stack safety)
//        - Estimated effort: 3 hours
//        - Depends on: TC-16 GREEN
//        - Tools: Stack depth tracker, overflow detector
//
//   ⚪ [@AC-1,US-8] TC-18: verifyCallbackException_byConcurrentThrows_expectIsolation
//        - Description: Exception in callback under concurrency
//        - Category: Exception safety
//        - Estimated effort: 3 hours
//        - Depends on: TC-15 GREEN
//
//   ⚪ [@AC-1,US-9] TC-19: verifyThreadSafety_byIntensiveRandomOps_expectNoRaceConditions
//        - Description: Randomized stress test
//        - Category: Fuzzing/stress testing
//        - Estimated effort: 4 hours
//        - Depends on: All HIGH GREEN
//        - Tools: ThreadSanitizer, random operation generator
//
// 🚪 GATE: MEDIUM priority for production-grade quality
//
//===================================================================================================
// 🥉 LOW PRIORITY - Comprehensive Coverage (Nice-to-Have)
//===================================================================================================
//   ⚪ [@AC-2,US-1] TC-2: verifyP1MultiClientSend_by100Clients100Chunks_expectStress
//        - Description: Stress test with high client count
//        - Category: Scalability testing
//        - Estimated effort: 2 hours
//        - Depends on: TC-1 GREEN
//
//   ⚪ [@AC-2,US-1] TC-3: verifyP1MultiClientSend_byLargePayload_expectNoBufferOverflow
//        - Description: Large payload (100KB) concurrency
//        - Category: Buffer management
//        - Estimated effort: 2 hours
//        - Depends on: TC-1 GREEN
//
//   ⚪ [@AC-2,US-2] TC-8: verifyP2ServiceBroadcast_by50Clients_expectScalability
//        - Description: High client count broadcast
//        - Category: Scalability testing
//        - Estimated effort: 2 hours
//        - Depends on: TC-4 GREEN
//
//   ⚪ [@AC-2,US-4] TC-9: verifyBufferPressure_byReceiverBlocked_expectProperBackoff
//        - Description: Blocked receiver scenario
//        - Category: Deadlock prevention
//        - Estimated effort: 3 hours
//        - Depends on: TC-7 GREEN
//
//   ⚪ [@AC-2,US-4] TC-10: verifyBufferPressure_byOverflowCondition_expectGracefulError
//        - Description: Buffer overflow handling
//        - Category: Error handling
//        - Estimated effort: 2 hours
//        - Depends on: TC-7 GREEN
//
//   ⚪ [@AC-2,US-5] TC-12: verifyMultiLinkIsolation_by100Links_expectNoResourceContention
//        - Description: High link count isolation
//        - Category: Scalability testing
//        - Estimated effort: 3 hours
//        - Depends on: TC-11 GREEN
//
//   ⚪ [@AC-2,US-6] TC-20: verifyFlushThreadSafe_byFlushDuringHighRateSend_expectEventualSuccess
//        - Description: Flush under high send rate
//        - Category: Flush starvation prevention
//        - Estimated effort: 2 hours
//        - Depends on: TC-14 GREEN
//
//   ⚪ [@AC-2,US-6] TC-21: verifyFlushThreadSafe_byTimeoutIndependence_expectProperHandling
//        - Description: Flush and send timeout independence
//        - Category: Timeout interaction
//        - Estimated effort: 2 hours
//        - Depends on: TC-14 GREEN
//
//===================================================================================================
// 📊 PROGRESS SUMMARY
//===================================================================================================
// Fast-Fail Six:   0/6  GREEN (⚪⚪⚪⚪⚪⚪)
// HIGH Priority:   0/5  GREEN (⚪⚪⚪⚪⚪)
// MEDIUM Priority: 0/8  GREEN (⚪⚪⚪⚪⚪⚪⚪⚪)
// LOW Priority:    0/10 GREEN (⚪⚪⚪⚪⚪⚪⚪⚪⚪⚪)
// Total Tests:     0/29 GREEN
//
// Next Action: Implement Fast-Fail Six (smoke tests) → HIGH priority tests
//
//===================================================================================================
// 🛠️ IMPLEMENTATION ROADMAP (6-Week Plan)
//===================================================================================================
// Week 1: Fast-Fail Six + Test Infrastructure
//   - Days 1-2: Implement ConcurrencyCounters, DataChunk utilities
//   - Days 3-4: Implement DeadlockDetector, ThreadBarrier
//   - Day 5: Implement & validate Fast-Fail Six (FF-1 through FF-6)
//
// Week 2-3: HIGH Priority Tests (TC-1, TC-4, TC-11, TC-15, TC-16)
//   - Week 2: TC-1 (P1 multi-client) + TC-4 (P2 broadcast)
//   - Week 3: TC-11 (link isolation) + TC-15 (CRITICAL deadlock) + TC-16 (routing)
//   - Goal: All HIGH tests GREEN, ThreadSanitizer clean
//
// Week 4: MEDIUM Priority Tests (TC-5 through TC-19)
//   - Implement polling, buffer pressure, mixed ops tests
//   - Focus on state machine consistency and exception safety
//
// Week 5: LOW Priority Tests (TC-2, TC-3, TC-8 through TC-21)
//   - Stress tests, scalability validation
//   - Large payload, high client count scenarios
//
// Week 6: Integration & Documentation
//   - Run full suite with ThreadSanitizer + AddressSanitizer
//   - Fix any detected issues
//   - Document thread-safety guarantees in IOC_DatAPI.h
//   - Update user guide with concurrency best practices
//
//===================================================================================================
// ✅ COMPLETED TESTS (for reference, remove after stable)
//===================================================================================================
// (None yet - all tests in TODO state)
//
//======>END OF TODO/IMPLEMENTATION TRACKING SECTION===============================================
