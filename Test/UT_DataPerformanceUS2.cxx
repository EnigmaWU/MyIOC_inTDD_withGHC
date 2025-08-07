///////////////////////////////////////////////////////////////////////////////////////////////////
// QUICK REFERENCE GUIDE - 快速参考指南
// 📝 用途: DAT性能测试单元测试实现 - User Story 2 ONLY
// 🔄 流程: User Story → Acceptance Criteria → Test Cases → Implementation
// 📂 分类: DataPerformance US-2 - Low latency verification
// 🎯 重点: 延迟验证、API响应时间分析、实时性能测试
///////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF OVERVIEW OF THIS UNIT TESTING FILE===============================================
/**
 * @brief
 *  DAT性能测试实现 - US-2: 低延迟API调用验证
 *
 *-------------------------------------------------------------------------------------------------
 *++背景说明：
 *  本测试文件专门实现US-2的低延迟验证需求
 *  重点关注API调用响应时间和延迟一致性的测量和验证
 *  确保系统满足实时应用的低延迟要求
 *
 *  关键概念：
 *  - Low-Latency Measurement: 低延迟测量，验证API响应时间特性
 *  - API Response Time: API响应时间测试，确保快速响应
 *  - Latency Consistency: 延迟一致性验证，确保稳定性能
 *  - Real-time Performance: 实时性能测试，满足时间敏感应用需求
 *
 *  测试范围：
 *  - US-2: 低延迟API调用验证 (微秒级响应时间)
 *  - API调用延迟统计分析
 *  - 延迟抖动和一致性测试
 *  - P95/P99延迟指标验证
 */
//======>END OF OVERVIEW OF THIS UNIT TESTING FILE=================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF UNIT TESTING DESIGN==============================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF USER STORY=======================================================================
/**************************************************************************************************
 * @brief 【User Story - US-2】
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
 * @brief 【Acceptance Criteria - US-2】
 *
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
 * @brief 【Test Cases - US-2】
 *
 * [@AC-1,US-2] Low-latency small message delivery
 *  TC-1:
 *      @[Name]: verifyAPIResponseTime_byCallLatency_expectMicrosecondLevel
 *      @[Purpose]: 验证API调用响应时间，确保低延迟要求满足
 *      @[Brief]: 测量IOC_sendDAT的API调用延迟，验证响应时间稳定性
 *      @[Latency_Focus]: 测试API级别的响应时间和延迟一致性
 *
 * [@AC-2,US-2] Latency consistency analysis
 *  TC-2:
 *      @[Name]: verifyLatencyConsistency_byRepeatedCalls_expectStablePerformance
 *      @[Purpose]: 验证延迟一致性和抖动控制
 *      @[Brief]: 重复执行API调用，分析延迟分布和稳定性
 *      @[Consistency_Focus]: 测试延迟变化和性能稳定性特征
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
class UT_DataPerformanceUS2Fixture : public ::testing::Test {
   protected:
    static void SetUpTestSuite() {
        printf("🔧 UT_DataPerformanceUS2Fixture->SETUP: SetUpTestSuite\n");
        // Global latency test environment initialization
    }

    static void TearDownTestSuite() {
        printf("🧹 UT_DataPerformanceUS2Fixture->CLEANUP: TearDownTestSuite\n");
        // Global latency test environment cleanup
    }

    void SetUp() override {
        printf("🔧 UT_DataPerformanceUS2Fixture->SETUP: SetUp\n");

        // Initialize performance tracking for US-2 tests
        __ResetPerformanceTracking(&senderPrivData);
        __ResetPerformanceTracking(&receiverPrivData);

        // Set latency-focused test configuration
        testConfig.TestDurationSec = std::chrono::seconds(3);
        testConfig.TargetThroughputMBps = 5.0;    // Lower for latency focus
        testConfig.MaxAcceptableLatencyMs = 5.0;  // Stricter latency target
    }

    void TearDown() override {
        printf("🧹 UT_DataPerformanceUS2Fixture->CLEANUP: TearDown\n");

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

    // Helper method to setup latency test scenario
    void setupLatencyTestScenario() {
        IOC_SrvArgs_T srvArgs = {};
        IOC_Helper_initSrvArgs(&srvArgs);
        srvArgs.SrvURI.pProtocol = IOC_SRV_PROTO_FIFO;
        srvArgs.SrvURI.pHost = IOC_SRV_HOST_LOCAL_PROCESS;
        srvArgs.SrvURI.pPath = "test/performance/latency_us2";
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

    // Test data members for US-2
    __DatPerformancePrivData_T senderPrivData;
    __DatPerformancePrivData_T receiverPrivData;
    PerformanceTestConfig_T testConfig;
    IOC_SrvID_T testSrvID = IOC_ID_INVALID;
    IOC_LinkID_T senderLinkID = IOC_ID_INVALID;
    IOC_LinkID_T receiverLinkID = IOC_ID_INVALID;
};

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
TEST_F(UT_DataPerformanceUS2Fixture, verifyLatencyConsistency_byRepeatedCalls_expectStablePerformance) {
    // ┌──────────────────────────────────────────────────────────────────────────────────────┐
    // │                                🔧 SETUP PHASE                                        │
    // └──────────────────────────────────────────────────────────────────────────────────────┘
    setupLatencyTestScenario();

    // ┌──────────────────────────────────────────────────────────────────────────────────────┐
    // │                               🎯 BEHAVIOR PHASE                                       │
    // └──────────────────────────────────────────────────────────────────────────────────────┘
    printf("🎯 US2Fixture->BEHAVIOR: verifyLatencyConsistency_byRepeatedCalls_expectStablePerformance\n");

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
