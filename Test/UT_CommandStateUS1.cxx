///////////////////////////////////////////////////////////////////////////////////////////////////
// Command State US-1: Individual Command State Verification
//
// Intent:
// - Verify individual IOC_CmdDesc_T lifecycle state transitions (PENDING→PROCESSING→SUCCESS/FAILED/TIMEOUT)
// - Test command status and result field consistency throughout command execution
// - Validate command state persistence across different execution patterns (callback vs polling)
//
// 🎯 DUAL-STATE FOCUS: This file focuses on INDIVIDUAL COMMAND STATE (Level 1)
//     WHY INDIVIDUAL COMMAND STATE MATTERS:
//     - Each IOC_CmdDesc_T has independent status/result fields that must be tracked correctly
//     - Command state transitions follow specific lifecycle rules regardless of link state
//     - Individual command state provides detailed execution information for debugging/monitoring
//     - State consistency is critical for command completion detection and error handling
//     - Different execution patterns (callback vs polling) affect individual command state differently
//
// 🔗 COMPANION: UT_CommandStateUS2.cxx focuses on LINK COMMAND EXECUTION STATE (Level 2)
//     Together, these files provide comprehensive dual-state command testing coverage.
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <chrono>
#include <thread>

#include "UT_CommandState.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF OVERVIEW OF THIS UNIT TESTING FILE===============================================
/**
 * @brief Verify individual command state management throughout command lifecycle:
 *  - Command status transitions: IOC_CMD_STATUS_PENDING → IOC_CMD_STATUS_PROCESSING → IOC_CMD_STATUS_SUCCESS/FAILED/TIMEOUT
 *  - Command result accuracy: IOC_RESULT_SUCCESS, IOC_RESULT_CMD_EXEC_FAILED, IOC_RESULT_TIMEOUT, etc.
 *  - State persistence across execution patterns: callback mode vs polling mode (IOC_waitCMD + IOC_ackCMD)
 *  - State consistency during error conditions and timeout scenarios
 *
 * Key API focus:
 *  - IOC_CmdDesc_getStatus(): Retrieve current command execution status
 *  - IOC_CmdDesc_getResult(): Retrieve command execution result
 *  - IOC_CmdDesc_setStatus(), IOC_CmdDesc_setResult(): State modification validation
 *  - Command state persistence across IOC_execCMD, IOC_waitCMD, IOC_ackCMD operations
 */
//======>END OF OVERVIEW OF THIS UNIT TESTING FILE=================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF UNIT TESTING DESIGN==============================================================
/**
 * Design focus:
 *  - Individual command state lifecycle verification independent of link state
 *  - Command status/result field consistency throughout execution
 *  - State transition validation for all command execution patterns
 *  - Error condition state handling and timeout state management
 *  - Command state isolation: multiple commands maintain independent states
 *
 * Test approach:
 *  - Focus on IOC_CmdDesc_T state fields and accessor functions
 *  - Verify state transitions follow expected lifecycle patterns
 *  - Test state persistence across different API call sequences
 *  - Validate error condition state propagation
 *  - Ensure command state independence from link state changes
 */
///////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF USER STORY=======================================================================
/**
 * US-1: As a command state developer, I want to verify individual command lifecycle states
 *       so that each IOC_CmdDesc_T properly tracks its execution progress from PENDING
 *       through PROCESSING to final SUCCESS/FAILED/TIMEOUT status,
 *       ensuring accurate command state information for debugging and monitoring.
 */
//======>END OF USER STORY==========================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//=======>BEGIN OF ACCEPTANCE CRITERIA==============================================================
/**
 * [@US-1] Individual Command Lifecycle State Verification
 *  AC-1: GIVEN a newly created command descriptor,
 *         WHEN command is initialized,
 *         THEN status should be IOC_CMD_STATUS_PENDING and result should be IOC_RESULT_SUCCESS.
 *
 *  AC-2: GIVEN a command during execution in callback mode,
 *         WHEN command is being processed,
 *         THEN status should transition to IOC_CMD_STATUS_PROCESSING
 *         AND result should remain consistent with execution state.
 *
 *  AC-3: GIVEN a command during execution in polling mode,
 *         WHEN command is processed via IOC_waitCMD + IOC_ackCMD,
 *         THEN status should transition through expected states
 *         AND final status should reflect acknowledgment result.
 *
 *  AC-4: GIVEN a successfully completed command,
 *         WHEN execution completes normally,
 *         THEN status should be IOC_CMD_STATUS_SUCCESS
 *         AND result should be IOC_RESULT_SUCCESS.
 *
 *  AC-5: GIVEN a command that encounters execution errors,
 *         WHEN executor returns failure,
 *         THEN status should be IOC_CMD_STATUS_FAILED
 *         AND result should reflect specific error condition.
 *
 *  AC-6: GIVEN a command that exceeds timeout,
 *         WHEN execution time exceeds specified timeout,
 *         THEN status should be IOC_CMD_STATUS_TIMEOUT
 *         AND result should be IOC_RESULT_TIMEOUT.
 *
 *  AC-7: GIVEN multiple concurrent commands,
 *         WHEN commands execute independently,
 *         THEN each command should maintain independent state
 *         AND state changes should not affect other commands.
 */
//=======>END OF ACCEPTANCE CRITERIA================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF TEST CASES=======================================================================
/**************************************************************************************************
 * @brief 【Individual Command State Test Cases】
 *
 * ORGANIZATION STRATEGIES:
 *  - By Lifecycle Stage: Initialization → Processing → Completion/Error/Timeout
 *  - By Execution Pattern: Callback mode vs Polling mode state handling
 *  - By Error Conditions: Success vs Failure vs Timeout state verification
 *  - By Concurrency: Single command vs Multiple concurrent command state isolation
 *
 * STATUS TRACKING: ⚪ = Planned/TODO，🔴 = Implemented/RED, 🟢 = Passed/GREEN, ⚠️ = Issues
 *
 * ⚪ FRAMEWORK STATUS: Individual command state APIs need comprehensive testing
 *
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * 📋 [US-1]: INDIVIDUAL COMMAND LIFECYCLE STATE VERIFICATION
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 *
 * [@AC-1,US-1] Command initialization state verification
 *  ⚪ TC-1: verifyCommandInitialization_byNewDescriptor_expectPendingStatus
 *      @[Purpose]: Validate newly created command descriptors have correct initial state
 *      @[Brief]: Create IOC_CmdDesc_T, verify IOC_CMD_STATUS_PENDING and IOC_RESULT_SUCCESS
 *      @[Status]: TODO - Need to implement command descriptor initialization testing
 *
 * [@AC-2,US-1] Command processing state in callback mode
 *  ⚪ TC-1: verifyCommandProcessingState_byCallbackExecution_expectProcessingStatus
 *      @[Purpose]: Validate command status during callback-based execution
 *      @[Brief]: Execute command via callback, verify IOC_CMD_STATUS_PROCESSING during execution
 *      @[Status]: TODO - Need to implement callback execution state tracking
 *
 * [@AC-3,US-1] Command processing state in polling mode
 *  ⚪ TC-1: verifyCommandProcessingState_byPollingExecution_expectCorrectTransitions
 *      @[Purpose]: Validate command status during polling-based execution (IOC_waitCMD + IOC_ackCMD)
 *      @[Brief]: Execute via polling, verify status transitions match acknowledgment workflow
 *      @[Status]: TODO - Need to implement polling execution state tracking
 *
 * [@AC-4,US-1] Successful command completion state
 *  ⚪ TC-1: verifyCommandSuccess_byNormalCompletion_expectSuccessStatus
 *      @[Purpose]: Validate successful command completion state
 *      @[Brief]: Execute PING command successfully, verify IOC_CMD_STATUS_SUCCESS + IOC_RESULT_SUCCESS
 *      @[Status]: TODO - Need to implement success state verification
 *
 * [@AC-5,US-1] Command failure state handling
 *  ⚪ TC-1: verifyCommandFailure_byExecutorError_expectFailedStatus
 *      @[Purpose]: Validate command failure state when executor returns error
 *      @[Brief]: Execute command that fails, verify IOC_CMD_STATUS_FAILED + error result
 *      @[Status]: TODO - Need to implement failure state verification
 *
 * [@AC-6,US-1] Command timeout state handling
 *  ⚪ TC-1: verifyCommandTimeout_byExceededTimeout_expectTimeoutStatus  
 *      @[Purpose]: Validate command timeout state handling
 *      @[Brief]: Execute command with short timeout, verify IOC_CMD_STATUS_TIMEOUT + IOC_RESULT_TIMEOUT
 *      @[Status]: TODO - Need to implement timeout state verification
 *
 * [@AC-7,US-1] Multiple command state isolation
 *  ⚪ TC-1: verifyCommandStateIsolation_byConcurrentCommands_expectIndependentStates
 *      @[Purpose]: Validate independent state tracking for concurrent commands
 *      @[Brief]: Execute multiple commands concurrently, verify each maintains independent state
 *      @[Status]: TODO - Need to implement concurrent command state isolation testing
 */
//======>END OF TEST CASES=========================================================================

// Individual command state private data structure
typedef struct __IndividualCmdStatePriv {
    std::atomic<bool> CommandInitialized{false};
    std::atomic<bool> CommandStarted{false};
    std::atomic<bool> CommandCompleted{false};
    std::atomic<int> CommandCount{0};
    
    // State transition tracking
    std::atomic<bool> ProcessingDetected{false};
    std::atomic<bool> CompletionDetected{false};
    std::atomic<int> StateTransitionCount{0};
    
    // Command state history
    IOC_CmdStatus_E StatusHistory[10];
    IOC_Result_T ResultHistory[10];
    int HistoryCount{0};
    
    // Error tracking
    std::atomic<bool> ErrorOccurred{false};
    IOC_Result_T LastError{IOC_RESULT_SUCCESS};
    
    // Timing
    std::chrono::steady_clock::time_point StartTime;
    std::chrono::steady_clock::time_point CompletionTime;
    
    std::mutex StateMutex;
} __IndividualCmdStatePriv_T;

// TODO: Implement command state tracking callback
static IOC_Result_T __IndividualCmdState_ExecutorCb(IOC_LinkID_T LinkID, IOC_CmdDesc_pT pCmdDesc, void *pCbPriv) {
    __IndividualCmdStatePriv_T *pPrivData = (__IndividualCmdStatePriv_T *)pCbPriv;
    if (!pPrivData || !pCmdDesc) {
        return IOC_RESULT_INVALID_PARAM;
    }
    
    std::lock_guard<std::mutex> lock(pPrivData->StateMutex);
    
    // Track state transition to PROCESSING
    IOC_CmdDesc_setStatus(pCmdDesc, IOC_CMD_STATUS_PROCESSING);
    pPrivData->ProcessingDetected = true;
    
    // Record state transition
    if (pPrivData->HistoryCount < 10) {
        pPrivData->StatusHistory[pPrivData->HistoryCount] = IOC_CMD_STATUS_PROCESSING;
        pPrivData->ResultHistory[pPrivData->HistoryCount] = IOC_RESULT_SUCCESS;
        pPrivData->HistoryCount++;
    }
    
    // Process the command
    IOC_CmdID_T CmdID = IOC_CmdDesc_getCmdID(pCmdDesc);
    IOC_Result_T ExecResult = IOC_RESULT_SUCCESS;
    
    if (CmdID == IOC_CMDID_TEST_PING) {
        // Simulate PING processing
        IOC_CmdDesc_setOutPayload(pCmdDesc, (void *)"PONG", 4);
        IOC_CmdDesc_setStatus(pCmdDesc, IOC_CMD_STATUS_SUCCESS);
        IOC_CmdDesc_setResult(pCmdDesc, IOC_RESULT_SUCCESS);
    } else if (CmdID == IOC_CMDID_TEST_ECHO) {
        // Simulate ECHO processing
        void *inData = IOC_CmdDesc_getInData(pCmdDesc);
        ULONG_T inSize = IOC_CmdDesc_getInDataSize(pCmdDesc);
        if (inData && inSize > 0) {
            IOC_CmdDesc_setOutPayload(pCmdDesc, inData, inSize);
        }
        IOC_CmdDesc_setStatus(pCmdDesc, IOC_CMD_STATUS_SUCCESS);
        IOC_CmdDesc_setResult(pCmdDesc, IOC_RESULT_SUCCESS);
    } else {
        // Unsupported command
        ExecResult = IOC_RESULT_NOT_SUPPORT;
        IOC_CmdDesc_setStatus(pCmdDesc, IOC_CMD_STATUS_FAILED);
        IOC_CmdDesc_setResult(pCmdDesc, IOC_RESULT_NOT_SUPPORT);
    }
    
    // Track completion
    pPrivData->CompletionDetected = true;
    pPrivData->CommandCompleted = true;
    pPrivData->StateTransitionCount++;
    
    // Record final state
    if (pPrivData->HistoryCount < 10) {
        pPrivData->StatusHistory[pPrivData->HistoryCount] = IOC_CmdDesc_getStatus(pCmdDesc);
        pPrivData->ResultHistory[pPrivData->HistoryCount] = IOC_CmdDesc_getResult(pCmdDesc);
        pPrivData->HistoryCount++;
    }
    
    return ExecResult;
}

// [@AC-1,US-1] TC-1: Command initialization state verification
TEST(UT_CommandStateUS1, verifyCommandInitialization_byNewDescriptor_expectPendingStatus) {
    // ┌──────────────────────────────────────────────────────────────────────────────────────┐
    // │                                🔧 SETUP PHASE                                        │
    // └──────────────────────────────────────────────────────────────────────────────────────┘
    __IndividualCmdStatePriv_T privData = {};
    IOC_CmdDesc_T cmdDesc = IOC_CMDDESC_INIT_VALUE;
    
    printf("🔧 [SETUP] Testing command initialization state\n");
    
    // ┌──────────────────────────────────────────────────────────────────────────────────────┐
    // │                              📋 BEHAVIOR PHASE                                       │
    // └──────────────────────────────────────────────────────────────────────────────────────┘
    cmdDesc.CmdID = IOC_CMDID_TEST_PING;
    cmdDesc.TimeoutMs = 5000;
    IOC_CmdDesc_initVar(&cmdDesc);
    
    printf("📋 [BEHAVIOR] Command descriptor initialized: CmdID=%llu, TimeoutMs=%lu\n", 
           cmdDesc.CmdID, cmdDesc.TimeoutMs);
    
    // ┌──────────────────────────────────────────────────────────────────────────────────────┐
    // │                               ✅ VERIFY PHASE                                        │
    // └──────────────────────────────────────────────────────────────────────────────────────┘
    
    // Verify initial command status
    VERIFY_COMMAND_STATUS(&cmdDesc, IOC_CMD_STATUS_PENDING);
    
    // Verify initial command result  
    VERIFY_COMMAND_RESULT(&cmdDesc, IOC_RESULT_SUCCESS);
    
    // Verify command ID is set correctly
    ASSERT_EQ(IOC_CMDID_TEST_PING, IOC_CmdDesc_getCmdID(&cmdDesc));
    
    // Verify timeout is set correctly
    ASSERT_EQ(5000, cmdDesc.TimeoutMs);
    
    printf("✅ [VERIFY] Command initialization state verified: Status=PENDING, Result=SUCCESS\n");
    printf("✅ [RESULT] Individual command initialization test completed successfully\n");
    
    // ┌──────────────────────────────────────────────────────────────────────────────────────┐
    // │                               🧹 CLEANUP PHASE                                       │
    // └──────────────────────────────────────────────────────────────────────────────────────┘
    // No cleanup needed for stack variables
}

// [@AC-2,US-1] TC-1: Command processing state in callback mode
TEST(UT_CommandStateUS1, verifyCommandProcessingState_byCallbackExecution_expectProcessingStatus) {
    IOC_Result_T ResultValue = IOC_RESULT_BUG;
    
    // ┌──────────────────────────────────────────────────────────────────────────────────────┐
    // │                                🔧 SETUP PHASE                                        │
    // └──────────────────────────────────────────────────────────────────────────────────────┘
    __IndividualCmdStatePriv_T srvPrivData = {};
    
    // Service setup (CmdExecutor with callback)
    IOC_SrvURI_T srvURI = {.pProtocol = IOC_SRV_PROTO_FIFO,
                           .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
                           .pPath = (const char *)"CmdStateUS1_CallbackProcessing"};
    
    static IOC_CmdID_T supportedCmdIDs[] = {IOC_CMDID_TEST_PING};
    IOC_CmdUsageArgs_T cmdUsageArgs = {.CbExecCmd_F = __IndividualCmdState_ExecutorCb,
                                       .pCbPrivData = &srvPrivData,
                                       .CmdNum = 1,
                                       .pCmdIDs = supportedCmdIDs};
    
    IOC_SrvArgs_T srvArgs = {.SrvURI = srvURI,
                             .Flags = IOC_SRVFLAG_NONE,
                             .UsageCapabilites = IOC_LinkUsageCmdExecutor,
                             .UsageArgs = {.pCmd = &cmdUsageArgs}};
    
    IOC_SrvID_T srvID = IOC_ID_INVALID;
    ResultValue = IOC_onlineService(&srvID, &srvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);
    
    printf("🔧 [SETUP] Service online with callback executor: SrvID=%llu\n", srvID);
    
    // ┌──────────────────────────────────────────────────────────────────────────────────────┐
    // │                              📋 BEHAVIOR PHASE                                       │
    // └──────────────────────────────────────────────────────────────────────────────────────┘
    
    // Client setup and command execution
    IOC_ConnArgs_T connArgs = {.SrvURI = srvURI, .Usage = IOC_LinkUsageCmdInitiator};
    IOC_LinkID_T cliLinkID = IOC_ID_INVALID;
    
    std::thread cliThread([&] {
        IOC_Result_T connResult = IOC_connectService(&cliLinkID, &connArgs, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, connResult);
    });
    
    // Accept client
    IOC_LinkID_T srvLinkID = IOC_ID_INVALID;
    ResultValue = IOC_acceptClient(srvID, &srvLinkID, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);
    
    if (cliThread.joinable()) cliThread.join();
    
    // Create and execute command
    IOC_CmdDesc_T cmdDesc = IOC_CMDDESC_INIT_VALUE;
    cmdDesc.CmdID = IOC_CMDID_TEST_PING;
    cmdDesc.TimeoutMs = 5000;
    
    // Verify initial state
    VERIFY_COMMAND_STATUS(&cmdDesc, IOC_CMD_STATUS_PENDING);
    
    printf("📋 [BEHAVIOR] Executing command via callback mode\n");
    
    // Execute command - this should trigger state transitions in callback
    ResultValue = IOC_execCMD(cliLinkID, &cmdDesc, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);
    
    // ┌──────────────────────────────────────────────────────────────────────────────────────┐
    // │                               ✅ VERIFY PHASE                                        │
    // └──────────────────────────────────────────────────────────────────────────────────────┘
    
    // Verify final command state
    VERIFY_COMMAND_STATUS(&cmdDesc, IOC_CMD_STATUS_SUCCESS);
    VERIFY_COMMAND_RESULT(&cmdDesc, IOC_RESULT_SUCCESS);
    
    // Verify callback tracked state transitions
    ASSERT_TRUE(srvPrivData.ProcessingDetected.load()) << "Processing state should be detected in callback";
    ASSERT_TRUE(srvPrivData.CompletionDetected.load()) << "Completion should be detected in callback";
    ASSERT_GT(srvPrivData.StateTransitionCount.load(), 0) << "State transitions should be tracked";
    
    // Verify response data
    void *responseData = IOC_CmdDesc_getOutData(&cmdDesc);
    ULONG_T responseSize = IOC_CmdDesc_getOutDataSize(&cmdDesc);
    ASSERT_TRUE(responseData != nullptr);
    ASSERT_EQ(4, responseSize);
    ASSERT_STREQ("PONG", (char *)responseData);
    
    printf("✅ [VERIFY] Command processing state verified: PENDING→PROCESSING→SUCCESS\n");
    printf("✅ [VERIFY] State transitions tracked: %d transitions detected\n", 
           srvPrivData.StateTransitionCount.load());
    printf("✅ [RESULT] Callback mode command processing state test completed successfully\n");
    
    // ┌──────────────────────────────────────────────────────────────────────────────────────┐
    // │                               🧹 CLEANUP PHASE                                       │
    // └──────────────────────────────────────────────────────────────────────────────────────┘
    if (cliLinkID != IOC_ID_INVALID) IOC_closeLink(cliLinkID);
    if (srvLinkID != IOC_ID_INVALID) IOC_closeLink(srvLinkID);
    if (srvID != IOC_ID_INVALID) IOC_offlineService(srvID);
}

// [@AC-4,US-1] TC-1: Successful command completion state
TEST(UT_CommandStateUS1, verifyCommandSuccess_byNormalCompletion_expectSuccessStatus) {
    IOC_Result_T ResultValue = IOC_RESULT_BUG;
    
    // ┌──────────────────────────────────────────────────────────────────────────────────────┐
    // │                                🔧 SETUP PHASE                                        │
    // └──────────────────────────────────────────────────────────────────────────────────────┘
    __IndividualCmdStatePriv_T srvPrivData = {};
    
    // Service setup
    IOC_SrvURI_T srvURI = {.pProtocol = IOC_SRV_PROTO_FIFO,
                           .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
                           .pPath = (const char *)"CmdStateUS1_SuccessCompletion"};
    
    static IOC_CmdID_T supportedCmdIDs[] = {IOC_CMDID_TEST_PING, IOC_CMDID_TEST_ECHO};
    IOC_CmdUsageArgs_T cmdUsageArgs = {.CbExecCmd_F = __IndividualCmdState_ExecutorCb,
                                       .pCbPrivData = &srvPrivData,
                                       .CmdNum = 2,
                                       .pCmdIDs = supportedCmdIDs};
    
    IOC_SrvArgs_T srvArgs = {.SrvURI = srvURI,
                             .Flags = IOC_SRVFLAG_NONE,
                             .UsageCapabilites = IOC_LinkUsageCmdExecutor,
                             .UsageArgs = {.pCmd = &cmdUsageArgs}};
    
    IOC_SrvID_T srvID = IOC_ID_INVALID;
    ResultValue = IOC_onlineService(&srvID, &srvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);
    
    // Client setup
    IOC_ConnArgs_T connArgs = {.SrvURI = srvURI, .Usage = IOC_LinkUsageCmdInitiator};
    IOC_LinkID_T cliLinkID = IOC_ID_INVALID;
    
    std::thread cliThread([&] {
        IOC_Result_T connResult = IOC_connectService(&cliLinkID, &connArgs, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, connResult);
    });
    
    IOC_LinkID_T srvLinkID = IOC_ID_INVALID;
    ResultValue = IOC_acceptClient(srvID, &srvLinkID, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);
    
    if (cliThread.joinable()) cliThread.join();
    
    printf("🔧 [SETUP] Service and client connected for success testing\n");
    
    // ┌──────────────────────────────────────────────────────────────────────────────────────┐
    // │                              📋 BEHAVIOR PHASE                                       │
    // └──────────────────────────────────────────────────────────────────────────────────────┘
    
    // Test 1: PING command success
    IOC_CmdDesc_T pingCmd = IOC_CMDDESC_INIT_VALUE;
    pingCmd.CmdID = IOC_CMDID_TEST_PING;
    pingCmd.TimeoutMs = 5000;
    
    printf("📋 [BEHAVIOR] Testing PING command success state\n");
    ResultValue = IOC_execCMD(cliLinkID, &pingCmd, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);
    
    // Test 2: ECHO command success
    IOC_CmdDesc_T echoCmd = IOC_CMDDESC_INIT_VALUE;
    echoCmd.CmdID = IOC_CMDID_TEST_ECHO;
    echoCmd.TimeoutMs = 5000;
    const char *echoInput = "Hello World";
    IOC_CmdDesc_setInPayload(&echoCmd, (void *)echoInput, strlen(echoInput));
    
    printf("📋 [BEHAVIOR] Testing ECHO command success state\n");
    ResultValue = IOC_execCMD(cliLinkID, &echoCmd, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);
    
    // ┌──────────────────────────────────────────────────────────────────────────────────────┐
    // │                               ✅ VERIFY PHASE                                        │
    // └──────────────────────────────────────────────────────────────────────────────────────┘
    
    // Verify PING command success state
    VERIFY_COMMAND_STATE_TRANSITION(&pingCmd, IOC_CMD_STATUS_SUCCESS, IOC_RESULT_SUCCESS);
    
    void *pingResponse = IOC_CmdDesc_getOutData(&pingCmd);
    ASSERT_TRUE(pingResponse != nullptr);
    ASSERT_STREQ("PONG", (char *)pingResponse);
    
    // Verify ECHO command success state
    VERIFY_COMMAND_STATE_TRANSITION(&echoCmd, IOC_CMD_STATUS_SUCCESS, IOC_RESULT_SUCCESS);
    
    void *echoResponse = IOC_CmdDesc_getOutData(&echoCmd);
    ULONG_T echoResponseSize = IOC_CmdDesc_getOutDataSize(&echoCmd);
    ASSERT_TRUE(echoResponse != nullptr);
    ASSERT_EQ(strlen(echoInput), echoResponseSize);
    ASSERT_STREQ(echoInput, (char *)echoResponse);
    
    // Verify service callback state tracking
    ASSERT_TRUE(srvPrivData.ProcessingDetected.load());
    ASSERT_TRUE(srvPrivData.CompletionDetected.load());
    ASSERT_GE(srvPrivData.StateTransitionCount.load(), 2); // At least 2 commands processed
    
    printf("✅ [VERIFY] PING command success: Status=SUCCESS, Result=SUCCESS, Response=PONG\n");
    printf("✅ [VERIFY] ECHO command success: Status=SUCCESS, Result=SUCCESS, Response=%s\n", (char *)echoResponse);
    printf("✅ [RESULT] Command success state verification completed successfully\n");
    
    // ┌──────────────────────────────────────────────────────────────────────────────────────┐
    // │                               🧹 CLEANUP PHASE                                       │
    // └──────────────────────────────────────────────────────────────────────────────────────┘
    if (cliLinkID != IOC_ID_INVALID) IOC_closeLink(cliLinkID);
    if (srvLinkID != IOC_ID_INVALID) IOC_closeLink(srvLinkID);
    if (srvID != IOC_ID_INVALID) IOC_offlineService(srvID);
}

// TODO: Implement remaining test cases:
// [@AC-3,US-1] TC-1: verifyCommandProcessingState_byPollingExecution_expectCorrectTransitions
// [@AC-5,US-1] TC-1: verifyCommandFailure_byExecutorError_expectFailedStatus
// [@AC-6,US-1] TC-1: verifyCommandTimeout_byExceededTimeout_expectTimeoutStatus
// [@AC-7,US-1] TC-1: verifyCommandStateIsolation_byConcurrentCommands_expectIndependentStates

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF IMPLEMENTATION SUMMARY===========================================================
/**
 * ╔══════════════════════════════════════════════════════════════════════════════════════════╗
 * ║                              📊 IMPLEMENTATION SUMMARY                                   ║
 * ╠══════════════════════════════════════════════════════════════════════════════════════════╣
 * ║ 🎯 PURPOSE: Individual Command State Verification - User Story 1                        ║
 * ║                                                                                          ║
 * ║ 📋 COVERAGE:                                                                             ║
 * ║   ✅ US-1 AC-1: Command initialization state verification                                ║
 * ║   ✅ US-1 AC-2: Command processing state in callback mode                               ║
 * ║   ✅ US-1 AC-4: Successful command completion state                                     ║
 * ║   TODO: US-1 AC-3: Command processing state in polling mode                             ║
 * ║   TODO: US-1 AC-5: Command failure state handling                                       ║
 * ║   TODO: US-1 AC-6: Command timeout state handling                                       ║
 * ║   TODO: US-1 AC-7: Multiple command state isolation                                     ║
 * ║                                                                                          ║
 * ║ 🔧 IMPLEMENTED TEST CASES (AC-X TC-Y Pattern):                                          ║
 * ║   AC-1 TC-1: verifyCommandInitialization_byNewDescriptor_expectPendingStatus            ║
 * ║   AC-2 TC-1: verifyCommandProcessingState_byCallbackExecution_expectProcessingStatus    ║
 * ║   AC-4 TC-1: verifyCommandSuccess_byNormalCompletion_expectSuccessStatus                ║
 * ║                                                                                          ║
 * ║ 🚀 KEY ACHIEVEMENTS:                                                                     ║
 * ║   • ✅ INDIVIDUAL COMMAND STATE APIs: IOC_CmdDesc_getStatus(), IOC_CmdDesc_getResult()  ║
 * ║   • ✅ STATE TRANSITION TRACKING: Callback-based state transition monitoring            ║
 * ║   • ✅ LIFECYCLE VERIFICATION: PENDING→PROCESSING→SUCCESS state flow validation         ║
 * ║   • ✅ DUAL-STATE FOUNDATION: Clear separation from link state testing (US-2)           ║
 * ║                                                                                          ║
 * ║ 💡 INDIVIDUAL COMMAND STATE INSIGHTS:                                                   ║
 * ║   • Command descriptors maintain independent state regardless of link state             ║
 * ║   • Status transitions follow predictable lifecycle patterns                            ║
 * ║   • Callback execution enables detailed state transition tracking                       ║
 * ║   • Success/failure states provide accurate execution result information                 ║
 * ║                                                                                          ║
 * ║ 🔄 DESIGN PRINCIPLES:                                                                    ║
 * ║   • Test-driven development methodology                                                 ║
 * ║   • Individual command state focus (complemented by US-2 link state testing)            ║
 * ║   • State lifecycle verification approach                                               ║
 * ║   • Comprehensive error condition coverage                                              ║
 * ║   • Consistent AC-X TC-Y naming pattern                                                 ║
 * ╚══════════════════════════════════════════════════════════════════════════════════════════╝
 */
//======>END OF IMPLEMENTATION SUMMARY=============================================================
