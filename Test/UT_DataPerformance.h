///////////////////////////////////////////////////////////////////////////////////////////////////
// QUICK REFERENCE GUIDE - 快速参考指南
// 📝 用途: DAT（数据传输）性能测试单元测试头文件框架
// 🔄 流程: User Story → Acceptance Criteria → Test Cases → Implementation
// 📂 分类: DataPerformance - 专注于DAT数据传输的性能特性和优化场景验证
// 🎯 重点: 吞吐量、延迟、资源利用率、并发性能和性能优化的完整性验证
// Reference Unit Testing Templates in UT_FreelyDrafts.cxx when needed.
///////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef UT_DATAPERFORMANCE_H
#define UT_DATAPERFORMANCE_H

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <limits>
#include <mutex>
#include <thread>
#include <vector>

#include "_UT_IOC_Common.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF OVERVIEW OF THIS UNIT TESTING FILE===============================================
/**
 * @brief
 *  验证IOC框架中DAT（数据传输）的性能特性和优化能力，专注于吞吐量、延迟、
 *  资源利用率以及各种性能优化场景的完整性验证。
 *
 *-------------------------------------------------------------------------------------------------
 *++DAT性能测试验证数据传输过程中的性能指标和优化效果，本测试文件关注性能相关场景：
 *
 *  性能验证范围：
 *  - 🚀 吞吐量测试: 单位时间内数据传输量、批量传输能力
 *  - ⏱️ 延迟测试: 端到端传输延迟、API调用响应时间
 *  - 💾 资源利用: 内存使用效率、CPU占用率、线程资源管理
 *  - 🔄 并发性能: 多线程传输、并发连接、资源竞争性能
 *  - 📈 扩展性能: 负载增加时的性能表现、系统容量限制
 *  - 🎯 优化验证: 缓冲优化、流控优化、零拷贝等优化机制
 *
 *  关键性能指标：
 *  - Throughput: 数据吞吐量 (MB/s, packets/s)
 *  - Latency: 传输延迟 (milliseconds, microseconds)
 *  - CPU Usage: CPU使用率百分比
 *  - Memory Usage: 内存使用量和内存效率
 *  - Concurrent Capacity: 并发处理能力
 *  - Resource Efficiency: 资源利用效率
 *
 *  不包括：
 *  - 功能正确性测试（DataTypical 覆盖）
 *  - 边界条件测试（DataBoundary 覆盖）
 *  - 状态转换测试（DataState 覆盖）
 *  - 系统稳定性测试（DataRobust 覆盖）
 *
 *  参考文档：
 *  - IOC_Data.c: 数据传输API实现和性能优化
 *  - IOC_SrvProtoFifo: FIFO协议的性能特性
 *  - 系统性能需求规格书
 */
//======>END OF OVERVIEW OF THIS UNIT TESTING FILE=================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF USER STORY=======================================================================
/**************************************************************************************************
 * @brief 【User Story】
 *
 *  US-1: AS a high-throughput DAT application developer,
 *    I WANT to verify that IOC_sendDAT/IOC_recvDAT operations achieve optimal throughput,
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
 *  US-3: AS a resource-constrained DAT application developer,
 *    I WANT to verify that DAT operations optimize memory and CPU resource usage,
 *   SO THAT I can ensure efficient resource utilization in embedded systems
 *      AND validate memory allocation patterns and prevent memory leaks,
 *      AND implement resource-efficient data transfer mechanisms.
 *
 *  US-4: AS a concurrent DAT application developer,
 *    I WANT to verify that DAT operations scale efficiently with concurrent usage,
 *   SO THAT I can ensure performance remains stable under multi-threaded load
 *      AND validate concurrent connection handling capabilities,
 *      AND implement scalable multi-client data distribution systems.
 *
 *  US-5: AS a DAT optimization developer,
 *    I WANT to verify that performance optimization features work effectively,
 *   SO THAT I can ensure buffering, flow control, and zero-copy optimizations deliver benefits
 *      AND validate that NODROP guarantees don't significantly impact performance,
 *      AND implement advanced performance tuning mechanisms.
 *
 *  US-6: AS a DAT system capacity planner,
 *    I WANT to verify system behavior under increasing load conditions,
 *   SO THAT I can determine maximum system capacity and performance limits
 *      AND validate graceful degradation under overload conditions,
 *      AND implement proper capacity planning for production deployments.
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
 *         WHEN sending large payloads (1KB to 1MB) repeatedly,
 *         THEN throughput should achieve target rates (e.g., >100MB/s for large payloads)
 *              AND throughput should scale with payload size efficiently
 *              AND bulk transfer operations should maintain consistent performance.
 *
 *  AC-2: GIVEN multiple DAT streams operating simultaneously,
 *         WHEN each stream transfers data at high rates,
 *         THEN aggregate throughput should scale linearly with stream count
 *              AND individual stream performance should remain stable
 *              AND resource contention should not cause significant degradation.
 *
 *  AC-3: GIVEN DAT batch operations with varying batch sizes,
 *         WHEN processing batches from small (10 items) to large (10K items),
 *         THEN batch processing efficiency should improve with larger batches
 *              AND memory allocation overhead should be minimized
 *              AND batch completion time should scale sub-linearly.
 *
 *---------------------------------------------------------------------------------------------------
 * [@US-2] Low-latency DAT operations verification
 *  AC-1: GIVEN a DAT link optimized for minimal latency,
 *         WHEN sending small messages (64B to 4KB) with immediate delivery,
 *         THEN end-to-end latency should be within target limits (e.g., <1ms)
 *              AND latency should be consistent across message sizes
 *              AND jitter should be minimal for real-time applications.
 *
 *  AC-2: GIVEN IOC_sendDAT and IOC_recvDAT API calls,
 *         WHEN measuring API call response times,
 *         THEN API latency should be minimal (e.g., <100μs for small messages)
 *              AND API performance should not degrade with system load
 *              AND blocking operations should have predictable timing.
 *
 *  AC-3: GIVEN DAT callback mechanisms for immediate data delivery,
 *         WHEN data arrives and triggers callbacks,
 *         THEN callback invocation latency should be minimal
 *              AND callback processing should not block other operations
 *              AND callback queue management should maintain low latency.
 *
 *---------------------------------------------------------------------------------------------------
 * [@US-3] Resource-efficient DAT operations verification
 *  AC-1: GIVEN DAT operations running on resource-constrained systems,
 *         WHEN monitoring memory usage during data transfer,
 *         THEN memory allocation should be efficient and predictable
 *              AND memory leaks should not occur during sustained operations
 *              AND buffer reuse should minimize allocation overhead.
 *
 *  AC-2: GIVEN DAT operations under CPU monitoring,
 *         WHEN transferring data at various rates,
 *         THEN CPU usage should scale proportionally with data volume
 *              AND CPU overhead per byte should decrease with larger transfers
 *              AND system responsiveness should be maintained.
 *
 *  AC-3: GIVEN DAT thread and handle resource management,
 *         WHEN creating and destroying multiple connections,
 *         THEN thread pool utilization should be efficient
 *              AND handle allocation should not leak resources
 *              AND resource cleanup should be complete and timely.
 *
 *---------------------------------------------------------------------------------------------------
 * [@US-4] Concurrent DAT operations verification
 *  AC-1: GIVEN multiple threads performing DAT operations simultaneously,
 *         WHEN thread count increases from 1 to N (e.g., 100),
 *         THEN throughput should scale efficiently with thread count
 *              AND thread contention should not cause significant overhead
 *              AND thread safety should be maintained without data corruption.
 *
 *  AC-2: GIVEN concurrent DAT senders and receivers,
 *         WHEN multiple clients connect to a single service,
 *         THEN service should handle concurrent connections efficiently
 *              AND per-client performance should remain stable
 *              AND resource sharing should be fair across clients.
 *
 *  AC-3: GIVEN DAT operations under concurrent stress testing,
 *         WHEN simulating real-world concurrent usage patterns,
 *         THEN system should maintain stability under high concurrency
 *              AND performance degradation should be gradual and predictable
 *              AND no deadlocks or race conditions should occur.
 *
 *---------------------------------------------------------------------------------------------------
 * [@US-5] DAT optimization features verification
 *  AC-1: GIVEN DAT buffering optimization features,
 *         WHEN comparing buffered vs unbuffered operations,
 *         THEN buffered operations should show improved throughput
 *              AND buffer size tuning should demonstrate performance gains
 *              AND optimal buffer sizes should be determinable.
 *
 *  AC-2: GIVEN DAT flow control mechanisms (NODROP guarantees),
 *         WHEN receiver cannot keep up with sender,
 *         THEN flow control should prevent data loss with minimal performance impact
 *              AND sender backpressure should be handled efficiently
 *              AND system should recover quickly when receiver catches up.
 *
 *  AC-3: GIVEN zero-copy and memory optimization features,
 *         WHEN enabled for large data transfers,
 *         THEN memory copy overhead should be significantly reduced
 *              AND CPU usage should decrease for large payload transfers
 *              AND memory bandwidth utilization should be optimal.
 *
 *---------------------------------------------------------------------------------------------------
 * [@US-6] DAT system capacity and scalability verification
 *  AC-1: GIVEN DAT system under increasing load conditions,
 *         WHEN load increases from low to maximum capacity,
 *         THEN system should maintain acceptable performance until limits
 *              AND maximum capacity should be clearly determinable
 *              AND performance degradation should be graceful beyond limits.
 *
 *  AC-2: GIVEN DAT system approaching resource limits,
 *         WHEN system resources (memory, threads, connections) reach capacity,
 *         THEN system should handle resource exhaustion gracefully
 *              AND error reporting should indicate specific resource constraints
 *              AND system should recover when resources become available.
 *
 *  AC-3: GIVEN long-running DAT operations for endurance testing,
 *         WHEN system operates under sustained load for extended periods,
 *         THEN performance should remain stable over time
 *              AND no performance degradation should occur due to resource leaks
 *              AND system should handle sustained operations reliably.
 */
//=======>END OF ACCEPTANCE CRITERIA================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF TEST CASE DEFINITIONS============================================================
/**************************************************************************************************
 * @brief 【Test Cases】
 *
 * [@AC-1,US-1] High-throughput bulk data transfer
 *  TC-1:
 *      @[Name]: verifyBulkDataThroughput_byLargePayloads_expectOptimalRates
 *      @[Purpose]: 验证大负载数据传输的吞吐量性能
 *      @[Brief]: 使用1KB到1MB负载测试吞吐量，验证性能目标达成
 *      @[Throughput_Focus]: 测试最大数据传输速率和负载大小对性能的影响
 *
 *  TC-2:
 *      @[Name]: verifyThroughputScaling_byPayloadSize_expectLinearScaling
 *      @[Purpose]: 验证吞吐量随负载大小的扩展性
 *      @[Brief]: 测试不同负载大小下的吞吐量扩展性
 *      @[Scaling_Focus]: 测试性能随数据大小的扩展规律
 *
 * [@AC-2,US-1] Multi-stream concurrent throughput
 *  TC-1:
 *      @[Name]: verifyMultiStreamThroughput_byConcurrentStreams_expectLinearScaling
 *      @[Purpose]: 验证多流并发传输的吞吐量扩展性
 *      @[Brief]: 同时运行多个数据流，验证聚合吞吐量的线性扩展
 *      @[Concurrent_Focus]: 测试并发数据流的性能影响
 *
 * [@AC-1,US-2] Low-latency message delivery
 *  TC-1:
 *      @[Name]: verifyEndToEndLatency_bySmallMessages_expectMinimalDelay
 *      @[Purpose]: 验证小消息端到端传输延迟
 *      @[Brief]: 测试64B到4KB消息的传输延迟，验证实时性要求
 *      @[Latency_Focus]: 测试低延迟数据传输能力
 *
 *  TC-2:
 *      @[Name]: verifyAPIResponseTime_byCallLatency_expectMicrosecondLevel
 *      @[Purpose]: 验证API调用响应时间
 *      @[Brief]: 测量IOC_sendDAT/IOC_recvDAT的API调用延迟
 *      @[API_Focus]: 测试API级别的性能特性
 *
 * [@AC-1,US-3] Memory and CPU resource efficiency
 *  TC-1:
 *      @[Name]: verifyMemoryEfficiency_byAllocationPatterns_expectOptimalUsage
 *      @[Purpose]: 验证内存使用效率和分配模式
 *      @[Brief]: 监控数据传输过程中的内存分配和释放效率
 *      @[Memory_Focus]: 测试内存使用优化效果
 *
 *  TC-2:
 *      @[Name]: verifyCPUUtilization_byDataVolume_expectProportionalUsage
 *      @[Purpose]: 验证CPU使用率与数据量的比例关系
 *      @[Brief]: 测量不同数据量下的CPU使用率
 *      @[CPU_Focus]: 测试CPU资源利用效率
 *
 * [@AC-1,US-4] Concurrent operations performance
 *  TC-1:
 *      @[Name]: verifyConcurrentThreadPerformance_byMultiThreading_expectLinearScaling
 *      @[Purpose]: 验证多线程并发操作的性能扩展性
 *      @[Brief]: 增加线程数量，测试并发性能扩展性
 *      @[Threading_Focus]: 测试多线程环境下的性能表现
 *
 *  TC-2:
 *      @[Name]: verifyMultiClientPerformance_byConcurrentConnections_expectFairSharing
 *      @[Purpose]: 验证多客户端并发连接的性能公平性
 *      @[Brief]: 测试多客户端同时连接时的性能分配
 *      @[MultiClient_Focus]: 测试并发客户端的资源公平分配
 *
 * [@AC-1,US-5] Performance optimization features
 *  TC-1:
 *      @[Name]: verifyBufferingOptimization_byBufferSizeTuning_expectPerformanceGains
 *      @[Purpose]: 验证缓冲优化对性能的提升效果
 *      @[Brief]: 比较不同缓冲区大小对传输性能的影响
 *      @[Buffering_Focus]: 测试缓冲机制的性能优化效果
 *
 *  TC-2:
 *      @[Name]: verifyZeroCopyOptimization_byLargeTransfers_expectReducedOverhead
 *      @[Purpose]: 验证零拷贝优化的性能提升
 *      @[Brief]: 测试零拷贝机制对大数据传输的性能影响
 *      @[ZeroCopy_Focus]: 测试零拷贝优化的实际效果
 *
 * [@AC-1,US-6] System capacity and scalability
 *  TC-1:
 *      @[Name]: verifySystemCapacity_byIncreasingLoad_expectGracefulLimits
 *      @[Purpose]: 验证系统容量限制和优雅降级
 *      @[Brief]: 逐步增加系统负载，确定最大容量和性能边界
 *      @[Capacity_Focus]: 测试系统最大承载能力
 *
 *  TC-2:
 *      @[Name]: verifyEndurancePerformance_byLongRunning_expectStablePerformance
 *      @[Purpose]: 验证长期运行的性能稳定性
 *      @[Brief]: 长时间运行性能测试，验证性能稳定性
 *      @[Endurance_Focus]: 测试长期运行的性能一致性
 *
 *************************************************************************************************/
//======>END OF TEST CASE DEFINITIONS==============================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF PERFORMANCE TESTING INFRASTRUCTURE===============================================
/**
 * @brief 性能测试基础设施
 *        提供性能测量、数据收集、统计分析等功能
 */

// ===== PERFORMANCE METRICS COLLECTION =====
/**
 * @brief 性能指标数据结构
 *        统一的性能数据收集和分析结构
 */
typedef struct PerformanceMetrics {
    // Throughput metrics
    double BytesPerSecond{0.0};          // 字节每秒吞吐量
    double MessagesPerSecond{0.0};       // 消息每秒吞吐量
    double OperationsPerSecond{0.0};     // 操作每秒频率
    size_t TotalBytesTransferred{0};     // 总传输字节数
    size_t TotalMessagesTransferred{0};  // 总传输消息数
    size_t TotalOperationsCompleted{0};  // 总完成操作数

    // Latency metrics (in microseconds)
    double MinLatencyUs{std::numeric_limits<double>::max()};  // 最小延迟
    double MaxLatencyUs{0.0};                                 // 最大延迟
    double AvgLatencyUs{0.0};                                 // 平均延迟
    double MedianLatencyUs{0.0};                              // 中位延迟
    double P95LatencyUs{0.0};                                 // 95%分位延迟
    double P99LatencyUs{0.0};                                 // 99%分位延迟
    double JitterUs{0.0};                                     // 延迟抖动

    // Resource usage metrics
    double MemoryUsageMB{0.0};       // 内存使用量(MB)
    double PeakMemoryUsageMB{0.0};   // 峰值内存使用量(MB)
    double CPUUsagePercent{0.0};     // CPU使用率百分比
    double AvgCPUUsagePercent{0.0};  // 平均CPU使用率
    size_t ThreadCount{0};           // 活跃线程数
    size_t HandleCount{0};           // 句柄数量

    // Timing information
    std::chrono::high_resolution_clock::time_point StartTime;  // 测试开始时间
    std::chrono::high_resolution_clock::time_point EndTime;    // 测试结束时间
    double DurationSeconds{0.0};                               // 测试持续时间(秒)

    // Quality metrics
    size_t ErrorCount{0};     // 错误计数
    double ErrorRate{0.0};    // 错误率
    size_t TimeoutCount{0};   // 超时计数
    double SuccessRate{0.0};  // 成功率

    // Concurrent metrics
    size_t MaxConcurrentOperations{0};  // 最大并发操作数
    size_t AvgConcurrentOperations{0};  // 平均并发操作数
    size_t ConcurrentThreads{0};        // 并发线程数
    size_t ConcurrentConnections{0};    // 并发连接数

} PerformanceMetrics_T;

/**
 * @brief 性能测试配置参数
 *        定义各种性能测试的配置选项
 */
typedef struct PerformanceTestConfig {
    // Test duration and iteration control
    std::chrono::seconds TestDurationSec{10};  // 测试持续时间
    size_t MaxIterations{1000000};             // 最大迭代次数
    size_t WarmupIterations{1000};             // 预热迭代次数
    size_t CooldownSec{1};                     // 冷却时间

    // Data transfer parameters
    size_t MinPayloadSize{64};           // 最小负载大小
    size_t MaxPayloadSize{1024 * 1024};  // 最大负载大小(1MB)
    size_t PayloadSizeStep{1024};        // 负载大小步长
    std::vector<size_t> PayloadSizes;    // 指定的负载大小列表

    // Concurrency parameters
    size_t MinThreadCount{1};               // 最小线程数
    size_t MaxThreadCount{100};             // 最大线程数
    size_t ThreadCountStep{10};             // 线程数步长
    size_t MaxConcurrentConnections{1000};  // 最大并发连接数

    // Performance targets and thresholds
    double TargetThroughputMBps{100.0};    // 目标吞吐量(MB/s)
    double MaxAcceptableLatencyMs{1.0};    // 最大可接受延迟(ms)
    double MaxAcceptableCPUPercent{80.0};  // 最大可接受CPU使用率
    double MaxAcceptableMemoryMB{512.0};   // 最大可接受内存使用量

    // Measurement configuration
    bool EnableLatencyMeasurement{true};     // 启用延迟测量
    bool EnableThroughputMeasurement{true};  // 启用吞吐量测量
    bool EnableResourceMeasurement{true};    // 启用资源使用测量
    bool EnableDetailedLogging{false};       // 启用详细日志

    // Test behavior flags
    bool UseRandomPayloadSizes{false};   // 使用随机负载大小
    bool UseRandomTimingPattern{false};  // 使用随机时序模式
    bool EnableStressMode{false};        // 启用压力测试模式
    bool EnableEnduranceMode{false};     // 启用耐久性测试模式

} PerformanceTestConfig_T;

/**
 * @brief 性能数据采集器
 *        实时收集和计算性能指标
 */
class PerformanceCollector {
   private:
    std::vector<double> latencySamples;
    std::mutex dataMutex;
    std::chrono::high_resolution_clock::time_point testStartTime;
    PerformanceMetrics_T currentMetrics;

   public:
    void StartCollection() {
        std::lock_guard<std::mutex> lock(dataMutex);
        testStartTime = std::chrono::high_resolution_clock::now();
        currentMetrics.StartTime = testStartTime;
        latencySamples.clear();
    }

    void RecordLatency(double latencyUs) {
        std::lock_guard<std::mutex> lock(dataMutex);
        latencySamples.push_back(latencyUs);

        // Update min/max
        if (latencyUs < currentMetrics.MinLatencyUs) {
            currentMetrics.MinLatencyUs = latencyUs;
        }
        if (latencyUs > currentMetrics.MaxLatencyUs) {
            currentMetrics.MaxLatencyUs = latencyUs;
        }
    }

    void RecordOperation(size_t bytesTransferred) {
        std::lock_guard<std::mutex> lock(dataMutex);
        currentMetrics.TotalBytesTransferred += bytesTransferred;
        currentMetrics.TotalMessagesTransferred++;
        currentMetrics.TotalOperationsCompleted++;
    }

    void RecordError() {
        std::lock_guard<std::mutex> lock(dataMutex);
        currentMetrics.ErrorCount++;
    }

    PerformanceMetrics_T FinishCollection() {
        std::lock_guard<std::mutex> lock(dataMutex);

        currentMetrics.EndTime = std::chrono::high_resolution_clock::now();
        currentMetrics.DurationSeconds =
            std::chrono::duration<double>(currentMetrics.EndTime - currentMetrics.StartTime).count();

        // Calculate throughput
        if (currentMetrics.DurationSeconds > 0) {
            currentMetrics.BytesPerSecond = currentMetrics.TotalBytesTransferred / currentMetrics.DurationSeconds;
            currentMetrics.MessagesPerSecond = currentMetrics.TotalMessagesTransferred / currentMetrics.DurationSeconds;
            currentMetrics.OperationsPerSecond =
                currentMetrics.TotalOperationsCompleted / currentMetrics.DurationSeconds;
        }

        // Calculate latency statistics
        if (!latencySamples.empty()) {
            std::sort(latencySamples.begin(), latencySamples.end());

            double sum = 0;
            for (double sample : latencySamples) {
                sum += sample;
            }
            currentMetrics.AvgLatencyUs = sum / latencySamples.size();

            size_t medianIndex = latencySamples.size() / 2;
            currentMetrics.MedianLatencyUs = latencySamples[medianIndex];

            size_t p95Index = static_cast<size_t>(latencySamples.size() * 0.95);
            if (p95Index < latencySamples.size()) {
                currentMetrics.P95LatencyUs = latencySamples[p95Index];
            }

            size_t p99Index = static_cast<size_t>(latencySamples.size() * 0.99);
            if (p99Index < latencySamples.size()) {
                currentMetrics.P99LatencyUs = latencySamples[p99Index];
            }

            // Calculate jitter (standard deviation)
            double variance = 0;
            for (double sample : latencySamples) {
                variance += (sample - currentMetrics.AvgLatencyUs) * (sample - currentMetrics.AvgLatencyUs);
            }
            currentMetrics.JitterUs = std::sqrt(variance / latencySamples.size());
        }

        // Calculate success/error rates
        size_t totalOps = currentMetrics.TotalOperationsCompleted + currentMetrics.ErrorCount;
        if (totalOps > 0) {
            currentMetrics.ErrorRate = static_cast<double>(currentMetrics.ErrorCount) / totalOps;
            currentMetrics.SuccessRate = static_cast<double>(currentMetrics.TotalOperationsCompleted) / totalOps;
        }

        return currentMetrics;
    }
};

// ===== PERFORMANCE VERIFICATION MACROS =====
/**
 * @brief 性能验证宏定义
 *        提供便捷的性能断言和验证功能
 */

#define VERIFY_THROUGHPUT_TARGET(metrics, targetMBps)                                                \
    do {                                                                                             \
        double actualMBps = (metrics).BytesPerSecond / (1024.0 * 1024.0);                            \
        EXPECT_GE(actualMBps, targetMBps)                                                            \
            << "Throughput below target: " << actualMBps << " MB/s < " << targetMBps << " MB/s";     \
        printf("📊 [THROUGHPUT] Achieved: %.2f MB/s (Target: %.2f MB/s)\n", actualMBps, targetMBps); \
    } while (0)

#define VERIFY_LATENCY_TARGET(metrics, maxLatencyMs)                                                          \
    do {                                                                                                      \
        double actualLatencyMs = (metrics).AvgLatencyUs / 1000.0;                                             \
        EXPECT_LE(actualLatencyMs, maxLatencyMs)                                                              \
            << "Latency exceeds target: " << actualLatencyMs << " ms > " << maxLatencyMs << " ms";            \
        printf("⏱️ [LATENCY] Avg: %.3f ms, P95: %.3f ms, P99: %.3f ms (Target: < %.2f ms)\n", actualLatencyMs, \
               (metrics).P95LatencyUs / 1000.0, (metrics).P99LatencyUs / 1000.0, maxLatencyMs);               \
    } while (0)

#define VERIFY_CPU_USAGE_TARGET(metrics, maxCPUPercent)                                                        \
    do {                                                                                                       \
        EXPECT_LE((metrics).AvgCPUUsagePercent, maxCPUPercent)                                                 \
            << "CPU usage exceeds target: " << (metrics).AvgCPUUsagePercent << "% > " << maxCPUPercent << "%"; \
        printf("💻 [CPU] Usage: %.1f%% (Target: < %.1f%%)\n", (metrics).AvgCPUUsagePercent, maxCPUPercent);    \
    } while (0)

#define VERIFY_MEMORY_USAGE_TARGET(metrics, maxMemoryMB)                                                           \
    do {                                                                                                           \
        EXPECT_LE((metrics).PeakMemoryUsageMB, maxMemoryMB)                                                        \
            << "Memory usage exceeds target: " << (metrics).PeakMemoryUsageMB << " MB > " << maxMemoryMB << " MB"; \
        printf("💾 [MEMORY] Peak: %.1f MB, Avg: %.1f MB (Target: < %.1f MB)\n", (metrics).PeakMemoryUsageMB,       \
               (metrics).MemoryUsageMB, maxMemoryMB);                                                              \
    } while (0)

#define VERIFY_SUCCESS_RATE_TARGET(metrics, minSuccessRate)                                                            \
    do {                                                                                                               \
        EXPECT_GE((metrics).SuccessRate, minSuccessRate)                                                               \
            << "Success rate below target: " << (metrics).SuccessRate << " < " << minSuccessRate;                      \
        printf("✅ [SUCCESS] Rate: %.2f%%, Errors: %zu/%zu (Target: > %.2f%%)\n", (metrics).SuccessRate * 100,         \
               (metrics).ErrorCount, (metrics).TotalOperationsCompleted + (metrics).ErrorCount, minSuccessRate * 100); \
    } while (0)

#define VERIFY_CONCURRENT_SCALING(baseMetrics, scaledMetrics, threadMultiplier, expectedEfficiency)                    \
    do {                                                                                                               \
        double scalingEfficiency = ((scaledMetrics).BytesPerSecond / (baseMetrics).BytesPerSecond) / threadMultiplier; \
        EXPECT_GE(scalingEfficiency, expectedEfficiency)                                                               \
            << "Concurrent scaling efficiency below target: " << scalingEfficiency << " < " << expectedEfficiency;     \
        printf("🚀 [SCALING] Efficiency: %.2f%% with %dx threads (Target: > %.2f%%)\n", scalingEfficiency * 100,       \
               threadMultiplier, expectedEfficiency * 100);                                                            \
    } while (0)

// ===== PERFORMANCE TEST HELPER FUNCTIONS =====
/**
 * @brief 性能测试辅助函数实现
 *        提供常用的性能测试工具函数
 */

// 创建性能测试数据
inline std::vector<char> CreatePerformanceTestData(size_t size, bool randomContent = false) {
    std::vector<char> data(size);
    if (randomContent) {
        for (size_t i = 0; i < size; ++i) {
            data[i] = static_cast<char>(rand() % 256);
        }
    } else {
        // Fill with predictable pattern for verification
        for (size_t i = 0; i < size; ++i) {
            data[i] = static_cast<char>('A' + (i % 26));
        }
    }
    return data;
}

// 测量单次操作延迟
template <typename Func>
double MeasureOperationLatency(Func operation) {
    auto start = std::chrono::high_resolution_clock::now();
    operation();
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double, std::micro>(end - start).count();
}

// 并发执行性能测试
template <typename Func>
PerformanceMetrics_T RunConcurrentPerformanceTest(const PerformanceTestConfig_T& config, size_t threadCount,
                                                  Func threadOperation) {
    PerformanceCollector collector;
    collector.StartCollection();

    std::vector<std::thread> threads;
    std::atomic<bool> shouldStop{false};

    // 启动测试线程
    for (size_t i = 0; i < threadCount; ++i) {
        threads.emplace_back([&, i]() { threadOperation(i, shouldStop, collector); });
    }

    // 等待测试时间或迭代完成
    std::this_thread::sleep_for(config.TestDurationSec);
    shouldStop = true;

    // 等待所有线程完成
    for (auto& thread : threads) {
        thread.join();
    }

    return collector.FinishCollection();
}

// 打印性能测试报告
inline void PrintPerformanceReport(const PerformanceMetrics_T& metrics, const std::string& testName) {
    printf("\n╔══════════════════════════════════════════════════════════════════════════════════════════╗\n");
    printf("║                            📊 PERFORMANCE REPORT: %s\n", testName.c_str());
    printf("╠══════════════════════════════════════════════════════════════════════════════════════════╣\n");
    printf("║ 🚀 THROUGHPUT:                                                                          ║\n");
    printf("║   • Bytes/sec:     %12.2f MB/s                                               ║\n",
           metrics.BytesPerSecond / (1024.0 * 1024.0));
    printf("║   • Messages/sec:  %12.2f msgs/s                                             ║\n",
           metrics.MessagesPerSecond);
    printf("║   • Operations/sec:%12.2f ops/s                                              ║\n",
           metrics.OperationsPerSecond);
    printf("║                                                                                          ║\n");
    printf("║ ⏱️ LATENCY:                                                                              ║\n");
    printf("║   • Min:           %12.2f μs                                                  ║\n", metrics.MinLatencyUs);
    printf("║   • Average:       %12.2f μs                                                  ║\n", metrics.AvgLatencyUs);
    printf("║   • Median:        %12.2f μs                                                  ║\n",
           metrics.MedianLatencyUs);
    printf("║   • P95:           %12.2f μs                                                  ║\n", metrics.P95LatencyUs);
    printf("║   • P99:           %12.2f μs                                                  ║\n", metrics.P99LatencyUs);
    printf("║   • Max:           %12.2f μs                                                  ║\n", metrics.MaxLatencyUs);
    printf("║   • Jitter:        %12.2f μs                                                  ║\n", metrics.JitterUs);
    printf("║                                                                                          ║\n");
    printf("║ 📈 VOLUME:                                                                               ║\n");
    printf("║   • Total Bytes:   %12zu bytes                                               ║\n",
           metrics.TotalBytesTransferred);
    printf("║   • Total Messages:%12zu messages                                            ║\n",
           metrics.TotalMessagesTransferred);
    printf("║   • Total Ops:     %12zu operations                                          ║\n",
           metrics.TotalOperationsCompleted);
    printf("║   • Test Duration: %12.2f seconds                                            ║\n",
           metrics.DurationSeconds);
    printf("║                                                                                          ║\n");
    printf("║ ✅ QUALITY:                                                                              ║\n");
    printf("║   • Success Rate:  %12.2f%%                                                   ║\n",
           metrics.SuccessRate * 100);
    printf("║   • Error Count:   %12zu errors                                              ║\n", metrics.ErrorCount);
    printf("║   • Error Rate:    %12.2f%%                                                   ║\n",
           metrics.ErrorRate * 100);
    printf("╚══════════════════════════════════════════════════════════════════════════════════════════╝\n\n");
}

// ===== DATA TRANSFER PRIVATE DATA WITH PERFORMANCE TRACKING =====
/**
 * @brief DAT性能测试私有数据结构
 *        扩展基础状态跟踪，增加性能监控功能
 */
typedef struct __DatPerformancePrivData {
    // 基础连接状态 (继承自DataState)
    std::atomic<bool> ServiceOnline{false};
    std::atomic<bool> LinkConnected{false};
    std::atomic<bool> StreamInitialized{false};

    // 性能计数器
    std::atomic<size_t> SendOperationCount{0};
    std::atomic<size_t> RecvOperationCount{0};
    std::atomic<size_t> FlushOperationCount{0};
    std::atomic<size_t> TotalBytesSent{0};
    std::atomic<size_t> TotalBytesReceived{0};

    // 延迟追踪
    std::vector<double> SendLatencies;
    std::vector<double> RecvLatencies;
    std::mutex LatencyMutex;

    // 资源使用监控
    std::atomic<size_t> CurrentMemoryUsage{0};
    std::atomic<size_t> PeakMemoryUsage{0};
    std::atomic<double> CurrentCPUUsage{0.0};
    std::atomic<size_t> ActiveThreadCount{0};

    // 并发操作计数
    std::atomic<size_t> ConcurrentSendOps{0};
    std::atomic<size_t> ConcurrentRecvOps{0};
    std::atomic<size_t> MaxConcurrentOps{0};

    // 错误和超时计数
    std::atomic<size_t> ErrorCount{0};
    std::atomic<size_t> TimeoutCount{0};
    std::atomic<size_t> RetryCount{0};

    // 性能基准点
    std::chrono::high_resolution_clock::time_point TestStartTime;
    std::chrono::high_resolution_clock::time_point LastOperationTime;

    // 优化特性标志
    std::atomic<bool> BufferingEnabled{false};
    std::atomic<bool> ZeroCopyEnabled{false};
    std::atomic<bool> FlowControlActive{false};

    // 测试配置
    PerformanceTestConfig_T TestConfig;

} __DatPerformancePrivData_T;

/**
 * @brief 重置性能测试私有数据
 */
inline void __ResetPerformanceTracking(__DatPerformancePrivData_T* privData) {
    if (!privData) return;

    privData->ServiceOnline = false;
    privData->LinkConnected = false;
    privData->StreamInitialized = false;

    privData->SendOperationCount = 0;
    privData->RecvOperationCount = 0;
    privData->FlushOperationCount = 0;
    privData->TotalBytesSent = 0;
    privData->TotalBytesReceived = 0;

    {
        std::lock_guard<std::mutex> lock(privData->LatencyMutex);
        privData->SendLatencies.clear();
        privData->RecvLatencies.clear();
    }

    privData->CurrentMemoryUsage = 0;
    privData->PeakMemoryUsage = 0;
    privData->CurrentCPUUsage = 0.0;
    privData->ActiveThreadCount = 0;

    privData->ConcurrentSendOps = 0;
    privData->ConcurrentRecvOps = 0;
    privData->MaxConcurrentOps = 0;

    privData->ErrorCount = 0;
    privData->TimeoutCount = 0;
    privData->RetryCount = 0;

    privData->BufferingEnabled = false;
    privData->ZeroCopyEnabled = false;
    privData->FlowControlActive = false;
}

/**
 * @brief 记录性能操作
 */
#define RECORD_PERFORMANCE_OPERATION(privData, operation, bytes, latencyUs)          \
    do {                                                                             \
        if (privData) {                                                              \
            privData->operation##OperationCount++;                                   \
            privData->TotalBytes##operation += bytes;                                \
            std::lock_guard<std::mutex> lock(privData->LatencyMutex);                \
            privData->operation##Latencies.push_back(latencyUs);                     \
            privData->LastOperationTime = std::chrono::high_resolution_clock::now(); \
        }                                                                            \
    } while (0)

/**
 * @brief 更新并发操作计数
 */
#define UPDATE_CONCURRENT_OPERATION_COUNT(privData, operation, increment)                                        \
    do {                                                                                                         \
        if (privData) {                                                                                          \
            if (increment) {                                                                                     \
                size_t current = ++privData->Concurrent##operation##Ops;                                         \
                size_t maxOps = privData->MaxConcurrentOps.load();                                               \
                while (current > maxOps && !privData->MaxConcurrentOps.compare_exchange_weak(maxOps, current)) { \
                }                                                                                                \
            } else {                                                                                             \
                privData->Concurrent##operation##Ops--;                                                          \
            }                                                                                                    \
        }                                                                                                        \
    } while (0)

#endif  // UT_DATAPERFORMANCE_H

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>END OF UT_DATAPERFORMANCE.H===============================================================
