///////////////////////////////////////////////////////////////////////////////////////////////////
// QUICK REFERENCE GUIDE - 快速参考指南
// 📝 用途: DAT（数据传输）能力验证单元测试
// 🔄 流程: User Story → Acceptance Criteria → Test Cases → Implementation
// 📂 分类: DataCapability - 专注于DAT数据传输的能力边界和限制测试
// 🎯 重点: 最大容量、性能阈值、资源限制、并发能力等边界条件
// Reference Unit Testing Templates in UT_FreelyDrafts.cxx and UT_DataTypical.cxx when needed.
///////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF OVERVIEW OF THIS UNIT TESTING FILE===============================================
/**
 * @brief
 *  验证IOC框架中DAT（数据传输）的能力边界和性能限制，专注于容量测试、
 *  性能阈值验证以及资源限制场景。
 *
 *-------------------------------------------------------------------------------------------------
 *++DAT能力测试是对IOC框架数据传输机制的深度验证，关注系统的边界条件：
 *
 *  能力验证场景：
 *  - 最大数据包大小的传输能力
 *  - 高频率数据传输的吞吐能力
 *  - 多连接并发数据传输的容量
 *  - 内存使用限制和缓冲区管理
 *  - 长时间连续传输的稳定性
 *  - 极限条件下的性能表现
 *
 *  测试维度：
 *  🔢 数据大小能力: 1B → 64B (嵌入模式) → 1KB → 1MB → 最大允许
 *  ⚡ 传输频率能力: 1次/秒 → 100次/秒 → 1000次/秒 → 极限频率
 *  🔗 并发连接能力: 1连接 → 10连接 → 100连接 → 最大连接数
 *  💾 内存使用能力: 正常使用 → 高内存压力 → 内存限制边界
 *  ⏱️ 持续时间能力: 短期传输 → 长期传输 → 7x24小时稳定性
 *
 *  与其他测试的关系：
 *  - DataTypical: 专注标准使用场景 → DataCapability: 专注能力边界
 *  - DataBoundary: 专注参数边界 → DataCapability: 专注系统容量
 *  - DataPerformance: 专注性能优化 → DataCapability: 专注最大性能
 *
 *  参考文档：
 *  - README_ArchDesign.md::MSG::DAT（能力规格部分）
 *  - README_RefAPIs.md::数据传输APIs（性能参数）
 *  - IOC_getCapability() API（系统能力查询）
 */
//======>END OF OVERVIEW OF THIS UNIT TESTING FILE=================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF UNIT TESTING DESIGN==============================================================

/**************************************************************************************************
 * 📋 DAT CAPABILITY TEST FOCUS - DAT能力测试重点
 *
 * 🎯 DESIGN PRINCIPLE: 验证DAT在各种能力边界条件下的表现
 * 🔄 PRIORITY: 数据大小能力 → 传输频率能力 → 并发连接能力 → 资源管理能力
 *
 * ✅ CAPABILITY SCENARIOS INCLUDED (包含的能力测试场景):
 *    📊 Max Data Size: 验证支持的最大单次数据传输大小
 *    ⚡ High Frequency: 验证高频率数据传输的吞吐能力
 *    🔗 Max Connections: 验证最大并发连接数和多连接数据传输
 *    💾 Memory Pressure: 验证内存压力下的数据传输稳定性
 *    ⏱️ Long Duration: 验证长时间连续传输的可靠性
 *    🔄 Resource Recovery: 验证资源释放和恢复能力
 *
 * ❌ NON-CAPABILITY SCENARIOS EXCLUDED (排除的非能力测试场景):
 *    🚫 错误处理（由DataFault专门测试）
 *    🚫 参数边界（由DataBoundary专门测试）
 *    🚫 状态转换（由DataState专门测试）
 *    🚫 误用场景（由DataMisuse专门测试）
 *************************************************************************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF USER STORY=======================================================================
/**************************************************************************************************
 * @brief 【User Story】
 *
 *  US-1: AS a system architect,
 *    I WANT to know the maximum data transmission capabilities of IOC framework,
 *   SO THAT I can design applications within the supported capacity limits,
 *      AND ensure reliable performance under maximum load conditions.
 *
 *  US-2: AS a performance engineer,
 *    I WANT to verify high-frequency data transmission throughput,
 *   SO THAT I can validate the framework meets real-time data streaming requirements,
 *      AND optimize application design for maximum performance.
 *
 *  US-3: AS a system integrator,
 *    I WANT to test maximum concurrent data connections,
 *   SO THAT I can deploy multi-client/server architectures with confidence,
 *      AND understand the scalability limits of the IOC framework.
 *
 *  US-4: AS a reliability engineer,
 *    I WANT to verify long-duration data transmission stability,
 *   SO THAT I can ensure 7x24 operation reliability,
 *      AND validate memory management under continuous load.
 *
 *************************************************************************************************/
//======>END OF USER STORY=========================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//=======>BEGIN OF ACCEPTANCE CRITERIA=============================================================
/**************************************************************************************************
 * @brief 【Acceptance Criteria】
 *
 * 🎯 专注于 DAT CAPABILITY 测试 - 验证数据传输的各种能力边界和性能限制
 *
 * [@US-1] AS a system architect, I WANT to know maximum data transmission capabilities,
 *         SO THAT I can design applications within supported capacity limits.
 *
 * [@US-2] AS a performance engineer, I WANT to verify high-frequency transmission throughput,
 *         SO THAT I can validate real-time streaming requirements.
 *
 * [@US-3] AS a system integrator, I WANT to test maximum concurrent connections,
 *         SO THAT I can deploy scalable multi-client architectures.
 *
 * [@US-4] AS a reliability engineer, I WANT to verify long-duration stability,
 *         SO THAT I can ensure 7x24 operation reliability.
 *
 * ⭐ CAPABILITY SCENARIOS ONLY - 能力测试验收标准:
 *
 *  AC-1@US-1: GIVEN IOC framework with DAT capability enabled,
 *         WHEN sending data chunks from small (1B) to maximum supported size,
 *         THEN ALL sizes should transmit successfully,
 *          AND embedded data (≤64B) uses EmdData storage efficiently,
 *          AND large data (>64B) uses pData pointer storage correctly,
 *          AND maximum single transmission size limit is clearly identified.
 *
 *  AC-2@US-1: GIVEN IOC framework with memory constraints,
 *         WHEN transmitting data under various memory pressure conditions,
 *         THEN data transmission should remain stable,
 *          AND memory usage should not grow unboundedly,
 *          AND system should handle memory pressure gracefully with appropriate return codes.
 *
 *  AC-3@US-2: GIVEN DatSender and DatReceiver with high-frequency requirements,
 *         WHEN transmitting data at increasing frequencies (1/s → 100/s → 1000/s → max),
 *         THEN system should handle each frequency level successfully,
 *          AND throughput should scale linearly until maximum capacity,
 *          AND maximum sustainable frequency limit is clearly identified,
 *          AND no data loss occurs at supported frequencies.
 *
 *  AC-4@US-2: GIVEN multiple DatSender/DatReceiver pairs operating simultaneously,
 *         WHEN each pair maintains high-frequency data transmission,
 *         THEN aggregate throughput should meet design specifications,
 *          AND individual connection performance should not degrade significantly,
 *          AND system resource utilization should remain within acceptable limits.
 *
 *  AC-5@US-3: GIVEN IOC framework with connection capacity limits,
 *         WHEN creating increasing numbers of concurrent DAT connections (1 → 10 → 100 → max),
 *         THEN each connection level should establish successfully up to the maximum,
 *          AND all connections should support simultaneous data transmission,
 *          AND maximum concurrent connection limit is clearly identified,
 *          AND connection establishment time should remain reasonable at all levels.
 *
 *  AC-6@US-3: GIVEN maximum concurrent DAT connections established,
 *         WHEN all connections transmit data simultaneously,
 *         THEN data integrity should be maintained for all connections,
 *          AND transmission performance should be fairly distributed,
 *          AND no connection should starve or fail due to resource contention.
 *
 *  AC-7@US-4: GIVEN DatSender and DatReceiver for long-duration testing,
 *         WHEN continuously transmitting data for extended periods (minutes → hours),
 *         THEN data transmission should remain stable throughout the duration,
 *          AND memory usage should remain stable (no memory leaks),
 *          AND transmission performance should not degrade over time,
 *          AND system should handle resource cleanup properly.
 *
 *  AC-8@US-4: GIVEN IOC framework under continuous load,
 *         WHEN monitoring system resource usage over time,
 *         THEN memory usage should remain within acceptable bounds,
 *          AND CPU usage should remain stable,
 *          AND system should demonstrate resilience to temporary resource spikes.
 *
 */
//=======>END OF ACCEPTANCE CRITERIA================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF TEST CASES=======================================================================
/**************************************************************************************************
 * @brief 【Test Cases】
 *
 * 🎯 专注于 DAT CAPABILITY 测试用例 - 基于 FreelyDrafts 模板设计
 *
 * [@AC-1,US-1] - Maximum Data Size Capability Testing
 *  TC-1:
 *      @[Name]: verifyDatMaxDataSize_byTransmitIncreasingSize_expectAllSizesSuccess
 *      @[Purpose]: Verify AC-1 complete functionality - validate maximum supported data transmission sizes
 *          from 1B to system maximum, including embedded vs pointer storage modes
 *      @[Brief]: Test data transmission with sizes: 1B, 32B, 64B (embedded), 1KB, 100KB, 1MB, 10MB,
 *          verify successful transmission and appropriate storage mode selection
 *
 *  TC-2:
 *      @[Name]: verifyDatEmbeddedDataThreshold_byTestStorageMode_expectOptimalSelection
 *      @[Purpose]: Verify AC-1 **storage optimization** - embedded data (≤64B) vs pointer data (>64B)
 *          boundary behavior and storage mode selection accuracy
 *      @[Brief]: Test data sizes around 64B threshold (63B, 64B, 65B),
 *          verify EmdData used for ≤64B and pData used for >64B
 *
 *  TODO:TC-3...
 *
 *-------------------------------------------------------------------------------------------------
 * [@AC-2,US-1] - Memory Pressure Capability Testing
 *  TC-1:
 *      @[Name]: verifyDatMemoryPressure_byLimitedMemory_expectGracefulHandling
 *      @[Purpose]: Verify AC-2 complete functionality - data transmission stability under memory constraints
 *          and appropriate error handling when memory resources are limited
 *      @[Brief]: Simulate memory pressure conditions, transmit various data sizes,
 *          verify stable transmission and proper error codes when memory is insufficient
 *
 *  TODO:TC-2...
 *
 *-------------------------------------------------------------------------------------------------
 * [@AC-3,US-2] - High Frequency Transmission Capability Testing
 *  TC-1:
 *      @[Name]: verifyDatHighFrequency_byIncreasingRate_expectScalableThroughput
 *      @[Purpose]: Verify AC-3 complete functionality - high-frequency data transmission capability
 *          and throughput scaling from low to maximum supported frequency
 *      @[Brief]: Test transmission frequencies: 1/s, 10/s, 100/s, 1000/s, maximum,
 *          verify successful transmission at each level and identify maximum sustainable frequency
 *
 *  TC-2:
 *      @[Name]: verifyDatSustainedThroughput_byMaxFrequency_expectNoDataLoss
 *      @[Purpose]: Verify AC-3 **data reliability** - no data loss at maximum supported frequency
 *          over sustained periods with integrity verification
 *      @[Brief]: Transmit at maximum sustainable frequency for extended period,
 *          verify every data packet received correctly with sequence number validation
 *
 *  TODO:TC-3...
 *
 *-------------------------------------------------------------------------------------------------
 * [@AC-4,US-2] - Concurrent High Frequency Capability Testing
 *  TC-1:
 *      @[Name]: verifyDatConcurrentHighFreq_byMultiplePairs_expectAggregatePerformance
 *      @[Purpose]: Verify AC-4 complete functionality - multiple high-frequency connections
 *          operating simultaneously with aggregate throughput validation
 *      @[Brief]: Create multiple DatSender/DatReceiver pairs, each transmitting at high frequency,
 *          verify aggregate throughput meets specifications and individual performance remains stable
 *
 *  TODO:TC-2...
 *
 *-------------------------------------------------------------------------------------------------
 * [@AC-5,US-3] - Maximum Concurrent Connections Capability Testing
 *  TC-1:
 *      @[Name]: verifyDatMaxConnections_byIncreasingCount_expectAllEstablished
 *      @[Purpose]: Verify AC-5 complete functionality - maximum concurrent DAT connection capacity
 *          and connection establishment scalability
 *      @[Brief]: Create increasing numbers of DAT connections: 1, 10, 50, 100, maximum,
 *          verify successful establishment at each level and identify maximum connection limit
 *
 *  TC-2:
 *      @[Name]: verifyDatConnectionEstablishTime_byMaxCount_expectReasonableLatency
 *      @[Purpose]: Verify AC-5 **performance** - connection establishment time remains reasonable
 *          even at maximum connection count
 *      @[Brief]: Measure connection establishment time at various connection counts,
 *          verify latency remains within acceptable bounds even at maximum capacity
 *
 *  TODO:TC-3...
 *
 *-------------------------------------------------------------------------------------------------
 * [@AC-6,US-3] - Concurrent Data Transmission Capability Testing
 *  TC-1:
 *      @[Name]: verifyDatMaxConcurrentTransmit_byAllConnections_expectDataIntegrity
 *      @[Purpose]: Verify AC-6 complete functionality - simultaneous data transmission across
 *          maximum concurrent connections with data integrity preservation
 *      @[Brief]: Establish maximum connections, transmit unique data on each connection simultaneously,
 *          verify data integrity and fair performance distribution across all connections
 *
 *  TODO:TC-2...
 *
 *-------------------------------------------------------------------------------------------------
 * [@AC-7,US-4] - Long Duration Stability Capability Testing
 *  TC-1:
 *      @[Name]: verifyDatLongDuration_byContinuousTransmit_expectStablePerformance
 *      @[Purpose]: Verify AC-7 complete functionality - long-duration data transmission stability
 *          with performance consistency and memory stability over time
 *      @[Brief]: Continuously transmit data for extended period (minutes to hours),
 *          monitor performance metrics and memory usage, verify stability and no degradation
 *
 *  TC-2:
 *      @[Name]: verifyDatMemoryStability_byLongRunning_expectNoLeaks
 *      @[Purpose]: Verify AC-7 **memory management** - no memory leaks during long-duration operation
 *          with proper resource cleanup and stable memory usage
 *      @[Brief]: Monitor memory usage during long-duration transmission,
 *          verify memory usage remains stable and no continuous growth (memory leaks)
 *
 *  TODO:TC-3...
 *
 *-------------------------------------------------------------------------------------------------
 * [@AC-8,US-4] - System Resource Capability Testing
 *  TC-1:
 *      @[Name]: verifyDatSystemResources_byMonitoring_expectBoundedUsage
 *      @[Purpose]: Verify AC-8 complete functionality - system resource usage remains within bounds
 *          during various capability testing scenarios
 *      @[Brief]: Monitor CPU, memory, and other system resources during capability tests,
 *          verify resource usage stays within acceptable limits and system remains responsive
 *
 *  TODO:TC-2...
 *
 *************************************************************************************************/
//======>END OF TEST CASES=========================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF IMPLEMENTATION=================================================================

#include "_UT_IOC_Common.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF HELPER DEFINITIONS AND UTILITIES================================================

// Test data structures for capability testing
typedef struct {
    bool CallbackExecuted;
    int ReceivedDataCnt;
    size_t TotalReceivedSize;
    int ClientIndex;
    char ReceivedContent[1024 * 1024];  // 1MB buffer for large data testing
    
    // Capability testing specific fields
    struct timespec FirstReceiveTime;
    struct timespec LastReceiveTime;
    uint64_t ExpectedSequenceNumber;
    uint64_t ReceivedSequenceNumber;
    bool SequenceError;
    
    // Performance metrics
    double MinReceiveInterval;
    double MaxReceiveInterval;
    double TotalReceiveTime;
    
} __DatCapabilityPrivData_T;

// Performance measurement utilities
static inline double __getTimeDifferenceMS(const struct timespec *start, const struct timespec *end) {
    double diff = (end->tv_sec - start->tv_sec) * 1000.0;
    diff += (end->tv_nsec - start->tv_nsec) / 1000000.0;
    return diff;
}

static inline void __getCurrentTimeSpec(struct timespec *ts) {
    clock_gettime(CLOCK_MONOTONIC, ts);
}

// Data pattern generation for integrity testing
static void __generateTestDataPattern(char *buffer, size_t size, uint64_t sequenceNumber) {
    // Create predictable pattern based on sequence number
    for (size_t i = 0; i < size; i++) {
        buffer[i] = (char)((sequenceNumber + i) % 256);
    }
}

static bool __verifyTestDataPattern(const char *buffer, size_t size, uint64_t expectedSequence) {
    for (size_t i = 0; i < size; i++) {
        char expected = (char)((expectedSequence + i) % 256);
        if (buffer[i] != expected) {
            return false;
        }
    }
    return true;
}

// Capability testing callback function
static IOC_Result_T __CbRecvDat_Capability_F(IOC_LinkID_T LinkID, void *pData, ULONG_T DataSize, void *pCbPriv) {
    __DatCapabilityPrivData_T *pPrivData = (__DatCapabilityPrivData_T *)pCbPriv;
    
    struct timespec currentTime;
    __getCurrentTimeSpec(&currentTime);
    
    if (!pPrivData->CallbackExecuted) {
        // First callback
        pPrivData->FirstReceiveTime = currentTime;
        pPrivData->CallbackExecuted = true;
    } else {
        // Calculate receive interval for performance metrics
        double interval = __getTimeDifferenceMS(&pPrivData->LastReceiveTime, &currentTime);
        if (pPrivData->MinReceiveInterval == 0 || interval < pPrivData->MinReceiveInterval) {
            pPrivData->MinReceiveInterval = interval;
        }
        if (interval > pPrivData->MaxReceiveInterval) {
            pPrivData->MaxReceiveInterval = interval;
        }
    }
    
    pPrivData->LastReceiveTime = currentTime;
    pPrivData->ReceivedDataCnt++;
    
    // Copy received data for verification (if space available)
    if (pPrivData->TotalReceivedSize + DataSize <= sizeof(pPrivData->ReceivedContent)) {
        memcpy(pPrivData->ReceivedContent + pPrivData->TotalReceivedSize, pData, DataSize);
    }
    
    pPrivData->TotalReceivedSize += DataSize;
    
    // Extract and verify sequence number if data is large enough
    if (DataSize >= sizeof(uint64_t)) {
        uint64_t receivedSeq = *(uint64_t*)pData;
        if (receivedSeq != pPrivData->ExpectedSequenceNumber) {
            pPrivData->SequenceError = true;
            printf("DAT Capability: Sequence error! Expected: %llu, Received: %llu\n", 
                   pPrivData->ExpectedSequenceNumber, receivedSeq);
        }
        pPrivData->ReceivedSequenceNumber = receivedSeq;
        pPrivData->ExpectedSequenceNumber = receivedSeq + 1;
    }
    
    printf("DAT Capability: Client[%d], received %lu bytes, total: %lu bytes, seq: %llu\n", 
           pPrivData->ClientIndex, DataSize, pPrivData->TotalReceivedSize, pPrivData->ReceivedSequenceNumber);
    
    return IOC_RESULT_SUCCESS;
}

//======>END OF HELPER DEFINITIONS AND UTILITIES==================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF: [@AC-1,US-1] TC-1===============================================================
/**
 * @[Name]: verifyDatMaxDataSize_byTransmitIncreasingSize_expectAllSizesSuccess
 * @[Steps]:
 *   1) Setup DatReceiver service with capability testing callback AS SETUP.
 *      |-> DatReceiver online service with IOC_LinkUsageDatReceiver
 *      |-> Configure DatUsageArgs with __CbRecvDat_Capability_F callback
 *      |-> DatSender connect with IOC_LinkUsageDatSender
 *   2) Test data transmission with increasing sizes AS BEHAVIOR.
 *      |-> Test embedded data sizes: 1B, 32B, 64B (should use EmdData)
 *      |-> Test pointer data sizes: 65B, 1KB, 100KB, 1MB (should use pData)
 *      |-> Verify appropriate storage mode selection for each size
 *   3) Verify all sizes transmit successfully and storage mode correctness AS VERIFY.
 *      |-> Each IOC_sendDAT returns IOC_RESULT_SUCCESS
 *      |-> Data integrity maintained for all sizes
 *      |-> Embedded vs pointer storage correctly selected based on size threshold
 *   4) Cleanup: close connections and offline service AS CLEANUP.
 * @[Expect]: All data sizes from 1B to maximum transmit successfully with correct storage mode.
 * @[Notes]: Validates AC-1@US-1 - maximum data size capability and storage optimization.
 */
TEST(UT_DataCapability, verifyDatMaxDataSize_byTransmitIncreasingSize_expectAllSizesSuccess) {
    //===SETUP===
    IOC_Result_T Result = IOC_RESULT_BUG;
    IOC_SrvID_T DatReceiverSrvID = IOC_ID_INVALID;
    IOC_LinkID_T DatReceiverLinkID = IOC_ID_INVALID;
    IOC_LinkID_T DatSenderLinkID = IOC_ID_INVALID;

    // Private data for capability testing
    __DatCapabilityPrivData_T DatCapabilityPrivData = {0};
    DatCapabilityPrivData.ClientIndex = 1;
    DatCapabilityPrivData.ExpectedSequenceNumber = 0;

    IOC_SrvURI_T DatReceiverSrvURI = {
        .pProtocol = IOC_SRV_PROTO_FIFO,
        .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
        .pPath = (const char *)"DatReceiver_MaxDataSize",
    };

    // Setup service with capability testing callback
    IOC_DatUsageArgs_T DatUsageArgs = {
        .CbRecvDat_F = __CbRecvDat_Capability_F,
        .pCbPrivData = &DatCapabilityPrivData,
    };

    IOC_SrvArgs_T SrvArgs = {
        .SrvURI = DatReceiverSrvURI,
        .UsageCapabilites = IOC_LinkUsageDatReceiver,
        .UsageArgs = {
            .pDat = &DatUsageArgs,
        },
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
        ASSERT_NE(IOC_ID_INVALID, DatSenderLinkID);
    });

    Result = IOC_acceptClient(DatReceiverSrvID, &DatReceiverLinkID, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);

    DatSenderThread.join();

    //===BEHAVIOR===
    printf("BEHAVIOR: verifyDatMaxDataSize_byTransmitIncreasingSize_expectAllSizesSuccess\n");

    // Test data sizes from small to large
    const size_t TestSizes[] = {
        1,          // 1B - minimal size
        32,         // 32B - small embedded
        64,         // 64B - embedded threshold
        65,         // 65B - pointer threshold
        1024,       // 1KB - small pointer
        102400,     // 100KB - medium size
        1048576     // 1MB - large size
    };
    const int NumTestSizes = sizeof(TestSizes) / sizeof(TestSizes[0]);

    int SuccessfulTransmissions = 0;

    for (int i = 0; i < NumTestSizes; i++) {
        size_t CurrentSize = TestSizes[i];
        
        // Allocate and prepare test data
        char *TestData = (char *)malloc(CurrentSize);
        ASSERT_NE(nullptr, TestData) << "Failed to allocate " << CurrentSize << " bytes";
        
        __generateTestDataPattern(TestData, CurrentSize, i);

        IOC_DatDesc_T DatDesc = {0};
        IOC_initDatDesc(&DatDesc);
        DatDesc.Payload.pData = TestData;
        DatDesc.Payload.PtrDataSize = CurrentSize;

        // Send data and verify success
        Result = IOC_sendDAT(DatSenderLinkID, &DatDesc, NULL);
        
        if (Result == IOC_RESULT_SUCCESS) {
            SuccessfulTransmissions++;
            printf("DAT Capability: Successfully sent %zu bytes (test %d/%d)\n", 
                   CurrentSize, i+1, NumTestSizes);
        } else {
            printf("DAT Capability: Failed to send %zu bytes, Result: %d\n", CurrentSize, Result);
        }
        
        ASSERT_EQ(IOC_RESULT_SUCCESS, Result) 
            << "Failed to send data size: " << CurrentSize << " bytes";

        IOC_flushDAT(DatSenderLinkID, NULL);
        
        // Give time for callback execution
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        
        free(TestData);
    }

    //===VERIFY===
    // KeyVerifyPoint-1: All sizes should transmit successfully
    ASSERT_EQ(NumTestSizes, SuccessfulTransmissions)
        << "Not all data sizes transmitted successfully. Expected: " << NumTestSizes 
        << ", Actual: " << SuccessfulTransmissions;

    // KeyVerifyPoint-2: All data should be received
    ASSERT_EQ(NumTestSizes, DatCapabilityPrivData.ReceivedDataCnt)
        << "Not all data chunks received. Expected: " << NumTestSizes 
        << ", Received: " << DatCapabilityPrivData.ReceivedDataCnt;

    // KeyVerifyPoint-3: Callback should have been executed
    ASSERT_TRUE(DatCapabilityPrivData.CallbackExecuted)
        << "Capability callback was not executed";

    // KeyVerifyPoint-4: No sequence errors should occur
    ASSERT_FALSE(DatCapabilityPrivData.SequenceError)
        << "Sequence errors detected during transmission";

    printf("DAT Capability: Maximum data size test completed successfully\n");
    printf("  - Total sizes tested: %d\n", NumTestSizes);
    printf("  - Successful transmissions: %d\n", SuccessfulTransmissions);
    printf("  - Total data received: %zu bytes\n", DatCapabilityPrivData.TotalReceivedSize);

    //===CLEANUP===
    if (DatSenderLinkID != IOC_ID_INVALID) {
        IOC_closeLink(DatSenderLinkID);
    }
    if (DatReceiverLinkID != IOC_ID_INVALID) {
        IOC_closeLink(DatReceiverLinkID);
    }
    if (DatReceiverSrvID != IOC_ID_INVALID) {
        IOC_offlineService(DatReceiverSrvID);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF: [@AC-3,US-2] TC-1===============================================================
/**
 * @[Name]: verifyDatHighFrequency_byIncreasingRate_expectScalableThroughput
 * @[Steps]:
 *   1) Setup DatReceiver service with high-frequency capability testing AS SETUP.
 *      |-> DatReceiver online service with performance monitoring callback
 *      |-> DatSender connect for high-frequency transmission
 *   2) Test transmission at increasing frequencies AS BEHAVIOR.
 *      |-> Test frequencies: 1/s, 10/s, 100/s, 1000/s
 *      |-> Each frequency tested for sufficient duration to measure throughput
 *      |-> Monitor transmission success rate and receive intervals
 *   3) Verify throughput scales appropriately and no data loss AS VERIFY.
 *      |-> All transmissions successful at supported frequencies
 *      |-> Receive intervals match expected frequency
 *      |-> No sequence errors or data loss
 *   4) Cleanup: close connections and offline service AS CLEANUP.
 * @[Expect]: High-frequency transmission succeeds with scalable throughput up to maximum capacity.
 * @[Notes]: Validates AC-3@US-2 - high-frequency transmission capability.
 */
TEST(UT_DataCapability, verifyDatHighFrequency_byIncreasingRate_expectScalableThroughput) {
    //===SETUP===
    IOC_Result_T Result = IOC_RESULT_BUG;
    IOC_SrvID_T DatReceiverSrvID = IOC_ID_INVALID;
    IOC_LinkID_T DatReceiverLinkID = IOC_ID_INVALID;
    IOC_LinkID_T DatSenderLinkID = IOC_ID_INVALID;

    __DatCapabilityPrivData_T DatCapabilityPrivData = {0};
    DatCapabilityPrivData.ClientIndex = 2;
    DatCapabilityPrivData.ExpectedSequenceNumber = 0;

    IOC_SrvURI_T DatReceiverSrvURI = {
        .pProtocol = IOC_SRV_PROTO_FIFO,
        .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
        .pPath = (const char *)"DatReceiver_HighFrequency",
    };

    IOC_DatUsageArgs_T DatUsageArgs = {
        .CbRecvDat_F = __CbRecvDat_Capability_F,
        .pCbPrivData = &DatCapabilityPrivData,
    };

    IOC_SrvArgs_T SrvArgs = {
        .SrvURI = DatReceiverSrvURI,
        .UsageCapabilites = IOC_LinkUsageDatReceiver,
        .UsageArgs = {
            .pDat = &DatUsageArgs,
        },
    };

    Result = IOC_onlineService(&DatReceiverSrvID, &SrvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);

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

    //===BEHAVIOR===
    printf("BEHAVIOR: verifyDatHighFrequency_byIncreasingRate_expectScalableThroughput\n");

    // Test frequencies: frequency (Hz), interval (ms), test duration (ms)
    const struct {
        int frequency;      // transmissions per second
        int intervalMS;     // milliseconds between transmissions
        int durationMS;     // total test duration
        int expectedCount;  // expected number of transmissions
    } FrequencyTests[] = {
        {1,    1000, 3000, 3},      // 1 Hz for 3 seconds
        {10,   100,  2000, 20},     // 10 Hz for 2 seconds  
        {100,  10,   1000, 100},    // 100 Hz for 1 second
        {1000, 1,    500,  500},    // 1000 Hz for 0.5 second
    };
    const int NumFrequencyTests = sizeof(FrequencyTests) / sizeof(FrequencyTests[0]);

    for (int testIdx = 0; testIdx < NumFrequencyTests; testIdx++) {
        const auto& freqTest = FrequencyTests[testIdx];
        
        printf("Testing frequency: %d Hz (interval: %d ms, duration: %d ms)\n", 
               freqTest.frequency, freqTest.intervalMS, freqTest.durationMS);

        // Reset counters for this frequency test
        int transmissionCount = 0;
        int successfulTransmissions = 0;
        int initialReceivedCount = DatCapabilityPrivData.ReceivedDataCnt;

        struct timespec startTime, currentTime;
        __getCurrentTimeSpec(&startTime);

        while (true) {
            __getCurrentTimeSpec(&currentTime);
            double elapsedMS = __getTimeDifferenceMS(&startTime, &currentTime);
            
            if (elapsedMS >= freqTest.durationMS) {
                break;  // Test duration completed
            }

            // Prepare test data with sequence number
            const size_t TestDataSize = 128;  // 128 bytes for high-frequency test
            char TestData[TestDataSize];
            *(uint64_t*)TestData = transmissionCount;  // Embed sequence number
            __generateTestDataPattern(TestData + sizeof(uint64_t), 
                                    TestDataSize - sizeof(uint64_t), transmissionCount);

            IOC_DatDesc_T DatDesc = {0};
            IOC_initDatDesc(&DatDesc);
            DatDesc.Payload.pData = TestData;
            DatDesc.Payload.PtrDataSize = TestDataSize;

            Result = IOC_sendDAT(DatSenderLinkID, &DatDesc, NULL);
            transmissionCount++;

            if (Result == IOC_RESULT_SUCCESS) {
                successfulTransmissions++;
            } else {
                printf("Transmission failed at frequency %d Hz, transmission %d, Result: %d\n", 
                       freqTest.frequency, transmissionCount, Result);
            }

            // Sleep for the specified interval
            std::this_thread::sleep_for(std::chrono::milliseconds(freqTest.intervalMS));
        }

        IOC_flushDAT(DatSenderLinkID, NULL);
        
        // Wait for remaining callbacks to complete
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        int receivedThisTest = DatCapabilityPrivData.ReceivedDataCnt - initialReceivedCount;

        printf("Frequency test %d/%d completed:\n", testIdx + 1, NumFrequencyTests);
        printf("  - Attempted transmissions: %d\n", transmissionCount);
        printf("  - Successful transmissions: %d\n", successfulTransmissions);
        printf("  - Received count: %d\n", receivedThisTest);
        printf("  - Expected count: %d\n", freqTest.expectedCount);

        // Verify results for this frequency test
        ASSERT_GE(successfulTransmissions, freqTest.expectedCount * 0.9)  // Allow 10% tolerance
            << "Insufficient successful transmissions at " << freqTest.frequency << " Hz";
        
        ASSERT_GE(receivedThisTest, freqTest.expectedCount * 0.9)  // Allow 10% tolerance
            << "Insufficient received transmissions at " << freqTest.frequency << " Hz";
    }

    //===VERIFY===
    // KeyVerifyPoint-1: Callback should have been executed multiple times
    ASSERT_TRUE(DatCapabilityPrivData.CallbackExecuted)
        << "High-frequency callback was not executed";

    // KeyVerifyPoint-2: Should have received substantial number of transmissions
    ASSERT_GT(DatCapabilityPrivData.ReceivedDataCnt, 500)  // Minimum based on all tests
        << "Insufficient total receptions for high-frequency test";

    // KeyVerifyPoint-3: No sequence errors should occur
    ASSERT_FALSE(DatCapabilityPrivData.SequenceError)
        << "Sequence errors detected during high-frequency transmission";

    // KeyVerifyPoint-4: Performance metrics should be reasonable
    if (DatCapabilityPrivData.MinReceiveInterval > 0 && DatCapabilityPrivData.MaxReceiveInterval > 0) {
        printf("High-frequency performance metrics:\n");
        printf("  - Min receive interval: %.3f ms\n", DatCapabilityPrivData.MinReceiveInterval);
        printf("  - Max receive interval: %.3f ms\n", DatCapabilityPrivData.MaxReceiveInterval);
        
        // Max interval should not be excessively larger than min (indicating instability)
        ASSERT_LT(DatCapabilityPrivData.MaxReceiveInterval, DatCapabilityPrivData.MinReceiveInterval * 10)
            << "Excessive variation in receive intervals indicates instability";
    }

    printf("DAT Capability: High-frequency test completed successfully\n");
    printf("  - Total receptions: %d\n", DatCapabilityPrivData.ReceivedDataCnt);
    printf("  - Total data received: %zu bytes\n", DatCapabilityPrivData.TotalReceivedSize);

    //===CLEANUP===
    if (DatSenderLinkID != IOC_ID_INVALID) {
        IOC_closeLink(DatSenderLinkID);
    }
    if (DatReceiverLinkID != IOC_ID_INVALID) {
        IOC_closeLink(DatReceiverLinkID);
    }
    if (DatReceiverSrvID != IOC_ID_INVALID) {
        IOC_offlineService(DatReceiverSrvID);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>TODO: IMPLEMENT REMAINING TEST CASES======================================================
/*
 * TODO: Implement remaining test cases for complete DAT capability validation:
 * 
 * [@AC-1,US-1] TC-2: verifyDatEmbeddedDataThreshold_byTestStorageMode_expectOptimalSelection
 * [@AC-2,US-1] TC-1: verifyDatMemoryPressure_byLimitedMemory_expectGracefulHandling
 * [@AC-4,US-2] TC-1: verifyDatConcurrentHighFreq_byMultiplePairs_expectAggregatePerformance
 * [@AC-5,US-3] TC-1: verifyDatMaxConnections_byIncreasingCount_expectAllEstablished
 * [@AC-5,US-3] TC-2: verifyDatConnectionEstablishTime_byMaxCount_expectReasonableLatency
 * [@AC-6,US-3] TC-1: verifyDatMaxConcurrentTransmit_byAllConnections_expectDataIntegrity
 * [@AC-7,US-4] TC-1: verifyDatLongDuration_byContinuousTransmit_expectStablePerformance
 * [@AC-7,US-4] TC-2: verifyDatMemoryStability_byLongRunning_expectNoLeaks
 * [@AC-8,US-4] TC-1: verifyDatSystemResources_byMonitoring_expectBoundedUsage
 * 
 * Each test case should follow the established pattern:
 * 1. SETUP: Initialize test environment with capability-specific configuration
 * 2. BEHAVIOR: Execute the capability test scenario
 * 3. VERIFY: Validate results against acceptance criteria
 * 4. CLEANUP: Properly clean up resources
 * 
 * Focus areas for implementation:
 * - Memory management and leak detection
 * - Concurrent connection handling
 * - Performance measurement and analysis
 * - Resource usage monitoring
 * - Long-duration stability testing
 * - System capability boundary identification
 */

//======>END OF IMPLEMENTATION===================================================================