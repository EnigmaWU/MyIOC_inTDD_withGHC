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
//======>BEGIN OF USER STORY=======================================================================
/**************************************************************************************************
 * @brief 【User Story】
 *
 *  US-1: AS a DAT connection state developer,
 *    I WANT to verify that service online/offline and link connect/disconnect operations
 *          maintain correct state transitions,
 *   SO THAT I can ensure connection state consistency throughout the DAT lifecycle
 *      AND detect invalid state transitions during connection management,
 *      AND implement reliable connection state monitoring for DAT services.
 *
 *  US-2: AS a DAT transmission state developer,
 *    I WANT to verify that IOC_sendDAT/IOC_recvDAT operations properly track transmission states,
 *   SO THAT I can ensure data transmission state integrity during send/receive operations
 *      AND monitor concurrent transmission state consistency,
 *      AND implement proper state-aware error handling during data transfer.
 *
 *  US-3: AS a DAT buffer state developer,
 *    I WANT to verify that buffer fill/empty/overflow states are accurately tracked,
 *   SO THAT I can ensure buffer state synchronization across sender and receiver
 *      AND detect buffer overflow conditions with proper state reporting,
 *      AND implement buffer state-aware flow control mechanisms.
 *
 *  US-4: AS a DAT state transition developer,
 *    I WANT to verify that all state transitions are atomic and follow valid transition rules,
 *   SO THAT I can ensure state machine integrity under all conditions
 *      AND prevent invalid state transitions that could corrupt system state,
 *      AND implement robust state validation in DAT operations.
 *
 *  US-5: AS a DAT error recovery state developer,
 *    I WANT to verify that error conditions trigger proper state recovery mechanisms,
 *   SO THAT I can ensure system resilience during DAT operation failures
 *      AND implement automatic state recovery from transient errors,
 *      AND maintain state consistency during link breakage and timeout scenarios.
 *
 *************************************************************************************************/
//======>END OF USER STORY=========================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//=======>BEGIN OF ACCEPTANCE CRITERIA=============================================================
/**************************************************************************************************
 * @brief 【Acceptance Criteria】
 *
 * [@US-1] DAT connection state verification
 *  AC-1: GIVEN a DAT service that needs to be onlined,
 *         WHEN calling IOC_onlineService() to start the service,
 *         THEN service state should transition to online
 *          AND subsequent IOC_connectService() should be able to establish links
 *          AND link state should properly reflect connected status.
 *
 *  AC-2: GIVEN an established DAT link connection,
 *         WHEN calling IOC_closeLink() to disconnect the link,
 *         THEN link state should transition to disconnected
 *          AND further DAT operations on that LinkID should return appropriate error codes
 *          AND service state should remain stable after link disconnection.
 *
 *  AC-3: GIVEN a DAT service accepting multiple client connections,
 *         WHEN multiple clients connect and disconnect concurrently,
 *         THEN each link should maintain independent state tracking
 *          AND service state should remain consistent across all connection changes
 *          AND no state corruption should occur during concurrent operations.
 *
 *---------------------------------------------------------------------------------------------------
 * [@US-2] DAT transmission state verification
 *  AC-1: GIVEN an established DAT link for data transmission,
 *         WHEN calling IOC_sendDAT() to transmit data chunks,
 *         THEN send operation should properly track transmission state
 *          AND transmission state should be consistent across multiple sends
 *          AND concurrent send operations should maintain state integrity.
 *
 *  AC-2: GIVEN a DAT receiver waiting for data,
 *         WHEN data arrives and triggers receive callback or polling,
 *         THEN receive operation should properly track reception state
 *          AND receive state should accurately reflect data availability
 *          AND multiple concurrent receives should maintain state consistency.
 *
 *  AC-3: GIVEN buffered data that needs to be flushed,
 *         WHEN calling IOC_flushDAT() to force transmission,
 *         THEN flush operation should properly track flush state
 *          AND flush state should indicate completion status
 *          AND subsequent operations should reflect post-flush state.
 *
 *---------------------------------------------------------------------------------------------------
 * [@US-3] DAT buffer state verification
 *  AC-1: GIVEN an empty DAT buffer at initialization,
 *         WHEN data is sent and buffered by the IOC framework,
 *         THEN buffer state should accurately track buffer fill level
 *          AND buffer empty/partial/full states should be correctly reported
 *          AND buffer state should be synchronized between sender and receiver.
 *
 *  AC-2: GIVEN a DAT buffer approaching its capacity limit,
 *         WHEN additional data is sent that would exceed buffer capacity,
 *         THEN buffer overflow state should be properly detected and reported
 *          AND appropriate flow control mechanisms should engage
 *          AND buffer state should remain consistent during overflow handling.
 *
 *  AC-3: GIVEN DAT buffers that need synchronization across multiple operations,
 *         WHEN buffer state changes occur during concurrent access,
 *         THEN buffer state should be thread-safe and atomic
 *          AND buffer state reporting should be consistent across all operations
 *          AND no buffer state corruption should occur during concurrent access.
 *
 *---------------------------------------------------------------------------------------------------
 * [@US-4] DAT state transition verification
 *  AC-1: GIVEN a DAT link in any valid state,
 *         WHEN a state transition is triggered by valid operations,
 *         THEN state transition should follow predefined valid transition rules
 *          AND state transition should be atomic without intermediate invalid states
 *          AND state transition should be observable and verifiable.
 *
 *  AC-2: GIVEN a DAT link in any state,
 *         WHEN an invalid operation is attempted that would cause invalid transition,
 *         THEN invalid state transition should be prevented
 *          AND appropriate error codes should be returned
 *          AND current state should remain unchanged after invalid attempt.
 *
 *  AC-3: GIVEN multiple concurrent operations that could affect state,
 *         WHEN these operations execute simultaneously,
 *         THEN state transitions should remain atomic and consistent
 *          AND no race conditions should cause invalid intermediate states
 *          AND final state should be deterministic based on operation ordering.
 *
 *---------------------------------------------------------------------------------------------------
 * [@US-5] DAT error recovery state verification
 *  AC-1: GIVEN a DAT link that encounters transmission errors,
 *         WHEN error conditions are detected during operations,
 *         THEN error state should be properly recorded and reported
 *          AND error recovery mechanisms should be triggered
 *          AND state should transition to appropriate recovery or error state.
 *
 *  AC-2: GIVEN a DAT link that experiences timeout conditions,
 *         WHEN timeout occurs during send/receive/flush operations,
 *         THEN timeout state should be properly tracked and reported
 *          AND timeout recovery should restore link to operational state
 *          AND subsequent operations should work normally after timeout recovery.
 *
 *  AC-3: GIVEN a DAT link that becomes broken or disconnected,
 *         WHEN link breakage is detected during operations,
 *         THEN broken link state should be immediately detected and reported
 *          AND broken link recovery should restore connectivity if possible
 *          AND state should accurately reflect link operational status.
 *
 *************************************************************************************************/
//=======>END OF ACCEPTANCE CRITERIA================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF TEST CASES=======================================================================
/**************************************************************************************************
 * @brief 【Test Cases】
 *
 * [@AC-1,US-1] DAT service online state transition verification
 *  TC-1:
 *      @[Name]: verifyServiceOnlineState_byOnlineService_expectStateTransition
 *      @[Purpose]: Verify IOC_onlineService() properly transitions service to online state
 *      @[Brief]: Online DAT service, verify state transitions from offline to online, check link establishment
 *capability
 *
 *  TC-2:
 *      @[Name]: verifyLinkConnectState_byConnectService_expectConnectionState
 *      @[Purpose]: Verify IOC_connectService() establishes proper link connection state
 *      @[Brief]: Connect to online DAT service, verify link state transitions and connection establishment
 *
 *---------------------------------------------------------------------------------------------------
 * [@AC-2,US-1] DAT link disconnect state verification
 *  TC-1:
 *      @[Name]: verifyLinkDisconnectState_byCloseLink_expectDisconnectedState
 *      @[Purpose]: Verify IOC_closeLink() properly transitions link to disconnected state
 *      @[Brief]: Close established DAT link, verify state transitions and error handling for subsequent operations
 *
 *  TC-2:
 *      @[Name]: verifyServiceStability_afterLinkDisconnect_expectServiceStateIntact
 *      @[Purpose]: Verify service state remains stable after individual link disconnections
 *      @[Brief]: Disconnect individual links, verify service state consistency and stability
 *
 *---------------------------------------------------------------------------------------------------
 * [@AC-3,US-1] DAT concurrent connection state verification
 *  TC-1:
 *      @[Name]: verifyMultiClientState_byConcurrentConnections_expectIndependentStates
 *      @[Purpose]: Verify independent state tracking for multiple concurrent client connections
 *      @[Brief]: Establish multiple concurrent connections, verify each link maintains independent state
 *
 *  TC-2:
 *      @[Name]: verifyServiceStateConsistency_underConcurrentConnectionChanges_expectNoCorruption
 *      @[Purpose]: Verify service state consistency during concurrent connect/disconnect operations
 *      @[Brief]: Perform concurrent connection changes, verify no state corruption occurs
 *
 *---------------------------------------------------------------------------------------------------
 * [@AC-1,US-2] DAT send operation state tracking
 *  TC-1:
 *      @[Name]: verifySendOperationState_bySendDAT_expectTransmissionStateTracking
 *      @[Purpose]: Verify IOC_sendDAT() properly tracks transmission state during send operations
 *      @[Brief]: Send data chunks via IOC_sendDAT(), verify transmission state changes and consistency
 *
 *  TC-2:
 *      @[Name]: verifyConcurrentSendState_byMultipleSends_expectStateIntegrity
 *      @[Purpose]: Verify transmission state integrity during concurrent send operations
 *      @[Brief]: Perform concurrent sends, verify state consistency and no corruption
 *
 *---------------------------------------------------------------------------------------------------
 * [@AC-2,US-2] DAT receive operation state tracking
 *  TC-1:
 *      @[Name]: verifyReceiveOperationState_byRecvDAT_expectReceptionStateTracking
 *      @[Purpose]: Verify IOC_recvDAT() and callbacks properly track reception state
 *      @[Brief]: Receive data via callback and polling, verify reception state changes and accuracy
 *
 *  TC-2:
 *      @[Name]: verifyDataAvailabilityState_byReceiveOperations_expectAccurateStateReporting
 *      @[Purpose]: Verify receive state accurately reflects data availability status
 *      @[Brief]: Test data availability states, verify accurate state reporting for available/no-data scenarios
 *
 *---------------------------------------------------------------------------------------------------
 * [@AC-3,US-2] DAT flush operation state tracking
 *  TC-1:
 *      @[Name]: verifyFlushOperationState_byFlushDAT_expectFlushStateTracking
 *      @[Purpose]: Verify IOC_flushDAT() properly tracks flush operation state
 *      @[Brief]: Perform flush operations, verify flush state changes and completion tracking
 *
 *  TC-2:
 *      @[Name]: verifyPostFlushState_afterFlushCompletion_expectUpdatedState
 *      @[Purpose]: Verify state accurately reflects post-flush status
 *      @[Brief]: Complete flush operations, verify subsequent state reflects flushed status
 *
 *---------------------------------------------------------------------------------------------------
 * [@AC-1,US-3] DAT buffer fill state tracking
 *  TC-1:
 *      @[Name]: verifyBufferFillState_byDataTransmission_expectAccurateBufferTracking
 *      @[Purpose]: Verify buffer state accurately tracks fill level during data transmission
 *      @[Brief]: Send varying amounts of data, verify buffer state tracking from empty to full
 *
 *  TC-2:
 *      @[Name]: verifyBufferStateSynchronization_betweenSenderReceiver_expectConsistentState
 *      @[Purpose]: Verify buffer state synchronization between sender and receiver sides
 *      @[Brief]: Send/receive data, verify buffer state consistency across both sides
 *
 *---------------------------------------------------------------------------------------------------
 * [@AC-2,US-3] DAT buffer overflow state handling
 *  TC-1:
 *      @[Name]: verifyBufferOverflowDetection_byExceedingCapacity_expectOverflowState
 *      @[Purpose]: Verify buffer overflow detection when capacity limits are exceeded
 *      @[Brief]: Send data exceeding buffer capacity, verify overflow state detection and reporting
 *
 *  TC-2:
 *      @[Name]: verifyFlowControlState_duringBufferOverflow_expectProperFlowControl
 *      @[Purpose]: Verify flow control mechanisms engage properly during buffer overflow
 *      @[Brief]: Trigger buffer overflow, verify flow control state and mechanisms
 *
 *---------------------------------------------------------------------------------------------------
 * [@AC-3,US-3] DAT buffer state thread safety
 *  TC-1:
 *      @[Name]: verifyBufferStateThreadSafety_underConcurrentAccess_expectAtomicUpdates
 *      @[Purpose]: Verify buffer state remains thread-safe during concurrent operations
 *      @[Brief]: Perform concurrent buffer operations, verify atomic state updates and no corruption
 *
 *---------------------------------------------------------------------------------------------------
 * [@AC-1,US-4] Valid state transition verification
 *  TC-1:
 *      @[Name]: verifyValidStateTransitions_byValidOperations_expectCorrectTransitions
 *      @[Purpose]: Verify all valid state transitions follow predefined rules
 *      @[Brief]: Execute valid operations, verify state transitions follow expected patterns
 *
 *  TC-2:
 *      @[Name]: verifyAtomicStateTransitions_duringOperations_expectNoIntermediateStates
 *      @[Purpose]: Verify state transitions are atomic without invalid intermediate states
 *      @[Brief]: Monitor state during transitions, verify atomicity and no intermediate invalid states
 *
 *---------------------------------------------------------------------------------------------------
 * [@AC-2,US-4] Invalid state transition prevention
 *  TC-1:
 *      @[Name]: verifyInvalidTransitionPrevention_byInvalidOperations_expectPreventionAndErrors
 *      @[Purpose]: Verify invalid state transitions are properly prevented with error reporting
 *      @[Brief]: Attempt invalid operations, verify prevention and appropriate error codes
 *
 *---------------------------------------------------------------------------------------------------
 * [@AC-3,US-4] Concurrent state transition consistency
 *  TC-1:
 *      @[Name]: verifyConcurrentStateConsistency_underSimultaneousOperations_expectDeterministicFinalState
 *      @[Purpose]: Verify state consistency during concurrent operations with deterministic final state
 *      @[Brief]: Execute concurrent operations, verify final state is deterministic and consistent
 *
 *---------------------------------------------------------------------------------------------------
 * [@AC-1,US-5] Error state recovery
 *  TC-1:
 *      @[Name]: verifyErrorStateRecovery_afterTransmissionErrors_expectRecoveryMechanisms
 *      @[Purpose]: Verify error state recording and recovery mechanisms during transmission errors
 *      @[Brief]: Trigger transmission errors, verify error state tracking and recovery mechanisms
 *
 *---------------------------------------------------------------------------------------------------
 * [@AC-2,US-5] Timeout state handling
 *  TC-1:
 *      @[Name]: verifyTimeoutStateHandling_duringOperationTimeouts_expectTimeoutRecovery
 *      @[Purpose]: Verify timeout state tracking and recovery during operation timeouts
 *      @[Brief]: Trigger operation timeouts, verify timeout state handling and recovery
 *
 *---------------------------------------------------------------------------------------------------
 * [@AC-3,US-5] Broken link state recovery
 *  TC-1:
 *      @[Name]: verifyBrokenLinkStateRecovery_afterLinkBreakage_expectLinkRecovery
 *      @[Purpose]: Verify broken link state detection and recovery mechanisms
 *      @[Brief]: Simulate link breakage, verify broken state detection and recovery mechanisms
 *
 *************************************************************************************************/
//======>END OF TEST CASES=========================================================================
//======>END OF UNIT TESTING DESIGN================================================================

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
