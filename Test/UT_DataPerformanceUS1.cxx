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
 *  DAT性能测试实现 - 验证IOC框架中DAT服务的性能特性
 *
 *-------------------------------------------------------------------------------------------------
 *++背景说明：
 *  本测试文件实现DAT(Data Transfer)服务的性能验证
 *  重点关注吞吐量、延迟等关键性能指标的测量和验证
 *  确保系统在各种负载条件下的性能表现符合预期
 *
 *  关键概念：
 *  - Throughput Testing: 吞吐量测试，验证数据传输速率
 *  - Latency Measurement: 延迟测量，验证响应时间特性
 *  - Performance Benchmarking: 性能基准测试
 *  - Resource Monitoring: 资源使用监控
 */
//======>END OF OVERVIEW OF THIS UNIT TESTING FILE=================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======BEGIN OF UNIT TESTING IMPLEMENTATION=======================================================

#include <gtest/gtest.h>

#include "UT_DataPerformance.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF TEST FIXTURE CLASS===============================================================

/**
 * @brief DAT性能测试夹具类
 *        为性能相关测试用例提供公共的设置和清理
 *        专门测试IOC框架的性能特性
 */
class DATPerformanceTest : public ::testing::Test {
   protected:
    void SetUp() override {
        // Initialize performance tracking
        __ResetPerformanceTracking(&senderPrivData);
        __ResetPerformanceTracking(&receiverPrivData);

        // Set default test configuration
        testConfig.TestDurationSec = std::chrono::seconds(5);
        testConfig.TargetThroughputMBps = 10.0;    // Conservative target for tests
        testConfig.MaxAcceptableLatencyMs = 10.0;  // Conservative latency target

        printf("🔧 [SETUP] DATPerformanceTest initialized\n");
    }

    void TearDown() override {
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

        printf("🧹 [TEARDOWN] DATPerformanceTest cleaned up\n");
    }

    // Helper method to setup performance test scenario
    void setupPerformanceTestScenario() {
        // Setup Service as DatSender
        IOC_SrvArgs_T srvArgs = {};
        IOC_Helper_initSrvArgs(&srvArgs);
        srvArgs.SrvURI.pProtocol = IOC_SRV_PROTO_FIFO;
        srvArgs.SrvURI.pHost = IOC_SRV_HOST_LOCAL_PROCESS;
        srvArgs.SrvURI.pPath = "test/performance/scenario";
        srvArgs.UsageCapabilites = IOC_LinkUsageDatSender;
        srvArgs.Flags = IOC_SRVFLAG_AUTO_ACCEPT;

        IOC_Result_T result = IOC_onlineService(&testSrvID, &srvArgs);
        ASSERT_EQ(IOC_RESULT_SUCCESS, result) << "Service setup failed";

        // Setup Client connection as DatReceiver
        IOC_ConnArgs_T connArgs = {};
        IOC_Helper_initConnArgs(&connArgs);
        connArgs.SrvURI = srvArgs.SrvURI;
        connArgs.Usage = IOC_LinkUsageDatReceiver;

        result = IOC_connectService(&receiverLinkID, &connArgs, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, result) << "Client connection setup failed";

        senderLinkID = receiverLinkID;  // Same link for bi-directional testing

        // Update state tracking
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

//======>END OF TEST FIXTURE CLASS=================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>US-1 AC-1 TESTS: High-throughput bulk data transfer======================================

/**
 * ╔══════════════════════════════════════════════════════════════════════════════════════════╗
 * ║                           🚀 BULK DATA THROUGHPUT VERIFICATION                          ║
 * ╠══════════════════════════════════════════════════════════════════════════════════════════╣
 * ║ @[Name]: verifyBulkDataThroughput_byLargePayloads_expectOptimalRates                    ║
 * ║ @[Purpose]: 验证大负载数据传输的吞吐量性能                                               ║
 * ║ @[Steps]: 使用1KB到1MB负载测试吞吐量，验证性能目标达成                                  ║
 * ║ @[Expect]: 吞吐量达到目标值，性能随负载大小合理扩展                                      ║
 * ║ @[Notes]: 这是性能测试的核心用例                                                        ║
 * ║                                                                                          ║
 * ║ 🎯 Throughput测试重点：                                                                 ║
 * ║   • 验证最大数据传输速率                                                                 ║
 * ║   • 确保性能随负载大小的合理扩展                                                         ║
 * ║   • 测试批量传输的效率                                                                   ║
 * ║ @[TestPattern]: US-1 AC-1 TC-1 - 高吞吐量数据传输验证                                  ║
 * ╚══════════════════════════════════════════════════════════════════════════════════════════╝
 */
TEST_F(DATPerformanceTest, verifyBulkDataThroughput_byLargePayloads_expectOptimalRates) {
    // ┌──────────────────────────────────────────────────────────────────────────────────────┐
    // │                                🔧 SETUP PHASE                                        │
    // └──────────────────────────────────────────────────────────────────────────────────────┘
    printf("🧪 [TEST] verifyBulkDataThroughput_byLargePayloads_expectOptimalRates\n");

    setupPerformanceTestScenario();

    ASSERT_TRUE(senderPrivData.LinkConnected.load()) << "Sender link should be connected";

    // ┌──────────────────────────────────────────────────────────────────────────────────────┐
    // │                               🎯 BEHAVIOR PHASE                                       │
    // └──────────────────────────────────────────────────────────────────────────────────────┘
    printf("🚀 [ACTION] Testing bulk data throughput with large payloads\n");

    PerformanceCollector collector;
    collector.StartCollection();

    // Test different payload sizes
    std::vector<size_t> payloadSizes = {1024, 4096, 16384, 65536, 262144};  // 1KB to 256KB

    for (size_t payloadSize : payloadSizes) {
        printf("📊 [PAYLOAD] Testing with %zu bytes\n", payloadSize);

        // Create test data
        std::vector<char> testData = CreatePerformanceTestData(payloadSize, false);

        // Measure throughput for this payload size
        auto startTime = std::chrono::high_resolution_clock::now();
        size_t iterations = 100;  // Fixed number for consistent testing

        for (size_t i = 0; i < iterations; ++i) {
            IOC_DatDesc_T sendDesc = {};
            IOC_initDatDesc(&sendDesc);
            sendDesc.Payload.pData = testData.data();
            sendDesc.Payload.PtrDataSize = payloadSize;
            sendDesc.Payload.PtrDataLen = payloadSize;

            auto opStart = std::chrono::high_resolution_clock::now();
            IOC_Result_T result = IOC_sendDAT(senderLinkID, &sendDesc, NULL);
            auto opEnd = std::chrono::high_resolution_clock::now();

            if (result == IOC_RESULT_SUCCESS) {
                double latencyUs = std::chrono::duration<double, std::micro>(opEnd - opStart).count();
                collector.RecordLatency(latencyUs);
                collector.RecordOperation(payloadSize);

                // Update private data tracking
                senderPrivData.SendOperationCount++;
                senderPrivData.TotalBytesSent += payloadSize;
                {
                    std::lock_guard<std::mutex> lock(senderPrivData.LatencyMutex);
                    senderPrivData.SendLatencies.push_back(latencyUs);
                }
            } else {
                collector.RecordError();
                printf("⚠️ [WARNING] IOC_sendDAT failed with result %d for iteration %zu\n", result, i);
            }

            // Small delay to prevent overwhelming the system
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }

        auto endTime = std::chrono::high_resolution_clock::now();
        double durationSec = std::chrono::duration<double>(endTime - startTime).count();

        // Calculate and display throughput for this payload size
        double mbps = (iterations * payloadSize) / (durationSec * 1024.0 * 1024.0);
        printf("📈 [RESULT] Payload %zu bytes: %.2f MB/s over %.3f seconds\n", payloadSize, mbps, durationSec);
    }

    // ┌──────────────────────────────────────────────────────────────────────────────────────┐
    // │                                ✅ VERIFY PHASE                                        │
    // └──────────────────────────────────────────────────────────────────────────────────────┘
    PerformanceMetrics_T metrics = collector.FinishCollection();

    // Print comprehensive performance report
    PrintPerformanceReport(metrics, "Bulk Data Throughput Test");

    // @KeyVerifyPoint-1: Overall throughput should meet target
    VERIFY_THROUGHPUT_TARGET(metrics, testConfig.TargetThroughputMBps);

    // @KeyVerifyPoint-2: Success rate should be high
    VERIFY_SUCCESS_RATE_TARGET(metrics, 0.95);  // 95% success rate

    // @KeyVerifyPoint-3: Latency should be reasonable
    VERIFY_LATENCY_TARGET(metrics, testConfig.MaxAcceptableLatencyMs);

    // @KeyVerifyPoint-4: Verify performance data was collected
    ASSERT_GT(senderPrivData.SendOperationCount.load(), 0) << "Should have recorded send operations";
    ASSERT_GT(senderPrivData.TotalBytesSent.load(), 0) << "Should have sent some data";

    printf("✅ [SUCCESS] Bulk data throughput test completed\n");
    printf("📊 [SUMMARY] Sent %zu bytes in %zu operations\n", senderPrivData.TotalBytesSent.load(),
           senderPrivData.SendOperationCount.load());

    // ┌──────────────────────────────────────────────────────────────────────────────────────┐
    // │                               🧹 CLEANUP PHASE                                        │
    // └──────────────────────────────────────────────────────────────────────────────────────┘
    // Cleanup handled by TearDown()
}

/**
 * ╔══════════════════════════════════════════════════════════════════════════════════════════╗
 * ║                          ⏱️ API RESPONSE TIME VERIFICATION                               ║
 * ╠══════════════════════════════════════════════════════════════════════════════════════════╣
 * ║ @[Name]: verifyAPIResponseTime_byCallLatency_expectMicrosecondLevel                     ║
 * ║ @[Purpose]: 验证API调用响应时间                                                         ║
 * ║ @[Steps]: 测量IOC_sendDAT/IOC_recvDAT的API调用延迟                                      ║
 * ║ @[Expect]: API延迟在微秒级别，性能稳定                                                   ║
 * ║ @[Notes]: 专门测试API级别的性能特性                                                     ║
 * ║                                                                                          ║
 * ║ 🎯 API性能测试重点：                                                                    ║
 * ║   • 验证API调用的响应时间                                                               ║
 * ║   • 确保低延迟要求的满足                                                                ║
 * ║   • 测试API性能的一致性                                                                 ║
 * ║ @[TestPattern]: US-2 AC-2 TC-2 - API响应时间验证                                      ║
 * ╚══════════════════════════════════════════════════════════════════════════════════════════╝
 */
TEST_F(DATPerformanceTest, verifyAPIResponseTime_byCallLatency_expectMicrosecondLevel) {
    // ┌──────────────────────────────────────────────────────────────────────────────────────┐
    // │                                🔧 SETUP PHASE                                        │
    // └──────────────────────────────────────────────────────────────────────────────────────┘
    printf("🧪 [TEST] verifyAPIResponseTime_byCallLatency_expectMicrosecondLevel\n");

    setupPerformanceTestScenario();

    // ┌──────────────────────────────────────────────────────────────────────────────────────┐
    // │                               🎯 BEHAVIOR PHASE                                       │
    // └──────────────────────────────────────────────────────────────────────────────────────┘
    printf("⏱️ [ACTION] Measuring API call latency for small messages\n");

    PerformanceCollector collector;
    collector.StartCollection();

    // Test small message latency
    const size_t messageSize = 256;  // Small message for latency testing
    std::vector<char> testData = CreatePerformanceTestData(messageSize, false);

    const size_t testIterations = 1000;
    std::vector<double> apiLatencies;

    for (size_t i = 0; i < testIterations; ++i) {
        IOC_DatDesc_T sendDesc = {};
        IOC_initDatDesc(&sendDesc);
        sendDesc.Payload.pData = testData.data();
        sendDesc.Payload.PtrDataSize = messageSize;
        sendDesc.Payload.PtrDataLen = messageSize;

        // Measure API call latency precisely
        double latencyUs = MeasureOperationLatency([&]() {
            IOC_Result_T result = IOC_sendDAT(senderLinkID, &sendDesc, NULL);
            if (result != IOC_RESULT_SUCCESS) {
                collector.RecordError();
            }
        });

        apiLatencies.push_back(latencyUs);
        collector.RecordLatency(latencyUs);
        collector.RecordOperation(messageSize);

        // Update performance tracking
        senderPrivData.SendOperationCount++;
        senderPrivData.TotalBytesSent += messageSize;
        {
            std::lock_guard<std::mutex> lock(senderPrivData.LatencyMutex);
            senderPrivData.SendLatencies.push_back(latencyUs);
        }

        // Brief pause to prevent overwhelming
        if (i % 100 == 0) {
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }
    }

    // ┌──────────────────────────────────────────────────────────────────────────────────────┐
    // │                                ✅ VERIFY PHASE                                        │
    // └──────────────────────────────────────────────────────────────────────────────────────┘
    PerformanceMetrics_T metrics = collector.FinishCollection();

    // Print detailed latency analysis
    printf("\n📊 [LATENCY ANALYSIS]\n");
    printf("├─ Iterations: %zu\n", testIterations);
    printf("├─ Message Size: %zu bytes\n", messageSize);
    printf("├─ Min Latency: %.2f μs\n", metrics.MinLatencyUs);
    printf("├─ Max Latency: %.2f μs\n", metrics.MaxLatencyUs);
    printf("├─ Avg Latency: %.2f μs\n", metrics.AvgLatencyUs);
    printf("├─ Median Latency: %.2f μs\n", metrics.MedianLatencyUs);
    printf("├─ P95 Latency: %.2f μs\n", metrics.P95LatencyUs);
    printf("├─ P99 Latency: %.2f μs\n", metrics.P99LatencyUs);
    printf("└─ Jitter (StdDev): %.2f μs\n", metrics.JitterUs);

    // @KeyVerifyPoint-1: Average latency should be low
    double avgLatencyMs = metrics.AvgLatencyUs / 1000.0;
    EXPECT_LE(avgLatencyMs, testConfig.MaxAcceptableLatencyMs)
        << "Average API latency too high: " << avgLatencyMs << " ms";

    // @KeyVerifyPoint-2: P99 latency should be reasonable
    double p99LatencyMs = metrics.P99LatencyUs / 1000.0;
    EXPECT_LE(p99LatencyMs, testConfig.MaxAcceptableLatencyMs * 2)
        << "P99 API latency too high: " << p99LatencyMs << " ms";

    // @KeyVerifyPoint-3: Jitter should be minimal
    double jitterMs = metrics.JitterUs / 1000.0;
    EXPECT_LE(jitterMs, testConfig.MaxAcceptableLatencyMs) << "API latency jitter too high: " << jitterMs << " ms";

    // @KeyVerifyPoint-4: Success rate should be very high for API calls
    VERIFY_SUCCESS_RATE_TARGET(metrics, 0.99);  // 99% success rate expected

    printf("✅ [SUCCESS] API response time test completed\n");
    printf("📊 [SUMMARY] %zu API calls with avg latency %.2f μs\n", testIterations, metrics.AvgLatencyUs);

    // ┌──────────────────────────────────────────────────────────────────────────────────────┐
    // │                               🧹 CLEANUP PHASE                                        │
    // └──────────────────────────────────────────────────────────────────────────────────────┘
    // Cleanup handled by TearDown()
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>END OF US-1&2 IMPLEMENTATION==============================================================
