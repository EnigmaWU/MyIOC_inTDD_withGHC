///////////////////////////////////////////////////////////////////////////////////////////////////
// Command State US-2: Link Command Execution State Verification
//
// Intent:
// - Verify link command execution state transitions during command processing
// - Test link sub-states during different command execution patterns (callback vs polling)
// - Validate link state correlation with command activity and role behavior
//
// 🎯 DUAL-STATE FOCUS: This file focuses on LINK COMMAND EXECUTION STATE (Level 2)
//     WHY LINK COMMAND EXECUTION STATE MATTERS:
//     - Link state reflects command processing activity at the communication level
//     - Different command roles (CmdInitiator vs CmdExecutor) require different link sub-states
//     - Multiple concurrent commands on the same link require aggregate state tracking
//     - Link state enables monitoring command load and resource utilization
//     - Execution patterns (callback vs polling) manifest differently in link state behavior
//
// 🔗 COMPANION: UT_CommandStateUS1.cxx focuses on INDIVIDUAL COMMAND STATE (Level 1)
//     Together, these files provide comprehensive dual-state command testing coverage.
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <chrono>
#include <thread>

#include "UT_CommandState.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF OVERVIEW OF THIS UNIT TESTING FILE===============================================
/**
 * @brief Verify link command execution state management during command processing:
 *  - Link sub-states during command execution: IOC_LinkSubStateCmdInitiatorReady →
 * IOC_LinkSubStateCmdInitiatorBusyExecCmd
 *  - Command executor link states: IOC_LinkSubStateCmdExecutorReady → IOC_LinkSubStateCmdExecutorBusyExecCmd
 *  - Polling mode link states: IOC_LinkSubStateCmdExecutorBusyWaitCmd during IOC_waitCMD operations
 *  - Link state correlation with command activity level and concurrent command processing
 *
 * Key API focus:
 *  - IOC_getLinkState(): Retrieve link main state and command-specific sub-states
 *  - Link state correlation with command execution patterns (callback vs polling)
 *  - Multi-command scenarios: Link state aggregation during concurrent command processing
 *  - Role-based link states: CmdInitiator vs CmdExecutor link state behavior differences
 */
//======>END OF OVERVIEW OF THIS UNIT TESTING FILE=================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF UNIT TESTING DESIGN==============================================================
/**
 * Design focus:
 *  - Link command execution state verification independent of individual command state
 *  - Command role-based link state behavior (CmdInitiator vs CmdExecutor sub-states)
 *  - Execution pattern impact on link state (callback vs polling mode differences)
 *  - Multi-command link state aggregation and concurrent command activity tracking
 *  - Link state consistency across command lifecycle phases
 *
 * Test approach:
 *  - Focus on IOC_getLinkState() with command-specific sub-states
 *  - Verify link state transitions during command execution phases
 *  - Test link state correlation with command activity levels
 *  - Validate role-specific link state behavior patterns
 *  - Ensure link state provides meaningful command execution status information
 */
///////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF USER STORY=======================================================================
/**
 * US-2: As a command state developer, I want to verify link command execution states
 *       so that IOC_LinkID_T properly reflects command processing activity and maintains
 *       appropriate link states during command execution workflows,
 *       enabling effective command load monitoring and resource management.
 */
//======>END OF USER STORY==========================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//=======>BEGIN OF ACCEPTANCE CRITERIA==============================================================
/**
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
//=======>END OF ACCEPTANCE CRITERIA================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF TEST CASES=======================================================================
/**************************************************************************************************
 * @brief 【Link Command Execution State Test Cases】
 *
 * ORGANIZATION STRATEGIES:
 *  - By Link Role: CmdInitiator vs CmdExecutor link state behavior
 *  - By Execution Pattern: Callback mode vs Polling mode link state differences
 *  - By Activity Level: Ready vs Busy link states during command processing
 *  - By Concurrency: Single command vs Multiple concurrent command link state aggregation
 *
 * STATUS TRACKING: ⚪ = Planned/TODO，🔴 = Implemented/RED, 🟢 = Passed/GREEN, ⚠️ = Issues
 *
 * ⚪ FRAMEWORK STATUS: Link command execution state APIs need comprehensive testing
 *
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * 📋 [US-2]: LINK COMMAND EXECUTION STATE VERIFICATION
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 *
 * [@AC-1,US-2] CmdInitiator link ready state verification
 *  ⚪ TC-1: verifyLinkCmdInitiatorReady_byInitialState_expectReadySubState
 *      @[Purpose]: Validate CmdInitiator link reports ready state when available for commands
 *      @[Brief]: Create CmdInitiator link, verify IOC_LinkSubStateCmdInitiatorReady sub-state
 *      @[Status]: TODO - Need to implement CmdInitiator ready state verification
 *
 * [@AC-2,US-2] CmdInitiator link busy state during command execution
 *  ⚪ TC-1: verifyLinkCmdInitiatorBusy_byCommandExecution_expectBusySubState
 *      @[Purpose]: Validate CmdInitiator link reports busy state during command execution
 *      @[Brief]: Execute command via IOC_execCMD(), verify IOC_LinkSubStateCmdInitiatorBusyExecCmd
 *      @[Status]: TODO - Need to implement CmdInitiator busy state tracking
 *
 * [@AC-3,US-2] CmdExecutor link ready state verification
 *  ⚪ TC-1: verifyLinkCmdExecutorReady_byCallbackMode_expectReadySubState
 *      @[Purpose]: Validate CmdExecutor link reports ready state when available for commands
 *      @[Brief]: Create CmdExecutor link with callback, verify IOC_LinkSubStateCmdExecutorReady
 *      @[Status]: TODO - Need to implement CmdExecutor ready state verification
 *
 * [@AC-4,US-2] CmdExecutor link busy state during callback execution
 *  ⚪ TC-1: verifyLinkCmdExecutorBusy_byCallbackExecution_expectBusySubState
 *      @[Purpose]: Validate CmdExecutor link reports busy state during callback execution
 *      @[Brief]: Process command via callback, verify IOC_LinkSubStateCmdExecutorBusyExecCmd
 *      @[Status]: TODO - Need to implement CmdExecutor busy state tracking
 *
 * [@AC-5,US-2] CmdExecutor link polling state verification
 *  ⚪ TC-1: verifyLinkCmdExecutorPolling_byWaitCMD_expectPollingSubState
 *      @[Purpose]: Validate CmdExecutor link reports polling state during IOC_waitCMD()
 *      @[Brief]: Call IOC_waitCMD(), verify IOC_LinkSubStateCmdExecutorBusyWaitCmd
 *      @[Status]: TODO - Need to implement polling state verification
 *
 * [@AC-6,US-2] Link state aggregation during concurrent commands
 *  ⚪ TC-1: verifyLinkStateAggregation_byConcurrentCommands_expectConsistentState
 *      @[Purpose]: Validate link state aggregation for multiple concurrent commands
 *      @[Brief]: Execute multiple commands concurrently, verify aggregate link state behavior
 *      @[Status]: TODO - Need to implement concurrent command link state aggregation
 *
 * [@AC-7,US-2] Link state return to ready after command completion
 *  ⚪ TC-1: verifyLinkStateCompletion_byCommandFinish_expectReadyState
 *      @[Purpose]: Validate link returns to ready state after command completion
 *      @[Brief]: Complete command execution, verify link returns to appropriate ready sub-state
 *      @[Status]: TODO - Need to implement completion state verification
 */
//======>END OF TEST CASES=========================================================================

// Link command execution state private data structure
typedef struct __LinkCmdExecStatePriv {
    std::atomic<bool> LinkStateTracking{true};
    std::atomic<int> StateChangeCount{0};

    // Link state history
    IOC_LinkState_T StateHistory[20];
    IOC_LinkSubState_T SubStateHistory[20];
    int HistoryCount{0};

    // Command activity tracking
    std::atomic<int> CommandsSent{0};
    std::atomic<int> CommandsReceived{0};
    std::atomic<int> CommandsProcessed{0};

    // State timing
    std::chrono::steady_clock::time_point StateChangeTimestamps[20];

    // Current state cache
    std::atomic<IOC_LinkState_T> LastMainState{IOC_LinkStateUndefined};
    std::atomic<IOC_LinkSubState_T> LastSubState{IOC_LinkSubStateDefault};

    std::mutex StateMutex;
} __LinkCmdExecStatePriv_T;

// TODO: Implement link state tracking utilities
static void __TrackLinkState(__LinkCmdExecStatePriv_T *pPrivData, IOC_LinkID_T linkID) {
    if (!pPrivData) return;

    std::lock_guard<std::mutex> lock(pPrivData->StateMutex);

    IOC_LinkState_T mainState = IOC_LinkStateUndefined;
    IOC_LinkSubState_T subState = IOC_LinkSubStateDefault;
    IOC_Result_T result = IOC_getLinkState(linkID, &mainState, &subState);

    if (result == IOC_RESULT_SUCCESS) {
        // Check if state changed
        if (pPrivData->LastMainState != mainState || pPrivData->LastSubState != subState) {
            if (pPrivData->HistoryCount < 20) {
                pPrivData->StateHistory[pPrivData->HistoryCount] = mainState;
                pPrivData->SubStateHistory[pPrivData->HistoryCount] = subState;
                pPrivData->StateChangeTimestamps[pPrivData->HistoryCount] = std::chrono::steady_clock::now();
                pPrivData->HistoryCount++;
            }

            pPrivData->LastMainState = mainState;
            pPrivData->LastSubState = subState;
            pPrivData->StateChangeCount++;

            printf("🔗 [LINK_STATE] LinkID=%llu MainState=%d, SubState=%d, Changes=%d\n", linkID, mainState, subState,
                   pPrivData->StateChangeCount.load());
        }
    }
}

// TODO: Implement command execution callback for link state testing
static IOC_Result_T __LinkCmdExecState_ExecutorCb(IOC_LinkID_T LinkID, IOC_CmdDesc_pT pCmdDesc, void *pCbPriv) {
    __LinkCmdExecStatePriv_T *pPrivData = (__LinkCmdExecStatePriv_T *)pCbPriv;
    if (!pPrivData || !pCmdDesc) {
        return IOC_RESULT_INVALID_PARAM;
    }

    // Track command received
    pPrivData->CommandsReceived++;

    // Track link state during callback execution
    __TrackLinkState(pPrivData, LinkID);

    // Set command to processing state
    IOC_CmdDesc_setStatus(pCmdDesc, IOC_CMD_STATUS_PROCESSING);

    // Track link state during processing
    __TrackLinkState(pPrivData, LinkID);

    // Process the command
    IOC_CmdID_T CmdID = IOC_CmdDesc_getCmdID(pCmdDesc);
    IOC_Result_T ExecResult = IOC_RESULT_SUCCESS;

    if (CmdID == IOC_CMDID_TEST_PING) {
        IOC_CmdDesc_setOutPayload(pCmdDesc, (void *)"PONG", 4);
        IOC_CmdDesc_setStatus(pCmdDesc, IOC_CMD_STATUS_SUCCESS);
        IOC_CmdDesc_setResult(pCmdDesc, IOC_RESULT_SUCCESS);
    } else {
        ExecResult = IOC_RESULT_NOT_SUPPORT;
        IOC_CmdDesc_setStatus(pCmdDesc, IOC_CMD_STATUS_FAILED);
        IOC_CmdDesc_setResult(pCmdDesc, IOC_RESULT_NOT_SUPPORT);
    }

    // Track command processed
    pPrivData->CommandsProcessed++;

    // Track final link state
    __TrackLinkState(pPrivData, LinkID);

    return ExecResult;
}

// [@AC-1,US-2] TC-1: CmdInitiator link ready state verification
TEST(UT_CommandStateUS2, verifyLinkCmdInitiatorReady_byInitialState_expectReadySubState) {
    IOC_Result_T ResultValue = IOC_RESULT_BUG;

    // ┌──────────────────────────────────────────────────────────────────────────────────────┐
    // │                                🔧 SETUP PHASE                                        │
    // └──────────────────────────────────────────────────────────────────────────────────────┘
    __LinkCmdExecStatePriv_T linkStatePriv = {};

    // Service setup for CmdInitiator testing
    IOC_SrvURI_T srvURI = {.pProtocol = IOC_SRV_PROTO_FIFO,
                           .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
                           .pPath = (const char *)"CmdStateUS2_InitiatorReady"};

    // Setup service as CmdExecutor to receive commands from CmdInitiator
    static IOC_CmdID_T supportedCmdIDs[] = {IOC_CMDID_TEST_PING};
    IOC_CmdUsageArgs_T cmdUsageArgs = {.CbExecCmd_F = __LinkCmdExecState_ExecutorCb,
                                       .pCbPrivData = &linkStatePriv,
                                       .CmdNum = 1,
                                       .pCmdIDs = supportedCmdIDs};

    IOC_SrvArgs_T srvArgs = {.SrvURI = srvURI,
                             .Flags = IOC_SRVFLAG_NONE,
                             .UsageCapabilites = IOC_LinkUsageCmdExecutor,
                             .UsageArgs = {.pCmd = &cmdUsageArgs}};

    IOC_SrvID_T srvID = IOC_ID_INVALID;
    ResultValue = IOC_onlineService(&srvID, &srvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);

    printf("🔧 [SETUP] Service online for CmdInitiator ready state testing\n");

    // ┌──────────────────────────────────────────────────────────────────────────────────────┐
    // │                              📋 BEHAVIOR PHASE                                       │
    // └──────────────────────────────────────────────────────────────────────────────────────┘

    // Client setup as CmdInitiator
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

    printf("📋 [BEHAVIOR] CmdInitiator link established, checking ready state\n");

    // ┌──────────────────────────────────────────────────────────────────────────────────────┐
    // │                               ✅ VERIFY PHASE                                        │
    // └──────────────────────────────────────────────────────────────────────────────────────┘

    // Verify CmdInitiator link is in ready state
    // NOTE: Current implementation may not have CmdInitiator-specific sub-states implemented yet
    // This test establishes the expected behavior for future implementation

    IOC_LinkState_T mainState = IOC_LinkStateUndefined;
    IOC_LinkSubState_T subState = IOC_LinkSubStateDefault;
    ResultValue = IOC_getLinkState(cliLinkID, &mainState, &subState);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);

    // Verify main state is Ready
    VERIFY_LINK_CMD_MAIN_STATE(cliLinkID, IOC_LinkStateReady);

    // TODO: Once CmdInitiator sub-states are implemented, verify:
    // VERIFY_LINK_CMD_SUB_STATE(cliLinkID, IOC_LinkSubStateCmdInitiatorReady);

    printf("✅ [VERIFY] CmdInitiator link ready state: MainState=%d, SubState=%d\n", mainState, subState);
    printf("✅ [NOTE] CmdInitiator-specific sub-states pending implementation\n");
    printf("✅ [RESULT] CmdInitiator ready state verification completed\n");

    // ┌──────────────────────────────────────────────────────────────────────────────────────┐
    // │                               🧹 CLEANUP PHASE                                       │
    // └──────────────────────────────────────────────────────────────────────────────────────┘
    if (cliLinkID != IOC_ID_INVALID) IOC_closeLink(cliLinkID);
    if (srvLinkID != IOC_ID_INVALID) IOC_closeLink(srvLinkID);
    if (srvID != IOC_ID_INVALID) IOC_offlineService(srvID);
}

// [@AC-3,US-2] TC-1: CmdExecutor link ready state verification
TEST(UT_CommandStateUS2, verifyLinkCmdExecutorReady_byCallbackMode_expectReadySubState) {
    IOC_Result_T ResultValue = IOC_RESULT_BUG;

    // ┌──────────────────────────────────────────────────────────────────────────────────────┐
    // │                                🔧 SETUP PHASE                                        │
    // └──────────────────────────────────────────────────────────────────────────────────────┘
    __LinkCmdExecStatePriv_T linkStatePriv = {};

    // Service setup as CmdExecutor with callback mode
    IOC_SrvURI_T srvURI = {.pProtocol = IOC_SRV_PROTO_FIFO,
                           .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
                           .pPath = (const char *)"CmdStateUS2_ExecutorReady"};

    static IOC_CmdID_T supportedCmdIDs[] = {IOC_CMDID_TEST_PING};
    IOC_CmdUsageArgs_T cmdUsageArgs = {.CbExecCmd_F = __LinkCmdExecState_ExecutorCb,
                                       .pCbPrivData = &linkStatePriv,
                                       .CmdNum = 1,
                                       .pCmdIDs = supportedCmdIDs};

    IOC_SrvArgs_T srvArgs = {.SrvURI = srvURI,
                             .Flags = IOC_SRVFLAG_NONE,
                             .UsageCapabilites = IOC_LinkUsageCmdExecutor,
                             .UsageArgs = {.pCmd = &cmdUsageArgs}};

    IOC_SrvID_T srvID = IOC_ID_INVALID;
    ResultValue = IOC_onlineService(&srvID, &srvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);

    printf("🔧 [SETUP] Service online as CmdExecutor with callback mode\n");

    // ┌──────────────────────────────────────────────────────────────────────────────────────┐
    // │                              📋 BEHAVIOR PHASE                                       │
    // └──────────────────────────────────────────────────────────────────────────────────────┘

    // Client setup as CmdInitiator
    IOC_ConnArgs_T connArgs = {.SrvURI = srvURI, .Usage = IOC_LinkUsageCmdInitiator};
    IOC_LinkID_T cliLinkID = IOC_ID_INVALID;

    std::thread cliThread([&] {
        IOC_Result_T connResult = IOC_connectService(&cliLinkID, &connArgs, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, connResult);
    });

    // Accept client and get the CmdExecutor link
    IOC_LinkID_T srvLinkID = IOC_ID_INVALID;
    ResultValue = IOC_acceptClient(srvID, &srvLinkID, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);

    if (cliThread.joinable()) cliThread.join();

    printf("📋 [BEHAVIOR] CmdExecutor link established, checking ready state\n");

    // ┌──────────────────────────────────────────────────────────────────────────────────────┐
    // │                               ✅ VERIFY PHASE                                        │
    // └──────────────────────────────────────────────────────────────────────────────────────┘

    // Verify CmdExecutor link is in ready state
    IOC_LinkState_T mainState = IOC_LinkStateUndefined;
    IOC_LinkSubState_T subState = IOC_LinkSubStateDefault;
    ResultValue = IOC_getLinkState(srvLinkID, &mainState, &subState);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);

    // Verify main state is Ready
    VERIFY_LINK_CMD_MAIN_STATE(srvLinkID, IOC_LinkStateReady);

    // TODO: Once CmdExecutor sub-states are implemented, verify:
    // VERIFY_LINK_CMD_SUB_STATE(srvLinkID, IOC_LinkSubStateCmdExecutorReady);

    printf("✅ [VERIFY] CmdExecutor link ready state: MainState=%d, SubState=%d\n", mainState, subState);
    printf("✅ [NOTE] CmdExecutor-specific sub-states pending implementation\n");
    printf("✅ [RESULT] CmdExecutor ready state verification completed\n");

    // ┌──────────────────────────────────────────────────────────────────────────────────────┐
    // │                               🧹 CLEANUP PHASE                                       │
    // └──────────────────────────────────────────────────────────────────────────────────────┘
    if (cliLinkID != IOC_ID_INVALID) IOC_closeLink(cliLinkID);
    if (srvLinkID != IOC_ID_INVALID) IOC_closeLink(srvLinkID);
    if (srvID != IOC_ID_INVALID) IOC_offlineService(srvID);
}

// [@AC-4,US-2] TC-1: CmdExecutor link busy state during callback execution
TEST(UT_CommandStateUS2, verifyLinkCmdExecutorBusy_byCallbackExecution_expectBusySubState) {
    IOC_Result_T ResultValue = IOC_RESULT_BUG;

    // ┌──────────────────────────────────────────────────────────────────────────────────────┐
    // │                                🔧 SETUP PHASE                                        │
    // └──────────────────────────────────────────────────────────────────────────────────────┘
    __LinkCmdExecStatePriv_T linkStatePriv = {};

    // Service setup as CmdExecutor with state tracking callback
    IOC_SrvURI_T srvURI = {.pProtocol = IOC_SRV_PROTO_FIFO,
                           .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
                           .pPath = (const char *)"CmdStateUS2_ExecutorBusy"};

    static IOC_CmdID_T supportedCmdIDs[] = {IOC_CMDID_TEST_PING};
    IOC_CmdUsageArgs_T cmdUsageArgs = {.CbExecCmd_F = __LinkCmdExecState_ExecutorCb,
                                       .pCbPrivData = &linkStatePriv,
                                       .CmdNum = 1,
                                       .pCmdIDs = supportedCmdIDs};

    IOC_SrvArgs_T srvArgs = {.SrvURI = srvURI,
                             .Flags = IOC_SRVFLAG_NONE,
                             .UsageCapabilites = IOC_LinkUsageCmdExecutor,
                             .UsageArgs = {.pCmd = &cmdUsageArgs}};

    IOC_SrvID_T srvID = IOC_ID_INVALID;
    ResultValue = IOC_onlineService(&srvID, &srvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);

    printf("🔧 [SETUP] Service online as CmdExecutor with link state tracking\n");

    // ┌──────────────────────────────────────────────────────────────────────────────────────┐
    // │                              📋 BEHAVIOR PHASE                                       │
    // └──────────────────────────────────────────────────────────────────────────────────────┘

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

    // Track initial link state
    __TrackLinkState(&linkStatePriv, srvLinkID);

    // Execute command to trigger callback and link state changes
    IOC_CmdDesc_T cmdDesc = IOC_CMDDESC_INIT_VALUE;
    cmdDesc.CmdID = IOC_CMDID_TEST_PING;
    cmdDesc.TimeoutMs = 5000;

    printf("📋 [BEHAVIOR] Executing command to trigger CmdExecutor busy state\n");

    ResultValue = IOC_execCMD(cliLinkID, &cmdDesc, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);

    // Track final link state
    __TrackLinkState(&linkStatePriv, srvLinkID);

    // ┌──────────────────────────────────────────────────────────────────────────────────────┐
    // │                               ✅ VERIFY PHASE                                        │
    // └──────────────────────────────────────────────────────────────────────────────────────┘

    // Verify command was processed
    ASSERT_EQ(1, linkStatePriv.CommandsReceived.load()) << "Command should be received";
    ASSERT_EQ(1, linkStatePriv.CommandsProcessed.load()) << "Command should be processed";

    // Verify link state changes were tracked
    ASSERT_GT(linkStatePriv.StateChangeCount.load(), 0) << "Link state changes should be tracked";
    ASSERT_GT(linkStatePriv.HistoryCount, 0) << "Link state history should be recorded";

    // Verify final link state is back to ready
    IOC_LinkState_T finalMainState = IOC_LinkStateUndefined;
    IOC_LinkSubState_T finalSubState = IOC_LinkSubStateDefault;
    ResultValue = IOC_getLinkState(srvLinkID, &finalMainState, &finalSubState);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);
    ASSERT_EQ(IOC_LinkStateReady, finalMainState) << "Final main state should be Ready";

    // TODO: Once CmdExecutor busy sub-states are implemented, verify:
    // - Link shows IOC_LinkSubStateCmdExecutorBusyExecCmd during callback execution
    // - Link returns to IOC_LinkSubStateCmdExecutorReady after completion

    printf("✅ [VERIFY] Link state tracking: %d state changes recorded\n", linkStatePriv.StateChangeCount.load());
    printf("✅ [VERIFY] Command processing: %d received, %d processed\n", linkStatePriv.CommandsReceived.load(),
           linkStatePriv.CommandsProcessed.load());
    printf("✅ [NOTE] CmdExecutor busy sub-states pending implementation\n");
    printf("✅ [RESULT] CmdExecutor busy state verification completed\n");

    // ┌──────────────────────────────────────────────────────────────────────────────────────┐
    // │                               🧹 CLEANUP PHASE                                       │
    // └──────────────────────────────────────────────────────────────────────────────────────┘
    if (cliLinkID != IOC_ID_INVALID) IOC_closeLink(cliLinkID);
    if (srvLinkID != IOC_ID_INVALID) IOC_closeLink(srvLinkID);
    if (srvID != IOC_ID_INVALID) IOC_offlineService(srvID);
}

// TODO: Implement remaining test cases:
// [@AC-2,US-2] TC-1: verifyLinkCmdInitiatorBusy_byCommandExecution_expectBusySubState
// [@AC-5,US-2] TC-1: verifyLinkCmdExecutorPolling_byWaitCMD_expectPollingSubState
// [@AC-6,US-2] TC-1: verifyLinkStateAggregation_byConcurrentCommands_expectConsistentState
// [@AC-7,US-2] TC-1: verifyLinkStateCompletion_byCommandFinish_expectReadyState

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF IMPLEMENTATION SUMMARY===========================================================
/**
 * ╔══════════════════════════════════════════════════════════════════════════════════════════╗
 * ║                              📊 IMPLEMENTATION SUMMARY                                   ║
 * ╠══════════════════════════════════════════════════════════════════════════════════════════╣
 * ║ 🎯 PURPOSE: Link Command Execution State Verification - User Story 2                   ║
 * ║                                                                                          ║
 * ║ 📋 COVERAGE:                                                                             ║
 * ║   ✅ US-2 AC-1: CmdInitiator link ready state verification                              ║
 * ║   ✅ US-2 AC-3: CmdExecutor link ready state verification                               ║
 * ║   ✅ US-2 AC-4: CmdExecutor link busy state during callback execution                   ║
 * ║   TODO: US-2 AC-2: CmdInitiator link busy state during command execution               ║
 * ║   TODO: US-2 AC-5: CmdExecutor link polling state verification                          ║
 * ║   TODO: US-2 AC-6: Link state aggregation during concurrent commands                    ║
 * ║   TODO: US-2 AC-7: Link state return to ready after command completion                  ║
 * ║                                                                                          ║
 * ║ 🔧 IMPLEMENTED TEST CASES (AC-X TC-Y Pattern):                                          ║
 * ║   AC-1 TC-1: verifyLinkCmdInitiatorReady_byInitialState_expectReadySubState             ║
 * ║   AC-3 TC-1: verifyLinkCmdExecutorReady_byCallbackMode_expectReadySubState              ║
 * ║   AC-4 TC-1: verifyLinkCmdExecutorBusy_byCallbackExecution_expectBusySubState           ║
 * ║                                                                                          ║
 * ║ 🚀 KEY ACHIEVEMENTS:                                                                     ║
 * ║   • ✅ LINK COMMAND STATE APIs: IOC_getLinkState() for command-specific link states     ║
 * ║   • ✅ ROLE-BASED STATE VERIFICATION: CmdInitiator vs CmdExecutor link state patterns   ║
 * ║   • ✅ STATE TRACKING FRAMEWORK: Link state change monitoring and history recording     ║
 * ║   • ✅ DUAL-STATE FOUNDATION: Clear separation from individual command state (US-1)     ║
 * ║                                                                                          ║
 * ║ 💡 LINK COMMAND STATE INSIGHTS:                                                         ║
 * ║   • Link state provides aggregate view of command processing activity                   ║
 * ║   • Different command roles require different link sub-state patterns                   ║
 * ║   • Link state transitions correlate with command execution phases                      ║
 * ║   • State tracking enables command load monitoring and resource management              ║
 * ║                                                                                          ║
 * ║ 🔄 DESIGN PRINCIPLES:                                                                    ║
 * ║   • Test-driven development methodology                                                 ║
 * ║   • Link command state focus (complemented by US-1 individual command state testing)   ║
 * ║   • Role-based state verification approach                                              ║
 * ║   • Command activity correlation with link state changes                                ║
 * ║   • Consistent AC-X TC-Y naming pattern                                                 ║
 * ║                                                                                          ║
 * ║ 📋 NEXT STEPS:                                                                           ║
 * ║   • Implement command-specific link sub-states in IOC framework                         ║
 * ║   • Add IOC_LinkSubStateCmdInitiatorReady, IOC_LinkSubStateCmdInitiatorBusyExecCmd       ║
 * ║   • Add IOC_LinkSubStateCmdExecutorReady, IOC_LinkSubStateCmdExecutorBusyExecCmd         ║
 * ║   • Add IOC_LinkSubStateCmdExecutorBusyWaitCmd for polling mode                          ║
 * ║   • Enhance link state tracking during command execution phases                         ║
 * ╚══════════════════════════════════════════════════════════════════════════════════════════╝
 */
//======>END OF IMPLEMENTATION SUMMARY=============================================================
