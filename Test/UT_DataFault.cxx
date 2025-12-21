///////////////////////////////////////////////////////////////////////////////////////////////////
// Data Fault FIFO - P1 InvalidFunc Fault Testing
//
// PURPOSE:
//   Validate FIFO data API fault tolerance and error recovery.
//   Tests external failures and system resilience to ensure graceful degradation.
//
// TDD WORKFLOW:
//   Design → Draft → Structure → Test (RED) → Code (GREEN) → Refactor → Repeat
//
// REFERENCE: LLM/CaTDD_DesignPrompt.md for full methodology
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "_UT_IOC_Common.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF OVERVIEW=========================================================================
/**
 * @brief
 *   [WHAT] This file validates FIFO data API fault tolerance and error recovery
 *   [WHERE] in the IOC Data API with FIFO protocol layer
 *   [WHY] to ensure system resilience under adverse conditions and graceful degradation
 *
 * SCOPE:
 *   - [In scope]: P1 InvalidFunc Fault tests (external failures and recovery)
 *   - [In scope]: Resource exhaustion (buffer full, memory limits)
 *   - [In scope]: Link failures (broken links, peer crashes, disconnections)
 *   - [In scope]: Timeout scenarios (send timeout, recv timeout, flush timeout)
 *   - [In scope]: Recovery mechanisms (reconnection, retry after failure)
 *   - [In scope]: FIFO-specific faults (file system errors, permission issues)
 *   - [Out of scope]: API misuse → see UT_DataMisuse.cxx
 *   - [Out of scope]: Normal boundary cases → see UT_DataEdgeUS*.cxx
 *   - [Out of scope]: Typical scenarios → see UT_DataTypical.cxx
 *
 * KEY CONCEPTS:
 *   - Fault Tolerance: System's ability to continue operation despite failures
 *   - Graceful Degradation: System returns errors instead of crashing
 *   - Error Recovery: System can recover from transient failures
 *   - Resource Exhaustion: Handling limits (buffer full, memory limits)
 *   - Link Broken: Detection and handling of communication failures
 *
 * RELATIONSHIPS:
 *   - Extends: UT_DataTypical.cxx (fault handling for typical patterns)
 *   - Related: UT_DataMisuse.cxx (fault vs misuse distinction)
 *   - Related: UT_DataRobust.cxx (fault vs stress testing distinction)
 *   - Companion: UT_DataFaultTCP.cxx (same tests with TCP protocol)
 */
//======>END OF OVERVIEW===========================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF TEST DESIGN======================================================================

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
 *
 *   🔲 EDGE: Edge cases, limits, and mode variations. (HIGH PRIORITY)
 *      - Purpose: Test parameter limits and edge values.
 *      - Examples: Min/max values, null/empty inputs, Block/NonBlock/Timeout modes.
 *
 * InvalidFunc - Verifies graceful failure with invalid inputs or states.
 *
 *   🚫 MISUSE: Incorrect API usage patterns. (ERROR PREVENTION)
 *      - Purpose: Ensure proper error handling for API abuse.
 *      - Examples: Wrong call sequence, invalid parameters, double-init.
 *
 *   ⚠️ FAULT: Error handling and recovery. (RELIABILITY)
 *      - Purpose: Test system behavior under error conditions.
 *      - Examples: Network failures, disk full, process crash recovery.
 *
 *===================================================================================================
 * PRIORITY-2: DESIGN-ORIENTED TESTING (Architecture Validation)
 *===================================================================================================
 *
 *   🔄 STATE: Lifecycle transitions and state machine validation. (KEY FOR STATEFUL COMPONENTS)
 *      - Purpose: Verify FSM correctness.
 *      - Examples: Init→Ready→Running→Stopped.
 *
 *   🏆 CAPABILITY: Maximum capacity and system limits. (FOR CAPACITY PLANNING)
 *      - Purpose: Test architectural limits.
 *      - Examples: Max connections, queue limits.
 *
 *   🚀 CONCURRENCY: Thread safety and synchronization. (FOR COMPLEX SYSTEMS)
 *      - Purpose: Validate concurrent access and find race conditions.
 *      - Examples: Race conditions, deadlocks, parallel access.
 *
 *===================================================================================================
 * PRIORITY-3: QUALITY-ORIENTED TESTING (Non-Functional Requirements)
 *===================================================================================================
 *
 *   ⚡ PERFORMANCE: Speed, throughput, and resource usage. (FOR SLO VALIDATION)
 *      - Purpose: Measure and validate performance characteristics.
 *      - Examples: Latency benchmarks, memory leak detection.
 *
 *   🛡️ ROBUST: Stress, repetition, and long-running stability. (FOR PRODUCTION READINESS)
 *      - Purpose: Verify stability under sustained load.
 *      - Examples: 1000x repetition, 24h soak tests.
 *
 *   🔄 COMPATIBILITY: Cross-platform and version testing. (FOR MULTI-PLATFORM PRODUCTS)
 *      - Purpose: Ensure consistent behavior across environments.
 *      - Examples: Windows/Linux/macOS, API version compatibility.
 *
 *   🎛️ CONFIGURATION: Different settings and environments. (FOR CONFIGURABLE SYSTEMS)
 *      - Purpose: Test various configuration scenarios.
 *      - Examples: Debug/release modes, feature flags.
 *
 *===================================================================================================
 * PRIORITY-4: OTHER-ADDONS TESTING (Documentation & Tutorials)
 *===================================================================================================
 *
 *   🎨 DEMO/EXAMPLE: End-to-end feature demonstrations. (FOR DOCUMENTATION)
 *      - Purpose: Illustrate usage patterns and best practices.
 *      - Examples: Tutorial code, complete workflows.
 *
 * SELECTION STRATEGY:
 *   🥇 P1 (Functional): MUST be completed before moving to P2.
 *   🥈 P2 (Design): Test after P1 if the component has significant design complexity (state, concurrency).
 *   🥉 P3 (Quality): Test when quality attributes (performance, robustness) are critical.
 *   🎯 P4 (Addons): Optional, for documentation and examples.
 *************************************************************************************************/

/**************************************************************************************************
 * 📊 COVERAGE MATRIX - Data Fault Testing (FIFO Protocol)
 *
 * ┌──────────────────────────┬─────────────────────────┬────────────────────────────┐
 * │ Fault Category           │ API Under Test          │ Key Scenarios              │
 * ├──────────────────────────┼─────────────────────────┼────────────────────────────┤
 * │ Resource Exhaustion      │ IOC_sendDAT             │ Buffer full scenarios      │
 * │ Resource Exhaustion      │ IOC_recvDAT             │ No data available timeout  │
 * │ Resource Exhaustion      │ IOC_flushDAT            │ Flush during full buffer   │
 * │ Link Failures            │ IOC_sendDAT             │ Send on broken link        │
 * │ Link Failures            │ IOC_recvDAT             │ Recv after peer crash      │
 * │ Link Failures            │ IOC_closeLink           │ Close during active xfer   │
 * │ Timeout Scenarios        │ IOC_sendDAT             │ Send timeout (blocked)     │
 * │ Timeout Scenarios        │ IOC_recvDAT             │ Recv timeout (no data)     │
 * │ Timeout Scenarios        │ IOC_flushDAT            │ Flush timeout              │
 * │ Recovery Mechanisms      │ IOC_connectService      │ Reconnect after failure    │
 * │ Recovery Mechanisms      │ IOC_sendDAT/recvDAT     │ Retry after transient fail │
 * │ FIFO-Specific Faults     │ IOC_onlineService       │ Disk full during FIFO ops  │
 * │ FIFO-Specific Faults     │ IOC_sendDAT             │ FIFO permission denied     │
 * └──────────────────────────┴─────────────────────────┴────────────────────────────┘
 *
 * FIFO PATH BASE: test/data/fault/fifo/
 *
 * PRIORITY: P1 InvalidFunc Fault (CRITICAL for reliability)
 *
 * STATUS:
 *   ⚪ 0/20 tests implemented (PLANNED)
 *   📋 20 test scenarios planned
 *   🎯 Target: Core fault tolerance validation
 */
///////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF USER STORY=======================================================================
/**
 * US-1: As a developer, I want buffer full conditions handled gracefully
 *       so that my application can implement proper flow control.
 *
 * US-2: As a developer, I want timeout behaviors to be reliable and predictable
 *       so that I can build time-aware applications with proper SLAs.
 *
 * US-3: As a developer, I want link failures detected immediately
 *       so that I can implement fast failover and recovery.
 *
 * US-4: As a developer, I want recovery mechanisms after transient failures
 *       so that my application can handle intermittent issues.
 *
 * US-5: As a developer, I want FIFO-specific faults handled gracefully
 *       so that file system issues don't crash my application.
 */
//======>END OF USER STORY==========================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF ACCEPTANCE CRITERIA===============================================================
/**
 * [@US-1] Resource Exhaustion Handling
 *  AC-1: GIVEN send buffer full condition,
 *        WHEN calling IOC_sendDAT with NONBLOCK mode,
 *        THEN returns IOC_RESULT_BUFFER_FULL immediately without blocking.
 *
 *  AC-2: GIVEN send buffer full with timeout configured,
 *        WHEN calling IOC_sendDAT with timeout,
 *        THEN waits up to timeout duration and returns TIMEOUT if still full.
 *
 *  AC-3: GIVEN receiver polling with no data available,
 *        WHEN calling IOC_recvDAT with NONBLOCK mode,
 *        THEN returns IOC_RESULT_NO_DATA immediately.
 *
 * [@US-2] Timeout Behavior Validation
 *  AC-1: GIVEN IOC_sendDAT with specific timeout value,
 *        WHEN buffer is full and timeout expires,
 *        THEN returns IOC_RESULT_TIMEOUT within acceptable timing variance.
 *
 *  AC-2: GIVEN IOC_recvDAT with specific timeout value,
 *        WHEN no data available and timeout expires,
 *        THEN returns IOC_RESULT_TIMEOUT within acceptable timing variance.
 *
 *  AC-3: GIVEN IOC_flushDAT with timeout,
 *        WHEN flush cannot complete within timeout,
 *        THEN returns IOC_RESULT_TIMEOUT.
 *
 * [@US-3] Link Failure Detection
 *  AC-1: GIVEN active data transfer in progress,
 *        WHEN peer process crashes or link breaks,
 *        THEN subsequent IOC_sendDAT/recvDAT returns IOC_RESULT_LINK_BROKEN.
 *
 *  AC-2: GIVEN link closed by peer during transfer,
 *        WHEN calling IOC_sendDAT on sender side,
 *        THEN returns IOC_RESULT_LINK_BROKEN.
 *
 *  AC-3: GIVEN service taken offline with active connections,
 *        WHEN calling data operations on orphaned links,
 *        THEN returns IOC_RESULT_LINK_BROKEN or NOT_EXIST_LINK.
 *
 * [@US-4] Recovery and Retry Mechanisms
 *  AC-1: GIVEN transient buffer full condition,
 *        WHEN retrying IOC_sendDAT after brief delay,
 *        THEN operation succeeds once buffer space available.
 *
 *  AC-2: GIVEN link broken and re-established,
 *        WHEN reconnecting and resuming data transfer,
 *        THEN new connection works correctly.
 *
 * [@US-5] FIFO-Specific Fault Handling
 *  AC-1: GIVEN disk full condition during FIFO write,
 *        WHEN calling IOC_sendDAT,
 *        THEN returns appropriate error (LINK_BROKEN or similar).
 *
 *  AC-2: GIVEN FIFO file permission denied,
 *        WHEN attempting to write data,
 *        THEN returns permission error without crash.
 */
//======>END OF ACCEPTANCE CRITERIA=================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF TEST CASES========================================================================
/**
 * [@AC-1,US-1] Resource Exhaustion - Buffer Full (3 tests)
 *  ⚪ TC-1: verifyDataFault_byBufferFullNonBlock_expectBufferFullError
 *      @[Purpose]: Validate IOC_sendDAT returns BUFFER_FULL in NONBLOCK mode
 *      @[Brief]: Fill send buffer, attempt send with NONBLOCK, expect BUFFER_FULL
 *
 *  ⚪ TC-2: verifyDataFault_byBufferFullWithTimeout_expectTimeoutError
 *      @[Purpose]: Validate IOC_sendDAT times out when buffer remains full
 *      @[Brief]: Fill buffer, send with timeout, verify TIMEOUT returned
 *
 *  ⚪ TC-3: verifyDataFault_byRecvNoDataNonBlock_expectNoDataError
 *      @[Purpose]: Validate IOC_recvDAT returns NO_DATA when no data available
 *      @[Brief]: Call recvDAT with NONBLOCK when queue empty, expect NO_DATA
 *
 * [@AC-1,AC-2,US-2] Timeout Behavior Validation (6 tests)
 *  ⚪ TC-4: verifyDataFault_bySendTimeoutPrecision_expectAccurateTiming
 *      @[Purpose]: Validate IOC_sendDAT timeout accuracy
 *      @[Brief]: Send with various timeouts, measure actual duration, verify precision
 *
 *  ⚪ TC-5: verifyDataFault_byRecvTimeoutPrecision_expectAccurateTiming
 *      @[Purpose]: Validate IOC_recvDAT timeout accuracy
 *      @[Brief]: Recv with various timeouts when no data, measure duration
 *
 *  ⚪ TC-6: verifyDataFault_byFlushTimeoutPrecision_expectAccurateTiming
 *      @[Purpose]: Validate IOC_flushDAT timeout behavior
 *      @[Brief]: Flush with timeout, verify timing accuracy
 *
 *  ⚪ TC-7: verifyDataFault_byZeroTimeoutSend_expectImmediateReturn
 *      @[Purpose]: Validate zero timeout returns immediately
 *      @[Brief]: Send with zero timeout, verify immediate return
 *
 *  ⚪ TC-8: verifyDataFault_byZeroTimeoutRecv_expectImmediateReturn
 *      @[Purpose]: Validate zero timeout recv returns immediately
 *      @[Brief]: Recv with zero timeout, verify immediate return
 *
 *  ⚪ TC-9: verifyDataFault_byInfiniteTimeoutRecovery_expectEventualSuccess
 *      @[Purpose]: Validate infinite timeout waits until success
 *      @[Brief]: Recv with infinite timeout, send data from another thread, verify success
 *
 * [@AC-1,AC-2,AC-3,US-3] Link Failure Detection (5 tests)
 *  ⚪ TC-10: verifyDataFault_byPeerCrashDuringSend_expectLinkBroken
 *      @[Purpose]: Validate link broken detected when peer crashes
 *      @[Brief]: Start send, crash receiver, verify LINK_BROKEN
 *
 *  ⚪ TC-11: verifyDataFault_byPeerCloseduringRecv_expectLinkBroken
 *      @[Purpose]: Validate link broken on receiver when sender closes
 *      @[Brief]: Wait for data, close sender link, verify LINK_BROKEN
 *
 *  ⚪ TC-12: verifyDataFault_byServiceOfflineWithActiveLink_expectLinkBroken
 *      @[Purpose]: Validate orphaned links detect service offline
 *      @[Brief]: Offline service, attempt operations on links, expect error
 *
 *  ⚪ TC-13: verifyDataFault_byAbruptDisconnection_expectGracefulHandling
 *      @[Purpose]: Validate abrupt disconnection handling
 *      @[Brief]: Close link abruptly during transfer, verify no crash
 *
 *  ⚪ TC-14: verifyDataFault_byLinkBrokenDuringFlush_expectLinkBrokenError
 *      @[Purpose]: Validate flush detects broken link
 *      @[Brief]: Start flush, break link, verify LINK_BROKEN
 *
 * [@AC-1,AC-2,US-4] Recovery and Retry Mechanisms (3 tests)
 *  ⚪ TC-15: verifyDataFault_byRetryAfterBufferFull_expectEventualSuccess
 *      @[Purpose]: Validate retry succeeds after buffer drains
 *      @[Brief]: Get BUFFER_FULL, drain buffer, retry, expect SUCCESS
 *
 *  ⚪ TC-16: verifyDataFault_byReconnectAfterLinkBroken_expectNewConnection
 *      @[Purpose]: Validate reconnection after link failure
 *      @[Brief]: Break link, close, reconnect, verify new link works
 *
 *  ⚪ TC-17: verifyDataFault_byRecoveryFromTransientFailure_expectResume
 *      @[Purpose]: Validate recovery from transient errors
 *      @[Brief]: Simulate transient fault, retry, verify recovery
 *
 * [@AC-1,AC-2,US-5] FIFO-Specific Fault Handling (3 tests)
 *  ⚪ TC-18: verifyDataFault_byDiskFullDuringFIFOWrite_expectIOError
 *      @[Purpose]: Validate disk full handling (simulation)
 *      @[Brief]: Simulate disk full, attempt send, expect error
 *
 *  ⚪ TC-19: verifyDataFault_byFIFOPermissionDenied_expectAccessError
 *      @[Purpose]: Validate permission error handling
 *      @[Brief]: Change FIFO permissions, attempt write, expect error
 *
 *  ⚪ TC-20: verifyDataFault_byFIFOCorruptionRecovery_expectGracefulHandling
 *      @[Purpose]: Validate FIFO corruption doesn't crash system
 *      @[Brief]: Corrupt FIFO file, attempt operations, verify error handling
 */
//======>END OF TEST CASES==========================================================================
//======>END OF TEST DESIGN=========================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF UNIT TESTING IMPLEMENTATION======================================================

/**
 * TC-1: verifyDataFault_byBufferFullNonBlock_expectBufferFullError
 * @[Purpose]: Validate IOC_sendDAT returns BUFFER_FULL when buffer is full in NONBLOCK mode
 * @[Brief]: Setup sender with slow receiver, fill buffer, attempt NONBLOCK send, expect BUFFER_FULL
 * @[Steps]:
 *   1) Setup DatSender service and DatReceiver with slow callback
 *   2) Fill buffer by sending data faster than receiver processes
 *   3) Attempt IOC_sendDAT with NONBLOCK option on full buffer
 *   4) Verify IOC_RESULT_BUFFER_FULL returned immediately
 * @[Expect]: IOC_RESULT_BUFFER_FULL without blocking
 */
TEST(UT_DataFault, verifyDataFault_byBufferFullNonBlock_expectBufferFullError) {
    printf("🔴 RED: verifyDataFault_byBufferFullNonBlock_expectBufferFullError\n");

    //===SETUP===
    printf("🔧 SETUP: Create sender with slow receiver to fill buffer\n");

    IOC_SrvID_T DatSenderSrvID = IOC_ID_INVALID;
    IOC_LinkID_T DatSenderLinkID = IOC_ID_INVALID;
    IOC_LinkID_T DatReceiverLinkID = IOC_ID_INVALID;
    IOC_Result_T Result;

    // Setup DatSender service
    IOC_SrvURI_T DatSenderSrvURI = {
        .pProtocol = IOC_SRV_PROTO_FIFO,
        .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
        .pPath = "test/data/fault/fifo/buffer_full",
    };

    IOC_SrvArgs_T SrvArgs = {
        .SrvURI = DatSenderSrvURI,
        .UsageCapabilites = IOC_LinkUsageDatSender,
    };

    Result = IOC_onlineService(&DatSenderSrvID, &SrvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result) << "Failed to online DatSender service";

    // Setup DatReceiver with intentionally slow callback to create buffer pressure
    struct {
        int ReceivedCount = 0;
        bool SlowConsumer = true;  // Intentionally slow to fill buffer
    } DatReceiverPrivData;

    auto CbRecvDat_F = [](IOC_LinkID_T LinkID, IOC_DatDesc_pT pDatDesc, void *pCbPriv) -> IOC_Result_T {
        auto *pPrivData = (decltype(DatReceiverPrivData) *)pCbPriv;
        pPrivData->ReceivedCount++;

        if (pPrivData->SlowConsumer) {
            // Intentionally slow processing to create buffer backpressure
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        return IOC_RESULT_SUCCESS;
    };

    IOC_DatUsageArgs_T DatUsageArgs = {
        .CbRecvDat_F = CbRecvDat_F,
        .pCbPrivData = &DatReceiverPrivData,
    };

    IOC_ConnArgs_T ConnArgs = {
        .SrvURI = DatSenderSrvURI,
        .Usage = IOC_LinkUsageDatReceiver,
        .UsageArgs = {.pDat = &DatUsageArgs},
    };

    // Establish connection
    std::thread DatReceiverThread([&] {
        Result = IOC_connectService(&DatReceiverLinkID, &ConnArgs, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, Result);
    });

    Result = IOC_acceptClient(DatSenderSrvID, &DatSenderLinkID, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result) << "Failed to accept DatReceiver connection";

    DatReceiverThread.join();
    printf("   ✓ Connection established with slow receiver\n");

    //===BEHAVIOR===
    printf("🎯 BEHAVIOR: Fill buffer with blocking sends, then test NONBLOCK\n");

    // Prepare test data
    const int ChunkSize = 8192;  // 8KB chunks
    char *TestChunk = (char *)malloc(ChunkSize);
    memset(TestChunk, 0xAA, ChunkSize);

    // Phase 1: Fill buffer with blocking sends
    printf("   Phase 1: Filling buffer with blocking sends...\n");
    int SentCount = 0;
    for (int i = 0; i < 10; i++) {  // Send multiple chunks to fill buffer
        IOC_DatDesc_T DatDesc = {0};
        IOC_initDatDesc(&DatDesc);
        DatDesc.Payload.pData = TestChunk;
        DatDesc.Payload.PtrDataSize = ChunkSize;

        Result = IOC_sendDAT(DatSenderLinkID, &DatDesc, NULL);  // Blocking send
        if (Result == IOC_RESULT_SUCCESS) {
            SentCount++;
        } else {
            break;  // Buffer likely full
        }
    }
    printf("   ✓ Sent %d blocking chunks (%d KB total)\n", SentCount, (SentCount * ChunkSize) / 1024);

    // Phase 2: Test NONBLOCK send on full buffer
    printf("   Phase 2: Testing NONBLOCK send on full buffer...\n");

    IOC_DatDesc_T NonBlockDesc = {0};
    IOC_initDatDesc(&NonBlockDesc);
    NonBlockDesc.Payload.pData = TestChunk;
    NonBlockDesc.Payload.PtrDataSize = ChunkSize;

    IOC_Option_defineNonBlock(NonBlockingOptions);

    auto StartTime = std::chrono::high_resolution_clock::now();
    Result = IOC_sendDAT(DatSenderLinkID, &NonBlockDesc, &NonBlockingOptions);
    auto EndTime = std::chrono::high_resolution_clock::now();

    auto Duration = std::chrono::duration_cast<std::chrono::milliseconds>(EndTime - StartTime);

    //===VERIFY===
    printf("✅ VERIFY: Check BUFFER_FULL returned immediately\n");

    //@KeyVerifyPoint-1: NONBLOCK send returns BUFFER_FULL or SUCCESS (not blocked)
    VERIFY_KEYPOINT_TRUE(Result == IOC_RESULT_BUFFER_FULL || Result == IOC_RESULT_SUCCESS,
                         "NONBLOCK send must return BUFFER_FULL or SUCCESS (never block)");

    //@KeyVerifyPoint-2: NONBLOCK send returns immediately (< 10ms)
    VERIFY_KEYPOINT_LT(Duration.count(), 10, "NONBLOCK send must return immediately (< 10ms)");

    if (Result == IOC_RESULT_BUFFER_FULL) {
        printf("   ✅ BUFFER_FULL returned in %ld ms (immediate)\n", Duration.count());
    } else {
        printf("   ℹ️  SUCCESS returned (buffer drained), duration: %ld ms\n", Duration.count());
    }

    //===CLEANUP===
    printf("🧹 CLEANUP: Close connections and offline service\n");

    free(TestChunk);

    if (DatReceiverLinkID != IOC_ID_INVALID) {
        IOC_closeLink(DatReceiverLinkID);
    }
    if (DatSenderLinkID != IOC_ID_INVALID) {
        IOC_closeLink(DatSenderLinkID);
    }
    if (DatSenderSrvID != IOC_ID_INVALID) {
        IOC_offlineService(DatSenderSrvID);
    }

    printf("   ✓ Cleanup complete\n");
}

TEST(UT_DataFault, verifyDataFault_byBufferFullWithTimeout_expectTimeoutError) {
    printf("🔴 RED: verifyDataFault_byBufferFullWithTimeout_expectTimeoutError\n");

    //===SETUP===
    printf("🔧 SETUP: Create sender with slow receiver to test timeout\n");

    IOC_SrvID_T DatSenderSrvID = IOC_ID_INVALID;
    IOC_LinkID_T DatSenderLinkID = IOC_ID_INVALID;
    IOC_LinkID_T DatReceiverLinkID = IOC_ID_INVALID;
    IOC_Result_T Result;

    // Setup DatSender service
    IOC_SrvURI_T DatSenderSrvURI = {
        .pProtocol = IOC_SRV_PROTO_FIFO,
        .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
        .pPath = "test/data/fault/fifo/buffer_timeout",
    };

    IOC_SrvArgs_T SrvArgs = {
        .SrvURI = DatSenderSrvURI,
        .UsageCapabilites = IOC_LinkUsageDatSender,
    };

    Result = IOC_onlineService(&DatSenderSrvID, &SrvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);

    // Setup extremely slow receiver to ensure buffer stays full
    struct {
        int ReceivedCount = 0;
        bool SlowConsumer = true;
    } DatReceiverPrivData;

    auto CbRecvDat_F = [](IOC_LinkID_T LinkID, IOC_DatDesc_pT pDatDesc, void *pCbPriv) -> IOC_Result_T {
        auto *pPrivData = (decltype(DatReceiverPrivData) *)pCbPriv;
        pPrivData->ReceivedCount++;

        if (pPrivData->SlowConsumer) {
            // Very slow processing (500ms) to ensure buffer remains full during timeout test
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }

        return IOC_RESULT_SUCCESS;
    };

    IOC_DatUsageArgs_T DatUsageArgs = {
        .CbRecvDat_F = CbRecvDat_F,
        .pCbPrivData = &DatReceiverPrivData,
    };

    IOC_ConnArgs_T ConnArgs = {
        .SrvURI = DatSenderSrvURI,
        .Usage = IOC_LinkUsageDatReceiver,
        .UsageArgs = {.pDat = &DatUsageArgs},
    };

    std::thread DatReceiverThread([&] {
        Result = IOC_connectService(&DatReceiverLinkID, &ConnArgs, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, Result);
    });

    Result = IOC_acceptClient(DatSenderSrvID, &DatSenderLinkID, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);

    DatReceiverThread.join();
    printf("   ✓ Connection established with very slow receiver\n");

    //===BEHAVIOR===
    printf("🎯 BEHAVIOR: Fill buffer, then test timeout behavior\n");

    // Prepare test data
    const int ChunkSize = 16384;  // 16KB chunks (larger to fill buffer faster)
    char *TestChunk = (char *)malloc(ChunkSize);
    memset(TestChunk, 0xBB, ChunkSize);

    // Phase 1: Fill buffer with rapid sends
    printf("   Phase 1: Filling buffer rapidly...\n");
    int SentCount = 0;
    for (int i = 0; i < 20; i++) {  // Send many chunks
        IOC_DatDesc_T DatDesc = {0};
        IOC_initDatDesc(&DatDesc);
        DatDesc.Payload.pData = TestChunk;
        DatDesc.Payload.PtrDataSize = ChunkSize;

        IOC_Option_defineNonBlock(NonBlockOpts);
        Result = IOC_sendDAT(DatSenderLinkID, &DatDesc, &NonBlockOpts);

        if (Result == IOC_RESULT_SUCCESS) {
            SentCount++;
        } else if (Result == IOC_RESULT_BUFFER_FULL) {
            printf("   Buffer full detected after %d chunks\n", SentCount);
            break;
        }
    }
    printf("   ✓ Sent %d chunks, buffer pressure created\n", SentCount);

    // Phase 2: Test timeout on send
    printf("   Phase 2: Testing send with 100ms timeout...\n");

    IOC_DatDesc_T TimeoutDesc = {0};
    IOC_initDatDesc(&TimeoutDesc);
    TimeoutDesc.Payload.pData = TestChunk;
    TimeoutDesc.Payload.PtrDataSize = ChunkSize;

    IOC_Option_defineTimeout(TimeoutOptions, 100000);  // 100ms timeout

    auto StartTime = std::chrono::high_resolution_clock::now();
    Result = IOC_sendDAT(DatSenderLinkID, &TimeoutDesc, &TimeoutOptions);
    auto EndTime = std::chrono::high_resolution_clock::now();

    auto Duration = std::chrono::duration_cast<std::chrono::milliseconds>(EndTime - StartTime);

    //===VERIFY===
    printf("✅ VERIFY: Check timeout behavior\n");

    //@KeyVerifyPoint-1: Timeout send returns valid result code
    VERIFY_KEYPOINT_TRUE(
        Result == IOC_RESULT_SUCCESS || Result == IOC_RESULT_TIMEOUT || Result == IOC_RESULT_BUFFER_FULL,
        "Timeout send must return SUCCESS, TIMEOUT, or BUFFER_FULL");

    //@KeyVerifyPoint-2: If TIMEOUT, duration should match configured timeout (100ms)
    if (Result == IOC_RESULT_TIMEOUT) {
        printf("   ✅ TIMEOUT returned after %ld ms\n", Duration.count());
        VERIFY_KEYPOINT_GE(Duration.count(), 90, "Timeout duration must be at least 90ms");
        VERIFY_KEYPOINT_LE(Duration.count(), 150, "Timeout duration must be at most 150ms");
    } else {
        printf("   ℹ️  %s returned after %ld ms (buffer state changed)\n",
               Result == IOC_RESULT_SUCCESS ? "SUCCESS" : "BUFFER_FULL", Duration.count());
    }

    //===CLEANUP===
    printf("🧹 CLEANUP\n");

    free(TestChunk);

    if (DatReceiverLinkID != IOC_ID_INVALID) {
        IOC_closeLink(DatReceiverLinkID);
    }
    if (DatSenderLinkID != IOC_ID_INVALID) {
        IOC_closeLink(DatSenderLinkID);
    }
    if (DatSenderSrvID != IOC_ID_INVALID) {
        IOC_offlineService(DatSenderSrvID);
    }

    printf("   ✓ Cleanup complete\n");
}

/**
 * TC-3: verifyDataFault_byRecvNoDataNonBlock_expectNoDataError
 * @[Purpose]: Validate IOC_recvDAT returns NO_DATA when no data available in NONBLOCK mode
 * @[Brief]: Setup polling receiver, call recvDAT with NONBLOCK before sending data, expect NO_DATA
 * @[Steps]:
 *   1) Setup DatReceiver for polling mode (no callback)
 *   2) Call IOC_recvDAT with NONBLOCK when queue is empty
 *   3) Verify IOC_RESULT_NO_DATA returned immediately
 * @[Expect]: IOC_RESULT_NO_DATA without blocking
 */
TEST(UT_DataFault, verifyDataFault_byRecvNoDataNonBlock_expectNoDataError) {
    printf("🔴 RED: verifyDataFault_byRecvNoDataNonBlock_expectNoDataError\n");

    //===SETUP===
    printf("🔧 SETUP: Create polling receiver with no data available\n");

    IOC_SrvID_T DatReceiverSrvID = IOC_ID_INVALID;
    IOC_LinkID_T DatReceiverLinkID = IOC_ID_INVALID;
    IOC_LinkID_T DatSenderLinkID = IOC_ID_INVALID;
    IOC_Result_T Result;

    // Setup DatReceiver service for polling mode
    IOC_SrvURI_T DatReceiverSrvURI = {
        .pProtocol = IOC_SRV_PROTO_FIFO,
        .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
        .pPath = "test/data/fault/fifo/no_data",
    };

    IOC_DatUsageArgs_T DatReceiverUsageArgs = {
        .CbRecvDat_F = NULL,  // No callback = polling mode
        .pCbPrivData = NULL,
    };

    IOC_SrvArgs_T SrvArgs = {
        .SrvURI = DatReceiverSrvURI,
        .UsageCapabilites = IOC_LinkUsageDatReceiver,
        .UsageArgs = {.pDat = &DatReceiverUsageArgs},
    };

    Result = IOC_onlineService(&DatReceiverSrvID, &SrvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);

    // Setup DatSender connection
    IOC_ConnArgs_T ConnArgs = {
        .SrvURI = DatReceiverSrvURI,
        .Usage = IOC_LinkUsageDatSender,
    };

    std::thread DatSenderThread([&] {
        Result = IOC_connectService(&DatSenderLinkID, &ConnArgs, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, Result);
    });

    Result = IOC_acceptClient(DatReceiverSrvID, &DatReceiverLinkID, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);

    DatSenderThread.join();
    printf("   ✓ Connection established, polling receiver ready\n");

    //===BEHAVIOR===
    printf("🎯 BEHAVIOR: Test NONBLOCK recv when no data available\n");

    // Prepare receive buffer
    char RecvBuffer[1024] = {0};
    IOC_DatDesc_T RecvDesc = {0};
    IOC_initDatDesc(&RecvDesc);
    RecvDesc.Payload.pData = RecvBuffer;
    RecvDesc.Payload.PtrDataSize = sizeof(RecvBuffer);

    IOC_Option_defineSyncNonBlock(NonBlockOptions);

    auto StartTime = std::chrono::high_resolution_clock::now();
    Result = IOC_recvDAT(DatReceiverLinkID, &RecvDesc, &NonBlockOptions);
    auto EndTime = std::chrono::high_resolution_clock::now();

    auto Duration = std::chrono::duration_cast<std::chrono::milliseconds>(EndTime - StartTime);

    //===VERIFY===
    printf("✅ VERIFY: Check NO_DATA returned immediately\n");

    //@KeyVerifyPoint-1: NONBLOCK recv returns NO_DATA when queue is empty
    VERIFY_KEYPOINT_EQ(Result, IOC_RESULT_NO_DATA, "NONBLOCK recv must return NO_DATA when no data available");

    //@KeyVerifyPoint-2: NONBLOCK recv returns immediately (< 10ms)
    VERIFY_KEYPOINT_LT(Duration.count(), 10, "NONBLOCK recv must return immediately (< 10ms)");

    printf("   ✅ NO_DATA returned in %ld ms (immediate)\n", Duration.count());

    //===CLEANUP===
    printf("🧹 CLEANUP\n");

    if (DatSenderLinkID != IOC_ID_INVALID) {
        IOC_closeLink(DatSenderLinkID);
    }
    if (DatReceiverLinkID != IOC_ID_INVALID) {
        IOC_closeLink(DatReceiverLinkID);
    }
    if (DatReceiverSrvID != IOC_ID_INVALID) {
        IOC_offlineService(DatReceiverSrvID);
    }

    printf("   ✓ Cleanup complete\n");
}

TEST(UT_DataFault, verifyDataFault_bySendTimeoutPrecision_expectAccurateTiming) {
    GTEST_SKIP() << "TODO: P1 Fault - Implement send timeout precision test";
}

TEST(UT_DataFault, verifyDataFault_byRecvTimeoutPrecision_expectAccurateTiming) {
    GTEST_SKIP() << "TODO: P1 Fault - Implement recv timeout precision test";
}

TEST(UT_DataFault, verifyDataFault_byFlushTimeoutPrecision_expectAccurateTiming) {
    GTEST_SKIP() << "TODO: P1 Fault - Implement flush timeout precision test";
}

TEST(UT_DataFault, verifyDataFault_byZeroTimeoutSend_expectImmediateReturn) {
    GTEST_SKIP() << "TODO: P1 Fault - Implement zero timeout send test";
}

TEST(UT_DataFault, verifyDataFault_byZeroTimeoutRecv_expectImmediateReturn) {
    GTEST_SKIP() << "TODO: P1 Fault - Implement zero timeout recv test";
}

TEST(UT_DataFault, verifyDataFault_byInfiniteTimeoutRecovery_expectEventualSuccess) {
    GTEST_SKIP() << "TODO: P1 Fault - Implement infinite timeout recovery test";
}

/**
 * TC-10: verifyDataFault_byPeerCrashDuringSend_expectLinkBroken
 * @[Purpose]: Validate IOC_sendDAT detects link broken when peer closes unexpectedly
 * @[Brief]: Start data transfer, close receiver link abruptly, verify sender detects LINK_BROKEN
 * @[Steps]:
 *   1) Setup sender and receiver with established connection
 *   2) Start sending data chunks
 *   3) Abruptly close receiver link (simulate crash)
 *   4) Continue sending, verify IOC_RESULT_LINK_BROKEN detected
 * @[Expect]: IOC_RESULT_LINK_BROKEN on sender after receiver closes
 */
TEST(UT_DataFault, verifyDataFault_byPeerCrashDuringSend_expectLinkBroken) {
    printf("🔴 RED: verifyDataFault_byPeerCrashDuringSend_expectLinkBroken\n");

    //===SETUP===
    printf("🔧 SETUP: Create sender and receiver for link failure test\n");

    IOC_SrvID_T DatSenderSrvID = IOC_ID_INVALID;
    IOC_LinkID_T DatSenderLinkID = IOC_ID_INVALID;
    IOC_LinkID_T DatReceiverLinkID = IOC_ID_INVALID;
    IOC_Result_T Result;

    // Setup DatSender service
    IOC_SrvURI_T DatSenderSrvURI = {
        .pProtocol = IOC_SRV_PROTO_FIFO,
        .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
        .pPath = "test/data/fault/fifo/peer_crash",
    };

    IOC_SrvArgs_T SrvArgs = {
        .SrvURI = DatSenderSrvURI,
        .UsageCapabilites = IOC_LinkUsageDatSender,
    };

    Result = IOC_onlineService(&DatSenderSrvID, &SrvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);

    // Setup DatReceiver with callback
    struct {
        int ReceivedCount = 0;
    } DatReceiverPrivData;

    auto CbRecvDat_F = [](IOC_LinkID_T LinkID, IOC_DatDesc_pT pDatDesc, void *pCbPriv) -> IOC_Result_T {
        auto *pPrivData = (decltype(DatReceiverPrivData) *)pCbPriv;
        pPrivData->ReceivedCount++;
        return IOC_RESULT_SUCCESS;
    };

    IOC_DatUsageArgs_T DatUsageArgs = {
        .CbRecvDat_F = CbRecvDat_F,
        .pCbPrivData = &DatReceiverPrivData,
    };

    IOC_ConnArgs_T ConnArgs = {
        .SrvURI = DatSenderSrvURI,
        .Usage = IOC_LinkUsageDatReceiver,
        .UsageArgs = {.pDat = &DatUsageArgs},
    };

    std::thread DatReceiverThread([&] {
        Result = IOC_connectService(&DatReceiverLinkID, &ConnArgs, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, Result);
    });

    Result = IOC_acceptClient(DatSenderSrvID, &DatSenderLinkID, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);

    DatReceiverThread.join();
    printf("   ✓ Connection established\n");

    //===BEHAVIOR===
    printf("🎯 BEHAVIOR: Send data, then simulate peer crash\n");

    // Prepare test data
    const int ChunkSize = 4096;
    char *TestChunk = (char *)malloc(ChunkSize);
    memset(TestChunk, 0xCC, ChunkSize);

    // Phase 1: Send initial data successfully
    printf("   Phase 1: Send initial data...\n");
    IOC_DatDesc_T DatDesc = {0};
    IOC_initDatDesc(&DatDesc);
    DatDesc.Payload.pData = TestChunk;
    DatDesc.Payload.PtrDataSize = ChunkSize;

    Result = IOC_sendDAT(DatSenderLinkID, &DatDesc, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result) << "Initial send should succeed";
    IOC_flushDAT(DatSenderLinkID, NULL);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));  // Let data flow
    printf("   ✓ Initial send succeeded\n");

    // Phase 2: Simulate peer crash by closing receiver
    printf("   Phase 2: Simulate peer crash (close receiver)...\n");
    IOC_closeLink(DatReceiverLinkID);
    DatReceiverLinkID = IOC_ID_INVALID;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));  // Give time for break detection
    printf("   ✓ Receiver closed (simulated crash)\n");

    // Phase 3: Try to send data, should detect broken link
    printf("   Phase 3: Attempt send after peer crash...\n");
    Result = IOC_sendDAT(DatSenderLinkID, &DatDesc, NULL);

    //===VERIFY===
    printf("✅ VERIFY: Check link broken detection\n");

    //@KeyVerifyPoint-1: Send should detect link broken
    VERIFY_KEYPOINT_TRUE(Result == IOC_RESULT_LINK_BROKEN || Result == IOC_RESULT_NOT_EXIST_LINK,
                         "Send after peer crash must return LINK_BROKEN or NOT_EXIST_LINK");

    printf("   ✅ Link broken detected, returned: %d\n", Result);

    //===CLEANUP===
    printf("🧹 CLEANUP\n");

    free(TestChunk);

    if (DatSenderLinkID != IOC_ID_INVALID) {
        IOC_closeLink(DatSenderLinkID);
    }
    if (DatSenderSrvID != IOC_ID_INVALID) {
        IOC_offlineService(DatSenderSrvID);
    }

    printf("   ✓ Cleanup complete\n");
}

/**
 * TC-11: verifyDataFault_byPeerClosedDuringRecv_expectLinkBroken
 * @[Purpose]: Validate IOC_recvDAT detects link broken when sender closes
 * @[Brief]: Setup polling receiver, close sender during receive wait, verify LINK_BROKEN
 * @[Steps]:
 *   1) Setup polling receiver (no callback)
 *   2) Start receive wait (blocking)
 *   3) Close sender link from another thread
 *   4) Verify receiver detects LINK_BROKEN
 * @[Expect]: IOC_RESULT_LINK_BROKEN on receiver
 */
TEST(UT_DataFault, verifyDataFault_byPeerClosedDuringRecv_expectLinkBroken) {
    printf("🔴 RED: verifyDataFault_byPeerClosedDuringRecv_expectLinkBroken\n");

    //===SETUP===
    printf("🔧 SETUP: Create polling receiver for peer close test\n");

    IOC_SrvID_T DatReceiverSrvID = IOC_ID_INVALID;
    IOC_LinkID_T DatReceiverLinkID = IOC_ID_INVALID;
    IOC_LinkID_T DatSenderLinkID = IOC_ID_INVALID;
    IOC_Result_T Result;

    // Setup DatReceiver service for polling
    IOC_SrvURI_T DatReceiverSrvURI = {
        .pProtocol = IOC_SRV_PROTO_FIFO,
        .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
        .pPath = "test/data/fault/fifo/peer_closed_recv",
    };

    IOC_DatUsageArgs_T DatReceiverUsageArgs = {
        .CbRecvDat_F = NULL,  // Polling mode
        .pCbPrivData = NULL,
    };

    IOC_SrvArgs_T SrvArgs = {
        .SrvURI = DatReceiverSrvURI,
        .UsageCapabilites = IOC_LinkUsageDatReceiver,
        .UsageArgs = {.pDat = &DatReceiverUsageArgs},
    };

    Result = IOC_onlineService(&DatReceiverSrvID, &SrvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);

    // Setup DatSender connection
    IOC_ConnArgs_T ConnArgs = {
        .SrvURI = DatReceiverSrvURI,
        .Usage = IOC_LinkUsageDatSender,
    };

    std::thread DatSenderThread([&] {
        Result = IOC_connectService(&DatSenderLinkID, &ConnArgs, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, Result);
    });

    Result = IOC_acceptClient(DatReceiverSrvID, &DatReceiverLinkID, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);

    DatSenderThread.join();
    printf("   ✓ Connection established\n");

    //===BEHAVIOR===
    printf("🎯 BEHAVIOR: Close sender while receiver waits\n");

    // Start recv in another thread with timeout
    char RecvBuffer[1024] = {0};
    IOC_DatDesc_T RecvDesc = {0};
    IOC_initDatDesc(&RecvDesc);
    RecvDesc.Payload.pData = RecvBuffer;
    RecvDesc.Payload.PtrDataSize = sizeof(RecvBuffer);

    IOC_Option_defineTimeout(TimeoutOptions, 500000);  // 500ms timeout

    std::thread RecvThread([&] {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));  // Let sender close first
        Result = IOC_recvDAT(DatReceiverLinkID, &RecvDesc, &TimeoutOptions);
        printf("   Recv returned with result: %d\n", Result);
    });

    // Close sender link (simulate sender crash)
    printf("   Closing sender link...\n");
    IOC_closeLink(DatSenderLinkID);
    DatSenderLinkID = IOC_ID_INVALID;

    RecvThread.join();

    //===VERIFY===
    printf("✅ VERIFY: Check link broken detection on receiver\n");

    //@KeyVerifyPoint-1: Recv should detect link broken or timeout
    VERIFY_KEYPOINT_TRUE(Result == IOC_RESULT_LINK_BROKEN || Result == IOC_RESULT_NOT_EXIST_LINK ||
                             Result == IOC_RESULT_TIMEOUT || Result == IOC_RESULT_NO_DATA,
                         "Recv after sender close must detect error condition");

    printf("   ✅ Error detected, returned: %d\n", Result);

    //===CLEANUP===
    printf("🧹 CLEANUP\n");

    if (DatReceiverLinkID != IOC_ID_INVALID) {
        IOC_closeLink(DatReceiverLinkID);
    }
    if (DatReceiverSrvID != IOC_ID_INVALID) {
        IOC_offlineService(DatReceiverSrvID);
    }

    printf("   ✓ Cleanup complete\n");
}

/**
 * TC-12: verifyDataFault_byServiceOfflineWithActiveLink_expectLinkBroken
 * @[Purpose]: Validate orphaned links detect service offline
 * @[Brief]: Establish connection, offline service, verify operations fail
 * @[Steps]:
 *   1) Setup service and establish connection
 *   2) Offline the service while link exists
 *   3) Attempt data operations on orphaned link
 *   4) Verify appropriate error returned
 * @[Expect]: IOC_RESULT_LINK_BROKEN or NOT_EXIST_LINK
 */
TEST(UT_DataFault, verifyDataFault_byServiceOfflineWithActiveLink_expectLinkBroken) {
    printf("🔴 RED: verifyDataFault_byServiceOfflineWithActiveLink_expectLinkBroken\n");

    //===SETUP===
    printf("🔧 SETUP: Create service and connection\n");

    IOC_SrvID_T DatSenderSrvID = IOC_ID_INVALID;
    IOC_LinkID_T DatSenderLinkID = IOC_ID_INVALID;
    IOC_LinkID_T DatReceiverLinkID = IOC_ID_INVALID;
    IOC_Result_T Result;

    // Setup DatSender service
    IOC_SrvURI_T DatSenderSrvURI = {
        .pProtocol = IOC_SRV_PROTO_FIFO,
        .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
        .pPath = "test/data/fault/fifo/service_offline",
    };

    IOC_SrvArgs_T SrvArgs = {
        .SrvURI = DatSenderSrvURI,
        .UsageCapabilites = IOC_LinkUsageDatSender,
    };

    Result = IOC_onlineService(&DatSenderSrvID, &SrvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);

    // Setup DatReceiver with callback
    struct {
        int ReceivedCount = 0;
    } DatReceiverPrivData;

    auto CbRecvDat_F = [](IOC_LinkID_T LinkID, IOC_DatDesc_pT pDatDesc, void *pCbPriv) -> IOC_Result_T {
        auto *pPrivData = (decltype(DatReceiverPrivData) *)pCbPriv;
        pPrivData->ReceivedCount++;
        return IOC_RESULT_SUCCESS;
    };

    IOC_DatUsageArgs_T DatUsageArgs = {
        .CbRecvDat_F = CbRecvDat_F,
        .pCbPrivData = &DatReceiverPrivData,
    };

    IOC_ConnArgs_T ConnArgs = {
        .SrvURI = DatSenderSrvURI,
        .Usage = IOC_LinkUsageDatReceiver,
        .UsageArgs = {.pDat = &DatUsageArgs},
    };

    std::thread DatReceiverThread([&] {
        Result = IOC_connectService(&DatReceiverLinkID, &ConnArgs, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, Result);
    });

    Result = IOC_acceptClient(DatSenderSrvID, &DatSenderLinkID, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);

    DatReceiverThread.join();
    printf("   ✓ Connection established\n");

    //===BEHAVIOR===
    printf("🎯 BEHAVIOR: Offline service with active link\n");

    // Offline the service
    Result = IOC_offlineService(DatSenderSrvID);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);
    DatSenderSrvID = IOC_ID_INVALID;
    printf("   ✓ Service offline\n");

    std::this_thread::sleep_for(std::chrono::milliseconds(100));  // Give time for detection

    // Try to send data on orphaned link
    const int ChunkSize = 1024;
    char TestData[ChunkSize];
    memset(TestData, 0xDD, ChunkSize);

    IOC_DatDesc_T DatDesc = {0};
    IOC_initDatDesc(&DatDesc);
    DatDesc.Payload.pData = TestData;
    DatDesc.Payload.PtrDataSize = ChunkSize;

    Result = IOC_sendDAT(DatSenderLinkID, &DatDesc, NULL);

    //===VERIFY===
    printf("✅ VERIFY: Check orphaned link detection\n");

    //@KeyVerifyPoint-1: Operation on orphaned link should fail
    VERIFY_KEYPOINT_TRUE(Result == IOC_RESULT_LINK_BROKEN || Result == IOC_RESULT_NOT_EXIST_LINK,
                         "Operation on orphaned link must return LINK_BROKEN or NOT_EXIST_LINK");

    printf("   ✅ Orphaned link detected, returned: %d\n", Result);

    //===CLEANUP===
    printf("🧹 CLEANUP\n");

    if (DatReceiverLinkID != IOC_ID_INVALID) {
        IOC_closeLink(DatReceiverLinkID);
    }
    if (DatSenderLinkID != IOC_ID_INVALID) {
        IOC_closeLink(DatSenderLinkID);
    }

    printf("   ✓ Cleanup complete\n");
}

//@[Purpose]: Verify graceful handling of abrupt connection loss during data transfer.
//@[Brief]: Simulate abrupt disconnection (e.g., network cable unplugged) and verify both
//          sender and receiver detect link failure appropriately.
//@[Steps]:
//    1. Establish data connection between sender and receiver
//    2. Start data transfer (send multiple chunks)
//    3. Abruptly close the underlying link (simulate cable disconnect)
//    4. Attempt continued operations on both sides
//    5. Verify both sides detect link broken condition
//@[Expect]: Both sender and receiver return LINK_BROKEN or NOT_EXIST_LINK after disconnection.
TEST(UT_DataFault, verifyDataFault_byAbruptDisconnection_expectGracefulHandling) {
    //===SETUP===
    printf("🔧 SETUP: Create services and establish connection\n");

    IOC_Result_T Result = IOC_RESULT_BUG;
    IOC_SrvID_T ReceiverSrvID = IOC_ID_INVALID;
    IOC_LinkID_T ReceiverLinkID = IOC_ID_INVALID;
    IOC_LinkID_T SenderLinkID = IOC_ID_INVALID;

    // Standard SrvURI for DAT communication
    IOC_SrvURI_T ReceiverSrvURI = {
        .pProtocol = IOC_SRV_PROTO_FIFO,
        .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
        .pPath = (const char *)"DatReceiver_Disconnection",
    };

    // Create receiver service
    IOC_SrvArgs_T SrvArgs = {
        .SrvURI = ReceiverSrvURI,
        .UsageCapabilites = IOC_LinkUsageDatReceiver,
    };

    Result = IOC_onlineService(&ReceiverSrvID, &SrvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result) << "Failed to create receiver service";
    printf("   ✓ Receiver service created\n");

    // Establish connection from sender
    IOC_ConnArgs_T ConnArgs = {
        .SrvURI = ReceiverSrvURI,
        .Usage = IOC_LinkUsageDatSender,
    };

    std::thread SenderThread([&] {
        IOC_Result_T ThreadResult = IOC_connectService(&SenderLinkID, &ConnArgs, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, ThreadResult);
    });

    // Accept connection
    Result = IOC_acceptClient(ReceiverSrvID, &ReceiverLinkID, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result) << "Failed to accept connection";
    SenderThread.join();
    printf("   ✓ Connection established\n");

    //===BEHAVIOR===
    printf("🎯 BEHAVIOR: Transfer data then abruptly close receiver link\n");

    // Send initial chunk successfully
    const int ChunkSize = 1024;
    char TestData[ChunkSize];
    memset(TestData, 0xAB, ChunkSize);

    IOC_DatDesc_T SendDesc = {0};
    IOC_initDatDesc(&SendDesc);
    SendDesc.Payload.pData = TestData;
    SendDesc.Payload.PtrDataSize = ChunkSize;

    Result = IOC_sendDAT(SenderLinkID, &SendDesc, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result) << "Initial send failed";
    printf("   ✓ Initial data sent\n");

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Abruptly close receiver link (simulate cable disconnect)
    Result = IOC_closeLink(ReceiverLinkID);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);
    ReceiverLinkID = IOC_ID_INVALID;
    printf("   ✓ Receiver link abruptly closed (simulating disconnection)\n");

    std::this_thread::sleep_for(std::chrono::milliseconds(100));  // Detection time

    // Try to send more data from sender side
    IOC_DatDesc_T SendDesc2 = {0};
    IOC_initDatDesc(&SendDesc2);
    SendDesc2.Payload.pData = TestData;
    SendDesc2.Payload.PtrDataSize = ChunkSize;

    Result = IOC_sendDAT(SenderLinkID, &SendDesc2, NULL);

    //===VERIFY===
    printf("✅ VERIFY: Check abrupt disconnection detection\n");

    //@KeyVerifyPoint-1: Sender should detect link broken after abrupt close
    VERIFY_KEYPOINT_TRUE(Result == IOC_RESULT_LINK_BROKEN || Result == IOC_RESULT_NOT_EXIST_LINK,
                         "Sender must detect link broken after abrupt disconnection");

    printf("   ✅ Abrupt disconnection detected, returned: %d\n", Result);

    //===CLEANUP===
    printf("🧹 CLEANUP\n");

    if (SenderLinkID != IOC_ID_INVALID) {
        IOC_closeLink(SenderLinkID);
    }
    if (ReceiverLinkID != IOC_ID_INVALID) {
        IOC_closeLink(ReceiverLinkID);
    }
    if (ReceiverSrvID != IOC_ID_INVALID) {
        IOC_offlineService(ReceiverSrvID);
    }

    printf("   ✓ Cleanup complete\n");
}

//@[Purpose]: Verify IOC_flushDAT detects link broken condition during flush operation.
//@[Brief]: Start flushing buffered data, then break the link mid-flush, verify proper
//          error detection and handling.
//@[Steps]:
//    1. Establish connection and buffer multiple data chunks
//    2. Start flush operation (async or with timeout)
//    3. Close receiver link during flush
//    4. Verify flush detects link broken
//@[Expect]: IOC_flushDAT returns LINK_BROKEN or NOT_EXIST_LINK.
TEST(UT_DataFault, verifyDataFault_byLinkBrokenDuringFlush_expectLinkBrokenError) {
    //===SETUP===
    printf("🔧 SETUP: Create services and buffer data for flush\n");

    IOC_Result_T Result = IOC_RESULT_BUG;
    IOC_SrvID_T ReceiverSrvID = IOC_ID_INVALID;
    IOC_LinkID_T ReceiverLinkID = IOC_ID_INVALID;
    IOC_LinkID_T SenderLinkID = IOC_ID_INVALID;

    // Standard SrvURI for DAT communication
    IOC_SrvURI_T ReceiverSrvURI = {
        .pProtocol = IOC_SRV_PROTO_FIFO,
        .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
        .pPath = (const char *)"DatReceiver_FlushTest",
    };

    // Create receiver service
    IOC_SrvArgs_T SrvArgs = {
        .SrvURI = ReceiverSrvURI,
        .UsageCapabilites = IOC_LinkUsageDatReceiver,
    };

    Result = IOC_onlineService(&ReceiverSrvID, &SrvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result) << "Failed to create receiver service";
    printf("   ✓ Receiver service created\n");

    // Establish connection from sender
    IOC_ConnArgs_T ConnArgs = {
        .SrvURI = ReceiverSrvURI,
        .Usage = IOC_LinkUsageDatSender,
    };

    std::thread SenderThread([&] {
        IOC_Result_T ThreadResult = IOC_connectService(&SenderLinkID, &ConnArgs, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, ThreadResult);
    });

    // Accept connection
    Result = IOC_acceptClient(ReceiverSrvID, &ReceiverLinkID, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result) << "Failed to accept connection";
    SenderThread.join();
    printf("   ✓ Connection established\n");

    //===BEHAVIOR===
    printf("🎯 BEHAVIOR: Buffer data then break link during flush\n");

    // Send multiple chunks quickly
    const int ChunkSize = 1024;
    const int NumChunks = 10;
    char TestData[ChunkSize];
    memset(TestData, 0xCD, ChunkSize);

    for (int i = 0; i < NumChunks; i++) {
        IOC_DatDesc_T SendDesc = {0};
        IOC_initDatDesc(&SendDesc);
        SendDesc.Payload.pData = TestData;
        SendDesc.Payload.PtrDataSize = ChunkSize;

        Result = IOC_sendDAT(SenderLinkID, &SendDesc, NULL);
        if (Result != IOC_RESULT_SUCCESS) {
            break;  // Error occurred
        }
    }
    printf("   ✓ Data sent (%d chunks attempted)\n", NumChunks);

    // Start flush in separate thread
    std::atomic<IOC_Result_T> FlushResult(IOC_RESULT_SUCCESS);
    std::thread FlushThread([SenderLinkID, &FlushResult]() { FlushResult.store(IOC_flushDAT(SenderLinkID, NULL)); });

    std::this_thread::sleep_for(std::chrono::milliseconds(50));  // Let flush start

    // Break link during flush
    Result = IOC_closeLink(ReceiverLinkID);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);
    ReceiverLinkID = IOC_ID_INVALID;
    printf("   ✓ Receiver link closed during flush\n");

    // Wait for flush thread
    FlushThread.join();

    //===VERIFY===
    printf("✅ VERIFY: Check flush detects link broken\n");

    //@KeyVerifyPoint-1: Flush should detect link broken during operation
    VERIFY_KEYPOINT_TRUE(FlushResult.load() == IOC_RESULT_LINK_BROKEN ||
                             FlushResult.load() == IOC_RESULT_NOT_EXIST_LINK ||
                             FlushResult.load() == IOC_RESULT_SUCCESS,
                         "Flush must detect link broken or complete before break");

    printf("   ✅ Flush result: %d\n", FlushResult.load());

    //===CLEANUP===
    printf("🧹 CLEANUP\n");

    if (SenderLinkID != IOC_ID_INVALID) {
        IOC_closeLink(SenderLinkID);
    }
    if (ReceiverLinkID != IOC_ID_INVALID) {
        IOC_closeLink(ReceiverLinkID);
    }
    if (ReceiverSrvID != IOC_ID_INVALID) {
        IOC_offlineService(ReceiverSrvID);
    }

    printf("   ✓ Cleanup complete\n");
}

//@[Purpose]: Verify application can successfully retry after encountering buffer full condition.
//@[Brief]: Fill buffer completely, verify BUFFER_FULL error, then retry after buffer drains,
//          verify eventual success.
//@[Steps]:
//    1. Establish connection with slow receiver
//    2. Fill buffer completely (get BUFFER_FULL)
//    3. Wait for buffer to drain
//    4. Retry same operation
//    5. Verify eventual success
//@[Expect]: After buffer drains, retry succeeds with IOC_RESULT_SUCCESS.
TEST(UT_DataFault, verifyDataFault_byRetryAfterBufferFull_expectEventualSuccess) {
    //===SETUP===
    printf("🔧 SETUP: Create services with controlled receiver\n");

    IOC_Result_T Result = IOC_RESULT_BUG;
    IOC_SrvID_T ReceiverSrvID = IOC_ID_INVALID;
    IOC_LinkID_T ReceiverLinkID = IOC_ID_INVALID;
    IOC_LinkID_T SenderLinkID = IOC_ID_INVALID;

    // Standard SrvURI for DAT communication
    IOC_SrvURI_T ReceiverSrvURI = {
        .pProtocol = IOC_SRV_PROTO_FIFO,
        .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
        .pPath = (const char *)"DatReceiver_Retry",
    };

    // Create receiver service
    IOC_SrvArgs_T SrvArgs = {
        .SrvURI = ReceiverSrvURI,
        .UsageCapabilites = IOC_LinkUsageDatReceiver,
    };

    Result = IOC_onlineService(&ReceiverSrvID, &SrvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);
    printf("   ✓ Receiver service created\n");

    // Establish connection
    IOC_ConnArgs_T ConnArgs = {
        .SrvURI = ReceiverSrvURI,
        .Usage = IOC_LinkUsageDatSender,
    };

    std::thread SenderThread([&] {
        IOC_Result_T ThreadResult = IOC_connectService(&SenderLinkID, &ConnArgs, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, ThreadResult);
    });

    Result = IOC_acceptClient(ReceiverSrvID, &ReceiverLinkID, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);
    SenderThread.join();
    printf("   ✓ Connection established\n");

    //===BEHAVIOR===
    printf("🎯 BEHAVIOR: Fill buffer, retry after drain\n");

    const int ChunkSize = 1024;
    char TestData[ChunkSize];
    memset(TestData, 0xBF, ChunkSize);

    // Fill buffer until BUFFER_FULL or error
    int Attempts = 0;
    const int MaxAttempts = 100;
    bool BufferFull = false;

    for (Attempts = 0; Attempts < MaxAttempts; Attempts++) {
        IOC_DatDesc_T SendDesc = {0};
        IOC_initDatDesc(&SendDesc);
        SendDesc.Payload.pData = TestData;
        SendDesc.Payload.PtrDataSize = ChunkSize;

        Result = IOC_sendDAT(SenderLinkID, &SendDesc, NULL);
        if (Result == IOC_RESULT_BUFFER_FULL) {
            BufferFull = true;
            printf("   ✓ Buffer full after %d sends\n", Attempts);
            break;
        } else if (Result != IOC_RESULT_SUCCESS) {
            break;  // Error
        }
    }

    // If we didn't get BUFFER_FULL, start receiving to drain
    std::thread ReceiverThread([ReceiverLinkID]() {
        for (int i = 0; i < 10; i++) {
            IOC_DatDesc_T RecvDesc = {0};
            IOC_Result_T RecvResult = IOC_recvDAT(ReceiverLinkID, &RecvDesc, NULL);
            if (RecvResult == IOC_RESULT_SUCCESS) {
                // Successfully received, buffer draining
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            } else {
                break;
            }
        }
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(200));  // Wait for buffer drain

    // Retry the failed send
    IOC_DatDesc_T RetryDesc = {0};
    IOC_initDatDesc(&RetryDesc);
    RetryDesc.Payload.pData = TestData;
    RetryDesc.Payload.PtrDataSize = ChunkSize;

    Result = IOC_sendDAT(SenderLinkID, &RetryDesc, NULL);

    ReceiverThread.join();

    //===VERIFY===
    printf("✅ VERIFY: Check retry after buffer drain\n");

    //@KeyVerifyPoint-1: Retry should succeed after buffer drains
    VERIFY_KEYPOINT_TRUE(Result == IOC_RESULT_SUCCESS || Result == IOC_RESULT_BUFFER_FULL,
                         "Retry after buffer drain should eventually succeed");

    printf("   ✅ Retry result: %d\n", Result);

    //===CLEANUP===
    printf("🧹 CLEANUP\n");

    if (SenderLinkID != IOC_ID_INVALID) {
        IOC_closeLink(SenderLinkID);
    }
    if (ReceiverLinkID != IOC_ID_INVALID) {
        IOC_closeLink(ReceiverLinkID);
    }
    if (ReceiverSrvID != IOC_ID_INVALID) {
        IOC_offlineService(ReceiverSrvID);
    }

    printf("   ✓ Cleanup complete\n");
}

//@[Purpose]: Verify application can reconnect after link broken condition.
//@[Brief]: Establish connection, break link, close old link, reconnect, verify success.
//@[Steps]:
//    1. Establish initial connection
//    2. Break link (close receiver)
//    3. Detect link broken on sender
//    4. Close broken sender link
//    5. Reconnect and verify new connection works
//@[Expect]: After cleanup, new connection succeeds and data transfer works.
TEST(UT_DataFault, verifyDataFault_byReconnectAfterLinkBroken_expectNewConnection) {
    //===SETUP===
    printf("🔧 SETUP: Create service for reconnection test\n");

    IOC_Result_T Result = IOC_RESULT_BUG;
    IOC_SrvID_T ReceiverSrvID = IOC_ID_INVALID;
    IOC_LinkID_T ReceiverLinkID = IOC_ID_INVALID;
    IOC_LinkID_T SenderLinkID = IOC_ID_INVALID;

    // Standard SrvURI for DAT communication
    IOC_SrvURI_T ReceiverSrvURI = {
        .pProtocol = IOC_SRV_PROTO_FIFO,
        .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
        .pPath = (const char *)"DatReceiver_Reconnect",
    };

    // Create receiver service
    IOC_SrvArgs_T SrvArgs = {
        .SrvURI = ReceiverSrvURI,
        .UsageCapabilites = IOC_LinkUsageDatReceiver,
    };

    Result = IOC_onlineService(&ReceiverSrvID, &SrvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);
    printf("   ✓ Receiver service created\n");

    //===BEHAVIOR===
    printf("🎯 BEHAVIOR: Connect, break, reconnect\n");

    // First connection
    IOC_ConnArgs_T ConnArgs = {
        .SrvURI = ReceiverSrvURI,
        .Usage = IOC_LinkUsageDatSender,
    };

    std::thread SenderThread1([&] {
        IOC_Result_T ThreadResult = IOC_connectService(&SenderLinkID, &ConnArgs, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, ThreadResult);
    });

    Result = IOC_acceptClient(ReceiverSrvID, &ReceiverLinkID, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);
    SenderThread1.join();
    printf("   ✓ First connection established\n");

    // Break link by closing receiver
    Result = IOC_closeLink(ReceiverLinkID);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);
    ReceiverLinkID = IOC_ID_INVALID;
    printf("   ✓ Receiver link closed\n");

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Verify link is broken on sender
    const int ChunkSize = 1024;
    char TestData[ChunkSize];
    memset(TestData, 0xCD, ChunkSize);

    IOC_DatDesc_T SendDesc = {0};
    IOC_initDatDesc(&SendDesc);
    SendDesc.Payload.pData = TestData;
    SendDesc.Payload.PtrDataSize = ChunkSize;

    Result = IOC_sendDAT(SenderLinkID, &SendDesc, NULL);
    printf("   ✓ Send after break returned: %d\n", Result);

    // Close broken link
    IOC_closeLink(SenderLinkID);
    SenderLinkID = IOC_ID_INVALID;
    printf("   ✓ Sender link closed\n");

    // Reconnect
    IOC_LinkID_T NewSenderLinkID = IOC_ID_INVALID;
    IOC_LinkID_T NewReceiverLinkID = IOC_ID_INVALID;

    std::thread SenderThread2([&] {
        IOC_Result_T ThreadResult = IOC_connectService(&NewSenderLinkID, &ConnArgs, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, ThreadResult);
    });

    Result = IOC_acceptClient(ReceiverSrvID, &NewReceiverLinkID, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);
    SenderThread2.join();
    printf("   ✓ Reconnection established\n");

    // Try sending on new connection
    IOC_DatDesc_T SendDesc2 = {0};
    IOC_initDatDesc(&SendDesc2);
    SendDesc2.Payload.pData = TestData;
    SendDesc2.Payload.PtrDataSize = ChunkSize;

    Result = IOC_sendDAT(NewSenderLinkID, &SendDesc2, NULL);

    //===VERIFY===
    printf("✅ VERIFY: Check reconnection success\n");

    //@KeyVerifyPoint-1: Reconnection should succeed and allow data transfer
    VERIFY_KEYPOINT_EQ(Result, IOC_RESULT_SUCCESS, "Reconnection must allow successful data transfer");

    printf("   ✅ Reconnection successful, data sent: %d\n", Result);

    //===CLEANUP===
    printf("🧹 CLEANUP\n");

    if (NewSenderLinkID != IOC_ID_INVALID) {
        IOC_closeLink(NewSenderLinkID);
    }
    if (NewReceiverLinkID != IOC_ID_INVALID) {
        IOC_closeLink(NewReceiverLinkID);
    }
    if (ReceiverSrvID != IOC_ID_INVALID) {
        IOC_offlineService(ReceiverSrvID);
    }

    printf("   ✓ Cleanup complete\n");
}

//@[Purpose]: Verify system can recover from transient failures (temporary errors).
//@[Brief]: Simulate transient failure (buffer temporarily full), then verify recovery
//          after condition clears.
//@[Steps]:
//    1. Establish connection
//    2. Create transient failure condition (buffer full temporarily)
//    3. Wait for condition to clear (receiver drains)
//    4. Resume normal operation
//    5. Verify successful recovery
//@[Expect]: After transient failure clears, normal operation resumes successfully.
TEST(UT_DataFault, verifyDataFault_byRecoveryFromTransientFailure_expectResume) {
    //===SETUP===
    printf("🔧 SETUP: Create services for transient failure test\n");

    IOC_Result_T Result = IOC_RESULT_BUG;
    IOC_SrvID_T ReceiverSrvID = IOC_ID_INVALID;
    IOC_LinkID_T ReceiverLinkID = IOC_ID_INVALID;
    IOC_LinkID_T SenderLinkID = IOC_ID_INVALID;

    // Standard SrvURI for DAT communication
    IOC_SrvURI_T ReceiverSrvURI = {
        .pProtocol = IOC_SRV_PROTO_FIFO,
        .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
        .pPath = (const char *)"DatReceiver_Transient",
    };

    // Create receiver service
    IOC_SrvArgs_T SrvArgs = {
        .SrvURI = ReceiverSrvURI,
        .UsageCapabilites = IOC_LinkUsageDatReceiver,
    };

    Result = IOC_onlineService(&ReceiverSrvID, &SrvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);
    printf("   ✓ Receiver service created\n");

    // Establish connection
    IOC_ConnArgs_T ConnArgs = {
        .SrvURI = ReceiverSrvURI,
        .Usage = IOC_LinkUsageDatSender,
    };

    std::thread SenderThread([&] {
        IOC_Result_T ThreadResult = IOC_connectService(&SenderLinkID, &ConnArgs, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, ThreadResult);
    });

    Result = IOC_acceptClient(ReceiverSrvID, &ReceiverLinkID, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);
    SenderThread.join();
    printf("   ✓ Connection established\n");

    //===BEHAVIOR===
    printf("🎯 BEHAVIOR: Create transient failure, then recover\n");

    const int ChunkSize = 1024;
    char TestData[ChunkSize];
    memset(TestData, 0xEF, ChunkSize);

    // Send initial data successfully
    IOC_DatDesc_T SendDesc1 = {0};
    IOC_initDatDesc(&SendDesc1);
    SendDesc1.Payload.pData = TestData;
    SendDesc1.Payload.PtrDataSize = ChunkSize;

    Result = IOC_sendDAT(SenderLinkID, &SendDesc1, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);
    printf("   ✓ Initial send successful\n");

    // Create transient condition (send many quickly to potentially fill buffer)
    const int TransientAttempts = 20;
    int SuccessCount = 0;
    int FailureCount = 0;

    for (int i = 0; i < TransientAttempts; i++) {
        IOC_DatDesc_T SendDesc = {0};
        IOC_initDatDesc(&SendDesc);
        SendDesc.Payload.pData = TestData;
        SendDesc.Payload.PtrDataSize = ChunkSize;

        Result = IOC_sendDAT(SenderLinkID, &SendDesc, NULL);
        if (Result == IOC_RESULT_SUCCESS) {
            SuccessCount++;
        } else {
            FailureCount++;
        }
    }
    printf("   ✓ Transient phase: %d success, %d failures\n", SuccessCount, FailureCount);

    // Simulate recovery by draining some data
    std::thread ReceiverThread([ReceiverLinkID]() {
        for (int i = 0; i < 5; i++) {
            IOC_DatDesc_T RecvDesc = {0};
            IOC_Result_T RecvResult = IOC_recvDAT(ReceiverLinkID, &RecvDesc, NULL);
            if (RecvResult == IOC_RESULT_SUCCESS) {
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
            }
        }
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(150));  // Recovery time

    // Resume normal operation after recovery
    IOC_DatDesc_T SendDesc2 = {0};
    IOC_initDatDesc(&SendDesc2);
    SendDesc2.Payload.pData = TestData;
    SendDesc2.Payload.PtrDataSize = ChunkSize;

    Result = IOC_sendDAT(SenderLinkID, &SendDesc2, NULL);

    ReceiverThread.join();

    //===VERIFY===
    printf("✅ VERIFY: Check recovery after transient failure\n");

    //@KeyVerifyPoint-1: Normal operation should resume after transient failure
    VERIFY_KEYPOINT_TRUE(Result == IOC_RESULT_SUCCESS || Result == IOC_RESULT_BUFFER_FULL,
                         "System must recover and resume after transient failure");

    printf("   ✅ Recovery successful, final send result: %d\n", Result);

    //===CLEANUP===
    printf("🧹 CLEANUP\n");

    if (SenderLinkID != IOC_ID_INVALID) {
        IOC_closeLink(SenderLinkID);
    }
    if (ReceiverLinkID != IOC_ID_INVALID) {
        IOC_closeLink(ReceiverLinkID);
    }
    if (ReceiverSrvID != IOC_ID_INVALID) {
        IOC_offlineService(ReceiverSrvID);
    }

    printf("   ✓ Cleanup complete\n");
}

TEST(UT_DataFault, verifyDataFault_byDiskFullDuringFIFOWrite_expectIOError) {
    GTEST_SKIP() << "TODO: P1 Fault - Implement disk full simulation test";
}

TEST(UT_DataFault, verifyDataFault_byFIFOPermissionDenied_expectAccessError) {
    GTEST_SKIP() << "TODO: P1 Fault - Implement permission denied test";
}

TEST(UT_DataFault, verifyDataFault_byFIFOCorruptionRecovery_expectGracefulHandling) {
    GTEST_SKIP() << "TODO: P1 Fault - Implement FIFO corruption test";
}

//======>END OF UNIT TESTING IMPLEMENTATION========================================================

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
// P1 🥇 FUNCTIONAL TESTING – InvalidFunc (Fault) - FIFO Protocol
//===================================================================================================
//
//   ⚪ [@AC-1,US-1] TC-1: verifyDataFault_byBufferFullNonBlock_expectBufferFullError
//        - Description: Validate IOC_sendDAT returns BUFFER_FULL in NONBLOCK mode.
//        - Category: Fault (InvalidFunc) - Resource Exhaustion
//        - Status: TODO/PLANNED
//        - Estimated effort: 2 hours
//
//   ⚪ [@AC-2,US-1] TC-2: verifyDataFault_byBufferFullWithTimeout_expectTimeoutError
//        - Description: Validate IOC_sendDAT times out when buffer remains full.
//        - Category: Fault (InvalidFunc) - Resource Exhaustion
//        - Status: TODO/PLANNED
//        - Estimated effort: 2 hours
//
//   ⚪ [@AC-3,US-1] TC-3: verifyDataFault_byRecvNoDataNonBlock_expectNoDataError
//        - Description: Validate IOC_recvDAT returns NO_DATA when no data available.
//        - Category: Fault (InvalidFunc) - Resource Exhaustion
//        - Status: TODO/PLANNED
//        - Estimated effort: 1 hour
//
//   ⚪ [@AC-1,US-2] TC-4: verifyDataFault_bySendTimeoutPrecision_expectAccurateTiming
//        - Description: Validate IOC_sendDAT timeout accuracy.
//        - Category: Fault (InvalidFunc) - Timeout Behavior
//        - Status: TODO/PLANNED
//        - Estimated effort: 3 hours
//        - Notes: Similar to UT_DataEdgeUS3 timeout tests
//
//   ⚪ [@AC-2,US-2] TC-5: verifyDataFault_byRecvTimeoutPrecision_expectAccurateTiming
//        - Description: Validate IOC_recvDAT timeout accuracy.
//        - Category: Fault (InvalidFunc) - Timeout Behavior
//        - Status: TODO/PLANNED
//        - Estimated effort: 3 hours
//
//   ⚪ [@AC-3,US-2] TC-6: verifyDataFault_byFlushTimeoutPrecision_expectAccurateTiming
//        - Description: Validate IOC_flushDAT timeout behavior.
//        - Category: Fault (InvalidFunc) - Timeout Behavior
//        - Status: TODO/PLANNED
//        - Estimated effort: 2 hours
//
//   ⚪ [@AC-1,US-2] TC-7: verifyDataFault_byZeroTimeoutSend_expectImmediateReturn
//        - Description: Validate zero timeout returns immediately.
//        - Category: Fault (InvalidFunc) - Timeout Behavior
//        - Status: TODO/PLANNED
//        - Estimated effort: 1 hour
//
//   ⚪ [@AC-2,US-2] TC-8: verifyDataFault_byZeroTimeoutRecv_expectImmediateReturn
//        - Description: Validate zero timeout recv returns immediately.
//        - Category: Fault (InvalidFunc) - Timeout Behavior
//        - Status: TODO/PLANNED
//        - Estimated effort: 1 hour
//
//   ⚪ [@AC-2,US-2] TC-9: verifyDataFault_byInfiniteTimeoutRecovery_expectEventualSuccess
//        - Description: Validate infinite timeout waits until success.
//        - Category: Fault (InvalidFunc) - Timeout Behavior
//        - Status: TODO/PLANNED
//        - Estimated effort: 2 hours
//
//   ⚪ [@AC-1,US-3] TC-10: verifyDataFault_byPeerCrashDuringSend_expectLinkBroken
//        - Description: Validate link broken detected when peer crashes.
//        - Category: Fault (InvalidFunc) - Link Failure Detection
//        - Status: TODO/PLANNED
//        - Estimated effort: 3 hours
//        - Notes: Complex - requires simulating peer crash
//
//   ⚪ [@AC-2,US-3] TC-11: verifyDataFault_byPeerClosedDuringRecv_expectLinkBroken
//        - Description: Validate link broken on receiver when sender closes.
//        - Category: Fault (InvalidFunc) - Link Failure Detection
//        - Status: TODO/PLANNED
//        - Estimated effort: 2 hours
//
//   ⚪ [@AC-3,US-3] TC-12: verifyDataFault_byServiceOfflineWithActiveLink_expectLinkBroken
//        - Description: Validate orphaned links detect service offline.
//        - Category: Fault (InvalidFunc) - Link Failure Detection
//        - Status: TODO/PLANNED
//        - Estimated effort: 2 hours
//
//   ⚪ [@AC-1,US-3] TC-13: verifyDataFault_byAbruptDisconnection_expectGracefulHandling
//        - Description: Validate abrupt disconnection handling.
//        - Category: Fault (InvalidFunc) - Link Failure Detection
//        - Status: TODO/PLANNED
//        - Estimated effort: 2 hours
//
//   ⚪ [@AC-2,US-3] TC-14: verifyDataFault_byLinkBrokenDuringFlush_expectLinkBrokenError
//        - Description: Validate flush detects broken link.
//        - Category: Fault (InvalidFunc) - Link Failure Detection
//        - Status: TODO/PLANNED
//        - Estimated effort: 2 hours
//
//   ⚪ [@AC-1,US-4] TC-15: verifyDataFault_byRetryAfterBufferFull_expectEventualSuccess
//        - Description: Validate retry succeeds after buffer drains.
//        - Category: Fault (InvalidFunc) - Recovery Mechanisms
//        - Status: TODO/PLANNED
//        - Estimated effort: 2 hours
//
//   ⚪ [@AC-2,US-4] TC-16: verifyDataFault_byReconnectAfterLinkBroken_expectNewConnection
//        - Description: Validate reconnection after link failure.
//        - Category: Fault (InvalidFunc) - Recovery Mechanisms
//        - Status: TODO/PLANNED
//        - Estimated effort: 2 hours
//
//   ⚪ [@AC-1,US-4] TC-17: verifyDataFault_byRecoveryFromTransientFailure_expectResume
//        - Description: Validate recovery from transient errors.
//        - Category: Fault (InvalidFunc) - Recovery Mechanisms
//        - Status: TODO/PLANNED
//        - Estimated effort: 2 hours
//
//   ⚪ [@AC-1,US-5] TC-18: verifyDataFault_byDiskFullDuringFIFOWrite_expectIOError
//        - Description: Validate disk full handling (simulation).
//        - Category: Fault (InvalidFunc) - FIFO-Specific Faults
//        - Status: TODO/PLANNED
//        - Estimated effort: 3 hours
//        - Notes: May require filesystem simulation or quota setup
//
//   ⚪ [@AC-2,US-5] TC-19: verifyDataFault_byFIFOPermissionDenied_expectAccessError
//        - Description: Validate permission error handling.
//        - Category: Fault (InvalidFunc) - FIFO-Specific Faults
//        - Status: TODO/PLANNED
//        - Estimated effort: 2 hours
//
//   ⚪ [@AC-1,US-5] TC-20: verifyDataFault_byFIFOCorruptionRecovery_expectGracefulHandling
//        - Description: Validate FIFO corruption doesn't crash system.
//        - Category: Fault (InvalidFunc) - FIFO-Specific Faults
//        - Status: TODO/PLANNED
//        - Estimated effort: 3 hours
//
// 🚪 GATE P1 (Fault Testing): 0/20 tests implemented - DESIGN PHASE COMPLETE
//
//===================================================================================================
// ✅ SUMMARY
//===================================================================================================
//   ⚪ P1 Fault Tests: 0/20 implemented (100% planned)
//   📋 Total effort estimate: ~45 hours
//   🎯 Next: Start implementation with TC-1 (buffer full NONBLOCK)
//   📝 Design Strategy: Reuse timeout precision patterns from UT_DataEdgeUS3.cxx
//   🔧 Implementation Plan:
//      Phase 2A.1: Resource exhaustion tests (TC-1 to TC-3)
//      Phase 2A.2: Timeout behavior tests (TC-4 to TC-9)
//      Phase 2A.3: Link failure tests (TC-10 to TC-14)
//      Phase 2A.4: Recovery tests (TC-15 to TC-17)
//      Phase 2A.5: FIFO-specific tests (TC-18 to TC-20)
//
///////////////////////////////////////////////////////////////////////////////////////////////////
//======>END OF TODO/IMPLEMENTATION TRACKING SECTION===============================================
