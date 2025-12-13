///////////////////////////////////////////////////////////////////////////////////////////////////
// UT_DataEdgeUS5.cxx - DAT Edge Testing: US-5 Stream Granularity Edge Validation
// 📝 Purpose: Test Cases for User Story 5 - Stream processing developer granularity boundary testing
// 🔄 Focus: DAT stream behavior with different send/receive granularities (byte-by-byte vs block-by-block)
// 🎯 Coverage: [@US-5] Stream granularity boundary validation (AC-1, AC-2, AC-3)
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "UT_DataEdge.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF US-5 TEST CASES==================================================================
/**************************************************************************************************
 * @brief 【US-5 Test Cases】- Stream Granularity Edge Validation
 *
 * [@AC-1,US-5] Stream granularity validation - Byte-by-byte send, block receive
 *  TC-1:
 *      @[Name]: verifyDatStreamGranularity_byByteToBlockPattern_expectDataIntegrity
 *      @[Purpose]: Verify DAT stream handles byte-by-byte sending with block-by-block receiving
 *      @[Brief]: Send data 1 byte at a time, receive in larger blocks, verify data reconstruction
 *      @[Coverage]: 1-byte sends, multi-byte receives, stream ordering, data integrity
 *
 *  TC-2:
 *      @[Name]: verifyDatStreamGranularity_byBurstThenPausePattern_expectBatchingBehavior
 *      @[Purpose]: TDD test for batching behavior - send 1024 bytes byte-by-byte, expect batched delivery
 *      @[Brief]: Send 1024 bytes continuously byte-by-byte, then pause 10ms, expect fewer larger callbacks
 *      @[Coverage]: TDD expectation, burst sending, timing-based batching, internal buffering requirement
 *
 *-------------------------------------------------------------------------------------------------
 * [@AC-2,US-5] Stream granularity validation - Block send, byte-by-byte receive
 *  TC-1:
 *      @[Name]: verifyDatStreamGranularity_byBlockToBytePattern_expectFragmentationSupport
 *      @[Purpose]: Verify DAT stream handles block sending with byte-by-byte receiving
 *      @[Brief]: Send large blocks, attempt to receive in small fragments, verify partial reception
 *      @[Coverage]: Large block sends, small fragment receives, partial data handling
 *
 *  TODO: TC-2: ...
 *
 *-------------------------------------------------------------------------------------------------
 * [@AC-3,US-5] Stream granularity validation - Variable patterns
 *  TC-1:
 *      @[Name]: verifyDatStreamGranularity_byVariablePatterns_expectConsistentBehavior
 *      @[Purpose]: Verify DAT stream handles mixed granularity patterns consistently
 *      @[Brief]: Alternate between different send/receive sizes, verify stream consistency
 *      @[Coverage]: Mixed patterns, rapid switching, buffer management, end-to-end integrity
 *
 *  TODO: TC-2: ...
 *
 *************************************************************************************************/
//======>END OF US-5 TEST CASES====================================================================

#include "UT_DataEdge.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF US-5 TEST IMPLEMENTATIONS========================================================

//======>BEGIN OF: [@AC-1,US-5] TC-1===============================================================
/**
 * @[Name]: verifyDatStreamGranularity_byByteToBlockPattern_expectDataIntegrity
 * @[Steps]:
 *   1) Setup DatSender and DatReceiver connections AS SETUP.
 *   2) Send test data byte-by-byte using multiple IOC_sendDAT(1-byte) calls AS BEHAVIOR.
 *   3) Receive data in larger blocks using IOC_recvDAT or callback AS BEHAVIOR.
 *   4) Verify complete data reconstruction and integrity AS VERIFY.
 *   5) Cleanup connections AS CLEANUP.
 * @[Expect]: Byte-by-byte transmission successfully reconstructed into blocks with data integrity preserved.
 * @[Notes]: Tests fundamental DAT STREAM behavior - granularity independence.
 */
TEST(UT_DataEdge, verifyDatStreamGranularity_byByteToBlockPattern_expectDataIntegrity) {
    printf("\n📋 [@AC-1,US-5] TC-1: DAT Stream Granularity - Byte-to-Block Pattern\n");

    //===SETUP===
    IOC_Result_T Result = IOC_RESULT_FAILURE;
    IOC_SrvID_T DatReceiverSrvID = IOC_ID_INVALID;
    IOC_LinkID_T DatSenderLinkID = IOC_ID_INVALID;
    IOC_LinkID_T DatReceiverLinkID = IOC_ID_INVALID;

    __DatEdgePrivData_T DatReceiverPrivData = {0};
    DatReceiverPrivData.ClientIndex = 1;
    DatReceiverPrivData.ReceivedContentWritePos = 0;

    printf("📋 Setting up DAT stream granularity testing environment...\n");

    // Setup DatReceiver service
    IOC_SrvURI_T DatReceiverSrvURI = {
        .pProtocol = IOC_SRV_PROTO_FIFO,
        .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
        .pPath = "DatGranularityReceiver",
    };

    IOC_DatUsageArgs_T DatReceiverUsageArgs = {
        .CbRecvDat_F = __CbRecvDat_Edge_F,
        .pCbPrivData = &DatReceiverPrivData,
    };

    IOC_SrvArgs_T DatReceiverSrvArgs = {
        .SrvURI = DatReceiverSrvURI,
        .UsageCapabilites = IOC_LinkUsageDatReceiver,
        .UsageArgs = {.pDat = &DatReceiverUsageArgs},
    };

    Result = IOC_onlineService(&DatReceiverSrvID, &DatReceiverSrvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result) << "DatReceiver service should come online successfully";

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
    printf("   ✓ Stream granularity test connections established\n");

    //===BEHAVIOR===
    printf("📋 Testing byte-by-byte send with block-by-block receive pattern...\n");

    // Test data: 100 bytes with recognizable pattern
    const int TestDataSize = 100;
    char TestData[TestDataSize];
    for (int i = 0; i < TestDataSize; i++) {
        TestData[i] = (char)('A' + (i % 26));  // A-Z repeating pattern
    }

    printf("🧪 Sending %d bytes one-by-one...\n", TestDataSize);

    // Send data byte-by-byte
    for (int i = 0; i < TestDataSize; i++) {
        IOC_DatDesc_T ByteDesc = {0};
        IOC_initDatDesc(&ByteDesc);
        ByteDesc.Payload.pData = &TestData[i];
        ByteDesc.Payload.PtrDataSize = 1;  // Single byte
        ByteDesc.Payload.PtrDataLen = 1;

        Result = IOC_sendDAT(DatSenderLinkID, &ByteDesc, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, Result) << "Byte " << i << " should send successfully";

        // Small delay to ensure stream behavior (not batch)
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }

    // Force transmission and allow time for callback processing
    IOC_flushDAT(DatSenderLinkID, NULL);
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    //===VERIFY===
    printf("📋 Verifying stream reconstruction from byte-by-byte to block reception...\n");

    // KeyVerifyPoint-1: All data should be received via callback
    ASSERT_TRUE(DatReceiverPrivData.CallbackExecuted)
        << "Callback should execute when byte-by-byte data is transmitted";

    // KeyVerifyPoint-2: Total received size should match sent size
    ASSERT_EQ(TestDataSize, DatReceiverPrivData.TotalReceivedSize)
        << "Total received size should equal sent size. Expected: " << TestDataSize
        << ", Actual: " << DatReceiverPrivData.TotalReceivedSize;

    // KeyVerifyPoint-3: Data integrity should be preserved
    ASSERT_EQ(0, memcmp(TestData, DatReceiverPrivData.ReceivedContent, TestDataSize))
        << "Reconstructed data should match original byte sequence";

    // KeyVerifyPoint-4: Multiple callback invocations expected (block reception of byte sends)
    // Note: IOC may buffer multiple bytes before callback, so we expect <= TestDataSize callbacks
    ASSERT_LE(DatReceiverPrivData.ReceivedDataCnt, TestDataSize)
        << "Callback count should not exceed number of bytes sent";
    ASSERT_GE(DatReceiverPrivData.ReceivedDataCnt, 1) << "At least one callback should occur";

    printf("   ✅ Stream granularity test completed successfully!\n");
    printf("   📊 Sent: %d bytes (1-byte chunks), Received: %lu bytes in %lu callbacks\n", TestDataSize,
           DatReceiverPrivData.TotalReceivedSize, DatReceiverPrivData.ReceivedDataCnt);

    //===CLEANUP===
    if (DatReceiverLinkID != IOC_ID_INVALID) {
        IOC_closeLink(DatReceiverLinkID);
    }
    if (DatSenderLinkID != IOC_ID_INVALID) {
        IOC_closeLink(DatSenderLinkID);
    }
    if (DatReceiverSrvID != IOC_ID_INVALID) {
        IOC_offlineService(DatReceiverSrvID);
    }
}
//======>END OF: [@AC-1,US-5] TC-1=================================================================

//======>BEGIN OF: [@AC-1,US-5] TC-2===============================================================
/**
 * @[Name]: verifyDatStreamGranularity_byBurstThenPausePattern_expectBatchingBehavior
 * @[Steps]:
 *   1) Setup DatSender and DatReceiver connections with slow callback AS SETUP.
 *   2) Send 1024 bytes continuously byte-by-byte (no delays between sends) AS BEHAVIOR.
 *   3) First callback pauses 10ms (simulating slow receiver) AS BEHAVIOR.
 *   4) Verify that subsequent sends are batched while callback is paused AS VERIFY.
 *   5) Cleanup connections AS CLEANUP.
 * @[Expect]: TDD expectation - rapid sends should accumulate and be batched while receiver is busy.
 * @[Notes]: Tests the specific question: "May I receive 1024 bytes once each 10ms?" - slow receiver batching pattern.
 */
TEST(UT_DataEdge, verifyDatStreamGranularity_byBurstThenPausePattern_expectBatchingBehavior) {
    printf("\n📋 [@AC-1,US-5] TC-2: DAT Stream Granularity - Burst-Then-Pause Pattern\n");

    //===SETUP===
    IOC_Result_T Result = IOC_RESULT_FAILURE;
    IOC_SrvID_T DatReceiverSrvID = IOC_ID_INVALID;
    IOC_LinkID_T DatSenderLinkID = IOC_ID_INVALID;
    IOC_LinkID_T DatReceiverLinkID = IOC_ID_INVALID;

    __DatEdgePrivData_T DatReceiverPrivData = {0};
    DatReceiverPrivData.ClientIndex = 2;
    DatReceiverPrivData.ReceivedContentWritePos = 0;
    DatReceiverPrivData.FirstCallbackRecorded = false;
    DatReceiverPrivData.LargestSingleCallback = 0;

    // Configure slow receiver mode for batching test
    DatReceiverPrivData.SlowReceiverMode = true;
    DatReceiverPrivData.SlowReceiverPauseMs = 10;  // 10ms pause on first callback
    DatReceiverPrivData.FirstCallbackPaused = false;

    printf("📋 Setting up DAT slow receiver batching behavior testing...\n");

    // Setup DatReceiver service with slow receiver callback
    IOC_SrvURI_T DatReceiverSrvURI = {
        .pProtocol = IOC_SRV_PROTO_FIFO,
        .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
        .pPath = "DatSlowReceiverBatching",
    };

    IOC_DatUsageArgs_T DatReceiverUsageArgs = {
        .CbRecvDat_F = __CbRecvDat_SlowReceiver_F,  // Use slow receiver callback
        .pCbPrivData = &DatReceiverPrivData,
    };

    IOC_SrvArgs_T DatReceiverSrvArgs = {
        .SrvURI = DatReceiverSrvURI,
        .UsageCapabilites = IOC_LinkUsageDatReceiver,
        .UsageArgs = {.pDat = &DatReceiverUsageArgs},
    };

    Result = IOC_onlineService(&DatReceiverSrvID, &DatReceiverSrvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result) << "DatReceiver service should come online successfully";

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
    printf("   ✓ Slow receiver batching test connections established\n");

    //===BEHAVIOR===
    printf("📋 Testing slow receiver batching behavior...\n");

    // Test data: 1024 bytes with recognizable pattern
    const int BurstSize = 1024;
    char BurstData[BurstSize];
    for (int i = 0; i < BurstSize; i++) {
        BurstData[i] = (char)('0' + (i % 10));  // 0-9 repeating pattern
    }

    printf("🧪 Sending %d bytes rapidly while receiver callback is slow...\n", BurstSize);
    printf("   Expected: First callback pauses 10ms, subsequent sends should batch\n");

    // Record start time for burst sending
    auto burstStartTime = std::chrono::high_resolution_clock::now();

    // Send data byte-by-byte continuously (burst pattern)
    // The first callback will pause for 10ms, during which subsequent sends should accumulate
    for (int i = 0; i < BurstSize; i++) {
        IOC_DatDesc_T ByteDesc = {0};
        IOC_initDatDesc(&ByteDesc);
        ByteDesc.Payload.pData = &BurstData[i];
        ByteDesc.Payload.PtrDataSize = 1;  // Single byte
        ByteDesc.Payload.PtrDataLen = 1;

        Result = IOC_sendDAT(DatSenderLinkID, &ByteDesc, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, Result) << "Burst byte " << i << " should send successfully";

        // NO DELAY - rapid sending to test batching during slow callback
    }

    auto burstEndTime = std::chrono::high_resolution_clock::now();
    auto burstDuration = std::chrono::duration_cast<std::chrono::microseconds>(burstEndTime - burstStartTime);

    printf("   Burst sending completed in %lld microseconds\n", burstDuration.count());

    // Force transmission to ensure all data is delivered
    IOC_flushDAT(DatSenderLinkID, NULL);

    // Allow time for all callbacks to complete (including the slow first one)
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    //===VERIFY===
    printf("📋 Verifying slow receiver batching behavior...\n");

    // KeyVerifyPoint-1: All data should be received
    ASSERT_TRUE(DatReceiverPrivData.CallbackExecuted) << "Callback should execute when burst data is transmitted";

    // KeyVerifyPoint-2: Total received size should match sent size
    ASSERT_EQ(BurstSize, DatReceiverPrivData.TotalReceivedSize)
        << "Total received size should equal burst size. Expected: " << BurstSize
        << ", Actual: " << DatReceiverPrivData.TotalReceivedSize;

    // KeyVerifyPoint-3: Data integrity should be preserved
    ASSERT_EQ(0, memcmp(BurstData, DatReceiverPrivData.ReceivedContent, BurstSize))
        << "Reconstructed burst data should match original sequence";

    // KeyVerifyPoint-4: Analyze batching behavior
    printf("   📊 Slow Receiver Batching Analysis:\n");
    printf("      - Total callbacks: %lu\n", DatReceiverPrivData.ReceivedDataCnt);
    printf("      - Largest single callback: %lu bytes\n", DatReceiverPrivData.LargestSingleCallback);
    printf("      - Average callback size: %.2f bytes\n",
           (double)DatReceiverPrivData.TotalReceivedSize / DatReceiverPrivData.ReceivedDataCnt);
    printf("      - First callback paused: %s\n", DatReceiverPrivData.FirstCallbackPaused ? "Yes" : "No");

    // Print individual callback sizes for analysis
    printf("      - Callback sizes: ");
    for (size_t i = 0; i < DatReceiverPrivData.CallbackSizes.size() && i < 10; i++) {
        printf("%lu ", DatReceiverPrivData.CallbackSizes[i]);
        if (i == 9 && DatReceiverPrivData.CallbackSizes.size() > 10) {
            printf("...(+%lu more) ", DatReceiverPrivData.CallbackSizes.size() - 10);
        }
    }
    printf("\n");

    // KeyVerifyPoint-5: TDD Expectation - Slow Receiver Batching Analysis
    // Your original question: "May I receive 1024 bytes once each 10ms?"
    // TDD Expectation: While first callback is paused, subsequent sends should accumulate and be batched

    printf("   🎯 TESTING TDD EXPECTATION: 'May I receive 1024 bytes once each 10ms?'\n");
    printf("      - Expected: YES - IOC should batch rapid sends while receiver is busy\n");
    printf("      - Slow receiver simulation: First callback paused for %d ms\n",
           DatReceiverPrivData.SlowReceiverPauseMs);
    printf("      - Total callbacks: %lu\n", DatReceiverPrivData.ReceivedDataCnt);
    printf("      - Largest single callback: %lu bytes\n", DatReceiverPrivData.LargestSingleCallback);

    // TDD Analysis: Slow receiver batching behavior
    bool batchingBehavior = false;

    // Check if batching occurred: fewer callbacks than sends and larger callback sizes
    if (DatReceiverPrivData.ReceivedDataCnt < BurstSize && DatReceiverPrivData.LargestSingleCallback > 1) {
        batchingBehavior = true;
        printf("      - ✅ SLOW RECEIVER BATCHING: Sends accumulated while callback was paused\n");
        printf("      - 📈 Batching efficiency: %.1f%% reduction in callbacks\n",
               (1.0 - (double)DatReceiverPrivData.ReceivedDataCnt / BurstSize) * 100.0);
    }
    // Check for significant batch sizes even if callback count is high
    else if (DatReceiverPrivData.LargestSingleCallback > 100) {
        batchingBehavior = true;
        printf("      - ✅ PARTIAL BATCHING: Some sends were batched into larger chunks\n");
    }
    // No batching - immediate individual delivery even during slow callback
    else {
        printf("      - ❌ NO BATCHING: Each send triggers separate callback, even during slow processing\n");
        printf("      - 💡 Framework Reality: IOC delivers each send individually, no queuing\n");
        printf("      - 🔧 Design Decision Needed: Accept no-batching or implement send queuing\n");
    }

    // TDD Assertions: We EXPECT batching behavior when receiver is slow
    EXPECT_TRUE(batchingBehavior) << "TDD EXPECTATION: Should demonstrate batching when receiver is slow. "
                                  << "Total callbacks: " << DatReceiverPrivData.ReceivedDataCnt
                                  << ", Max callback size: " << DatReceiverPrivData.LargestSingleCallback;

    // Additional expectation: Significant batching should occur during slow callback
    EXPECT_LT(DatReceiverPrivData.ReceivedDataCnt, BurstSize)
        << "TDD EXPECTATION: Should receive fewer callbacks than bytes sent when receiver is slow. "
        << "Expected: < " << BurstSize << " callbacks, Actual: " << DatReceiverPrivData.ReceivedDataCnt;

    // Additional expectation: Large callback sizes should result from batching
    EXPECT_GT(DatReceiverPrivData.LargestSingleCallback, 10)
        << "TDD EXPECTATION: Should receive batched data during slow callback. "
        << "Expected: > 10 bytes per largest callback, Actual max: " << DatReceiverPrivData.LargestSingleCallback;

    // KeyVerifyPoint-6: Timing analysis
    if (DatReceiverPrivData.FirstCallbackRecorded && DatReceiverPrivData.ReceivedDataCnt > 1) {
        auto totalCallbackDuration = std::chrono::duration_cast<std::chrono::milliseconds>(
            DatReceiverPrivData.LastCallbackTime - DatReceiverPrivData.FirstCallbackTime);
        printf("   ⏱️  Callback timing: First to last span = %lld ms\n", totalCallbackDuration.count());
    }

    printf("   ✅ Slow receiver batching test completed successfully!\n");
    printf("   📊 Result: Sent %d bytes (burst), Received %lu bytes in %lu callbacks\n", BurstSize,
           DatReceiverPrivData.TotalReceivedSize, DatReceiverPrivData.ReceivedDataCnt);

    //===CLEANUP===
    if (DatReceiverLinkID != IOC_ID_INVALID) {
        IOC_closeLink(DatReceiverLinkID);
    }
    if (DatSenderLinkID != IOC_ID_INVALID) {
        IOC_closeLink(DatSenderLinkID);
    }
    if (DatReceiverSrvID != IOC_ID_INVALID) {
        IOC_offlineService(DatReceiverSrvID);
    }
}
//======>END OF: [@AC-1,US-5] TC-2=================================================================

//======>BEGIN OF: [@AC-1,US-5] TC-2B==============================================================
/**
 * @[Name]: verifyDatStreamGranularity_bySlowSendSlowReceive_expectInterleavedBatching
 * @[Steps]:
 *   1) Setup DatSender and DatReceiver with slow receiver (10ms every callback) AS SETUP.
 *   2) Send 3 bursts (128, 256, 512 bytes) with 10ms delay between each send AS BEHAVIOR.
 *   3) Analyze the actual batching pattern that emerges AS VERIFY.
 * @[Expect]: TDD RED TEST - IOC should provide timing-based batching for slow send + slow receive scenarios.
 * @[Notes]: TDD REQUIREMENT - IOC should be smart enough to batch sends during timing overlap windows.
 *           This test will FAIL until IOC implements timing-aware batching capability.
 *           RED → GREEN → REFACTOR: Test drives implementation of enhanced IOC batching.
 */
TEST(UT_DataEdge, verifyDatStreamGranularity_bySlowSendSlowReceive_expectInterleavedBatching) {
    printf("\n📋 [@AC-1,US-5] TC-2B: DAT Stream Granularity - Slow Send + Slow Receive Pattern\n");

    //===SETUP===
    IOC_Result_T Result = IOC_RESULT_FAILURE;
    IOC_SrvID_T DatReceiverSrvID = IOC_ID_INVALID;
    IOC_LinkID_T DatSenderLinkID = IOC_ID_INVALID;
    IOC_LinkID_T DatReceiverLinkID = IOC_ID_INVALID;

    __DatEdgePrivData_T DatReceiverPrivData = {0};
    DatReceiverPrivData.ClientIndex = 4;  // New client ID
    DatReceiverPrivData.ReceivedContentWritePos = 0;
    DatReceiverPrivData.FirstCallbackRecorded = false;
    DatReceiverPrivData.LargestSingleCallback = 0;

    // Configure slow receiver mode - EVERY callback is slow (not just first)
    DatReceiverPrivData.SlowReceiverMode = true;
    DatReceiverPrivData.SlowReceiverPauseMs = 10;  // 10ms pause on EVERY callback
    DatReceiverPrivData.FirstCallbackPaused = false;
    DatReceiverPrivData.AlwaysSlowMode = true;  // New flag for always slow

    printf("📋 Setting up DAT slow-send + slow-receive timing analysis...\n");
    printf("   Configuration: Every callback has 10ms delay, sends are RAPID (no delay)\n");
    printf("   TDD RED TEST: Expecting IOC to implement timing-based batching\n");
    printf("   REQUIREMENT: IOC should batch rapid sends during slow 10ms callback windows\n");

    // Setup DatReceiver service with always-slow receiver callback
    IOC_SrvURI_T DatReceiverSrvURI = {
        .pProtocol = IOC_SRV_PROTO_FIFO,
        .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
        .pPath = "DatSlowSendSlowReceive",
    };

    IOC_DatUsageArgs_T DatReceiverUsageArgs = {
        .CbRecvDat_F = __CbRecvDat_SlowReceiver_F,  // Use slow receiver callback
        .pCbPrivData = &DatReceiverPrivData,
    };

    IOC_SrvArgs_T DatReceiverSrvArgs = {
        .SrvURI = DatReceiverSrvURI,
        .UsageCapabilites = IOC_LinkUsageDatReceiver,
        .UsageArgs = {.pDat = &DatReceiverUsageArgs},
    };

    Result = IOC_onlineService(&DatReceiverSrvID, &DatReceiverSrvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result) << "DatReceiver service should come online successfully";

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
    printf("   ✓ Slow-send + slow-receive test connections established\n");

    //===BEHAVIOR===
    printf("📋 Testing interleaved batching with slow sends and slow receives...\n");

    // Test pattern: 3 bursts of different sizes with 10ms delay between each send
    struct {
        int size;
        char fillChar;
        const char* description;
    } testBursts[] = {{128, 'A', "128-byte burst (A pattern)"},
                      {256, 'B', "256-byte burst (B pattern)"},
                      {512, 'C', "512-byte burst (C pattern)"}};

    auto testStartTime = std::chrono::high_resolution_clock::now();

    for (int burstIdx = 0; burstIdx < 3; burstIdx++) {
        printf("🧪 Sending burst %d: %s...\n", burstIdx + 1, testBursts[burstIdx].description);

        auto burstStartTime = std::chrono::high_resolution_clock::now();

        // Send each burst byte-by-byte RAPIDLY (no delay) to create batching opportunity
        for (int i = 0; i < testBursts[burstIdx].size; i++) {
            IOC_DatDesc_T ByteDesc = {0};
            IOC_initDatDesc(&ByteDesc);

            char dataByte = testBursts[burstIdx].fillChar;
            ByteDesc.Payload.pData = &dataByte;
            ByteDesc.Payload.PtrDataSize = 1;
            ByteDesc.Payload.PtrDataLen = 1;

            Result = IOC_sendDAT(DatSenderLinkID, &ByteDesc, NULL);
            ASSERT_EQ(IOC_RESULT_SUCCESS, Result)
                << "Burst " << burstIdx << " byte " << i << " should send successfully";

            // NO DELAY - rapid sending creates overlap with slow 10ms callbacks = batching!
        }

        // sleep-10ms to simulate slow callback processing
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        auto burstEndTime = std::chrono::high_resolution_clock::now();
        auto burstDuration = std::chrono::duration_cast<std::chrono::milliseconds>(burstEndTime - burstStartTime);
        printf("   Burst %d completed in %lld ms\n", burstIdx + 1, burstDuration.count());
    }

    // Force final flush and allow processing to complete
    IOC_flushDAT(DatSenderLinkID, NULL);
    std::this_thread::sleep_for(std::chrono::milliseconds(300));  // Extra time for slow callbacks

    auto testEndTime = std::chrono::high_resolution_clock::now();
    auto totalDuration = std::chrono::duration_cast<std::chrono::milliseconds>(testEndTime - testStartTime);

    //===VERIFY===
    printf("📋 Analyzing interleaved batching pattern...\n");

    int totalSentBytes = 128 + 256 + 512;  // 896 bytes total

    // KeyVerifyPoint-1: All data should be received
    ASSERT_TRUE(DatReceiverPrivData.CallbackExecuted) << "Callbacks should execute";
    ASSERT_EQ(totalSentBytes, DatReceiverPrivData.TotalReceivedSize)
        << "Total received size should equal total sent. Expected: " << totalSentBytes
        << ", Actual: " << DatReceiverPrivData.TotalReceivedSize;

    // KeyVerifyPoint-2: Timing analysis - understand the batching pattern
    printf("   📊 Timing Analysis Results:\n");
    printf("      - Total test duration: %lld ms\n", totalDuration.count());
    printf("      - Total callbacks: %lu\n", DatReceiverPrivData.ReceivedDataCnt);
    printf("      - Total bytes sent: %d (in %d individual sends)\n", totalSentBytes, totalSentBytes);
    printf("      - Average callback size: %.2f bytes\n",
           (double)DatReceiverPrivData.TotalReceivedSize / DatReceiverPrivData.ReceivedDataCnt);
    printf("      - Largest single callback: %lu bytes\n", DatReceiverPrivData.LargestSingleCallback);

    // Print detailed callback pattern
    printf("      - Callback size pattern: ");
    for (size_t i = 0; i < DatReceiverPrivData.CallbackSizes.size() && i < 20; i++) {
        printf("%lu ", DatReceiverPrivData.CallbackSizes[i]);
        if (i == 19 && DatReceiverPrivData.CallbackSizes.size() > 20) {
            printf("...(+%lu more) ", DatReceiverPrivData.CallbackSizes.size() - 20);
        }
    }
    printf("\n");

    // KeyVerifyPoint-3: TDD RED TEST - Expect timing-based batching capability
    printf("   🔴 TDD RED TEST - TIMING-BASED BATCHING REQUIREMENT:\n");
    printf("      - Send pattern: RAPID bursts (no delay between bytes)\n");
    printf("      - Callback duration: 10ms each (creates overlap opportunity)\n");
    printf("      - TDD REQUIREMENT: IOC should batch rapid sends during slow callback\n");
    printf("      - Expected result: Much fewer callbacks than sends (128→1, 256→1, 512→1)\n");

    // TDD RED TEST: IOC should provide timing-aware batching
    // This test will FAIL until you implement timing-based batching in IOC

    EXPECT_LT(DatReceiverPrivData.ReceivedDataCnt, totalSentBytes)
        << "TDD RED TEST REQUIREMENT: IOC should provide timing-based batching. "
        << "Expected: Fewer than " << totalSentBytes << " callbacks due to timing overlap, "
        << "Actual: " << DatReceiverPrivData.ReceivedDataCnt << " callbacks. "
        << "This test will FAIL until IOC implements smart timing-aware batching capability.";

    // TDD RED TEST: Should see larger batch sizes due to timing windows
    EXPECT_GT(DatReceiverPrivData.LargestSingleCallback, 1)
        << "TDD RED TEST REQUIREMENT: Should see batching during timing overlap windows. "
        << "Expected: > 1 byte per largest callback due to timing-based accumulation, "
        << "Actual: " << DatReceiverPrivData.LargestSingleCallback << " bytes. "
        << "This indicates IOC needs enhanced timing-aware batching logic.";

    EXPECT_GT(DatReceiverPrivData.ReceivedDataCnt, 3)
        << "Should have more callbacks than perfect burst batching (3), but fewer than individual sends ("
        << totalSentBytes << ") due to timing-based batching";

    // TDD RED TEST analysis
    printf("   🔴 TDD RED TEST RESULT:\n");
    if (DatReceiverPrivData.ReceivedDataCnt == totalSentBytes) {
        printf("      - ❌ TEST FAILED: No timing-based batching capability in current IOC\n");
        printf("      - 📋 Implementation needed: IOC timing-aware batching logic\n");
        printf("      - 🎯 Next step: Implement IOC enhancement to make this test GREEN\n");
    } else if (DatReceiverPrivData.ReceivedDataCnt < totalSentBytes) {
        printf("      - ✅ TEST PASSED: IOC has timing-based batching capability!\n");
        printf("      - 📊 Batching achieved: %lu callbacks for %d sends\n", DatReceiverPrivData.ReceivedDataCnt,
               totalSentBytes);
    }

    // Analyze the expected vs actual pattern
    printf("   📈 TDD RED TEST - REQUIREMENT vs CURRENT IMPLEMENTATION:\n");
    printf("      - TDD requirement: Smart timing-based batching during overlap windows\n");
    printf("      - Current IOC result: %lu callbacks (will fail until enhanced)\n",
           DatReceiverPrivData.ReceivedDataCnt);
    printf("      - Implementation task: Add timing-aware batching logic to IOC framework\n");
    printf("      - Success criteria: Achieve significant callback reduction through smart timing\n");

    if (DatReceiverPrivData.ReceivedDataCnt == totalSentBytes) {
        printf("      - ✅ DOCUMENTED BEHAVIOR: IOC overflow-based batching confirmed\n");
        printf("      - 📋 Design constraint: No timing-window batching in current IOC\n");
        printf("      - 🎯 Test value: Documents actual IOC behavior vs initial design assumptions\n");
    } else if (DatReceiverPrivData.ReceivedDataCnt == 3) {
        printf("      - 🎉 SURPRISE: Perfect burst-based batching achieved!\n");
        printf("      - 📚 Framework capability: IOC has more sophisticated batching than expected\n");
    } else {
        printf("      - � COMPLEX PATTERN: Some batching occurred - investigate timing details\n");
        printf("      - 📊 Partial batching: %lu callbacks for %d sends\n", DatReceiverPrivData.ReceivedDataCnt,
               totalSentBytes);
    }

    printf("   🔴 TDD RED TEST completed - Implementation needed to make GREEN!\n");

    //===CLEANUP===
    if (DatReceiverLinkID != IOC_ID_INVALID) {
        IOC_closeLink(DatReceiverLinkID);
    }
    if (DatSenderLinkID != IOC_ID_INVALID) {
        IOC_closeLink(DatSenderLinkID);
    }
    if (DatReceiverSrvID != IOC_ID_INVALID) {
        IOC_offlineService(DatReceiverSrvID);
    }
}
//======>END OF: [@AC-1,US-5] TC-2B=================================================================

//======>BEGIN OF: [@AC-2,US-5] TC-1===============================================================
/**
 * @[Name]: verifyDatStreamGranularity_byBlockToBytePattern_expectFragmentationSupport
 * @[Steps]:
 *   1) Setup DatSender and DatReceiver connections with polling mode AS SETUP.
 *   2) Send large data blocks using IOC_sendDAT AS BEHAVIOR.
 *   3) Attempt to receive data in small fragments using IOC_recvDAT AS BEHAVIOR.
 *   4) Verify partial reception and data reconstruction AS VERIFY.
 *   5) Cleanup connections AS CLEANUP.
 * @[Expect]: Large block transmission successfully fragmented and received in smaller pieces.
 * @[Notes]: Tests DAT STREAM fragmentation capability - receiver granularity control.
 */
TEST(UT_DataEdge, verifyDatStreamGranularity_byBlockToBytePattern_expectFragmentationSupport) {
    printf("\n📋 [@AC-2,US-5] TC-1: DAT Stream Granularity - Block-to-Byte Pattern\n");

    //===SETUP===
    IOC_Result_T Result = IOC_RESULT_FAILURE;
    IOC_SrvID_T DatSenderSrvID = IOC_ID_INVALID;
    IOC_LinkID_T DatSenderLinkID = IOC_ID_INVALID;
    IOC_LinkID_T DatReceiverLinkID = IOC_ID_INVALID;

    printf("📋 Setting up DAT block-to-fragment granularity testing...\n");

    // Setup DatSender service (no callback - polling mode for receiver)
    IOC_SrvURI_T DatSenderSrvURI = {
        .pProtocol = IOC_SRV_PROTO_FIFO,
        .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
        .pPath = "DatBlockToFragmentSender",
    };

    IOC_SrvArgs_T DatSenderSrvArgs = {
        .SrvURI = DatSenderSrvURI,
        .UsageCapabilites = IOC_LinkUsageDatSender,
        .UsageArgs = {.pDat = NULL},  // No usage args for sender
    };

    Result = IOC_onlineService(&DatSenderSrvID, &DatSenderSrvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result) << "DatSender service should come online successfully";

    // Setup DatReceiver connection (polling mode)
    IOC_ConnArgs_T DatReceiverConnArgs = {
        .SrvURI = DatSenderSrvURI,
        .Usage = IOC_LinkUsageDatReceiver,
        .UsageArgs = {.pDat = NULL},  // Polling mode - no callback
    };

    std::thread DatReceiverThread([&] {
        IOC_Result_T ThreadResult = IOC_connectService(&DatReceiverLinkID, &DatReceiverConnArgs, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, ThreadResult);
        ASSERT_NE(IOC_ID_INVALID, DatReceiverLinkID);
    });

    Result = IOC_acceptClient(DatSenderSrvID, &DatSenderLinkID, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result) << "DatSender should accept connection";

    DatReceiverThread.join();
    printf("   ✓ Block-to-fragment test connections established\n");

    //===BEHAVIOR===
    printf("📋 Testing block-by-block send with fragment-by-fragment receive pattern...\n");

    // Send large block of data
    const int BlockSize = 1024;  // 1KB block
    char LargeBlock[BlockSize];
    for (int i = 0; i < BlockSize; i++) {
        LargeBlock[i] = (char)('0' + (i % 10));  // 0-9 repeating pattern
    }

    printf("🧪 Sending large block (%d bytes)...\n", BlockSize);

    IOC_DatDesc_T BlockDesc = {0};
    IOC_initDatDesc(&BlockDesc);
    BlockDesc.Payload.pData = LargeBlock;
    BlockDesc.Payload.PtrDataSize = BlockSize;
    BlockDesc.Payload.PtrDataLen = BlockSize;

    Result = IOC_sendDAT(DatSenderLinkID, &BlockDesc, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result) << "Large block should send successfully";

    IOC_flushDAT(DatSenderLinkID, NULL);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));  // Allow data to arrive

    // Receive data in small fragments
    printf("🧪 Receiving data in small fragments...\n");

    char ReconstructedData[BlockSize];
    int TotalReceived = 0;
    int FragmentCount = 0;
    const int FragmentSize = 16;  // 16-byte fragments

    IOC_Option_defineSyncMayBlock(RecvOptions);

    while (TotalReceived < BlockSize) {
        IOC_DatDesc_T FragmentDesc = {0};
        IOC_initDatDesc(&FragmentDesc);

        char FragmentBuffer[FragmentSize];
        FragmentDesc.Payload.pData = FragmentBuffer;
        FragmentDesc.Payload.PtrDataSize = FragmentSize;
        FragmentDesc.Payload.PtrDataLen = 0;

        Result = IOC_recvDAT(DatReceiverLinkID, &FragmentDesc, &RecvOptions);

        if (Result == IOC_RESULT_SUCCESS) {
            // Copy received fragment to reconstruction buffer
            memcpy(ReconstructedData + TotalReceived, FragmentBuffer, FragmentDesc.Payload.PtrDataSize);
            TotalReceived += FragmentDesc.Payload.PtrDataSize;
            FragmentCount++;

            printf("   Fragment %d: received %lu bytes (total: %d/%d)\n", FragmentCount,
                   FragmentDesc.Payload.PtrDataSize, TotalReceived, BlockSize);
        } else if (Result == IOC_RESULT_NO_DATA) {
            // No more data available - this is expected when we've received everything
            printf("   No more data available after %d bytes\n", TotalReceived);
            break;
        } else {
            FAIL() << "Unexpected result from IOC_recvDAT: " << Result;
        }

        // Safety check to prevent infinite loops
        if (FragmentCount > BlockSize) {
            FAIL() << "Too many fragments received - possible infinite loop";
        }
    }

    //===VERIFY===
    printf("📋 Verifying block-to-fragment stream reconstruction...\n");

    // KeyVerifyPoint-1: All data should be received
    ASSERT_EQ(BlockSize, TotalReceived) << "Should receive complete block data. Expected: " << BlockSize
                                        << ", Actual: " << TotalReceived;

    // KeyVerifyPoint-2: Data integrity should be preserved
    ASSERT_EQ(0, memcmp(LargeBlock, ReconstructedData, BlockSize)) << "Reconstructed data should match original block";

    // KeyVerifyPoint-3: Multiple fragments should be created from single block
    ASSERT_GT(FragmentCount, 1) << "Should receive multiple fragments from single large block";

    printf("   ✅ Block-to-fragment granularity test completed successfully!\n");
    printf("   📊 Sent: 1 block (%d bytes), Received: %d fragments (%d bytes total)\n", BlockSize, FragmentCount,
           TotalReceived);

    //===CLEANUP===
    if (DatReceiverLinkID != IOC_ID_INVALID) {
        IOC_closeLink(DatReceiverLinkID);
    }
    if (DatSenderLinkID != IOC_ID_INVALID) {
        IOC_closeLink(DatSenderLinkID);
    }
    if (DatSenderSrvID != IOC_ID_INVALID) {
        IOC_offlineService(DatSenderSrvID);
    }
}
//======>END OF: [@AC-2,US-5] TC-1=================================================================

//======>BEGIN OF: [@AC-3,US-5] TC-1===============================================================
/**
 * @[Name]: verifyDatStreamGranularity_byVariablePatterns_expectConsistentBehavior
 * @[Steps]:
 *   1) Setup DatSender and DatReceiver connections AS SETUP.
 *   2) Send data using variable chunk sizes (1B, 10B, 100B, 1KB alternating) AS BEHAVIOR.
 *   3) Receive data using variable buffer sizes AS BEHAVIOR.
 *   4) Verify stream consistency across all granularity changes AS VERIFY.
 *   5) Cleanup connections AS CLEANUP.
 * @[Expect]: Variable granularity patterns maintain stream consistency and data integrity.
 * @[Notes]: Tests DAT STREAM adaptability - real-world mixed granularity scenarios.
 */
TEST(UT_DataEdge, verifyDatStreamGranularity_byVariablePatterns_expectConsistentBehavior) {
    printf("\n📋 [@AC-3,US-5] TC-1: DAT Stream Granularity - Variable Patterns\n");

    //===SETUP===
    IOC_Result_T Result = IOC_RESULT_FAILURE;
    IOC_SrvID_T DatReceiverSrvID = IOC_ID_INVALID;
    IOC_LinkID_T DatSenderLinkID = IOC_ID_INVALID;
    IOC_LinkID_T DatReceiverLinkID = IOC_ID_INVALID;

    __DatEdgePrivData_T DatReceiverPrivData = {0};
    DatReceiverPrivData.ClientIndex = 3;
    DatReceiverPrivData.ReceivedContentWritePos = 0;

    printf("📋 Setting up DAT variable granularity pattern testing...\n");

    // Setup similar to TC-1 but with callback for easier verification
    IOC_SrvURI_T DatReceiverSrvURI = {
        .pProtocol = IOC_SRV_PROTO_FIFO,
        .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
        .pPath = "DatVariableGranularityReceiver",
    };

    IOC_DatUsageArgs_T DatReceiverUsageArgs = {
        .CbRecvDat_F = __CbRecvDat_Edge_F,
        .pCbPrivData = &DatReceiverPrivData,
    };

    IOC_SrvArgs_T DatReceiverSrvArgs = {
        .SrvURI = DatReceiverSrvURI,
        .UsageCapabilites = IOC_LinkUsageDatReceiver,
        .UsageArgs = {.pDat = &DatReceiverUsageArgs},
    };

    Result = IOC_onlineService(&DatReceiverSrvID, &DatReceiverSrvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);

    IOC_ConnArgs_T DatSenderConnArgs = {
        .SrvURI = DatReceiverSrvURI,
        .Usage = IOC_LinkUsageDatSender,
    };

    std::thread DatSenderThread([&] {
        IOC_Result_T ThreadResult = IOC_connectService(&DatSenderLinkID, &DatSenderConnArgs, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, ThreadResult);
    });

    Result = IOC_acceptClient(DatReceiverSrvID, &DatReceiverLinkID, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);

    DatSenderThread.join();
    printf("   ✓ Variable granularity test connections established\n");

    //===BEHAVIOR===
    printf("📋 Testing variable granularity send patterns...\n");

    // Define variable chunk sizes to test
    struct ChunkPattern {
        int size;
        char fillChar;
        const char* description;
    };

    ChunkPattern patterns[] = {{1, 'A', "1-byte micro-chunk"},      {10, 'B', "10-byte small chunk"},
                               {100, 'C', "100-byte medium chunk"}, {1000, 'D', "1000-byte large chunk"},
                               {1, 'E', "1-byte return to micro"},  {500, 'F', "500-byte mid-size chunk"},
                               {2, 'G', "2-byte tiny chunk"},       {50, 'H', "50-byte small-medium chunk"}};

    int numPatterns = sizeof(patterns) / sizeof(patterns[0]);
    int totalExpectedSize = 0;

    // Calculate expected total size
    for (int i = 0; i < numPatterns; i++) {
        totalExpectedSize += patterns[i].size;
    }

    printf("🧪 Sending %d variable-size chunks (total: %d bytes)...\n", numPatterns, totalExpectedSize);

    // Send data with variable chunk sizes
    for (int i = 0; i < numPatterns; i++) {
        // Create chunk data
        std::vector<char> chunkData(patterns[i].size);
        for (int j = 0; j < patterns[i].size; j++) {
            chunkData[j] = patterns[i].fillChar;
        }

        IOC_DatDesc_T ChunkDesc = {0};
        IOC_initDatDesc(&ChunkDesc);
        ChunkDesc.Payload.pData = chunkData.data();
        ChunkDesc.Payload.PtrDataSize = patterns[i].size;
        ChunkDesc.Payload.PtrDataLen = patterns[i].size;

        Result = IOC_sendDAT(DatSenderLinkID, &ChunkDesc, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, Result)
            << "Pattern " << i << " (" << patterns[i].description << ") should send successfully";

        printf("   Sent pattern %d: %s (%d bytes)\n", i + 1, patterns[i].description, patterns[i].size);

        // Variable delay between sends to test different timing patterns
        if (i % 2 == 0) {
            std::this_thread::sleep_for(std::chrono::microseconds(100));  // Fast
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));  // Slow
        }
    }

    IOC_flushDAT(DatSenderLinkID, NULL);
    std::this_thread::sleep_for(std::chrono::milliseconds(300));  // Allow processing

    //===VERIFY===
    printf("📋 Verifying variable granularity stream consistency...\n");

    // KeyVerifyPoint-1: All data should be received
    ASSERT_TRUE(DatReceiverPrivData.CallbackExecuted) << "Callback should execute for variable granularity data";

    // KeyVerifyPoint-2: Total size should match
    ASSERT_EQ(totalExpectedSize, DatReceiverPrivData.TotalReceivedSize)
        << "Total received size should equal sent size. Expected: " << totalExpectedSize
        << ", Actual: " << DatReceiverPrivData.TotalReceivedSize;

    // KeyVerifyPoint-3: Data pattern verification (check pattern boundaries)
    char* receivedPtr = DatReceiverPrivData.ReceivedContent;
    int offset = 0;

    for (int i = 0; i < numPatterns && offset < sizeof(DatReceiverPrivData.ReceivedContent); i++) {
        int patternSize = std::min(patterns[i].size, (int)sizeof(DatReceiverPrivData.ReceivedContent) - offset);

        // Verify this pattern's data
        for (int j = 0; j < patternSize; j++) {
            ASSERT_EQ(patterns[i].fillChar, receivedPtr[offset + j])
                << "Pattern " << i << " data mismatch at byte " << j << ". Expected: " << patterns[i].fillChar
                << ", Got: " << receivedPtr[offset + j];
        }

        offset += patterns[i].size;
    }

    // KeyVerifyPoint-4: Stream should handle rapid granularity changes
    ASSERT_GE(DatReceiverPrivData.ReceivedDataCnt, 1) << "Should receive at least one data callback";

    printf("   ✅ Variable granularity pattern test completed successfully!\n");
    printf("   📊 Sent: %d patterns (%d bytes), Received: %lu bytes in %lu callbacks\n", numPatterns, totalExpectedSize,
           DatReceiverPrivData.TotalReceivedSize, DatReceiverPrivData.ReceivedDataCnt);

    //===CLEANUP===
    if (DatReceiverLinkID != IOC_ID_INVALID) {
        IOC_closeLink(DatReceiverLinkID);
    }
    if (DatSenderLinkID != IOC_ID_INVALID) {
        IOC_closeLink(DatSenderLinkID);
    }
    if (DatReceiverSrvID != IOC_ID_INVALID) {
        IOC_offlineService(DatReceiverSrvID);
    }
}
//======>END OF: [@AC-3,US-5] TC-1=================================================================

//======>END OF US-5 TEST IMPLEMENTATIONS==========================================================
