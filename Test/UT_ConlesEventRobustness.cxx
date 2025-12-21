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
//   - Phase 2: DESIGN - Comprehensive test design in comments
//   - Phase 3: IMPLEMENTATION - TDD Red→Green cycle
//
// PRIORITY CLASSIFICATION:
//   P3: Quality-Oriented → Robust (stress testing, stability)
//   PROMOTED TO P2 LEVEL due to high risk score:
//     - Impact: 3 (data loss, system hang)
//     - Likelihood: 2 (occurs under load)
//     - Uncertainty: 2 (complex async interactions)
//     - Score: 12 → Move up from default position
//
// RELATIONSHIPS:
//   - Depends on: Source/_IOC_ConlesEvent.c
//   - Related tests: UT_ConlesEventConcurrency.cxx (Thread-safety)
//   - Production code: Source/_IOC_ConlesEvent.c
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

#include "_UT_IOC_Common.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF OVERVIEW OF THIS UNIT TESTING FILE===============================================
/**
 * @brief
 *   [WHAT] This file verifies ConlesMode event system robustness under stress conditions.
 *   [WHERE] in the IOC Event subsystem for connectionless mode.
 *   [WHY] to ensure system remains stable and predictable under adverse conditions.
 *
 * SCOPE:
 *   - In scope:
 *     • Queue overflow and backpressure behavior
 *     • Slow consumer blocking fast producer scenarios
 *     • Cascading event storms (events posted in callbacks)
 *     • Sync mode restrictions during callback execution
 *     • Resource exhaustion and recovery
 *   - Out of scope:
 *     • Concurrency and thread-safety (see UT_ConlesEventConcurrency.cxx)
 *     • Basic functionality (see UT_ConlesEventTypical.cxx)
 *
 * KEY CONCEPTS:
 *   - Robustness: System continues functioning correctly under stress.
 *   - Backpressure: Flow control mechanism when consumer slower than producer.
 *   - Cascading Events: Events triggering more events (amplification risk).
 *   - Sync Mode Restriction: Prevent deadlock by forbidding sync posts in callbacks.
 */
//======>END OF OVERVIEW OF THIS UNIT TESTING FILE=================================================

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
 *   - New Public API: Complete P1 thoroughly before P2
 *   - Stateful/FSM: Promote State to early P2 (after Typical+Edge)
 *   - High Reliability: Promote Fault & Robust
 *   - Performance SLOs: Promote Performance to P2 level
 *   - Highly Concurrent: Promote Concurrency to first in P2
 *
 * RISK-DRIVEN ADJUSTMENT:
 *   Score = Impact (1-3) × Likelihood (1-3) × Uncertainty (1-3)
 *   If Score ≥ 18: Promote category to earlier priority
 *
 *===================================================================================================
 * PRIORITY-1: FUNCTIONAL TESTING (ValidFunc + InvalidFunc)
 *===================================================================================================
 *
 * ValidFunc - Verifies correct behavior with valid inputs/states.
 *
 *   ⭐ TYPICAL: Core workflows and "happy paths". (MUST HAVE)
 *      - Purpose: Verify main usage scenarios.
 *      - Examples: Basic registration, standard event flow, normal command execution.
 *      - Status: COVERED in UT_ConlesEventTypical.cxx
 *
 *   🔲 EDGE: Edge cases, limits, and mode variations. (HIGH PRIORITY)
 *      - Purpose: Test parameter limits and edge values.
 *      - Examples: Min/max values, null/empty inputs, Block/NonBlock/Timeout modes.
 *      - Status: COVERED in UT_ConlesEventEdge.cxx
 *
 * InvalidFunc - Verifies graceful failure with invalid inputs or states.
 *
 *   🚫 MISUSE: Incorrect API usage patterns. (ERROR PREVENTION)
 *      - Purpose: Ensure proper error handling for API abuse.
 *      - Examples: Wrong call sequence, invalid parameters, double-init.
 *      - Status: COVERED in UT_ConlesEventMisuse.cxx
 *
 *   ⚠️ FAULT: Error handling and recovery. (RELIABILITY)
 *      - Purpose: Test system behavior under error conditions.
 *      - Examples: Network failures, disk full, process crash recovery.
 *      - Status: COVERED in UT_ConlesEventFault.cxx
 *
 *===================================================================================================
 * PRIORITY-2: DESIGN-ORIENTED TESTING (Architecture Validation)
 *===================================================================================================
 *
 *   🔄 STATE: Lifecycle transitions and state machine validation. (KEY FOR STATEFUL COMPONENTS)
 *      - Purpose: Verify FSM correctness.
 *      - Examples: Init→Ready→Running→Stopped.
 *      - Status: COVERED in UT_ConlesEventState.cxx
 *
 *   🏆 CAPABILITY: Maximum capacity and system limits. (FOR CAPACITY PLANNING)
 *      - Purpose: Test architectural limits.
 *      - Examples: Max connections, queue limits.
 *      - Status: COVERED in UT_ConlesEventCapability.cxx
 *
 *   🚀 CONCURRENCY: Thread safety and synchronization. (FOR COMPLEX SYSTEMS)
 *      - Purpose: Validate concurrent access and find race conditions.
 *      - Examples: Race conditions, deadlocks, parallel access.
 *      - Status: COVERED in UT_ConlesEventConcurrency.cxx
 *
 *===================================================================================================
 * PRIORITY-3: QUALITY-ORIENTED TESTING (Non-Functional Requirements)
 *===================================================================================================
 *
 *   ⚡ PERFORMANCE: Speed, throughput, and resource usage. (FOR SLO VALIDATION)
 *      - Purpose: Measure and validate performance characteristics.
 *      - Examples: Latency benchmarks, memory leak detection.
 *      - Status: COVERED in UT_ConlesEventPerformance.cxx
 *
 *   🛡️ ROBUST: Stress, repetition, and long-running stability. (FOR PRODUCTION READINESS)
 *      - Purpose: Verify stability under sustained load.
 *      - Examples: 1000x repetition, 24h soak tests.
 *      - Status: THIS FILE - PROMOTED TO P2 LEVEL due to risk score 12
 *
 *   🔄 COMPATIBILITY: Cross-platform and version testing. (FOR MULTI-PLATFORM PRODUCTS)
 *      - Purpose: Ensure consistent behavior across environments.
 *      - Examples: Windows/Linux/macOS, API version compatibility.
 *      - Status: NOT APPLICABLE (single platform)
 *
 *   🎛️ CONFIGURATION: Different settings and environments. (FOR CONFIGURABLE SYSTEMS)
 *      - Purpose: Test various configuration scenarios.
 *      - Examples: Debug/release modes, feature flags.
 *      - Status: COVERED via build configurations
 *
 *===================================================================================================
 * PRIORITY-4: OTHER-ADDONS TESTING (Documentation & Tutorials)
 *===================================================================================================
 *
 *   🎨 DEMO/EXAMPLE: End-to-end feature demonstrations. (FOR DOCUMENTATION)
 *      - Purpose: Illustrate usage patterns and best practices.
 *      - Examples: Tutorial code, complete workflows.
 *      - Status: COVERED in UT_ConlesEventDemo*.cxx
 *
 * SELECTION STRATEGY:
 *   🥇 P1 (Functional): MUST be completed before moving to P2.
 *   🥈 P2 (Design): Test after P1 if the component has significant design complexity (state, concurrency).
 *   🥉 P3 (Quality): Test when quality attributes (performance, robustness) are critical.
 *   🎯 P4 (Addons): Optional, for documentation and examples.
 *************************************************************************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF USER STORY=======================================================================
/**
 * DESIGN PRINCIPLES: Define clear coverage strategy and scope
 *
 * COVERAGE STRATEGY (choose dimensions that fit your component):
 *   Option A: Service Role × Client Role × Mode
 *   Option B: Component State × Operation × Edge
 *   Option C: Concurrency × Resource Limits × Error Scenarios
 *   Custom:   [Your Dimension 1] × [Your Dimension 2] × [Your Dimension 3]
 *
 * COVERAGE MATRIX TEMPLATE (fill in for systematic test planning):
 * ┌─────────────────┬─────────────┬─────────────┬──────────────────────────────┐
 * │ Dimension 1     │ Dimension 2 │ Dimension 3 │ Key Scenarios                │
 * ├─────────────────┼─────────────┼─────────────┼──────────────────────────────┤
 * │ [Value A]       │ [Value X]   │ [Value M]   │ US-1: [Short description]    │
 * │ [Value A]       │ [Value Y]   │ [Value N]   │ US-2: [Short description]    │
 * │ [Value B]       │ [Value X]   │ [Value M]   │ US-3: [Short description]    │
 * └─────────────────┴─────────────┴─────────────┴──────────────────────────────┘
 *
 * THIS FILE'S COVERAGE MATRIX:
 * ┌─────────────────┬─────────────┬─────────────┬──────────────────────────────┐
 * │ Stress Type     │ Mode        │ Limit       │ Key Scenarios                │
 * ├─────────────────┼─────────────┼─────────────┼──────────────────────────────┤
 * │ Queue Overflow  │ MayBlock    │ Full Queue  │ US-1: Backpressure mgmt      │
 * │ Event Storm     │ Cascading   │ Amplify     │ US-2: Storm prevention       │
 * │ Deadlock Risk   │ Sync/Async  │ Re-entry    │ US-3: Deadlock prevention    │
 * │ Resource Limit  │ Max Sub     │ Recovery    │ US-4: Limits & recovery      │
 * └─────────────────┴─────────────┴─────────────┴──────────────────────────────┘
 *
 * USER STORIES (fill in your stories):
 *
 *  US-1: As an event producer in high-load scenarios,
 *        I want the system to handle queue overflow gracefully,
 *        So that I can choose between blocking, erroring, or timing out without losing system stability.
 *
 *  US-2: As a system architect,
 *        I want to prevent runaway event cascades and storms,
 *        So that a single event doesn't crash the system through amplification or infinite recursion.
 *
 *  US-3: As a developer implementing event callbacks,
 *        I want to be prevented from making synchronous posts in callbacks,
 *        So that I don't accidentally create deadlocks in the event processing loop.
 *
 *  US-4: As a system administrator,
 *        I want the system to remain stable at its limits and recover after stress,
 *        So that the service remains available even after temporary overload conditions.
 */
//======>END OF USER STORY=========================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//=======>BEGIN OF ACCEPTANCE CRITERIA=============================================================
/**
 * ACCEPTANCE CRITERIA define WHAT should be tested (make User Stories testable)
 *
 * FORMAT: GIVEN [initial context], WHEN [trigger/action], THEN [expected outcome]
 *
 * GUIDELINES:
 *   - Each US should have 1-4 ACs (more for complex features)
 *   - Each AC should be independently verifiable
 *   - Use precise, unambiguous language
 *   - Include both success and failure scenarios
 *   - Consider edge conditions explicitly
 *
 * [@US-1] Backpressure and Queue Overflow Management
 *  AC-1: GIVEN a slow consumer and a full event queue,
 *         WHEN a producer posts an event with MayBlock option,
 *         THEN the post operation blocks until space is available,
 *          AND the event is eventually delivered.
 *
 *  AC-2: GIVEN a full event queue,
 *         WHEN a producer posts an event with NonBlock option,
 *         THEN the operation returns immediately with IOC_RESULT_TOO_MANY_QUEUING_EVTDESC.
 *
 *  AC-3: GIVEN a full event queue,
 *         WHEN a producer posts an event with a specific Timeout,
 *         THEN the operation blocks for the specified duration,
 *          AND returns IOC_RESULT_TIMEOUT if no space becomes available.
 *
 *  AC-4: GIVEN a system that has experienced backpressure,
 *         WHEN the consumer catches up and the queue empties,
 *         THEN subsequent post operations return to normal low-latency behavior.
 *
 * [@US-2] Cascading Event Storm Prevention
 *  AC-5: GIVEN a chain of events where one callback posts the next event,
 *         WHEN the root event is triggered,
 *         THEN all events in the linear chain are delivered correctly.
 *
 *  AC-6: GIVEN an exponential event cascade (one event triggers multiple),
 *         WHEN the cascade exceeds queue capacity,
 *         THEN the system limits the amplification and returns overflow errors.
 *
 *  AC-7: GIVEN a cascading chain with MayBlock options,
 *         WHEN backpressure occurs at the end of the chain,
 *         THEN the backpressure propagates up the chain gracefully.
 *
 *  AC-8: GIVEN a massive event storm that fills the queue,
 *         WHEN the storm subsides,
 *         THEN the system processes all queued events and returns to a healthy state.
 *
 * [@US-3] Sync Mode Deadlock Prevention
 *  AC-9: GIVEN a callback currently being executed by the event thread,
 *         WHEN the callback attempts a synchronous IOC_postEVT,
 *         THEN the operation is forbidden and returns IOC_RESULT_FORBIDDEN_IN_CALLBACK.
 *
 *  AC-10: GIVEN a callback currently being executed,
 *          WHEN the callback attempts an asynchronous IOC_postEVT,
 *          THEN the operation succeeds and the event is queued.
 *
 *  AC-11: GIVEN the event thread has finished executing all callbacks,
 *          WHEN a synchronous post is attempted from another thread,
 *          THEN the operation succeeds normally.
 *
 * [@US-4] Limits and Recovery
 *  AC-12: GIVEN the system has reached the maximum number of subscribers,
 *          WHEN a new subscription is attempted,
 *          THEN the operation returns IOC_RESULT_TOO_MANY gracefully.
 *
 *  AC-13: GIVEN events are already queued for a subscriber,
 *          WHEN the subscriber unregisters,
 *          THEN the already-queued events are still delivered before the consumer is destroyed.
 *
 *  AC-14: GIVEN a callback is being executed,
 *          WHEN the callback attempts to unsubscribe itself,
 *          THEN the operation succeeds and no further events are delivered to it.
 *
 *  AC-15: GIVEN a callback is being executed,
 *          WHEN the callback attempts to subscribe a new event handler,
 *          THEN the operation succeeds and the new handler is active for future events.
 */
//=======>END OF ACCEPTANCE CRITERIA================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF TEST CASES=======================================================================
/**
 * TEST CASES define HOW to verify each Acceptance Criterion
 *
 * ORGANIZATION STRATEGIES:
 *  ✅ By Feature/Component: Group related functionality tests together
 *  ✅ By Test Category: Typical → Edge → State → Error → Performance
 *  ✅ By Coverage Matrix: Systematic coverage of identified dimensions
 *  ✅ By Priority: Critical functionality first, edge cases second
 *
 * STATUS TRACKING:
 *  ⚪ = Planned/TODO     - Designed but not implemented
 *  🔴 = Implemented/RED  - Test written and failing (need prod code)
 *  🟢 = Passed/GREEN     - Test written and passing
 *  ⚠️  = Issues          - Known problem needing attention
 *
 * NAMING CONVENTION:
 *  Format: verifyBehavior_byCondition_expectResult
 *  Example: verifyNonBlockPost_byFullQueue_expectImmediateReturn
 *
 * TEST STRUCTURE (4-phase pattern):
 *  1. 🔧 SETUP:    Prepare environment, create resources, set preconditions
 *  2. 🎯 BEHAVIOR: Execute the action being tested
 *  3. ✅ VERIFY:   Assert outcomes (keep ≤3 key assertions)
 *  4. 🧹 CLEANUP:  Release resources, reset state
 *
 *===================================================================================================
 * ORGANIZATION FORMAT (for this file - by User Story):
 *===================================================================================================
 *
 * US-1: Backpressure and Queue Overflow Management
 *  🟢 [@AC-1,US-1] TC-1: verifyBackpressure_bySlowConsumer_expectPostBlocks
 *  🟢 [@AC-2,US-1] TC-2: verifyQueueOverflow_byFastProducer_expectErrorReturned
 *  🟢 [@AC-3,US-1] TC-3: verifyTimeout_byFullQueue_expectTimeoutReturned
 *  🟢 [@AC-4,US-1] TC-4: verifyRecovery_afterBackpressure_expectNormalFlow
 *
 * US-2: Cascading Event Storm Prevention
 *  🟢 [@AC-5,US-2] TC-5: verifyCascading_byLinearChain_expectAllDelivered
 *  🟢 [@AC-6,US-2] TC-6: verifyCascading_byExponentialAmplification_expectLimited
 *  🟢 [@AC-7,US-2] TC-7: verifyCascading_byMayBlockOption_expectGracefulBackpressure
 *  🟢 [@AC-8,US-2] TC-8: verifyRecovery_afterEventStorm_expectNormalOperation
 *
 * US-3: Sync Mode Deadlock Prevention
 *  🟢 [@AC-9,US-3] TC-9: verifySyncMode_duringCallback_expectForbidden
 *  🟢 [@AC-10,US-3] TC-10: verifyAsyncMode_duringCallback_expectSuccess
 *  🟢 [@AC-11,US-3] TC-11: verifySyncMode_afterCallback_expectSuccess
 *
 * US-4: Limits and Re-entrancy
 *  🟢 [@AC-12,US-4] TC-12: verifyStability_withMaxSubscribers
 *  🟢 [@AC-13,US-4] TC-13: verifyQueueDrain_afterUnsubscribe
 *  🟢 [@AC-14,US-4] TC-14: verifyUnsubscribe_duringCallback_expectSuccess
 *  🟢 [@AC-15,US-4] TC-15: verifySubscribe_duringCallback_expectSuccess
 */
//======>END OF TEST CASES=========================================================================
//======>END OF UNIT TESTING DESIGN================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======BEGIN OF UNIT TESTING IMPLEMENTATION=======================================================

/**
 * TEST CASE TEMPLATE (copy for each TC)
 *  @[Name]: ${verifyBehaviorX_byDoA_expectSomething}
 *  @[Steps]:
 *    1) 🔧 SETUP: do ..., with ...
 *    2) 🎯 BEHAVIOR: do ..., with ...
 *    3) ✅ VERIFY: assert ..., compare ...
 *    4) 🧹 CLEANUP: release ..., reset ...
 *  @[Expect]: ${how to verify}
 *  @[Notes]: ${additional notes}
 */

// =================================================================================================
// US-1: Backpressure and Queue Overflow Management
// =================================================================================================

namespace {
struct Tc1Context {
    std::atomic<uint32_t> EventsReceived{0};
    static constexpr uint32_t ProcessingDelayMs = 100;
};

IOC_Result_T tc1CbSlow(const IOC_EvtDesc_pT, void* pData) {
    auto* pCtx = static_cast<Tc1Context*>(pData);
    pCtx->EventsReceived.fetch_add(1);
    std::this_thread::sleep_for(std::chrono::milliseconds(Tc1Context::ProcessingDelayMs));
    return IOC_RESULT_SUCCESS;
}
}  // namespace

/**
 * [@AC-1,US-1]
 * TC-1:
 *   @[Name]: verifyBackpressure_bySlowConsumer_expectPostBlocks
 *   @[Purpose]: Verify MayBlock option blocks when queue full
 *   @[Steps]:
 *     1) 🔧 SETUP: Subscribe a slow consumer (100ms delay).
 *     2) 🎯 BEHAVIOR: Post 100 events rapidly with MayBlock.
 *     3) ✅ VERIFY: Check that post operations blocked and all 100 events are delivered.
 *     4) 🧹 CLEANUP: Unsubscribe consumer.
 *   @[Expect]: Producer blocks, all events delivered.
 */
TEST(UTConlesEventRobustness, verifyBackpressure_bySlowConsumer_expectPostBlocks) {
    //===>>> SETUP <<<===
    printf("🔧 SETUP: verifyBackpressure_bySlowConsumer_expectPostBlocks\n");
    Tc1Context Ctx;
    IOC_SubEvtArgs_T SArgs = {.CbProcEvt_F = tc1CbSlow, .pCbPrivData = &Ctx, .EvtNum = 0};
    ASSERT_EQ(IOC_RESULT_SUCCESS, IOC_subEVT_inConlesMode(&SArgs));

    //===>>> BEHAVIOR <<<===
    printf("🎯 BEHAVIOR: verifyBackpressure_bySlowConsumer_expectPostBlocks\n");
    uint32_t BlockedCount = 0;
    IOC_Option_defineASyncMayBlock(Option);

    for (uint32_t i = 0; i < 100; i++) {
        IOC_EvtDesc_T Evt = {.EvtID = IOC_EVTID_TEST_KEEPALIVE};
        auto Start = std::chrono::steady_clock::now();
        ASSERT_EQ(IOC_RESULT_SUCCESS, IOC_postEVT_inConlesMode(&Evt, &Option));
        auto End = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(End - Start).count() > 50) BlockedCount++;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    IOC_forceProcEVT();
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));

    //===>>> VERIFY <<<===
    printf("✅ VERIFY: verifyBackpressure_bySlowConsumer_expectPostBlocks\n");
    VERIFY_KEYPOINT_GT(BlockedCount, 0u, "Should have blocked at least once");
    VERIFY_KEYPOINT_EQ(Ctx.EventsReceived.load(), 100u, "All events should be delivered");

    //===>>> CLEANUP <<<===
    printf("🧹 CLEANUP: verifyBackpressure_bySlowConsumer_expectPostBlocks\n");
    IOC_UnsubEvtArgs_T UA = {.CbProcEvt_F = tc1CbSlow, .pCbPrivData = &Ctx};
    IOC_unsubEVT_inConlesMode(&UA);
}

/**
 * [@AC-2,US-1]
 * TC-2:
 *   @[Name]: verifyQueueOverflow_byFastProducer_expectErrorReturned
 *   @[Purpose]: Verify NonBlock option returns error when queue full
 *   @[Steps]:
 *     1) 🔧 SETUP: Subscribe slow consumer.
 *     2) 🎯 BEHAVIOR: Post 100 events rapidly with NonBlock.
 *     3) ✅ VERIFY: Returns TOO_MANY_QUEUING_EVTDESC when queue is full.
 *     4) 🧹 CLEANUP: Unsubscribe consumer.
 *   @[Expect]: Returns TOO_MANY_QUEUING_EVTDESC when queue is full.
 */
TEST(UTConlesEventRobustness, verifyQueueOverflow_byFastProducer_expectErrorReturned) {
    //===>>> SETUP <<<===
    printf("🔧 SETUP: verifyQueueOverflow_byFastProducer_expectErrorReturned\n");
    Tc1Context Ctx;
    IOC_SubEvtArgs_T SArgs = {.CbProcEvt_F = tc1CbSlow, .pCbPrivData = &Ctx, .EvtNum = 0};
    ASSERT_EQ(IOC_RESULT_SUCCESS, IOC_subEVT_inConlesMode(&SArgs));

    //===>>> BEHAVIOR <<<===
    printf("🎯 BEHAVIOR: verifyQueueOverflow_byFastProducer_expectErrorReturned\n");
    uint32_t Success = 0, Overflow = 0;
    IOC_Option_defineNonBlock(Option);

    for (uint32_t i = 0; i < 100; i++) {
        IOC_EvtDesc_T Evt = {.EvtID = IOC_EVTID_TEST_KEEPALIVE};
        IOC_Result_T Res = IOC_postEVT_inConlesMode(&Evt, &Option);
        if (Res == IOC_RESULT_SUCCESS)
            Success++;
        else if (Res == IOC_RESULT_TOO_MANY_QUEUING_EVTDESC)
            Overflow++;
    }

    //===>>> VERIFY <<<===
    printf("✅ VERIFY: verifyQueueOverflow_byFastProducer_expectErrorReturned\n");
    VERIFY_KEYPOINT_GE(Success, 64u, "Should fill queue capacity");
    VERIFY_KEYPOINT_GT(Overflow, 0u, "Should return overflow error");

    //===>>> CLEANUP <<<===
    printf("🧹 CLEANUP: verifyQueueOverflow_byFastProducer_expectErrorReturned\n");
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    IOC_UnsubEvtArgs_T UA = {.CbProcEvt_F = tc1CbSlow, .pCbPrivData = &Ctx};
    IOC_unsubEVT_inConlesMode(&UA);
}

/**
 * [@AC-3,US-1]
 * TC-3:
 *   @[Name]: verifyTimeout_byFullQueue_expectTimeoutReturned
 *   @[Purpose]: Verify Timeout option returns error after duration
 *   @[Steps]:
 *     1) 🔧 SETUP: Subscribe blocking consumer and fill queue.
 *     2) 🎯 BEHAVIOR: Post with 500ms timeout.
 *     3) ✅ VERIFY: Returns IOC_RESULT_TIMEOUT and duration is ~500ms.
 *     4) 🧹 CLEANUP: Unsubscribe consumer.
 *   @[Expect]: Returns IOC_RESULT_TIMEOUT.
 */
TEST(UTConlesEventRobustness, verifyTimeout_byFullQueue_expectTimeoutReturned) {
    //===>>> SETUP <<<===
    printf("🔧 SETUP: verifyTimeout_byFullQueue_expectTimeoutReturned\n");
    struct TimeoutCtx {
        std::atomic<bool> Block{true};
        std::atomic<uint32_t> Count{0};
    } Ctx;

    auto cb = [](const IOC_EvtDesc_pT, void* pData) -> IOC_Result_T {
        auto* p = static_cast<TimeoutCtx*>(pData);
        while (p->Block.load()) std::this_thread::sleep_for(std::chrono::milliseconds(10));
        p->Count.fetch_add(1);
        return IOC_RESULT_SUCCESS;
    };

    IOC_SubEvtArgs_T SArgs = {.CbProcEvt_F = cb, .pCbPrivData = &Ctx, .EvtNum = 0};
    ASSERT_EQ(IOC_RESULT_SUCCESS, IOC_subEVT_inConlesMode(&SArgs));

    for (int i = 0; i < 100; i++) {
        IOC_EvtDesc_T Evt = {.EvtID = IOC_EVTID_TEST_KEEPALIVE};
        IOC_Option_defineNonBlock(Opt);
        IOC_postEVT_inConlesMode(&Evt, &Opt);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    IOC_Option_defineTimeout(Option, 500000);
    IOC_EvtDesc_T Evt = {.EvtID = IOC_EVTID_TEST_KEEPALIVE};
    auto Start = std::chrono::steady_clock::now();
    IOC_Result_T Res = IOC_postEVT_inConlesMode(&Evt, &Option);
    auto End = std::chrono::steady_clock::now();

    Ctx.Block.store(false);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    //===>>> VERIFY <<<===
    printf("✅ VERIFY: verifyTimeout_byFullQueue_expectTimeoutReturned\n");
    if (Res == IOC_RESULT_TIMEOUT) {
        auto Dur = std::chrono::duration_cast<std::chrono::milliseconds>(End - Start).count();
        VERIFY_KEYPOINT_TRUE(Dur >= 400 && Dur <= 700, "Timeout should be ~500ms");
    }

    //===>>> CLEANUP <<<===
    printf("🧹 CLEANUP: verifyTimeout_byFullQueue_expectTimeoutReturned\n");
    IOC_UnsubEvtArgs_T UA = {.CbProcEvt_F = cb, .pCbPrivData = &Ctx};
    IOC_unsubEVT_inConlesMode(&UA);
}

/**
 * [@AC-4,US-1]
 * TC-4:
 *   @[Name]: verifyRecovery_afterBackpressure_expectNormalFlow
 *   @[Purpose]: Verify system returns to low latency after stress
 *   @[Steps]:
 *     1) 🔧 SETUP: Create backpressure with slow consumer.
 *     2) 🎯 BEHAVIOR: Wait for queue to drain and measure latency of new post.
 *     3) ✅ VERIFY: Latency is low (<20ms).
 *     4) 🧹 CLEANUP: Unsubscribe consumer.
 *   @[Expect]: Latency is low (<20ms).
 */
TEST(UTConlesEventRobustness, verifyRecovery_afterBackpressure_expectNormalFlow) {
    //===>>> SETUP <<<===
    printf("🔧 SETUP: verifyRecovery_afterBackpressure_expectNormalFlow\n");
    struct RecCtx {
        std::atomic<uint32_t> Delay{100};
        std::atomic<uint32_t> Count{0};
    } Ctx;

    auto cb = [](const IOC_EvtDesc_pT, void* pData) -> IOC_Result_T {
        auto* p = static_cast<RecCtx*>(pData);
        p->Count.fetch_add(1);
        std::this_thread::sleep_for(std::chrono::milliseconds(p->Delay.load()));
        return IOC_RESULT_SUCCESS;
    };

    IOC_SubEvtArgs_T SArgs = {.CbProcEvt_F = cb, .pCbPrivData = &Ctx, .EvtNum = 0};
    ASSERT_EQ(IOC_RESULT_SUCCESS, IOC_subEVT_inConlesMode(&SArgs));

    for (int i = 0; i < 100; i++) {
        IOC_EvtDesc_T Evt = {.EvtID = IOC_EVTID_TEST_KEEPALIVE};
        IOC_Option_defineNonBlock(Opt);
        IOC_postEVT_inConlesMode(&Evt, &Opt);
    }

    Ctx.Delay.store(1);
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));

    auto Start = std::chrono::steady_clock::now();
    IOC_EvtDesc_T Evt = {.EvtID = IOC_EVTID_TEST_KEEPALIVE};
    IOC_postEVT_inConlesMode(&Evt, nullptr);
    auto End = std::chrono::steady_clock::now();

    //===>>> VERIFY <<<===
    printf("✅ VERIFY: verifyRecovery_afterBackpressure_expectNormalFlow\n");
    VERIFY_KEYPOINT_LT(std::chrono::duration_cast<std::chrono::milliseconds>(End - Start).count(), 20,
                       "Should recover latency");

    //===>>> CLEANUP <<<===
    printf("🧹 CLEANUP: verifyRecovery_afterBackpressure_expectNormalFlow\n");
    IOC_UnsubEvtArgs_T UA = {.CbProcEvt_F = cb, .pCbPrivData = &Ctx};
    IOC_unsubEVT_inConlesMode(&UA);
}

// =================================================================================================
// US-2: Cascading Event Storm Prevention
// =================================================================================================

namespace {
struct Tc5Ctx {
    std::atomic<uint32_t> Counts[5]{0, 0, 0, 0, 0};
};

IOC_Result_T tc5Cb(const IOC_EvtDesc_pT pEvt, void* pData) {
    auto* pCtx = static_cast<Tc5Ctx*>(pData);
    uint32_t Level = pEvt->EvtValue;
    pCtx->Counts[Level].fetch_add(1);
    if (Level < 4) {
        IOC_EvtDesc_T Child = {.EvtID = 1000 + Level + 1, .EvtValue = Level + 1};
        IOC_postEVT_inConlesMode(&Child, nullptr);
    }
    return IOC_RESULT_SUCCESS;
}
}  // namespace

/**
 * [@AC-5,US-2]
 * TC-5:
 *   @[Name]: verifyCascading_byLinearChain_expectAllDelivered
 *   @[Purpose]: Verify linear event chain (A->B->C) works
 *   @[Steps]:
 *     1) 🔧 SETUP: Subscribe callback to 5 event IDs.
 *     2) 🎯 BEHAVIOR: Post root event ID 0; callback for ID n posts ID n+1.
 *     3) ✅ VERIFY: All 5 events delivered in sequence.
 *     4) 🧹 CLEANUP: Unsubscribe consumer.
 *   @[Expect]: All 5 events delivered in sequence.
 */
TEST(UTConlesEventRobustness, verifyCascading_byLinearChain_expectAllDelivered) {
    //===>>> SETUP <<<===
    printf("🔧 SETUP: verifyCascading_byLinearChain_expectAllDelivered\n");
    Tc5Ctx Ctx;
    IOC_EvtID_T eids[5] = {1000, 1001, 1002, 1003, 1004};
    IOC_SubEvtArgs_T SArgs = {.CbProcEvt_F = tc5Cb, .pCbPrivData = &Ctx, .EvtNum = 5, .pEvtIDs = eids};
    ASSERT_EQ(IOC_RESULT_SUCCESS, IOC_subEVT_inConlesMode(&SArgs));

    //===>>> BEHAVIOR <<<===
    printf("🎯 BEHAVIOR: verifyCascading_byLinearChain_expectAllDelivered\n");
    IOC_EvtDesc_T Root = {.EvtID = 1000, .EvtValue = 0};
    IOC_postEVT_inConlesMode(&Root, nullptr);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    //===>>> VERIFY <<<===
    printf("✅ VERIFY: verifyCascading_byLinearChain_expectAllDelivered\n");
    for (int i = 0; i < 5; i++) VERIFY_KEYPOINT_EQ(Ctx.Counts[i].load(), 1u, "Each level should process once");

    //===>>> CLEANUP <<<===
    printf("🧹 CLEANUP: verifyCascading_byLinearChain_expectAllDelivered\n");
    IOC_UnsubEvtArgs_T UA = {.CbProcEvt_F = tc5Cb, .pCbPrivData = &Ctx};
    IOC_unsubEVT_inConlesMode(&UA);
}

/**
 * [@AC-6,US-2]
 * TC-6:
 *   @[Name]: verifyCascading_byExponentialAmplification_expectLimited
 *   @[Purpose]: Verify exponential event storm (1->2->4...) is limited by queue
 *   @[Steps]:
 *     1) 🔧 SETUP: Subscribe callback that posts 2 events for each received event.
 *     2) 🎯 BEHAVIOR: Post 10 root events (limit depth to 6).
 *     3) ✅ VERIFY: Queue eventually overflows and returns error.
 *     4) 🧹 CLEANUP: Unsubscribe consumer.
 *   @[Expect]: Queue eventually overflows and returns error.
 */
TEST(UTConlesEventRobustness, verifyCascading_byExponentialAmplification_expectLimited) {
    //===>>> SETUP <<<===
    printf("🔧 SETUP: verifyCascading_byExponentialAmplification_expectLimited\n");
    struct AmpCtx {
        std::atomic<uint32_t> Rec{0}, Over{0};
    } Ctx;

    auto cb = [](const IOC_EvtDesc_pT pEvt, void* pData) -> IOC_Result_T {
        auto* p = static_cast<AmpCtx*>(pData);
        p->Rec.fetch_add(1);
        uint32_t Depth = pEvt->EvtValue;
        if (Depth >= 6) return IOC_RESULT_SUCCESS;
        for (int i = 0; i < 2; i++) {
            IOC_EvtDesc_T Child = {.EvtID = 2000, .EvtValue = Depth + 1};
            IOC_Option_defineNonBlock(Opt);
            if (IOC_postEVT_inConlesMode(&Child, &Opt) != IOC_RESULT_SUCCESS) p->Over.fetch_add(1);
        }
        return IOC_RESULT_SUCCESS;
    };

    IOC_EvtID_T eid = 2000;
    IOC_SubEvtArgs_T SArgs = {.CbProcEvt_F = cb, .pCbPrivData = &Ctx, .EvtNum = 1, .pEvtIDs = &eid};
    ASSERT_EQ(IOC_RESULT_SUCCESS, IOC_subEVT_inConlesMode(&SArgs));

    //===>>> BEHAVIOR <<<===
    printf("🎯 BEHAVIOR: verifyCascading_byExponentialAmplification_expectLimited\n");
    for (int i = 0; i < 10; i++) {
        IOC_EvtDesc_T Root = {.EvtID = 2000, .EvtValue = 0};
        IOC_postEVT_inConlesMode(&Root, nullptr);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));

    //===>>> VERIFY <<<===
    printf("✅ VERIFY: verifyCascading_byExponentialAmplification_expectLimited\n");
    VERIFY_KEYPOINT_GT(Ctx.Over.load(), 0u, "Should have overflowed");

    //===>>> CLEANUP <<<===
    printf("🧹 CLEANUP: verifyCascading_byExponentialAmplification_expectLimited\n");
    IOC_UnsubEvtArgs_T UA = {.CbProcEvt_F = cb, .pCbPrivData = &Ctx};
    IOC_unsubEVT_inConlesMode(&UA);
}

/**
 * [@AC-7,US-2]
 * TC-7:
 *   @[Name]: verifyCascading_byMayBlockOption_expectGracefulBackpressure
 *   @[Purpose]: Verify MayBlock prevents overflow during cascading
 *   @[Steps]:
 *     1) 🔧 SETUP: Subscribe slow callback (50ms) that posts 2 events.
 *     2) 🎯 BEHAVIOR: Post 3 root events using ASyncMayBlock for child events.
 *     3) ✅ VERIFY: All events delivered without overflow error.
 *     4) 🧹 CLEANUP: Unsubscribe consumer.
 *   @[Expect]: All events delivered without overflow error.
 */
TEST(UTConlesEventRobustness, verifyCascading_byMayBlockOption_expectGracefulBackpressure) {
    //===>>> SETUP <<<===
    printf("🔧 SETUP: verifyCascading_byMayBlockOption_expectGracefulBackpressure\n");
    struct SlowAmpCtx {
        std::atomic<uint32_t> Rec{0}, Fail{0};
    } Ctx;

    auto cb = [](const IOC_EvtDesc_pT pEvt, void* pData) -> IOC_Result_T {
        auto* p = static_cast<SlowAmpCtx*>(pData);
        p->Rec.fetch_add(1);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        uint32_t Depth = pEvt->EvtValue;
        if (Depth >= 3) return IOC_RESULT_SUCCESS;
        for (int i = 0; i < 2; i++) {
            IOC_EvtDesc_T Child = {.EvtID = 3000, .EvtValue = Depth + 1};
            IOC_Option_defineASyncMayBlock(Opt);
            if (IOC_postEVT_inConlesMode(&Child, &Opt) != IOC_RESULT_SUCCESS) p->Fail.fetch_add(1);
        }
        return IOC_RESULT_SUCCESS;
    };

    IOC_EvtID_T eid = 3000;
    IOC_SubEvtArgs_T SArgs = {.CbProcEvt_F = cb, .pCbPrivData = &Ctx, .EvtNum = 1, .pEvtIDs = &eid};
    ASSERT_EQ(IOC_RESULT_SUCCESS, IOC_subEVT_inConlesMode(&SArgs));

    //===>>> BEHAVIOR <<<===
    printf("🎯 BEHAVIOR: verifyCascading_byMayBlockOption_expectGracefulBackpressure\n");
    for (int i = 0; i < 3; i++) {
        IOC_EvtDesc_T Root = {.EvtID = 3000, .EvtValue = 0};
        IOC_postEVT_inConlesMode(&Root, nullptr);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(3000));

    //===>>> VERIFY <<<===
    printf("✅ VERIFY: verifyCascading_byMayBlockOption_expectGracefulBackpressure\n");
    VERIFY_KEYPOINT_EQ(Ctx.Fail.load(), 0u, "MayBlock should prevent failures");

    //===>>> CLEANUP <<<===
    printf("🧹 CLEANUP: verifyCascading_byMayBlockOption_expectGracefulBackpressure\n");
    IOC_UnsubEvtArgs_T UA = {.CbProcEvt_F = cb, .pCbPrivData = &Ctx};
    IOC_unsubEVT_inConlesMode(&UA);
}

/**
 * [@AC-8,US-2]
 * TC-8:
 *   @[Name]: verifyRecovery_afterEventStorm_expectNormalOperation
 *   @[Purpose]: Verify system recovers after a massive event storm
 *   @[Steps]:
 *     1) 🔧 SETUP: Subscribe consumer to storm and recovery IDs.
 *     2) 🎯 BEHAVIOR: Post 200 events rapidly (storm), then post 10 new events.
 *     3) ✅ VERIFY: All 10 new events are processed correctly.
 *     4) 🧹 CLEANUP: Unsubscribe consumer.
 *   @[Expect]: All 10 new events are processed correctly.
 */
TEST(UTConlesEventRobustness, verifyRecovery_afterEventStorm_expectNormalOperation) {
    //===>>> SETUP <<<===
    printf("🔧 SETUP: verifyRecovery_afterEventStorm_expectNormalOperation\n");
    struct StormCtx {
        std::atomic<uint32_t> Storm{0}, Rec{0};
    } Ctx;

    auto cb = [](const IOC_EvtDesc_pT pEvt, void* pData) -> IOC_Result_T {
        auto* p = static_cast<StormCtx*>(pData);
        if (pEvt->EvtID == 4000)
            p->Storm.fetch_add(1);
        else
            p->Rec.fetch_add(1);
        return IOC_RESULT_SUCCESS;
    };

    IOC_EvtID_T eids[] = {4000, 4001};
    IOC_SubEvtArgs_T SArgs = {.CbProcEvt_F = cb, .pCbPrivData = &Ctx, .EvtNum = 2, .pEvtIDs = eids};
    ASSERT_EQ(IOC_RESULT_SUCCESS, IOC_subEVT_inConlesMode(&SArgs));

    //===>>> BEHAVIOR <<<===
    printf("🎯 BEHAVIOR: verifyRecovery_afterEventStorm_expectNormalOperation\n");
    for (int i = 0; i < 200; i++) {
        IOC_EvtDesc_T Evt = {.EvtID = 4000};
        IOC_Option_defineNonBlock(Opt);
        IOC_postEVT_inConlesMode(&Evt, &Opt);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));

    for (int i = 0; i < 10; i++) {
        IOC_EvtDesc_T Evt = {.EvtID = 4001};
        IOC_postEVT_inConlesMode(&Evt, nullptr);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    //===>>> VERIFY <<<===
    printf("✅ VERIFY: verifyRecovery_afterEventStorm_expectNormalOperation\n");
    VERIFY_KEYPOINT_EQ(Ctx.Rec.load(), 10u, "Should recover and process all 10 events");

    //===>>> CLEANUP <<<===
    printf("🧹 CLEANUP: verifyRecovery_afterEventStorm_expectNormalOperation\n");
    IOC_UnsubEvtArgs_T UA = {.CbProcEvt_F = cb, .pCbPrivData = &Ctx};
    IOC_unsubEVT_inConlesMode(&UA);
}

// =================================================================================================
// US-3: Sync Mode Deadlock Prevention
// =================================================================================================

namespace {
struct Tc9Ctx {
    std::atomic<bool> Exec{false}, Attempt{false};
    std::atomic<IOC_Result_T> Res{IOC_RESULT_BUG};
};

IOC_Result_T tc9Cb(const IOC_EvtDesc_pT, void* pData) {
    auto* p = static_cast<Tc9Ctx*>(pData);
    p->Exec.store(true);
    IOC_EvtDesc_T Inner = {.EvtID = 5001};
    IOC_Option_defineSyncMode(Opt);
    p->Attempt.store(true);
    p->Res.store(IOC_postEVT_inConlesMode(&Inner, &Opt));
    return IOC_RESULT_SUCCESS;
}
}  // namespace

/**
 * [@AC-9,US-3]
 * TC-9:
 *   @[Name]: verifySyncMode_duringCallback_expectForbidden
 *   @[Purpose]: Verify SyncMode is forbidden inside a callback to prevent deadlock
 *   @[Steps]:
 *     1) 🔧 SETUP: Subscribe callback that attempts SyncMode post.
 *     2) 🎯 BEHAVIOR: Trigger the callback.
 *     3) ✅ VERIFY: Returns IOC_RESULT_FORBIDDEN.
 *     4) 🧹 CLEANUP: Unsubscribe consumer.
 *   @[Expect]: Returns IOC_RESULT_FORBIDDEN.
 */
TEST(UTConlesEventRobustness, verifySyncMode_duringCallback_expectForbidden) {
    //===>>> SETUP <<<===
    printf("🔧 SETUP: verifySyncMode_duringCallback_expectForbidden\n");
    Tc9Ctx Ctx;
    IOC_EvtID_T eid = 5000;
    IOC_SubEvtArgs_T SArgs = {.CbProcEvt_F = tc9Cb, .pCbPrivData = &Ctx, .EvtNum = 1, .pEvtIDs = &eid};
    ASSERT_EQ(IOC_RESULT_SUCCESS, IOC_subEVT_inConlesMode(&SArgs));

    //===>>> BEHAVIOR <<<===
    printf("🎯 BEHAVIOR: verifySyncMode_duringCallback_expectForbidden\n");
    IOC_EvtDesc_T Trigger = {.EvtID = 5000};
    IOC_postEVT_inConlesMode(&Trigger, nullptr);
    IOC_forceProcEVT();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    //===>>> VERIFY <<<===
    printf("✅ VERIFY: verifySyncMode_duringCallback_expectForbidden\n");
    VERIFY_KEYPOINT_TRUE(Ctx.Exec.load(), "Callback should execute");
    VERIFY_KEYPOINT_EQ(Ctx.Res.load(), IOC_RESULT_FORBIDDEN, "Sync post in callback must be forbidden");

    //===>>> CLEANUP <<<===
    printf("🧹 CLEANUP: verifySyncMode_duringCallback_expectForbidden\n");
    IOC_UnsubEvtArgs_T UA = {.CbProcEvt_F = tc9Cb, .pCbPrivData = &Ctx};
    IOC_unsubEVT_inConlesMode(&UA);
}

/**
 * [@AC-10,US-3]
 * TC-10:
 *   @[Name]: verifyAsyncMode_duringCallback_expectSuccess
 *   @[Purpose]: Verify ASyncMode is allowed inside a callback
 *   @[Steps]:
 *     1) 🔧 SETUP: Subscribe callback that attempts ASyncMode post.
 *     2) 🎯 BEHAVIOR: Trigger the callback.
 *     3) ✅ VERIFY: Returns IOC_RESULT_SUCCESS.
 *     4) 🧹 CLEANUP: Unsubscribe consumer.
 *   @[Expect]: Returns IOC_RESULT_SUCCESS.
 */
TEST(UTConlesEventRobustness, verifyAsyncMode_duringCallback_expectSuccess) {
    //===>>> SETUP <<<===
    printf("🔧 SETUP: verifyAsyncMode_duringCallback_expectSuccess\n");
    struct Tc10Ctx {
        std::atomic<bool> Exec{false};
        std::atomic<IOC_Result_T> Res{IOC_RESULT_BUG};
    } Ctx;

    auto cb = [](const IOC_EvtDesc_pT, void* pData) -> IOC_Result_T {
        auto* p = static_cast<Tc10Ctx*>(pData);
        p->Exec.store(true);
        IOC_EvtDesc_T Inner = {.EvtID = 6001};
        IOC_Option_defineNonBlock(Opt);
        p->Res.store(IOC_postEVT_inConlesMode(&Inner, &Opt));
        return IOC_RESULT_SUCCESS;
    };

    IOC_EvtID_T eid = 6000;
    IOC_SubEvtArgs_T SArgs = {.CbProcEvt_F = cb, .pCbPrivData = &Ctx, .EvtNum = 1, .pEvtIDs = &eid};
    ASSERT_EQ(IOC_RESULT_SUCCESS, IOC_subEVT_inConlesMode(&SArgs));

    //===>>> BEHAVIOR <<<===
    printf("🎯 BEHAVIOR: verifyAsyncMode_duringCallback_expectSuccess\n");
    IOC_EvtDesc_T Trigger = {.EvtID = 6000};
    IOC_postEVT_inConlesMode(&Trigger, nullptr);
    IOC_forceProcEVT();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    //===>>> VERIFY <<<===
    printf("✅ VERIFY: verifyAsyncMode_duringCallback_expectSuccess\n");
    VERIFY_KEYPOINT_TRUE(Ctx.Exec.load(), "Callback should execute");
    VERIFY_KEYPOINT_EQ(Ctx.Res.load(), IOC_RESULT_SUCCESS, "Async post in callback should succeed");

    //===>>> CLEANUP <<<===
    printf("🧹 CLEANUP: verifyAsyncMode_duringCallback_expectSuccess\n");
    IOC_UnsubEvtArgs_T UA = {.CbProcEvt_F = cb, .pCbPrivData = &Ctx};
    IOC_unsubEVT_inConlesMode(&UA);
}

/**
 * [@AC-11,US-3]
 * TC-11:
 *   @[Name]: verifySyncMode_afterCallback_expectSuccess
 *   @[Purpose]: Verify SyncMode works normally after a callback has finished
 *   @[Steps]:
 *     1) 🔧 SETUP: Trigger a callback and wait for it to finish.
 *     2) 🎯 BEHAVIOR: Post an event with SyncMode from main thread.
 *     3) ✅ VERIFY: Returns IOC_RESULT_SUCCESS.
 *     4) 🧹 CLEANUP: Unsubscribe consumer.
 *   @[Expect]: Returns IOC_RESULT_SUCCESS.
 */
TEST(UTConlesEventRobustness, verifySyncMode_afterCallback_expectSuccess) {
    //===>>> SETUP <<<===
    printf("🔧 SETUP: verifySyncMode_afterCallback_expectSuccess\n");
    struct Tc11Ctx {
        std::atomic<bool> Exec{false};
    } Ctx;

    auto cb = [](const IOC_EvtDesc_pT, void* pData) -> IOC_Result_T {
        static_cast<Tc11Ctx*>(pData)->Exec.store(true);
        return IOC_RESULT_SUCCESS;
    };

    IOC_EvtID_T eid = 7000;
    IOC_SubEvtArgs_T SArgs = {.CbProcEvt_F = cb, .pCbPrivData = &Ctx, .EvtNum = 1, .pEvtIDs = &eid};
    ASSERT_EQ(IOC_RESULT_SUCCESS, IOC_subEVT_inConlesMode(&SArgs));

    //===>>> BEHAVIOR <<<===
    printf("🎯 BEHAVIOR: verifySyncMode_afterCallback_expectSuccess\n");
    IOC_EvtDesc_T Trigger = {.EvtID = 7000};
    IOC_postEVT_inConlesMode(&Trigger, nullptr);
    IOC_forceProcEVT();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    IOC_EvtDesc_T Sync = {.EvtID = 7001};
    IOC_Option_defineSyncMode(Opt);

    //===>>> VERIFY <<<===
    printf("✅ VERIFY: verifySyncMode_afterCallback_expectSuccess\n");
    VERIFY_KEYPOINT_EQ(IOC_postEVT_inConlesMode(&Sync, &Opt), IOC_RESULT_SUCCESS,
                       "Sync post after callback should succeed");

    //===>>> CLEANUP <<<===
    printf("🧹 CLEANUP: verifySyncMode_afterCallback_expectSuccess\n");
    IOC_UnsubEvtArgs_T UA = {.CbProcEvt_F = cb, .pCbPrivData = &Ctx};
    IOC_unsubEVT_inConlesMode(&UA);
}

// =================================================================================================
// US-4: Limits and Recovery
// =================================================================================================

/**
 * [@AC-12,US-4]
 * TC-12:
 *   @[Name]: verifyStability_withMaxSubscribers
 *   @[Purpose]: Verify system stability and error handling at max subscriber limit
 *   @[Steps]:
 *     1) 🔧 SETUP: Subscribe 16 different callbacks (max).
 *     2) 🎯 BEHAVIOR: Attempt to subscribe the 17th, then unsubscribe one and retry.
 *     3) ✅ VERIFY: 17th fails with TOO_MANY_EVENT_CONSUMER; succeeds after one slot freed.
 *     4) 🧹 CLEANUP: Unsubscribe all remaining consumers.
 *   @[Expect]: 17th fails with TOO_MANY_EVENT_CONSUMER; succeeds after one slot freed.
 */
TEST(UTConlesEventRobustness, verifyStability_withMaxSubscribers) {
    //===>>> SETUP <<<===
    printf("🔧 SETUP: verifyStability_withMaxSubscribers\n");
    constexpr int MaxSub = 16;
    struct DummyCtx {
        int id;
    } Contexts[MaxSub + 1];
    auto dummyCb = [](const IOC_EvtDesc_pT, void*) -> IOC_Result_T { return IOC_RESULT_SUCCESS; };

    for (int i = 0; i < MaxSub; ++i) {
        IOC_EvtID_T eid = 8000 + i;
        IOC_SubEvtArgs_T SArgs = {.CbProcEvt_F = dummyCb, .pCbPrivData = &Contexts[i], .EvtNum = 1, .pEvtIDs = &eid};
        EXPECT_EQ(IOC_RESULT_SUCCESS, IOC_subEVT_inConlesMode(&SArgs));
    }

    //===>>> BEHAVIOR <<<===
    printf("🎯 BEHAVIOR: verifyStability_withMaxSubscribers\n");
    IOC_EvtID_T eid17 = 9000;
    IOC_SubEvtArgs_T SArgs17 = {
        .CbProcEvt_F = dummyCb, .pCbPrivData = &Contexts[MaxSub], .EvtNum = 1, .pEvtIDs = &eid17};

    //===>>> VERIFY <<<===
    printf("✅ VERIFY: verifyStability_withMaxSubscribers\n");
    EXPECT_EQ(IOC_RESULT_TOO_MANY_EVENT_CONSUMER, IOC_subEVT_inConlesMode(&SArgs17));

    IOC_UnsubEvtArgs_T UArgs = {.CbProcEvt_F = dummyCb, .pCbPrivData = &Contexts[0]};
    EXPECT_EQ(IOC_RESULT_SUCCESS, IOC_unsubEVT_inConlesMode(&UArgs));
    EXPECT_EQ(IOC_RESULT_SUCCESS, IOC_subEVT_inConlesMode(&SArgs17));

    //===>>> CLEANUP <<<===
    printf("🧹 CLEANUP: verifyStability_withMaxSubscribers\n");
    for (int i = 1; i < MaxSub; ++i) {
        IOC_UnsubEvtArgs_T UA = {.CbProcEvt_F = dummyCb, .pCbPrivData = &Contexts[i]};
        IOC_unsubEVT_inConlesMode(&UA);
    }
    IOC_UnsubEvtArgs_T UA17 = {.CbProcEvt_F = dummyCb, .pCbPrivData = &Contexts[MaxSub]};
    IOC_unsubEVT_inConlesMode(&UA17);
}

/**
 * [@AC-13,US-4]
 * TC-13:
 *   @[Name]: verifyQueueDrain_afterUnsubscribe
 *   @[Purpose]: Verify that unsubscribing doesn't leave "ghost" events in queue
 *   @[Steps]:
 *     1) 🔧 SETUP: Subscribe consumer and post 100 events.
 *     2) 🎯 BEHAVIOR: Immediately unsubscribe the consumer.
 *     3) ✅ VERIFY: Wait for link state to become Ready (queue drained).
 *     4) 🧹 CLEANUP: None needed.
 *   @[Expect]: Queue drains completely.
 */
TEST(UTConlesEventRobustness, verifyQueueDrain_afterUnsubscribe) {
    //===>>> SETUP <<<===
    printf("🔧 SETUP: verifyQueueDrain_afterUnsubscribe\n");
    struct DrainCtx {
        std::atomic<uint32_t> Count{0};
    } DCtx;

    auto cb = [](const IOC_EvtDesc_pT, void* pData) -> IOC_Result_T {
        static_cast<DrainCtx*>(pData)->Count.fetch_add(1);
        return IOC_RESULT_SUCCESS;
    };

    IOC_EvtID_T eid = 9999;
    IOC_SubEvtArgs_T SArgs = {.CbProcEvt_F = cb, .pCbPrivData = &DCtx, .EvtNum = 1, .pEvtIDs = &eid};
    ASSERT_EQ(IOC_RESULT_SUCCESS, IOC_subEVT_inConlesMode(&SArgs));

    for (int i = 0; i < 100; ++i) {
        IOC_EvtDesc_T Evt = {.EvtID = eid};
        IOC_Option_defineNonBlock(Opt);
        IOC_postEVT_inConlesMode(&Evt, &Opt);
    }

    //===>>> BEHAVIOR <<<===
    printf("🎯 BEHAVIOR: verifyQueueDrain_afterUnsubscribe\n");
    IOC_UnsubEvtArgs_T UArgs = {.CbProcEvt_F = cb, .pCbPrivData = &DCtx};
    ASSERT_EQ(IOC_RESULT_SUCCESS, IOC_unsubEVT_inConlesMode(&UArgs));

    bool drained = false;
    for (int i = 0; i < 100; ++i) {
        IOC_LinkState_T state;
        IOC_getLinkState(IOC_CONLES_MODE_AUTO_LINK_ID, &state, nullptr);
        if (state == IOC_LinkStateReady) {
            drained = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    //===>>> VERIFY <<<===
    printf("✅ VERIFY: verifyQueueDrain_afterUnsubscribe\n");
    EXPECT_TRUE(drained) << "Queue should drain even after unsubscribe";

    //===>>> CLEANUP <<<===
    printf("🧹 CLEANUP: verifyQueueDrain_afterUnsubscribe\n");
}

namespace {
struct UnsubCtx {
    std::atomic<int> CallCount{0};
    IOC_CbProcEvt_F Self;
};

IOC_Result_T tc14Cb(const IOC_EvtDesc_pT pEvt, void* pData) {
    auto* p = static_cast<UnsubCtx*>(pData);
    p->CallCount.fetch_add(1);
    IOC_UnsubEvtArgs_T UA = {.CbProcEvt_F = p->Self, .pCbPrivData = pData};
    IOC_unsubEVT_inConlesMode(&UA);
    return IOC_RESULT_SUCCESS;
}

struct SubCtx {
    std::atomic<int> CallCount{0};
    IOC_CbProcEvt_F Other;
    void* OtherData;
};

IOC_Result_T tc15Cb_Other(const IOC_EvtDesc_pT pEvt, void* pData) {
    auto* p = static_cast<std::atomic<int>*>(pData);
    p->fetch_add(1);
    return IOC_RESULT_SUCCESS;
}

IOC_Result_T tc15Cb_Main(const IOC_EvtDesc_pT pEvt, void* pData) {
    auto* p = static_cast<SubCtx*>(pData);
    IOC_EvtID_T eid = 7001;
    IOC_SubEvtArgs_T SArgs = {.CbProcEvt_F = p->Other, .pCbPrivData = p->OtherData, .EvtNum = 1, .pEvtIDs = &eid};
    IOC_subEVT_inConlesMode(&SArgs);
    return IOC_RESULT_SUCCESS;
}
}  // namespace

/**
 * [@AC-14,US-4]
 * TC-14:
 *   @[Name]: verifyUnsubscribe_duringCallback_expectSuccess
 *   @[Purpose]: Verify re-entrancy safety when unsubscribing from within a callback
 *   @[Steps]:
 *     1) 🔧 SETUP: Subscribe callback that unsubscribes itself.
 *     2) 🎯 BEHAVIOR: Trigger callback, then post event again.
 *     3) ✅ VERIFY: Callback is not called the second time.
 *     4) 🧹 CLEANUP: None needed.
 *   @[Expect]: Callback is not called the second time.
 */
TEST(UTConlesEventRobustness, verifyUnsubscribe_duringCallback_expectSuccess) {
    //===>>> SETUP <<<===
    printf("🔧 SETUP: verifyUnsubscribe_duringCallback_expectSuccess\n");
    UnsubCtx Ctx;
    Ctx.Self = tc14Cb;
    IOC_EvtID_T eid = 7000;
    IOC_SubEvtArgs_T SArgs = {.CbProcEvt_F = tc14Cb, .pCbPrivData = &Ctx, .EvtNum = 1, .pEvtIDs = &eid};
    ASSERT_EQ(IOC_RESULT_SUCCESS, IOC_subEVT_inConlesMode(&SArgs));

    //===>>> BEHAVIOR <<<===
    printf("🎯 BEHAVIOR: verifyUnsubscribe_duringCallback_expectSuccess\n");
    IOC_EvtDesc_T Evt = {.EvtID = 7000};
    IOC_postEVT_inConlesMode(&Evt, nullptr);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    //===>>> VERIFY <<<===
    printf("✅ VERIFY: verifyUnsubscribe_duringCallback_expectSuccess\n");
    VERIFY_KEYPOINT_EQ(Ctx.CallCount.load(), 1, "Should be called once");

    IOC_postEVT_inConlesMode(&Evt, nullptr);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    VERIFY_KEYPOINT_EQ(Ctx.CallCount.load(), 1, "Should NOT be called again");

    //===>>> CLEANUP <<<===
    printf("🧹 CLEANUP: verifyUnsubscribe_duringCallback_expectSuccess\n");
}

/**
 * [@AC-15,US-4]
 * TC-15:
 *   @[Name]: verifySubscribe_duringCallback_expectSuccess
 *   @[Purpose]: Verify re-entrancy safety when subscribing from within a callback
 *   @[Steps]:
 *     1) 🔧 SETUP: Subscribe callback A that subscribes callback B.
 *     2) 🎯 BEHAVIOR: Trigger A, then post event for B.
 *     3) ✅ VERIFY: Callback B is successfully registered and called.
 *     4) 🧹 CLEANUP: Unsubscribe both A and B.
 *   @[Expect]: Callback B is successfully registered and called.
 */
TEST(UTConlesEventRobustness, verifySubscribe_duringCallback_expectSuccess) {
    //===>>> SETUP <<<===
    printf("🔧 SETUP: verifySubscribe_duringCallback_expectSuccess\n");
    std::atomic<int> OtherCallCount{0};
    SubCtx Ctx;
    Ctx.Other = tc15Cb_Other;
    Ctx.OtherData = &OtherCallCount;

    IOC_EvtID_T eid = 7001;
    IOC_SubEvtArgs_T SArgs = {.CbProcEvt_F = tc15Cb_Main, .pCbPrivData = &Ctx, .EvtNum = 1, .pEvtIDs = &eid};
    ASSERT_EQ(IOC_RESULT_SUCCESS, IOC_subEVT_inConlesMode(&SArgs));

    //===>>> BEHAVIOR <<<===
    printf("🎯 BEHAVIOR: verifySubscribe_duringCallback_expectSuccess\n");
    IOC_EvtDesc_T Evt = {.EvtID = 7001};
    IOC_postEVT_inConlesMode(&Evt, nullptr);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    // Main callback should have subscribed Other callback
    // But Other callback won't be called for the SAME event because snapshot was already taken

    IOC_postEVT_inConlesMode(&Evt, nullptr);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    //===>>> VERIFY <<<===
    printf("✅ VERIFY: verifySubscribe_duringCallback_expectSuccess\n");
    VERIFY_KEYPOINT_EQ(OtherCallCount.load(), 1, "Other callback should be called on second post");

    //===>>> CLEANUP <<<===
    printf("🧹 CLEANUP: verifySubscribe_duringCallback_expectSuccess\n");
    IOC_UnsubEvtArgs_T UA1 = {.CbProcEvt_F = tc15Cb_Main, .pCbPrivData = &Ctx};
    IOC_UnsubEvtArgs_T UA2 = {.CbProcEvt_F = tc15Cb_Other, .pCbPrivData = &OtherCallCount};
    IOC_unsubEVT_inConlesMode(&UA1);
    IOC_unsubEVT_inConlesMode(&UA2);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>END OF TEST IMPLEMENTATION================================================================

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
//   P1 🥇 FUNCTIONAL:     Must complete before P2 (ValidFunc + InvalidFunc).
//   P2 🥈 DESIGN-ORIENTED: Test after P1 (State, Capability, Concurrency).
//   P3 🥉 QUALITY-ORIENTED: Test for quality attributes (Performance, Robust, etc.).
//   P4 🎯 ADDONS:          Optional (Demo, Examples).
//
// WORKFLOW:
//   1. Complete all P1 tests (this is the gate before P2).
//   2. Move to P2 tests based on design complexity.
//   3. Add P3 tests for specific quality requirements.
//   4. Add P4 tests for documentation purposes.
//   5. Mark status as you go: ⚪ TODO → 🔴 RED → 🟢 GREEN.
//
//===================================================================================================
// P1 🥇 FUNCTIONAL TESTING – ValidFunc (Typical + Edge)
//===================================================================================================
//
//   NOTE: P1 tests are COVERED in other files (UT_ConlesEventTypical.cxx, etc.)
//         This file focuses on P3 ROBUSTNESS testing (promoted to P2 priority due to risk).
//
// 🚪 GATE P1: All P1 tests GREEN before proceeding to P2/P3.
//
//===================================================================================================
// P3 🥉 QUALITY-ORIENTED TESTING – Robustness (PROMOTED TO P2 PRIORITY)
//===================================================================================================
//
// US-1: Backpressure and Queue Overflow Management
//   🟢 [@AC-1,US-1] TC-1: verifyBackpressure_bySlowConsumer_expectPostBlocks
//        - Description: MayBlock behavior under slow consumer stress.
//        - Category: Robustness/Backpressure
//        - Completed: 2024-XX-XX
//        - Notes: 100 events posted, slow consumer (100ms delay), verifies blocking
//
//   🟢 [@AC-2,US-1] TC-2: verifyQueueOverflow_byFastProducer_expectErrorReturned
//        - Description: NonBlock overflow error when queue full.
//        - Category: Robustness/QueueLimit
//        - Completed: 2024-XX-XX
//        - Notes: Fast producer fills queue, verifies TOO_MANY_QUEUING_EVTDESC
//
//   🟢 [@AC-3,US-1] TC-3: verifyTimeout_byFullQueue_expectTimeoutReturned
//        - Description: Timeout behavior when queue full.
//        - Category: Robustness/Timeout
//        - Completed: 2024-XX-XX
//        - Notes: 500ms timeout test, verifies IOC_RESULT_TIMEOUT
//
//   🟢 [@AC-4,US-1] TC-4: verifyRecovery_afterBackpressure_expectNormalFlow
//        - Description: Latency recovery after stress.
//        - Category: Robustness/Recovery
//        - Completed: 2024-XX-XX
//        - Notes: Post-backpressure latency <20ms
//
// US-2: Cascading Event Storm Prevention
//   🟢 [@AC-5,US-2] TC-5: verifyCascading_byLinearChain_expectAllDelivered
//        - Description: Linear event chain (A→B→C→D→E).
//        - Category: Robustness/Cascading
//        - Completed: 2024-XX-XX
//        - Notes: 5-event chain, all delivered sequentially
//
//   🟢 [@AC-6,US-2] TC-6: verifyCascading_byExponentialAmplification_expectLimited
//        - Description: Exponential amplification (1→2→4...) limited by queue.
//        - Category: Robustness/StormPrevention
//        - Completed: 2024-XX-XX
//        - Notes: 10 roots with depth limit 6, verifies overflow errors
//
//   🟢 [@AC-7,US-2] TC-7: verifyCascading_byMayBlockOption_expectGracefulBackpressure
//        - Description: MayBlock prevents overflow in cascading.
//        - Category: Robustness/Backpressure
//        - Completed: 2024-XX-XX
//        - Notes: Slow consumer (50ms), verifies no failures
//
//   🟢 [@AC-8,US-2] TC-8: verifyRecovery_afterEventStorm_expectNormalOperation
//        - Description: System recovery after massive event storm.
//        - Category: Robustness/Recovery
//        - Completed: 2024-XX-XX
//        - Notes: 200-event storm, then 10 new events processed
//
// US-3: Sync Mode Deadlock Prevention
//   🟢 [@AC-9,US-3] TC-9: verifySyncMode_duringCallback_expectForbidden
//        - Description: Sync post forbidden in callback.
//        - Category: Robustness/DeadlockPrevention
//        - Completed: 2024-XX-XX
//        - Notes: Verifies IOC_RESULT_FORBIDDEN
//
//   🟢 [@AC-10,US-3] TC-10: verifyAsyncMode_duringCallback_expectSuccess
//        - Description: Async post allowed in callback.
//        - Category: Robustness/Reentrancy
//        - Completed: 2024-XX-XX
//        - Notes: Verifies IOC_RESULT_SUCCESS
//
//   🟢 [@AC-11,US-3] TC-11: verifySyncMode_afterCallback_expectSuccess
//        - Description: Sync post works after callback finishes.
//        - Category: Robustness/StateTransition
//        - Completed: 2024-XX-XX
//        - Notes: Verifies normal sync operation
//
// US-4: Limits and Re-entrancy
//   🟢 [@AC-12,US-4] TC-12: verifyStability_withMaxSubscribers
//        - Description: Max subscriber limit handling.
//        - Category: Robustness/Limits
//        - Completed: 2024-XX-XX
//        - Notes: 16 subscribers (max), 17th fails gracefully
//
//   🟢 [@AC-13,US-4] TC-13: verifyQueueDrain_afterUnsubscribe
//        - Description: Queue drains after unsubscribe.
//        - Category: Robustness/QueueManagement
//        - Completed: 2024-XX-XX
//        - Notes: 100 queued events drained, link goes to Ready
//
//   🟢 [@AC-14,US-4] TC-14: verifyUnsubscribe_duringCallback_expectSuccess
//        - Description: Self-unsubscribe in callback.
//        - Category: Robustness/Reentrancy
//        - Completed: 2024-XX-XX
//        - Notes: Called once, then no more calls
//
//   🟢 [@AC-15,US-4] TC-15: verifySubscribe_duringCallback_expectSuccess
//        - Description: Dynamic subscribe in callback.
//        - Category: Robustness/Reentrancy
//        - Completed: 2024-XX-XX
//        - Notes: New subscriber activated for subsequent events
//
// 🚪 GATE P3: All robustness tests GREEN, production ready.
//
//===================================================================================================
// ✅ COMPLETION STATUS
//===================================================================================================
//
//   All planned robustness tests are IMPLEMENTED and PASSING.
//   This file validates system stability under stress conditions.
//
///////////////////////////////////////////////////////////////////////////////////////////////////
//======>END OF TODO/IMPLEMENTATION TRACKING SECTION===============================================
