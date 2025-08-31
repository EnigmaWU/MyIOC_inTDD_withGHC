///////////////////////////////////////////////////////////////////////////////////////////////////
// Command State Testing - Common Header
//
// Intent:
// - Provides shared utilities, macros, and data structures for Command State testing
// - Supports DUAL-STATE testing approach: Individual Command State + Link Command Execution State
// - Follows the established Data State testing pattern (UT_DataState.h)
//
// 🎯 DUAL-STATE RATIONALE:
//     Command state verification requires testing TWO distinct but related state aspects:
//     1. INDIVIDUAL COMMAND STATE: Each IOC_CmdDesc_T's lifecycle (PENDING→PROCESSING→SUCCESS/FAILED/TIMEOUT)
//     2. LINK COMMAND EXECUTION STATE: How IOC_LinkID_T reflects command processing activity
//
//     WHY DUAL-STATE IS NECESSARY:
//     - Individual commands have their own status/result fields independent of link state
//     - Links can process multiple concurrent commands, requiring aggregate state tracking
//     - Command execution patterns (callback vs polling) affect link state differently
//     - State correlation between command and link levels must be validated for consistency
//     - Different execution roles (CmdInitiator vs CmdExecutor) have different state behaviors
///////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef __UT_COMMAND_STATE_H__
#define __UT_COMMAND_STATE_H__

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <vector>

#include "IOC/IOC_CmdAPI.h"
#include "IOC/IOC_CmdDesc.h"
#include "IOC/IOC_Option.h"
#include "_UT_IOC_Common.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF DUAL-STATE COMMAND TESTING FRAMEWORK============================================
/**
 * @brief Dual-State Command Testing Framework
 *
 * This framework enables comprehensive testing of command state at two levels:
 *
 * 📋 LEVEL 1: INDIVIDUAL COMMAND STATE (IOC_CmdDesc_T level)
 *    - Command Status: IOC_CMD_STATUS_PENDING → IOC_CMD_STATUS_PROCESSING → IOC_CMD_STATUS_SUCCESS/FAILED/TIMEOUT
 *    - Command Result: IOC_RESULT_SUCCESS, IOC_RESULT_CMD_EXEC_FAILED, IOC_RESULT_TIMEOUT, etc.
 *    - Command Context: Timeout handling, payload state, execution context
 *    - API: IOC_CmdDesc_getStatus(), IOC_CmdDesc_getResult()
 *
 * 🔗 LEVEL 2: LINK COMMAND EXECUTION STATE (IOC_LinkID_T level)
 *    - Link SubStates: IOC_LinkSubStateCmdInitiatorReady, IOC_LinkSubStateCmdInitiatorBusyExecCmd, etc.
 *    - Command Queue State: How many commands are pending/processing on the link
 *    - Role State: CmdInitiator activity vs CmdExecutor activity
 *    - API: IOC_getLinkState() with command-specific substates
 *
 * 🔄 STATE CORRELATION VALIDATION:
 *    - Individual command state changes must correlate with link state changes
 *    - Multiple commands on same link: independent command states, coordinated link state
 *    - State consistency across execution patterns (callback vs polling)
 *    - Error propagation between command and link state levels
 */
//======>END OF DUAL-STATE COMMAND TESTING FRAMEWORK==============================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF COMMAND STATE DATA STRUCTURES===================================================

// Maximum number of concurrent commands for testing
#define _UT_CMD_STATE_MAX_CONCURRENT_COMMANDS 10
#define _UT_CMD_STATE_MAX_CLIENTS 5
#define _UT_CMD_STATE_COMMAND_TIMEOUT_MS 5000
#define _UT_CMD_STATE_POLLING_TIMEOUT_MS 1000

/**
 * @brief Individual Command State Tracking Structure
 *        Tracks state of a single IOC_CmdDesc_T throughout its lifecycle
 */
typedef struct __CmdStateTracking {
    // Command Identity
    IOC_CmdID_T CmdID{0};
    ULONG_T SeqID{0};
    IOC_LinkID_T LinkID{IOC_ID_INVALID};

    // Command State History
    std::vector<IOC_CmdStatus_E> StatusHistory;
    std::vector<IOC_Result_T> ResultHistory;
    std::vector<std::chrono::steady_clock::time_point> StateChangeTimestamps;

    // Current State
    std::atomic<IOC_CmdStatus_E> CurrentStatus{IOC_CMD_STATUS_PENDING};
    std::atomic<IOC_Result_T> CurrentResult{IOC_RESULT_BUG};

    // State Transition Tracking
    std::atomic<bool> StatusChanged{false};
    std::atomic<int> StatusChangeCount{0};
    std::atomic<bool> CompletionDetected{false};

    // Timing Information
    std::chrono::steady_clock::time_point CreationTime;
    std::chrono::steady_clock::time_point StartTime;
    std::chrono::steady_clock::time_point CompletionTime;

    // Synchronization
    std::mutex StateMutex;
    std::condition_variable StateChangeCV;
} __CmdStateTracking_T;

/**
 * @brief Link Command Execution State Tracking Structure
 *        Tracks how IOC_LinkID_T state changes during command execution
 */
typedef struct __LinkCmdStateTracking {
    // Link Identity
    IOC_LinkID_T LinkID{IOC_ID_INVALID};
    IOC_LinkUsage_T Usage{IOC_LinkUsageUndefined};

    // Link State History
    std::vector<IOC_LinkState_T> MainStateHistory;
    std::vector<IOC_LinkSubState_T> SubStateHistory;
    std::vector<std::chrono::steady_clock::time_point> LinkStateChangeTimestamps;

    // Current Link State
    std::atomic<IOC_LinkState_T> CurrentMainState{IOC_LinkStateUndefined};
    std::atomic<IOC_LinkSubState_T> CurrentSubState{IOC_LinkSubStateDefault};

    // Command Activity Tracking
    std::atomic<int> ActiveCommandCount{0};
    std::atomic<int> TotalCommandsProcessed{0};
    std::atomic<bool> CommandActivityDetected{false};

    // State Change Detection
    std::atomic<bool> StateChanged{false};
    std::atomic<int> StateChangeCount{0};

    // Synchronization
    std::mutex LinkStateMutex;
    std::condition_variable LinkStateChangeCV;
} __LinkCmdStateTracking_T;

/**
 * @brief Dual-State Command Testing Private Data
 *        Comprehensive tracking for both individual command and link command state testing
 */
typedef struct __CmdDualStatePrivData {
    // Individual Command State Tracking
    __CmdStateTracking_T Commands[_UT_CMD_STATE_MAX_CONCURRENT_COMMANDS];
    std::atomic<int> ActiveCommandCount{0};
    std::atomic<int> TotalCommandsTracked{0};

    // Link Command State Tracking
    __LinkCmdStateTracking_T Links[_UT_CMD_STATE_MAX_CLIENTS];
    std::atomic<int> ActiveLinkCount{0};
    std::atomic<int> TotalLinksTracked{0};

    // State Correlation Tracking
    std::atomic<bool> StateCorrelationValid{true};
    std::atomic<int> CorrelationViolations{0};

    // Test Control
    std::atomic<bool> TrackingEnabled{true};
    std::atomic<bool> ShouldStop{false};

    // Error Tracking
    std::atomic<bool> ErrorOccurred{false};
    std::atomic<int> ErrorCount{0};
    IOC_Result_T LastErrorCode{IOC_RESULT_SUCCESS};

    // Global Synchronization
    std::mutex GlobalMutex;
    std::condition_variable GlobalCV;
} __CmdDualStatePrivData_T;

//======>END OF COMMAND STATE DATA STRUCTURES=====================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF COMMAND STATE VERIFICATION MACROS==============================================

/**
 * @brief Individual Command State Verification Macros
 *        Use IOC_CmdDesc_getStatus() and IOC_CmdDesc_getResult() for command-level state verification
 */

#define VERIFY_COMMAND_STATUS(pCmdDesc, expectedStatus)                                              \
    do {                                                                                             \
        ASSERT_NE(nullptr, pCmdDesc) << "Command descriptor cannot be null";                         \
        IOC_CmdStatus_E actualStatus = IOC_CmdDesc_getStatus(pCmdDesc);                              \
        ASSERT_EQ(expectedStatus, actualStatus)                                                      \
            << "Command status mismatch: expected=" << expectedStatus << ", actual=" << actualStatus \
            << ", CmdID=" << IOC_CmdDesc_getCmdID(pCmdDesc);                                         \
    } while (0)

#define VERIFY_COMMAND_RESULT(pCmdDesc, expectedResult)                                              \
    do {                                                                                             \
        ASSERT_NE(nullptr, pCmdDesc) << "Command descriptor cannot be null";                         \
        IOC_Result_T actualResult = IOC_CmdDesc_getResult(pCmdDesc);                                 \
        ASSERT_EQ(expectedResult, actualResult)                                                      \
            << "Command result mismatch: expected=" << expectedResult << ", actual=" << actualResult \
            << ", CmdID=" << IOC_CmdDesc_getCmdID(pCmdDesc);                                         \
    } while (0)

#define VERIFY_COMMAND_STATE_TRANSITION(pCmdDesc, expectedStatus, expectedResult) \
    do {                                                                          \
        VERIFY_COMMAND_STATUS(pCmdDesc, expectedStatus);                          \
        VERIFY_COMMAND_RESULT(pCmdDesc, expectedResult);                          \
    } while (0)

/**
 * @brief Link Command State Verification Macros
 *        Use IOC_getLinkState() for link-level command state verification
 */

#define VERIFY_LINK_CMD_MAIN_STATE(linkID, expectedMainState)                                         \
    do {                                                                                              \
        IOC_LinkState_T actualMainState = IOC_LinkStateUndefined;                                     \
        IOC_Result_T result = IOC_getLinkState(linkID, &actualMainState, NULL);                       \
        ASSERT_EQ(IOC_RESULT_SUCCESS, result) << "Failed to get link state for LinkID=" << linkID;    \
        ASSERT_EQ(expectedMainState, actualMainState)                                                 \
            << "Link main state mismatch for LinkID=" << linkID << ", expected=" << expectedMainState \
            << ", actual=" << actualMainState;                                                        \
    } while (0)

#define VERIFY_LINK_CMD_SUB_STATE(linkID, expectedSubState)                                            \
    do {                                                                                               \
        IOC_LinkState_T mainState = IOC_LinkStateUndefined;                                            \
        IOC_LinkSubState_T actualSubState = IOC_LinkSubStateDefault;                                   \
        IOC_Result_T result = IOC_getLinkState(linkID, &mainState, &actualSubState);                   \
        ASSERT_EQ(IOC_RESULT_SUCCESS, result) << "Failed to get link sub-state for LinkID=" << linkID; \
        ASSERT_EQ(expectedSubState, actualSubState)                                                    \
            << "Link sub-state mismatch for LinkID=" << linkID << ", expected=" << expectedSubState    \
            << ", actual=" << actualSubState;                                                          \
    } while (0)

#define VERIFY_LINK_CMD_FULL_STATE(linkID, expectedMainState, expectedSubState) \
    do {                                                                        \
        VERIFY_LINK_CMD_MAIN_STATE(linkID, expectedMainState);                  \
        VERIFY_LINK_CMD_SUB_STATE(linkID, expectedSubState);                    \
    } while (0)

/**
 * @brief Dual-State Correlation Verification Macros
 *        Verify consistency between individual command state and link command state
 */

#define VERIFY_CMD_LINK_STATE_CORRELATION(pCmdDesc, linkID, expectedCmdStatus, expectedLinkSubState) \
    do {                                                                                             \
        VERIFY_COMMAND_STATUS(pCmdDesc, expectedCmdStatus);                                          \
        VERIFY_LINK_CMD_SUB_STATE(linkID, expectedLinkSubState);                                     \
    } while (0)

#define VERIFY_CONCURRENT_CMD_ISOLATION(pCmdDesc1, pCmdDesc2, linkID)                                                  \
    do {                                                                                                               \
        IOC_CmdStatus_E status1 = IOC_CmdDesc_getStatus(pCmdDesc1);                                                    \
        IOC_CmdStatus_E status2 = IOC_CmdDesc_getStatus(pCmdDesc2);                                                    \
        IOC_LinkState_T linkMainState = IOC_LinkStateUndefined;                                                        \
        IOC_Result_T result = IOC_getLinkState(linkID, &linkMainState, NULL);                                          \
        ASSERT_EQ(IOC_RESULT_SUCCESS, result) << "Failed to get link state for concurrent command verification";       \
        printf("🔍 [ISOLATION] Cmd1 Status=%d, Cmd2 Status=%d, Link MainState=%d\n", status1, status2, linkMainState); \
    } while (0)

//======>END OF COMMAND STATE VERIFICATION MACROS================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF COMMAND STATE UTILITY FUNCTIONS================================================

/**
 * @brief Initialize dual-state command tracking data structure
 */
static void __ResetCmdDualStateTracking(__CmdDualStatePrivData_T *pPrivData) {
    if (!pPrivData) return;

    // Reset command tracking
    pPrivData->ActiveCommandCount = 0;
    pPrivData->TotalCommandsTracked = 0;

    // Reset link tracking
    pPrivData->ActiveLinkCount = 0;
    pPrivData->TotalLinksTracked = 0;

    // Reset correlation tracking
    pPrivData->StateCorrelationValid = true;
    pPrivData->CorrelationViolations = 0;

    // Reset control flags
    pPrivData->TrackingEnabled = true;
    pPrivData->ShouldStop = false;

    // Reset error tracking
    pPrivData->ErrorOccurred = false;
    pPrivData->ErrorCount = 0;
    pPrivData->LastErrorCode = IOC_RESULT_SUCCESS;

    // Initialize command tracking structures
    for (int i = 0; i < _UT_CMD_STATE_MAX_CONCURRENT_COMMANDS; i++) {
        auto &cmd = pPrivData->Commands[i];
        cmd.CmdID = 0;
        cmd.SeqID = 0;
        cmd.LinkID = IOC_ID_INVALID;
        cmd.StatusHistory.clear();
        cmd.ResultHistory.clear();
        cmd.StateChangeTimestamps.clear();
        cmd.CurrentStatus = IOC_CMD_STATUS_PENDING;
        cmd.CurrentResult = IOC_RESULT_BUG;
        cmd.StatusChanged = false;
        cmd.StatusChangeCount = 0;
        cmd.CompletionDetected = false;
    }

    // Initialize link tracking structures
    for (int i = 0; i < _UT_CMD_STATE_MAX_CLIENTS; i++) {
        auto &link = pPrivData->Links[i];
        link.LinkID = IOC_ID_INVALID;
        link.Usage = IOC_LinkUsageUndefined;
        link.MainStateHistory.clear();
        link.SubStateHistory.clear();
        link.LinkStateChangeTimestamps.clear();
        link.CurrentMainState = IOC_LinkStateUndefined;
        link.CurrentSubState = IOC_LinkSubStateDefault;
        link.ActiveCommandCount = 0;
        link.TotalCommandsProcessed = 0;
        link.CommandActivityDetected = false;
        link.StateChanged = false;
        link.StateChangeCount = 0;
    }

    printf("🔧 [SETUP] Dual-state command tracking initialized\n");
}

/**
 * @brief Track individual command state change
 */
static void __TrackCommandStateChange(__CmdDualStatePrivData_T *pPrivData, IOC_CmdDesc_pT pCmdDesc, int cmdIndex) {
    if (!pPrivData || !pCmdDesc || cmdIndex >= _UT_CMD_STATE_MAX_CONCURRENT_COMMANDS) return;

    auto &cmd = pPrivData->Commands[cmdIndex];
    std::lock_guard<std::mutex> lock(cmd.StateMutex);

    IOC_CmdStatus_E currentStatus = IOC_CmdDesc_getStatus(pCmdDesc);
    IOC_Result_T currentResult = IOC_CmdDesc_getResult(pCmdDesc);

    // Record state change if different
    if (cmd.CurrentStatus != currentStatus || cmd.CurrentResult != currentResult) {
        cmd.StatusHistory.push_back(currentStatus);
        cmd.ResultHistory.push_back(currentResult);
        cmd.StateChangeTimestamps.push_back(std::chrono::steady_clock::now());

        cmd.CurrentStatus = currentStatus;
        cmd.CurrentResult = currentResult;
        cmd.StatusChanged = true;
        cmd.StatusChangeCount++;

        if (currentStatus >= IOC_CMD_STATUS_SUCCESS) {
            cmd.CompletionDetected = true;
            cmd.CompletionTime = std::chrono::steady_clock::now();
        }

        cmd.StateChangeCV.notify_all();

        printf(
            "📋 [CMD_STATE] Cmd[%d] Status=%d→%d, Result=%d, Changes=%d\n", cmdIndex,
            (cmd.StatusHistory.size() > 1 ? cmd.StatusHistory[cmd.StatusHistory.size() - 2] : IOC_CMD_STATUS_PENDING),
            currentStatus, currentResult, cmd.StatusChangeCount.load());
    }
}

/**
 * @brief Track link command state change
 */
static void __TrackLinkCmdStateChange(__CmdDualStatePrivData_T *pPrivData, IOC_LinkID_T linkID, int linkIndex) {
    if (!pPrivData || linkIndex >= _UT_CMD_STATE_MAX_CLIENTS) return;

    auto &link = pPrivData->Links[linkIndex];
    std::lock_guard<std::mutex> lock(link.LinkStateMutex);

    IOC_LinkState_T mainState = IOC_LinkStateUndefined;
    IOC_LinkSubState_T subState = IOC_LinkSubStateDefault;
    IOC_Result_T result = IOC_getLinkState(linkID, &mainState, &subState);

    if (result == IOC_RESULT_SUCCESS) {
        // Record state change if different
        if (link.CurrentMainState != mainState || link.CurrentSubState != subState) {
            link.MainStateHistory.push_back(mainState);
            link.SubStateHistory.push_back(subState);
            link.LinkStateChangeTimestamps.push_back(std::chrono::steady_clock::now());

            link.CurrentMainState = mainState;
            link.CurrentSubState = subState;
            link.StateChanged = true;
            link.StateChangeCount++;

            link.LinkStateChangeCV.notify_all();

            printf("🔗 [LINK_STATE] Link[%d] MainState=%d, SubState=%d, Changes=%d\n", linkIndex, mainState, subState,
                   link.StateChangeCount.load());
        }
    }
}

//======>END OF COMMAND STATE UTILITY FUNCTIONS==================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF COMMAND STATE TESTING PATTERNS=================================================

/**
 * @brief Command State Testing Pattern: Individual Command Lifecycle
 *        Tests a single command's state progression through its complete lifecycle
 */
#define CMD_STATE_PATTERN_INDIVIDUAL_LIFECYCLE(testName, cmdSetup, cmdExecution, expectedFinalStatus) \
    TEST(UT_CommandState, testName) {                                                                 \
        __CmdDualStatePrivData_T privData = {};                                                       \
        __ResetCmdDualStateTracking(&privData);                                                       \
                                                                                                      \
        IOC_CmdDesc_T cmdDesc = IOC_CMDDESC_INIT_VALUE;                                               \
        cmdSetup;                                                                                     \
                                                                                                      \
        VERIFY_COMMAND_STATUS(&cmdDesc, IOC_CMD_STATUS_PENDING);                                      \
                                                                                                      \
        cmdExecution;                                                                                 \
                                                                                                      \
        VERIFY_COMMAND_STATUS(&cmdDesc, expectedFinalStatus);                                         \
        printf("✅ [%s] Individual command lifecycle verified\n", #testName);                         \
    }

/**
 * @brief Command State Testing Pattern: Link Command State Evolution
 *        Tests how link state changes during command execution
 */
#define CMD_STATE_PATTERN_LINK_EVOLUTION(testName, linkSetup, cmdExecution, expectedFinalSubState) \
    TEST(UT_CommandState, testName) {                                                              \
        __CmdDualStatePrivData_T privData = {};                                                    \
        __ResetCmdDualStateTracking(&privData);                                                    \
                                                                                                   \
        IOC_LinkID_T linkID = IOC_ID_INVALID;                                                      \
        linkSetup;                                                                                 \
                                                                                                   \
        VERIFY_LINK_CMD_SUB_STATE(linkID, IOC_LinkSubStateCmdExecutorReady);                       \
                                                                                                   \
        cmdExecution;                                                                              \
                                                                                                   \
        VERIFY_LINK_CMD_SUB_STATE(linkID, expectedFinalSubState);                                  \
        printf("✅ [%s] Link command state evolution verified\n", #testName);                      \
    }

//======>END OF COMMAND STATE TESTING PATTERNS===================================================

#endif  // __UT_COMMAND_STATE_H__
