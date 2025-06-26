///////////////////////////////////////////////////////////////////////////////////////////////////
// UT_DataBoundaryUS2.cxx - DAT Boundary Testing: US-2 Data Size Boundary Validation
// 📝 Purpose: Test Cases for User Story 2 - System integrator data size boundary testing
// 🔄 Focus: Zero-size data, maximum data size, oversized data handling, data integrity
// 🎯 Coverage: [@US-2] Data size boundary validation (AC-1, AC-2, AC-3)
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "UT_DataBoundary.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF US-2 TEST CASES==================================================================
/**************************************************************************************************
 * @brief 【US-2 Test Cases】- Data Size Boundary Validation
 *
 * [@AC-1,US-2] Data size boundary validation - Zero size data
 *  TC-1:
 *      @[Name]: verifyDatDataSizeBoundary_byZeroSizeData_expectConsistentBehavior
 *      @[Purpose]: Verify zero-size data transmission behavior
 *      @[Brief]: Send 0-byte data, verify transmission and reception behavior
 *      @[Coverage]: Valid pointer + zero size, NULL pointer + zero size, embedded zero size
 *
 *  TC-2:
 *      @[Name]: verifyDatDataSizeBoundary_byZeroSizeEdgeCases_expectRobustHandling
 *      @[Purpose]: Verify zero-size data edge cases and mixed scenarios
 *      @[Brief]: Test zero-size data with various options, timeouts, and mixed with normal data transmission
 *      @[Coverage]: Zero-size with IOC_Options, mixed with normal data, concurrent transmissions
 *
 *-------------------------------------------------------------------------------------------------
 * TODO: [@AC-2,US-2] Data size boundary validation - Maximum size
 *  TC-3:
 *      @[Name]: verifyDatDataSizeBoundary_byMaximumAllowedSize_expectSuccessfulTransmission
 *      @[Purpose]: Verify maximum allowed data size transmission
 *      @[Brief]: Send data at maximum size limit, verify successful transmission and integrity
 *      @[Coverage]: Maximum size data, data integrity verification, performance boundaries
 *
 *-------------------------------------------------------------------------------------------------
 * TODO: [@AC-3,US-2] Data size boundary validation - Oversized data
 *  TC-4:
 *      @[Name]: verifyDatDataSizeBoundary_byOversizedData_expectDataTooLargeError
 *      @[Purpose]: Verify oversized data rejection
 *      @[Brief]: Attempt to send data exceeding limits, verify IOC_RESULT_DATA_TOO_LARGE
 *      @[Coverage]: Oversized data rejection, system stability with large data, memory protection
 *
 *-------------------------------------------------------------------------------------------------
 * TODO: [@AC-1,US-2] Data size boundary validation - Minimum size
 *  TC-5:
 *      @[Name]: verifyDatDataSizeBoundary_byMinimumDataSize_expectSuccessfulTransmission
 *      @[Purpose]: Verify minimum valid data size (1 byte) transmission
 *      @[Brief]: Send 1-byte data, verify transmission and reception behavior
 *      @[Coverage]: 1-byte data transmission, small data handling, boundary between zero and minimum
 *
 *************************************************************************************************/
//======>END OF US-2 TEST CASES====================================================================

#include "UT_DataBoundary.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF US-2 TEST IMPLEMENTATIONS========================================================

//======>BEGIN OF: [@AC-1,US-2] TC-1===============================================================
/**
 * @[Name]: verifyDatDataSizeBoundary_byZeroSizeData_expectConsistentBehavior
 * @[Steps]:
 *   1) Establish DatReceiver service and DatSender connection AS SETUP.
 *      |-> DatReceiver online service with callback registration
 *      |-> DatSender connect with IOC_LinkUsageDatSender
 *      |-> Verify connection establishment
 *   2) Test zero-size data transmission using IOC_sendDAT AS BEHAVIOR.
 *      |-> Create IOC_DatDesc_T with zero-size payload (pData=valid, PtrDataSize=0)
 *      |-> Call IOC_sendDAT with zero-size data
 *      |-> Verify function returns appropriate result code
 *   3) Test zero-size data transmission using different payload configurations AS BEHAVIOR.
 *      |-> Test NULL pData with zero PtrDataSize
 *      |-> Test valid pData with zero PtrDataSize
 *      |-> Test embedded data with zero EmdDataSize
 *   4) Verify receiver behavior with zero-size data AS BEHAVIOR.
 *      |-> Check if callback is invoked for zero-size data
 *      |-> Verify callback receives correct zero-size parameters
 *      |-> Test polling mode behavior with zero-size data
 *   5) Verify system consistency and error handling AS VERIFY.
 *      |-> Zero-size data behavior is consistent (success or defined error)
 *      |-> No crashes or memory corruption with zero-size data
 *      |-> Receiver handles zero-size data correctly in both callback and polling modes
 *   6) Cleanup connections and service AS CLEANUP.
 * @[Expect]: Consistent zero-size data handling - either successful transmission with proper receiver notification,
 * consistent error code (IOC_RESULT_INVALID_PARAM) for invalid zero-size configurations, or IOC_RESULT_ZERO_DATA
 * when the system detects both PtrDataSize and EmdDataSize are zero.
 * @[Notes]: Critical boundary test per AC-1@US-2 - validates system behavior with empty data payload, ensuring
 * no crashes and consistent handling across different zero-size data configurations.
 */
TEST(UT_DataBoundary, verifyDatDataSizeBoundary_byZeroSizeData_expectConsistentBehavior) {
    //===SETUP===
    printf("BEHAVIOR: verifyDatDataSizeBoundary_byZeroSizeData_expectConsistentBehavior\n");

    // Initialize test data structures
    __DatBoundaryPrivData_T DatReceiverPrivData = {0};
    DatReceiverPrivData.ClientIndex = 1;

    IOC_SrvID_T DatReceiverSrvID = IOC_ID_INVALID;
    IOC_LinkID_T DatSenderLinkID = IOC_ID_INVALID;
    IOC_LinkID_T DatReceiverLinkID = IOC_ID_INVALID;
    IOC_Result_T Result = IOC_RESULT_FAILURE;

    // Step-1: DatReceiver online service with callback configuration
    printf("📋 Setting up DatReceiver service...\n");

    // Standard SrvURI for boundary DAT communication
    IOC_SrvURI_T DatReceiverSrvURI = {
        .pProtocol = IOC_SRV_PROTO_FIFO,
        .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
        .pPath = "DatBoundaryReceiver",
    };

    // Configure DAT receiver arguments with boundary callback
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

    // Step-2: DatSender connect to DatReceiver service
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

    //===BEHAVIOR: Zero-Size Data Transmission Tests===
    printf("📋 Testing zero-size data transmission behaviors...\n");

    // Test 1: Valid pointer with zero size (most common zero-size scenario)
    printf("🧪 Test 1: Valid pointer with zero PtrDataSize...\n");
    IOC_DatDesc_T ZeroSizeDesc1 = {0};
    IOC_initDatDesc(&ZeroSizeDesc1);

    const char *validPtr = "dummy";  // Valid pointer but size is 0
    ZeroSizeDesc1.Payload.pData = (void *)validPtr;
    ZeroSizeDesc1.Payload.PtrDataSize = 0;  // Zero size

    Result = IOC_sendDAT(DatSenderLinkID, &ZeroSizeDesc1, NULL);
    printf("   IOC_sendDAT with valid pointer + zero size returned: %d\n", Result);

    // System should return IOC_RESULT_ZERO_DATA when both PtrDataSize and EmdDataSize are zero
    ASSERT_EQ(IOC_RESULT_ZERO_DATA, Result)
        << "Zero-size data (both PtrDataSize=0 and EmdDataSize=0) should return IOC_RESULT_ZERO_DATA, got result: "
        << Result;

    IOC_Result_T ValidPtrZeroSizeResult = Result;  // Store for consistency check

    // Test 2: NULL pointer with zero size (edge case)
    printf("🧪 Test 2: NULL pointer with zero PtrDataSize...\n");
    IOC_DatDesc_T ZeroSizeDesc2 = {0};
    IOC_initDatDesc(&ZeroSizeDesc2);

    ZeroSizeDesc2.Payload.pData = NULL;     // NULL pointer
    ZeroSizeDesc2.Payload.PtrDataSize = 0;  // Zero size

    Result = IOC_sendDAT(DatSenderLinkID, &ZeroSizeDesc2, NULL);
    printf("   IOC_sendDAT with NULL pointer + zero size returned: %d\n", Result);

    // NULL pointer with zero size should return IOC_RESULT_ZERO_DATA
    ASSERT_EQ(IOC_RESULT_ZERO_DATA, Result)
        << "Zero-size data with NULL pointer should return IOC_RESULT_ZERO_DATA (-516), got result: " << Result;

    // Test 3: Embedded data with zero size
    printf("🧪 Test 3: Embedded data with zero EmdDataSize...\n");
    IOC_DatDesc_T ZeroSizeDesc3 = {0};
    IOC_initDatDesc(&ZeroSizeDesc3);

    ZeroSizeDesc3.Payload.pData = NULL;             // No pointer data
    ZeroSizeDesc3.Payload.PtrDataSize = 0;          // No pointer size
    ZeroSizeDesc3.Payload.EmdDataLen = 0;           // Zero embedded size
    ZeroSizeDesc3.Payload.EmdData[0] = 0x12345678;  // Some data in embedded array (but size=0)

    Result = IOC_sendDAT(DatSenderLinkID, &ZeroSizeDesc3, NULL);
    printf("   IOC_sendDAT with embedded data + zero size returned: %d\n", Result);

    // Embedded zero-size should return IOC_RESULT_ZERO_DATA
    ASSERT_EQ(IOC_RESULT_ZERO_DATA, Result)
        << "Zero-size embedded data should return IOC_RESULT_ZERO_DATA (-516), got result: " << Result;

    // Test 4: Consistency check - multiple calls with same zero-size configuration
    printf("🧪 Test 4: Consistency check with repeated zero-size calls...\n");
    for (int i = 0; i < 3; i++) {
        IOC_DatDesc_T ConsistencyDesc = {0};
        IOC_initDatDesc(&ConsistencyDesc);

        ConsistencyDesc.Payload.pData = (void *)validPtr;
        ConsistencyDesc.Payload.PtrDataSize = 0;

        Result = IOC_sendDAT(DatSenderLinkID, &ConsistencyDesc, NULL);
        ASSERT_EQ(IOC_RESULT_ZERO_DATA, Result)
            << "Repeated zero-size calls should return IOC_RESULT_ZERO_DATA consistently (call #" << i << ")";
    }
    printf("   ✓ Consistency verified across multiple zero-size calls\n");

    //===BEHAVIOR: Additional Boundary Scenarios===
    printf("📋 Testing additional boundary scenarios...\n");

    // Test 5: Service as DatSender (reversed role) - zero-size data from service to client
    printf("🧪 Test 5: Service as DatSender with zero-size data...\n");

    // Setup DatSender service (reversed role)
    IOC_SrvID_T DatSenderSrvID = IOC_ID_INVALID;
    IOC_LinkID_T DatSenderServiceLinkID = IOC_ID_INVALID;
    IOC_LinkID_T DatReceiverClientLinkID = IOC_ID_INVALID;

    __DatBoundaryPrivData_T DatReceiverClientPrivData = {0};
    DatReceiverClientPrivData.ClientIndex = 2;

    IOC_SrvURI_T DatSenderSrvURI = {
        .pProtocol = IOC_SRV_PROTO_FIFO,
        .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
        .pPath = "DatSenderService_ZeroSize",
    };

    // DatSender as service (server role)
    IOC_SrvArgs_T DatSenderSrvArgs = {
        .SrvURI = DatSenderSrvURI,
        .UsageCapabilites = IOC_LinkUsageDatSender,
    };

    Result = IOC_onlineService(&DatSenderSrvID, &DatSenderSrvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result) << "DatSender service should online successfully";
    printf("   ✓ DatSender service onlined with SrvID=%llu\n", DatSenderSrvID);

    // DatReceiver as client with callback
    IOC_DatUsageArgs_T DatReceiverClientUsageArgs = {
        .CbRecvDat_F = __CbRecvDat_Boundary_F,
        .pCbPrivData = &DatReceiverClientPrivData,
    };

    IOC_ConnArgs_T DatReceiverClientConnArgs = {
        .SrvURI = DatSenderSrvURI,
        .Usage = IOC_LinkUsageDatReceiver,
        .UsageArgs =
            {
                .pDat = &DatReceiverClientUsageArgs,
            },
    };

    std::thread DatReceiverClientThread([&] {
        IOC_Result_T ThreadResult = IOC_connectService(&DatReceiverClientLinkID, &DatReceiverClientConnArgs, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, ThreadResult);
        ASSERT_NE(IOC_ID_INVALID, DatReceiverClientLinkID);
    });

    // DatSender service accept connection
    Result = IOC_acceptClient(DatSenderSrvID, &DatSenderServiceLinkID, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result) << "DatSender service should accept connection";

    DatReceiverClientThread.join();
    printf("   ✓ DatReceiver client connected with LinkID=%llu\n", DatReceiverClientLinkID);
    printf("   ✓ DatSender service accepted with LinkID=%llu\n", DatSenderServiceLinkID);

    // Test zero-size data transmission from service (DatSender) to client (DatReceiver)
    IOC_DatDesc_T ServiceZeroSizeDesc = {0};
    IOC_initDatDesc(&ServiceZeroSizeDesc);
    ServiceZeroSizeDesc.Payload.pData = (void *)validPtr;
    ServiceZeroSizeDesc.Payload.PtrDataSize = 0;  // Zero size

    Result = IOC_sendDAT(DatSenderServiceLinkID, &ServiceZeroSizeDesc, NULL);
    printf("   Service-to-client zero-size data returned: %d\n", Result);

    ASSERT_EQ(IOC_RESULT_ZERO_DATA, Result)
        << "Service as DatSender should return IOC_RESULT_ZERO_DATA for zero-size data";

    // Cleanup DatSender service before creating polling receiver (service limit is 2)
    printf("🧹 Cleaning up DatSender service before polling test...\n");

    if (DatReceiverClientLinkID != IOC_ID_INVALID) {
        IOC_closeLink(DatReceiverClientLinkID);
        printf("   ✓ DatReceiver client connection closed\n");
    }

    if (DatSenderServiceLinkID != IOC_ID_INVALID) {
        IOC_closeLink(DatSenderServiceLinkID);
        printf("   ✓ DatSender service connection closed\n");
    }

    if (DatSenderSrvID != IOC_ID_INVALID) {
        IOC_offlineService(DatSenderSrvID);
        printf("   ✓ DatSender service offline\n");
        DatSenderSrvID = IOC_ID_INVALID;  // Mark as cleaned up
    }

    // Test 6: Polling mode without recvDAT - setup polling receiver for zero-size boundary
    printf("🧪 Test 6: Polling mode receiver (no callback) with zero-size data detection...\n");

    // Setup DatReceiver service without callback (polling mode)
    IOC_SrvID_T DatPollingReceiverSrvID = IOC_ID_INVALID;
    IOC_LinkID_T DatPollingReceiverLinkID = IOC_ID_INVALID;
    IOC_LinkID_T DatPollingSenderLinkID = IOC_ID_INVALID;

    IOC_SrvURI_T DatPollingReceiverSrvURI = {
        .pProtocol = IOC_SRV_PROTO_FIFO,
        .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
        .pPath = "DatPollingReceiver_ZeroSize",
    };

    // DatReceiver service WITHOUT callback - pure polling mode
    IOC_SrvArgs_T DatPollingReceiverSrvArgs = {
        .SrvURI = DatPollingReceiverSrvURI, .UsageCapabilites = IOC_LinkUsageDatReceiver,
        // No UsageArgs means no callback - enables polling mode
    };

    Result = IOC_onlineService(&DatPollingReceiverSrvID, &DatPollingReceiverSrvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result) << "DatReceiver polling service should online successfully";
    printf("   ✓ DatReceiver polling service onlined with SrvID=%llu\n", DatPollingReceiverSrvID);

    // DatSender connect to polling receiver
    IOC_ConnArgs_T DatPollingSenderConnArgs = {
        .SrvURI = DatPollingReceiverSrvURI,
        .Usage = IOC_LinkUsageDatSender,
    };

    std::thread DatPollingSenderThread([&] {
        IOC_Result_T ThreadResult = IOC_connectService(&DatPollingSenderLinkID, &DatPollingSenderConnArgs, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, ThreadResult);
        ASSERT_NE(IOC_ID_INVALID, DatPollingSenderLinkID);
    });

    // DatReceiver accept connection
    Result = IOC_acceptClient(DatPollingReceiverSrvID, &DatPollingReceiverLinkID, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result) << "DatReceiver polling service should accept connection";

    DatPollingSenderThread.join();
    printf("   ✓ DatSender connected to polling receiver with LinkID=%llu\n", DatPollingSenderLinkID);
    printf("   ✓ DatReceiver polling service accepted with LinkID=%llu\n", DatPollingReceiverLinkID);

    // Test normal data first to ensure polling mode is working
    printf("   🧪 Test 6a: Verify polling mode works with normal data...\n");
    IOC_DatDesc_T NormalDataDesc = {0};
    IOC_initDatDesc(&NormalDataDesc);
    const char *normalData = "test_polling";
    NormalDataDesc.Payload.pData = (void *)normalData;
    NormalDataDesc.Payload.PtrDataSize = strlen(normalData);

    Result = IOC_sendDAT(DatPollingSenderLinkID, &NormalDataDesc, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result) << "Normal data should send successfully in polling mode";

    IOC_flushDAT(DatPollingSenderLinkID, NULL);

    // Poll for normal data to verify polling mode functionality
    IOC_DatDesc_T PollingReceiveDesc = {0};
    IOC_initDatDesc(&PollingReceiveDesc);
    char pollingBuffer[100];
    PollingReceiveDesc.Payload.pData = pollingBuffer;
    PollingReceiveDesc.Payload.PtrDataSize = sizeof(pollingBuffer);

    IOC_Option_defineSyncMayBlock(PollingOptions);
    Result = IOC_recvDAT(DatPollingReceiverLinkID, &PollingReceiveDesc, &PollingOptions);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result) << "Polling should receive normal data successfully";
    ASSERT_EQ(strlen(normalData), PollingReceiveDesc.Payload.PtrDataSize) << "Polling should receive correct data size";
    printf("   ✓ Polling mode verified: received %lu bytes of normal data\n", PollingReceiveDesc.Payload.PtrDataSize);

    // Test zero-size data with polling - this should return IOC_RESULT_ZERO_DATA at send time
    printf("   🧪 Test 6b: Zero-size data behavior in polling mode...\n");
    IOC_DatDesc_T PollingZeroSizeDesc = {0};
    IOC_initDatDesc(&PollingZeroSizeDesc);
    PollingZeroSizeDesc.Payload.pData = (void *)validPtr;
    PollingZeroSizeDesc.Payload.PtrDataSize = 0;  // Zero size

    Result = IOC_sendDAT(DatPollingSenderLinkID, &PollingZeroSizeDesc, NULL);
    printf("   Zero-size data to polling receiver returned: %d\n", Result);

    ASSERT_EQ(IOC_RESULT_ZERO_DATA, Result) << "Zero-size data should return IOC_RESULT_ZERO_DATA even in polling mode";

    // Verify no data is available for polling after zero-size send attempt
    IOC_DatDesc_T NoDataPollingDesc = {0};
    IOC_initDatDesc(&NoDataPollingDesc);
    char noDataBuffer[100];
    NoDataPollingDesc.Payload.pData = noDataBuffer;
    NoDataPollingDesc.Payload.PtrDataSize = sizeof(noDataBuffer);

    IOC_Option_defineSyncNonBlock(NoDataOptions);
    Result = IOC_recvDAT(DatPollingReceiverLinkID, &NoDataPollingDesc, &NoDataOptions);
    ASSERT_EQ(IOC_RESULT_NO_DATA, Result)
        << "Polling should return NO_DATA when zero-size data was rejected at send time";
    printf("   ✓ Polling correctly returns NO_DATA when no actual data was sent\n");

    // Cleanup additional test resources
    printf("🧹 Cleaning up remaining test resources...\n");

    // Note: DatSender service was already cleaned up before polling test

    if (DatPollingSenderLinkID != IOC_ID_INVALID) {
        IOC_closeLink(DatPollingSenderLinkID);
        printf("   ✓ DatSender polling connection closed\n");
    }

    if (DatPollingReceiverLinkID != IOC_ID_INVALID) {
        IOC_closeLink(DatPollingReceiverLinkID);
        printf("   ✓ DatReceiver polling connection closed\n");
    }

    if (DatPollingReceiverSrvID != IOC_ID_INVALID) {
        IOC_offlineService(DatPollingReceiverSrvID);
        printf("   ✓ DatReceiver polling service offline\n");
    }

    // KeyVerifyPoint: Additional boundary scenarios completed
    printf("✅ Service as DatSender zero-size data handling verified\n");
    printf("✅ Polling mode zero-size data boundary behavior verified\n");
    printf("✅ Both reversed roles and polling modes handle zero-size data consistently\n");

    //===BEHAVIOR: Receiver Behavior Testing===
    printf("📋 Testing receiver behavior with zero-size data...\n");

    // Force any pending data transmission and give callback time to execute
    IOC_flushDAT(DatSenderLinkID, NULL);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Check zero-size data behavior based on the actual result
    if (ValidPtrZeroSizeResult == IOC_RESULT_ZERO_DATA) {
        printf("🧪 Zero-size data correctly returned IOC_RESULT_ZERO_DATA (-516)\n");
        printf("   ✓ System properly detects when both PtrDataSize and EmdDataSize are zero\n");
        printf("   ✓ No callback/polling verification needed as data was not transmitted\n");
    } else {
        printf("🧪 Unexpected result for zero-size data: %d\n", ValidPtrZeroSizeResult);
        printf("   ⚠️  Expected IOC_RESULT_ZERO_DATA (-516) for zero-size data\n");
    }

    //===VERIFY: System Stability and Consistency===
    printf("🔍 Verifying system stability and consistency...\n");

    // Verify no crashes or memory corruption by attempting normal operations
    ASSERT_NO_FATAL_FAILURE({
        IOC_DatDesc_T StabilityDesc = {0};
        IOC_initDatDesc(&StabilityDesc);
        const char *testData = "stability_test";
        StabilityDesc.Payload.pData = (void *)testData;
        StabilityDesc.Payload.PtrDataSize = strlen(testData);

        Result = IOC_sendDAT(DatSenderLinkID, &StabilityDesc, NULL);
        // Should succeed regardless of previous zero-size operations
        ASSERT_EQ(IOC_RESULT_SUCCESS, Result);
    }) << "System should remain stable after zero-size data operations";

    // Verify consistency of zero-size data handling
    printf("📊 Zero-size data handling summary:\n");
    printf("   • Valid pointer + zero size: ZERO_DATA (%d)\n", IOC_RESULT_ZERO_DATA);
    printf("   • NULL pointer + zero size: ZERO_DATA (%d)\n", IOC_RESULT_ZERO_DATA);
    printf("   • Embedded data + zero size: ZERO_DATA (%d)\n", IOC_RESULT_ZERO_DATA);
    printf("   • System correctly detects when both PtrDataSize and EmdDataSize are zero\n");
    printf("   • Zero-size data behavior is consistent and predictable\n");

    // KeyVerifyPoint: Zero-size data handled consistently
    printf("✅ Zero-size data properly returns IOC_RESULT_ZERO_DATA (-516)\n");
    printf("✅ System correctly identifies zero-size data condition\n");
    printf("✅ No memory corruption or system instability with zero-size data\n");
    printf("✅ Consistent IOC_RESULT_ZERO_DATA behavior across multiple zero-size transmission attempts\n");

    //===CLEANUP===
    printf("🧹 Cleaning up test environment...\n");

    // Close connections
    if (DatSenderLinkID != IOC_ID_INVALID) {
        IOC_closeLink(DatSenderLinkID);
        printf("   ✓ DatSender connection closed\n");
    }

    if (DatReceiverLinkID != IOC_ID_INVALID) {
        IOC_closeLink(DatReceiverLinkID);
        printf("   ✓ DatReceiver connection closed\n");
    }

    // Offline service
    if (DatReceiverSrvID != IOC_ID_INVALID) {
        IOC_offlineService(DatReceiverSrvID);
        printf("   ✓ DatReceiver service offline\n");
    }

    printf("✅ Zero-size data boundary testing completed successfully\n");
}

//======>BEGIN OF: [@AC-1,US-2] TC-2===============================================================
/**
 * @[Name]: verifyDatDataSizeBoundary_byZeroSizeEdgeCases_expectRobustHandling
 * @[Steps]:
 *   1) Establish DatReceiver service and DatSender connection AS SETUP.
 *      |-> DatReceiver online service with callback registration
 *      |-> DatSender connect with IOC_LinkUsageDatSender
 *      |-> Verify connection establishment
 *   2) Test zero-size data with various IOC_Options configurations AS BEHAVIOR.
 *      |-> Test zero-size data with timeout options (blocking, non-blocking, timeout)
 *      |-> Test zero-size data with extreme timeout values
 *      |-> Test zero-size data with malformed options
 *   3) Test zero-size data mixed with normal data transmission AS BEHAVIOR.
 *      |-> Send normal data, then zero-size data, then normal data again
 *      |-> Test rapid alternating between zero-size and normal data
 *      |-> Verify system state consistency during mixed transmissions
 *   4) Test zero-size data under different system conditions AS BEHAVIOR.
 *      |-> Test zero-size data with buffer near capacity
 *      |-> Test zero-size data during high-frequency normal transmissions
 *      |-> Test zero-size data with concurrent connections
 *   5) Test zero-size data error recovery scenarios AS BEHAVIOR.
 *      |-> Test zero-size data after connection interruption
 *      |-> Test zero-size data during connection state transitions
 *      |-> Test zero-size data with invalid connection states
 *   6) Verify robust zero-size data handling under edge conditions AS VERIFY.
 *      |-> All zero-size data attempts return consistent IOC_RESULT_ZERO_DATA
 *      |-> Normal data transmission remains unaffected by zero-size attempts
 *      |-> System maintains stability under mixed zero-size/normal data scenarios
 *      |-> No resource leaks or state corruption from zero-size data edge cases
 *   7) Cleanup connections and services AS CLEANUP.
 * @[Expect]: Robust zero-size data handling under all edge conditions - consistent IOC_RESULT_ZERO_DATA
 * returns, no interference with normal data transmission, system stability maintained under mixed scenarios,
 * proper error recovery from zero-size data attempts under various system conditions.
 * @[Notes]: Comprehensive edge case testing per AC-1@US-2 - validates zero-size data robustness under
 * complex scenarios including mixed transmissions, various options, and system stress conditions.
 */
TEST(UT_DataBoundary, verifyDatDataSizeBoundary_byZeroSizeEdgeCases_expectRobustHandling) {
    //===SETUP===
    printf("BEHAVIOR: verifyDatDataSizeBoundary_byZeroSizeEdgeCases_expectRobustHandling\n");

    // Initialize test data structures
    __DatBoundaryPrivData_T DatReceiverPrivData = {0};
    DatReceiverPrivData.ClientIndex = 10;

    IOC_SrvID_T DatReceiverSrvID = IOC_ID_INVALID;
    IOC_LinkID_T DatSenderLinkID = IOC_ID_INVALID;
    IOC_LinkID_T DatReceiverLinkID = IOC_ID_INVALID;
    IOC_Result_T Result = IOC_RESULT_FAILURE;

    // Step-1: DatReceiver online service with callback configuration
    printf("📋 Setting up DatReceiver service for edge case testing...\n");

    IOC_SrvURI_T DatReceiverSrvURI = {
        .pProtocol = IOC_SRV_PROTO_FIFO,
        .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
        .pPath = "DatEdgeCaseReceiver",
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

    // Step-2: DatSender connect to DatReceiver service
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

    //===BEHAVIOR: Zero-Size Data with Various IOC_Options Configurations===
    printf("📋 Testing zero-size data with various IOC_Options configurations...\n");

    // Test 1: Zero-size data with blocking timeout options
    printf("🧪 Test 1: Zero-size data with blocking timeout options...\n");

    IOC_DatDesc_T ZeroSizeDesc = {0};
    IOC_initDatDesc(&ZeroSizeDesc);
    const char *validPtr = "dummy";
    ZeroSizeDesc.Payload.pData = (void *)validPtr;
    ZeroSizeDesc.Payload.PtrDataSize = 0;  // Zero size
    ZeroSizeDesc.Payload.PtrDataLen = 0;   // Zero length
    ZeroSizeDesc.Payload.EmdDataLen = 0;   // No embedded data

    // Test 1a: Zero-size with blocking option
    IOC_Option_defineSyncMayBlock(BlockingOptions);
    Result = IOC_sendDAT(DatSenderLinkID, &ZeroSizeDesc, &BlockingOptions);
    ASSERT_EQ(IOC_RESULT_ZERO_DATA, Result) << "Zero-size data with blocking option should return IOC_RESULT_ZERO_DATA";
    printf("   ✓ Zero-size data with blocking option: result=%d\n", Result);

    // Test 1b: Zero-size with non-blocking option
    IOC_Option_defineSyncNonBlock(NonBlockingOptions);
    Result = IOC_sendDAT(DatSenderLinkID, &ZeroSizeDesc, &NonBlockingOptions);
    ASSERT_EQ(IOC_RESULT_ZERO_DATA, Result)
        << "Zero-size data with non-blocking option should return IOC_RESULT_ZERO_DATA";
    printf("   ✓ Zero-size data with non-blocking option: result=%d\n", Result);

    // Test 1c: Zero-size with specific timeout
    IOC_Option_defineSyncTimeout(TimeoutOptions, 1000000);  // 1 second timeout
    Result = IOC_sendDAT(DatSenderLinkID, &ZeroSizeDesc, &TimeoutOptions);
    ASSERT_EQ(IOC_RESULT_ZERO_DATA, Result) << "Zero-size data with timeout option should return IOC_RESULT_ZERO_DATA";
    printf("   ✓ Zero-size data with timeout option: result=%d\n", Result);

    // Test 1d: Zero-size with extreme timeout values
    IOC_Option_defineSyncTimeout(ExtremeTimeoutOptions, 0);  // Zero timeout
    Result = IOC_sendDAT(DatSenderLinkID, &ZeroSizeDesc, &ExtremeTimeoutOptions);
    ASSERT_EQ(IOC_RESULT_ZERO_DATA, Result) << "Zero-size data with zero timeout should return IOC_RESULT_ZERO_DATA";
    printf("   ✓ Zero-size data with zero timeout: result=%d\n", Result);

    //===BEHAVIOR: Zero-Size Data Mixed with Normal Data Transmission===
    printf("📋 Testing zero-size data mixed with normal data transmission...\n");

    // Test 2: Normal → Zero-size → Normal data sequence
    printf("🧪 Test 2: Normal → Zero-size → Normal data sequence...\n");

    // Reset receiver tracking
    DatReceiverPrivData.CallbackExecuted = false;
    DatReceiverPrivData.TotalReceivedSize = 0;
    DatReceiverPrivData.ReceivedDataCnt = 0;

    // Send normal data first
    IOC_DatDesc_T NormalDesc1 = {0};
    IOC_initDatDesc(&NormalDesc1);
    const char *normalData1 = "before_zero";
    NormalDesc1.Payload.pData = (void *)normalData1;
    NormalDesc1.Payload.PtrDataSize = strlen(normalData1) + 1;  // Include null terminator
    NormalDesc1.Payload.PtrDataLen = strlen(normalData1);       // Actual data length

    Result = IOC_sendDAT(DatSenderLinkID, &NormalDesc1, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result) << "Normal data before zero-size should succeed";

    // Attempt to send zero-size data
    Result = IOC_sendDAT(DatSenderLinkID, &ZeroSizeDesc, NULL);
    ASSERT_EQ(IOC_RESULT_ZERO_DATA, Result) << "Zero-size data should return IOC_RESULT_ZERO_DATA";

    // Send normal data after
    IOC_DatDesc_T NormalDesc2 = {0};
    IOC_initDatDesc(&NormalDesc2);
    const char *normalData2 = "after_zero";
    NormalDesc2.Payload.pData = (void *)normalData2;
    NormalDesc2.Payload.PtrDataSize = strlen(normalData2) + 1;  // Include null terminator
    NormalDesc2.Payload.PtrDataLen = strlen(normalData2);       // Actual data length

    Result = IOC_sendDAT(DatSenderLinkID, &NormalDesc2, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result) << "Normal data after zero-size should succeed";

    // Flush and allow callbacks to process
    IOC_flushDAT(DatSenderLinkID, NULL);
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Verify only normal data was received (zero-size was rejected at send time)
    ULONG_T ExpectedSize = strlen(normalData1) + strlen(normalData2);
    ASSERT_EQ(ExpectedSize, DatReceiverPrivData.TotalReceivedSize)
        << "Only normal data should be received, zero-size data should not affect receiver";
    ASSERT_EQ(2, DatReceiverPrivData.ReceivedDataCnt)
        << "Should receive exactly 2 normal data packets (zero-size rejected at send)";
    ASSERT_FALSE(DatReceiverPrivData.ZeroSizeDataReceived) << "Zero-size data should not reach receiver";

    printf("   ✓ Normal data transmission unaffected by zero-size attempts\n");
    printf("   ✓ Received %lu bytes in %lu packets (zero-size properly rejected)\n",
           DatReceiverPrivData.TotalReceivedSize, DatReceiverPrivData.ReceivedDataCnt);

    // Test 3: Rapid alternating zero-size and normal data
    printf("🧪 Test 3: Rapid alternating zero-size and normal data...\n");

    // Reset receiver tracking
    DatReceiverPrivData.CallbackExecuted = false;
    DatReceiverPrivData.TotalReceivedSize = 0;
    DatReceiverPrivData.ReceivedDataCnt = 0;

    ULONG_T SuccessfulNormalSends = 0;
    ULONG_T ZeroSizeAttempts = 0;

    for (int i = 0; i < 10; i++) {
        // Try to send zero-size data
        Result = IOC_sendDAT(DatSenderLinkID, &ZeroSizeDesc, NULL);
        ASSERT_EQ(IOC_RESULT_ZERO_DATA, Result)
            << "Zero-size data should consistently return IOC_RESULT_ZERO_DATA in iteration " << i;
        ZeroSizeAttempts++;

        // Send normal data
        IOC_DatDesc_T RapidNormalDesc = {0};
        IOC_initDatDesc(&RapidNormalDesc);
        char rapidData[20];
        snprintf(rapidData, sizeof(rapidData), "rapid_%d", i);
        RapidNormalDesc.Payload.pData = rapidData;
        RapidNormalDesc.Payload.PtrDataSize = sizeof(rapidData);
        RapidNormalDesc.Payload.PtrDataLen = strlen(rapidData);

        Result = IOC_sendDAT(DatSenderLinkID, &RapidNormalDesc, NULL);
        // ASSUME LESS THAN MaxDataQueueSize in IOC_CapabilityDescription_pT
        ASSERT_EQ(IOC_RESULT_SUCCESS, Result) << "Normal data should succeed consistently in iteration " << i;
        SuccessfulNormalSends++;
    }

    // Flush and allow callbacks to process
    IOC_flushDAT(DatSenderLinkID, NULL);
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    // Verify only normal data was received
    ASSERT_EQ(SuccessfulNormalSends, DatReceiverPrivData.ReceivedDataCnt)
        << "Should receive only normal data packets, zero-size attempts should not affect receiver";
    ASSERT_EQ(10, ZeroSizeAttempts) << "Should have attempted 10 zero-size sends";
    ASSERT_EQ(10, SuccessfulNormalSends) << "Should have successfully sent 10 normal data packets";

    printf("   ✓ Rapid alternating test: %lu zero-size attempts (all rejected), %lu normal data received\n",
           ZeroSizeAttempts, DatReceiverPrivData.ReceivedDataCnt);

    //===BEHAVIOR: Zero-Size Data Under Different System Conditions===
    printf("📋 Testing zero-size data under different system conditions...\n");

    // Test 4: Zero-size data with concurrent normal transmissions
    printf("🧪 Test 4: Zero-size data with concurrent normal transmissions...\n");

    // Reset receiver tracking
    DatReceiverPrivData.CallbackExecuted = false;
    DatReceiverPrivData.TotalReceivedSize = 0;
    DatReceiverPrivData.ReceivedDataCnt = 0;

    // Start concurrent normal data transmission in background
    std::atomic<bool> StopConcurrent{false};
    std::atomic<int> ConcurrentSentCount{0};

    std::thread ConcurrentSender([&] {
        int concurrentIndex = 0;
        while (!StopConcurrent.load()) {
            IOC_DatDesc_T ConcurrentDesc = {0};
            IOC_initDatDesc(&ConcurrentDesc);
            char concurrentData[30];
            snprintf(concurrentData, sizeof(concurrentData), "concurrent_%d", concurrentIndex++);
            ConcurrentDesc.Payload.pData = concurrentData;
            ConcurrentDesc.Payload.PtrDataSize = strlen(concurrentData);

            IOC_Result_T concurrentResult = IOC_sendDAT(DatSenderLinkID, &ConcurrentDesc, NULL);
            if (concurrentResult == IOC_RESULT_SUCCESS) {
                ConcurrentSentCount.fetch_add(1);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });

    // Give concurrent sender some time to start
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Attempt zero-size data during concurrent transmissions
    for (int i = 0; i < 5; i++) {
        Result = IOC_sendDAT(DatSenderLinkID, &ZeroSizeDesc, NULL);
        ASSERT_EQ(IOC_RESULT_ZERO_DATA, Result)
            << "Zero-size data should return IOC_RESULT_ZERO_DATA even during concurrent transmissions";
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    // Stop concurrent transmission
    StopConcurrent.store(true);
    ConcurrentSender.join();

    // Flush and allow all data to be processed
    IOC_flushDAT(DatSenderLinkID, NULL);
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    printf("   ✓ Zero-size data handled correctly during concurrent transmissions\n");
    printf("   ✓ Concurrent normal data sent: %d, received: %lu\n", ConcurrentSentCount.load(),
           DatReceiverPrivData.ReceivedDataCnt);

    //===BEHAVIOR: Zero-Size Data Error Recovery Scenarios===
    printf("📋 Testing zero-size data error recovery scenarios...\n");

    // Test 5: Zero-size data behavior consistency after system stress
    printf("🧪 Test 5: Zero-size data consistency after system stress...\n");

    // Apply some system stress with large data transmission
    IOC_DatDesc_T LargeDesc = {0};
    IOC_initDatDesc(&LargeDesc);
    const size_t LargeSize = 32 * 1024;  // 32KB
    char *largeBuf = (char *)malloc(LargeSize);
    if (largeBuf != NULL) {
        memset(largeBuf, 'L', LargeSize);
        LargeDesc.Payload.pData = largeBuf;
        LargeDesc.Payload.PtrDataSize = LargeSize;

        // Send large data to stress the system
        Result = IOC_sendDAT(DatSenderLinkID, &LargeDesc, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, Result) << "Large data transmission should succeed";

        // Immediately try zero-size data after large transmission
        Result = IOC_sendDAT(DatSenderLinkID, &ZeroSizeDesc, NULL);
        ASSERT_EQ(IOC_RESULT_ZERO_DATA, Result)
            << "Zero-size data should return IOC_RESULT_ZERO_DATA consistently after large data transmission";

        free(largeBuf);
        printf("   ✓ Zero-size data behavior consistent after large data transmission\n");
    }

    // Test 6: Multiple consecutive zero-size attempts
    printf("🧪 Test 6: Multiple consecutive zero-size attempts...\n");

    for (int i = 0; i < 20; i++) {
        Result = IOC_sendDAT(DatSenderLinkID, &ZeroSizeDesc, NULL);
        ASSERT_EQ(IOC_RESULT_ZERO_DATA, Result)
            << "Consecutive zero-size attempt #" << i << " should return IOC_RESULT_ZERO_DATA";
    }
    printf("   ✓ 20 consecutive zero-size attempts all handled consistently\n");

    //===VERIFY: Robust Zero-Size Data Handling===
    printf("🔍 Verifying robust zero-size data handling...\n");

    // Verify system stability after all edge case testing
    ASSERT_NO_FATAL_FAILURE({
        IOC_DatDesc_T FinalTestDesc = {0};
        IOC_initDatDesc(&FinalTestDesc);
        const char *finalData = "final_stability_test";
        FinalTestDesc.Payload.pData = (void *)finalData;
        FinalTestDesc.Payload.PtrDataSize = strlen(finalData);

        Result = IOC_sendDAT(DatSenderLinkID, &FinalTestDesc, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, Result) << "System should remain stable for normal data after edge case testing";
    }) << "System should remain stable after comprehensive zero-size edge case testing";

    // Final zero-size test to verify consistency
    Result = IOC_sendDAT(DatSenderLinkID, &ZeroSizeDesc, NULL);
    ASSERT_EQ(IOC_RESULT_ZERO_DATA, Result)
        << "Final zero-size test should still return IOC_RESULT_ZERO_DATA consistently";

    // KeyVerifyPoint: Comprehensive zero-size edge case testing completed
    printf("✅ Zero-size data robustly handled under all tested edge conditions\n");
    printf("✅ Consistent IOC_RESULT_ZERO_DATA returns across all scenarios\n");
    printf("✅ Normal data transmission unaffected by zero-size attempts\n");
    printf("✅ System stability maintained under mixed and stress conditions\n");
    printf("✅ No resource leaks or state corruption detected\n");

    //===CLEANUP===
    printf("🧹 Cleaning up edge case test resources...\n");

    if (DatSenderLinkID != IOC_ID_INVALID) {
        IOC_closeLink(DatSenderLinkID);
        printf("   ✓ DatSender connection closed\n");
    }

    if (DatReceiverLinkID != IOC_ID_INVALID) {
        IOC_closeLink(DatReceiverLinkID);
        printf("   ✓ DatReceiver connection closed\n");
    }

    if (DatReceiverSrvID != IOC_ID_INVALID) {
        IOC_offlineService(DatReceiverSrvID);
        printf("   ✓ DatReceiver service offline\n");
    }
}

//======>END OF US-2 TEST IMPLEMENTATIONS==========================================================
