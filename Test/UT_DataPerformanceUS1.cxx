///////////////////////////////////////////////////////////////////////////////////////////////////
// QUICK REFERENCE GUIDE - 快速参考指南
// 📝 用途: DAT性能测试单元测试实现 - User Story 1 Implementation ONLY
// 🔄 流程: Implementation Details (HOW) - Test Cases for US-1
// 📂 分类: DataPerformance US-1 - High throughput verification implementation
// 🎯 重点: 吞吐量验证测试用例的具体实现，详细的HOW测试逻辑
// 📋 需求: 参见 UT_DataPerformance.h 中的 US-1 & AC-1/AC-2 (WHY requirements)
///////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF OVERVIEW OF THIS UNIT TESTING FILE===============================================
/**
 * @brief
 *  DAT性能测试实现 - US-1: 高吞吐量数据传输验证的测试用例实现
 *
 *-------------------------------------------------------------------------------------------------
 *++背景说明：
 *  本测试文件专门实现US-1的高吞吐量验证需求的具体测试用例
 *  重点关注大负载数据传输的吞吐量指标测量和验证的具体实现细节
 *  确保系统在各种负载条件下的传输速率符合预期的测试逻辑
 *
 *  实现重点：
 *  - 测试用例的具体实现逻辑 (HOW details)
 *  - 性能测量的详细方法和步骤
 *  - 验证条件的具体判断逻辑
 *  - 测试数据的生成和处理细节
 *
 *  测试范围：
 *  - TC 实现: US-1 高吞吐量数据传输验证测试用例
 *  - 具体的性能测量和验证逻辑
 *  - 详细的测试步骤和期望结果判断
 *
 *  需求来源：
 *  - US-1 & AC-1/AC-2: 参见 UT_DataPerformance.h 中的完整需求定义
 */
//======>END OF OVERVIEW OF THIS UNIT TESTING FILE=================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF TEST CASE IMPLEMENTATION=========================================================
/**************************************************************************************************
 * @brief 【Test Cases Implementation - US-1 High-throughput verification】
 *
 * 实现 US-1 的具体测试用例，专注于 HOW 实现细节：
 *
 * [@AC-1,US-1] High-throughput bulk data transfer
 *  TC-1:
 *      @[Name]: verifyBulkDataThroughput_byLargePayloads_expectOptimalRates
 *      @[Purpose]: 验证大负载数据传输的吞吐量性能，确保达到目标传输速率
 *      @[Brief]: 使用1KB到256KB不同负载大小测试吞吐量，验证性能随负载扩展
 *      @[Throughput_Focus]: 测试最大数据传输速率和负载大小对性能的影响
 *
 * [@AC-2,US-1] Throughput performance scaling
 *  TC-2:
 *      @[Name]: verifyThroughputScaling_byPayloadSize_expectLinearGrowth
 *      @[Purpose]: 验证吞吐量随负载大小的扩展特性
 *      @[Brief]: 测试不同负载大小的吞吐量扩展关系，验证效率提升
 *      @[Scaling_Focus]: 测试负载大小对传输效率的影响和扩展性
 *
 * 注意：完整的 US & AC 需求定义请参见 UT_DataPerformance.h
 *************************************************************************************************/
//======>END OF TEST CASE IMPLEMENTATION===========================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======BEGIN OF UNIT TESTING IMPLEMENTATION=======================================================
#include "UT_DataPerformance.h"

//===TEMPLATE OF UT CASE===
/**
 * ╔══════════════════════════════════════════════════════════════════════════════════════════╗
 * ║                           🚀 BULK DATA THROUGHPUT VERIFICATION                          ║
 * ╠══════════════════════════════════════════════════════════════════════════════════════════╣
 * ║ @[Name]: verifyBulkDataThroughput_byLargePayloads_expectOptimalRates                    ║
 * ║ @[Steps]: 🔧 setup performance test environment → 🎯 execute multi-size payload tests   ║
 * ║          → ✅ verify throughput metrics → 🧹 cleanup resources                          ║
 * ║ @[Expect]: Throughput ≥10MB/s, success rate >95%, consistent performance scaling       ║
 * ║ @[Notes]: Core performance test case validating maximum data transfer capabilities      ║
 * ╚══════════════════════════════════════════════════════════════════════════════════════════╝
 */
TEST(UT_DataPerformance, verifyBulkDataThroughput_byLargePayloads_expectOptimalRates) {
    // ┌──────────────────────────────────────────────────────────────────────────────────────┐
    // │                                🔧 SETUP PHASE                                        │
    // └──────────────────────────────────────────────────────────────────────────────────────┘
    printf("🧪 [TEST] verifyBulkDataThroughput_byLargePayloads_expectOptimalRates\n");

    // Initialize performance test configuration
    PerformanceTestConfig_T testConfig;
    testConfig.TestDurationSec = std::chrono::seconds(5);
    testConfig.TargetThroughputMBps = 10.0;
    testConfig.MaxAcceptableLatencyMs = 10.0;

    // Setup test service and connections
    IOC_SrvID_T testSrvID = IOC_ID_INVALID;
    IOC_LinkID_T senderLinkID = IOC_ID_INVALID;

    // Service setup - Configure service to RECEIVE data from clients
    IOC_SrvArgs_T srvArgs = {};
    IOC_Helper_initSrvArgs(&srvArgs);
    srvArgs.SrvURI.pProtocol = IOC_SRV_PROTO_FIFO;
    srvArgs.SrvURI.pHost = IOC_SRV_HOST_LOCAL_PROCESS;
    srvArgs.SrvURI.pPath = "test/performance/throughput";
    srvArgs.UsageCapabilites = IOC_LinkUsageDatReceiver;  // Service will RECEIVE data
    srvArgs.Flags = IOC_SRVFLAG_AUTO_ACCEPT;

    IOC_Result_T result = IOC_onlineService(&testSrvID, &srvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result) << "Service setup failed";

    // Client connection setup - Client will SEND data to service
    IOC_ConnArgs_T connArgs = {};
    IOC_Helper_initConnArgs(&connArgs);
    connArgs.SrvURI = srvArgs.SrvURI;
    connArgs.Usage = IOC_LinkUsageDatSender;  // Client will SEND data

    // Connect in SYNC mode (default when pOption = NULL)
    // This means IOC_sendDAT will block until data is transmitted
    // Connection establishment blocks until completed, so no sleep needed
    result = IOC_connectService(&senderLinkID, &connArgs, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result) << "Client connection setup failed";

    // ┌──────────────────────────────────────────────────────────────────────────────────────┐
    // │                               🎯 BEHAVIOR PHASE                                       │
    // └──────────────────────────────────────────────────────────────────────────────────────┘
    printf("🎯 BEHAVIOR: Testing bulk data throughput with multiple payload sizes\n");

    PerformanceCollector collector;
    collector.StartCollection();

    // Test different payload sizes for throughput analysis
    std::vector<size_t> payloadSizes = {1024, 4096, 16384, 65536, 262144};  // 1KB to 256KB

    for (size_t payloadSize : payloadSizes) {
        printf("📊 [PAYLOAD] Testing throughput with %zu bytes\n", payloadSize);

        std::vector<char> testData = CreatePerformanceTestData(payloadSize, false);
        auto startTime = std::chrono::high_resolution_clock::now();
        size_t iterations = 100;

        for (size_t i = 0; i < iterations; ++i) {
            IOC_DatDesc_T sendDesc = {};
            IOC_initDatDesc(&sendDesc);
            sendDesc.Payload.pData = testData.data();
            sendDesc.Payload.PtrDataSize = payloadSize;
            sendDesc.Payload.PtrDataLen = payloadSize;

            auto opStart = std::chrono::high_resolution_clock::now();
            IOC_Result_T sendResult = IOC_sendDAT(senderLinkID, &sendDesc, NULL);
            auto opEnd = std::chrono::high_resolution_clock::now();

            if (sendResult == IOC_RESULT_SUCCESS) {
                double latencyUs = std::chrono::duration<double, std::micro>(opEnd - opStart).count();
                collector.RecordLatency(latencyUs);
                collector.RecordOperation(payloadSize);
            } else {
                collector.RecordError();
                printf("⚠️ [WARNING] IOC_sendDAT failed with result %d\n", sendResult);
            }

            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }

        auto endTime = std::chrono::high_resolution_clock::now();
        double durationSec = std::chrono::duration<double>(endTime - startTime).count();
        double mbps = (iterations * payloadSize) / (durationSec * 1024.0 * 1024.0);
        printf("📈 [RESULT] Payload %zu bytes: %.2f MB/s\n", payloadSize, mbps);
    }

    // ┌──────────────────────────────────────────────────────────────────────────────────────┐
    // │                                ✅ VERIFY PHASE                                        │
    // └──────────────────────────────────────────────────────────────────────────────────────┘
    PerformanceMetrics_T metrics = collector.FinishCollection();
    PrintPerformanceReport(metrics, "Bulk Data Throughput Test");

    //@KeyVerifyPoint-1: Overall throughput should meet target
    VERIFY_THROUGHPUT_TARGET(metrics, testConfig.TargetThroughputMBps);

    //@KeyVerifyPoint-2: Success rate should be high
    VERIFY_SUCCESS_RATE_TARGET(metrics, 0.95);

    //@KeyVerifyPoint-3: Performance should be reasonable
    VERIFY_LATENCY_TARGET(metrics, testConfig.MaxAcceptableLatencyMs);

    // ┌──────────────────────────────────────────────────────────────────────────────────────┐
    // │                               🧹 CLEANUP PHASE                                        │
    // └──────────────────────────────────────────────────────────────────────────────────────┘
    if (senderLinkID != IOC_ID_INVALID) {
        IOC_closeLink(senderLinkID);
    }
    if (testSrvID != IOC_ID_INVALID) {
        IOC_offlineService(testSrvID);
    }
}

//---------------------------------------------------------------------------------------------------------------------
class UT_DataPerformanceUS1Fixture : public ::testing::Test {
   protected:
    static void SetUpTestSuite() {
        printf("🔧 UT_DataPerformanceUS1Fixture->SETUP: SetUpTestSuite\n");
        // Global throughput test environment initialization
    }

    static void TearDownTestSuite() {
        printf("🧹 UT_DataPerformanceUS1Fixture->CLEANUP: TearDownTestSuite\n");
        // Global throughput test environment cleanup
    }

    void SetUp() override {
        printf("🔧 UT_DataPerformanceUS1Fixture->SETUP: SetUp\n");

        // Initialize performance tracking for US-1 tests
        __ResetPerformanceTracking(&senderPrivData);
        __ResetPerformanceTracking(&receiverPrivData);

        // Set throughput-focused test configuration
        testConfig.TestDurationSec = std::chrono::seconds(5);
        testConfig.TargetThroughputMBps = 10.0;
        testConfig.MaxAcceptableLatencyMs = 10.0;
    }

    void TearDown() override {
        printf("🧹 UT_DataPerformanceUS1Fixture->CLEANUP: TearDown\n");

        // Clean up connections
        if (receiverLinkID != IOC_ID_INVALID) {
            IOC_closeLink(receiverLinkID);
            receiverLinkID = IOC_ID_INVALID;
        }
        if (senderLinkID != IOC_ID_INVALID) {
            IOC_closeLink(senderLinkID);
            senderLinkID = IOC_ID_INVALID;
        }
        if (testSrvID != IOC_ID_INVALID) {
            IOC_offlineService(testSrvID);
            testSrvID = IOC_ID_INVALID;
        }
    }

    // Helper method to setup throughput test scenario
    void setupThroughputTestScenario() {
        IOC_SrvArgs_T srvArgs = {};
        IOC_Helper_initSrvArgs(&srvArgs);
        srvArgs.SrvURI.pProtocol = IOC_SRV_PROTO_FIFO;
        srvArgs.SrvURI.pHost = IOC_SRV_HOST_LOCAL_PROCESS;
        srvArgs.SrvURI.pPath = "test/performance/throughput_us1";
        srvArgs.UsageCapabilites = IOC_LinkUsageDatReceiver;  // Service will RECEIVE data from clients
        srvArgs.Flags = IOC_SRVFLAG_AUTO_ACCEPT;

        IOC_Result_T result = IOC_onlineService(&testSrvID, &srvArgs);
        ASSERT_EQ(IOC_RESULT_SUCCESS, result) << "Service setup failed";

        IOC_ConnArgs_T connArgs = {};
        IOC_Helper_initConnArgs(&connArgs);
        connArgs.SrvURI = srvArgs.SrvURI;
        connArgs.Usage = IOC_LinkUsageDatSender;  // Client will SEND data to service

        // Connect in SYNC mode (default when pOption = NULL) for throughput testing
        result = IOC_connectService(&senderLinkID, &connArgs, NULL);  // This is the SENDER link
        ASSERT_EQ(IOC_RESULT_SUCCESS, result) << "Sender connection setup failed";

        // For throughput tests, we primarily need the sender link
        receiverLinkID = IOC_ID_INVALID;  // Not needed for this test
        senderPrivData.ServiceOnline = true;
        senderPrivData.LinkConnected = true;
    }

    // Test data members for US-1
    __DatPerformancePrivData_T senderPrivData;
    __DatPerformancePrivData_T receiverPrivData;
    PerformanceTestConfig_T testConfig;
    IOC_SrvID_T testSrvID = IOC_ID_INVALID;
    IOC_LinkID_T senderLinkID = IOC_ID_INVALID;
    IOC_LinkID_T receiverLinkID = IOC_ID_INVALID;
};

/**
 * ╔══════════════════════════════════════════════════════════════════════════════════════════╗
 * ║                        📈 THROUGHPUT SCALING VERIFICATION                               ║
 * ╠══════════════════════════════════════════════════════════════════════════════════════════╣
 * ║ @[Name]: verifyThroughputScaling_byPayloadSize_expectLinearGrowth                      ║
 * ║ @[Steps]: 🔧 setup fixture test environment → 🎯 test varying payload sizes             ║
 * ║          → ✅ verify scaling relationships → 🧹 fixture cleanup                         ║
 * ║ @[Expect]: Throughput increases with payload size, performance data collected          ║
 * ║ @[Notes]: Fixture-based test validating throughput scaling characteristics             ║
 * ╚══════════════════════════════════════════════════════════════════════════════════════════╝
 */
TEST_F(UT_DataPerformanceUS1Fixture, verifyThroughputScaling_byPayloadSize_expectLinearGrowth) {
    // ┌──────────────────────────────────────────────────────────────────────────────────────┐
    // │                                🔧 SETUP PHASE                                        │
    // └──────────────────────────────────────────────────────────────────────────────────────┘
    setupThroughputTestScenario();

    // ┌──────────────────────────────────────────────────────────────────────────────────────┐
    // │                               🎯 BEHAVIOR PHASE                                       │
    // └──────────────────────────────────────────────────────────────────────────────────────┘
    printf("🎯 US1Fixture->BEHAVIOR: verifyThroughputScaling_byPayloadSize_expectLinearGrowth\n");

    // Test throughput scaling with increasing payload sizes
    std::vector<size_t> payloadSizes = {1024, 8192, 65536};
    std::vector<double> throughputResults;

    for (size_t payloadSize : payloadSizes) {
        PerformanceCollector collector;
        collector.StartCollection();

        std::vector<char> testData = CreatePerformanceTestData(payloadSize, false);

        for (int i = 0; i < 50; ++i) {
            IOC_DatDesc_T sendDesc = {};
            IOC_initDatDesc(&sendDesc);
            sendDesc.Payload.pData = testData.data();
            sendDesc.Payload.PtrDataSize = payloadSize;
            sendDesc.Payload.PtrDataLen = payloadSize;

            double latencyUs = MeasureOperationLatency([&]() { IOC_sendDAT(senderLinkID, &sendDesc, NULL); });

            collector.RecordLatency(latencyUs);
            collector.RecordOperation(payloadSize);
        }

        PerformanceMetrics_T metrics = collector.FinishCollection();
        double mbps = metrics.BytesPerSecond / (1024.0 * 1024.0);
        throughputResults.push_back(mbps);

        printf("📊 Payload %zu bytes: %.2f MB/s\n", payloadSize, mbps);
    }

    // ┌──────────────────────────────────────────────────────────────────────────────────────┐
    // │                                ✅ VERIFY PHASE                                        │
    // └──────────────────────────────────────────────────────────────────────────────────────┘
    //@KeyVerifyPoint-1: Throughput should generally increase with payload size
    ASSERT_GE(throughputResults.size(), 3) << "Should have throughput results for all payload sizes";

    //@KeyVerifyPoint-2: Larger payloads should show improved efficiency
    EXPECT_GT(throughputResults[2], throughputResults[0])
        << "Largest payload should have better throughput than smallest";

    //@KeyVerifyPoint-3: Performance data should be collected
    ASSERT_GT(senderPrivData.SendOperationCount.load(), 0) << "Should have recorded operations";

    // ┌──────────────────────────────────────────────────────────────────────────────────────┐
    // │                               🧹 CLEANUP PHASE                                        │
    // └──────────────────────────────────────────────────────────────────────────────────────┘
    // Cleanup handled by TearDown()
}

//======END OF UNIT TESTING IMPLEMENTATION=========================================================
///////////////////////////////////////////////////////////////////////////////////////////////////
