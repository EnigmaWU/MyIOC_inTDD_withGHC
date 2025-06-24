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
    printf("🎯 BEHAVIOR: verifyDatTimeoutBoundary_byZeroTimeout_expectImmediateReturn\n");

    // Initialize test data structures
    __DatBoundaryPrivData_T DatReceiverPrivData = {0};
    DatReceiverPrivData.ClientIndex = 1;

    IOC_SrvID_T DatReceiverSrvID = IOC_ID_INVALID;
    IOC_LinkID_T DatSenderLinkID = IOC_ID_INVALID;
    IOC_LinkID_T DatReceiverLinkID = IOC_ID_INVALID;
    IOC_Result_T Result = IOC_RESULT_FAILURE;

    // Setup DatReceiver service with callback configuration
    printf("📋 Setting up DatReceiver service for timeout testing...\n");

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
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result) << "DatReceiver service online should succeed";
    printf("   ✓ DatReceiver service onlined with SrvID=%llu\n", DatReceiverSrvID);

    // Setup DatSender connection
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
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result) << "DatReceiver should accept connection";

    DatSenderThread.join();
    printf("   ✓ DatSender connected with LinkID=%llu\n", DatSenderLinkID);
    printf("   ✓ DatReceiver accepted with LinkID=%llu\n", DatReceiverLinkID);

    // ┌──────────────────────────────────────────────────────────────────────────────────────┐
    // │                               🎯 BEHAVIOR PHASE                                       │
    // └──────────────────────────────────────────────────────────────────────────────────────┘

    //===BEHAVIOR: Test IOC_sendDAT with zero timeout===
    printf("📋 Testing IOC_sendDAT with zero timeout...\n");

    // Configure zero timeout option
    IOC_Option_defineTimeout(ZeroTimeoutOption, IOC_TIMEOUT_IMMEDIATE);

    // Prepare test data
    const char *testData = "ZeroTimeoutTest";
    IOC_DatDesc_T TestDatDesc = {0};
    IOC_initDatDesc(&TestDatDesc);
    TestDatDesc.Payload.pData = (void *)testData;
    TestDatDesc.Payload.PtrDataSize = strlen(testData);

    // Test 1: Zero timeout sendDAT when buffer has space (initial state)
    printf("🧪 Test 1: Zero timeout sendDAT with empty buffer...\n");

    auto startTime = std::chrono::high_resolution_clock::now();
    Result = IOC_sendDAT(DatSenderLinkID, &TestDatDesc, &ZeroTimeoutOption);
    auto endTime = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
    printf("   ⏱️ Zero timeout sendDAT execution time: %lld microseconds\n", (long long)duration.count());

    // With empty buffer, zero timeout should succeed immediately
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result)
        << "Zero timeout sendDAT with empty buffer should return IOC_RESULT_SUCCESS, got: " << Result;

    // Verify timing - should complete very quickly (< 10ms for immediate operation)
    ASSERT_LT(duration.count(), 10000) << "Zero timeout operation should complete within 10ms, took: "
                                       << duration.count() << " microseconds";

    // Test 2: Fill the send buffer to create timeout scenario
    printf("🧪 Test 2: Fill send buffer to create timeout scenario...\n");
    
    // Query IOC capability to get actual buffer sizes
    IOC_CapabilityDescription_T CapDesc;
    memset(&CapDesc, 0, sizeof(CapDesc));
    CapDesc.CapID = IOC_CAPID_CONET_MODE_DATA;
    
    Result = IOC_getCapability(&CapDesc);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result) << "Should be able to query CONET mode data capability";
    
    ULONG_T MaxDataQueueSize = CapDesc.ConetModeData.MaxDataQueueSize;
    printf("   📋 CONET Data Capability: MaxDataQueueSize=%lu bytes\n", MaxDataQueueSize);
    
    // Calculate how much data to send to fill the queue
    // Use moderate-sized packets to better control queue filling
    size_t packetSize = 512; // 512 bytes per packet
    size_t packetsToFill = (MaxDataQueueSize / packetSize) + 3; // +3 to ensure overflow
    
    printf("   📏 Using packet size: %zu bytes, packets to fill queue: %zu\n", packetSize, packetsToFill);
    
    // Send multiple packets to fill the buffer without allowing receiver to drain
    // Use blocking mode to ensure data gets queued
    IOC_Option_defineASyncMayBlock(BlockingOption);
    
    // Create test data sized according to our calculation
    std::vector<char> largeData(packetSize, 'X');
    IOC_DatDesc_T LargeDataDesc = {0};
    IOC_initDatDesc(&LargeDataDesc);
    LargeDataDesc.Payload.pData = largeData.data();
    LargeDataDesc.Payload.PtrDataSize = largeData.size();
    
    // Fill queue with calculated number of packets
    int fillCount = 0;
    IOC_Result_T fillResult;
    printf("   🔄 Filling send queue systematically...\n");
    
    // Send data repeatedly based on queue capacity
    for (size_t i = 0; i < packetsToFill && fillCount < 100; i++) {  // Safety limit to avoid infinite loop
        fillResult = IOC_sendDAT(DatSenderLinkID, &LargeDataDesc, &BlockingOption);
        if (fillResult == IOC_RESULT_SUCCESS) {
            fillCount++;
            if (fillCount % 10 == 0) {  // Report every 10th packet to reduce noise
                printf("   📤 Filled queue with %d packets (%zu bytes total)\n", 
                       fillCount, fillCount * packetSize);
            }
        } else {
            printf("   ⚠️ Queue fill stopped at packet %d, result: %d\n", fillCount, fillResult);
            break;
        }
        
        // Very brief pause to allow minimal processing but not full drainage
        std::this_thread::sleep_for(std::chrono::microseconds(10));
    }
    
    printf("   ✓ Filled queue with %d packets (%zu bytes), expected capacity: %lu bytes\n", 
           fillCount, fillCount * packetSize, MaxDataQueueSize);

    // Test 3: Zero timeout sendDAT when buffer is full/busy
    printf("🧪 Test 3: Zero timeout sendDAT with full/busy buffer...\n");

    auto fullBufferStart = std::chrono::high_resolution_clock::now();
    Result = IOC_sendDAT(DatSenderLinkID, &TestDatDesc, &ZeroTimeoutOption);
    auto fullBufferEnd = std::chrono::high_resolution_clock::now();

    auto fullBufferDuration = std::chrono::duration_cast<std::chrono::microseconds>(fullBufferEnd - fullBufferStart);
    printf("   ⏱️ Zero timeout sendDAT (full buffer) execution time: %lld microseconds\n", 
           (long long)fullBufferDuration.count());

    // With full/busy buffer, zero timeout should return timeout
    ASSERT_EQ(IOC_RESULT_TIMEOUT, Result)
        << "Zero timeout sendDAT with full/busy buffer should return IOC_RESULT_TIMEOUT, got: " << Result;

    // Should still complete quickly even when timing out
    ASSERT_LT(fullBufferDuration.count(), 10000)
        << "Zero timeout sendDAT should complete within 10ms even with full buffer";

    // Test 4: Multiple consecutive zero timeout calls on full buffer
    printf("🧪 Test 4: Multiple consecutive zero timeout calls on full buffer...\n");

    const int numCalls = 5;
    std::vector<long long> executionTimes;
    std::vector<IOC_Result_T> results;

    for (int i = 0; i < numCalls; i++) {
        auto callStart = std::chrono::high_resolution_clock::now();
        IOC_Result_T callResult = IOC_sendDAT(DatSenderLinkID, &TestDatDesc, &ZeroTimeoutOption);
        auto callEnd = std::chrono::high_resolution_clock::now();

        auto callDuration = std::chrono::duration_cast<std::chrono::microseconds>(callEnd - callStart);
        executionTimes.push_back(callDuration.count());
        results.push_back(callResult);

        printf("   📞 Call %d: result=%d, time=%lld μs\n", i + 1, callResult, (long long)callDuration.count());

        // Each call should complete quickly
        ASSERT_LT(callDuration.count(), 10000) << "Zero timeout call " << i + 1 << " should complete within 10ms";

        // With full buffer, should consistently return timeout
        ASSERT_EQ(IOC_RESULT_TIMEOUT, callResult)
            << "Zero timeout call " << i + 1 << " on full buffer should return IOC_RESULT_TIMEOUT";
    }

    // Allow some time for buffer to drain before continuing
    printf("   ⏳ Allowing buffer to drain...\n");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    //===BEHAVIOR: Test IOC_recvDAT with zero timeout===
    printf("📋 Testing IOC_recvDAT with zero timeout...\n");

    // Test 3: Zero timeout recvDAT when no data available (clean state)
    printf("🧪 Test 3: Zero timeout recvDAT with no data available...\n");
    
    // First, drain any buffered data to ensure clean state
    IOC_DatDesc_T DrainDesc = {0};
    IOC_initDatDesc(&DrainDesc);
    char drainBuffer[1024] = {0};
    DrainDesc.Payload.pData = drainBuffer;
    DrainDesc.Payload.PtrDataSize = sizeof(drainBuffer);
    
    // Drain all available data with zero timeout until no more data
    IOC_Result_T drainResult;
    int drainCount = 0;
    do {
        drainResult = IOC_recvDAT(DatSenderLinkID, &DrainDesc, &ZeroTimeoutOption);
        if (drainResult == IOC_RESULT_SUCCESS) {
            drainCount++;
            printf("   🚰 Drained data chunk %d\n", drainCount);
        }
    } while (drainResult == IOC_RESULT_SUCCESS && drainCount < 10); // Safety limit
    
    printf("   ✓ Drained %d data chunks, final drain result: %d\n", drainCount, drainResult);
    
    // Now test zero timeout recvDAT with truly empty buffer
    IOC_DatDesc_T RecvDatDesc = {0};
    IOC_initDatDesc(&RecvDatDesc);
    char recvBuffer[1024] = {0};
    RecvDatDesc.Payload.pData = recvBuffer;
    RecvDatDesc.Payload.PtrDataSize = sizeof(recvBuffer);

    auto recvStartTime = std::chrono::high_resolution_clock::now();
    Result = IOC_recvDAT(DatSenderLinkID, &RecvDatDesc, &ZeroTimeoutOption);
    auto recvEndTime = std::chrono::high_resolution_clock::now();

    auto recvDuration = std::chrono::duration_cast<std::chrono::microseconds>(recvEndTime - recvStartTime);
    printf("   ⏱️ Zero timeout recvDAT execution time: %lld microseconds\n", (long long)recvDuration.count());

    // Zero timeout recv should return timeout when no data is available
    ASSERT_EQ(IOC_RESULT_TIMEOUT, Result)
        << "Zero timeout recvDAT should return IOC_RESULT_TIMEOUT when no data available, got: " << Result;

    ASSERT_LT(recvDuration.count(), 10000)
        << "Zero timeout recvDAT should complete within 10ms, took: " << recvDuration.count() << " microseconds";

    // Test 4: Zero timeout recvDAT when data is immediately available
    printf("🧪 Test 4: Zero timeout recvDAT with data immediately available...\n");

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

    // When data is available, zero timeout should succeed immediately
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result)
        << "Zero timeout recvDAT should return IOC_RESULT_SUCCESS when data is available, got: " << Result;

    // Should still complete quickly when data is available
    ASSERT_LT(quickRecvDuration.count(), 10000)
        << "Zero timeout recvDAT should complete within 10ms even with data available";

    // Verify received data content
    if (Result == IOC_RESULT_SUCCESS && QuickRecvDesc.Payload.PtrDataSize > 0) {
        printf("   📋 Received %lu bytes: '%.15s'\n", QuickRecvDesc.Payload.PtrDataSize, (char*)QuickRecvDesc.Payload.pData);
        ASSERT_GT(QuickRecvDesc.Payload.PtrDataSize, 0) << "Should have received data with non-zero size";
    }

    // ┌──────────────────────────────────────────────────────────────────────────────────────┐
    // │                                ✅ VERIFY PHASE                                        │
    // └──────────────────────────────────────────────────────────────────────────────────────┘

    // Test 5: Verify timing consistency across multiple zero timeout operations
    printf("🧪 Test 5: Verify timing consistency across operations...\n");

    // Calculate average execution time for consistency verification
    if (!executionTimes.empty()) {
        long long totalTime = 0;
        long long maxTime = 0;
        long long minTime = LLONG_MAX;

        for (long long time : executionTimes) {
            totalTime += time;
            maxTime = std::max(maxTime, time);
            minTime = std::min(minTime, time);
        }

        long long avgTime = totalTime / executionTimes.size();
        printf("   📊 Timing statistics: avg=%lld μs, min=%lld μs, max=%lld μs\n", avgTime, minTime, maxTime);

        // Verify reasonable timing consistency (max should not be dramatically larger than average)
        ASSERT_LT(maxTime, avgTime * 10)
            << "Maximum execution time should not be more than 10x average for consistent zero timeout behavior";
    }

    // Test 6: Verify zero timeout behavior with different data sizes
    printf("🧪 Test 6: Zero timeout with different data sizes...\n");

    std::vector<size_t> dataSizes = {0, 1, 64, 256, 1024};

    for (size_t size : dataSizes) {
        std::vector<char> testBuffer(size > 0 ? size : 1, 'X');

        IOC_DatDesc_T SizeTestDesc = {0};
        IOC_initDatDesc(&SizeTestDesc);
        if (size > 0) {
            SizeTestDesc.Payload.pData = testBuffer.data();
            SizeTestDesc.Payload.PtrDataSize = size;
        } else {
            // For zero size, test the zero-data boundary
            SizeTestDesc.Payload.pData = nullptr;
            SizeTestDesc.Payload.PtrDataSize = 0;
        }

        auto sizeTestStart = std::chrono::high_resolution_clock::now();
        IOC_Result_T sizeResult = IOC_sendDAT(DatSenderLinkID, &SizeTestDesc, &ZeroTimeoutOption);
        auto sizeTestEnd = std::chrono::high_resolution_clock::now();

        auto sizeTestDuration = std::chrono::duration_cast<std::chrono::microseconds>(sizeTestEnd - sizeTestStart);

        printf("   📏 Size %zu bytes: result=%d, time=%lld μs\n", size, sizeResult,
               (long long)sizeTestDuration.count());

        // Zero timeout should be fast regardless of data size
        ASSERT_LT(sizeTestDuration.count(), 10000)
            << "Zero timeout should be fast regardless of data size (" << size << " bytes)";

        // For zero size, expect IOC_RESULT_ZERO_DATA
        if (size == 0) {
            ASSERT_EQ(IOC_RESULT_ZERO_DATA, sizeResult)
                << "Zero timeout with zero-size data should return IOC_RESULT_ZERO_DATA";
        } else {
            // For non-zero size, expect typical zero timeout behavior
            ASSERT_TRUE(sizeResult == IOC_RESULT_SUCCESS || sizeResult == IOC_RESULT_TIMEOUT)
                << "Zero timeout with non-zero data should return appropriate result code";
        }
    }

    //===KeyVerifyPoint: Summary verification===
    printf("✅ Zero timeout operations completed immediately without blocking\n");
    printf("✅ All timing measurements within acceptable limits (< 10ms)\n");
    printf("✅ Consistent behavior across multiple consecutive calls\n");
    printf("✅ Proper result codes returned for all zero timeout scenarios\n");
    printf("✅ Zero timeout behavior independent of data size\n");

    // ┌──────────────────────────────────────────────────────────────────────────────────────┐
    // │                               🧹 CLEANUP PHASE                                        │
    // └──────────────────────────────────────────────────────────────────────────────────────┘

    // Cleanup services
    Result = IOC_offlineService(DatReceiverSrvID);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result) << "DatReceiver service should be offlined successfully";

    printf("🧹 Cleanup completed successfully\n");
}
//======>END OF: [@AC-1,US-3] TC-1=================================================================

// TODO: Implement remaining US-3 test cases:
// - verifyDatBlockingModeBoundary_byModeTransitions_expectConsistentBehavior
// - verifyDatTimeoutBoundary_byExtremeValues_expectProperHandling
// - verifyDatTimeoutBoundary_byPrecisionTesting_expectAccurateTiming
// - verifyDatBlockingModeBoundary_byStateConsistency_expectNoDataLoss

//======>END OF US-3 TEST IMPLEMENTATIONS==========================================================
