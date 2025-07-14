///////////////////////////////////////////////////////////////////////////////////////////////////
// QUICK REFERENCE GUIDE - 快速参考指南
// 📝 用途: DAT（数据传输）状态测试单元测试头文件框架
// 🔄 流程: User Story → Acceptance Criteria → Test Cases → Implementation
// 📂 分类: DataState - 专注于DAT数据传输的状态机验证和状态转换测试
// 🎯 重点: 连接状态、传输状态、缓冲状态和状态转换的完整性验证
// Reference Unit Testing Templates in UT_FreelyDrafts.cxx when needed.
///////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef UT_DATASTATE_H
#define UT_DATASTATE_H

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>

#include "_UT_IOC_Common.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF OVERVIEW OF THIS UNIT TESTING FILE===============================================
/**
 * @brief
 *  验证IOC框架中DAT（数据传输）的状态机行为和状态转换正确性，专注于连接状态、
 *  传输状态、缓冲状态以及各种状态转换场景的完整性验证。
 *
 *-------------------------------------------------------------------------------------------------
 *++DAT状态测试验证数据传输过程中的状态机行为，本测试文件关注状态相关场景：
 *
 *  状态验证范围：
 *  - 🔗 连接状态: 连接建立、断开、重连过程中的状态转换
 *  - 📡 传输状态: 发送、接收过程中的状态变化和状态一致性
 *  - 📋 缓冲状态: 缓冲区满、空、部分填充状态的行为验证
 *  - 🔄 状态转换: 各种状态间转换的正确性和完整性
 *  - 🚨 异常状态: 错误、超时、中断状态的恢复机制
 *
 *  关键验证点：
 *  - LinkID 有效性与状态对应关系
 *  - IOC_sendDAT/IOC_recvDAT 操作期间的状态转换
 *  - IOC_flushDAT 操作对状态的影响
 *  - 多线程环境下状态的一致性
 *  - 错误情况下状态的恢复能力
 *
 *  不包括：
 *  - 典型使用场景（DataTypical 覆盖）
 *  - 边界条件测试（DataBoundary 覆盖）
 *  - 系统容量测试（DataCapability 覆盖）
 *  - 性能优化场景
 *
 *  参考文档：
 *  - README_ArchDesign.md::State::DAT（状态定义）
 *  - IOC_Types.h::IOC_LinkState_T（状态枚举）
 *  - README_RefAPIs.md::IOC_getLinkState（状态查询API）
 */
//======>END OF OVERVIEW OF THIS UNIT TESTING FILE=================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF UNIT TESTING DESIGN==============================================================

/**************************************************************************************************
 * 📋 DAT STATE TEST FOCUS - DAT状态测试重点
 *
 * 🎯 DESIGN PRINCIPLE: 验证DAT数据传输状态机的正确性和完整性
 * 🔄 PRIORITY ORDER: 连接状态 → 传输状态 → 缓冲状态 → 状态转换 → 异常恢复
 *
 * 🔄 STATE (状态测试):
 *    💭 Purpose: 验证对象生命周期、状态机转换、状态一致性
 *    🎯 Focus: 连接建立/断开、发送/接收状态、错误状态恢复
 *    📝 Examples: 服务状态（Init→Ready→Running→Stopped）、事件状态、链接状态
 *    ⏰ When: 状态化组件、FSM验证、状态一致性检查
 *
 * ✅ STATE SCENARIOS COVERED:
 *    🔗 Connection States: Service online/offline, Link connect/disconnect, Accept/close
 *    📡 Transmission States: Sending, receiving, buffering, flushing states
 *    📋 Buffer States: Empty, partial, full, overflow buffer states
 *    🔄 State Transitions: Valid transitions, invalid attempts, atomic transitions
 *    🚨 Error Recovery: Broken link recovery, timeout recovery, error state transitions
 *
 * ❌ EXCLUDED FROM STATE TESTING:
 *    ✅ 典型数据传输流程（DataTypical覆盖）
 *    🔲 参数边界验证（DataBoundary覆盖）
 *    🚀 性能和容量测试（DataCapability覆盖）
 *    📊 长期稳定性测试
 *    🛠️ 协议特定实现细节
 *
 * 🎯 STATE TESTING CATEGORIES:
 *    🔗 CONNECTION_STATE: 服务上线/下线、连接建立/断开状态
 *    📡 TRANSMISSION_STATE: 数据发送/接收过程中的状态变化
 *    📋 BUFFER_STATE: 缓冲区状态管理和状态同步
 *    🔄 TRANSITION_STATE: 状态转换的正确性和原子性
 *    🚨 RECOVERY_STATE: 错误和异常情况下的状态恢复
 *************************************************************************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF DATA STRUCTURES AND HELPERS====================================================

/**
 * @brief 状态测试私有数据结构
 *        用于跟踪和验证状态转换过程中的各种信息
 */
typedef struct __DatStatePrivData {
    // Connection state tracking
    std::atomic<bool> ServiceOnline{false};
    std::atomic<bool> LinkConnected{false};
    std::atomic<bool> LinkAccepted{false};

    // Transmission state tracking
    std::atomic<bool> SendInProgress{false};
    std::atomic<bool> ReceiveInProgress{false};
    std::atomic<bool> FlushInProgress{false};

    // Buffer state tracking
    std::atomic<size_t> BufferedDataSize{0};
    std::atomic<bool> BufferFull{false};
    std::atomic<bool> BufferEmpty{true};

    // State transition tracking
    std::atomic<int> StateTransitionCount{0};
    std::mutex StateMutex;
    std::condition_variable StateCV;

    // Callback execution tracking
    std::atomic<bool> CallbackExecuted{false};
    std::atomic<int> CallbackCount{0};
    IOC_LinkID_T LastCallbackLinkID{IOC_ID_INVALID};

    // Error and recovery tracking
    std::atomic<bool> ErrorOccurred{false};
    std::atomic<bool> RecoveryTriggered{false};
    IOC_Result_T LastErrorCode{IOC_RESULT_SUCCESS};

    // Data integrity tracking
    size_t TotalDataSent{0};
    size_t TotalDataReceived{0};
    std::atomic<bool> DataIntegrityValid{true};

    // Timing and synchronization
    std::chrono::high_resolution_clock::time_point LastStateChangeTime;
    std::atomic<bool> TimeoutOccurred{false};

    // Client identification (for multi-client scenarios)
    int ClientIndex{0};
    char ClientName[64]{0};

} __DatStatePrivData_T;

/**
 * @brief 状态验证宏定义
 *        提供便捷的状态检查和验证功能
 */
#define VERIFY_LINK_STATE(linkID, expectedState)                                                                 \
    do {                                                                                                         \
        IOC_LinkState_T currentState = IOC_LinkStateUndefined;                                                   \
        IOC_Result_T result = IOC_getLinkState(linkID, &currentState, NULL);                                     \
        ASSERT_EQ(IOC_RESULT_SUCCESS, result) << "Failed to get link state for LinkID=" << linkID;               \
        ASSERT_EQ(expectedState, currentState) << "Link state mismatch for LinkID=" << linkID                    \
                                               << ", expected=" << expectedState << ", actual=" << currentState; \
    } while (0)

#define VERIFY_STATE_TRANSITION_WITHIN_TIME(privData, timeoutMs)                                           \
    do {                                                                                                   \
        std::unique_lock<std::mutex> lock((privData)->StateMutex);                                         \
        bool success = (privData)->StateCV.wait_for(lock, std::chrono::milliseconds(timeoutMs),            \
                                                    [&] { return (privData)->StateTransitionCount > 0; }); \
        ASSERT_TRUE(success) << "State transition did not occur within " << timeoutMs << "ms";             \
    } while (0)

#define RECORD_STATE_CHANGE(privData)                                                \
    do {                                                                             \
        std::lock_guard<std::mutex> lock((privData)->StateMutex);                    \
        (privData)->StateTransitionCount++;                                          \
        (privData)->LastStateChangeTime = std::chrono::high_resolution_clock::now(); \
        (privData)->StateCV.notify_all();                                            \
    } while (0)

/**
 * @brief 状态测试的数据回调函数
 *        用于监控数据传输过程中的状态变化
 */
static IOC_Result_T __CbRecvDat_State_F(IOC_LinkID_T LinkID, IOC_DatDesc_pT pDatDesc, void *pCbPriv) {
    __DatStatePrivData_T *pPrivData = (__DatStatePrivData_T *)pCbPriv;

    // Record callback execution state
    pPrivData->CallbackExecuted = true;
    pPrivData->CallbackCount++;
    pPrivData->LastCallbackLinkID = LinkID;
    pPrivData->ReceiveInProgress = true;

    // Extract data from DatDesc for state tracking
    void *pData;
    ULONG_T DataSize;
    IOC_Result_T result = IOC_getDatPayload(pDatDesc, &pData, &DataSize);
    if (result != IOC_RESULT_SUCCESS) {
        pPrivData->ErrorOccurred = true;
        pPrivData->LastErrorCode = result;
        pPrivData->ReceiveInProgress = false;
        return result;
    }

    // Update receive state tracking
    pPrivData->TotalDataReceived += DataSize;

    // Update buffer state simulation
    pPrivData->BufferedDataSize += DataSize;
    pPrivData->BufferEmpty = (pPrivData->BufferedDataSize == 0);

    // Record state change
    RECORD_STATE_CHANGE(pPrivData);

    pPrivData->ReceiveInProgress = false;

    printf("📊 STATE CALLBACK: LinkID=%llu, DataSize=%lu, TotalReceived=%zu, CallbackCount=%d\n", LinkID, DataSize,
           pPrivData->TotalDataReceived, pPrivData->CallbackCount.load());

    return IOC_RESULT_SUCCESS;
}

/**
 * @brief 状态变化监控回调函数
 *        用于监控连接状态的变化（如果IOC框架支持状态变化通知）
 */
static void __StateChangeNotify_F(IOC_LinkID_T LinkID, IOC_LinkState_T OldState, IOC_LinkState_T NewState,
                                  void *pPrivData) {
    __DatStatePrivData_T *pStateData = (__DatStatePrivData_T *)pPrivData;

    RECORD_STATE_CHANGE(pStateData);

    printf("📊 STATE CHANGE: LinkID=%llu, %d→%d, Count=%d\n", LinkID, OldState, NewState,
           pStateData->StateTransitionCount.load());
}

/**
 * @brief 状态测试辅助函数：验证服务状态
 */
static bool __VerifyServiceState(IOC_SrvID_T srvID, bool expectOnline) {
    // 注意：当前IOC框架可能没有直接的服务状态查询API
    // 这里使用间接方法验证服务状态
    if (expectOnline) {
        return srvID != IOC_ID_INVALID;
    } else {
        return srvID == IOC_ID_INVALID;
    }
}

/**
 * @brief 状态测试辅助函数：等待状态转换
 */
static bool __WaitForStateTransition(__DatStatePrivData_T *pPrivData, int expectedCount, int timeoutMs) {
    std::unique_lock<std::mutex> lock(pPrivData->StateMutex);
    return pPrivData->StateCV.wait_for(lock, std::chrono::milliseconds(timeoutMs),
                                       [&] { return pPrivData->StateTransitionCount >= expectedCount; });
}

/**
 * @brief 状态测试辅助函数：重置状态跟踪数据
 */
static void __ResetStateTracking(__DatStatePrivData_T *pPrivData) {
    pPrivData->ServiceOnline = false;
    pPrivData->LinkConnected = false;
    pPrivData->LinkAccepted = false;
    pPrivData->SendInProgress = false;
    pPrivData->ReceiveInProgress = false;
    pPrivData->FlushInProgress = false;
    pPrivData->BufferedDataSize = 0;
    pPrivData->BufferFull = false;
    pPrivData->BufferEmpty = true;
    pPrivData->StateTransitionCount = 0;
    pPrivData->CallbackExecuted = false;
    pPrivData->CallbackCount = 0;
    pPrivData->LastCallbackLinkID = IOC_ID_INVALID;
    pPrivData->ErrorOccurred = false;
    pPrivData->RecoveryTriggered = false;
    pPrivData->LastErrorCode = IOC_RESULT_SUCCESS;
    pPrivData->TotalDataSent = 0;
    pPrivData->TotalDataReceived = 0;
    pPrivData->DataIntegrityValid = true;
    pPrivData->TimeoutOccurred = false;
}

//======>END OF DATA STRUCTURES AND HELPERS======================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF STATE TEST SCENARIOS============================================================

/**************************************************************************************************
 * 📋 CONNECTION STATE TEST SCENARIOS - 连接状态测试场景
 *
 * 🔗 CS-1: Service Online/Offline State Transitions
 *    验证服务上线下线过程中的状态转换
 *
 * 🔗 CS-2: Link Connect/Disconnect State Management
 *    验证链接连接断开过程中的状态管理
 *
 * 🔗 CS-3: Accept/Close Client State Consistency
 *    验证接受关闭客户端过程中的状态一致性
 *************************************************************************************************/

/**************************************************************************************************
 * 📋 TRANSMISSION STATE TEST SCENARIOS - 传输状态测试场景
 *
 * 📡 TS-1: Send Operation State Tracking
 *    验证发送操作期间的状态跟踪
 *
 * 📡 TS-2: Receive Operation State Management
 *    验证接收操作期间的状态管理
 *
 * 📡 TS-3: Flush Operation State Transitions
 *    验证刷新操作期间的状态转换
 *
 * 📡 TS-4: Concurrent Operation State Consistency
 *    验证并发操作时的状态一致性
 *************************************************************************************************/

/**************************************************************************************************
 * 📋 BUFFER STATE TEST SCENARIOS - 缓冲状态测试场景
 *
 * 📋 BS-1: Buffer Fill/Empty State Tracking
 *    验证缓冲区填充清空过程的状态跟踪
 *
 * 📋 BS-2: Buffer Overflow State Handling
 *    验证缓冲区溢出情况的状态处理
 *
 * 📋 BS-3: Buffer Synchronization State
 *    验证缓冲区同步过程的状态管理
 *************************************************************************************************/

/**************************************************************************************************
 * 📋 TRANSITION STATE TEST SCENARIOS - 状态转换测试场景
 *
 * 🔄 TS-1: Valid State Transition Verification
 *    验证有效状态转换的正确性
 *
 * 🔄 TS-2: Invalid State Transition Prevention
 *    验证无效状态转换的阻止机制
 *
 * 🔄 TS-3: Atomic State Transition Consistency
 *    验证原子状态转换的一致性
 *************************************************************************************************/

/**************************************************************************************************
 * 📋 RECOVERY STATE TEST SCENARIOS - 恢复状态测试场景
 *
 * 🚨 RS-1: Error State Recovery Mechanism
 *    验证错误状态的恢复机制
 *
 * 🚨 RS-2: Timeout State Handling
 *    验证超时状态的处理
 *
 * 🚨 RS-3: Broken Link State Recovery
 *    验证断开链接的状态恢复
 *************************************************************************************************/

//======>END OF STATE TEST SCENARIOS==============================================================

#endif  // UT_DATASTATE_H
