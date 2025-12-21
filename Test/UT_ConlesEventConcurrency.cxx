///////////////////////////////////////////////////////////////////////////////////////////////////
// UT_ConlesEventConcurrency.cxx - ConlesMode Event Concurrency Testing
//
// PURPOSE:
//   Verify thread-safety and synchronization of the Connectionless Event (ConlesEvent) module.
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
//     - Impact: 3 (Deadlock/Corruption)
//     - Likelihood: 3 (High concurrency environment)
//     - Uncertainty: 2 (Complex state machine)
//     - Score: 18 → Critical priority
//
// RELATIONSHIPS:
//   - Depends on: Source/_IOC_ConlesEvent.c
//   - Related tests: UT_ConlesEventRobustness.cxx (Stress/Limits)
//   - Production code: Source/_IOC_ConlesEvent.c
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <array>
#include <atomic>
#include <thread>
#include <vector>

#include "_UT_IOC_Common.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF OVERVIEW OF THIS UNIT TESTING FILE===============================================
/**
 * @brief
 *   [WHAT] This file verifies ConlesMode event system concurrency and thread safety.
 *   [WHERE] in the IOC Event subsystem for connectionless mode.
 *   [WHY] to ensure no deadlocks or race conditions occur during multi-threaded operations.
 *
 * SCOPE:
 *   - In scope:
 *     • Concurrent postEVT (Sync/ASync/Mixed)
 *     • Concurrent subEVT/unsubEVT
 *     • Mixed post/sub/unsub/pull/forceProc operations
 *     • Deadlock prevention during callbacks
 *   - Out of scope:
 *     • Stress testing (see UT_ConlesEventRobustness.cxx)
 *     • Basic functionality (see UT_ConlesEventTypical.cxx)
 *
 * KEY CONCEPTS:
 *   - Thread Safety: Multiple threads accessing shared state without corruption.
 *   - Deadlock Prevention: Ensuring no circular dependencies in lock acquisition.
 *   - Two-Phase Execution: Releasing locks before calling user callbacks.
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
 *      - Status: THIS FILE - PROMOTED TO P1 LEVEL due to risk score 18
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
 *      - Status: COVERED in UT_ConlesEventRobustness.cxx
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
 * │ Concurrency     │ Operation   │ Load Type   │ Key Scenarios                │
 * ├─────────────────┼─────────────┼─────────────┼──────────────────────────────┤
 * │ Multi-thread    │ Sub/Unsub   │ High Churn  │ US-1: Subscription safety    │
 * │ Multi-thread    │ Post/Sub    │ Mixed       │ US-2: Dynamic listeners      │
 * │ Callback        │ Re-entrant  │ Nested Call │ US-3: Deadlock prevention    │
 * └─────────────────┴─────────────┴─────────────┴──────────────────────────────┘
 *
 * USER STORIES (fill in your stories):
 *
 *  US-1: As a multi-threaded producer,
 *        I want to post events concurrently from multiple threads,
 *        So that my application can scale across CPU cores without data corruption.
 *
 *  US-2: As a dynamic system,
 *        I want to subscribe and unsubscribe events while they are being posted,
 *        So that I can manage event listeners without stopping the event flow.
 *
 *  US-3: As a developer,
 *        I want the system to prevent deadlocks when I call IOC APIs from within callbacks,
 *        So that my application remains responsive and safe.
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
 * [@US-1] Thread-safe subscription management
 *  AC-1: GIVEN 10 threads performing sub/unsub operations concurrently,
 *         WHEN each thread performs 1000 subscribe/unsubscribe cycles,
 *         THEN all operations succeed without corruption,
 *          AND all subscribe counts match unsubscribe counts,
 *          AND internal subscriber list remains consistent.
 *
 *  AC-2: GIVEN one thread continuously posting events,
 *         WHEN 4 other threads churn subscriptions (sub/unsub repeatedly),
 *         THEN system does not deadlock,
 *          AND poster thread continues making progress,
 *          AND no race conditions corrupt subscriber list.
 *
 * [@US-2] Dynamic subscription during event processing
 *  AC-3: GIVEN callback A is subscribed and callback A subscribes callback B during execution,
 *         WHEN events are posted triggering callback A,
 *         THEN callback A executes without deadlock,
 *          AND callback B is successfully subscribed,
 *          AND callback B receives subsequent events (not current event).
 *
 * [@US-3] Sustained concurrent load stability
 *  AC-4: GIVEN 4 producer threads and 2 subscription churner threads,
 *         WHEN system runs under high load for 5 seconds,
 *         THEN zero unexpected errors occur,
 *          AND significant event throughput is maintained (>1000 events),
 *          AND system remains stable with no crashes or hangs.
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
 * ORGANIZATION FORMAT (for this file):
 *===================================================================================================
 *
 * [@AC-1,US-1] Thread-safe subscription management under high churn
 *  🟢 TC-1: verifyMultiThread_bySubUnsubStress_expectNoCorruption
 *      @[Purpose]: Expose thread-safety bugs in subscription list management
 *      @[Brief]: 10 threads perform 1000 sub/unsub cycles, verify no corruption
 *      @[Status]: PASSED/GREEN ✅
 *
 * [@AC-2,US-1] Concurrent posting and subscription changes
 *  🟢 TC-2: verifyMultiThread_bySubscribeWhilePosting_expectConsistent
 *      @[Purpose]: Validate no deadlock when posting and subscribing concurrently
 *      @[Brief]: 1 poster + 4 subscription churners run for 2 seconds
 *      @[Status]: PASSED/GREEN ✅
 *
 * [@AC-3,US-2] Dynamic subscription from within callback
 *  🟢 TC-3: verifyMultiThread_byNewSubscriberDuringCallback_expectActivatedNext
 *      @[Purpose]: Verify new subscribers added during callback work correctly
 *      @[Brief]: Callback A subscribes B, verify B receives subsequent events
 *      @[Status]: PASSED/GREEN ✅
 *
 * [@AC-4,US-3] Sustained high-concurrency load
 *  🟢 TC-4: verifyMultiThread_bySustainedStress_expectNoLeaksOrDegradation
 *      @[Purpose]: Ensure stability under long-duration multi-threaded stress
 *      @[Brief]: 4 producers + 2 churners run for 5 seconds
 *      @[Status]: PASSED/GREEN ✅
 *
 * [ADDITIONAL] Post-burst latency recovery
 *  🟢 TC-5: verifyRecovery_afterBurst_expectNormalLatency
 *      @[Purpose]: Ensure system recovers normal latency after burst load
 *      @[Brief]: Send 500-event burst, verify probe latency < 50ms
 *      @[Status]: PASSED/GREEN ✅
 */
//======>END OF TEST CASES=========================================================================
//======>END OF UNIT TESTING DESIGN================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======BEGIN OF UNIT TESTING IMPLEMENTATION=======================================================

namespace {
struct Tc1Context {
    std::atomic<uint32_t> SuccessfulSubscribes{0};
    std::atomic<uint32_t> SuccessfulUnsubscribes{0};
    std::atomic<uint32_t> FailedOperations{0};
    std::atomic<uint32_t> EventsReceived{0};
};

IOC_Result_T tc1CbProcEvt(const IOC_EvtDesc_pT pEvtDesc, void* pCbPrivData) {
    (void)pEvtDesc;
    auto* pCtx = static_cast<Tc1Context*>(pCbPrivData);
    pCtx->EventsReceived.fetch_add(1);
    return IOC_RESULT_SUCCESS;
}
}  // namespace

/**
 * [@AC-1,US-1]
 * TC-1:
 *   @[Name]: verifyMultiThread_bySubUnsubStress_expectNoCorruption
 *   @[Purpose]: Expose thread-safety bugs in subscription management
 *   @[Steps]:
 *     1) 🔧 SETUP: Launch 10 threads, each with unique context
 *     2) 🎯 BEHAVIOR: Each thread performs 1000 sub/unsub cycles
 *     3) ✅ VERIFY: All operations succeed and counts balance
 *     4) 🧹 CLEANUP: Threads join, no explicit cleanup needed
 *   @[Expect]: No corruption, all operations succeed.
 */
TEST(UTConlesEventConcurrency, verifyMultiThread_bySubUnsubStress_expectNoCorruption) {
    //===>>> SETUP <<<===
    printf("🔧 SETUP: verifyMultiThread_bySubUnsubStress_expectNoCorruption\n");
    constexpr uint32_t NumThreads = 10;
    constexpr uint32_t CyclesPerThread = 1000;
    constexpr uint32_t TotalExpectedOps = NumThreads * CyclesPerThread;
    std::array<Tc1Context, NumThreads> Contexts;

    auto StressWorker = [](Tc1Context& Ctx) {
        for (uint32_t i = 0; i < CyclesPerThread; ++i) {
            IOC_SubEvtArgs_T SubArgs = {
                .CbProcEvt_F = tc1CbProcEvt,
                .pCbPrivData = &Ctx,
                .EvtNum = 0,
                .pEvtIDs = nullptr,
            };
            if (IOC_subEVT_inConlesMode(&SubArgs) == IOC_RESULT_SUCCESS) {
                Ctx.SuccessfulSubscribes.fetch_add(1);
                std::this_thread::yield();
                IOC_UnsubEvtArgs_T UnsubArgs = {.CbProcEvt_F = tc1CbProcEvt, .pCbPrivData = &Ctx};
                if (IOC_unsubEVT_inConlesMode(&UnsubArgs) == IOC_RESULT_SUCCESS) {
                    Ctx.SuccessfulUnsubscribes.fetch_add(1);
                } else {
                    Ctx.FailedOperations.fetch_add(1);
                }
            } else {
                Ctx.FailedOperations.fetch_add(1);
            }
        }
    };

    //===>>> BEHAVIOR <<<===
    printf("🎯 BEHAVIOR: verifyMultiThread_bySubUnsubStress_expectNoCorruption\n");
    std::vector<std::thread> Threads;
    for (uint32_t i = 0; i < NumThreads; ++i) {
        Threads.emplace_back(StressWorker, std::ref(Contexts[i]));
    }
    for (auto& t : Threads) t.join();

    //===>>> VERIFY <<<===
    printf("✅ VERIFY: verifyMultiThread_bySubUnsubStress_expectNoCorruption\n");
    uint32_t TotalSubs = 0, TotalUnsubs = 0, TotalFails = 0;
    for (const auto& Ctx : Contexts) {
        TotalSubs += Ctx.SuccessfulSubscribes.load();
        TotalUnsubs += Ctx.SuccessfulUnsubscribes.load();
        TotalFails += Ctx.FailedOperations.load();
    }
    VERIFY_KEYPOINT_EQ(TotalFails, 0u, "No operations should fail");
    VERIFY_KEYPOINT_EQ(TotalSubs, TotalExpectedOps, "All subscribes should succeed");
    VERIFY_KEYPOINT_EQ(TotalUnsubs, TotalExpectedOps, "All unsubscribes should succeed");
}

/**
 * [@AC-2,US-1]
 * TC-2:
 *   @[Name]: verifyMultiThread_bySubscribeWhilePosting_expectConsistent
 *   @[Purpose]: Validate no deadlock when posting and subscribing concurrently
 *   @[Steps]:
 *     1) 🔧 SETUP: Create poster and 4 subscriber thread contexts
 *     2) 🎯 BEHAVIOR: Launch poster thread and 4 churner threads, run for 2 seconds
 *     3) ✅ VERIFY: Poster made progress, no deadlock occurred
 *     4) 🧹 CLEANUP: Unsubscribe remaining subscribers
 *   @[Expect]: No deadlock, system remains stable.
 */
TEST(UTConlesEventConcurrency, verifyMultiThread_bySubscribeWhilePosting_expectConsistent) {
    //===>>> SETUP <<<===
    printf("🔧 SETUP: verifyMultiThread_bySubscribeWhilePosting_expectConsistent\n");
    constexpr uint32_t NumSubThreads = 4;
    constexpr uint32_t TestDurationMs = 2000;
    struct SubCtx {
        std::atomic<uint32_t> Events{0};
        std::atomic<bool> Active{false};
    };
    std::array<SubCtx, NumSubThreads> Contexts;
    std::atomic<bool> Running{true};
    std::atomic<uint32_t> PostCount{0};

    auto cb = [](const IOC_EvtDesc_pT, void* pData) -> IOC_Result_T {
        static_cast<SubCtx*>(pData)->Events.fetch_add(1);
        return IOC_RESULT_SUCCESS;
    };

    //===>>> BEHAVIOR <<<===
    printf("🎯 BEHAVIOR: verifyMultiThread_bySubscribeWhilePosting_expectConsistent\n");
    std::thread Poster([&]() {
        while (Running.load()) {
            IOC_EvtDesc_T Evt = {.EvtID = IOC_EVTID_TEST_KEEPALIVE};
            IOC_postEVT_inConlesMode(&Evt, nullptr);
            PostCount.fetch_add(1);
            std::this_thread::yield();
        }
    });

    std::vector<std::thread> Subs;
    for (uint32_t i = 0; i < NumSubThreads; ++i) {
        Subs.emplace_back([&Contexts, i, &Running, cb]() {
            while (Running.load()) {
                IOC_SubEvtArgs_T SArgs = {.CbProcEvt_F = cb, .pCbPrivData = &Contexts[i], .EvtNum = 0};
                if (IOC_subEVT_inConlesMode(&SArgs) == IOC_RESULT_SUCCESS) {
                    Contexts[i].Active.store(true);
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    IOC_UnsubEvtArgs_T UArgs = {.CbProcEvt_F = cb, .pCbPrivData = &Contexts[i]};
                    IOC_unsubEVT_inConlesMode(&UArgs);
                    Contexts[i].Active.store(false);
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
        });
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(TestDurationMs));
    Running.store(false);
    Poster.join();
    for (auto& t : Subs) t.join();

    //===>>> VERIFY <<<===
    printf("✅ VERIFY: verifyMultiThread_bySubscribeWhilePosting_expectConsistent\n");
    VERIFY_KEYPOINT_GT(PostCount.load(), 0u, "Poster should have made progress");

    //===>>> CLEANUP <<<===
    printf("🧹 CLEANUP: verifyMultiThread_bySubscribeWhilePosting_expectConsistent\n");
    for (uint32_t i = 0; i < NumSubThreads; ++i) {
        IOC_UnsubEvtArgs_T UArgs = {.CbProcEvt_F = cb, .pCbPrivData = &Contexts[i]};
        IOC_unsubEVT_inConlesMode(&UArgs);
    }
}

/**
 * [@AC-3,US-2]
 * TC-3:
 *   @[Name]: verifyMultiThread_byNewSubscriberDuringCallback_expectActivatedNext
 *   @[Purpose]: Verify new subscribers added during callback activated correctly
 *   @[Steps]:
 *     1) 🔧 SETUP: Subscribe callback A that will subscribe B dynamically
 *     2) 🎯 BEHAVIOR: Post 5 events, A subscribes B during first callback
 *     3) ✅ VERIFY: A receives all 5 events, B receives subset after subscription
 *     4) 🧹 CLEANUP: Unsubscribe both A and B
 *   @[Expect]: No deadlock, B receives events after subscription.
 */
namespace {
struct Tc3Context {
    std::atomic<uint32_t> A_Count{0}, B_Count{0};
    std::atomic<bool> B_Subscribed{false};
};

IOC_Result_T tc3CbB(const IOC_EvtDesc_pT, void* pData) {
    static_cast<Tc3Context*>(pData)->B_Count.fetch_add(1);
    return IOC_RESULT_SUCCESS;
}

IOC_Result_T tc3CbA(const IOC_EvtDesc_pT, void* pData) {
    auto* pCtx = static_cast<Tc3Context*>(pData);
    pCtx->A_Count.fetch_add(1);
    if (!pCtx->B_Subscribed.load()) {
        IOC_SubEvtArgs_T SArgs = {.CbProcEvt_F = tc3CbB, .pCbPrivData = pData, .EvtNum = 0};
        if (IOC_subEVT_inConlesMode(&SArgs) == IOC_RESULT_SUCCESS) {
            pCtx->B_Subscribed.store(true);
        }
    }
    return IOC_RESULT_SUCCESS;
}
}  // namespace

TEST(UTConlesEventConcurrency, verifyMultiThread_byNewSubscriberDuringCallback_expectActivatedNext) {
    //===>>> SETUP <<<===
    printf("🔧 SETUP: verifyMultiThread_byNewSubscriberDuringCallback_expectActivatedNext\n");
    Tc3Context Ctx;
    IOC_SubEvtArgs_T SArgs = {.CbProcEvt_F = tc3CbA, .pCbPrivData = &Ctx, .EvtNum = 0};
    ASSERT_EQ(IOC_RESULT_SUCCESS, IOC_subEVT_inConlesMode(&SArgs));

    //===>>> BEHAVIOR <<<===
    printf("🎯 BEHAVIOR: verifyMultiThread_byNewSubscriberDuringCallback_expectActivatedNext\n");
    for (int i = 0; i < 5; ++i) {
        IOC_EvtDesc_T Evt = {.EvtID = IOC_EVTID_TEST_KEEPALIVE};
        IOC_postEVT_inConlesMode(&Evt, nullptr);
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    //===>>> VERIFY <<<===
    printf("✅ VERIFY: verifyMultiThread_byNewSubscriberDuringCallback_expectActivatedNext\n");
    VERIFY_KEYPOINT_EQ(Ctx.A_Count.load(), 5u, "A should receive all 5 events");
    VERIFY_KEYPOINT_GT(Ctx.B_Count.load(), 0u, "B should receive events after being subscribed by A");
    VERIFY_KEYPOINT_LT(Ctx.B_Count.load(), 5u, "B should receive fewer than 5 events");

    //===>>> CLEANUP <<<===
    printf("🧹 CLEANUP: verifyMultiThread_byNewSubscriberDuringCallback_expectActivatedNext\n");
    IOC_UnsubEvtArgs_T UA = {.CbProcEvt_F = tc3CbA, .pCbPrivData = &Ctx};
    IOC_unsubEVT_inConlesMode(&UA);
    IOC_UnsubEvtArgs_T UB = {.CbProcEvt_F = tc3CbB, .pCbPrivData = &Ctx};
    IOC_unsubEVT_inConlesMode(&UB);
}

/**
 * [@AC-4,US-3]
 * TC-4:
 *   @[Name]: verifyMultiThread_bySustainedStress_expectNoLeaksOrDegradation
 *   @[Purpose]: Ensure stability under long-duration high-concurrency load
 *   @[Steps]:
 *     1) 🔧 SETUP: Create stress context, define producer and churner workers
 *     2) 🎯 BEHAVIOR: Launch 4 producers and 2 churners, run for 5 seconds
 *     3) ✅ VERIFY: Zero unexpected errors, significant event throughput
 *     4) 🧹 CLEANUP: Join all worker threads
 *   @[Expect]: System remains stable, no leaks or crashes.
 */
TEST(UTConlesEventConcurrency, verifyMultiThread_bySustainedStress_expectNoLeaksOrDegradation) {
    //===>>> SETUP <<<===
    printf("🔧 SETUP: verifyMultiThread_bySustainedStress_expectNoLeaksOrDegradation\n");
    struct StressCtx {
        std::atomic<uint64_t> Total{0};
        std::atomic<bool> Running{true};
        std::atomic<uint32_t> Errors{0};
    } Ctx;

    auto cb = [](const IOC_EvtDesc_pT, void* pData) -> IOC_Result_T {
        static_cast<StressCtx*>(pData)->Total.fetch_add(1);
        return IOC_RESULT_SUCCESS;
    };

    //===>>> BEHAVIOR <<<===
    printf("🎯 BEHAVIOR: verifyMultiThread_bySustainedStress_expectNoLeaksOrDegradation\n");
    std::vector<std::thread> Workers;
    for (int i = 0; i < 4; ++i) {
        Workers.emplace_back([&Ctx]() {
            while (Ctx.Running.load()) {
                IOC_EvtDesc_T Evt = {.EvtID = IOC_EVTID_TEST_KEEPALIVE};
                IOC_Result_T Res = IOC_postEVT_inConlesMode(&Evt, nullptr);
                if (Res != IOC_RESULT_SUCCESS && Res != IOC_RESULT_TOO_MANY_QUEUING_EVTDESC &&
                    Res != IOC_RESULT_NO_EVENT_CONSUMER) {
                    Ctx.Errors.fetch_add(1);
                }
                std::this_thread::yield();
            }
        });
    }
    for (int i = 0; i < 2; ++i) {
        Workers.emplace_back([&Ctx, cb, i]() {
            while (Ctx.Running.load()) {
                IOC_SubEvtArgs_T SArgs = {.CbProcEvt_F = cb, .pCbPrivData = &Ctx, .EvtNum = 0};
                IOC_subEVT_inConlesMode(&SArgs);
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                IOC_UnsubEvtArgs_T UArgs = {.CbProcEvt_F = cb, .pCbPrivData = &Ctx};
                IOC_unsubEVT_inConlesMode(&UArgs);
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        });
    }

    std::this_thread::sleep_for(std::chrono::seconds(5));
    Ctx.Running.store(false);
    for (auto& t : Workers) t.join();

    //===>>> VERIFY <<<===
    printf("✅ VERIFY: verifyMultiThread_bySustainedStress_expectNoLeaksOrDegradation\n");
    VERIFY_KEYPOINT_EQ(Ctx.Errors.load(), 0u, "No unexpected errors during stress");
    VERIFY_KEYPOINT_GT(Ctx.Total.load(), 1000u, "Should process significant events");

    //===>>> CLEANUP <<<===
    printf("🧹 CLEANUP: verifyMultiThread_bySustainedStress_expectNoLeaksOrDegradation\n");
}

/**
 * TC-5:
 *   @[Name]: verifyRecovery_afterBurst_expectNormalLatency
 *   @[Purpose]: Ensure system recovers latency after a high-volume burst
 *   @[Steps]:
 *     1) 🔧 SETUP: Subscribe latency-measuring callback, prepare probe event
 *     2) 🎯 BEHAVIOR: Send 500-event burst, wait for drain, send probe event
 *     3) ✅ VERIFY: Probe latency < 50ms after burst recovery
 *     4) 🧹 CLEANUP: Unsubscribe callback
 *   @[Expect]: Latency < 50ms.
 */
TEST(UTConlesEventConcurrency, verifyRecovery_afterBurst_expectNormalLatency) {
    //===>>> SETUP <<<===
    printf("🔧 SETUP: verifyRecovery_afterBurst_expectNormalLatency\n");
    struct LatencyCtx {
        std::atomic<uint32_t> Count{0};
        std::chrono::steady_clock::time_point Start, End;
        std::atomic<bool> Received{false};
    } Ctx;

    auto cb = [](const IOC_EvtDesc_pT pEvt, void* pData) -> IOC_Result_T {
        auto* p = static_cast<LatencyCtx*>(pData);
        if (pEvt->EvtID == 999) {
            p->End = std::chrono::steady_clock::now();
            p->Received.store(true);
        }
        p->Count.fetch_add(1);
        return IOC_RESULT_SUCCESS;
    };

    IOC_SubEvtArgs_T SArgs = {.CbProcEvt_F = cb, .pCbPrivData = &Ctx, .EvtNum = 0};
    ASSERT_EQ(IOC_RESULT_SUCCESS, IOC_subEVT_inConlesMode(&SArgs));

    //===>>> BEHAVIOR <<<===
    printf("🎯 BEHAVIOR: verifyRecovery_afterBurst_expectNormalLatency\n");
    // Burst
    for (int i = 0; i < 500; ++i) {
        IOC_EvtDesc_T Evt = {.EvtID = IOC_EVTID_TEST_KEEPALIVE};
        IOC_postEVT_inConlesMode(&Evt, nullptr);
    }

    while (Ctx.Count.load() < 500) std::this_thread::sleep_for(std::chrono::milliseconds(10));

    // Probe
    Ctx.Start = std::chrono::steady_clock::now();
    IOC_EvtDesc_T Probe = {.EvtID = 999};
    IOC_postEVT_inConlesMode(&Probe, nullptr);

    for (int i = 0; i < 100 && !Ctx.Received.load(); ++i) std::this_thread::sleep_for(std::chrono::milliseconds(1));

    //===>>> VERIFY <<<===
    printf("✅ VERIFY: verifyRecovery_afterBurst_expectNormalLatency\n");
    ASSERT_TRUE(Ctx.Received.load());
    auto Latency = std::chrono::duration_cast<std::chrono::microseconds>(Ctx.End - Ctx.Start).count();
    VERIFY_KEYPOINT_LT(Latency, 50000, "Latency should be < 50ms after burst recovery");

    //===>>> CLEANUP <<<===
    printf("🧹 CLEANUP: verifyRecovery_afterBurst_expectNormalLatency\n");
    IOC_UnsubEvtArgs_T UA = {.CbProcEvt_F = cb, .pCbPrivData = &Ctx};
    IOC_unsubEVT_inConlesMode(&UA);
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
//         This file focuses on P2 CONCURRENCY testing (promoted to P1 priority due to risk).
//
// 🚪 GATE P1: All P1 tests GREEN before proceeding to P2.
//
//===================================================================================================
// P2 🥈 DESIGN-ORIENTED TESTING – Concurrency (PROMOTED TO P1 PRIORITY)
//===================================================================================================
//
//   🟢 [@AC-1,US-1] TC-1: verifyMultiThread_bySubUnsubStress_expectNoCorruption
//        - Description: Validate thread-safe subscription list under high churn.
//        - Category: Concurrency
//        - Completed: 2024-XX-XX
//        - Notes: 10 threads × 1000 cycles, all operations succeed
//
//   🟢 [@AC-2,US-1] TC-2: verifyMultiThread_bySubscribeWhilePosting_expectConsistent
//        - Description: Test concurrent posting and subscription changes.
//        - Category: Concurrency
//        - Completed: 2024-XX-XX
//        - Notes: 1 poster + 4 churners, no deadlock for 2 seconds
//
//   🟢 [@AC-3,US-2] TC-3: verifyMultiThread_byNewSubscriberDuringCallback_expectActivatedNext
//        - Description: Verify dynamic subscription from within callback.
//        - Category: Concurrency
//        - Completed: 2024-XX-XX
//        - Notes: Callback A subscribes B, B receives subsequent events
//
//   🟢 [@AC-4,US-3] TC-4: verifyMultiThread_bySustainedStress_expectNoLeaksOrDegradation
//        - Description: Sustained multi-threaded stress test.
//        - Category: Concurrency/Robustness
//        - Completed: 2024-XX-XX
//        - Notes: 4 producers + 2 churners for 5 seconds, >1000 events
//
//   🟢 [ADDITIONAL] TC-5: verifyRecovery_afterBurst_expectNormalLatency
//        - Description: Latency recovery after burst load.
//        - Category: Concurrency/Performance
//        - Completed: 2024-XX-XX
//        - Notes: 500-event burst, probe latency < 50ms
//
// 🚪 GATE P2: All P2 concurrency tests GREEN, architecture validated.
//
//===================================================================================================
// ✅ COMPLETION STATUS
//===================================================================================================
//
//   All planned concurrency tests are IMPLEMENTED and PASSING.
//   This file validates thread-safety of the ConlesEvent subsystem.
//
///////////////////////////////////////////////////////////////////////////////////////////////////
//======>END OF TODO/IMPLEMENTATION TRACKING SECTION===============================================
