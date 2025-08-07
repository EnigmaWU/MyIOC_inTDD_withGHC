///////////////////////////////////////////////////////////////////////////////////////////////////
// QUICK REFERENCE GUIDE - 快速参考指南
// 📝 用途: DAT性能测试单元测试实现 - User Story 1 & 2
// 🔄 流程: User Story → Acceptance Criteria → Test Cases → Implementation
// 📂 分类: DataPerformance US-1&2 - High throughput and low latency verification
// 🎯 重点: 吞吐量验证、延迟测量、API性能分析
///////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF OVERVIEW OF THIS UNIT TESTING FILE===============================================
/**
 * @brief
 *  DAT性能测试实现 - 验证IOC框架中DAT服务的高吞吐量和低延迟性能特性
 *
 *-------------------------------------------------------------------------------------------------
 *++背景说明：
 *  本测试文件实现DAT(Data Transfer)服务的性能验证，专注于US-1和US-2的核心性能需求
 *  重点关注吞吐量、延迟等关键性能指标的测量和验证
 *  确保系统在各种负载条件下的性能表现符合预期
 *
 *  关键概念：
 *  - High-Throughput Testing: 高吞吐量测试，验证大负载数据传输速率
 *  - Low-Latency Measurement: 低延迟测量，验证API响应时间特性
 *  - Performance Benchmarking: 性能基准测试，建立性能基线
 *  - Resource Monitoring: 资源使用监控，确保效率优化
 *
 *  测试范围：
 *  - US-1: 高吞吐量数据传输验证 (1KB到256KB负载)
 *  - US-2: 低延迟API调用验证 (微秒级响应时间)
 *  - 性能指标收集和分析
 *  - 成功率和错误率统计
 */
//======>END OF OVERVIEW OF THIS UNIT TESTING FILE=================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF UNIT TESTING DESIGN==============================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF USER STORY=======================================================================
/**************************************************************************************************
 * @brief 【User Story】
 *
 *  US-1: AS a high-throughput DAT application developer,
 *    I WANT to verify that IOC_sendDAT operations achieve optimal throughput,
 *   SO THAT I can ensure maximum data transfer rates under various payload sizes
 *      AND validate batch transfer efficiency for bulk data operations,
 *      AND implement high-performance data streaming solutions.
 *
 *  US-2: AS a low-latency DAT application developer,
 *    I WANT to verify that DAT operations maintain minimal end-to-end latency,
 *   SO THAT I can ensure real-time data delivery requirements are met
 *      AND validate API call response times are within acceptable limits,
 *      AND implement time-critical data communication systems.
 *
 *************************************************************************************************/
//======>END OF USER STORY=========================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//=======>BEGIN OF ACCEPTANCE CRITERIA=============================================================
/**************************************************************************************************
 * @brief 【Acceptance Criteria】
 *
 * [@US-1] High-throughput DAT operations verification
 *  AC-1: GIVEN a DAT link configured for bulk data transfer,
 *         WHEN sending large payloads (1KB to 256KB) repeatedly,
 *         THEN throughput should achieve target rates (e.g., >10MB/s for test environment)
 *              AND throughput should scale with payload size efficiently
 *              AND bulk transfer operations should maintain consistent performance.
 *
 *  AC-2: GIVEN multiple payload sizes for throughput testing,
 *         WHEN measuring transfer rates across different data volumes,
 *         THEN larger payloads should demonstrate improved efficiency
 *              AND performance metrics should be accurately collected
 *              AND success rate should remain above 95%.
 *
 *---------------------------------------------------------------------------------------------------
 * [@US-2] Low-latency DAT operations verification
 *  AC-1: GIVEN a DAT link optimized for minimal latency,
 *         WHEN sending small messages (256 bytes) with immediate delivery,
 *         THEN end-to-end latency should be within target limits (e.g., <10ms for test)
 *              AND latency should be consistent across message iterations
 *              AND jitter should be minimal for stable performance.
 *
 *  AC-2: GIVEN IOC_sendDAT API calls for latency measurement,
 *         WHEN measuring API call response times precisely,
 *         THEN API latency should be minimal and predictable
 *              AND P95/P99 latencies should be within acceptable bounds
 *              AND success rate should be very high (>99%).
 */
//=======>END OF ACCEPTANCE CRITERIA================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF TEST CASES=======================================================================
/**************************************************************************************************
 * @brief 【Test Cases】
 *
 * [@AC-1,US-1] High-throughput bulk data transfer
 *  TC-1:
 *      @[Name]: verifyBulkDataThroughput_byLargePayloads_expectOptimalRates
 *      @[Purpose]: 验证大负载数据传输的吞吐量性能，确保达到目标传输速率
 *      @[Brief]: 使用1KB到256KB不同负载大小测试吞吐量，验证性能随负载扩展
 *      @[Throughput_Focus]: 测试最大数据传输速率和负载大小对性能的影响
 *
 * [@AC-2,US-1] Throughput performance metrics validation
 *  TC-2:
 *      @[Name]: verifyThroughputMetrics_byPerformanceCollection_expectAccurateAnalysis
 *      @[Purpose]: 验证性能指标收集的准确性和完整性
 *      @[Brief]: 收集并分析吞吐量测试过程中的各项性能数据
 *      @[Metrics_Focus]: 测试性能数据收集和统计分析的正确性
 *
 *---------------------------------------------------------------------------------------------------
 * [@AC-1,US-2] Low-latency small message delivery
 *  TC-1:
 *      @[Name]: verifyAPIResponseTime_byCallLatency_expectMicrosecondLevel
 *      @[Purpose]: 验证API调用响应时间，确保低延迟要求满足
 *      @[Brief]: 测量IOC_sendDAT的API调用延迟，验证响应时间稳定性
 *      @[Latency_Focus]: 测试API级别的响应时间和延迟一致性
 *
 * [@AC-2,US-2] Latency statistical analysis
 *  TC-2:
 *      @[Name]: verifyLatencyDistribution_byStatisticalAnalysis_expectConsistentPerformance
 *      @[Purpose]: 验证延迟分布特性和统计指标
 *      @[Brief]: 分析P95、P99延迟和抖动，验证性能稳定性
 *      @[Statistics_Focus]: 测试延迟统计分析和性能一致性验证
 *
 *************************************************************************************************/
//======>END OF TEST CASES=========================================================================
//======>END OF UNIT TESTING DESIGN================================================================

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

    // Service setup
    IOC_SrvArgs_T srvArgs = {};
    IOC_Helper_initSrvArgs(&srvArgs);
    srvArgs.SrvURI.pProtocol = IOC_SRV_PROTO_FIFO;
    srvArgs.SrvURI.pHost = IOC_SRV_HOST_LOCAL_PROCESS;
    srvArgs.SrvURI.pPath = "test/performance/throughput";
    srvArgs.UsageCapabilites = IOC_LinkUsageDatSender;
    srvArgs.Flags = IOC_SRVFLAG_AUTO_ACCEPT;

    IOC_Result_T result = IOC_onlineService(&testSrvID, &srvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result) << "Service setup failed";

    // Client connection setup
    IOC_ConnArgs_T connArgs = {};
    IOC_Helper_initConnArgs(&connArgs);
    connArgs.SrvURI = srvArgs.SrvURI;
    connArgs.Usage = IOC_LinkUsageDatReceiver;

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

/**
 * ╔══════════════════════════════════════════════════════════════════════════════════════════╗
 * ║                          ⏱️ API RESPONSE TIME VERIFICATION                               ║
 * ╠══════════════════════════════════════════════════════════════════════════════════════════╣
 * ║ @[Name]: verifyAPIResponseTime_byCallLatency_expectMicrosecondLevel                     ║
 * ║ @[Steps]: 🔧 setup low-latency test environment → 🎯 execute many small API calls       ║
 * ║          → ✅ analyze latency statistics → 🧹 cleanup resources                         ║
 * ║ @[Expect]: Avg latency <10ms, P99 <20ms, success rate >99%                             ║
 * ║ @[Notes]: API-level performance test ensuring real-time application requirements       ║
 * ╚══════════════════════════════════════════════════════════════════════════════════════════╝
 */
TEST(UT_DataPerformance, verifyAPIResponseTime_byCallLatency_expectMicrosecondLevel) {
    // ┌──────────────────────────────────────────────────────────────────────────────────────┐
    // │                                🔧 SETUP PHASE                                        │
    // └──────────────────────────────────────────────────────────────────────────────────────┘
    printf("🧪 [TEST] verifyAPIResponseTime_byCallLatency_expectMicrosecondLevel\n");

    // Setup performance test configuration for latency testing
    PerformanceTestConfig_T testConfig;
    testConfig.MaxAcceptableLatencyMs = 10.0;

    // Setup test service and connections
    IOC_SrvID_T testSrvID = IOC_ID_INVALID;
    IOC_LinkID_T senderLinkID = IOC_ID_INVALID;

    IOC_SrvArgs_T srvArgs = {};
    IOC_Helper_initSrvArgs(&srvArgs);
    srvArgs.SrvURI.pProtocol = IOC_SRV_PROTO_FIFO;
    srvArgs.SrvURI.pHost = IOC_SRV_HOST_LOCAL_PROCESS;
    srvArgs.SrvURI.pPath = "test/performance/latency";
    srvArgs.UsageCapabilites = IOC_LinkUsageDatSender;
    srvArgs.Flags = IOC_SRVFLAG_AUTO_ACCEPT;

    IOC_Result_T result = IOC_onlineService(&testSrvID, &srvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result) << "Service setup failed";

    IOC_ConnArgs_T connArgs = {};
    IOC_Helper_initConnArgs(&connArgs);
    connArgs.SrvURI = srvArgs.SrvURI;
    connArgs.Usage = IOC_LinkUsageDatReceiver;

    result = IOC_connectService(&senderLinkID, &connArgs, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result) << "Client connection setup failed";

    // ┌──────────────────────────────────────────────────────────────────────────────────────┐
    // │                               🎯 BEHAVIOR PHASE                                       │
    // └──────────────────────────────────────────────────────────────────────────────────────┘
    printf("🎯 BEHAVIOR: Measuring API call latency for small messages\n");

    PerformanceCollector collector;
    collector.StartCollection();

    const size_t messageSize = 256;
    const size_t testIterations = 1000;
    std::vector<char> testData = CreatePerformanceTestData(messageSize, false);

    for (size_t i = 0; i < testIterations; ++i) {
        IOC_DatDesc_T sendDesc = {};
        IOC_initDatDesc(&sendDesc);
        sendDesc.Payload.pData = testData.data();
        sendDesc.Payload.PtrDataSize = messageSize;
        sendDesc.Payload.PtrDataLen = messageSize;

        double latencyUs = MeasureOperationLatency([&]() {
            IOC_Result_T sendResult = IOC_sendDAT(senderLinkID, &sendDesc, NULL);
            if (sendResult != IOC_RESULT_SUCCESS) {
                collector.RecordError();
            }
        });

        collector.RecordLatency(latencyUs);
        collector.RecordOperation(messageSize);

        if (i % 100 == 0) {
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }
    }

    // ┌──────────────────────────────────────────────────────────────────────────────────────┐
    // │                                ✅ VERIFY PHASE                                        │
    // └──────────────────────────────────────────────────────────────────────────────────────┘
    PerformanceMetrics_T metrics = collector.FinishCollection();

    printf("\n📊 [LATENCY ANALYSIS]\n");
    printf("├─ Iterations: %zu\n", testIterations);
    printf("├─ Avg Latency: %.2f μs\n", metrics.AvgLatencyUs);
    printf("├─ P95 Latency: %.2f μs\n", metrics.P95LatencyUs);
    printf("└─ P99 Latency: %.2f μs\n", metrics.P99LatencyUs);

    //@KeyVerifyPoint-1: Average latency should be low
    VERIFY_LATENCY_TARGET(metrics, testConfig.MaxAcceptableLatencyMs);

    //@KeyVerifyPoint-2: Success rate should be very high
    VERIFY_SUCCESS_RATE_TARGET(metrics, 0.99);

    //@KeyVerifyPoint-3: Jitter should be minimal
    double jitterMs = metrics.JitterUs / 1000.0;
    EXPECT_LE(jitterMs, testConfig.MaxAcceptableLatencyMs) << "API latency jitter too high: " << jitterMs << " ms";

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
class UT_DataPerformanceFixture : public ::testing::Test {
   protected:
    static void SetUpTestSuite() {
        printf("🔧 UT_DataPerformanceFixture->SETUP: SetUpTestSuite\n");
        // Global performance test environment initialization
    }

    static void TearDownTestSuite() {
        printf("🧹 UT_DataPerformanceFixture->CLEANUP: TearDownTestSuite\n");
        // Global performance test environment cleanup
    }

    void SetUp() override {
        printf("🔧 UT_DataPerformanceFixture->SETUP: SetUp\n");

        // Initialize performance tracking for fixture tests
        __ResetPerformanceTracking(&senderPrivData);
        __ResetPerformanceTracking(&receiverPrivData);

        // Set conservative test configuration
        testConfig.TestDurationSec = std::chrono::seconds(5);
        testConfig.TargetThroughputMBps = 10.0;
        testConfig.MaxAcceptableLatencyMs = 10.0;
    }

    void TearDown() override {
        printf("🧹 UT_DataPerformanceFixture->CLEANUP: TearDown\n");

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

    // Helper method to setup performance test scenario
    void setupPerformanceTestScenario() {
        IOC_SrvArgs_T srvArgs = {};
        IOC_Helper_initSrvArgs(&srvArgs);
        srvArgs.SrvURI.pProtocol = IOC_SRV_PROTO_FIFO;
        srvArgs.SrvURI.pHost = IOC_SRV_HOST_LOCAL_PROCESS;
        srvArgs.SrvURI.pPath = "test/performance/fixture";
        srvArgs.UsageCapabilites = IOC_LinkUsageDatSender;
        srvArgs.Flags = IOC_SRVFLAG_AUTO_ACCEPT;

        IOC_Result_T result = IOC_onlineService(&testSrvID, &srvArgs);
        ASSERT_EQ(IOC_RESULT_SUCCESS, result) << "Service setup failed";

        IOC_ConnArgs_T connArgs = {};
        IOC_Helper_initConnArgs(&connArgs);
        connArgs.SrvURI = srvArgs.SrvURI;
        connArgs.Usage = IOC_LinkUsageDatReceiver;

        result = IOC_connectService(&receiverLinkID, &connArgs, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, result) << "Client connection setup failed";

        senderLinkID = receiverLinkID;  // Same link for bi-directional testing
        senderPrivData.ServiceOnline = true;
        senderPrivData.LinkConnected = true;
        receiverPrivData.LinkConnected = true;
    }

    // Test data members
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
TEST_F(UT_DataPerformanceFixture, verifyThroughputScaling_byPayloadSize_expectLinearGrowth) {
    // ┌──────────────────────────────────────────────────────────────────────────────────────┐
    // │                                🔧 SETUP PHASE                                        │
    // └──────────────────────────────────────────────────────────────────────────────────────┘
    setupPerformanceTestScenario();

    // ┌──────────────────────────────────────────────────────────────────────────────────────┐
    // │                               🎯 BEHAVIOR PHASE                                       │
    // └──────────────────────────────────────────────────────────────────────────────────────┘
    printf("🎯 DataPerformanceFixture->BEHAVIOR: verifyThroughputScaling_byPayloadSize_expectLinearGrowth\n");

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

/**
 * ╔══════════════════════════════════════════════════════════════════════════════════════════╗
 * ║                        🔄 LATENCY CONSISTENCY VERIFICATION                              ║
 * ╠══════════════════════════════════════════════════════════════════════════════════════════╣
 * ║ @[Name]: verifyLatencyConsistency_byRepeatedCalls_expectStablePerformance              ║
 * ║ @[Steps]: 🔧 setup fixture environment → 🎯 execute repeated operations                 ║
 * ║          → ✅ verify latency consistency → 🧹 fixture cleanup                           ║
 * ║ @[Expect]: Latency within bounds, low jitter, all operations recorded                  ║
 * ║ @[Notes]: Fixture-based test ensuring consistent latency performance                   ║
 * ╚══════════════════════════════════════════════════════════════════════════════════════════╝
 */
TEST_F(UT_DataPerformanceFixture, verifyLatencyConsistency_byRepeatedCalls_expectStablePerformance) {
    // ┌──────────────────────────────────────────────────────────────────────────────────────┐
    // │                                🔧 SETUP PHASE                                        │
    // └──────────────────────────────────────────────────────────────────────────────────────┘
    setupPerformanceTestScenario();

    // ┌──────────────────────────────────────────────────────────────────────────────────────┐
    // │                               🎯 BEHAVIOR PHASE                                       │
    // └──────────────────────────────────────────────────────────────────────────────────────┘
    printf("🎯 DataPerformanceFixture->BEHAVIOR: verifyLatencyConsistency_byRepeatedCalls_expectStablePerformance\n");

    PerformanceCollector collector;
    collector.StartCollection();

    const size_t messageSize = 512;
    const size_t iterations = 200;
    std::vector<char> testData = CreatePerformanceTestData(messageSize, false);

    for (size_t i = 0; i < iterations; ++i) {
        IOC_DatDesc_T sendDesc = {};
        IOC_initDatDesc(&sendDesc);
        sendDesc.Payload.pData = testData.data();
        sendDesc.Payload.PtrDataSize = messageSize;
        sendDesc.Payload.PtrDataLen = messageSize;

        double latencyUs = MeasureOperationLatency([&]() { IOC_sendDAT(senderLinkID, &sendDesc, NULL); });

        collector.RecordLatency(latencyUs);
        collector.RecordOperation(messageSize);

        // Track in private data
        senderPrivData.SendOperationCount++;
        senderPrivData.TotalBytesSent += messageSize;
    }

    // ┌──────────────────────────────────────────────────────────────────────────────────────┐
    // │                                ✅ VERIFY PHASE                                        │
    // └──────────────────────────────────────────────────────────────────────────────────────┘
    PerformanceMetrics_T metrics = collector.FinishCollection();

    //@KeyVerifyPoint-1: Latency should be within acceptable bounds
    VERIFY_LATENCY_TARGET(metrics, testConfig.MaxAcceptableLatencyMs);

    //@KeyVerifyPoint-2: Jitter should be reasonable (low variability)
    double jitterMs = metrics.JitterUs / 1000.0;
    EXPECT_LE(jitterMs, testConfig.MaxAcceptableLatencyMs * 0.5)
        << "Latency jitter should be less than half the target latency";

    //@KeyVerifyPoint-3: All operations should be recorded
    ASSERT_EQ(senderPrivData.SendOperationCount.load(), iterations) << "Should have recorded all send operations";

    // ┌──────────────────────────────────────────────────────────────────────────────────────┐
    // │                               🧹 CLEANUP PHASE                                        │
    // └──────────────────────────────────────────────────────────────────────────────────────┘
    // Cleanup handled by TearDown()
}

//======END OF UNIT TESTING IMPLEMENTATION=========================================================
///////////////////////////////////////////////////////////////////////////////////////////////////
