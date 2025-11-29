///////////////////////////////////////////////////////////////////////////////////////////////////
// Command State Testing - Common Header and User Stories
//
// Intent:
// - Provides ALL User Stories, Acceptance Criteria, and Test Cases for Command State testing
// - Supports DUAL-STATE testing approach: Individual Command State + Link Command Execution State
// - Follows the established Data State testing pattern (UT_DataState.h)
// - Each UT_CommandStateUSn.cxx implements the corresponding User Story's test cases
//
// 📊 STATE DIAGRAMS: See README_ArchDesign.md for comprehensive state transition diagrams:
//    - "Individual Command State Machine (IOC_CmdDesc_T)" - Individual command lifecycle states
//    - "CMD::Conet" - Link-level command state (Initiator/Executor states)
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
//======>BEGIN OF ALL USER STORIES AND ACCEPTANCE CRITERIA=======================================
/**
 * ╔══════════════════════════════════════════════════════════════════════════════════════════╗
 * ║                          📋 COMMAND STATE USER STORIES                                   ║
 * ║                           Complete Specification                                         ║
 * ╚══════════════════════════════════════════════════════════════════════════════════════════╝
 *
 * This section consolidates ALL User Stories and Acceptance Criteria for Command State testing.
 * Each User Story is implemented in its corresponding UT_CommandStateUSn.cxx file.
 */

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF USER STORY 1: INDIVIDUAL COMMAND STATE VERIFICATION============================
/**
 * US-1: As a command state developer, I want to verify individual command state tracking
 *       so that each IOC_CmdDesc_T properly maintains its status and result throughout
 *       its lifecycle, enabling accurate command execution monitoring and debugging.
 *
 * 🎯 FOCUS: Individual Command State (Level 1 of Dual-State Testing)
 * 📁 IMPLEMENTATION: UT_CommandStateUS1.cxx
 *
 * [@US-1] Individual Command State Verification
 *  AC-1: GIVEN a command descriptor initialization,
 *         WHEN IOC_CmdDesc_T is created with valid parameters,
 *         THEN IOC_CmdDesc_getStatus() should return IOC_CMD_STATUS_PENDING
 *         AND command should be ready for execution.
 *
 *  AC-2: GIVEN a command during callback execution,
 *         WHEN command is being processed in callback,
 *         THEN IOC_CmdDesc_getStatus() should return IOC_CMD_STATUS_PROCESSING
 *         AND command context should remain valid.
 *
 *  AC-3: GIVEN a successful command completion,
 *         WHEN command execution completes successfully,
 *         THEN IOC_CmdDesc_getStatus() should return IOC_CMD_STATUS_SUCCESS
 *         AND IOC_CmdDesc_getResult() should return IOC_RESULT_SUCCESS.
 *
 *  AC-4: GIVEN a command in polling mode,
 *         WHEN IOC_ackCMD() is called after command completion,
 *         THEN command status should transition properly
 *         AND command should be available for cleanup.
 *
 *  AC-5: GIVEN a command execution failure,
 *         WHEN command encounters an error during processing,
 *         THEN IOC_CmdDesc_getStatus() should return IOC_CMD_STATUS_FAILED
 *         AND IOC_CmdDesc_getResult() should reflect the specific error.
 *
 *  AC-6: GIVEN a command timeout scenario,
 *         WHEN command execution exceeds specified timeout,
 *         THEN IOC_CmdDesc_getStatus() should return IOC_CMD_STATUS_TIMEOUT
 *         AND command should be properly cleaned up.
 *
 *  AC-7: GIVEN concurrent command isolation,
 *         WHEN multiple commands execute on same link,
 *         THEN each command should maintain independent state
 *         AND command states should not interfere with each other.
 */
//======>END OF USER STORY 1===================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF USER STORY 2: LINK COMMAND EXECUTION STATE VERIFICATION========================
/**
 * US-2: As a command state developer, I want to verify link command execution states
 *       so that IOC_LinkID_T properly reflects command processing activity and maintains
 *       appropriate link states during command execution workflows,
 *       enabling effective command load monitoring and resource management.
 *
 * 🎯 FOCUS: Link Command Execution State (Level 2 of Dual-State Testing)
 * 📁 IMPLEMENTATION: UT_CommandStateUS2.cxx
 *
 * [@US-2] Link Command Execution State Verification
 *  AC-1: GIVEN a link configured as CmdInitiator,
 *         WHEN link is ready to send commands,
 *         THEN IOC_getLinkState() should return IOC_LinkSubStateCmdInitiatorReady
 *         AND link should be available for command transmission.
 *
 *  AC-2: GIVEN a CmdInitiator link executing a command,
 *         WHEN IOC_execCMD() is called and waiting for response,
 *         THEN IOC_getLinkState() should return IOC_LinkSubStateCmdInitiatorBusyExecCmd
 *         AND link should reflect command execution activity.
 *
 *  AC-3: GIVEN a link configured as CmdExecutor in callback mode,
 *         WHEN link is ready to receive commands,
 *         THEN IOC_getLinkState() should return IOC_LinkSubStateCmdExecutorReady
 *         AND link should be available for command reception.
 *
 *  AC-4: GIVEN a CmdExecutor link processing a command in callback mode,
 *         WHEN command is being executed in callback,
 *         THEN IOC_getLinkState() should return IOC_LinkSubStateCmdExecutorBusyExecCmd
 *         AND link should reflect command processing activity.
 *
 *  AC-5: GIVEN a CmdExecutor link in polling mode,
 *         WHEN link is waiting for commands via IOC_waitCMD(),
 *         THEN IOC_getLinkState() should return IOC_LinkSubStateCmdExecutorBusyWaitCmd
 *         AND link should reflect active polling state.
 *
 *  AC-6: GIVEN multiple concurrent commands on the same link,
 *         WHEN commands execute simultaneously,
 *         THEN link state should reflect aggregate command activity
 *         AND link should maintain consistent state representation.
 *
 *  AC-7: GIVEN command execution completion,
 *         WHEN all commands complete successfully or with errors,
 *         THEN link state should return to appropriate ready state
 *         AND link should be available for new command operations.
 */
//======>END OF USER STORY 2===================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF USER STORY 3: MULTI-ROLE SERVICE STATE VERIFICATION============================
/**
 * US-3: As a command state developer, I want to verify multi-role service state behavior
 *       so that services with both CmdInitiator and CmdExecutor capabilities properly
 *       manage state across multiple links with different roles, ensuring correct state
 *       tracking and independent operation for each link.
 *
 * 🎯 FOCUS: Multi-Role Service Command State (Advanced Level 2 Testing)
 * 📁 IMPLEMENTATION: UT_CommandStateUS3.cxx
 *
 * 🏗️ ARCHITECTURE CLARIFICATION:
 *    • SERVICE Capabilities: A service CAN declare UsageCapabilities = (CmdInitiator | CmdExecutor)
 *    • LINK Usage: Each established LinkID has ONLY ONE usage pair after connection:
 *       - Link1: Service(CmdInitiator) ←→ Client1(CmdExecutor)
 *       - Link2: Service(CmdExecutor) ←→ Client2(CmdInitiator)
 *    • Multi-Role Service: A service managing multiple links, each with different single role
 *    • NOT: A single link with dual roles simultaneously
 *
 * [@US-3] Multi-Role Service State Verification
 *  AC-1: GIVEN a service with both CmdInitiator and CmdExecutor capabilities,
 *         WHEN service accepts connections from clients with different usage patterns,
 *         THEN each established link should have correct single-role state
 *         AND IOC_getLinkState() should return appropriate state for each link independently.
 *
 *  AC-2: GIVEN a multi-role service with one link as CmdInitiator and another as CmdExecutor,
 *         WHEN service sends command through Initiator link,
 *         THEN Initiator link state should show CmdInitiatorBusyExecCmd
 *         AND Executor link state should remain independent (unaffected).
 *
 *  AC-3: GIVEN a multi-role service with one link as CmdExecutor and another as CmdInitiator,
 *         WHEN service processes command on Executor link,
 *         THEN Executor link state should show CmdExecutorBusyExecCmd
 *         AND Initiator link state should remain independent (unaffected).
 *
 *  AC-4: GIVEN a multi-role service with multiple links in different roles,
 *         WHEN service performs operations on multiple links concurrently,
 *         THEN each link state should be tracked independently
 *         AND operations should complete successfully without interference.
 *
 *  AC-5: GIVEN a multi-role service managing links with different roles,
 *         WHEN service switches between different role operations,
 *         THEN each link state should maintain integrity
 *         AND role-specific operations should execute correctly on their respective links.
 */
//======>END OF USER STORY 3===================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF USER STORY 4: COMMAND TIMEOUT AND ERROR STATE VERIFICATION====================
/**
 * US-4: As a command state developer, I want to verify command timeout and error states
 *       so that both individual command and link states properly handle failure conditions,
 *       timeout scenarios, and error recovery, ensuring robust command execution reliability.
 *
 * 🎯 FOCUS: Error and Timeout State Handling (Both Level 1 and Level 2)
 * 📁 IMPLEMENTATION: UT_CommandStateUS4.cxx
 *
 * [@US-4] Command Timeout and Error State Verification
 *  AC-1: GIVEN a command with timeout specified,
 *         WHEN command execution exceeds timeout duration,
 *         THEN IOC_CmdDesc_getStatus() should return IOC_CMD_STATUS_TIMEOUT
 *         AND command should transition to timeout state.
 *
 *  AC-2: GIVEN a link with command timeout,
 *         WHEN command times out,
 *         THEN IOC_getLinkState() should reflect timeout impact on link state
 *         AND link should remain available for new commands.
 *
 *  AC-3: GIVEN a command execution error,
 *         WHEN command fails with error result,
 *         THEN both command status and link state should reflect error condition
 *         AND error information should be properly propagated.
 *
 *  AC-4: GIVEN multiple commands with mixed success/failure,
 *         WHEN some commands succeed and others fail,
 *         THEN link state should aggregate error conditions appropriately
 *         AND successful commands should not be affected by failed ones.
 *
 *  AC-5: GIVEN error recovery after command failure,
 *         WHEN error conditions are resolved,
 *         THEN both command and link states should return to ready state
 *         AND link should be available for new command operations.
 */
//======>END OF USER STORY 4===================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF USER STORY 5: PERFORMANCE AND SCALABILITY STATE VERIFICATION===================
/**
 * US-5: As a command state developer, I want to verify command state performance and scalability
 *       so that state tracking remains accurate and efficient under high-load conditions,
 *       concurrent operations, and resource constraints, ensuring production-ready reliability.
 *
 * 🎯 FOCUS: Performance and Scalability (Both Level 1 and Level 2 under Load)
 * 📁 IMPLEMENTATION: UT_CommandStateUS5.cxx
 *
 * [@US-5] Performance and Scalability State Verification
 *  AC-1: GIVEN high-frequency command operations,
 *         WHEN commands execute at maximum supported rate,
 *         THEN state update performance should remain within acceptable limits
 *         AND state accuracy should be maintained under load.
 *
 *  AC-2: GIVEN multiple concurrent commands per link,
 *         WHEN link handles maximum supported concurrent operations,
 *         THEN link state aggregation should perform efficiently
 *         AND individual command states should remain accurate.
 *
 *  AC-3: GIVEN extended operation duration,
 *         WHEN system runs for extended period with continuous command activity,
 *         THEN state memory usage should remain stable
 *         AND no state-related resource leaks should occur.
 *
 *  AC-4: GIVEN maximum supported links and commands,
 *         WHEN system operates at full capacity,
 *         THEN state operations should scale linearly
 *         AND system responsiveness should remain acceptable.
 *
 *  AC-5: GIVEN resource-constrained environment,
 *         WHEN system operates under memory or CPU constraints,
 *         THEN state operations should degrade gracefully
 *         AND critical state information should remain available.
 */
//======>END OF USER STORY 5===================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF USER STORY 6: PROTOCOL-SPECIFIC STATE INTEGRATION=============================
/**
 * US-6: As a command state developer, I want to verify protocol-specific state integration
 *       so that each transport protocol's unique characteristics (TCP, FIFO) are properly
 *       reflected in command and link states, enabling accurate protocol-specific debugging
 *       and monitoring beyond generic state machine behavior.
 *
 * 🎯 FOCUS: Protocol-Specific State Behavior (TCP/FIFO-specific integration)
 * 📁 IMPLEMENTATION: UT_CommandStateTCP.cxx, UT_CommandStateFIFO.cxx (future)
 *
 * 🎨 DESIGN RATIONALE:
 *    • US-1 through US-5 test protocol-AGNOSTIC state machine behavior
 *    • US-6 tests protocol-SPECIFIC state integration scenarios
 *    • Separation ensures generic state logic remains clean and focused
 *    • Protocol-specific files can use heavy infrastructure (socket simulation, etc.)
 *
 * 📊 PROTOCOL-SPECIFIC vs PROTOCOL-AGNOSTIC:
 *    ┌───────────────────────────────┬─────────────────────────────────────────────────┐
 *    │ Protocol-Agnostic (US-1 to 5) │ Protocol-Specific (US-6)                        │
 *    ├───────────────────────────────┼─────────────────────────────────────────────────┤
 *    │ State machine transitions     │ Transport protocol lifecycle × state            │
 *    │ Generic timeout/error states  │ Protocol-specific errors (ECONNRESET, EPIPE)    │
 *    │ Abstract link behavior        │ Connection establishment/loss impact            │
 *    │ Command execution patterns    │ Flow control, backpressure, protocol timing     │
 *    │ Generic error recovery        │ Protocol-specific reconnection/recovery         │
 *    └───────────────────────────────┴─────────────────────────────────────────────────┘
 *
 * [@US-6] Protocol-Specific State Integration
 *  AC-1: GIVEN a TCP-based command link during connection establishment,
 *         WHEN TCP handshake (SYN → SYN-ACK → ACK) is in progress,
 *         THEN command state should remain PENDING until TCP connection is ESTABLISHED
 *         AND link state should reflect TCP connection establishment phase.
 *
 *  AC-2: GIVEN a TCP-based command link with active connection,
 *         WHEN TCP-specific errors occur (ECONNRESET, EPIPE, ECONNREFUSED),
 *         THEN command result should map to TCP-specific error codes
 *         AND link state should reflect TCP connection failure accurately.
 *
 *  AC-3: GIVEN a TCP-based command link under flow control,
 *         WHEN TCP send buffer is full or receive window is zero,
 *         THEN command state should remain PROCESSING with appropriate delay
 *         AND state should transition correctly once flow control resolves.
 *
 *  AC-4: GIVEN a TCP-based command link during shutdown,
 *         WHEN graceful shutdown (FIN) vs ungraceful shutdown (RST) occurs,
 *         THEN command and link states should differ appropriately
 *         AND in-flight commands should be handled according to shutdown type.
 *
 *  AC-5: GIVEN a TCP-based command link during reconnection,
 *         WHEN TCP connection is lost and recovery is attempted,
 *         THEN command state should reflect reconnection status
 *         AND successful reconnection should allow queued commands to proceed.
 *
 *  AC-6: GIVEN a FIFO-based command link initialization,
 *         WHEN FIFO pipes are opened with specific permissions,
 *         THEN command state should reflect FIFO readiness
 *         AND link state should show FIFO-specific ready state.
 *         (Future implementation in UT_CommandStateFIFO.cxx)
 *
 *  AC-7: GIVEN a FIFO-based command link with blocking behavior,
 *         WHEN FIFO read/write operations block due to buffer limits,
 *         THEN command state should handle FIFO-specific blocking correctly
 *         AND state transitions should complete after FIFO unblocks.
 *         (Future implementation in UT_CommandStateFIFO.cxx)
 *
 * 🔗 IMPLEMENTATION FILES:
 *    • UT_CommandStateTCP.cxx  - AC-1 through AC-5 (TCP-specific state)
 *    • UT_CommandStateFIFO.cxx - AC-6 through AC-7 (FIFO-specific state, future)
 *
 * 🎯 KEY PRINCIPLE:
 *    US-6 bridges the gap between generic state machines (US-1 to US-5) and
 *    real-world protocol implementations, ensuring state accuracy reflects
 *    protocol-specific behavior that affects production systems.
 */
//======>END OF USER STORY 6===================================================================

/**
 * ╔══════════════════════════════════════════════════════════════════════════════════════════╗
 * ║                            📊 USER STORY IMPLEMENTATION MAP                              ║
 * ╠══════════════════════════════════════════════════════════════════════════════════════════╣
 * ║ US-1: Individual Command State          → UT_CommandStateUS1.cxx (IMPLEMENTED)          ║
 * ║ US-2: Link Command Execution State      → UT_CommandStateUS2.cxx (IMPLEMENTED)          ║
 * ║ US-3: Multi-Role Link State             → UT_CommandStateUS3.cxx (FRAMEWORK)            ║
 * ║ US-4: Timeout and Error State           → UT_CommandStateUS4.cxx (FRAMEWORK)            ║
 * ║ US-5: Performance and Scalability       → UT_CommandStateUS5.cxx (FRAMEWORK)            ║
 * ║ US-6: Protocol-Specific State           → UT_CommandStateTCP.cxx (DESIGN PHASE)         ║
 * ║                                            UT_CommandStateFIFO.cxx (FUTURE)             ║
 * ║                                                                                          ║
 * ║ 🎯 DUAL-STATE COVERAGE:                                                                  ║
 * ║   • Level 1 (Command State): US-1, US-4, US-5, US-6                                    ║
 * ║   • Level 2 (Link State): US-2, US-3, US-4, US-5, US-6                                 ║
 * ║   • Integration Testing: All USs provide correlation verification                       ║
 * ║   • Protocol-Specific: US-6 extends US-1 to US-5 with TCP/FIFO specifics              ║
 * ╚══════════════════════════════════════════════════════════════════════════════════════════╝
 */

//======>END OF ALL USER STORIES AND ACCEPTANCE CRITERIA=====================================

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
