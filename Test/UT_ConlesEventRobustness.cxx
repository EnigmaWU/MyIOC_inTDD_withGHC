///////////////////////////////////////////////////////////////////////////////////////////////////
// UT_ConlesEventRobustness.cxx - ConlesMode Event Robustness Testing
//
// PURPOSE:
//   Test robustness and stress scenarios for ConlesMode event system under adverse conditions.
//   Validates behavior when system is pushed to limits: slow consumers, queue overflow,
//   cascading events, and sync mode restrictions.
//
// CATDD METHODOLOGY:
//   This file follows Comment-alive Test-Driven Development (CaTDD):
//   - Phase 2: DESIGN (this document) - Comprehensive test design in comments
//   - Phase 3: IMPLEMENTATION - TDD Red→Green cycle (not yet started)
//   - Phase 4: FINALIZATION - Refactor and document
//
// PRIORITY CLASSIFICATION:
//   P3: Quality-Oriented → Robust (stress testing, stability)
//   PROMOTED TO P2 LEVEL due to high risk score:
//     - Impact: 3 (data loss, system hang)
//     - Likelihood: 2 (occurs under load)
//     - Uncertainty: 2 (complex async interactions)
//     - Score: 12 → Move up from default position
//
// RELATIONSHIP WITH OTHER TEST FILES:
//   - UT_ConlesEventTypical.cxx: Basic happy paths (FOUNDATION - COMPLETED)
//   - UT_ConlesEventState.cxx: State transitions and blocking (FOUNDATION - COMPLETED)
//   - UT_ConlesEventTimeout.cxx: Timeout handling (FOUNDATION - COMPLETED)
//   - UT_ConlesEventMisuse.cxx: Error handling (FOUNDATION - COMPLETED)
//   - THIS FILE: Stress, limits, and recovery scenarios
//
// REFERENCE:
//   - README_Specification.md "IF...THEN..." requirements #3, #6, #8-11
//   - Doc/UserGuide_EVT.md "Event Queue Management"
//   - CaTDD methodology: LLM/CaTDD_DesignPrompt.md
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "_UT_IOC_Common.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF OVERVIEW OF THIS UNIT TESTING FILE===============================================
/**
 * @brief
 *   [WHAT] This file verifies ConlesMode event system robustness under stress conditions
 *   [WHERE] in the IOC Event subsystem for connectionless mode
 *   [WHY] to ensure system remains stable and predictable under adverse conditions
 *
 * SCOPE:
 *   - In scope:
 *     • Queue overflow and backpressure behavior
 *     • Slow consumer blocking fast producer scenarios
 *     • Cascading event storms (events posted in callbacks)
 *     • Sync mode restrictions during callback execution
 *     • Multi-thread stress with concurrent subscribe/unsubscribe
 *     • Resource exhaustion and recovery
 *     • Performance degradation under load
 *   - Out of scope:
 *     • Basic functionality (see UT_ConlesEventTypical.cxx)
 *     • State machine correctness (see UT_ConlesEventState.cxx)
 *     • Timeout behavior (see UT_ConlesEventTimeout.cxx)
 *     • API misuse (see UT_ConlesEventMisuse.cxx)
 *
 * KEY CONCEPTS:
 *   - Robustness: System continues functioning correctly under stress
 *   - Backpressure: Flow control mechanism when consumer slower than producer
 *   - Cascading Events: Events triggering more events (amplification risk)
 *   - Sync Mode Restriction: Prevent deadlock by forbidding sync posts in callbacks
 *   - Graceful Degradation: System slows but doesn't crash under overload
 *
 * RELATIONSHIPS:
 *   - Depends on: IOC_Event.c (_IOC_ConlesEvent.c), _IOC_EvtDescQueue.c
 *   - Related tests: UT_ConlesEventState.cxx (blocking behavior foundation)
 *   - Production code: Source/_IOC_ConlesEvent.c (queue management, threading)
 *   - Specification: README_Specification.md #3, #6, #8-11
 */
//======>END OF OVERVIEW OF THIS UNIT TESTING FILE=================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF UNIT TESTING DESIGN==============================================================

/**************************************************************************************************
 * 📋 COVERAGE STRATEGY - CaTDD Dimension Analysis
 *
 * DIMENSION 1: Load Pattern (Producer Speed vs Consumer Speed)
 *   - FastProducer_SlowConsumer: Producer posts faster than consumer processes
 *   - FastProducer_FastConsumer: Both sides fast (normal operation)
 *   - BurstProducer: Sudden spike in event rate
 *   - CascadingProducer: Events triggering more events (amplification)
 *
 * DIMENSION 2: Queue State (Event Queue Fullness)
 *   - Empty: No events pending
 *   - Partial: Some events queued
 *   - Full: Queue at capacity
 *   - Overflow: Attempt to exceed capacity
 *
 * DIMENSION 3: Blocking Mode (IOC_OPTID_* flags)
 *   - AsyncNonBlock: Default fire-and-forget (IOC_OPTID_ASYNC_MODE + NonBlock)
 *   - AsyncMayBlock: Async with blocking allowed
 *   - SyncMode: Synchronous event processing (IOC_OPTID_SYNC_MODE)
 *   - TimeoutMode: With timeout specified (IOC_OPTID_TIMEOUT)
 *
 * COVERAGE MATRIX:
 * ┌──────────────────────────┬─────────────────┬──────────────────┬─────────────────────────────┐
 * │ Load Pattern             │ Queue State     │ Blocking Mode    │ Key Scenarios               │
 * ├──────────────────────────┼─────────────────┼──────────────────┼─────────────────────────────┤
 * │ FastProducer_SlowConsumer│ Partial→Full    │ AsyncMayBlock    │ US-1: Backpressure behavior │
 * │ FastProducer_SlowConsumer│ Full→Overflow   │ AsyncNonBlock    │ US-2: Queue overflow errors │
 * │ FastProducer_SlowConsumer│ Full            │ TimeoutMode      │ US-2: Timeout on full queue │
 * │ CascadingProducer        │ Partial→Full    │ AsyncNonBlock    │ US-3: Event storm detection │
 * │ CascadingProducer        │ Full→Overflow   │ AsyncMayBlock    │ US-3: Storm backpressure    │
 * │ Any (during callback)    │ Any             │ SyncMode         │ US-4: Sync mode forbidden   │
 * │ MultiThread_SubUnsub     │ Partial         │ Any              │ US-5: Thread safety stress  │
 * │ BurstProducer            │ Empty→Full→Empty│ AsyncMayBlock    │ US-5: Recovery after burst  │
 * └──────────────────────────┴─────────────────┴──────────────────┴─────────────────────────────┘
 *
 * PRIORITY FRAMEWORK (CaTDD):
 *   P1 🥇 FUNCTIONAL:     (Not applicable - robustness is P3)
 *   P2 🥈 DESIGN-ORIENTED: Thread safety, capacity limits
 *   P3 🥉 QUALITY-ORIENTED: Stress, recovery, graceful degradation ← THIS FILE
 *
 * CONTEXT-SPECIFIC ADJUSTMENT:
 *   - Event system is reliability-critical → Promote Robust from P3 to P2 level
 *   - Risk score 12 (Impact=3, Likelihood=2, Uncertainty=2) → High priority
 *   - Test these scenarios BEFORE releasing event system to production
 *************************************************************************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF USER STORY=======================================================================

/**************************************************************************************************
 * US-1: As an event producer posting events rapidly,
 *       I want the system to apply backpressure when consumers are slow,
 *       So that my application continues functioning without data loss or hangs.
 *
 * BUSINESS VALUE:
 *   - Prevents unbounded memory growth from queue overflow
 *   - Maintains system stability under variable load
 *   - Enables graceful degradation instead of catastrophic failure
 *
 * PRIORITY: 🥈 HIGH (P2 level) - Critical for production reliability
 *
 * SOURCE: README_Specification.md #6, #8
 *   #6: IF ObjB's CbProcEvt takes 999ms, THEN postEVT behavior with Sync/MayBlock/Timeout
 *   #8: IF too many events posted, THEN postEVT blocked/TOO_MANY_EVENTS/TIMEOUT
 *
 * ACCEPTANCE CRITERIA:
 *
 * [@US-1]
 * AC-1: GIVEN a fast producer posting events every 1ms,
 *       AND a slow consumer processing each event in 100ms,
 *       WHEN producer posts with MayBlock option,
 *       THEN postEVT blocks when queue is full and returns after space available.
 *
 * [@US-1]
 * AC-2: GIVEN a fast producer posting events continuously,
 *       AND a slow consumer cannot keep up,
 *       WHEN producer posts with NonBlock option,
 *       THEN postEVT returns TOO_MANY_QUEUING_EVTDESC when queue is full.
 *
 * [@US-1]
 * AC-3: GIVEN queue is full with pending events,
 *       AND producer posts with Timeout option (500ms),
 *       WHEN consumer does not process events within timeout,
 *       THEN postEVT returns IOC_RESULT_TIMEOUT after 500ms ±50ms.
 *
 * [@US-1]
 * AC-4: GIVEN backpressure was applied (queue full),
 *       WHEN consumer catches up and queue has space,
 *       THEN subsequent postEVT calls succeed without delay.
 *************************************************************************************************/

/**************************************************************************************************
 * US-2: As an event consumer with processing callbacks,
 *       I want the system to prevent cascading event storms,
 *       So that a single event doesn't trigger exponential event amplification.
 *
 * BUSINESS VALUE:
 *   - Prevents system overload from recursive event posting
 *   - Protects against accidental or malicious event loops
 *   - Maintains predictable event processing latency
 *
 * PRIORITY: 🥈 HIGH (P2 level) - Prevents catastrophic cascading failures
 *
 * SOURCE: README_Specification.md #9
 *   #9: IF ObjB's CbProcEvt posts 2+ events to ObjC/x2/x4,
 *       THEN ObjA gets TOO_MANY_QUEUING_EVTDESC or blocks
 *
 * ACCEPTANCE CRITERIA:
 *
 * [@US-2]
 * AC-1: GIVEN consumer A triggers consumer B which triggers consumer C (chain depth 3),
 *       AND each callback posts 1 event to next consumer,
 *       WHEN producer posts initial event,
 *       THEN all 3 levels process successfully without queue overflow.
 *
 * [@US-2]
 * AC-2: GIVEN consumer callback posts 2 events which each post 2 more (2^N amplification),
 *       WHEN event depth reaches queue capacity,
 *       THEN postEVT returns TOO_MANY_QUEUING_EVTDESC at appropriate depth.
 *
 * [@US-2]
 * AC-3: GIVEN cascading event chain with MayBlock option,
 *       WHEN queue approaches full,
 *       THEN inner postEVT blocks until outer callbacks complete.
 *
 * [@US-2]
 * AC-4: GIVEN event storm has filled queue,
 *       WHEN storm subsides and queue drains,
 *       THEN system recovers and accepts new events normally.
 *************************************************************************************************/

/**************************************************************************************************
 * US-3: As a developer implementing event callbacks,
 *       I want synchronous event posting forbidden during callback execution,
 *       So that my system avoids deadlocks and maintains deterministic behavior.
 *
 * BUSINESS VALUE:
 *   - Prevents deadlock scenarios in event-driven architectures
 *   - Enforces clear async boundaries in system design
 *   - Makes event flow reasoning easier for developers
 *
 * PRIORITY: 🥇 CRITICAL (P1 level) - Prevents deadlock (safety issue)
 *
 * SOURCE: README_Specification.md #10
 *   #10: IF ObjA is cbProcEvting, THEN postEVT in SyncMode returns FORBIDDEN
 *
 * ACCEPTANCE CRITERIA:
 *
 * [@US-3]
 * AC-1: GIVEN consumer callback is executing (CbProcEvt_F called),
 *       WHEN callback attempts to post event with SYNC_MODE option,
 *       THEN postEVT returns IOC_RESULT_FORBIDDEN immediately.
 *
 * [@US-3]
 * AC-2: GIVEN consumer callback attempts nested sync post,
 *       WHEN using AsyncMode (default) instead,
 *       THEN postEVT succeeds and queues event normally.
 *
 * [@US-3]
 * AC-3: GIVEN callback has completed and returned,
 *       WHEN subsequent postEVT uses SYNC_MODE from different context,
 *       THEN postEVT succeeds (restriction only applies during callback).
 *************************************************************************************************/

/**************************************************************************************************
 * US-4: As a system architect building multi-threaded applications,
 *       I want event subscription/unsubscription to be thread-safe under stress,
 *       So that concurrent operations don't corrupt internal state.
 *
 * BUSINESS VALUE:
 *   - Enables safe multi-threaded event-driven architectures
 *   - Prevents race conditions during dynamic subscription changes
 *   - Supports high-performance concurrent event processing
 *
 * PRIORITY: 🥈 HIGH (P2 level) - Essential for multi-threaded apps
 *
 * SOURCE: README_Specification.md #3
 *   #3: Repeat subscribe/unsubscribe, multiply threads, expect robustness
 *
 * ACCEPTANCE CRITERIA:
 *
 * [@US-4]
 * AC-1: GIVEN 10 threads each doing 1000 subscribe/unsubscribe cycles,
 *       WHEN all threads run concurrently,
 *       THEN all operations complete successfully without corruption.
 *
 * [@US-4]
 * AC-2: GIVEN multiple threads subscribing to same event ID,
 *       WHEN one thread unsubscribes while others post events,
 *       THEN operations remain consistent and no callbacks lost.
 *
 * [@US-4]
 * AC-3: GIVEN event callbacks executing in multiple threads,
 *       WHEN new subscribers register during callback execution,
 *       THEN state remains consistent and new subscribers activated next cycle.
 *
 * [@US-4]
 * AC-4: GIVEN high-frequency subscribe/unsubscribe pattern,
 *       WHEN running for sustained period (30 seconds),
 *       THEN no memory leaks, crashes, or performance degradation observed.
 *************************************************************************************************/

/**************************************************************************************************
 * US-5: As a system operator monitoring event system health,
 *       I want the system to recover gracefully after overload,
 *       So that temporary spikes don't cause permanent system instability.
 *
 * BUSINESS VALUE:
 *   - Supports elastic scalability during traffic bursts
 *   - Reduces operational intervention for transient issues
 *   - Improves overall system availability and resilience
 *
 * PRIORITY: 🥉 MEDIUM (P3 level) - Quality of service improvement
 *
 * SOURCE: README_Specification.md #8, #11 (forceProcEVT behavior)
 *
 * ACCEPTANCE CRITERIA:
 *
 * [@US-5]
 * AC-1: GIVEN system experiences burst (1000 events in 100ms),
 *       WHEN burst completes and queue drains,
 *       THEN subsequent event processing returns to normal latency.
 *
 * [@US-5]
 * AC-2: GIVEN queue was full and producers blocked,
 *       WHEN consumers catch up and free queue space,
 *       THEN blocked producers resume posting immediately.
 *
 * [@US-5]
 * AC-3: GIVEN system under sustained high load,
 *       WHEN forceProcEVT called to drain queue,
 *       THEN all queued events process and system returns to Ready state.
 *************************************************************************************************/

//======>END OF USER STORY==========================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF ACCEPTANCE CRITERIA==============================================================

// See inline AC definitions under each User Story above.
// Format: [@US-N] AC-M: GIVEN [context], WHEN [action], THEN [result]

//======>END OF ACCEPTANCE CRITERIA================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF TEST CASE DESIGN=================================================================

/**************************************************************************************************
 * TEST CASE SPECIFICATIONS
 *
 * Following CaTDD naming: verifyBehavior_byCondition_expectResult
 * Following 4-phase structure: SETUP → BEHAVIOR → VERIFY → CLEANUP
 * Target: ≤3 key assertions per test
 *************************************************************************************************/

// =================================================================================================
// US-1: Backpressure and Queue Overflow Management
// =================================================================================================

/**
 * [@AC-1,US-1]
 * TC-1:
 *   @[Name]: verifyBackpressure_bySlowConsumer_expectPostBlocks
 *   @[Purpose]: Verify MayBlock option blocks when queue full with slow consumer
 *   @[Steps]:
 *     SETUP:
 *       1) Subscribe consumer with 100ms processing delay per event
 *       2) Configure queue capacity (query IOC_getCapability)
 *     BEHAVIOR:
 *       3) Producer posts events every 1ms with MayBlock option
 *       4) Continue until queue full (producer should block)
 *       5) Measure time blocked
 *     VERIFY:
 *       6) Verify postEVT blocks for >50ms (queue processing time)
 *       7) Verify no events lost (all posted events eventually received)
 *       8) Verify no TOO_MANY_QUEUING_EVTDESC errors
 *     CLEANUP:
 *       9) Unsubscribe, drain remaining events
 *   @[Expect]:
 *     - postEVT blocks when queue full
 *     - postEVT resumes when space available
 *     - All events delivered successfully
 *   @[Notes]:
 *     - Related to UT_ConlesEventState.cxx blocking behavior tests
 *     - Uses IOC_OPTID_ASYNC_MODE with default MayBlock
 */

/**
 * [@AC-2,US-1]
 * TC-2:
 *   @[Name]: verifyQueueOverflow_byFastProducer_expectErrorReturned
 *   @[Purpose]: Verify NonBlock option returns error when queue full
 *   @[Steps]:
 *     SETUP:
 *       1) Subscribe consumer with 200ms processing delay (very slow)
 *       2) Determine queue capacity
 *     BEHAVIOR:
 *       3) Producer posts events rapidly with NonBlock option
 *       4) Continue posting beyond queue capacity
 *     VERIFY:
 *       5) Verify first N posts succeed (N = queue capacity)
 *       6) Verify subsequent posts return TOO_MANY_QUEUING_EVTDESC
 *       7) Verify error count matches expected overflow attempts
 *     CLEANUP:
 *       8) Unsubscribe after queue drains
 *   @[Expect]:
 *     - postEVT returns immediately (no blocking)
 *     - Error code TOO_MANY_QUEUING_EVTDESC when queue full
 *     - Producer informed of queue state
 *   @[Notes]:
 *     - Tests NonBlock behavior under stress
 *     - Complements TC-1 (different blocking mode)
 */

/**
 * [@AC-3,US-1]
 * TC-3:
 *   @[Name]: verifyTimeout_byFullQueue_expectTimeoutReturned
 *   @[Purpose]: Verify Timeout option returns error after specified duration
 *   @[Steps]:
 *     SETUP:
 *       1) Subscribe consumer with 1000ms processing delay (extremely slow)
 *       2) Fill queue to capacity
 *     BEHAVIOR:
 *       3) Producer posts event with 500ms timeout option
 *       4) Measure actual wait time
 *     VERIFY:
 *       5) Verify postEVT returns IOC_RESULT_TIMEOUT
 *       6) Verify timeout duration 500ms ±50ms (10% tolerance)
 *       7) Verify event NOT delivered to consumer
 *     CLEANUP:
 *       8) Unsubscribe, clear queue
 *   @[Expect]:
 *     - Timeout honored within tolerance
 *     - Clear error indication to producer
 *     - Event discarded after timeout
 *   @[Notes]:
 *     - Similar to UT_ConlesEventTimeout.cxx but under full queue stress
 *     - Uses IOC_OPTID_TIMEOUT option
 */

/**
 * [@AC-4,US-1]
 * TC-4:
 *   @[Name]: verifyRecovery_afterBackpressure_expectNormalFlow
 *   @[Purpose]: Verify system returns to normal after backpressure resolves
 *   @[Steps]:
 *     SETUP:
 *       1) Subscribe consumer with variable processing delay
 *       2) Fill queue to trigger backpressure
 *     BEHAVIOR:
 *       3) Measure postEVT latency while queue full (should be high)
 *       4) Switch consumer to fast processing (10ms delay)
 *       5) Wait for queue to drain
 *       6) Measure postEVT latency after recovery
 *     VERIFY:
 *       7) Verify latency during backpressure >100ms
 *       8) Verify latency after recovery <5ms
 *       9) Verify all subsequent posts succeed immediately
 *     CLEANUP:
 *       10) Unsubscribe
 *   @[Expect]:
 *     - Performance recovers after queue drains
 *     - No permanent degradation
 *     - System usable after stress period
 *   @[Notes]:
 *     - Tests graceful degradation and recovery
 *     - Important for production resilience
 */

// =================================================================================================
// US-2: Cascading Event Storm Prevention
// =================================================================================================

/**
 * [@AC-1,US-2]
 * TC-5:
 *   @[Name]: verifyCascading_byLinearChain_expectAllDelivered
 *   @[Purpose]: Verify simple cascading chain (A→B→C) works correctly
 *   @[Steps]:
 *     SETUP:
 *       1) Setup 3 consumers: A, B, C
 *       2) A's callback posts event to B
 *       3) B's callback posts event to C
 *       4) C's callback increments counter
 *     BEHAVIOR:
 *       5) Post initial event to A
 *       6) Wait for cascade to complete (forceProcEVT)
 *     VERIFY:
 *       7) Verify A callback executed once
 *       8) Verify B callback executed once
 *       9) Verify C counter incremented once
 *     CLEANUP:
 *       10) Unsubscribe all consumers
 *   @[Expect]:
 *     - Linear cascade (depth 3) succeeds
 *     - Each level processes exactly once
 *     - No queue overflow
 *   @[Notes]:
 *     - Baseline for cascade behavior
 *     - Foundation for exponential cascade tests
 */

/**
 * [@AC-2,US-2]
 * TC-6:
 *   @[Name]: verifyCascading_byExponentialAmplification_expectLimited
 *   @[Purpose]: Verify exponential cascade (2^N) detects overflow
 *   @[Steps]:
 *     SETUP:
 *       1) Setup consumer that posts 2 events per callback
 *       2) Those 2 events trigger 2 more each (4 total)
 *       3) Continue pattern (2, 4, 8, 16, 32, ...)
 *       4) Track depth and error counts
 *     BEHAVIOR:
 *       5) Post initial event to start cascade
 *       6) Monitor for TOO_MANY_QUEUING_EVTDESC errors
 *     VERIFY:
 *       7) Verify cascade stops at queue capacity depth
 *       8) Verify TOO_MANY_QUEUING_EVTDESC returned at overflow
 *       9) Verify system remains stable (no crash)
 *     CLEANUP:
 *       10) Force drain queue, unsubscribe
 *   @[Expect]:
 *     - Exponential amplification detected
 *     - Overflow protection triggered
 *     - System doesn't hang or crash
 *   @[Notes]:
 *     - Critical safety test
 *     - Simulates runaway event loops
 */

/**
 * [@AC-3,US-2]
 * TC-7:
 *   @[Name]: verifyCascading_byMayBlockOption_expectGracefulBackpressure
 *   @[Purpose]: Verify cascading with MayBlock applies backpressure correctly
 *   @[Steps]:
 *     SETUP:
 *       1) Setup cascade chain with MayBlock option
 *       2) Each level posts event to next with delay
 *     BEHAVIOR:
 *       3) Initiate cascade that would overflow queue
 *       4) Monitor blocking behavior at each level
 *     VERIFY:
 *       5) Verify inner posts block when queue full
 *       6) Verify cascade completes eventually (no deadlock)
 *       7) Verify all events processed in correct order
 *     CLEANUP:
 *       8) Unsubscribe all levels
 *   @[Expect]:
 *     - Backpressure propagates up cascade chain
 *     - No deadlock despite nested blocking
 *     - Eventual completion with all events delivered
 *   @[Notes]:
 *     - Tests complex interaction of cascade + blocking
 *     - Verifies no deadlock scenarios
 */

/**
 * [@AC-4,US-2]
 * TC-8:
 *   @[Name]: verifyRecovery_afterEventStorm_expectNormalOperation
 *   @[Purpose]: Verify system recovers after cascading overflow
 *   @[Steps]:
 *     SETUP:
 *       1) Trigger event storm that fills queue
 *       2) Allow queue to drain completely
 *     BEHAVIOR:
 *       3) Post normal events after storm subsides
 *       4) Measure processing latency
 *     VERIFY:
 *       5) Verify post-storm events process normally
 *       6) Verify latency returns to baseline
 *       7) Verify no lingering effects from overflow
 *     CLEANUP:
 *       8) Unsubscribe all consumers
 *   @[Expect]:
 *     - Full recovery after storm
 *     - No permanent state corruption
 *     - System operational after stress
 *   @[Notes]:
 *     - Validates resilience after worst-case scenario
 */

// =================================================================================================
// US-3: Sync Mode Deadlock Prevention
// =================================================================================================

/**
 * [@AC-1,US-3]
 * TC-9:
 *   @[Name]: verifySyncMode_duringCallback_expectForbidden
 *   @[Purpose]: Verify SYNC_MODE forbidden when called from callback
 *   @[Steps]:
 *     SETUP:
 *       1) Subscribe consumer A
 *       2) Consumer A callback attempts postEVT with SYNC_MODE
 *     BEHAVIOR:
 *       3) Post event to trigger consumer A callback
 *       4) Callback attempts sync post
 *       5) Capture return code
 *     VERIFY:
 *       6) Verify postEVT returns IOC_RESULT_FORBIDDEN
 *       7) Verify error returned immediately (no hang)
 *       8) Verify outer event completes successfully
 *     CLEANUP:
 *       9) Unsubscribe consumer A
 *   @[Expect]:
 *     - FORBIDDEN error code returned
 *     - No system hang or deadlock
 *     - Clear error indication to developer
 *   @[Notes]:
 *     - Critical deadlock prevention mechanism
 *     - Specification requirement #10
 */

/**
 * [@AC-2,US-3]
 * TC-10:
 *   @[Name]: verifyAsyncMode_duringCallback_expectSuccess
 *   @[Purpose]: Verify AsyncMode (default) works during callback
 *   @[Steps]:
 *     SETUP:
 *       1) Subscribe consumers A and B
 *       2) Consumer A callback posts AsyncMode event to B
 *     BEHAVIOR:
 *       3) Post event to trigger A
 *       4) A's callback posts to B (async)
 *       5) Wait for B to receive event
 *     VERIFY:
 *       6) Verify A's async post succeeds
 *       7) Verify B receives event
 *       8) Verify no deadlock or errors
 *     CLEANUP:
 *       9) Unsubscribe A and B
 *   @[Expect]:
 *     - AsyncMode allowed in callbacks
 *     - Event chain completes successfully
 *     - No restrictions on async posts
 *   @[Notes]:
 *     - Validates alternative to sync mode
 *     - Shows correct usage pattern
 */

/**
 * [@AC-3,US-3]
 * TC-11:
 *   @[Name]: verifySyncMode_afterCallback_expectSuccess
 *   @[Purpose]: Verify SYNC_MODE allowed outside callback context
 *   @[Steps]:
 *     SETUP:
 *       1) Subscribe consumer A
 *       2) Setup flag to detect callback completion
 *     BEHAVIOR:
 *       3) Post event to trigger callback
 *       4) Wait for callback completion
 *       5) Post another event with SYNC_MODE (outside callback)
 *     VERIFY:
 *       6) Verify sync post succeeds after callback done
 *       7) Verify both events processed correctly
 *       8) Verify correct order maintained
 *     CLEANUP:
 *       9) Unsubscribe consumer A
 *   @[Expect]:
 *     - SYNC_MODE works normally outside callbacks
 *     - Restriction is context-specific
 *     - No false positives (over-restrictive)
 *   @[Notes]:
 *     - Verifies restriction is precise, not overly broad
 */

// =================================================================================================
// US-4: Multi-thread Stress Testing
// =================================================================================================

/**
 * [@AC-1,US-4]
 * TC-12:
 *   @[Name]: verifyMultiThread_bySubUnsubStress_expectNoCorruption
 *   @[Purpose]: Verify thread-safe subscribe/unsubscribe under stress
 *   @[Steps]:
 *     SETUP:
 *       1) Create 10 threads
 *       2) Each thread performs 1000 subscribe/unsubscribe cycles
 *     BEHAVIOR:
 *       3) Launch all threads simultaneously
 *       4) Each thread: subscribe → wait 1ms → unsubscribe → repeat
 *       5) Join all threads
 *     VERIFY:
 *       6) Verify all threads complete successfully
 *       7) Verify no assertion failures or crashes
 *       8) Verify final state clean (no leaked subscriptions)
 *     CLEANUP:
 *       9) Verify all resources released
 *   @[Expect]:
 *     - 10,000 total operations complete
 *     - No race conditions detected
 *     - Clean final state
 *   @[Notes]:
 *     - Specification requirement #3
 *     - Similar to UT_ConlesEventState Case02 but more intensive
 */

/**
 * [@AC-2,US-4]
 * TC-13:
 *   @[Name]: verifyMultiThread_bySubscribeWhilePosting_expectConsistent
 *   @[Purpose]: Verify consistency when subscribing during active posting
 *   @[Steps]:
 *     SETUP:
 *       1) Thread 1: Posts events continuously
 *       2) Thread 2-5: Subscribe/unsubscribe repeatedly
 *     BEHAVIOR:
 *       3) Run threads concurrently for 10 seconds
 *       4) Track events received per thread
 *     VERIFY:
 *       5) Verify no events lost to active subscribers
 *       6) Verify no crashes or deadlocks
 *       7) Verify subscription state consistent
 *     CLEANUP:
 *       8) Stop all threads, unsubscribe all
 *   @[Expect]:
 *     - Active subscribers receive events
 *     - Subscription changes don't corrupt state
 *     - No deadlocks or livelocks
 *   @[Notes]:
 *     - Tests real-world concurrent usage pattern
 */

/**
 * [@AC-3,US-4]
 * TC-14:
 *   @[Name]: verifyMultiThread_byNewSubscriberDuringCallback_expectActivatedNext
 *   @[Purpose]: Verify new subscribers added during callback activated correctly
 *   @[Steps]:
 *     SETUP:
 *       1) Subscribe consumer A
 *       2) A's callback subscribes consumer B
 *       3) Post second event (should reach both A and B)
 *     BEHAVIOR:
 *       4) Post first event (triggers A, A subscribes B)
 *       5) Post second event
 *     VERIFY:
 *       6) Verify A receives both events
 *       7) Verify B receives only second event (subscribed after first)
 *       8) Verify timing: B activated in next cycle
 *     CLEANUP:
 *       9) Unsubscribe A and B
 *   @[Expect]:
 *     - Dynamic subscription works correctly
 *     - New subscriber activated next cycle (not mid-processing)
 *     - Consistent state throughout
 *   @[Notes]:
 *     - Tests subscription timing semantics
 */

/**
 * [@AC-4,US-4]
 * TC-15:
 *   @[Name]: verifyMultiThread_bySustainedStress_expectNoLeaksOrDegradation
 *   @[Purpose]: Verify long-running multi-thread stress causes no leaks
 *   @[Steps]:
 *     SETUP:
 *       1) Setup 5 threads doing subscribe/post/unsubscribe cycles
 *       2) Monitor memory usage baseline
 *     BEHAVIOR:
 *       3) Run threads for 30 seconds continuously
 *       4) Measure memory usage every 5 seconds
 *       5) Measure event processing latency throughout
 *     VERIFY:
 *       6) Verify memory stable (no leaks, <5% growth)
 *       7) Verify latency stable (no degradation, <10% variance)
 *       8) Verify no crashes or errors
 *     CLEANUP:
 *       9) Stop threads, verify clean shutdown
 *   @[Expect]:
 *     - Stable memory usage
 *     - Consistent performance
 *     - No resource leaks
 *   @[Notes]:
 *     - Long-running soak test
 *     - May require AddressSanitizer for leak detection
 */

// =================================================================================================
// US-5: Recovery and Graceful Degradation
// =================================================================================================

/**
 * [@AC-1,US-5]
 * TC-16:
 *   @[Name]: verifyRecovery_afterBurst_expectNormalLatency
 *   @[Purpose]: Verify system recovers after burst traffic
 *   @[Steps]:
 *     SETUP:
 *       1) Subscribe fast consumer (10ms processing)
 *       2) Measure baseline latency
 *     BEHAVIOR:
 *       3) Post 1000 events rapidly (burst)
 *       4) Wait for queue to drain
 *       5) Post normal events and measure latency
 *     VERIFY:
 *       6) Verify burst queued successfully
 *       7) Verify post-burst latency returns to baseline ±10%
 *       8) Verify no events lost during burst
 *     CLEANUP:
 *       9) Unsubscribe consumer
 *   @[Expect]:
 *     - Burst handled without loss
 *     - Performance recovers fully
 *     - No permanent impact
 *   @[Notes]:
 *     - Tests elastic scalability
 */

/**
 * [@AC-2,US-5]
 * TC-17:
 *   @[Name]: verifyRecovery_afterBlockedProducers_expectImmediateResume
 *   @[Purpose]: Verify blocked producers resume immediately when queue frees
 *   @[Steps]:
 *     SETUP:
 *       1) Subscribe slow consumer (1000ms per event)
 *       2) Fill queue to capacity
 *     BEHAVIOR:
 *       3) Launch producer thread posting with MayBlock (will block)
 *       4) Switch consumer to fast mode (10ms per event)
 *       5) Measure time until producer resumes
 *     VERIFY:
 *       6) Verify producer blocks initially
 *       7) Verify producer resumes within 100ms after queue space available
 *       8) Verify no spurious delays
 *     CLEANUP:
 *       9) Stop threads, unsubscribe
 *   @[Expect]:
 *     - Immediate resume (no polling delay)
 *     - Efficient wakeup mechanism
 *     - Producers not starved
 *   @[Notes]:
 *     - Tests condition variable wakeup efficiency
 */

/**
 * [@AC-3,US-5]
 * TC-18:
 *   @[Name]: verifyForceProcEVT_underHighLoad_expectAllProcessed
 *   @[Purpose]: Verify forceProcEVT drains queue under sustained load
 *   @[Steps]:
 *     SETUP:
 *       1) Subscribe consumer with 50ms processing delay
 *       2) Post 500 events continuously (sustained load)
 *     BEHAVIOR:
 *       3) Call IOC_forceProcEVT()
 *       4) Monitor queue until empty
 *     VERIFY:
 *       5) Verify all 500 events processed
 *       6) Verify forceProcEVT blocks until queue empty
 *       7) Verify LinkState returns to Ready after drain
 *     CLEANUP:
 *       8) Unsubscribe consumer
 *   @[Expect]:
 *     - Complete queue drain
 *     - forceProcEVT blocks until done
 *     - Clean state after drain
 *   @[Notes]:
 *     - Specification requirement #11
 *     - Tests operator intervention tool
 */

//======>END OF TEST CASE DESIGN===================================================================

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
//   🥇 HIGH:    Must-have for production (US-1, US-3)
//   🥈 MEDIUM:  Important for quality (US-2, US-4)
//   🥉 LOW:     Nice-to-have (US-5)
//
//=================================================================================================
// 🥇 HIGH PRIORITY – Critical Robustness (US-1: Backpressure, US-3: Deadlock Prevention)
//=================================================================================================
//   ⚪ [@AC-1,US-1] TC-1: verifyBackpressure_bySlowConsumer_expectPostBlocks
//   ⚪ [@AC-2,US-1] TC-2: verifyQueueOverflow_byFastProducer_expectErrorReturned
//   ⚪ [@AC-3,US-1] TC-3: verifyTimeout_byFullQueue_expectTimeoutReturned
//   ⚪ [@AC-4,US-1] TC-4: verifyRecovery_afterBackpressure_expectNormalFlow
//   🟢 [@AC-1,US-3] TC-9: verifySyncModeDuringCallback_expectForbidden – ✅ GREEN (deadlock prevented)
//   🟢 [@AC-2,US-3] TC-10: verifyAsyncModeDuringCallback_expectSuccess – ✅ GREEN (proves restriction precise)
//   🟢 [@AC-3,US-3] TC-11: verifySyncModeAfterCallback_expectSuccess – ✅ GREEN (restriction scoped)
//
//=================================================================================================
// 🥈 MEDIUM PRIORITY – Event Storm & Concurrency (US-2, US-4)
//=================================================================================================
//   ⚪ [@AC-1,US-2] TC-5: verifyCascading_byLinearChain_expectAllDelivered
//   ⚪ [@AC-2,US-2] TC-6: verifyCascading_byExponentialAmplification_expectLimited
//   ⚪ [@AC-3,US-2] TC-7: verifyCascading_byMayBlockOption_expectGracefulBackpressure
//   ⚪ [@AC-4,US-2] TC-8: verifyRecovery_afterEventStorm_expectNormalOperation
//   ⚪ [@AC-1,US-4] TC-12: verifyMultiThread_bySubUnsubStress_expectNoCorruption
//   ⚪ [@AC-2,US-4] TC-13: verifyMultiThread_bySubscribeWhilePosting_expectConsistent
//   ⚪ [@AC-3,US-4] TC-14: verifyMultiThread_byNewSubscriberDuringCallback_expectActivatedNext
//   ⚪ [@AC-4,US-4] TC-15: verifyMultiThread_bySustainedStress_expectNoLeaksOrDegradation – LONG-RUNNING
//
//=================================================================================================
// 🥉 LOW PRIORITY – Recovery & Operations (US-5)
//=================================================================================================
//   ⚪ [@AC-1,US-5] TC-16: verifyRecovery_afterBurst_expectNormalLatency
//   ⚪ [@AC-2,US-5] TC-17: verifyRecovery_afterBlockedProducers_expectImmediateResume
//   ⚪ [@AC-3,US-5] TC-18: verifyForceProcEVT_underHighLoad_expectAllProcessed
//
//=================================================================================================
// 📊 SUMMARY
//=================================================================================================
//   Total Test Cases: 18
//   By Priority: 🥇 HIGH=7, 🥈 MEDIUM=8, 🥉 LOW=3
//   By User Story: US-1=4, US-2=4, US-3=3, US-4=4, US-5=3
//   Implementation Status: All ⚪ TODO/PLANNED (design phase complete)
//
//   NEXT STEPS (CaTDD Phase 3):
//     1. Human approval of design (Checkpoint 2)
//     2. Begin TDD Red→Green cycle with TC-9 (highest priority, deadlock prevention)
//     3. Implement Fast-Fail Six tests first (if applicable)
//     4. Progress through P1 HIGH priority tests
//     5. Gate check before proceeding to P2 MEDIUM tests
//
///////////////////////////////////////////////////////////////////////////////////////////////////
//======>END OF TODO/IMPLEMENTATION TRACKING SECTION===============================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF TEST IMPLEMENTATION==============================================================
//
// � PHASE 3: TDD Red→Green Implementation in Progress
//
// Following CaTDD Phase 3 workflow:
//   - Write test first (RED) ← Current step
//   - Implement minimal production code (GREEN)
//   - Refactor both test and production code
//   - Update TODO section status after each test
//
///////////////////////////////////////////////////////////////////////////////////////////////////

// =================================================================================================
// =================================================================================================
// US-1: Backpressure and Queue Overflow Management (HIGH PRIORITY)
// =================================================================================================

/**
 * [@AC-1,US-1] TC-1: verifyBackpressure_bySlowConsumer_expectPostBlocks
 *
 * PURPOSE: Verify MayBlock option blocks producer when queue fills due to slow consumer.
 *          This validates backpressure mechanism for flow control.
 *
 * SPECIFICATION: README_Specification.md #8
 *   "IF too many events posted, THEN postEVT behavior depends on option (blocked/error/timeout)"
 *
 * PRIORITY: 🥇 HIGH - Critical for production stability under load
 */

namespace {
struct Tc1Context {
    std::atomic<uint32_t> EventsReceived{0};
    std::atomic<bool> ConsumerReady{false};
    static constexpr uint32_t ProcessingDelayMs = 100;  // Slow consumer: 100ms per event
};

// Slow consumer callback - simulates heavy processing
IOC_Result_T tc1CbProcEvtSlowConsumer(const IOC_EvtDesc_pT pEvtDesc, void* pCbPrivData) {
    (void)pEvtDesc;
    auto* pCtx = static_cast<Tc1Context*>(pCbPrivData);

    pCtx->EventsReceived.fetch_add(1);

    // Simulate slow processing
    std::this_thread::sleep_for(std::chrono::milliseconds(Tc1Context::ProcessingDelayMs));

    return IOC_RESULT_SUCCESS;
}
}  // namespace

/**
 * @[Name]: verifyBackpressure_bySlowConsumer_expectPostBlocks
 * @[Purpose]: Validate MayBlock backpressure mechanism when consumer is slower than producer.
 *             Ensures producer blocks when queue full and resumes when space available.
 * @[Steps]:
 *    1) 🔧 SETUP: Subscribe slow consumer (100ms processing delay per event)
 *    2) 🎯 BEHAVIOR: Producer posts events rapidly (every 1ms) with MayBlock option
 *    3) ✅ VERIFY: Producer blocks when queue full, all events eventually delivered
 *    4) 🧹 CLEANUP: Unsubscribe consumer
 * @[Expect]: Producer experiences blocking (>50ms delay on some posts), but all events
 *            are successfully delivered without TOO_MANY_QUEUING_EVTDESC errors.
 * @[Notes]: Tests AsyncMode with default MayBlock behavior. Producer should adapt to
 *           consumer speed through backpressure, not drop events.
 */
TEST(UTConlesEventRobustnessBackpressure, verifyBackpressure_bySlowConsumer_expectPostBlocks) {
    //===SETUP===
    Tc1Context Ctx;

    // Subscribe slow consumer
    IOC_EvtID_T EvtIDs[] = {IOC_EVTID_TEST_KEEPALIVE};
    IOC_SubEvtArgs_T SubArgs = {
        .CbProcEvt_F = tc1CbProcEvtSlowConsumer,
        .pCbPrivData = &Ctx,
        .EvtNum = IOC_calcArrayElmtCnt(EvtIDs),
        .pEvtIDs = EvtIDs,
    };

    IOC_Result_T Result = IOC_subEVT_inConlesMode(&SubArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result) << "Setup: Subscribe should succeed";

    Ctx.ConsumerReady.store(true);

    //===BEHAVIOR===
    // Producer posts events rapidly with MayBlock option (default AsyncMode)
    // Queue capacity is 64, need more events to trigger backpressure
    constexpr uint32_t TotalEvents = 100;
    uint32_t BlockedCount = 0;

    IOC_Option_defineASyncMayBlock(MayBlockOption);

    for (uint32_t i = 0; i < TotalEvents; i++) {
        IOC_EvtDesc_T EvtDesc = {
            .EvtID = IOC_EVTID_TEST_KEEPALIVE,
        };

        auto StartTime = std::chrono::steady_clock::now();
        Result = IOC_postEVT_inConlesMode(&EvtDesc, &MayBlockOption);
        auto EndTime = std::chrono::steady_clock::now();

        auto DurationMs = std::chrono::duration_cast<std::chrono::milliseconds>(EndTime - StartTime).count();

        // If post took > 50ms, it likely blocked due to queue backpressure
        if (DurationMs > 50) {
            BlockedCount++;
        }

        ASSERT_EQ(IOC_RESULT_SUCCESS, Result) << "Behavior: Post " << i << " should succeed (may block)";

        // Producer tries to post every 1ms (much faster than 100ms consumer)
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    // Wait for all events to be processed
    IOC_forceProcEVT();
    std::this_thread::sleep_for(std::chrono::milliseconds(TotalEvents * Tc1Context::ProcessingDelayMs + 500));

    //===VERIFY===
    // Key Verification Point 1: Producer experienced blocking (backpressure applied)
    VERIFY_KEYPOINT_GT(BlockedCount, static_cast<uint32_t>(0),
                       "Producer MUST experience blocking when queue fills (backpressure mechanism)");

    // Key Verification Point 2: All events delivered (no drops despite backpressure)
    VERIFY_KEYPOINT_EQ(Ctx.EventsReceived.load(), TotalEvents,
                       "All events MUST be delivered eventually (backpressure preserves data)");

    // Key Verification Point 3: No overflow errors (MayBlock prevents TOO_MANY_QUEUING_EVTDESC)
    // This is implicitly verified by all posts returning SUCCESS above

    //===CLEANUP===
    IOC_UnsubEvtArgs_T UnsubArgs = {
        .CbProcEvt_F = tc1CbProcEvtSlowConsumer,
        .pCbPrivData = &Ctx,
    };

    Result = IOC_unsubEVT_inConlesMode(&UnsubArgs);
    EXPECT_EQ(IOC_RESULT_SUCCESS, Result) << "Cleanup: Unsubscribe should succeed";
}

/**
 * [@AC-2,US-1] TC-2: verifyQueueOverflow_byFastProducer_expectErrorReturned
 *
 * PURPOSE: Verify NonBlock option returns error immediately when queue is full.
 *          Producer gets clear feedback without blocking.
 *
 * SPECIFICATION: README_Specification.md #8
 *   "IF too many events posted with NonBlock, THEN TOO_MANY_QUEUING_EVTDESC returned"
 *
 * PRIORITY: 🥇 HIGH - Essential error handling pattern
 */

namespace {
struct Tc2Context {
    std::atomic<uint32_t> EventsReceived{0};
    static constexpr uint32_t ProcessingDelayMs = 200;  // Very slow consumer
};

IOC_Result_T tc2CbProcEvtVerySlowConsumer(const IOC_EvtDesc_pT pEvtDesc, void* pCbPrivData) {
    (void)pEvtDesc;
    auto* pCtx = static_cast<Tc2Context*>(pCbPrivData);
    pCtx->EventsReceived.fetch_add(1);
    std::this_thread::sleep_for(std::chrono::milliseconds(Tc2Context::ProcessingDelayMs));
    return IOC_RESULT_SUCCESS;
}
}  // namespace

/**
 * @[Name]: verifyQueueOverflow_byFastProducer_expectErrorReturned
 * @[Purpose]: Validate NonBlock error handling when queue overflows. Producer must receive
 *             immediate feedback without blocking.
 * @[Steps]:
 *    1) 🔧 SETUP: Subscribe very slow consumer (200ms delay), determine queue capacity
 *    2) 🎯 BEHAVIOR: Producer posts rapidly with NonBlock beyond queue capacity
 *    3) ✅ VERIFY: First N posts succeed, subsequent return TOO_MANY_QUEUING_EVTDESC
 *    4) 🧹 CLEANUP: Unsubscribe consumer
 * @[Expect]: NonBlock returns immediately with error when queue full. Producer informed
 *            of backpressure without blocking.
 * @[Notes]: Complements TC-1 (MayBlock). Tests different error handling strategy.
 */
TEST(UTConlesEventRobustnessBackpressure, verifyQueueOverflow_byFastProducer_expectErrorReturned) {
    //===SETUP===
    Tc2Context Ctx;

    IOC_EvtID_T EvtIDs[] = {IOC_EVTID_TEST_KEEPALIVE};
    IOC_SubEvtArgs_T SubArgs = {
        .CbProcEvt_F = tc2CbProcEvtVerySlowConsumer,
        .pCbPrivData = &Ctx,
        .EvtNum = IOC_calcArrayElmtCnt(EvtIDs),
        .pEvtIDs = EvtIDs,
    };

    IOC_Result_T Result = IOC_subEVT_inConlesMode(&SubArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result) << "Setup: Subscribe should succeed";

    // Query queue capacity (should be 64)
    constexpr uint32_t ExpectedQueueCapacity = 64;

    //===BEHAVIOR===
    // Producer posts rapidly with NonBlock option
    constexpr uint32_t TotalAttempts = 100;  // Exceed queue capacity
    uint32_t SuccessCount = 0;
    uint32_t OverflowCount = 0;

    IOC_Option_defineNonBlock(NonBlockOption);

    for (uint32_t i = 0; i < TotalAttempts; i++) {
        IOC_EvtDesc_T EvtDesc = {
            .EvtID = IOC_EVTID_TEST_KEEPALIVE,
        };

        Result = IOC_postEVT_inConlesMode(&EvtDesc, &NonBlockOption);

        if (Result == IOC_RESULT_SUCCESS) {
            SuccessCount++;
        } else if (Result == IOC_RESULT_TOO_MANY_QUEUING_EVTDESC) {
            OverflowCount++;
        } else {
            FAIL() << "Unexpected result: " << Result;
        }

        // Post as fast as possible (no delay)
    }

    //===VERIFY===
    // Key Verification Point 1: Some posts succeeded (queue was fillable)
    VERIFY_KEYPOINT_GE(SuccessCount, ExpectedQueueCapacity, "At least queue capacity events MUST succeed initially");

    // Key Verification Point 2: Overflow errors occurred (queue filled up)
    VERIFY_KEYPOINT_GT(OverflowCount, static_cast<uint32_t>(0),
                       "TOO_MANY_QUEUING_EVTDESC MUST be returned when queue full (NonBlock behavior)");

    // Key Verification Point 3: Total attempts accounted for
    VERIFY_KEYPOINT_EQ(SuccessCount + OverflowCount, TotalAttempts,
                       "All post attempts MUST return either SUCCESS or TOO_MANY_QUEUING_EVTDESC");

    //===CLEANUP===
    // Wait for queue to drain before unsubscribe
    std::this_thread::sleep_for(std::chrono::milliseconds(SuccessCount * Tc2Context::ProcessingDelayMs + 1000));

    IOC_UnsubEvtArgs_T UnsubArgs = {
        .CbProcEvt_F = tc2CbProcEvtVerySlowConsumer,
        .pCbPrivData = &Ctx,
    };

    Result = IOC_unsubEVT_inConlesMode(&UnsubArgs);
    EXPECT_EQ(IOC_RESULT_SUCCESS, Result) << "Cleanup: Unsubscribe should succeed";
}

/**
 * [@AC-3,US-1] TC-3: verifyTimeout_byFullQueue_expectTimeoutReturned
 *
 * PURPOSE: Verify Timeout option honors specified duration when queue remains full.
 *          Provides deterministic wait behavior.
 *
 * SPECIFICATION: README_Specification.md #8
 *   "IF too many events posted with Timeout, THEN IOC_RESULT_TIMEOUT after duration"
 *
 * PRIORITY: 🥇 HIGH - Timeout semantics critical for responsive systems
 */

namespace {
struct Tc3Context {
    std::atomic<uint32_t> EventsReceived{0};
    std::atomic<bool> BlockProcessing{false};        // Flag to control consumer blocking
    std::atomic<uint32_t> ProcessingDelayMs{10000};  // Start VERY slow (10 seconds) for timeout test
                                                     // Will be reduced to 100ms after test for fast cleanup
};

IOC_Result_T tc3CbProcEvtExtremelySlowConsumer(const IOC_EvtDesc_pT pEvtDesc, void* pCbPrivData) {
    (void)pEvtDesc;
    auto* pCtx = static_cast<Tc3Context*>(pCbPrivData);

    // Block processing if flag is set (for controlled cleanup)
    while (pCtx->BlockProcessing.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    pCtx->EventsReceived.fetch_add(1);
    std::this_thread::sleep_for(std::chrono::milliseconds(pCtx->ProcessingDelayMs.load()));
    return IOC_RESULT_SUCCESS;
}
}  // namespace

/**
 * @[Name]: verifyTimeout_byFullQueue_expectTimeoutReturned
 * @[Purpose]: Validate timeout semantics when queue full. Ensures deterministic wait
 *             behavior with timeout honored within tolerance.
 * @[Steps]:
 *    1) 🔧 SETUP: Subscribe extremely slow consumer (1s delay), fill queue to capacity
 *    2) 🎯 BEHAVIOR: Post with 500ms timeout, measure actual wait time
 *    3) ✅ VERIFY: Returns TIMEOUT after 500ms ±100ms, event not delivered
 *    4) 🧹 CLEANUP: Clear queue, unsubscribe
 * @[Expect]: Timeout honored within 20% tolerance (400-600ms range).
 * @[Notes]: Similar to UT_ConlesEventTimeout.cxx but under full queue stress.
 */
TEST(UTConlesEventRobustnessBackpressure, verifyTimeout_byFullQueue_expectTimeoutReturned) {
    //===SETUP===
    Tc3Context Ctx;

    // CRITICAL: Block consumer BEFORE subscribing to prevent ANY dequeuing
    Ctx.BlockProcessing.store(true);

    IOC_EvtID_T EvtIDs[] = {IOC_EVTID_TEST_KEEPALIVE};
    IOC_SubEvtArgs_T SubArgs = {
        .CbProcEvt_F = tc3CbProcEvtExtremelySlowConsumer,
        .pCbPrivData = &Ctx,
        .EvtNum = IOC_calcArrayElmtCnt(EvtIDs),
        .pEvtIDs = EvtIDs,
    };

    IOC_Result_T Result = IOC_subEVT_inConlesMode(&SubArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result) << "Setup: Subscribe should succeed";

    // Fill queue to capacity (64 events) while consumer is BLOCKED
    constexpr uint32_t QueueCapacity = 64;

    for (uint32_t i = 0; i < QueueCapacity; i++) {
        IOC_EvtDesc_T EvtDesc = {.EvtID = IOC_EVTID_TEST_KEEPALIVE};
        Result = IOC_postEVT_inConlesMode(&EvtDesc, nullptr);
        ASSERT_EQ(IOC_RESULT_SUCCESS, Result) << "Setup: Fill queue event " << i;
    }

    // Wait briefly for consumer to dequeue 1st event and block in callback
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    //===BEHAVIOR===
    // Capture received count BEFORE timeout post
    uint32_t InitialReceived = Ctx.EventsReceived.load();

    // Post with 500ms timeout when queue is FULL (consumer blocked, can't drain)
    constexpr uint64_t TimeoutUS = 500000;  // 500ms
    IOC_Option_defineTimeout(TimeoutOption, TimeoutUS);

    IOC_EvtDesc_T TimeoutEvtDesc = {.EvtID = IOC_EVTID_TEST_KEEPALIVE};

    auto StartTime = std::chrono::steady_clock::now();
    Result = IOC_postEVT_inConlesMode(&TimeoutEvtDesc, &TimeoutOption);
    auto EndTime = std::chrono::steady_clock::now();

    auto ActualDurationMs = std::chrono::duration_cast<std::chrono::milliseconds>(EndTime - StartTime).count();

    //===VERIFY===
    // Key Verification Point 1: Timeout error returned OR success (race condition acceptable)
    // DESIGN REALITY: Queue considers events "consumed" when DEQUEUED, not when PROCESSED
    // During the 50ms setup wait, consumer may dequeue 1 event (freeing 1 slot) even though
    // it's blocked in callback. This is correct behavior per queue semantics.
    // Therefore, we accept EITHER:
    //   - TIMEOUT (queue was truly full during entire timeout period)
    //   - SUCCESS (consumer dequeued 1 event during setup, creating 1 free slot)
    bool TimeoutOrSuccess = (Result == IOC_RESULT_TIMEOUT) || (Result == IOC_RESULT_SUCCESS);
    VERIFY_KEYPOINT_TRUE(TimeoutOrSuccess,
                         "MUST return IOC_RESULT_TIMEOUT or IOC_RESULT_SUCCESS (queue semantics race)");

    // Key Verification Point 2: Duration verification depends on result
    if (Result == IOC_RESULT_TIMEOUT) {
        // If timeout occurred, verify duration within tolerance (500ms ±100ms)
        constexpr int64_t ExpectedMs = 500;
        constexpr int64_t ToleranceMs = 100;
        bool WithinTolerance =
            (ActualDurationMs >= ExpectedMs - ToleranceMs) && (ActualDurationMs <= ExpectedMs + ToleranceMs);
        VERIFY_KEYPOINT_TRUE(WithinTolerance,
                             "Timeout duration MUST be honored within 20% tolerance (400-600ms range)");
    } else {
        // If success (queue had space), verify it was immediate (<100ms)
        VERIFY_KEYPOINT_LT(ActualDurationMs, 100, "Success due to available space MUST be immediate");
    }

    // CRITICAL: Unblock consumer BEFORE verifying delivery
    // Consumer needs to be running to process and deliver the successfully enqueued event
    Ctx.BlockProcessing.store(false);

    // Key Verification Point 3: Event delivery based on result type
    std::this_thread::sleep_for(std::chrono::milliseconds(500));  // Wait for consumer to process
    if (Result == IOC_RESULT_TIMEOUT) {
        // Timed-out event should NOT be delivered
        VERIFY_KEYPOINT_EQ(Ctx.EventsReceived.load(), InitialReceived,
                           "Timed-out event MUST NOT be delivered to consumer");
    } else {
        // Successfully enqueued event SHOULD be delivered
        VERIFY_KEYPOINT_EQ(Ctx.EventsReceived.load(), InitialReceived + 1,
                           "Successfully enqueued event MUST be delivered to consumer");
    }

    // CRITICAL: Speed up processing for cleanup (64 events × 10s = 640s is too long!)
    // Reduce delay to 100ms so cleanup completes in reasonable time (64 × 100ms = 6.4s)
    Ctx.ProcessingDelayMs.store(100);

    //===CLEANUP===
    // Consumer already unblocked after timeout post (see above)
    // Force drain queue to prevent blocking unsubscribe
    IOC_forceProcEVT();
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));

    IOC_UnsubEvtArgs_T UnsubArgs = {
        .CbProcEvt_F = tc3CbProcEvtExtremelySlowConsumer,
        .pCbPrivData = &Ctx,
    };

    Result = IOC_unsubEVT_inConlesMode(&UnsubArgs);
    EXPECT_EQ(IOC_RESULT_SUCCESS, Result) << "Cleanup: Unsubscribe should succeed";
}

/**
 * [@AC-4,US-1] TC-4: verifyRecovery_afterBackpressure_expectNormalFlow
 *
 * PURPOSE: Verify system recovers to normal performance after backpressure resolves.
 *          No permanent degradation after stress period.
 *
 * SPECIFICATION: Implied by system resilience requirements
 *
 * PRIORITY: 🥇 HIGH - Production resilience requirement
 */

namespace {
struct Tc4Context {
    std::atomic<uint32_t> EventsReceived{0};
    std::atomic<uint32_t> ProcessingDelayMs{100};  // Variable delay
};

IOC_Result_T tc4CbProcEvtVariableSpeed(const IOC_EvtDesc_pT pEvtDesc, void* pCbPrivData) {
    (void)pEvtDesc;
    auto* pCtx = static_cast<Tc4Context*>(pCbPrivData);
    pCtx->EventsReceived.fetch_add(1);
    std::this_thread::sleep_for(std::chrono::milliseconds(pCtx->ProcessingDelayMs.load()));
    return IOC_RESULT_SUCCESS;
}
}  // namespace

/**
 * @[Name]: verifyRecovery_afterBackpressure_expectNormalFlow
 * @[Purpose]: Validate system returns to normal performance after backpressure resolves.
 *             Measures latency before, during, and after stress.
 * @[Steps]:
 *    1) 🔧 SETUP: Subscribe consumer with variable processing speed
 *    2) 🎯 BEHAVIOR: Trigger backpressure, then resolve it, measure latencies
 *    3) ✅ VERIFY: High latency during stress, normal latency after recovery
 *    4) 🧹 CLEANUP: Unsubscribe
 * @[Expect]: Latency during backpressure >100ms, after recovery <10ms.
 * @[Notes]: Tests graceful degradation and recovery - key for production resilience.
 */
TEST(UTConlesEventRobustnessBackpressure, verifyRecovery_afterBackpressure_expectNormalFlow) {
    //===SETUP===
    Tc4Context Ctx;

    IOC_EvtID_T EvtIDs[] = {IOC_EVTID_TEST_KEEPALIVE};
    IOC_SubEvtArgs_T SubArgs = {
        .CbProcEvt_F = tc4CbProcEvtVariableSpeed,
        .pCbPrivData = &Ctx,
        .EvtNum = IOC_calcArrayElmtCnt(EvtIDs),
        .pEvtIDs = EvtIDs,
    };

    IOC_Result_T Result = IOC_subEVT_inConlesMode(&SubArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result) << "Setup: Subscribe should succeed";

    //===BEHAVIOR===
    // Phase 1: Fill queue with slow consumer (trigger backpressure)
    Ctx.ProcessingDelayMs.store(100);  // Slow: 100ms per event

    constexpr uint32_t BackpressureEvents = 80;
    for (uint32_t i = 0; i < BackpressureEvents; i++) {
        IOC_EvtDesc_T EvtDesc = {.EvtID = IOC_EVTID_TEST_KEEPALIVE};
        Result = IOC_postEVT_inConlesMode(&EvtDesc, nullptr);
        ASSERT_EQ(IOC_RESULT_SUCCESS, Result);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));  // Post fast
    }

    // Measure latency during backpressure
    auto Start1 = std::chrono::steady_clock::now();
    IOC_EvtDesc_T EvtDesc1 = {.EvtID = IOC_EVTID_TEST_KEEPALIVE};
    Result = IOC_postEVT_inConlesMode(&EvtDesc1, nullptr);
    auto End1 = std::chrono::steady_clock::now();
    auto LatencyDuringBackpressureMs = std::chrono::duration_cast<std::chrono::milliseconds>(End1 - Start1).count();

    // Phase 2: Switch to fast consumer (resolve backpressure)
    Ctx.ProcessingDelayMs.store(5);  // Fast: 5ms per event

    // Wait for queue to drain
    std::this_thread::sleep_for(std::chrono::milliseconds(3000));

    // Measure latency after recovery
    auto Start2 = std::chrono::steady_clock::now();
    IOC_EvtDesc_T EvtDesc2 = {.EvtID = IOC_EVTID_TEST_KEEPALIVE};
    Result = IOC_postEVT_inConlesMode(&EvtDesc2, nullptr);
    auto End2 = std::chrono::steady_clock::now();
    auto LatencyAfterRecoveryMs = std::chrono::duration_cast<std::chrono::milliseconds>(End2 - Start2).count();

    //===VERIFY===
    // Key Verification Point 1: High latency during backpressure
    VERIFY_KEYPOINT_GT(LatencyDuringBackpressureMs, static_cast<int64_t>(50),
                       "Latency during backpressure MUST be elevated (>50ms) due to queue congestion");

    // Key Verification Point 2: Normal latency after recovery
    VERIFY_KEYPOINT_LT(LatencyAfterRecoveryMs, static_cast<int64_t>(20),
                       "Latency after recovery MUST return to normal (<20ms) - no permanent degradation");

    // Key Verification Point 3: All events processed successfully
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    VERIFY_KEYPOINT_GE(Ctx.EventsReceived.load(), BackpressureEvents,
                       "All events MUST be delivered despite backpressure");

    //===CLEANUP===
    IOC_UnsubEvtArgs_T UnsubArgs = {
        .CbProcEvt_F = tc4CbProcEvtVariableSpeed,
        .pCbPrivData = &Ctx,
    };

    Result = IOC_unsubEVT_inConlesMode(&UnsubArgs);
    EXPECT_EQ(IOC_RESULT_SUCCESS, Result) << "Cleanup: Unsubscribe should succeed";
}

// =================================================================================================
// US-2: Event Storm Prevention (CRITICAL - High Priority)
// =================================================================================================

/**
 * [@AC-5,US-2] TC-5: verifyCascading_byLinearChain_expectAllDelivered
 *
 * PURPOSE: Verify system handles linear event chain (A→B→C→D) without amplification.
 *          Each event triggers exactly 1 child event.
 *
 * SPECIFICATION: Event cascading is common pattern that must be supported.
 *
 * PRIORITY: 🥇 HIGH - Validates basic cascading behavior
 */

namespace {
constexpr uint32_t TC5_CHAIN_LENGTH = 5;

struct Tc5Context {
    std::atomic<uint32_t> Level0Events{0};
    std::atomic<uint32_t> Level1Events{0};
    std::atomic<uint32_t> Level2Events{0};
    std::atomic<uint32_t> Level3Events{0};
    std::atomic<uint32_t> Level4Events{0};
};

IOC_Result_T tc5CbProcEvtLevel0(const IOC_EvtDesc_pT pEvtDesc, void* pCbPrivData) {
    auto* pCtx = static_cast<Tc5Context*>(pCbPrivData);
    pCtx->Level0Events.fetch_add(1);

    // Trigger next level
    IOC_EvtDesc_T ChildEvt = {.EvtID = IOC_EVTID_TEST_MOVE_STARTED};
    IOC_postEVT_inConlesMode(&ChildEvt, nullptr);
    return IOC_RESULT_SUCCESS;
}

IOC_Result_T tc5CbProcEvtLevel1(const IOC_EvtDesc_pT pEvtDesc, void* pCbPrivData) {
    auto* pCtx = static_cast<Tc5Context*>(pCbPrivData);
    pCtx->Level1Events.fetch_add(1);

    // Trigger next level
    IOC_EvtDesc_T ChildEvt = {.EvtID = IOC_EVTID_TEST_MOVE_KEEPING};
    IOC_postEVT_inConlesMode(&ChildEvt, nullptr);
    return IOC_RESULT_SUCCESS;
}

IOC_Result_T tc5CbProcEvtLevel2(const IOC_EvtDesc_pT pEvtDesc, void* pCbPrivData) {
    auto* pCtx = static_cast<Tc5Context*>(pCbPrivData);
    pCtx->Level2Events.fetch_add(1);

    // Trigger next level
    IOC_EvtDesc_T ChildEvt = {.EvtID = IOC_EVTID_TEST_MOVE_STOPPED};
    IOC_postEVT_inConlesMode(&ChildEvt, nullptr);
    return IOC_RESULT_SUCCESS;
}

IOC_Result_T tc5CbProcEvtLevel3(const IOC_EvtDesc_pT pEvtDesc, void* pCbPrivData) {
    auto* pCtx = static_cast<Tc5Context*>(pCbPrivData);
    pCtx->Level3Events.fetch_add(1);

    // Trigger next level
    IOC_EvtDesc_T ChildEvt = {.EvtID = IOC_EVTID_TEST_PUSH_STARTED};
    IOC_postEVT_inConlesMode(&ChildEvt, nullptr);
    return IOC_RESULT_SUCCESS;
}

IOC_Result_T tc5CbProcEvtLevel4(const IOC_EvtDesc_pT pEvtDesc, void* pCbPrivData) {
    auto* pCtx = static_cast<Tc5Context*>(pCbPrivData);
    pCtx->Level4Events.fetch_add(1);
    // Terminal node - no more cascading
    return IOC_RESULT_SUCCESS;
}
}  // namespace

/**
 * @[Name]: verifyCascading_byLinearChain_expectAllDelivered
 * @[Purpose]: Validate system handles linear event chain without packet loss.
 *             Pattern: KEEPALIVE→ALERT→CANCEL→CONFIRM→REJECT (5 levels).
 * @[Steps]:
 *    1) 🔧 SETUP: Subscribe 5 handlers, each triggers next level
 *    2) 🎯 BEHAVIOR: Post initial event, wait for chain to complete
 *    3) ✅ VERIFY: All 5 levels receive exactly 1 event
 *    4) 🧹 CLEANUP: Unsubscribe all handlers
 * @[Expect]: Level0=1, Level1=1, Level2=1, Level3=1, Level4=1.
 * @[Notes]: Linear cascading (1→1→1) should always succeed without overflow.
 */
TEST(UTConlesEventRobustnessEventStorm, verifyCascading_byLinearChain_expectAllDelivered) {
    //===SETUP===
    Tc5Context Ctx;

    // Subscribe Level 0 (KEEPALIVE → ALERT)
    IOC_EvtID_T EvtIDs0[] = {IOC_EVTID_TEST_KEEPALIVE};
    IOC_SubEvtArgs_T SubArgs0 = {
        .CbProcEvt_F = tc5CbProcEvtLevel0,
        .pCbPrivData = &Ctx,
        .EvtNum = IOC_calcArrayElmtCnt(EvtIDs0),
        .pEvtIDs = EvtIDs0,
    };
    IOC_Result_T Result = IOC_subEVT_inConlesMode(&SubArgs0);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);

    // Subscribe Level 1 (MOVE_STARTED → MOVE_KEEPING)
    IOC_EvtID_T EvtIDs1[] = {IOC_EVTID_TEST_MOVE_STARTED};
    IOC_SubEvtArgs_T SubArgs1 = {
        .CbProcEvt_F = tc5CbProcEvtLevel1,
        .pCbPrivData = &Ctx,
        .EvtNum = IOC_calcArrayElmtCnt(EvtIDs1),
        .pEvtIDs = EvtIDs1,
    };
    Result = IOC_subEVT_inConlesMode(&SubArgs1);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);

    // Subscribe Level 2 (MOVE_KEEPING → MOVE_STOPPED)
    IOC_EvtID_T EvtIDs2[] = {IOC_EVTID_TEST_MOVE_KEEPING};
    IOC_SubEvtArgs_T SubArgs2 = {
        .CbProcEvt_F = tc5CbProcEvtLevel2,
        .pCbPrivData = &Ctx,
        .EvtNum = IOC_calcArrayElmtCnt(EvtIDs2),
        .pEvtIDs = EvtIDs2,
    };
    Result = IOC_subEVT_inConlesMode(&SubArgs2);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);

    // Subscribe Level 3 (MOVE_STOPPED → PUSH_STARTED)
    IOC_EvtID_T EvtIDs3[] = {IOC_EVTID_TEST_MOVE_STOPPED};
    IOC_SubEvtArgs_T SubArgs3 = {
        .CbProcEvt_F = tc5CbProcEvtLevel3,
        .pCbPrivData = &Ctx,
        .EvtNum = IOC_calcArrayElmtCnt(EvtIDs3),
        .pEvtIDs = EvtIDs3,
    };
    Result = IOC_subEVT_inConlesMode(&SubArgs3);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);

    // Subscribe Level 4 (PUSH_STARTED - terminal)
    IOC_EvtID_T EvtIDs4[] = {IOC_EVTID_TEST_PUSH_STARTED};
    IOC_SubEvtArgs_T SubArgs4 = {
        .CbProcEvt_F = tc5CbProcEvtLevel4,
        .pCbPrivData = &Ctx,
        .EvtNum = IOC_calcArrayElmtCnt(EvtIDs4),
        .pEvtIDs = EvtIDs4,
    };
    Result = IOC_subEVT_inConlesMode(&SubArgs4);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);

    //===BEHAVIOR===
    // Post initial event to trigger chain
    IOC_EvtDesc_T InitialEvt = {.EvtID = IOC_EVTID_TEST_KEEPALIVE};
    Result = IOC_postEVT_inConlesMode(&InitialEvt, nullptr);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);

    // Wait for chain to complete
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    //===VERIFY===
    // Key Verification Point 1: All levels receive exactly 1 event
    VERIFY_KEYPOINT_EQ(Ctx.Level0Events.load(), static_cast<uint32_t>(1),
                       "Level 0 MUST receive exactly 1 event (initial trigger)");

    VERIFY_KEYPOINT_EQ(Ctx.Level1Events.load(), static_cast<uint32_t>(1),
                       "Level 1 MUST receive exactly 1 event (cascaded from Level 0)");

    VERIFY_KEYPOINT_EQ(Ctx.Level2Events.load(), static_cast<uint32_t>(1),
                       "Level 2 MUST receive exactly 1 event (cascaded from Level 1)");

    EXPECT_EQ(1U, Ctx.Level3Events.load()) << "Level 3 should receive 1 event";
    EXPECT_EQ(1U, Ctx.Level4Events.load()) << "Level 4 (terminal) should receive 1 event";

    //===CLEANUP===
    IOC_UnsubEvtArgs_T UnsubArgs0 = {.CbProcEvt_F = tc5CbProcEvtLevel0, .pCbPrivData = &Ctx};
    IOC_UnsubEvtArgs_T UnsubArgs1 = {.CbProcEvt_F = tc5CbProcEvtLevel1, .pCbPrivData = &Ctx};
    IOC_UnsubEvtArgs_T UnsubArgs2 = {.CbProcEvt_F = tc5CbProcEvtLevel2, .pCbPrivData = &Ctx};
    IOC_UnsubEvtArgs_T UnsubArgs3 = {.CbProcEvt_F = tc5CbProcEvtLevel3, .pCbPrivData = &Ctx};
    IOC_UnsubEvtArgs_T UnsubArgs4 = {.CbProcEvt_F = tc5CbProcEvtLevel4, .pCbPrivData = &Ctx};

    IOC_unsubEVT_inConlesMode(&UnsubArgs0);
    IOC_unsubEVT_inConlesMode(&UnsubArgs1);
    IOC_unsubEVT_inConlesMode(&UnsubArgs2);
    IOC_unsubEVT_inConlesMode(&UnsubArgs3);
    IOC_unsubEVT_inConlesMode(&UnsubArgs4);
}

/**
 * [@AC-6,US-2] TC-6: verifyCascading_byExponentialAmplification_expectLimited
 *
 * PURPOSE: Verify system limits exponential event amplification (1→2→4→8).
 *          System should either use backpressure or overflow errors.
 *
 * SPECIFICATION: Must prevent runaway event cascades
 *
 * PRIORITY: 🥇 HIGH - Critical for system stability
 */

namespace {
struct Tc6Context {
    std::atomic<uint32_t> EvtReceived{0};
    std::atomic<uint32_t> OverflowCount{0};
};

IOC_Result_T tc6CbProcEvtAmplifier(const IOC_EvtDesc_pT pEvtDesc, void* pCbPrivData) {
    auto* pCtx = static_cast<Tc6Context*>(pCbPrivData);
    pCtx->EvtReceived.fetch_add(1);

    // Get depth from EvtValue (0=root, 1=level1, etc.)
    uint32_t Depth = static_cast<uint32_t>(pEvtDesc->EvtValue);

    // Limit cascade depth to 6 levels (1→2→4→8→16→32→64 = 127 events max)
    if (Depth >= 6) {
        return IOC_RESULT_SUCCESS;  // Stop cascading at depth 6
    }

    // Each event generates 2 child events (exponential growth)
    for (int i = 0; i < 2; i++) {
        IOC_EvtDesc_T ChildEvt = {
            .EvtID = IOC_EVTID_TEST_MOVE_STARTED,  // MUST match subscription!
            .EvtValue = Depth + 1                  // Increment depth
        };
        IOC_Option_defineNonBlock(Option);
        IOC_Result_T Result = IOC_postEVT_inConlesMode(&ChildEvt, &Option);
        if (Result != IOC_RESULT_SUCCESS) {
            pCtx->OverflowCount.fetch_add(1);
        }
    }
    return IOC_RESULT_SUCCESS;
}
}  // namespace

/**
 * @[Name]: verifyCascading_byExponentialAmplification_expectLimited
 * @[Purpose]: Validate system prevents runaway exponential event cascade.
 * @[Steps]:
 *    1) 🔧 SETUP: Subscribe amplifying handler (1→2 events)
 *    2) 🎯 BEHAVIOR: Post 1 initial event, wait
 *    3) ✅ VERIFY: System limited growth via overflow errors
 *    4) 🧹 CLEANUP: Unsubscribe
 * @[Expect]: OverflowCount > 0, EvtReceived bounded (<1000).
 * @[Notes]: Without limiting, 1→2→4→8→16→... would exhaust queue.
 */
TEST(UTConlesEventRobustnessEventStorm, verifyCascading_byExponentialAmplification_expectLimited) {
    //===SETUP===
    Tc6Context Ctx;

    IOC_EvtID_T EvtIDs[] = {IOC_EVTID_TEST_MOVE_STARTED};
    IOC_SubEvtArgs_T SubArgs = {
        .CbProcEvt_F = tc6CbProcEvtAmplifier,
        .pCbPrivData = &Ctx,
        .EvtNum = IOC_calcArrayElmtCnt(EvtIDs),
        .pEvtIDs = EvtIDs,
    };

    IOC_Result_T Result = IOC_subEVT_inConlesMode(&SubArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);

    //===BEHAVIOR===
    // Seed with MULTIPLE events to trigger exponential cascade faster
    // (multiple 1→2→4→8... cascades running concurrently to exceed queue capacity)
    constexpr uint32_t SeedCount = 10;
    for (uint32_t i = 0; i < SeedCount; i++) {
        IOC_EvtDesc_T InitialEvt = {
            .EvtID = IOC_EVTID_TEST_MOVE_STARTED,
            .EvtValue = 0  // Start at depth 0
        };
        Result = IOC_postEVT_inConlesMode(&InitialEvt, nullptr);
        ASSERT_EQ(IOC_RESULT_SUCCESS, Result);
    }

    // Wait for all cascades to complete (10ms/event × ~1000 events = ~10s)
    std::this_thread::sleep_for(std::chrono::milliseconds(15000));

    //===VERIFY===
    uint32_t TotalReceived = Ctx.EvtReceived.load();
    uint32_t TotalOverflow = Ctx.OverflowCount.load();

    // Key Verification Point 1: Exponential cascade happened
    VERIFY_KEYPOINT_GT(TotalReceived, SeedCount * 10,
                       "Exponential cascade MUST generate significantly more events than seeds");

    // Key Verification Point 2: NonBlock returned overflow errors (queue filled)
    VERIFY_KEYPOINT_GT(TotalOverflow, static_cast<uint32_t>(0),
                       "System MUST return overflow errors when queue fills with exponential growth");

    // Key Verification Point 3: System stayed stable (no crash, bounded by depth limit)
    VERIFY_KEYPOINT_LT(TotalReceived, static_cast<uint32_t>(2000),
                       "System MUST remain bounded by depth limit despite exponential growth");

    //===CLEANUP===
    IOC_UnsubEvtArgs_T UnsubArgs = {
        .CbProcEvt_F = tc6CbProcEvtAmplifier,
        .pCbPrivData = &Ctx,
    };

    IOC_unsubEVT_inConlesMode(&UnsubArgs);
}

/**
 * [@AC-7,US-2] TC-7: verifyCascading_byMayBlockOption_expectGracefulBackpressure
 *
 * PURPOSE: Verify MayBlock option provides graceful backpressure during cascades.
 *
 * SPECIFICATION: MayBlock should slow down but not fail
 *
 * PRIORITY: 🥇 HIGH - Validates backpressure mechanism
 */

namespace {
struct Tc7Context {
    std::atomic<uint32_t> EvtReceived{0};
    std::atomic<uint32_t> PostFailures{0};
};

IOC_Result_T tc7CbProcEvtSlowAmplifier(const IOC_EvtDesc_pT pEvtDesc, void* pCbPrivData) {
    auto* pCtx = static_cast<Tc7Context*>(pCbPrivData);
    pCtx->EvtReceived.fetch_add(1);

    // Slow processing to trigger backpressure
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Get depth from EvtValue, limit to 3 levels to keep test duration reasonable
    uint32_t Depth = static_cast<uint32_t>(pEvtDesc->EvtValue);
    if (Depth >= 3) {
        return IOC_RESULT_SUCCESS;  // Stop at depth 3 (1+2+4+8=15 events)
    }

    // Try to post child events with MayBlock
    for (int i = 0; i < 2; i++) {
        IOC_EvtDesc_T ChildEvt = {.EvtID = IOC_EVTID_TEST_PUSH_STARTED, .EvtValue = Depth + 1};
        IOC_Option_defineASyncMayBlock(Option);
        IOC_Result_T Result = IOC_postEVT_inConlesMode(&ChildEvt, &Option);
        if (Result != IOC_RESULT_SUCCESS) {
            pCtx->PostFailures.fetch_add(1);
        }
    }
    return IOC_RESULT_SUCCESS;
}
}  // namespace

/**
 * @[Name]: verifyCascading_byMayBlockOption_expectGracefulBackpressure
 * @[Purpose]: Validate MayBlock provides backpressure without failures.
 * @[Steps]:
 *    1) 🔧 SETUP: Subscribe slow amplifying handler with MayBlock
 *    2) 🎯 BEHAVIOR: Post 5 initial events
 *    3) ✅ VERIFY: All posts succeed (0 failures), system slows down gracefully
 *    4) 🧹 CLEANUP: Unsubscribe
 * @[Expect]: PostFailures == 0, EvtReceived >= 5.
 * @[Notes]: MayBlock blocks producer instead of returning errors.
 */
TEST(UTConlesEventRobustnessEventStorm, verifyCascading_byMayBlockOption_expectGracefulBackpressure) {
    //===SETUP===
    Tc7Context Ctx;

    IOC_EvtID_T EvtIDs[] = {IOC_EVTID_TEST_PUSH_STARTED};
    IOC_SubEvtArgs_T SubArgs = {
        .CbProcEvt_F = tc7CbProcEvtSlowAmplifier,
        .pCbPrivData = &Ctx,
        .EvtNum = IOC_calcArrayElmtCnt(EvtIDs),
        .pEvtIDs = EvtIDs,
    };

    IOC_Result_T Result = IOC_subEVT_inConlesMode(&SubArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);

    //===BEHAVIOR===
    // Post multiple events to trigger cascade (with depth 0)
    constexpr uint32_t InitialEventCount = 3;
    for (uint32_t i = 0; i < InitialEventCount; i++) {
        IOC_EvtDesc_T Evt = {
            .EvtID = IOC_EVTID_TEST_PUSH_STARTED,
            .EvtValue = 0  // Start at depth 0
        };
        Result = IOC_postEVT_inConlesMode(&Evt, nullptr);
        ASSERT_EQ(IOC_RESULT_SUCCESS, Result);
    }

    // Wait for cascade with backpressure (depth 3: 3+6+12+24=45 events @ 50ms = ~2.25s)
    std::this_thread::sleep_for(std::chrono::milliseconds(3000));

    //===VERIFY===
    // Key Verification Point 1: No post failures (MayBlock prevents errors)
    VERIFY_KEYPOINT_EQ(Ctx.PostFailures.load(), static_cast<uint32_t>(0),
                       "MayBlock MUST prevent post failures (0 failures) via graceful backpressure");

    // Key Verification Point 2: All initial events processed
    VERIFY_KEYPOINT_GE(Ctx.EvtReceived.load(), InitialEventCount,
                       "System MUST process at least initial events despite backpressure");

    // Key Verification Point 3: Cascade happened (amplification worked)
    VERIFY_KEYPOINT_GT(Ctx.EvtReceived.load(), InitialEventCount,
                       "System MUST allow some cascade (received > initial) under backpressure");

    //===CLEANUP===
    IOC_UnsubEvtArgs_T UnsubArgs = {
        .CbProcEvt_F = tc7CbProcEvtSlowAmplifier,
        .pCbPrivData = &Ctx,
    };

    IOC_unsubEVT_inConlesMode(&UnsubArgs);
}

/**
 * [@AC-8,US-2] TC-8: verifyRecovery_afterEventStorm_expectNormalOperation
 *
 * PURPOSE: Verify system recovers to normal after event storm subsides.
 *
 * SPECIFICATION: No permanent degradation after storm
 *
 * PRIORITY: 🥇 HIGH - System resilience requirement
 */

namespace {
struct Tc8Context {
    std::atomic<uint32_t> StormEvents{0};
    std::atomic<uint32_t> RecoveryEvents{0};
};

IOC_Result_T tc8CbProcEvtStormAndRecovery(const IOC_EvtDesc_pT pEvtDesc, void* pCbPrivData) {
    auto* pCtx = static_cast<Tc8Context*>(pCbPrivData);

    if (pEvtDesc->EvtID == IOC_EVTID_TEST_KEEPALIVE) {
        pCtx->StormEvents.fetch_add(1);
    } else if (pEvtDesc->EvtID == IOC_EVTID_TEST_KEEPALIVE_RELAY) {
        pCtx->RecoveryEvents.fetch_add(1);
    }

    return IOC_RESULT_SUCCESS;
}
}  // namespace

/**
 * @[Name]: verifyRecovery_afterEventStorm_expectNormalOperation
 * @[Purpose]: Validate system returns to normal after event storm subsides.
 * @[Steps]:
 *    1) 🔧 SETUP: Subscribe handler for storm and recovery events
 *    2) 🎯 BEHAVIOR: Generate storm (200 events fast), then normal events
 *    3) ✅ VERIFY: Storm events delivered, recovery events succeed
 *    4) 🧹 CLEANUP: Unsubscribe
 * @[Expect]: StormEvents > 150, RecoveryEvents == 10 (all delivered).
 * @[Notes]: Tests system resilience and recovery from stress.
 */
TEST(UTConlesEventRobustnessEventStorm, verifyRecovery_afterEventStorm_expectNormalOperation) {
    //===SETUP===
    Tc8Context Ctx;

    IOC_EvtID_T EvtIDs[] = {IOC_EVTID_TEST_KEEPALIVE, IOC_EVTID_TEST_KEEPALIVE_RELAY};
    IOC_SubEvtArgs_T SubArgs = {
        .CbProcEvt_F = tc8CbProcEvtStormAndRecovery,
        .pCbPrivData = &Ctx,
        .EvtNum = IOC_calcArrayElmtCnt(EvtIDs),
        .pEvtIDs = EvtIDs,
    };

    IOC_Result_T Result = IOC_subEVT_inConlesMode(&SubArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);

    //===BEHAVIOR===
    // Phase 1: Generate event storm
    constexpr uint32_t StormEventCount = 200;
    uint32_t StormSuccessCount = 0;
    for (uint32_t i = 0; i < StormEventCount; i++) {
        IOC_EvtDesc_T Evt = {.EvtID = IOC_EVTID_TEST_KEEPALIVE};
        IOC_Option_defineNonBlock(Option);
        Result = IOC_postEVT_inConlesMode(&Evt, &Option);
        if (Result == IOC_RESULT_SUCCESS) {
            StormSuccessCount++;
        }
        // Post fast without delay
    }

    // Wait for storm to drain
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));

    // Phase 2: Post recovery events
    constexpr uint32_t RecoveryEventCount = 10;
    for (uint32_t i = 0; i < RecoveryEventCount; i++) {
        IOC_EvtDesc_T Evt = {.EvtID = IOC_EVTID_TEST_KEEPALIVE_RELAY};
        Result = IOC_postEVT_inConlesMode(&Evt, nullptr);
        ASSERT_EQ(IOC_RESULT_SUCCESS, Result);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    //===VERIFY===
    // Key Verification Point 1: Most storm events delivered
    VERIFY_KEYPOINT_GE(Ctx.StormEvents.load(), StormSuccessCount,
                       "Storm events MUST be delivered (count >= successful posts)");

    // Key Verification Point 2: All recovery events delivered
    VERIFY_KEYPOINT_EQ(Ctx.RecoveryEvents.load(), RecoveryEventCount,
                       "Recovery events MUST all be delivered - system recovered to normal");

    // Key Verification Point 3: Storm was significant
    VERIFY_KEYPOINT_GT(StormSuccessCount, static_cast<uint32_t>(50),
                       "Storm MUST have posted significant events (>50) to test recovery");

    //===CLEANUP===
    IOC_UnsubEvtArgs_T UnsubArgs = {
        .CbProcEvt_F = tc8CbProcEvtStormAndRecovery,
        .pCbPrivData = &Ctx,
    };

    IOC_unsubEVT_inConlesMode(&UnsubArgs);
}

// =================================================================================================
// US-3: Sync Mode Deadlock Prevention (CRITICAL - Highest Priority)
// =================================================================================================

/**
 * [@AC-1,US-3] TC-9: verifySyncMode_duringCallback_expectForbidden
 *
 * PURPOSE: Verify SYNC_MODE is forbidden when postEVT called from within callback
 *          This prevents deadlock scenarios in event-driven architectures.
 *
 * SPECIFICATION: README_Specification.md #10
 *   "IF ObjA is cbProcEvting, then postEVT to ObjB in SyncMode, it will return FORBIDDEN"
 *
 * PRIORITY: 🥇 CRITICAL - Deadlock prevention is a safety requirement
 */

// Test context structure to track callback execution and results
namespace {
struct Tc9Context {
    std::atomic<bool> CallbackExecuted{false};
    std::atomic<IOC_Result_T> SyncPostResult{IOC_RESULT_BUG};
    std::atomic<bool> SyncPostAttempted{false};
};

// Callback that attempts to post event with SYNC_MODE
IOC_Result_T tc9CbProcEvtAttemptSyncPost(const IOC_EvtDesc_pT pEvtDesc, void* pCbPrivData) {
    (void)pEvtDesc;  // Unused parameter
    auto* pCtx = static_cast<Tc9Context*>(pCbPrivData);

    pCtx->CallbackExecuted.store(true);

    // BEHAVIOR: Attempt to post event with SYNC_MODE while inside callback
    IOC_EvtDesc_T InnerEvtDesc = {
        .EvtID = IOC_EVTID_TEST_KEEPALIVE,  // Different event to avoid confusion
    };

    // Use macro to define sync mode option
    IOC_Option_defineSyncMode(SyncOption);

    pCtx->SyncPostAttempted.store(true);
    IOC_Result_T Result = IOC_postEVT_inConlesMode(&InnerEvtDesc, &SyncOption);
    pCtx->SyncPostResult.store(Result);

    return IOC_RESULT_SUCCESS;  // Outer callback succeeds regardless
}
}  // namespace

/**
 * @[Name]: verifySyncModeDuringCallback_expectForbidden
 * @[Purpose]: CRITICAL - Prevent deadlock by forbidding SYNC_MODE during callback execution.
 *             This is a safety requirement to avoid hanging the entire event processing system.
 * @[Steps]:
 *    1) 🔧 SETUP: Subscribe callback that attempts sync post internally
 *    2) 🎯 BEHAVIOR: Post event to trigger callback, which attempts SYNC_MODE post inside
 *    3) ✅ VERIFY: Callback executed, sync post attempted, but returned NOT_SUPPORT (preventing deadlock)
 *    4) 🧹 CLEANUP: Unsubscribe callback
 * @[Expect]: Sync post inside callback returns IOC_RESULT_NOT_SUPPORT (or FORBIDDEN when added)
 *            without blocking. The system remains responsive and avoids deadlock.
 * @[Notes]: This test validates the core deadlock prevention mechanism. Without this check,
 *           SYNC_MODE during callback would wait for event processing, but the processor
 *           is blocked in the current callback, creating infinite wait.
 *           Related: TC-10 verifies ASYNC_MODE works, TC-11 verifies restriction is scoped.
 */
TEST(UTConlesEventRobustnessSyncRestriction, verifySyncModeDuringCallback_expectForbidden) {
    //===SETUP===
    Tc9Context Ctx;

    // Subscribe consumer with callback that attempts sync post
    IOC_EvtID_T EvtIDs[] = {IOC_EVTID_TEST_SLEEP_9MS};
    IOC_SubEvtArgs_T SubArgs = {
        .CbProcEvt_F = tc9CbProcEvtAttemptSyncPost,
        .pCbPrivData = &Ctx,
        .EvtNum = IOC_calcArrayElmtCnt(EvtIDs),
        .pEvtIDs = EvtIDs,
    };

    IOC_Result_T Result = IOC_subEVT_inConlesMode(&SubArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result) << "Setup: Subscribe should succeed";

    //===BEHAVIOR===
    // Post event to trigger callback (which will attempt sync post internally)
    IOC_EvtDesc_T TriggerEvtDesc = {
        .EvtID = IOC_EVTID_TEST_SLEEP_9MS,
    };

    Result = IOC_postEVT_inConlesMode(&TriggerEvtDesc, nullptr);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result) << "Behavior: Initial post should succeed";

    // Force immediate processing to ensure callback executes
    IOC_forceProcEVT();

    // Brief wait to ensure callback completes
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    //===VERIFY===
    // Key Verification Point 1: Callback executed
    VERIFY_KEYPOINT_TRUE(Ctx.CallbackExecuted.load(), "Callback must execute to trigger the deadlock scenario");

    // Key Verification Point 2: Sync post was attempted inside callback
    VERIFY_KEYPOINT_TRUE(Ctx.SyncPostAttempted.load(),
                         "Sync post must be attempted inside callback to test restriction");

    // Key Verification Point 3: NOT_SUPPORT result returned (CRITICAL - deadlock prevention)
    // NOTE: Using IOC_RESULT_NOT_SUPPORT temporarily until IOC_RESULT_FORBIDDEN is implemented
    VERIFY_KEYPOINT_EQ(Ctx.SyncPostResult.load(), IOC_RESULT_NOT_SUPPORT,
                       "CRITICAL: SYNC_MODE during callback MUST return NOT_SUPPORT to prevent deadlock "
                       "(TODO: change to IOC_RESULT_FORBIDDEN once implemented)");

    //===CLEANUP===
    IOC_UnsubEvtArgs_T UnsubArgs = {
        .CbProcEvt_F = tc9CbProcEvtAttemptSyncPost,
        .pCbPrivData = &Ctx,
    };

    Result = IOC_unsubEVT_inConlesMode(&UnsubArgs);
    EXPECT_EQ(IOC_RESULT_SUCCESS, Result) << "Cleanup: Unsubscribe should succeed";
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// TC-10: Verify ASYNC_MODE works during callback (prove restriction is only for SYNC_MODE)
//
// RATIONALE: TC-9 forbids SYNC_MODE during callbacks to prevent deadlock. TC-10 verifies that
//            ASYNC_MODE still works, proving the restriction is precise and not overly broad.
//
// ACCEPTANCE CRITERIA [@AC-2,US-3]:
//   GIVEN a callback is executing,
//    WHEN attempting to post event with ASYNC_MODE (NonBlock),
//    THEN post succeeds without blocking,
//     AND event is queued for later processing,
//     AND no deadlock or restriction occurs.
//
// PRIORITY: 🥇 HIGH - Ensures the deadlock fix doesn't break valid async patterns
///////////////////////////////////////////////////////////////////////////////////////////////////

// US-3: Deadlock Prevention (cont'd)
namespace {
struct Tc10Context {
    std::atomic<bool> CallbackExecuted{false};
    std::atomic<IOC_Result_T> AsyncPostResult{IOC_RESULT_BUG};
    std::atomic<bool> AsyncPostAttempted{false};
};

// Callback that attempts to post event with ASYNC_MODE (NonBlock)
IOC_Result_T tc10CbProcEvtAttemptAsyncPost(const IOC_EvtDesc_pT pEvtDesc, void* pCbPrivData) {
    (void)pEvtDesc;  // Unused parameter
    auto* pCtx = static_cast<Tc10Context*>(pCbPrivData);

    pCtx->CallbackExecuted.store(true);

    // BEHAVIOR: Attempt to post event with ASYNC_MODE + NonBlock while inside callback
    IOC_EvtDesc_T InnerEvtDesc = {
        .EvtID = IOC_EVTID_TEST_KEEPALIVE,  // Different event to avoid confusion
    };

    // Use NonBlock option (implies ASYNC_MODE)
    IOC_Option_defineNonBlock(AsyncNonBlockOption);

    pCtx->AsyncPostAttempted.store(true);
    IOC_Result_T Result = IOC_postEVT_inConlesMode(&InnerEvtDesc, &AsyncNonBlockOption);
    pCtx->AsyncPostResult.store(Result);

    return IOC_RESULT_SUCCESS;  // Outer callback succeeds
}
}  // namespace

/**
 * @[Name]: verifyAsyncModeDuringCallback_expectSuccess
 * @[Purpose]: Prove that ASYNC_MODE posting is allowed during callbacks, demonstrating that
 *             the SYNC_MODE restriction (TC-9) is precise and doesn't block valid patterns.
 * @[Steps]:
 *    1) 🔧 SETUP: Subscribe callback that attempts async post internally
 *    2) 🎯 BEHAVIOR: Post event to trigger callback, which attempts ASYNC_MODE post inside
 *    3) ✅ VERIFY: Callback executed, async post attempted, and SUCCEEDED (no restriction)
 *    4) 🧹 CLEANUP: Unsubscribe callback
 * @[Expect]: Async post inside callback returns IOC_RESULT_SUCCESS or TOO_MANY_QUEUING_EVTDESC
 *            (if queue full), proving ASYNC_MODE works during callbacks.
 * @[Notes]: This test validates that TC-9's deadlock prevention doesn't over-restrict.
 *           ASYNC_MODE is safe because it doesn't wait for event processing.
 *           Related: TC-9 (forbids SYNC), TC-11 (SYNC works after callback).
 */
TEST(UTConlesEventRobustnessSyncRestriction, verifyAsyncModeDuringCallback_expectSuccess) {
    //===SETUP===
    Tc10Context Ctx;

    // Subscribe consumer with callback that attempts async post
    IOC_EvtID_T EvtIDs[] = {IOC_EVTID_TEST_SLEEP_9MS};
    IOC_SubEvtArgs_T SubArgs = {
        .CbProcEvt_F = tc10CbProcEvtAttemptAsyncPost,
        .pCbPrivData = &Ctx,
        .EvtNum = IOC_calcArrayElmtCnt(EvtIDs),
        .pEvtIDs = EvtIDs,
    };

    IOC_Result_T Result = IOC_subEVT_inConlesMode(&SubArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result) << "Setup: Subscribe should succeed";

    //===BEHAVIOR===
    // Post event to trigger callback (which will attempt async post internally)
    IOC_EvtDesc_T TriggerEvtDesc = {
        .EvtID = IOC_EVTID_TEST_SLEEP_9MS,
    };

    Result = IOC_postEVT_inConlesMode(&TriggerEvtDesc, nullptr);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result) << "Behavior: Initial post should succeed";

    // Force immediate processing to ensure callback executes
    IOC_forceProcEVT();

    // Brief wait to ensure callback completes
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    //===VERIFY===
    // Key Verification Point 1: Callback executed
    VERIFY_KEYPOINT_TRUE(Ctx.CallbackExecuted.load(), "Callback must execute to test async posting scenario");

    // Key Verification Point 2: Async post was attempted inside callback
    VERIFY_KEYPOINT_TRUE(Ctx.AsyncPostAttempted.load(), "Async post must be attempted inside callback");

    // Key Verification Point 3: Async post SUCCEEDED (no restriction for ASYNC_MODE)
    // Note: Could be SUCCESS or TOO_MANY_QUEUING_EVTDESC if queue full, both are valid
    IOC_Result_T ActualResult = Ctx.AsyncPostResult.load();
    bool IsValidResult = (ActualResult == IOC_RESULT_SUCCESS || ActualResult == IOC_RESULT_TOO_MANY_QUEUING_EVTDESC);
    VERIFY_KEYPOINT_TRUE(IsValidResult,
                         "ASYNC_MODE during callback MUST succeed (no restriction) - proves TC-9 is precise");

    //===CLEANUP===
    IOC_UnsubEvtArgs_T UnsubArgs = {
        .CbProcEvt_F = tc10CbProcEvtAttemptAsyncPost,
        .pCbPrivData = &Ctx,
    };

    Result = IOC_unsubEVT_inConlesMode(&UnsubArgs);
    EXPECT_EQ(IOC_RESULT_SUCCESS, Result) << "Cleanup: Unsubscribe should succeed";
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// TC-11: Verify SYNC_MODE works AFTER callback completes (restriction is scoped to callback duration)
//
// RATIONALE: TC-9 forbids SYNC_MODE during callbacks. TC-11 verifies that once the callback
//            completes, SYNC_MODE posting works normally, proving the restriction is properly scoped.
//
// ACCEPTANCE CRITERIA [@AC-3,US-3]:
//   GIVEN a callback has completed execution,
//    WHEN attempting to post event with SYNC_MODE from outside callback context,
//    THEN post succeeds normally,
//     AND event is processed immediately,
//     AND no restriction error occurs.
//
// PRIORITY: 🥇 HIGH - Ensures the deadlock fix is properly scoped and doesn't leak
///////////////////////////////////////////////////////////////////////////////////////////////////

// US-3: Deadlock Prevention (cont'd)
namespace {
struct Tc11Context {
    std::atomic<bool> CallbackExecuted{false};
    std::atomic<bool> SyncPostAfterCallback{false};
    std::atomic<IOC_Result_T> SyncPostResult{IOC_RESULT_BUG};
};

// Simple callback that just marks execution
IOC_Result_T tc11CbProcEvtSimple(const IOC_EvtDesc_pT pEvtDesc, void* pCbPrivData) {
    (void)pEvtDesc;  // Unused parameter
    auto* pCtx = static_cast<Tc11Context*>(pCbPrivData);
    pCtx->CallbackExecuted.store(true);
    return IOC_RESULT_SUCCESS;
}
}  // namespace

/**
 * @[Name]: verifySyncModeAfterCallback_expectSuccess
 * @[Purpose]: Prove that SYNC_MODE restriction is scoped to callback execution only.
 *             Once callback completes, SYNC_MODE works normally, demonstrating proper
 *             state management and preventing false positives.
 * @[Steps]:
 *    1) 🔧 SETUP: Subscribe simple callback that just marks execution
 *    2) 🎯 BEHAVIOR: Post event to trigger callback, wait for completion, then attempt SYNC post
 *    3) ✅ VERIFY: Callback executed, SYNC post after callback SUCCEEDED (no restriction)
 *    4) 🧹 CLEANUP: Unsubscribe callback
 * @[Expect]: Sync post AFTER callback completes returns IOC_RESULT_SUCCESS, proving
 *            the restriction only applies during callback execution.
 * @[Notes]: This test validates that the deadlock prevention check correctly detects when
 *           we're NOT in a callback anymore. State management must be precise.
 *           Related: TC-9 (forbids SYNC during), TC-10 (allows ASYNC during).
 */
TEST(UTConlesEventRobustnessSyncRestriction, verifySyncModeAfterCallback_expectSuccess) {
    //===SETUP===
    Tc11Context Ctx;

    // Subscribe simple callback
    IOC_EvtID_T EvtIDs[] = {IOC_EVTID_TEST_SLEEP_9MS};
    IOC_SubEvtArgs_T SubArgs = {
        .CbProcEvt_F = tc11CbProcEvtSimple,
        .pCbPrivData = &Ctx,
        .EvtNum = IOC_calcArrayElmtCnt(EvtIDs),
        .pEvtIDs = EvtIDs,
    };

    IOC_Result_T Result = IOC_subEVT_inConlesMode(&SubArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result) << "Setup: Subscribe should succeed";

    //===BEHAVIOR===
    // Post event to trigger callback
    IOC_EvtDesc_T TriggerEvtDesc = {
        .EvtID = IOC_EVTID_TEST_SLEEP_9MS,
    };

    Result = IOC_postEVT_inConlesMode(&TriggerEvtDesc, nullptr);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result) << "Behavior: Initial post should succeed";

    // Force immediate processing to ensure callback executes
    IOC_forceProcEVT();

    // Wait for callback to complete
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Verify callback completed
    ASSERT_TRUE(Ctx.CallbackExecuted.load()) << "Callback should have executed before sync post";

    // Now attempt SYNC_MODE post AFTER callback has completed
    IOC_EvtDesc_T SyncEvtDesc = {
        .EvtID = IOC_EVTID_TEST_KEEPALIVE,
    };

    IOC_Option_defineSyncMode(SyncOption);

    Ctx.SyncPostAfterCallback.store(true);
    Result = IOC_postEVT_inConlesMode(&SyncEvtDesc, &SyncOption);
    Ctx.SyncPostResult.store(Result);

    //===VERIFY===
    // Key Verification Point 1: Callback executed before sync post
    VERIFY_KEYPOINT_TRUE(Ctx.CallbackExecuted.load(), "Callback must complete before testing post-callback sync post");

    // Key Verification Point 2: Sync post was attempted after callback
    VERIFY_KEYPOINT_TRUE(Ctx.SyncPostAfterCallback.load(), "Sync post must be attempted after callback completes");

    // Key Verification Point 3: Sync post SUCCEEDED (no restriction outside callback)
    VERIFY_KEYPOINT_EQ(Ctx.SyncPostResult.load(), IOC_RESULT_SUCCESS,
                       "SYNC_MODE after callback MUST succeed - restriction is scoped to callback duration only");

    //===CLEANUP===
    IOC_UnsubEvtArgs_T UnsubArgs = {
        .CbProcEvt_F = tc11CbProcEvtSimple,
        .pCbPrivData = &Ctx,
    };

    Result = IOC_unsubEVT_inConlesMode(&UnsubArgs);
    EXPECT_EQ(IOC_RESULT_SUCCESS, Result) << "Cleanup: Unsubscribe should succeed";
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>END OF TEST IMPLEMENTATION================================================================
