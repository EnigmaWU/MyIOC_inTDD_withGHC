///////////////////////////////////////////////////////////////////////////////////////////////////
// UT_DataConcurrencyCallback.cxx - Advanced Callback Concurrency Testing
//
// PURPOSE:
//   Deep-dive testing of callback (CbRecvDat_F) concurrency scenarios for both FIFO and TCP.
//   Focuses on re-entrancy, deadlock prevention, and callback-initiated operations.
//
// CATDD METHODOLOGY:
//   This file follows Comment-alive Test-Driven Development (CaTDD):
//   - Phase 2: DESIGN - Comprehensive callback concurrency design
//   - Phase 3: IMPLEMENTATION - TDD Red→Green cycle
//
// PRIORITY CLASSIFICATION:
//   P2 → P1 (Promoted due to risk score 18)
//   Callback deadlocks are common, critical to prevent
//
// SCOPE:
//   Protocol-agnostic callback concurrency patterns applicable to both FIFO and TCP
//
// RELATIONSHIPS:
//   - Complements: UT_DataConcurrency.cxx (FIFO), UT_DataConcurrencyTCP.cxx (TCP)
//   - Focuses on: Advanced re-entrant callback patterns not covered in base files
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <atomic>
#include <chrono>
#include <functional>
#include <thread>
#include <vector>

#include "_UT_IOC_Common.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF OVERVIEW========================================================================
/**
 * @brief
 *   [WHAT] Advanced callback concurrency scenarios for Data API.
 *   [WHERE] CbRecvDat_F callback re-entrancy and deadlock testing.
 *   [WHY] Callback patterns are complex, prone to deadlocks if locks not carefully ordered.
 *
 * FOCUS AREAS:
 *   1. Re-entrant API calls from within CbRecvDat_F
 *   2. Nested callback chains (A→B→C→A)
 *   3. Callback modifying subscription state
 *   4. Callback calling APIs on same vs different LinkIDs
 *   5. Timeout interactions with callback execution
 *   6. Exception safety in callbacks under concurrency
 */
//======>END OF OVERVIEW==========================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF FREELY DRAFTED IDEAS=============================================================
/**
 * BRAINSTORMING: Callback concurrency edge cases
 * (CaTDD Step 2: Freely draft without constraints)
 *
 * What if scenarios for callback concurrency:
 *  • What if callback calls sendDAT and triggers another callback? → Recursion depth limit
 *  • What if callback takes 5 seconds, blocking other operations? → Timeout interaction
 *  • What if callback throws exception during concurrent sends? → State corruption risk
 *  • What if callback closes the LinkID it's executing on? → Self-destruction safety
 *  • What if callback A triggers callback B which triggers A? → Circular dependency
 *  • What if 10 callbacks fire simultaneously on same LinkID? → Callback serialization
 *  • What if callback allocates resources, then another callback OOMs? → Cleanup order
 *  • What if callback modifies global state read by other callbacks? → Race condition
 *  • What if callback calls IOC_flushDAT while data still arriving? → Flush semantics
 *  • What if callback unregisters itself during execution? → Self-removal safety
 *
 * Edge cases to explore:
 *  • Callback execution context: IOC thread vs user thread vs callback thread
 *  • Callback return value handling: Does error propagate? To whom?
 *  • Callback lifetime: Can callback outlive the LinkID?
 *  • Callback order: FIFO, LIFO, or undefined for concurrent triggers?
 *  • Callback atomicity: Can callback be interrupted mid-execution?
 *
 * Gotchas to verify:
 *  • Lock inversion: User callback → IOC lock → user callback (deadlock)
 *  • Stack overflow: Deep callback nesting
 *  • Resource leaks: Callback exception mid-operation
 *  • State machine corruption: Callback called during state transition
 *  • Priority inversion: High-priority thread blocked by callback
 */
//======>END OF FREELY DRAFTED IDEAS===============================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF DESIGN==========================================================================

/**************************************************************************************************
 * CALLBACK CONCURRENCY USER STORIES
 *
 *  US-CB1: AS a developer implementing request-response pattern,
 *          I WANT to call IOC_sendDAT from within CbRecvDat_F callback on SAME LinkID,
 *          SO THAT I can implement synchronous reply without deadlock.
 *
 *  US-CB2: AS a data router,
 *          I WANT to call IOC_sendDAT from callback on DIFFERENT LinkID (forwarding pattern),
 *          SO THAT I can route data without deadlock or blocking.
 *
 *  US-CB3: AS a dynamic system,
 *          I WANT to modify link state (close/reconnect) from within callback safely,
 *          SO THAT I can implement error recovery without deadlock.
 *
 *  US-CB4: AS a developer with nested data flows,
 *          I WANT nested callback chains to either work or fail gracefully,
 *          SO THAT my system doesn't infinite-loop or deadlock.
 *
 *  US-CB5: AS a callback implementor,
 *          I WANT callback exceptions to not corrupt IOC internal state under concurrency,
 *          SO THAT one callback failure doesn't break other threads.
 *
 *  US-CB6: AS a timeout-sensitive application,
 *          I WANT callback execution time to not interfere with send/recv timeouts,
 *          SO THAT slow callbacks don't cause unexpected timeout errors.
 *************************************************************************************************/

/**************************************************************************************************
 * ACCEPTANCE CRITERIA
 *
 * [@US-CB1] Same-link callback send
 *  AC-CB1: GIVEN CbRecvDat_F calls IOC_sendDAT on same LinkID (echo pattern),
 *           WHEN multiple threads trigger callbacks concurrently,
 *           THEN no deadlock occurs,
 *            AND either sends succeed or proper re-entrancy error,
 *            AND system remains responsive.
 *
 * [@US-CB2] Cross-link callback send
 *  AC-CB2: GIVEN CbRecvDat_F calls IOC_sendDAT on different LinkID (routing pattern),
 *           WHEN A→B and B→A both active,
 *           THEN no circular deadlock,
 *            AND data flows bidirectionally,
 *            AND proper lock ordering maintained.
 *
 * [@US-CB3] Callback-initiated state changes
 *  AC-CB3: GIVEN CbRecvDat_F calls IOC_disconnectService on its own link,
 *           WHEN callback executing,
 *           THEN either deferred disconnect or immediate with proper cleanup,
 *            AND no use-after-free,
 *            AND callback completes safely.
 *
 * [@US-CB4] Nested callback chains
 *  AC-CB4: GIVEN callbacks form chain A→B→C,
 *           WHEN chain depth exceeds threshold (e.g., 10),
 *           THEN system either detects recursion or allows with stack safety,
 *            AND no infinite loop,
 *            AND proper termination.
 *
 * [@US-CB5] Exception safety
 *  AC-CB5: GIVEN CbRecvDat_F throws exception during concurrent operation,
 *           WHEN multiple callbacks executing,
 *           THEN exception isolated to failing callback's thread,
 *            AND IOC internal state consistent,
 *            AND other callbacks continue processing.
 *
 * [@US-CB6] Timeout independence
 *  AC-CB6: GIVEN CbRecvDat_F executes for 1 second,
 *           WHEN another thread calls IOC_sendDAT with 100ms timeout,
 *           THEN send timeout independent of callback duration,
 *            AND send succeeds/times-out based on send operation only,
 *            AND callback continues unaffected.
 *************************************************************************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF FAST-FAIL CALLBACK SIX===========================================================
/**
 * CALLBACK-SPECIFIC FAST-FAIL SIX (run before full callback suite)
 *
 * FAST-FAIL CALLBACK-SIX:
 *
 * 1. **Callback No-Op Baseline**: Verify callback infrastructure works
 *    - Test: Callback that does nothing, just returns success
 *    - Purpose: Prove callback registration and invocation functional
 *    - Fail indicator: Callback not called, or IOC error
 *
 * 2. **Simple Echo No Deadlock**: Minimal same-link send
 *    - Test: Callback sends tiny response on same LinkID, single thread
 *    - Purpose: Catch obvious same-link deadlock
 *    - Fail indicator: Test hangs
 *
 * 3. **Callback Exception Handled**: Exception doesn't crash
 *    - Test: Callback throws std::exception
 *    - Purpose: Verify exception boundary protection
 *    - Fail indicator: Process crash, IOC state corruption
 *
 * 4. **Cross-Link Simple Route**: A→B routing baseline
 *    - Test: Link A callback sends to Link B (not back to A)
 *    - Purpose: Prove cross-link send works from callback
 *    - Fail indicator: Deadlock, send fails
 *
 * 5. **Callback Timeout Smoke**: Fast callback vs slow send
 *    - Test: Callback completes in 10ms, send has 1s timeout
 *    - Purpose: Verify timeouts don't interact incorrectly
 *    - Fail indicator: Premature timeout, callback blocked
 *
 * 6. **Callback Concurrency Baseline**: 2 callbacks concurrent
 *    - Test: 2 threads trigger callbacks simultaneously
 *    - Purpose: Detect basic callback serialization issues
 *    - Fail indicator: Race condition, data corruption
 */
//======>END OF FAST-FAIL CALLBACK SIX=============================================================

/**************************************************************************************************
 * TEST CASES
 *
 *  ⚪ TC-CB1: verifyCallbackSameLink_byEchoPattern_expectNoDeadlock
 *      @[Purpose]: Critical echo pattern (receive→send same link)
 *      @[Brief]: Callback sends reply on same LinkID, verify no deadlock
 *
 *  ⚪ TC-CB2: verifyCallbackCrossLink_byBidirectionalRouting_expectNoCircularDeadlock
 *      @[Purpose]: A↔B routing pattern deadlock prevention
 *      @[Brief]: Link A callback sends to B, B callback sends to A
 *
 *  ⚪ TC-CB3: verifyCallbackDisconnect_byCloseDuringSend_expectDeferredCleanup
 *      @[Purpose]: Callback-initiated disconnect safety
 *      @[Brief]: Callback calls IOC_disconnectService, verify safe cleanup
 *
 *  ⚪ TC-CB4: verifyCallbackNesting_byChainDepth10_expectStackSafe
 *      @[Purpose]: Deep nested callback chain handling
 *      @[Brief]: A→B→C→...→J chain, verify stack safety
 *
 *  ⚪ TC-CB5: verifyCallbackException_byConcurrentThrows_expectIsolation
 *      @[Purpose]: Exception safety in callbacks
 *      @[Brief]: Callback throws, verify IOC state consistent
 *
 *  ⚪ TC-CB6: verifyCallbackTimeout_bySlowCallbackFastSend_expectIndependent
 *      @[Purpose]: Callback duration vs timeout independence
 *      @[Brief]: Slow callback (1s) doesn't affect send timeout (100ms)
 *************************************************************************************************/

//======>END OF DESIGN============================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF IMPLEMENTATION TRACKING=========================================================
//
//=================================================================================================
// 🥇 CRITICAL - Must implement before release
//=================================================================================================
//   ⚪ [@AC-CB1,US-CB1] TC-CB1: verifyCallbackSameLink_byEchoPattern_expectNoDeadlock – MOST CRITICAL
//   ⚪ [@AC-CB2,US-CB2] TC-CB2: verifyCallbackCrossLink_byBidirectionalRouting_expectNoCircularDeadlock
//
//=================================================================================================
// 🥈 IMPORTANT - Quality assurance
//=================================================================================================
//   ⚪ [@AC-CB3,US-CB3] TC-CB3: verifyCallbackDisconnect_byCloseDuringSend_expectDeferredCleanup
//   ⚪ [@AC-CB5,US-CB5] TC-CB5: verifyCallbackException_byConcurrentThrows_expectIsolation
//
//=================================================================================================
// 🥉 NICE-TO-HAVE - Edge cases
//=================================================================================================
//   ⚪ [@AC-CB4,US-CB4] TC-CB4: verifyCallbackNesting_byChainDepth10_expectStackSafe
//   ⚪ [@AC-CB6,US-CB6] TC-CB6: verifyCallbackTimeout_bySlowCallbackFastSend_expectIndependent
//
///////////////////////////////////////////////////////////////////////////////////////////////////
//======>END OF IMPLEMENTATION TRACKING===========================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF TEST HELPER UTILITIES============================================================
/**
 * CALLBACK CONCURRENCY TEST INFRASTRUCTURE:
 *
 * Callback-Specific Utilities:
 *  • EchoCallbackContext: Context for same-link echo pattern (TC-CB1)
 *    - ResponseLinkID: Where to send reply
 *    - EchoCount: Number of echo operations performed
 *    - DeadlockDetected: Timeout flag
 *
 *  • RoutingCallbackContext: Context for cross-link routing (TC-CB2)
 *    - TargetLinkID: Destination for forwarded data
 *    - RoutedCount: Number of routing operations
 *    - CircularDetected: Flag for A→B→A cycles
 *
 *  • NestingDepthTracker: Track callback chain depth (TC-CB4)
 *    - thread_local CurrentDepth
 *    - MaxDepthAllowed (e.g., 10)
 *    - OverflowDetected flag
 *
 *  • ExceptionSafetyContext: Exception testing context (TC-CB5)
 *    - ExceptionThrown: Atomic bool
 *    - IOCStateValid: Post-exception state check
 *    - ConcurrentCallbacksCompleted: Count of successful callbacks
 *
 * Callback Functions:
 *  • EchoCbRecvDat(): Callback that sends on same LinkID (echo pattern)
 *  • RoutingCbRecvDat(): Callback that forwards to different LinkID
 *  • ExceptionThrowingCbRecvDat(): Callback that throws for testing
 *  • SlowCbRecvDat(): Callback with configurable delay (timeout tests)
 *
 * Synchronization Helpers:
 *  • CallbackBarrier: Wait for N callbacks to be ready
 *  • DeadlockTimer: Timeout detection for callback tests
 *
 * Future Utilities (TODO):
 *  • CallbackStackProfiler: Measure stack usage in nested callbacks
 *  • CallbackTimelineRecorder: Track callback execution order
 */
//======>END OF TEST HELPER UTILITIES==============================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF IMPLEMENTATION==================================================================

namespace {

// Callback context for echo pattern (same-link send)
struct EchoCallbackContext {
    IOC_LinkID_T LinkID;
    std::atomic<uint32_t> EchoCount{0};
    std::atomic<uint32_t> DeadlockDetected{0};
    std::atomic<bool> Running{true};
};

// Echo callback - sends reply on same link
IOC_Result_T EchoCbRecvDat(const IOC_DatDesc_pT pDatDesc, void* pCbPrivData) {
    auto* pCtx = static_cast<EchoCallbackContext*>(pCbPrivData);

    // CRITICAL: Call IOC_sendDAT on SAME LinkID
    // This is the classic deadlock scenario if locks not ordered correctly
    IOC_DatDesc_T Reply = *pDatDesc;  // Echo back same data
    IOC_Result_T Result = IOC_sendDAT(pCtx->LinkID, &Reply, nullptr);

    if (Result == IOC_RESULT_SUCCESS) {
        pCtx->EchoCount.fetch_add(1);
    } else {
        pCtx->DeadlockDetected.fetch_add(1);
    }

    return IOC_RESULT_SUCCESS;
}

}  // namespace

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF: [@AC-CB1,US-CB1] Echo Pattern Deadlock Test===================================

/**
 * @[Name]: verifyCallbackSameLink_byEchoPattern_expectNoDeadlock
 * @[Purpose]: MOST CRITICAL callback test - echo pattern on same link
 * @[Steps]:
 *   1) 🔧 SETUP: Create bi-directional link with echo callback
 *   2) 🎯 BEHAVIOR: Send data that triggers callback to send on same link
 *   3) ✅ VERIFY: No deadlock (test completes within timeout)
 *   4) ✅ VERIFY: Echo succeeds OR proper re-entrancy error
 *   5) 🧹 CLEANUP: Close link
 * @[Expect]: No deadlock, test completes
 * @[Risk]: CRITICAL - Common pattern, must not deadlock
 */
TEST(UT_DataConcurrencyCallback, verifyCallbackSameLink_byEchoPattern_expectNoDeadlock) {
    //===SETUP===
    printf("🔧 SETUP: CRITICAL echo pattern - callback sends on same LinkID\n");

    // TODO: Implement MOST CRITICAL callback test
    // 1. Create bidirectional data link (both sides send/receive)
    // 2. Register callback that calls IOC_sendDAT on same LinkID
    // 3. Send initial data to trigger callback
    // 4. Use timeout (5 seconds) to detect deadlock
    // 5. Verify callback completes without deadlock

    GTEST_SKIP() << "⚪ TODO: Implement MOST CRITICAL callback echo deadlock test";
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF: [@AC-CB2,US-CB2] Bidirectional Routing Test====================================

TEST(UT_DataConcurrencyCallback, verifyCallbackCrossLink_byBidirectionalRouting_expectNoCircularDeadlock) {
    //===SETUP===
    printf("🔧 SETUP: Bidirectional routing - A→B and B→A callbacks\n");

    // TODO: Implement bidirectional routing deadlock test
    // 1. Create two links: A and B
    // 2. Link A callback sends to Link B
    // 3. Link B callback sends to Link A
    // 4. Trigger initial send to start ping-pong
    // 5. Verify no circular deadlock with proper lock ordering

    GTEST_SKIP() << "⚪ TODO: Implement bidirectional routing deadlock test";
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF: Additional Callback Tests=====================================================

TEST(UT_DataConcurrencyCallback, verifyCallbackDisconnect_byCloseDuringSend_expectDeferredCleanup) {
    GTEST_SKIP() << "⚪ TODO: Implement callback-initiated disconnect test";
}

TEST(UT_DataConcurrencyCallback, verifyCallbackNesting_byChainDepth10_expectStackSafe) {
    GTEST_SKIP() << "⚪ TODO: Implement nested callback chain test";
}

TEST(UT_DataConcurrencyCallback, verifyCallbackException_byConcurrentThrows_expectIsolation) {
    GTEST_SKIP() << "⚪ TODO: Implement callback exception safety test";
}

TEST(UT_DataConcurrencyCallback, verifyCallbackTimeout_bySlowCallbackFastSend_expectIndependent) {
    GTEST_SKIP() << "⚪ TODO: Implement callback vs timeout independence test";
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>END OF IMPLEMENTATION====================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF TODO/IMPLEMENTATION TRACKING SECTION=============================================
// 🔴 CALLBACK CONCURRENCY IMPLEMENTATION STATUS - TDD Red→Green Methodology
//
// PURPOSE:
//   Track advanced callback concurrency test implementation.
//   Focus on re-entrancy, deadlock prevention, and callback safety patterns.
//
// STATUS LEGEND:
//   ⚪ TODO/PLANNED:      Designed but not implemented yet
//   🔴 RED/FAILING:       Test written, failing (need production code fix)
//   🟢 GREEN/PASSED:      Test written and passing
//   ⚠️  ISSUES:           Known problem needing attention
//   🚫 BLOCKED:          Cannot proceed due to dependency
//
// PRIORITY LEVELS:
//   🥇 CRITICAL:  Deadlock risk, common usage patterns (production blockers)
//   🥈 IMPORTANT: Safety properties, error handling (quality assurance)
//   🥉 NICE-TO-HAVE: Edge cases, advanced patterns (comprehensive coverage)
//
// WORKFLOW:
//   1. Implement Fast-Fail Callback-Six first (callback smoke tests)
//   2. Complete CRITICAL priority (TC-CB1, TC-CB2)
//   3. Move to IMPORTANT priority (TC-CB3, TC-CB5)
//   4. Add NICE-TO-HAVE tests (TC-CB4, TC-CB6)
//   5. Mark status: ⚪ TODO → 🔴 RED → 🟢 GREEN
//
//===================================================================================================
// 🎯 FAST-FAIL CALLBACK-SIX - Callback Smoke Tests (Run First)
//===================================================================================================
//   ⚪ FF-CB-1: Callback No-Op Baseline
//        - Description: Callback does nothing, just returns success
//        - Category: Smoke Test
//        - Estimated effort: 30 min
//        - Depends on: None
//        - Verification: Callback registered and invoked
//
//   ⚪ FF-CB-2: Simple Echo No Deadlock
//        - Description: Callback sends tiny response on same LinkID, single thread
//        - Category: Smoke Test (deadlock baseline)
//        - Estimated effort: 1 hour
//        - Depends on: FF-CB-1 GREEN
//        - Verification: Test completes (no hang)
//
//   ⚪ FF-CB-3: Callback Exception Handled
//        - Description: Callback throws std::exception
//        - Category: Smoke Test (exception boundary)
//        - Estimated effort: 45 min
//        - Depends on: FF-CB-1 GREEN
//        - Verification: Process doesn't crash, IOC state valid
//
//   ⚪ FF-CB-4: Cross-Link Simple Route
//        - Description: Link A callback sends to Link B (not back to A)
//        - Category: Smoke Test
//        - Estimated effort: 1 hour
//        - Depends on: FF-CB-1 GREEN
//        - Verification: Cross-link send works, no deadlock
//
//   ⚪ FF-CB-5: Callback Timeout Smoke
//        - Description: Fast callback (10ms) vs slow send (1s timeout)
//        - Category: Smoke Test
//        - Estimated effort: 45 min
//        - Depends on: FF-CB-1 GREEN
//        - Verification: Timeout independent of callback duration
//
//   ⚪ FF-CB-6: Callback Concurrency Baseline
//        - Description: 2 threads trigger callbacks simultaneously
//        - Category: Smoke Test
//        - Estimated effort: 1 hour
//        - Depends on: FF-CB-1 GREEN
//        - Verification: Callbacks serialize or run safely concurrent
//
// 🚪 GATE: Fast-Fail Callback-Six must be GREEN before main callback tests
//
//===================================================================================================
// 🥇 CRITICAL PRIORITY - Deadlock Risk & Common Patterns (Production Blockers)
//===================================================================================================
//   ⚪ [@AC-1,US-CB1] TC-CB1: verifyCallbackSameLink_byEchoPattern_expectNoDeadlock
//        - Description: MOST CRITICAL - Echo pattern (callback sends on same LinkID)
//        - Category: Callback re-entrancy (deadlock prevention)
//        - Priority: MOST CRITICAL
//        - Estimated effort: 6 hours
//        - Depends on: FF-CB-2, FF-CB-6 GREEN
//        - Tools: DeadlockDetector (5s timeout), re-entrancy depth tracker
//        - Verification: Completes within 5s, no deadlock, proper error or success
//        - Risk: HIGHEST - Common usage pattern, production blocker if deadlock
//        - Implementation notes:
//          * Release link lock before invoking callback
//          * Detect re-entrancy: thread_local recursion depth counter
//          * Return IOC_RESULT_REENTRANT_CALL or allow with lock release
//          * Two-phase execution: prepare, unlock, execute callback, relock
//
//   ⚪ [@AC-CB2,US-CB2] TC-CB2: verifyCallbackCrossLink_byBidirectionalRouting_expectNoCircularDeadlock
//        - Description: Bidirectional routing (A→B and B→A)
//        - Category: Callback routing (circular deadlock prevention)
//        - Priority: CRITICAL
//        - Estimated effort: 5 hours
//        - Depends on: TC-CB1 GREEN, FF-CB-4 GREEN
//        - Tools: Lock ordering validator, circular dependency detector
//        - Verification: Bidirectional flow works, no circular deadlock
//        - Implementation notes:
//          * Always acquire LinkID locks in ascending order
//          * Use try_lock with backoff for circular cases
//          * Ping-pong termination mechanism (hop count limit)
//
// 🚪 GATE: CRITICAL tests GREEN required for production deployment
//
//===================================================================================================
// 🥈 IMPORTANT PRIORITY - Safety Properties & Error Handling (Quality Assurance)
//===================================================================================================
//   ⚪ [@AC-CB3,US-CB3] TC-CB3: verifyCallbackDisconnect_byCloseDuringSend_expectDeferredCleanup
//        - Description: Callback closes its own LinkID
//        - Category: Self-destruction safety
//        - Estimated effort: 4 hours
//        - Depends on: TC-CB1 GREEN
//        - Tools: Resource leak detector, use-after-free sanitizer
//        - Verification: Deferred disconnect or immediate with safe cleanup, no UAF
//        - Implementation notes:
//          * Deferred cleanup: mark for deletion, process after callback returns
//          * Reference counting on LinkID context
//          * Callback completion signaling before resource release
//
//   ⚪ [@AC-1,US-CB5] TC-CB5: verifyCallbackException_byConcurrentThrows_expectIsolation
//        - Description: Exception in callback under concurrency
//        - Category: Exception safety
//        - Estimated effort: 4 hours
//        - Depends on: FF-CB-3, FF-CB-6 GREEN
//        - Tools: Exception boundary monitor, state consistency checker
//        - Verification: Exception isolated, IOC state consistent, other callbacks continue
//        - Implementation notes:
//          * Wrap callback invocation in try-catch
//          * Restore IOC internal state on exception
//          * Log exception, don't propagate to other threads
//          * Resource cleanup (RAII guards)
//
//   ⚪ [@AC-1,US-CB6] TC-CB6: verifyCallbackTimeout_bySlowCallbackFastSend_expectIndependent
//        - Description: Callback duration vs operation timeout independence
//        - Category: Timeout semantics
//        - Estimated effort: 3 hours
//        - Depends on: FF-CB-5 GREEN
//        - Tools: Timeout precision monitor, timing analyzer
//        - Verification: Send timeout based on send operation, not callback duration
//        - Implementation notes:
//          * Separate timers: callback execution vs API operation
//          * Callback timeout (if any) independent of send/recv timeout
//          * Callback blocking doesn't extend operation timeout
//
// 🚪 GATE: IMPORTANT tests GREEN for production-grade callback handling
//
//===================================================================================================
// 🥉 NICE-TO-HAVE PRIORITY - Edge Cases & Advanced Patterns
//===================================================================================================
//   ⚪ [@AC-1,US-CB4] TC-CB4: verifyCallbackNesting_byChainDepth10_expectStackSafe
//        - Description: Deep nested callback chains (A→B→C→...→J)
//        - Category: Callback nesting (stack safety)
//        - Estimated effort: 4 hours
//        - Depends on: TC-CB2 GREEN
//        - Tools: Stack depth profiler, overflow detector
//        - Verification: System allows depth or detects recursion, no stack overflow
//        - Implementation notes:
//          * thread_local recursion depth counter (e.g., max 10)
//          * Return IOC_RESULT_TOO_DEEP_RECURSION beyond limit
//          * OR allow with stack usage monitoring
//          * Infinite loop prevention (hop count header)
//
//===================================================================================================
// 📊 PROGRESS SUMMARY
//===================================================================================================
// Fast-Fail CB-Six:   0/6  GREEN (⚪⚪⚪⚪⚪⚪)
// CRITICAL Priority:  0/2  GREEN (⚪⚪)
// IMPORTANT Priority: 0/3  GREEN (⚪⚪⚪)
// NICE-TO-HAVE:       0/1  GREEN (⚪)
// Total CB Tests:     0/12 GREEN
//
// Next Action: Implement Fast-Fail Callback-Six → TC-CB1 (MOST CRITICAL echo pattern)
//
//===================================================================================================
// 🛠️ CALLBACK IMPLEMENTATION ROADMAP (3-Week Plan)
//===================================================================================================
// Week 1: Callback Infrastructure + Fast-Fail Callback-Six
//   - Days 1-2: Callback test infrastructure (contexts, timers, deadlock detector)
//   - Days 3-5: Implement & validate Fast-Fail Callback-Six
//   - Goal: All callback smoke tests GREEN
//
// Week 2: CRITICAL Priority Tests (TC-CB1, TC-CB2)
//   - Days 1-3: TC-CB1 echo pattern deadlock (MOST CRITICAL)
//   - Days 4-5: TC-CB2 bidirectional routing
//   - Goal: Production blockers resolved, common patterns safe
//
// Week 3: IMPORTANT & NICE-TO-HAVE Tests
//   - Days 1-2: TC-CB3 self-destruction + TC-CB5 exception safety
//   - Day 3: TC-CB6 timeout independence
//   - Days 4-5: TC-CB4 deep nesting + integration testing
//   - Goal: Complete callback concurrency coverage
//
//===================================================================================================
// 🔧 CALLBACK-SPECIFIC IMPLEMENTATION NOTES
//===================================================================================================
// 1. Lock Ordering Rules (TC-CB2):
//    ✓ Always acquire LinkID locks in ascending order: Lock(min(A,B)), Lock(max(A,B))
//    ✓ Release link lock BEFORE invoking callback
//    ✓ Re-acquire if callback calls back into IOC
//    ✓ Use try_lock with backoff for potential circular dependencies
//
// 2. Re-entrancy Detection (TC-CB1):
//    ✓ thread_local recursion depth counter
//    ✓ Increment on callback entry, decrement on exit
//    ✓ Return IOC_RESULT_REENTRANT_CALL if depth > threshold (e.g., 5)
//    ✓ OR allow re-entrancy with proper lock release (preferred for echo pattern)
//
// 3. Exception Safety (TC-CB5):
//    ✓ Wrap callback invocation: try { callback(...); } catch(...) { restore_state(); }
//    ✓ Use RAII guards for resource cleanup (lock guards, memory guards)
//    ✓ Log exception to user (IOC_Log if available)
//    ✓ Verify IOC internal state consistent after exception
//
// 4. Deadlock Prevention Patterns (TC-CB1, TC-CB2):
//    ✓ Two-phase callback execution:
//      - Phase 1: Prepare callback context, collect data
//      - Phase 2: Release lock, execute callback, re-acquire if needed
//    ✓ Callback context must NOT hold IOC locks
//    ✓ Use condition variables for callback completion signaling
//    ✓ Timeout-based deadlock detector (5-second max for tests)
//
// 5. Testing Strategy:
//    ✓ Deadlock detector with timeout + thread dump
//    ✓ ThreadSanitizer for lock ordering validation
//    ✓ AddressSanitizer for use-after-free in TC-CB3
//    ✓ Stress test: rapid callback triggering (1000/sec)
//    ✓ Callback execution time profiling
//
// 6. Lock-Free Alternative (Future Consideration):
//    ✓ Use lock-free queue for callback dispatch
//    ✓ Dedicated callback worker threads
//    ✓ Async callback execution model
//    ✓ Trade-off: Complexity vs performance
//
//===================================================================================================
// ✅ COMPLETED TESTS (for reference, remove after stable)
//===================================================================================================
// (None yet - all callback tests in TODO state)
//
//======>END OF TODO/IMPLEMENTATION TRACKING SECTION===============================================
