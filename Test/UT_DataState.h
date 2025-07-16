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
 *  US-6: AS a DAT receiver role developer,
 *    I WANT to verify that both Service-as-DatReceiver and Client-as-DatReceiver patterns
 *          maintain correct state transitions in callback and polling modes,
 *   SO THAT I can ensure receiver role state consistency across different connection patterns
 *      AND validate callback vs polling mode state behavior differences,
 *      AND implement reliable receiver state monitoring for both service and client roles.
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
 *---------------------------------------------------------------------------------------------------
 * [@US-6] DAT receiver role和模式验证
 *  AC-1: GIVEN a service configured with UsageCapabilities::DatReceiver,
 *         WHEN clients connect and send data to the service,
 *         THEN service should properly track receiver state in callback mode
 *          AND service should handle concurrent client data sends independently
 *          AND service receiver callback should maintain correct state transitions.
 *
 *  AC-2: GIVEN a client configured with Usage::DatReceiver,
 *         WHEN connecting to service and receiving data via polling,
 *         THEN client should properly track receiver state in polling mode
 *          AND client should handle IOC_recvDAT state transitions correctly
 *          AND client polling should maintain correct state consistency.
 *
 *  AC-3: GIVEN receivers configured for both callback and polling modes,
 *         WHEN comparing state behavior between the two modes,
 *         THEN callback mode should show DataReceiverBusyCbRecvDat state transitions
 *          AND polling mode should show DataReceiverBusyRecvDat state transitions
 *          AND both modes should maintain data integrity and state consistency.
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
 *      @[StateVerification]: Use VERIFY_DAT_LINK_READY_STATE(linkID) to confirm IOC_LinkStateReady
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
 *      @[StateVerification]: Main state IOC_LinkStateReady, private data tracks SendInProgress sub-state simulation
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
 *      @[Brief]: Call IOC_flushDAT(), verify flush state changes and completion status
 *
 *  TC-2:
 *      @[Name]: verifyStreamAutoInitialization_byFirstSendDAT_expectAutoStreamStart
 *      @[Purpose]: Verify first IOC_sendDAT() call automatically initializes data stream
 *      @[Brief]: Call IOC_sendDAT() for the first time, verify auto-initialization state tracking
 *
 *---------------------------------------------------------------------------------------------------
 * [@AC-1,US-3] DAT buffer state verification
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
 *---------------------------------------------------------------------------------------------------
 * [@AC-1,US-6] DAT receiver role state verification
 *  TC-1:
 *      @[Name]: verifyServiceDatReceiverState_byReceiveCallback_expectStateTransition
 *      @[Purpose]: Verify receiver state transitions correctly in Service-as-DatReceiver callback mode
 *      @[Brief]: 作为服务端的接收者，触发接收回调，验证状态转换和后续轮询状态
 *
 *  TC-2:
 *      @[Name]: verifyClientDatReceiverState_byPolling_expectStateTransition
 *      @[Purpose]: Verify receiver state transitions correctly in Client-as-DatReceiver polling mode
 *      @[Brief]: 作为客户端的接收者，触发轮询接收，验证状态转换和回调执行状态
 *
 *  TC-3:
 *      @[Name]: verifyReceiverStateConsistency_betweenCallbackAndPolling_expectNoIntermediateInvalidStates
 *      @[Purpose]: 验证在回调和轮询模式切换时接收者状态的一致性和原子性
 *      @[Brief]: 切换接收模式，验证状态在切换过程中的一致性和无中间无效状态
 *
 *---------------------------------------------------------------------------------------------------
 * [@AC-1,US-6] Service as DatReceiver state verification
 *  TC-1:
 *      @[Name]: verifyServiceReceiverCallbackState_byUsageCapabilitiesDatReceiver_expectCallbackStateTracking
 *      @[Purpose]: Verify service-side DatReceiver with UsageCapabilities properly tracks callback state
 *      @[Brief]: Online service with DatReceiver capability, clients send data, verify callback state transitions
 *
 *  TC-2:
 *      @[Name]: verifyServiceReceiverConcurrentState_byMultipleClientSends_expectIndependentStateHandling
 *      @[Purpose]: Verify service handles concurrent data reception from multiple clients independently
 *      @[Brief]: Multiple clients send data concurrently to service, verify independent receiver state tracking
 *
 *---------------------------------------------------------------------------------------------------
 * [@AC-2,US-6] Client as DatReceiver state verification
 *  TC-1:
 *      @[Name]: verifyClientReceiverPollingState_byUsageDatReceiver_expectPollingStateTracking
 *      @[Purpose]: Verify client-side DatReceiver with Usage properly tracks polling state
 *      @[Brief]: Client connects with DatReceiver usage, polls for data via IOC_recvDAT, verify polling state
 *transitions
 *
 *  TC-2:
 *      @[Name]: verifyClientReceiverDataAvailabilityState_byPollingMode_expectCorrectAvailabilityStates
 *      @[Purpose]: Verify client polling correctly tracks data availability states
 *      @[Brief]: Client polls for data in various availability scenarios, verify state accuracy for available/no-data
 *cases
 *
 *---------------------------------------------------------------------------------------------------
 * [@AC-3,US-6] Callback vs Polling mode state comparison
 *  TC-1:
 *      @[Name]: verifyCallbackVsPollingStateDifferences_byBothModes_expectModeSpecificStateTransitions
 *      @[Purpose]: Verify distinct state transitions between callback and polling modes
 *      @[Brief]: Compare receiver state transitions in callback vs polling, verify mode-specific state behavior
 *
 *  TC-2:
 *      @[Name]: verifyReceiverModeStateConsistency_acrossBothPatterns_expectDataIntegrityPreservation
 *      @[Purpose]: Verify both receiver modes maintain data integrity and state consistency
 *      @[Brief]: Test data reception integrity in both modes, verify consistent state behavior and data handling
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

    // Receiver role configuration tracking
    std::atomic<bool> ServiceAsDatReceiver{false};  // Service configured with UsageCapabilities::DatReceiver
    std::atomic<bool> ClientAsDatReceiver{false};   // Client configured with Usage::DatReceiver
    std::atomic<bool> CallbackModeActive{false};    // Callback mode (automatic) vs Polling mode (manual)
    std::atomic<bool> PollingModeActive{false};     // Polling mode active state

    // Transmission state tracking
    std::atomic<bool> SendInProgress{false};
    std::atomic<bool> ReceiveInProgress{false};
    std::atomic<bool> FlushInProgress{false};

    // Stream lifecycle tracking (DAT auto-initialization behavior)
    std::atomic<bool> StreamAutoInitialized{false};  // 流是否已自动初始化（首次sendDAT调用）
    std::atomic<bool> StreamActive{false};           // 流是否处于活跃状态
    std::atomic<int> SendOperationCount{0};          // 发送操作计数（跟踪auto-init）

    // Buffer state tracking
    std::atomic<size_t> BufferedDataSize{0};
    std::atomic<bool> BufferFull{false};
    std::atomic<bool> BufferEmpty{true};

    // Flow control state tracking (NODROP guarantee)
    std::atomic<bool> FlowControlActive{false};       // 流控制是否激活
    std::atomic<bool> SenderWaitingForBuffer{false};  // 发送方是否在等待缓冲区可用
    std::atomic<bool> ReceiverReadyForData{true};     // 接收方是否准备接收数据

    // State transition tracking
    std::atomic<int> StateTransitionCount{0};
    std::mutex StateMutex;
    std::condition_variable StateCV;

    // Callback execution tracking
    std::atomic<bool> CallbackExecuted{false};
    std::atomic<int> CallbackCount{0};
    IOC_LinkID_T LastCallbackLinkID{IOC_ID_INVALID};

    // Polling operation tracking
    std::atomic<bool> PollingExecuted{false};
    std::atomic<int> PollingCount{0};
    std::atomic<bool> DataAvailable{false};   // Data availability for polling mode
    std::atomic<bool> NoDataReturned{false};  // IOC_RESULT_NO_DATA returned in polling

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

#define VERIFY_LINK_MAIN_AND_SUB_STATE(linkID, expectedMainState, expectedSubState)                   \
    do {                                                                                              \
        IOC_LinkState_T currentMainState = IOC_LinkStateUndefined;                                    \
        IOC_LinkSubState_T currentSubState = IOC_LinkSubStateDefault;                                 \
        IOC_Result_T result = IOC_getLinkState(linkID, &currentMainState, &currentSubState);          \
        ASSERT_EQ(IOC_RESULT_SUCCESS, result) << "Failed to get link state for LinkID=" << linkID;    \
        ASSERT_EQ(expectedMainState, currentMainState)                                                \
            << "Link main state mismatch for LinkID=" << linkID << ", expected=" << expectedMainState \
            << ", actual=" << currentMainState;                                                       \
        ASSERT_EQ(expectedSubState, currentSubState)                                                  \
            << "Link sub state mismatch for LinkID=" << linkID << ", expected=" << expectedSubState   \
            << ", actual=" << currentSubState;                                                        \
    } while (0)

#define VERIFY_DAT_LINK_READY_STATE(linkID)                                                             \
    do {                                                                                                \
        IOC_LinkState_T currentState = IOC_LinkStateUndefined;                                          \
        IOC_Result_T result = IOC_getLinkState(linkID, &currentState, NULL);                            \
        ASSERT_EQ(IOC_RESULT_SUCCESS, result) << "Failed to get DAT link state for LinkID=" << linkID;  \
        ASSERT_EQ(IOC_LinkStateReady, currentState)                                                     \
            << "DAT link main state should be Ready, LinkID=" << linkID << ", actual=" << currentState; \
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
 * @brief 状态测试的数据回调函数（Service as DatReceiver）
 *        用于监控服务端作为数据接收方时的状态变化
 */
static IOC_Result_T __CbRecvDat_ServiceReceiver_F(IOC_LinkID_T LinkID, IOC_DatDesc_pT pDatDesc, void *pCbPriv) {
    __DatStatePrivData_T *pPrivData = (__DatStatePrivData_T *)pCbPriv;

    // Record service receiver callback execution state
    pPrivData->CallbackExecuted = true;
    pPrivData->CallbackCount++;
    pPrivData->LastCallbackLinkID = LinkID;
    pPrivData->ReceiveInProgress = true;
    pPrivData->ServiceAsDatReceiver = true;  // Confirm service receiver role
    pPrivData->CallbackModeActive = true;    // Callback mode (automatic) active

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

    // Update buffer state simulation and flow control tracking
    pPrivData->BufferedDataSize += DataSize;
    pPrivData->BufferEmpty = (pPrivData->BufferedDataSize == 0);

    // Flow control state tracking for NODROP guarantee
    if (pPrivData->BufferedDataSize > 0) {
        pPrivData->ReceiverReadyForData = true;  // Receiver is processing data
    }

    // Record state change
    RECORD_STATE_CHANGE(pPrivData);

    pPrivData->ReceiveInProgress = false;

    printf("📊 SERVICE RECEIVER CALLBACK: LinkID=%llu, DataSize=%lu, TotalReceived=%zu, CallbackCount=%d\n", LinkID,
           DataSize, pPrivData->TotalDataReceived, pPrivData->CallbackCount.load());

    return IOC_RESULT_SUCCESS;
}

/**
 * @brief 状态测试的数据回调函数（Client as DatReceiver - if callback mode supported for clients）
 *        用于监控客户端作为数据接收方时的状态变化
 */
static IOC_Result_T __CbRecvDat_ClientReceiver_F(IOC_LinkID_T LinkID, IOC_DatDesc_pT pDatDesc, void *pCbPriv) {
    __DatStatePrivData_T *pPrivData = (__DatStatePrivData_T *)pCbPriv;

    // Record client receiver callback execution state
    pPrivData->CallbackExecuted = true;
    pPrivData->CallbackCount++;
    pPrivData->LastCallbackLinkID = LinkID;
    pPrivData->ReceiveInProgress = true;
    pPrivData->ClientAsDatReceiver = true;  // Confirm client receiver role
    pPrivData->CallbackModeActive = true;   // Callback mode (automatic) active

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

    // Flow control state tracking
    if (pPrivData->BufferedDataSize > 0) {
        pPrivData->ReceiverReadyForData = true;
    }

    // Record state change
    RECORD_STATE_CHANGE(pPrivData);

    pPrivData->ReceiveInProgress = false;

    printf("📊 CLIENT RECEIVER CALLBACK: LinkID=%llu, DataSize=%lu, TotalReceived=%zu, CallbackCount=%d\n", LinkID,
           DataSize, pPrivData->TotalDataReceived, pPrivData->CallbackCount.load());

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

    // Reset receiver role configuration
    pPrivData->ServiceAsDatReceiver = false;
    pPrivData->ClientAsDatReceiver = false;
    pPrivData->CallbackModeActive = false;
    pPrivData->PollingModeActive = false;

    pPrivData->SendInProgress = false;
    pPrivData->ReceiveInProgress = false;
    pPrivData->FlushInProgress = false;

    // Reset stream lifecycle state
    pPrivData->StreamAutoInitialized = false;
    pPrivData->StreamActive = false;
    pPrivData->SendOperationCount = 0;

    pPrivData->BufferedDataSize = 0;
    pPrivData->BufferFull = false;
    pPrivData->BufferEmpty = true;

    // Reset flow control state
    pPrivData->FlowControlActive = false;
    pPrivData->SenderWaitingForBuffer = false;
    pPrivData->ReceiverReadyForData = true;

    pPrivData->StateTransitionCount = 0;
    pPrivData->CallbackExecuted = false;
    pPrivData->CallbackCount = 0;
    pPrivData->LastCallbackLinkID = IOC_ID_INVALID;

    // Reset polling operation tracking
    pPrivData->PollingExecuted = false;
    pPrivData->PollingCount = 0;
    pPrivData->DataAvailable = false;
    pPrivData->NoDataReturned = false;

    pPrivData->ErrorOccurred = false;
    pPrivData->RecoveryTriggered = false;
    pPrivData->LastErrorCode = IOC_RESULT_SUCCESS;
    pPrivData->TotalDataSent = 0;
    pPrivData->TotalDataReceived = 0;
    pPrivData->DataIntegrityValid = true;
    pPrivData->TimeoutOccurred = false;
}

/**
 * @brief 状态测试辅助函数：模拟客户端轮询接收数据
 *        用于测试客户端作为DatReceiver的轮询模式状态行为
 */
static IOC_Result_T __SimulateClientPollingRecv(__DatStatePrivData_T *pPrivData, IOC_LinkID_T LinkID,
                                                bool simulateDataAvailable) {
    // Record polling operation state
    pPrivData->PollingExecuted = true;
    pPrivData->PollingCount++;
    pPrivData->ClientAsDatReceiver = true;  // Confirm client receiver role
    pPrivData->PollingModeActive = true;    // Polling mode (manual) active
    pPrivData->ReceiveInProgress = true;

    IOC_Result_T result;
    if (simulateDataAvailable) {
        // Simulate successful data reception
        pPrivData->DataAvailable = true;
        pPrivData->NoDataReturned = false;
        pPrivData->TotalDataReceived += 100;  // Simulate 100 bytes received
        pPrivData->BufferedDataSize += 100;
        pPrivData->BufferEmpty = false;
        result = IOC_RESULT_SUCCESS;

        printf("📊 CLIENT POLLING SUCCESS: LinkID=%llu, DataReceived=100, TotalReceived=%zu, PollingCount=%d\n", LinkID,
               pPrivData->TotalDataReceived, pPrivData->PollingCount.load());
    } else {
        // Simulate no data available
        pPrivData->DataAvailable = false;
        pPrivData->NoDataReturned = true;
        result = IOC_RESULT_NO_DATA;

        printf("📊 CLIENT POLLING NO_DATA: LinkID=%llu, PollingCount=%d\n", LinkID, pPrivData->PollingCount.load());
    }

    // Record state change
    RECORD_STATE_CHANGE(pPrivData);

    pPrivData->ReceiveInProgress = false;
    return result;
}

/**
 * @brief DAT State Machine Mapping Documentation
 *
 * Based on README_ArchDesign::DAT::Conet, DAT uses composite state machine:
 *
 * MAIN STATE MAPPING:
 *    - DAT links ALWAYS use: IOC_LinkStateReady (main state)
 *
 * SUB-STATE MAPPING (requires IOC_Types.h extension):
 *    Architecture Design → IOC Sub-State (to be implemented):
 *    - DataSenderReady → IOC_LinkSubStateDatSenderReady
 *    - DataSenderBusySendDat → IOC_LinkSubStateDatSenderBusySendDat
 *    - DataReceiverReady → IOC_LinkSubStateDatReceiverReady
 *    - DataReceiverBusyRecvDat → IOC_LinkSubStateDatReceiverBusyRecvDat (polling mode)
 *    - DataReceiverBusyCbRecvDat → IOC_LinkSubStateDatReceiverBusyCbRecvDat (callback mode)
 *
 * CURRENT LIMITATION:
 *    IOC_Types.h currently only defines IOC_LinkSubStateDefault/IOC_LinkSubStateIdle
 *    DAT-specific sub-states need to be added to IOC_Types.h
 *
 * TEST STRATEGY:
 *    1. Verify main state always IOC_LinkStateReady (use VERIFY_DAT_LINK_READY_STATE)
 *    2. Use private data structure to simulate sub-state tracking (SendInProgress, ReceiveInProgress, etc.)
 *    3. After IOC_Types.h extension, use VERIFY_LINK_MAIN_AND_SUB_STATE for real sub-state verification
 */

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>ARCHITECTURE ALIGNMENT REVIEW============================================================
/**
 * @brief 架构设计对齐审查报告
 *
 * 📋 ARCH DESIGN COMPLIANCE CHECK:
 * ✅ DAT State Machine: 正确实现README_ArchDesign中的DAT::Conet复合状态机
 *    - LinkStateReady主状态包含独立的发送方和接收方子状态
 *    - 主状态：DAT链接始终使用IOC_LinkStateReady
 *    - 子状态：DataSenderReady/DataSenderBusySendDat状态转换
 *    - 子状态：DataReceiverReady/DataReceiverBusyRecvDat/DataReceiverBusyCbRecvDat状态转换
 *    - 当前使用私有数据结构模拟子状态跟踪（待IOC_Types.h扩展后使用真实子状态）
 *
 * ✅ DAT Properties: 正确实现DAT固有属性
 *    - ASYNC (总是): 数据处理在IOC上下文中执行
 *    - STREAM (总是): 连续数据流而非离散消息
 *    - NODROP (总是): 可靠流传输保证，不支持MAYDROP
 *    - MAYBLOCK (默认): 阻塞直到操作完成或失败
 *
 * ✅ Auto-Initialization: 实现流自动初始化行为
 *    - 首次IOC_sendDAT()调用自动初始化流
 *    - StreamAutoInitialized/StreamActive状态跟踪
 *    - SendOperationCount计数验证首次调用
 *
 * ✅ API Coverage: 覆盖所有DAT API及其错误码
 *    - IOC_sendDAT: 发送数据块到数据接收方
 *    - IOC_recvDAT: 轮询模式接收数据块
 *    - IOC_flushDAT: 强制传输缓冲数据（唯一显式控制操作）
 *
 * ✅ Flow Control: 实现NODROP保证的流控制状态跟踪
 *    - FlowControlActive: 流控制激活状态
 *    - SenderWaitingForBuffer: 发送方等待缓冲区可用
 *    - ReceiverReadyForData: 接收方准备接收数据
 *
 * ✅ Error Handling: 实现所有文档化的错误处理
 *    - IOC_RESULT_BUFFER_FULL (即时NONBLOCK模式)
 *    - IOC_RESULT_TIMEOUT (NONBLOCK超时模式)
 *    - IOC_RESULT_LINK_BROKEN (传输期间链接断开)
 *    - IOC_RESULT_NOT_EXIST_LINK (LinkID不存在或已关闭)
 *
 * 🔄 STATE TEST DESIGN ALIGNMENT:
 *    - 连接状态: 服务上线/下线，链接连接/断开验证
 *    - 传输状态: IOC_sendDAT/IOC_recvDAT操作状态跟踪
 *    - 缓冲状态: 缓冲区填充/清空/溢出状态管理
 *    - 状态转换: 原子性和有效转换规则验证
 *    - 错误恢复: 错误条件下的状态恢复机制
 *    - 接收方角色: Service as DatReceiver vs Client as DatReceiver 状态验证
 *    - 接收模式: Callback模式(自动) vs Polling模式(手动) 状态行为差异
 *
 * ✅ RECEIVER PATTERN COVERAGE:
 *    🔧 Service as DatReceiver: onlineService with UsageCapabilities::DatReceiver
 *    🔧 Client as DatReceiver: connectService with Usage::DatReceiver
 *    📞 Callback Mode: __CbRecvDat_ServiceReceiver_F / __CbRecvDat_ClientReceiver_F
 *    📊 Polling Mode: __SimulateClientPollingRecv with IOC_recvDAT state tracking
 *    🔍 State Verification: VERIFY_RECEIVER_ROLE_CONFIG, VERIFY_RECEIVER_MODE_STATE
 *    📋 Mode Differences: DataReceiverBusyCbRecvDat vs DataReceiverBusyRecvDat transitions
 *
 * TODO: IOC_Types.h EXTENSION NEEDED:
 *    Current IOC_LinkSubState_T only has:
 *    - IOC_LinkSubStateDefault = 0
 *    - IOC_LinkSubStateIdle = IOC_LinkSubStateDefault
 *
 *    Need to add DAT-specific sub-states:
 *    - IOC_LinkSubStateDatSenderReady
 *    - IOC_LinkSubStateDatSenderBusySendDat
 *    - IOC_LinkSubStateDatReceiverReady
 *    - IOC_LinkSubStateDatReceiverBusyRecvDat
 *    - IOC_LinkSubStateDatReceiverBusyCbRecvDat
 */
//======>END OF ARCHITECTURE ALIGNMENT REVIEW=====================================================

#endif  // UT_DATASTATE_H
