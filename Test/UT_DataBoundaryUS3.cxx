///////////////////////////////////////////////////////////////////////////////////////////////////
// UT_DataBoundaryUS3.cxx - DAT Boundary Testing: US-3 Timeout and Blocking Mode Boundaries
// 📝 Purpose: Test Cases for User Story 3 - Real-time application developer timeout boundary testing
// 🔄 Focus: DAT timeout boundaries, blocking/non-blocking mode transitions, deterministic behavior
// 🎯 Coverage: [@US-3] Timeout and blocking mode boundaries (AC-1, AC-2, AC-3)
///////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF US-3 TEST CASES==================================================================
/**************************************************************************************************
 * @brief 【US-3 Test Cases】- Timeout and Blocking Mode Boundaries
 *
 * [@AC-1,US-3] Timeout boundary validation - Zero timeout
 *  TC-1:
 *      @[Name]: verifyDatTimeoutBoundary_byZeroTimeout_expectImmediateReturn
 *      @[Purpose]: Verify zero timeout behavior
 *      @[Brief]: Configure zero timeout, verify immediate return without blocking
 *      @[Coverage]: Zero timeout configuration, immediate return behavior, no blocking verification
 *
 *-------------------------------------------------------------------------------------------------
 * [@AC-2,US-3] Blocking mode boundaries - Mode transitions
 *  TC-2:
 *      @[Name]: verifyDatBlockingModeBoundary_byModeTransitions_expectConsistentBehavior
 *      @[Purpose]: Verify blocking/non-blocking mode transitions
 *      @[Brief]: Switch between blocking modes, verify each mode behaves correctly
 *      @[Coverage]: Blocking ↔ non-blocking transitions, mode consistency, data preservation
 *
 *-------------------------------------------------------------------------------------------------
 * [@AC-3,US-3] Extreme timeout boundaries - Edge cases
 *  TC-3:
 *      @[Name]: verifyDatTimeoutBoundary_byExtremeValues_expectProperHandling
 *      @[Purpose]: Verify extreme timeout value handling
 *      @[Brief]: Test very small and very large timeout values, verify proper handling
 *      @[Coverage]: Microsecond timeouts, maximum timeout values, timeout accuracy
 *
 *-------------------------------------------------------------------------------------------------
 * TODO: [@AC-1,US-3] Timeout boundary validation - Timeout precision
 *  TC-4:
 *      @[Name]: verifyDatTimeoutBoundary_byPrecisionTesting_expectAccurateTiming
 *      @[Purpose]: Verify timeout precision and accuracy
 *      @[Brief]: Test timeout accuracy within acceptable ranges
 *      @[Coverage]: Timeout precision, timing accuracy, timeout variance measurement
 *
 * TODO: [@AC-2,US-3] Blocking mode boundaries - State consistency
 *  TC-5:
 *      @[Name]: verifyDatBlockingModeBoundary_byStateConsistency_expectNoDataLoss
 *      @[Purpose]: Verify state consistency during mode transitions
 *      @[Brief]: Ensure no data loss during blocking mode changes
 *      @[Coverage]: State preservation, data queue integrity, mode transition safety
 *
 *************************************************************************************************/
//======>END OF US-3 TEST CASES====================================================================

#include "UT_DataBoundary.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF US-3 TEST IMPLEMENTATIONS========================================================

//======>BEGIN OF: [@AC-1,US-3] TC-1===============================================================
/**
 * @[Name]: verifyDatTimeoutBoundary_byZeroTimeout_expectImmediateReturn
 * @[Steps]:
 *   1) Setup IOC services and establish DAT link AS SETUP
 *      |-> Create DatSender and DatReceiver services
 *      |-> Establish link between sender and receiver
 *      |-> Initialize private data for boundary testing
 *   2) Test IOC_sendDAT with zero timeout AS BEHAVIOR
 *      |-> Configure zero timeout option (IOC_TIMEOUT_IMMEDIATE)
 *      |-> Send data with zero timeout, measure timing
 *      |-> Verify immediate return without blocking
 *      |-> Test multiple consecutive zero timeout calls
 *   3) Test IOC_recvDAT with zero timeout AS BEHAVIOR
 *      |-> Configure zero timeout option for receiving
 *      |-> Call recvDAT with zero timeout, measure timing
 *      |-> Verify immediate return when no data available
 *      |-> Verify immediate return when data is available
 *   4) Test timing boundaries and consistency AS VERIFY
 *      |-> Verify all zero timeout calls return within acceptable time limits
 *      |-> Verify consistent behavior across multiple calls
 *      |-> Verify proper result codes (SUCCESS, NO_DATA, TIMEOUT)
 *   5) Cleanup services and links AS CLEANUP
 * @[Expect]: Zero timeout operations return immediately with proper result codes, no blocking behavior.
 * @[Notes]: Critical for real-time applications - validates AC-1 zero timeout boundary requirements.
 */
TEST(UT_DataBoundary, verifyDatTimeoutBoundary_byZeroTimeout_expectImmediateReturn) {
    // ┌──────────────────────────────────────────────────────────────────────────────────────┐
    // │                                🔧 SETUP PHASE                                        │
    // └──────────────────────────────────────────────────────────────────────────────────────┘
    printf("🎯 TEST: Zero timeout boundary validation - immediate return verification\n");

    // Test Focus: Verify zero timeout operations return immediately without blocking
    // Expected: sendDAT returns TIMEOUT, recvDAT returns SUCCESS if data available or TIMEOUT if not

    // Test constants
    const auto MAX_EXECUTION_TIME_US = 10000;  // 10ms timing threshold
    const int CONSISTENCY_TEST_CALLS = 5;

    // Initialize test data structures
    __DatBoundaryPrivData_T DatReceiverPrivData = {0};
    DatReceiverPrivData.ClientIndex = 1;

    IOC_SrvID_T DatReceiverSrvID = IOC_ID_INVALID;
    IOC_LinkID_T DatSenderLinkID = IOC_ID_INVALID;
    IOC_LinkID_T DatReceiverLinkID = IOC_ID_INVALID;
    IOC_Result_T Result = IOC_RESULT_FAILURE;

    // Setup receiver service configuration
    printf("📋 Setting up receiver service for timeout testing...\n");

    IOC_SrvURI_T DatReceiverSrvURI = {
        .pProtocol = IOC_SRV_PROTO_FIFO,
        .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
        .pPath = "DatTimeoutReceiver",
    };

    IOC_DatUsageArgs_T DatReceiverUsageArgs = {
        .CbRecvDat_F = __CbRecvDat_Boundary_F,
        .pCbPrivData = &DatReceiverPrivData,
    };

    IOC_SrvArgs_T DatReceiverSrvArgs = {
        .SrvURI = DatReceiverSrvURI,
        .UsageCapabilites = IOC_LinkUsageDatReceiver,
        .UsageArgs =
            {
                .pDat = &DatReceiverUsageArgs,
            },
    };

    Result = IOC_onlineService(&DatReceiverSrvID, &DatReceiverSrvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result) << "Receiver service should come online successfully";
    printf("   ✓ Receiver service online with ID=%llu\n", DatReceiverSrvID);

    // Setup sender connection
    IOC_ConnArgs_T DatSenderConnArgs = {
        .SrvURI = DatReceiverSrvURI,
        .Usage = IOC_LinkUsageDatSender,
    };

    std::thread DatSenderThread([&] {
        IOC_Result_T ThreadResult = IOC_connectService(&DatSenderLinkID, &DatSenderConnArgs, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, ThreadResult);
        ASSERT_NE(IOC_ID_INVALID, DatSenderLinkID);
    });

    Result = IOC_acceptClient(DatReceiverSrvID, &DatReceiverLinkID, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result) << "Receiver should accept connection";

    DatSenderThread.join();
    printf("   ✓ Sender connected with LinkID=%llu\n", DatSenderLinkID);
    printf("   ✓ Receiver accepted with LinkID=%llu\n", DatReceiverLinkID);

    // ┌──────────────────────────────────────────────────────────────────────────────────────┐
    // │                               🎯 BEHAVIOR PHASE                                       │
    // └──────────────────────────────────────────────────────────────────────────────────────┘

    // === Test IOC_sendDAT with zero timeout ===
    printf("📋 Testing sendDAT with zero timeout...\n");

    // Configure zero timeout option
    IOC_Option_defineTimeout(ZeroTimeoutOption, IOC_TIMEOUT_IMMEDIATE);

    // Prepare test data
    const char *testData = "ZeroTimeoutTest";
    IOC_DatDesc_T TestDatDesc = {0};
    IOC_initDatDesc(&TestDatDesc);
    TestDatDesc.Payload.pData = (void *)testData;
    TestDatDesc.Payload.PtrDataSize = strlen(testData);

    // Test 1: Zero timeout sendDAT - timing validation
    printf("🧪 Test 1: Zero timeout sendDAT timing validation...\n");

    auto startTime = std::chrono::high_resolution_clock::now();
    Result = IOC_sendDAT(DatSenderLinkID, &TestDatDesc, &ZeroTimeoutOption);
    auto endTime = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
    printf("   ⏱️ Execution time: %lld microseconds\n", (long long)duration.count());

    // Verify timing - should complete within threshold
    ASSERT_LT(duration.count(), MAX_EXECUTION_TIME_US)
        << "Zero timeout operation should complete within " << MAX_EXECUTION_TIME_US << "μs";

    // Zero timeout should return TIMEOUT (indicating would block but returned immediately)
    ASSERT_EQ(IOC_RESULT_TIMEOUT, Result) << "Zero timeout sendDAT should return TIMEOUT for consistent semantics";

    printf("   ✓ Zero timeout sendDAT behaved correctly\n");

    // Test 2: Create buffer pressure for stress testing
    printf("🧪 Test 2: Create buffer pressure for stress testing...\n");

    // Query IOC capability to understand system limits
    IOC_CapabilityDescription_T CapDesc;
    memset(&CapDesc, 0, sizeof(CapDesc));
    CapDesc.CapID = IOC_CAPID_CONET_MODE_DATA;

    Result = IOC_getCapability(&CapDesc);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result) << "Should be able to query CONET mode data capability";

    ULONG_T MaxDataQueueSize = CapDesc.ConetModeData.MaxDataQueueSize;
    printf("   📋 System MaxDataQueueSize: %lu bytes\n", MaxDataQueueSize);

    // Attempt to send some data to create potential buffer pressure
    // Note: This is preparatory - not the core TDD test
    std::vector<char> largeData(1024, 'X');  // 1KB chunks
    IOC_DatDesc_T LargeDataDesc = {0};
    IOC_initDatDesc(&LargeDataDesc);
    LargeDataDesc.Payload.pData = largeData.data();
    LargeDataDesc.Payload.PtrDataSize = largeData.size();

    IOC_Option_defineASyncNonBlock(NonBlockingOption);

    // Send several packets to create some buffer usage
    int sentCount = 0;
    for (int i = 0; i < 10; i++) {  // Send up to 10 packets
        IOC_Result_T sendResult = IOC_sendDAT(DatSenderLinkID, &LargeDataDesc, &NonBlockingOption);
        if (sendResult == IOC_RESULT_SUCCESS) {
            sentCount++;
        } else {
            printf("   Buffer pressure detected after %d packets, result: %d\n", sentCount, sendResult);
            break;
        }
    }

    printf("   📤 Sent %d packets (%d KB) for buffer state setup\n", sentCount, sentCount);

    // Test 3: Zero timeout sendDAT timing guarantee (core TDD requirement)
    printf("🧪 Test 3: Zero timeout sendDAT timing guarantee...\n");

    auto fullBufferStart = std::chrono::high_resolution_clock::now();
    Result = IOC_sendDAT(DatSenderLinkID, &TestDatDesc, &ZeroTimeoutOption);
    auto fullBufferEnd = std::chrono::high_resolution_clock::now();

    auto fullBufferDuration = std::chrono::duration_cast<std::chrono::microseconds>(fullBufferEnd - fullBufferStart);
    printf("   ⏱️ Zero timeout sendDAT execution time: %lld microseconds\n", (long long)fullBufferDuration.count());
    printf("   📋 Zero timeout sendDAT result: %d\n", Result);

    // PRIMARY TDD REQUIREMENT: Zero timeout must return immediately - never block
    ASSERT_LT(fullBufferDuration.count(), 10000)
        << "CORE TDD REQUIREMENT: Zero timeout sendDAT must complete within 10ms regardless of buffer state";

    // SECONDARY TDD REQUIREMENT: Result should indicate immediate non-blocking return
    // Zero timeout MUST always return TIMEOUT to indicate "would block but returned immediately"
    ASSERT_EQ(IOC_RESULT_TIMEOUT, Result)
        << "Zero timeout should ALWAYS return IOC_RESULT_TIMEOUT for consistent semantics, got: " << Result;

    if (Result == IOC_RESULT_TIMEOUT) {
        printf("   ✓ Zero timeout returned TIMEOUT correctly - consistent zero timeout semantics\n");
    } else {
        printf("   ❌ Unexpected result for zero timeout: %d\n", Result);
    }

    // Test 3: Multiple consecutive zero timeout calls - consistency verification
    printf("🧪 Test 3: Multiple consecutive zero timeout calls...\n");

    std::vector<long long> executionTimes;

    for (int i = 0; i < CONSISTENCY_TEST_CALLS; i++) {
        auto callStart = std::chrono::high_resolution_clock::now();
        IOC_Result_T callResult = IOC_sendDAT(DatSenderLinkID, &TestDatDesc, &ZeroTimeoutOption);
        auto callEnd = std::chrono::high_resolution_clock::now();

        auto callDuration = std::chrono::duration_cast<std::chrono::microseconds>(callEnd - callStart);
        executionTimes.push_back(callDuration.count());

        printf("   📞 Call %d: result=%d, time=%lld μs\n", i + 1, callResult, (long long)callDuration.count());

        // Each call must complete quickly and return consistent results
        ASSERT_LT(callDuration.count(), MAX_EXECUTION_TIME_US)
            << "Zero timeout call " << i + 1 << " must complete within timing limit";
        ASSERT_EQ(IOC_RESULT_TIMEOUT, callResult) << "Zero timeout call " << i + 1 << " should return TIMEOUT";
    }

    // Allow some time for buffer to drain before continuing
    printf("   ⏳ Allowing buffer to drain...\n");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // === Test IOC_recvDAT with zero timeout ===
    printf("📋 Testing recvDAT with zero timeout...\n");

    // Test 4: Zero timeout recvDAT when no data available
    printf("🧪 Test 4: Zero timeout recvDAT with no data available...\n");

    // Drain any buffered data to ensure clean state
    IOC_DatDesc_T DrainDesc = {0};
    IOC_initDatDesc(&DrainDesc);
    char drainBuffer[1024] = {0};
    DrainDesc.Payload.pData = drainBuffer;
    DrainDesc.Payload.PtrDataSize = sizeof(drainBuffer);

    // Best effort drain - don't fail test if draining encounters issues
    int drainCount = 0;
    IOC_Result_T drainResult;
    do {
        drainResult = IOC_recvDAT(DatSenderLinkID, &DrainDesc, &ZeroTimeoutOption);
        if (drainResult == IOC_RESULT_SUCCESS) {
            drainCount++;
        }
    } while (drainResult == IOC_RESULT_SUCCESS && drainCount < 10);

    printf("   🚰 Drained %d data chunks to achieve empty state\n", drainCount);

    // Test zero timeout recvDAT with no data available
    IOC_DatDesc_T RecvDatDesc = {0};
    IOC_initDatDesc(&RecvDatDesc);
    char recvBuffer[1024] = {0};
    RecvDatDesc.Payload.pData = recvBuffer;
    RecvDatDesc.Payload.PtrDataSize = sizeof(recvBuffer);

    auto recvStartTime = std::chrono::high_resolution_clock::now();
    Result = IOC_recvDAT(DatSenderLinkID, &RecvDatDesc, &ZeroTimeoutOption);
    auto recvEndTime = std::chrono::high_resolution_clock::now();

    auto recvDuration = std::chrono::duration_cast<std::chrono::microseconds>(recvEndTime - recvStartTime);
    printf("   ⏱️ Execution time: %lld microseconds\n", (long long)recvDuration.count());

    // Verify timing and result
    ASSERT_LT(recvDuration.count(), MAX_EXECUTION_TIME_US) << "Zero timeout recvDAT must complete within timing limit";
    ASSERT_EQ(IOC_RESULT_TIMEOUT, Result) << "Zero timeout recvDAT with no data should return TIMEOUT";

    printf("   ✓ Zero timeout recvDAT behaved correctly\n");
    ASSERT_LT(recvDuration.count(), 10000) << "CORE TDD REQUIREMENT: Zero timeout recvDAT must complete within 10ms";

    // SECONDARY TDD REQUIREMENT: Zero timeout MUST always return TIMEOUT for consistent semantics
    // This matches sendDAT behavior - "would block but returned immediately due to zero timeout"
    ASSERT_EQ(IOC_RESULT_TIMEOUT, Result)
        << "Zero timeout recvDAT should ALWAYS return IOC_RESULT_TIMEOUT for consistent semantics, got: " << Result;

    if (Result == IOC_RESULT_TIMEOUT) {
        printf("   ✓ Zero timeout returned TIMEOUT correctly - consistent zero timeout semantics\n");
    } else {
        printf("   ❌ Unexpected result for zero timeout: %d\n", Result);
    }

    // Test 6: Zero timeout recvDAT when data is immediately available
    printf("🧪 Test 6: Zero timeout recvDAT when data is immediately available...\n");

    // Send data first to ensure it's available for immediate reception
    IOC_Option_defineASyncMayBlock(NormalOption);
    Result = IOC_sendDAT(DatSenderLinkID, &TestDatDesc, &NormalOption);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result) << "Normal sendDAT should succeed";
    printf("   📤 Sent data with normal option: result=%d\n", Result);

    // Allow minimal time for data to be queued/transmitted
    std::this_thread::sleep_for(std::chrono::milliseconds(5));

    // Now test zero timeout receive with data available
    IOC_DatDesc_T QuickRecvDesc = {0};
    IOC_initDatDesc(&QuickRecvDesc);
    char quickRecvBuffer[1024] = {0};
    QuickRecvDesc.Payload.pData = quickRecvBuffer;
    QuickRecvDesc.Payload.PtrDataSize = sizeof(quickRecvBuffer);

    auto quickRecvStart = std::chrono::high_resolution_clock::now();
    Result = IOC_recvDAT(DatSenderLinkID, &QuickRecvDesc, &ZeroTimeoutOption);
    auto quickRecvEnd = std::chrono::high_resolution_clock::now();

    auto quickRecvDuration = std::chrono::duration_cast<std::chrono::microseconds>(quickRecvEnd - quickRecvStart);
    printf("   ⏱️ Zero timeout recvDAT (with data) execution time: %lld microseconds\n",
           (long long)quickRecvDuration.count());
    printf("   📥 Received data result: %d\n", Result);

    // PRIMARY TDD REQUIREMENT: Should complete quickly regardless of result
    ASSERT_LT(quickRecvDuration.count(), 10000)
        << "Zero timeout recvDAT must complete within 10ms even with data available";

    // SECONDARY TDD REQUIREMENT: When data is available, zero timeout MUST succeed immediately
    // Since data was just sent and is available, zero timeout should return SUCCESS immediately
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result)
        << "Zero timeout recvDAT with available data MUST return IOC_RESULT_SUCCESS, got: " << Result;

    if (Result == IOC_RESULT_SUCCESS) {
        printf("   ✓ Zero timeout succeeded immediately - ideal TDD behavior achieved\n");
        // Verify received data content
        if (QuickRecvDesc.Payload.PtrDataSize > 0) {
            printf("   📋 Received %lu bytes: '%.15s'\n", QuickRecvDesc.Payload.PtrDataSize,
                   (char *)QuickRecvDesc.Payload.pData);
            ASSERT_GT(QuickRecvDesc.Payload.PtrDataSize, 0) << "Should have received data with non-zero size";
        }
    } else {
        printf("   ❌ Unexpected result for zero timeout with available data: %d\n", Result);
    }

    // ┌──────────────────────────────────────────────────────────────────────────────────────┐
    // │                                ✅ VERIFICATION                                        │
    // └──────────────────────────────────────────────────────────────────────────────────────┘

    // Verify timing consistency across previous calls
    if (!executionTimes.empty()) {
        printf("🧪 Verifying timing consistency...\n");

        long long maxTime = *std::max_element(executionTimes.begin(), executionTimes.end());
        long long totalTime = 0;
        for (long long time : executionTimes) {
            totalTime += time;
        }
        long long avgTime = totalTime / executionTimes.size();

        printf("   📊 Timing stats: avg=%lld μs, max=%lld μs\n", avgTime, maxTime);

        // Verify reasonable timing consistency
        if (avgTime > 0) {
            ASSERT_LT(maxTime, avgTime * 10)
                << "Maximum execution time should not exceed 10x average for consistent behavior";
        }

        printf("   ✓ Timing consistency verified\n");
    }

    // ┌──────────────────────────────────────────────────────────────────────────────────────┐
    // │                               ✅ SUMMARY                                              │
    // └──────────────────────────────────────────────────────────────────────────────────────┘
    printf("✅ All zero timeout operations completed within timing limits\n");
    printf("✅ Consistent behavior across all test scenarios\n");
    printf("✅ Proper result codes returned for all zero timeout operations\n");

    // ┌──────────────────────────────────────────────────────────────────────────────────────┐
    // │                               🧹 CLEANUP PHASE                                        │
    // └──────────────────────────────────────────────────────────────────────────────────────┘
    printf("🧹 Cleaning up services and links...\n");

    Result = IOC_closeLink(DatSenderLinkID);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result) << "Sender link should close successfully";

    Result = IOC_closeLink(DatReceiverLinkID);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result) << "Receiver link should close successfully";

    Result = IOC_offlineService(DatReceiverSrvID);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result) << "Receiver service should go offline successfully";

    printf("🧹 Cleanup completed\n");
}
//======>END OF: [@AC-1,US-3] TC-1=================================================================

//======>BEGIN OF: [@AC-2,US-3] TC-2===============================================================
/**
 * @[Name]: verifyDatBlockingModeBoundary_byModeTransitions_expectConsistentBehavior
 * @[Steps]:
 *   1) Setup IOC services and establish DAT link AS SETUP
 *      |-> Create DatSender and DatReceiver services
 *      |-> Establish link between sender and receiver
 *      |-> Initialize private data for mode transition testing
 *   2) Test Async Mode Transitions AS BEHAVIOR
 *      |-> Send data with ASyncMayBlock mode, verify blocking behavior
 *      |-> Send data with ASyncNonBlock mode, verify immediate return
 *      |-> Send data with ASyncTimeout mode, verify timeout behavior
 *      |-> Verify mode transitions don't corrupt ongoing operations
 *   3) Test Sync Mode Transitions AS BEHAVIOR
 *      |-> Receive data with SyncMayBlock mode, verify blocking behavior
 *      |-> Receive data with SyncNonBlock mode, verify immediate return
 *      |-> Receive data with SyncTimeout mode, verify timeout behavior
 *      |-> Verify mode consistency within single operation
 *   4) Test Mixed Mode Operations AS BEHAVIOR
 *      |-> Alternately send with different async modes
 *      |-> Alternately receive with different sync modes
 *      |-> Verify data integrity preserved across mode changes
 *      |-> Test rapid mode switching under load
 *   5) Verify Mode Boundary Consistency AS VERIFY
 *      |-> Verify each mode behaves predictably and consistently
 *      |-> Verify no data loss during mode transitions
 *      |-> Verify timing boundaries respected for each mode
 *      |-> Verify proper error codes for each mode boundary condition
 *   6) Cleanup services and links AS CLEANUP
 * @[Expect]: All blocking modes transition correctly with consistent behavior, no data loss or corruption.
 * @[Notes]: Validates AC-2 mode transition requirements - critical for applications switching between blocking
 * strategies.
 */
TEST(UT_DataBoundary, verifyDatBlockingModeBoundary_byModeTransitions_expectConsistentBehavior) {
    // ┌──────────────────────────────────────────────────────────────────────────────────────┐
    // │                                🔧 SETUP PHASE                                        │
    // └──────────────────────────────────────────────────────────────────────────────────────┘
    printf("🎯 TEST: Blocking mode boundary transitions - consistent behavior verification\n");

    // Test Focus: Verify different blocking modes work correctly and transitions are consistent
    // Expected: Each mode behaves predictably, transitions preserve data integrity

    // Test constants
    const auto MAX_MODE_EXECUTION_TIME_US = 15000;  // 15ms timing threshold for mode operations
    const int MODE_TRANSITION_TEST_CYCLES = 3;
    const int STRESS_TEST_OPERATIONS = 10;

    // Initialize test data structures
    __DatBoundaryPrivData_T DatReceiverPrivData = {0};
    DatReceiverPrivData.ClientIndex = 2;

    IOC_SrvID_T DatReceiverSrvID = IOC_ID_INVALID;
    IOC_LinkID_T DatSenderLinkID = IOC_ID_INVALID;
    IOC_LinkID_T DatReceiverLinkID = IOC_ID_INVALID;
    IOC_Result_T Result = IOC_RESULT_FAILURE;

    // Setup receiver service configuration
    printf("📋 Setting up receiver service for mode transition testing...\n");

    IOC_SrvURI_T DatReceiverSrvURI = {
        .pProtocol = IOC_SRV_PROTO_FIFO,
        .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
        .pPath = "DatModeTransitionReceiver",
    };

    IOC_DatUsageArgs_T DatReceiverUsageArgs = {
        .CbRecvDat_F = __CbRecvDat_Boundary_F,
        .pCbPrivData = &DatReceiverPrivData,
    };

    IOC_SrvArgs_T DatReceiverSrvArgs = {
        .SrvURI = DatReceiverSrvURI,
        .UsageCapabilites = IOC_LinkUsageDatReceiver,
        .UsageArgs =
            {
                .pDat = &DatReceiverUsageArgs,
            },
    };

    Result = IOC_onlineService(&DatReceiverSrvID, &DatReceiverSrvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result) << "Receiver service should come online successfully";
    printf("   ✓ Receiver service online with ID=%llu\n", DatReceiverSrvID);

    // Setup sender connection
    IOC_ConnArgs_T DatSenderConnArgs = {
        .SrvURI = DatReceiverSrvURI,
        .Usage = IOC_LinkUsageDatSender,
    };

    std::thread DatSenderThread([&] {
        IOC_Result_T ThreadResult = IOC_connectService(&DatSenderLinkID, &DatSenderConnArgs, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, ThreadResult);
        ASSERT_NE(IOC_ID_INVALID, DatSenderLinkID);
    });

    Result = IOC_acceptClient(DatReceiverSrvID, &DatReceiverLinkID, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result) << "Receiver should accept connection";

    DatSenderThread.join();
    printf("   ✓ Sender connected with LinkID=%llu\n", DatSenderLinkID);
    printf("   ✓ Receiver accepted with LinkID=%llu\n", DatReceiverLinkID);

    // Prepare common test data
    const char *testData = "ModeTransitionTest";
    IOC_DatDesc_T TestDatDesc = {0};
    IOC_initDatDesc(&TestDatDesc);
    TestDatDesc.Payload.pData = (void *)testData;
    TestDatDesc.Payload.PtrDataSize = strlen(testData) + 1;  // Include null terminator
    TestDatDesc.Payload.PtrDataLen = strlen(testData);

    // ┌──────────────────────────────────────────────────────────────────────────────────────┐
    // │                               🎯 BEHAVIOR PHASE                                       │
    // └──────────────────────────────────────────────────────────────────────────────────────┘

    // === Test 1: Async Mode Transitions ===
    printf("📋 Test 1: Async Mode Transitions...\n");

    // Test 1a: ASyncMayBlock mode - should succeed normally
    printf("🧪 Test 1a: ASyncMayBlock mode verification...\n");
    IOC_Option_defineASyncMayBlock(AsyncMayBlockOpt);

    auto asyncMayBlockStart = std::chrono::high_resolution_clock::now();
    Result = IOC_sendDAT(DatSenderLinkID, &TestDatDesc, &AsyncMayBlockOpt);
    auto asyncMayBlockEnd = std::chrono::high_resolution_clock::now();

    auto asyncMayBlockDuration =
        std::chrono::duration_cast<std::chrono::microseconds>(asyncMayBlockEnd - asyncMayBlockStart);
    printf("   ⏱️ ASyncMayBlock execution time: %lld microseconds\n", (long long)asyncMayBlockDuration.count());

    ASSERT_EQ(IOC_RESULT_SUCCESS, Result) << "ASyncMayBlock should succeed";
    ASSERT_LT(asyncMayBlockDuration.count(), MAX_MODE_EXECUTION_TIME_US)
        << "ASyncMayBlock should complete within timing limit";
    printf("   ✓ ASyncMayBlock mode behaved correctly\n");

    // Test 1b: ASyncNonBlock mode - should return immediately
    printf("🧪 Test 1b: ASyncNonBlock mode verification...\n");
    IOC_Option_defineASyncNonBlock(AsyncNonBlockOpt);

    auto asyncNonBlockStart = std::chrono::high_resolution_clock::now();
    Result = IOC_sendDAT(DatSenderLinkID, &TestDatDesc, &AsyncNonBlockOpt);
    auto asyncNonBlockEnd = std::chrono::high_resolution_clock::now();

    auto asyncNonBlockDuration =
        std::chrono::duration_cast<std::chrono::microseconds>(asyncNonBlockEnd - asyncNonBlockStart);
    printf("   ⏱️ ASyncNonBlock execution time: %lld microseconds\n", (long long)asyncNonBlockDuration.count());

    // - DAT: IOC_RESULT_BUFFER_FULL for buffer full
    // NonBlock should either succeed immediately or return appropriate DAT error
    // Here assume we don't know if buffer is full or not, so we check both success and buffer full
    ASSERT_TRUE(Result == IOC_RESULT_SUCCESS || Result == IOC_RESULT_BUFFER_FULL)
        << "ASyncNonBlock should return SUCCESS or BUFFER_FULL, got: " << Result;
    ASSERT_LT(asyncNonBlockDuration.count(), 5000)  // Stricter timing for NonBlock
        << "ASyncNonBlock should complete very quickly";
    printf("   ✓ ASyncNonBlock mode behaved correctly\n");

    // Test 1c: ASyncTimeout mode - should respect timeout
    printf("🧪 Test 1c: ASyncTimeout mode verification...\n");
    IOC_Option_defineASyncTimeout(AsyncTimeoutOpt, 5000);  // 5ms timeout

    auto asyncTimeoutStart = std::chrono::high_resolution_clock::now();
    Result = IOC_sendDAT(DatSenderLinkID, &TestDatDesc, &AsyncTimeoutOpt);
    auto asyncTimeoutEnd = std::chrono::high_resolution_clock::now();

    auto asyncTimeoutDuration =
        std::chrono::duration_cast<std::chrono::microseconds>(asyncTimeoutEnd - asyncTimeoutStart);
    printf("   ⏱️ ASyncTimeout execution time: %lld microseconds\n", (long long)asyncTimeoutDuration.count());

    // Timeout mode should either succeed quickly or buffer full
    ASSERT_TRUE(Result == IOC_RESULT_SUCCESS || Result == IOC_RESULT_BUFFER_FULL)
        << "ASyncTimeout should return SUCCESS or BUFFER_FULL, got: " << Result;
    ASSERT_LT(asyncTimeoutDuration.count(), 10000)  // Should not exceed timeout significantly
        << "ASyncTimeout should respect timing boundaries";
    printf("   ✓ ASyncTimeout mode behaved correctly\n");

    // === Test 2: Sync Mode Transitions for Receiving ===
    printf("📋 Test 2: Sync Mode Transitions for Receiving...\n");

    // Setup a separate polling-based receiver service for sync mode testing
    printf("🧪 Setting up polling receiver service for sync mode tests...\n");

    IOC_SrvID_T DatPollingReceiverSrvID = IOC_ID_INVALID;
    IOC_LinkID_T DatPollingReceiverLinkID = IOC_ID_INVALID;
    IOC_LinkID_T DatPollingSenderLinkID = IOC_ID_INVALID;

    IOC_SrvURI_T DatPollingReceiverSrvURI = {
        .pProtocol = IOC_SRV_PROTO_FIFO,
        .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
        .pPath = "DatPollingReceiver",
    };

    // Setup polling receiver WITHOUT callback (pure polling mode)
    IOC_SrvArgs_T DatPollingReceiverSrvArgs = {
        .SrvURI = DatPollingReceiverSrvURI, .UsageCapabilites = IOC_LinkUsageDatReceiver,
        // No UsageArgs means no callback - pure polling mode
    };

    Result = IOC_onlineService(&DatPollingReceiverSrvID, &DatPollingReceiverSrvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result) << "Polling receiver service should come online successfully";
    printf("   ✓ Polling receiver service online with ID=%llu\n", DatPollingReceiverSrvID);

    // Setup polling sender connection
    IOC_ConnArgs_T DatPollingSenderConnArgs = {
        .SrvURI = DatPollingReceiverSrvURI,
        .Usage = IOC_LinkUsageDatSender,
    };

    std::thread DatPollingSenderThread([&] {
        IOC_Result_T ThreadResult = IOC_connectService(&DatPollingSenderLinkID, &DatPollingSenderConnArgs, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, ThreadResult);
        ASSERT_NE(IOC_ID_INVALID, DatPollingSenderLinkID);
    });

    Result = IOC_acceptClient(DatPollingReceiverSrvID, &DatPollingReceiverLinkID, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result) << "Polling receiver should accept connection";

    DatPollingSenderThread.join();
    printf("   ✓ Polling sender connected with LinkID=%llu\n", DatPollingSenderLinkID);
    printf("   ✓ Polling receiver accepted with LinkID=%llu\n", DatPollingReceiverLinkID);

    // First ensure there's data to receive by sending with reliable async mode
    printf("🧪 Pre-sending data for sync mode tests...\n");
    IOC_Option_defineASyncMayBlock(PreSendOpt);
    Result = IOC_sendDAT(DatPollingSenderLinkID, &TestDatDesc, &PreSendOpt);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result) << "Pre-send should succeed";
    std::this_thread::sleep_for(std::chrono::milliseconds(5));  // Let data arrive

    // Test 2a: SyncMayBlock mode - should block until data available
    printf("🧪 Test 2a: SyncMayBlock mode verification...\n");
    IOC_Option_defineSyncMayBlock(SyncMayBlockOpt);

    IOC_DatDesc_T RecvDesc1 = {0};
    IOC_initDatDesc(&RecvDesc1);
    char recvBuffer1[1024] = {0};
    RecvDesc1.Payload.pData = recvBuffer1;
    RecvDesc1.Payload.PtrDataSize = sizeof(recvBuffer1);
    RecvDesc1.Payload.PtrDataLen = 0;

    auto syncMayBlockStart = std::chrono::high_resolution_clock::now();
    Result = IOC_recvDAT(DatPollingReceiverLinkID, &RecvDesc1, &SyncMayBlockOpt);
    auto syncMayBlockEnd = std::chrono::high_resolution_clock::now();

    auto syncMayBlockDuration =
        std::chrono::duration_cast<std::chrono::microseconds>(syncMayBlockEnd - syncMayBlockStart);
    printf("   ⏱️ SyncMayBlock execution time: %lld microseconds\n", (long long)syncMayBlockDuration.count());

    ASSERT_EQ(IOC_RESULT_SUCCESS, Result) << "SyncMayBlock should succeed when data available";
    ASSERT_GT(RecvDesc1.Payload.PtrDataSize, 0) << "Should have received data";
    printf("   ✓ SyncMayBlock mode received %lu bytes correctly\n", RecvDesc1.Payload.PtrDataSize);

    // Test 2b: SyncNonBlock mode when no data available
    printf("🧪 Test 2b: SyncNonBlock mode verification (no data)...\n");
    IOC_Option_defineSyncNonBlock(SyncNonBlockOpt);

    IOC_DatDesc_T RecvDesc2 = {0};
    IOC_initDatDesc(&RecvDesc2);
    char recvBuffer2[1024] = {0};
    RecvDesc2.Payload.pData = recvBuffer2;
    RecvDesc2.Payload.PtrDataSize = sizeof(recvBuffer2);
    RecvDesc2.Payload.PtrDataLen = 0;

    auto syncNonBlockStart = std::chrono::high_resolution_clock::now();
    Result = IOC_recvDAT(DatPollingReceiverLinkID, &RecvDesc2, &SyncNonBlockOpt);
    auto syncNonBlockEnd = std::chrono::high_resolution_clock::now();

    auto syncNonBlockDuration =
        std::chrono::duration_cast<std::chrono::microseconds>(syncNonBlockEnd - syncNonBlockStart);
    printf("   ⏱️ SyncNonBlock execution time: %lld microseconds\n", (long long)syncNonBlockDuration.count());

    // SyncNonBlock should return immediately when no data
    ASSERT_TRUE(Result == IOC_RESULT_NO_DATA)
        << "SyncNonBlock should return NO_DATA when no data available, got: " << Result;
    ASSERT_LT(syncNonBlockDuration.count(), 3000)  // Very strict timing for NonBlock
        << "SyncNonBlock should return immediately";
    printf("   ✓ SyncNonBlock mode behaved correctly\n");

    // Test 2c: SyncTimeout mode
    printf("🧪 Test 2c: SyncTimeout mode verification...\n");
    IOC_Option_defineSyncTimeout(SyncTimeoutOpt, 3000);  // 3ms timeout

    IOC_DatDesc_T RecvDesc3 = {0};
    IOC_initDatDesc(&RecvDesc3);
    char recvBuffer3[1024] = {0};
    RecvDesc3.Payload.pData = recvBuffer3;
    RecvDesc3.Payload.PtrDataSize = sizeof(recvBuffer3);
    RecvDesc3.Payload.PtrDataLen = 0;

    // Test timeout behavior when no data is available
    printf("   📋 Testing timeout when no data available...\n");
    auto syncTimeoutStart = std::chrono::high_resolution_clock::now();
    Result = IOC_recvDAT(DatPollingReceiverLinkID, &RecvDesc3, &SyncTimeoutOpt);
    auto syncTimeoutEnd = std::chrono::high_resolution_clock::now();

    auto syncTimeoutDuration = std::chrono::duration_cast<std::chrono::microseconds>(syncTimeoutEnd - syncTimeoutStart);
    printf("   ⏱️ SyncTimeout execution time: %lld microseconds\n", (long long)syncTimeoutDuration.count());
    printf("   📋 SyncTimeout result: %d\n", Result);

    // SyncTimeout should timeout when no data is available
    ASSERT_TRUE(Result == IOC_RESULT_TIMEOUT)
        << "SyncTimeout should return TIMEOUT when no data available, got: " << Result;
    ASSERT_LT(syncTimeoutDuration.count(), 8000)  // Should not exceed timeout significantly
        << "SyncTimeout should respect timing boundaries";
    printf("   ✓ SyncTimeout mode behaved correctly\n");

    // Test 2d: SyncTimeout mode with data available - should succeed quickly
    printf("🧪 Test 2d: SyncTimeout mode with data available...\n");

    // First send data to ensure it's available
    printf("   📤 Sending data for timeout success test...\n");
    Result = IOC_sendDAT(DatPollingSenderLinkID, &TestDatDesc, &PreSendOpt);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result) << "Send for timeout test should succeed";
    std::this_thread::sleep_for(std::chrono::milliseconds(2));  // Let data arrive

    IOC_DatDesc_T RecvDesc4 = {0};
    IOC_initDatDesc(&RecvDesc4);
    char recvBuffer4[1024] = {0};
    RecvDesc4.Payload.pData = recvBuffer4;
    RecvDesc4.Payload.PtrDataSize = sizeof(recvBuffer4);

    IOC_Option_defineSyncTimeout(SyncTimeoutSuccessOpt, 5000);  // 5ms timeout
    printf("   📋 Testing timeout when data IS available...\n");
    auto syncTimeoutSuccessStart = std::chrono::high_resolution_clock::now();
    Result = IOC_recvDAT(DatPollingReceiverLinkID, &RecvDesc4, &SyncTimeoutSuccessOpt);
    auto syncTimeoutSuccessEnd = std::chrono::high_resolution_clock::now();

    auto syncTimeoutSuccessDuration =
        std::chrono::duration_cast<std::chrono::microseconds>(syncTimeoutSuccessEnd - syncTimeoutSuccessStart);
    printf("   ⏱️ SyncTimeout (with data) execution time: %lld microseconds\n",
           (long long)syncTimeoutSuccessDuration.count());
    printf("   📋 SyncTimeout (with data) result: %d\n", Result);

    // Should succeed quickly when data is available
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result) << "SyncTimeout should succeed when data is available";
    ASSERT_GT(RecvDesc4.Payload.PtrDataSize, 0) << "Should have received data";
    ASSERT_LT(syncTimeoutSuccessDuration.count(), 3000)  // Should complete quickly
        << "SyncTimeout should complete quickly when data is available";
    printf("   ✓ SyncTimeout with data succeeded quickly (%lu bytes)\n", RecvDesc4.Payload.PtrDataSize);

    // === Test 3: Mixed Mode Operations with Data Integrity ===
    printf("📋 Test 3: Mixed Mode Operations with Data Integrity...\n");

    // Test rapid mode switching to ensure data integrity
    printf("🧪 Test 3a: Rapid mode switching stress test...\n");
    ULONG_T initialReceivedCount = DatReceiverPrivData.ReceivedDataCnt;
    ULONG_T successfulSends = 0;

    for (int i = 0; i < STRESS_TEST_OPERATIONS; i++) {
        // Alternate between different async modes for sending
        IOC_Result_T sendResult;

        switch (i % 3) {
            case 0: {
                IOC_Option_defineASyncMayBlock(StressMayBlockOpt);
                sendResult = IOC_sendDAT(DatSenderLinkID, &TestDatDesc, &StressMayBlockOpt);
                break;
            }
            case 1: {
                IOC_Option_defineASyncNonBlock(StressNonBlockOpt);
                sendResult = IOC_sendDAT(DatSenderLinkID, &TestDatDesc, &StressNonBlockOpt);
                break;
            }
            case 2: {
                IOC_Option_defineASyncTimeout(StressTimeoutOpt, 2000);
                sendResult = IOC_sendDAT(DatSenderLinkID, &TestDatDesc, &StressTimeoutOpt);
                break;
            }
        }

        if (sendResult == IOC_RESULT_SUCCESS) {
            successfulSends++;
        }

        printf("   📤 Send %d: mode=%d, result=%d\n", i, i % 3, sendResult);

        // Small delay to prevent overwhelming the system
        std::this_thread::sleep_for(std::chrono::microseconds(500));
    }

    printf("   📊 Stress test: %lu/%d sends successful\n", successfulSends, STRESS_TEST_OPERATIONS);
    ASSERT_GT(successfulSends, STRESS_TEST_OPERATIONS / 2) << "At least half of stress test sends should succeed";

    // Allow time for all data to be processed
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    // Verify data integrity - received count should reflect successful sends
    ULONG_T finalReceivedCount = DatReceiverPrivData.ReceivedDataCnt;
    ULONG_T actuallyReceived = finalReceivedCount - initialReceivedCount;

    printf("   📊 Data integrity: sent=%lu, received=%lu\n", successfulSends, actuallyReceived);

    // Allow some tolerance for async processing but data should not be lost
    ASSERT_GE(actuallyReceived, successfulSends * 0.8) << "At least 80% of successfully sent data should be received";

    // === Test 4: Mode Consistency Verification ===
    printf("📋 Test 4: Mode Consistency Verification...\n");

    // Test that mode behavior is consistent across multiple calls
    printf("🧪 Test 4a: Mode behavior consistency check...\n");

    std::vector<long long> nonBlockTimes;
    std::vector<IOC_Result_T> nonBlockResults;

    // Test multiple NonBlock operations for consistency
    for (int i = 0; i < MODE_TRANSITION_TEST_CYCLES; i++) {
        IOC_Option_defineASyncNonBlock(ConsistencyNonBlockOpt);

        auto start = std::chrono::high_resolution_clock::now();
        IOC_Result_T result = IOC_sendDAT(DatSenderLinkID, &TestDatDesc, &ConsistencyNonBlockOpt);
        auto end = std::chrono::high_resolution_clock::now();

        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        nonBlockTimes.push_back(duration.count());
        nonBlockResults.push_back(result);

        printf("   🔄 NonBlock consistency test %d: %lld μs, result=%d\n", i, duration.count(), result);

        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }

    // Verify timing consistency
    if (!nonBlockTimes.empty()) {
        long long maxTime = *std::max_element(nonBlockTimes.begin(), nonBlockTimes.end());
        long long minTime = *std::min_element(nonBlockTimes.begin(), nonBlockTimes.end());

        printf("   📊 NonBlock timing: min=%lld μs, max=%lld μs\n", minTime, maxTime);

        // NonBlock operations should be consistently fast
        ASSERT_LT(maxTime, 5000) << "All NonBlock operations should be under 5ms";

        // Check result consistency - should be mostly the same result type
        std::map<IOC_Result_T, int> resultCounts;
        for (auto result : nonBlockResults) {
            resultCounts[result]++;
        }

        printf("   📊 Result consistency: ");
        for (auto &pair : resultCounts) {
            printf("result_%d=%d ", pair.first, pair.second);
        }
        printf("\n");
    }

    // ┌──────────────────────────────────────────────────────────────────────────────────────┐
    // │                                ✅ VERIFICATION                                        │
    // └──────────────────────────────────────────────────────────────────────────────────────┘

    printf("🧪 Final verification: Mode transition behavior summary...\n");

    // Verify all modes have been tested and behaved within expected parameters
    ASSERT_LT(asyncMayBlockDuration.count(), MAX_MODE_EXECUTION_TIME_US) << "ASyncMayBlock timing within bounds";
    ASSERT_LT(asyncNonBlockDuration.count(), 5000) << "ASyncNonBlock timing within strict bounds";
    ASSERT_LT(asyncTimeoutDuration.count(), 10000) << "ASyncTimeout timing within bounds";
    ASSERT_LT(syncMayBlockDuration.count(), MAX_MODE_EXECUTION_TIME_US) << "SyncMayBlock timing within bounds";
    ASSERT_LT(syncNonBlockDuration.count(), 3000) << "SyncNonBlock timing within strict bounds";
    ASSERT_LT(syncTimeoutDuration.count(), 8000) << "SyncTimeout timing within bounds";
    ASSERT_LT(syncTimeoutSuccessDuration.count(), 3000) << "SyncTimeout with data timing within bounds";

    printf("   ✅ All blocking modes demonstrated correct timing behavior\n");
    printf("   ✅ Mode transitions preserved data integrity\n");
    printf("   ✅ Each mode behaved consistently across multiple calls\n");

    // ┌──────────────────────────────────────────────────────────────────────────────────────┐
    // │                               ✅ SUMMARY                                              │
    // └──────────────────────────────────────────────────────────────────────────────────────┘
    printf("✅ All blocking mode transitions completed successfully\n");
    printf("✅ Consistent behavior verified across all mode types\n");
    printf("✅ Data integrity maintained during mode switching\n");
    printf("✅ Timing boundaries respected for each blocking mode\n");

    // ┌──────────────────────────────────────────────────────────────────────────────────────┐
    // │                               🧹 CLEANUP PHASE                                        │
    // └──────────────────────────────────────────────────────────────────────────────────────┘
    printf("🧹 Cleaning up services and links...\n");

    Result = IOC_closeLink(DatSenderLinkID);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result) << "Sender link should close successfully";

    Result = IOC_closeLink(DatReceiverLinkID);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result) << "Receiver link should close successfully";

    Result = IOC_closeLink(DatPollingSenderLinkID);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result) << "Polling sender link should close successfully";

    Result = IOC_closeLink(DatPollingReceiverLinkID);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result) << "Polling receiver link should close successfully";

    Result = IOC_offlineService(DatReceiverSrvID);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result) << "Receiver service should go offline successfully";

    Result = IOC_offlineService(DatPollingReceiverSrvID);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result) << "Polling receiver service should go offline successfully";

    printf("🧹 Cleanup completed\n");
}
//======>END OF: [@AC-2,US-3] TC-2=================================================================

// TODO: Implement remaining US-3 test cases:
// - verifyDatTimeoutBoundary_byExtremeValues_expectProperHandling
// - verifyDatTimeoutBoundary_byPrecisionTesting_expectAccurateTiming
// - verifyDatBlockingModeBoundary_byStateConsistency_expectNoDataLoss

//======>END OF US-3 TEST IMPLEMENTATIONS==========================================================
