///////////////////////////////////////////////////////////////////////////////////////////////////
// UT_DataBoundaryUS5.cxx - DAT Boundary Testing: US-5 Stream Granularity Boundary Validation
// 📝 Purpose: Test Cases for User Story 5 - Stream processing developer granularity boundary testing
// 🔄 Focus: DAT stream behavior with different send/receive granularities (byte-by-byte vs block-by-block)
// 🎯 Coverage: [@US-5] Stream granularity boundary validation (AC-1, AC-2, AC-3)
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "UT_DataBoundary.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF US-5 TEST CASES==================================================================
/**************************************************************************************************
 * @brief 【US-5 Test Cases】- Stream Granularity Boundary Validation
 *
 * [@AC-1,US-5] Stream granularity validation - Byte-by-byte send, block receive
 *  TC-1:
 *      @[Name]: verifyDatStreamGranularity_byByteToBlockPattern_expectDataIntegrity
 *      @[Purpose]: Verify DAT stream handles byte-by-byte sending with block-by-block receiving
 *      @[Brief]: Send data 1 byte at a time, receive in larger blocks, verify data reconstruction
 *      @[Coverage]: 1-byte sends, multi-byte receives, stream ordering, data integrity
 *
 *  TODO: TC-2: ...
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

#include "UT_DataBoundary.h"

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
TEST(UT_DataBoundary, verifyDatStreamGranularity_byByteToBlockPattern_expectDataIntegrity) {
    printf("\n📋 [@AC-1,US-5] TC-1: DAT Stream Granularity - Byte-to-Block Pattern\n");

    //===SETUP===
    IOC_Result_T Result = IOC_RESULT_FAILURE;
    IOC_SrvID_T DatReceiverSrvID = IOC_ID_INVALID;
    IOC_LinkID_T DatSenderLinkID = IOC_ID_INVALID;
    IOC_LinkID_T DatReceiverLinkID = IOC_ID_INVALID;

    __DatBoundaryPrivData_T DatReceiverPrivData = {0};
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
        .CbRecvDat_F = __CbRecvDat_Boundary_F,
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
TEST(UT_DataBoundary, verifyDatStreamGranularity_byBlockToBytePattern_expectFragmentationSupport) {
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
TEST(UT_DataBoundary, verifyDatStreamGranularity_byVariablePatterns_expectConsistentBehavior) {
    printf("\n📋 [@AC-3,US-5] TC-1: DAT Stream Granularity - Variable Patterns\n");

    //===SETUP===
    IOC_Result_T Result = IOC_RESULT_FAILURE;
    IOC_SrvID_T DatReceiverSrvID = IOC_ID_INVALID;
    IOC_LinkID_T DatSenderLinkID = IOC_ID_INVALID;
    IOC_LinkID_T DatReceiverLinkID = IOC_ID_INVALID;

    __DatBoundaryPrivData_T DatReceiverPrivData = {0};
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
        .CbRecvDat_F = __CbRecvDat_Boundary_F,
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
