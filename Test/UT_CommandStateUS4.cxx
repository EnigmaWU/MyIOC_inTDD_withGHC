///////////////////////////////////////////////////////////////////////////////////////////////////
// Command State US-4 Implementation: Command Timeout and Error State Verification
//
// 🎯 IMPLEMENTATION OF: User Story 4 (see UT_CommandState.h for complete specification)
// 📋 PURPOSE: Verify command timeout and error state handling at both command and link levels
// 🔗 DUAL-STATE LEVEL: Both Level 1 (Command) and Level 2 (Link) - Error/Timeout State Management
//
// This file implements all test cases for US-4 Acceptance Criteria.
// See UT_CommandState.h for complete User Story definition and Acceptance Criteria.
//
// 🎯 ERROR/TIMEOUT STATE VERIFICATION FOCUS:
//    ✅ Command Status: IOC_CMD_STATUS_TIMEOUT, IOC_CMD_STATUS_FAILED
//    ✅ Command Result: IOC_RESULT_TIMEOUT, IOC_RESULT_CMD_EXEC_FAILED
//    ✅ Link State: Proper recovery after error/timeout conditions
//    ✅ State Correlation: Error propagation between command (Level 1) and link (Level 2)
//    ✅ Execution Patterns: BOTH Callback Mode (CbExecCmd_F) AND Polling Mode (waitCMD/ackCMD)
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "UT_CommandState.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF IMPLEMENTATION OVERVIEW=========================================================
/**
 * @brief US-4 Implementation: Command Timeout and Error State Verification
 *
 * Implements test cases for User Story 4 (see UT_CommandState.h for complete US/AC specification):
 *  - AC-1: Command timeout mechanisms (TC-1: Descriptor, TC-2: waitCMD option, TC-3: execCMD option)
 *  - AC-2: Link recovery after timeout (TC-1: Callback, TC-2: Polling)
 *  - AC-3: Error propagation (TC-1: Callback, TC-2: Polling)
 *  - AC-4: Mixed results independence (TC-1: Callback, TC-2: Polling)
 *  - AC-5: Error recovery and cleanup (TC-1: Callback, TC-2: Polling)
 *
 * 🔧 Implementation Focus:
 *  - Command timeout detection in BOTH execution patterns (callback + polling)
 *  - IOC_CMD_STATUS_TIMEOUT transition for callback timeout and waitCMD timeout
 *  - Link state resilience - return to Ready after timeout/error in both modes
 *  - Error propagation: callback return value vs ackCMD error descriptor
 *  - Command state independence - errors don't contaminate other commands
 *  - Error recovery - state cleanup enables new operations in both patterns
 *
 * 📊 TIMEOUT/ERROR STATE REFERENCE (from IOC_CmdDesc.h and IOC_Types.h):
 *  Command Status:
 *   - IOC_CMD_STATUS_PENDING (2)      - After execCMD, before callback/timeout
 *   - IOC_CMD_STATUS_PROCESSING (3)   - During callback execution
 *   - IOC_CMD_STATUS_SUCCESS (4)      - Callback returned IOC_RESULT_SUCCESS
 *   - IOC_CMD_STATUS_FAILED (5)       - Callback returned error result
 *   - IOC_CMD_STATUS_TIMEOUT (6)      - Timeout occurred before/during callback
 *
 *  Command Result:
 *   - IOC_RESULT_SUCCESS (0)          - Successful execution
 *   - IOC_RESULT_TIMEOUT (-506)       - Timeout occurred
 *   - IOC_RESULT_CMD_EXEC_FAILED (-509) - Command execution failure
 *   - IOC_RESULT_BUG (-999)           - Unexpected error
 *
 *  Link SubState (should recover to Ready):
 *   - IOC_LinkSubStateCmdInitiatorReady (6)       - Ready after timeout/error
 *   - IOC_LinkSubStateCmdInitiatorBusyExecCmd (7) - During command execution
 *   - IOC_LinkSubStateCmdExecutorReady (8)        - Ready after timeout/error
 *   - IOC_LinkSubStateCmdExecutorBusyExecCmd (9)  - During callback processing
 *
 * 🎯 TIMEOUT MECHANISMS (TWO INDEPENDENT LEVELS):
 *
 *    LEVEL 1: Command Descriptor Timeout (Callback Execution Timeout)
 *      Field: pCmdDesc->TimeoutMs  // Timeout in milliseconds for callback execution
 *      Usage: cmdDesc.TimeoutMs = 100;  // Executor callback must complete within 100ms
 *      Scope: Protocol enforces timeout during callback execution (CbExecCmd_F)
 *      TC Coverage: AC-1 TC-1 tests this mechanism
 *
 *    LEVEL 2: API Call Option Timeout (execCMD/waitCMD Call Timeout)
 *      Parameter: pOption->Payload.TimeoutUS  // Timeout in microseconds for API call
 *      Usage: IOC_Option_defineTimeout(opt, 100000);  // API call must complete within 100ms (100,000us)
 *      Macros: IOC_Option_defineTimeout(), IOC_Option_defineNonBlock(), IOC_Option_defineMayBlock()
 *      Scope:
 *        - IOC_execCMD(LinkID, pCmdDesc, pOption) - timeout for entire command execution
 *        - IOC_waitCMD(LinkID, pCmdDesc, pOption) - timeout for waiting for command arrival
 *        - IOC_ackCMD(LinkID, pCmdDesc, pOption) - timeout for sending acknowledgment
 *      TC Coverage: AC-1 TC-3 (execCMD option), AC-1 TC-2 (waitCMD option) test these mechanisms
 *
 *    INTERACTION: Both mechanisms can coexist
 *      - pCmdDesc->TimeoutMs: Limits callback execution duration (executor-side)
 *      - pOption->TimeoutUS: Limits API call blocking duration (caller-side)
 *      - Timeout occurs at whichever limit is reached first
 *
 * 🏗️ ARCHITECTURE PRINCIPLES:
 *    ✅ Principle 1: TIMEOUT INDEPENDENCE - Timeout doesn't affect link availability
 *    ✅ Principle 2: ERROR ISOLATION - Command errors don't propagate to link failure
 *    ✅ Principle 3: STATE RECOVERY - Links auto-recover to Ready after error/timeout
 *    ✅ Principle 4: DUAL-LEVEL CORRELATION - Command state ↔ Link substate synchronization
 *    ✅ Principle 5: PATTERN SYMMETRY - Error handling consistent in callback + polling modes
 *    ✅ Principle 6: EXPLICIT CONTROL - Polling mode provides explicit error setting in descriptor
 */
//======>END OF IMPLEMENTATION OVERVIEW===========================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF TEST CASES=======================================================================
/**************************************************************************************************
 * @brief 【Command Timeout and Error State Test Cases - DESIGN PHASE】
 *
 * ORGANIZATION STRATEGY:
 *  - By Error Type: Timeout → Callback Error → Mixed Results → Recovery
 *  - By State Level: Command State (Level 1) → Link State (Level 2) → Correlation
 *  - By Lifecycle: Detection → Propagation → Cleanup → Recovery
 *  - By Complexity: Single error → Multiple errors → Error recovery
 *
 * 🔄 STATE FOCUS: This file tests BOTH command-level (Level 1) AND link-level (Level 2) states
 *    during error and timeout conditions, verifying proper correlation and recovery
 *
 * STATUS TRACKING: ⚪ = Planned/TODO，🔴 = Implemented/RED, 🟢 = Passed/GREEN, ⚠️ = Issues
 *
 * ⚪ FRAMEWORK STATUS: Timeout and error state verification - 0/10 TESTS (0%)
 *    ⚪ 0/10 tests implemented (5 callback + 5 polling mode tests)
 *    ⚪ 0/5 Acceptance Criteria verified (each AC has 2 tests: callback + polling)
 *    ✅ API discovery complete (IOC_CMD_STATUS_TIMEOUT, IOC_RESULT_TIMEOUT exist)
 *    ✅ Timeout mechanism identified (TimeoutMs field + waitCMD timeout option)
 *    ✅ Polling APIs identified (IOC_waitCMD, IOC_ackCMD)
 *    ⚠️ TDD EXPECTATION: Tests will likely REVEAL missing timeout enforcement logic
 *
 * 📊 COVERAGE PLAN (EXPANDED FOR BOTH EXECUTION PATTERNS + TIMEOUT MECHANISMS):
 *    ⚪ AC-1: 0/3 tests planned - Command timeout mechanisms
 *       • TC-1: Descriptor timeout (pCmdDesc->TimeoutMs) in callback mode
 *       • TC-2: API option timeout (pOption->TimeoutUS) in IOC_waitCMD
 *       • TC-3: API option timeout (pOption->TimeoutUS) in IOC_execCMD
 *    ⚪ AC-2: 0/2 tests planned - Link recovery after timeout
 *       • TC-1: Callback mode recovery
 *       • TC-2: Polling mode recovery
 *    ⚪ AC-3: 0/2 tests planned - Error propagation
 *       • TC-1: Callback return error
 *       • TC-2: Polling ackCMD error
 *    ⚪ AC-4: 0/2 tests planned - Mixed success/failure independence
 *       • TC-1: Callback mode sequential commands
 *       • TC-2: Polling mode wait/ack cycles
 *    ⚪ AC-5: 0/2 tests planned - Error recovery and cleanup
 *       • TC-1: Callback mode recovery
 *       • TC-2: Polling mode recovery
 *
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * 📋 [US-4]: COMMAND TIMEOUT AND ERROR STATE VERIFICATION
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 *
 * 🎯 TEST STRATEGY (COMPREHENSIVE TIMEOUT COVERAGE):
 *    ✅ DESCRIPTOR TIMEOUT: Use pCmdDesc->TimeoutMs for callback execution timeout
 *    ✅ API OPTION TIMEOUT: Use pOption->Payload.TimeoutUS for API call timeout
 *    ✅ CALLBACK MODE:
 *       - AC-1 TC-1: Test descriptor timeout (callback exceeds TimeoutMs)
 *       - AC-1 TC-3: Test API option timeout (IOC_execCMD with pOption timeout)
 *    ✅ POLLING MODE:
 *       - AC-1 TC-2: Test API option timeout (IOC_waitCMD with pOption timeout)
 *    ✅ Executor callback delays (std::this_thread::sleep_for) to trigger timeout
 *    ✅ Executor callback returns error codes to trigger failure states
 *    ✅ Polling mode: set error in descriptor before IOC_ackCMD
 *    ✅ Query command status/result with IOC_CmdDesc_getStatus/getResult
 *    ✅ Query link state with IOC_getLinkState to verify recovery
 *    ✅ Verify symmetry: both patterns and both timeout mechanisms behave consistently
 *
 * [@AC-1,US-4] Command timeout state transitions (THREE TIMEOUT MECHANISMS)
 *  🟢 TC-1: verifyCommandTimeout_byDescriptorTimeout_expectTimeoutStatus  [TIMEOUT-DESCRIPTOR]
 *      @[Purpose]: Validate command transitions to TIMEOUT when callback execution exceeds pCmdDesc->TimeoutMs
 *      @[Brief]: Descriptor timeout (100ms), callback delays 200ms, verify TIMEOUT status
 *      @[Strategy]: Service with LinkA1(Initiator) + Client-A1(Executor with slow callback)
 *                   → Setup: pCmdDesc->TimeoutMs = 100 (descriptor-level timeout)
 *                   → Client-A1 callback delays 200ms (exceeds timeout by 100ms)
 *                   → IOC_execCMD(LinkID, &cmdDesc, NULL) - NO pOption timeout
 *                   → Protocol enforces descriptor timeout during callback execution
 *      @[Key Assertions]:
 *          • ASSERTION 1: Command status = IOC_CMD_STATUS_TIMEOUT (6)
 *          • ASSERTION 2: Command result = IOC_RESULT_TIMEOUT (-506)
 *          • ASSERTION 3: Callback was invoked (even if it times out)
 *          • ASSERTION 4: IOC_execCMD returned at ~100ms (NOT 200ms!) ← CRITICAL TIMING!
 *      @[Architecture Principle]: Descriptor timeout limits callback execution duration
 *      @[TDD Expectation]: Timeout already implemented, test validates correct behavior
 *      @[Status]: ✅ COMPLETE - 4 assertions GREEN, descriptor timeout validated
 *
 *  ⚪ TC-2: verifyCommandTimeout_byWaitCmdOptionTimeout_expectTimeoutStatus  [TIMEOUT-WAITCMD-OPTION]
 *      @[Purpose]: Validate IOC_waitCMD times out via pOption when no command arrives within timeout
 *      @[Brief]: Executor calls IOC_waitCMD with pOption timeout, no command sent, verify timeout
 *      @[Strategy]: Service with LinkA1(Executor in polling mode) + Client-A1(Initiator)
 *                   → Client-A1 does NOT send command (executor would wait indefinitely)
 *                   → IOC_Option_defineTimeout(waitOpt, 100000); // 100ms = 100,000 microseconds
 *                   → Executor calls IOC_waitCMD(LinkA1, &cmdDesc, &waitOpt)
 *                   → After 100ms, IOC_waitCMD returns IOC_RESULT_TIMEOUT
 *      @[Key Assertions]:
 *          • ASSERTION 1: IOC_waitCMD returns IOC_RESULT_TIMEOUT (-506) after ~100ms
 *          • ASSERTION 2: Actual duration ~100ms ± 20ms (NOT indefinite wait)
 *          • ASSERTION 3: Link state after timeout = IOC_LinkSubStateCmdExecutorReady (8) ← RECOVERY!
 *          • ASSERTION 4: No command descriptor populated (timeout before arrival)
 *          • ASSERTION 5: Subsequent IOC_waitCMD succeeds (link operational)
 *      @[Architecture Principle]: API option timeout prevents indefinite blocking in waitCMD
 *      @[TDD Expectation]: Verify pOption timeout enforcement in IOC_waitCMD
 *      @[Status]: TODO - Implementation will test waitCMD pOption timeout mechanism
 *
 *  ⚪ TC-3: verifyCommandTimeout_byExecCmdOptionTimeout_expectTimeoutStatus  [TIMEOUT-EXECCMD-OPTION]
 *      @[Purpose]: Validate IOC_execCMD times out via pOption when callback exceeds timeout
 *      @[Brief]: API option timeout (100ms), callback delays 200ms, verify TIMEOUT status
 *      @[Strategy]: Service with LinkA1(Initiator) + Client-A1(Executor with slow callback)
 *                   → Setup: cmdDesc.TimeoutMs = 0 (NO descriptor timeout)
 *                   → Client-A1 callback delays 200ms
 *                   → IOC_Option_defineTimeout(execOpt, 100000); // 100ms = 100,000 microseconds
 *                   → IOC_execCMD(LinkID, &cmdDesc, &execOpt) - API-level timeout
 *                   → Protocol enforces pOption timeout on entire execCMD operation
 *      @[Key Assertions]:
 *          • ASSERTION 1: Command status = IOC_CMD_STATUS_TIMEOUT (6)
 *          • ASSERTION 2: Command result = IOC_RESULT_TIMEOUT (-506)
 *          • ASSERTION 3: IOC_execCMD returned at ~100ms (NOT 200ms!) ← CRITICAL!
 *          • ASSERTION 4: Callback was invoked (timeout during execution)
 *      @[Architecture Principle]: API option timeout limits entire execCMD call duration
 *      @[TDD Expectation]: MAY reveal gap if pOption timeout not implemented for execCMD
 *      @[Status]: TODO - Implementation will test execCMD pOption timeout mechanism
 *
 * [@AC-2,US-4] Link state recovery after timeout (BOTH execution patterns)
 *  ⚪ TC-1: verifyLinkRecovery_afterCallbackTimeout_expectReadyState  [RECOVERY-CALLBACK]
 *      @[Purpose]: Validate link recovers to Ready state after callback timeout
 *      @[Brief]: After command timeout in callback mode, link substate returns to Ready
 *      @[Strategy]: Service with LinkA1(Initiator) + Client-A1(Executor with slow callback)
 *                   → Setup: TimeoutMs = 100ms, callback delays 200ms → timeout occurs
 *                   → Verify link substate during timeout = BusyExecCmd (9)
 *                   → Verify link substate after timeout = ExecutorReady (8) ← AUTO RECOVERY!
 *                   → Send 2nd command to verify link operational after recovery
 *      @[Key Assertions]:
 *          • ASSERTION 1: Link substate during callback = IOC_LinkSubStateCmdExecutorBusyExecCmd (9)
 *          • ASSERTION 2: Link substate after timeout = IOC_LinkSubStateCmdExecutorReady (8)
 *          • ASSERTION 3: Link MainState remains IOC_LinkStateCmdExecutorActive (5) ← NO FAILURE!
 *          • ASSERTION 4: 2nd command executes successfully (link recovered)
 *          • ASSERTION 5: No error propagation to link state (timeout isolated to command)
 *      @[Architecture Principle]: Callback timeout isolates command failure from link failure
 *      @[TDD Expectation]: Verify link state cleanup in callback timeout path
 *      @[Status]: TODO - Validate link recovery mechanism
 *
 *  ⚪ TC-2: verifyLinkRecovery_afterPollingTimeout_expectReadyState  [RECOVERY-POLLING]
 *      @[Purpose]: Validate link recovers to Ready state after polling wait timeout
 *      @[Brief]: After IOC_waitCMD timeout, link returns to Ready for next operation
 *      @[Strategy]: Service with LinkA1(Executor in polling mode)
 *                   → Executor calls IOC_waitCMD with 100ms timeout, no command arrives
 *                   → Verify IOC_waitCMD returns IOC_RESULT_TIMEOUT after 100ms
 *                   → Verify link substate during wait = BusyWaitCmd (10)
 *                   → Verify link substate after timeout = ExecutorReady (8) ← AUTO RECOVERY!
 *                   → Call IOC_waitCMD again to verify link operational
 *      @[Key Assertions]:
 *          • ASSERTION 1: Link substate during IOC_waitCMD = IOC_LinkSubStateCmdExecutorBusyWaitCmd (10)
 *          • ASSERTION 2: Link substate after timeout = IOC_LinkSubStateCmdExecutorReady (8)
 *          • ASSERTION 3: Link MainState remains IOC_LinkStateCmdExecutorActive (5) ← NO FAILURE!
 *          • ASSERTION 4: Subsequent IOC_waitCMD succeeds (link recovered)
 *          • ASSERTION 5: No error propagation to link state (timeout isolated)
 *      @[Architecture Principle]: Polling timeout isolates wait failure from link failure
 *      @[TDD Expectation]: Verify link state cleanup in waitCMD timeout path
 *      @[Status]: TODO - Validate polling recovery mechanism
 *
 * [@AC-3,US-4] Error state propagation (BOTH execution patterns)
 *  ⚪ TC-1: verifyErrorPropagation_byCallbackReturnError_expectFailedStatus  [ERROR-CALLBACK]
 *      @[Purpose]: Validate callback return error propagates to command status FAILED
 *      @[Brief]: Executor callback returns error code, verify status→FAILED, result←error code
 *      @[Strategy]: Service with LinkA1(Initiator) + Client-A1(Executor with error callback)
 *                   → Setup: Callback returns IOC_RESULT_CMD_EXEC_FAILED (-509)
 *                   → IOC_execCMD called, callback executes immediately
 *                   → Query command status: expect FAILED (5)
 *                   → Query command result: expect IOC_RESULT_CMD_EXEC_FAILED (-509)
 *      @[Key Assertions]:
 *          • ASSERTION 1: Initial status = IOC_CMD_STATUS_PENDING (2)
 *          • ASSERTION 2: Status transitions to IOC_CMD_STATUS_PROCESSING (3) when callback starts
 *          • ASSERTION 3: Status transitions to IOC_CMD_STATUS_FAILED (5) when callback returns error
 *          • ASSERTION 4: Result = IOC_RESULT_CMD_EXEC_FAILED (-509) ← ERROR CODE PROPAGATED!
 *          • ASSERTION 5: Link state unaffected (remains ExecutorReady after callback)
 *      @[Architecture Principle]: Callback error return immediately propagates to command state
 *      @[TDD Expectation]: Verify callback error code mapping to command status
 *      @[Status]: TODO - Test error propagation path in callback mode
 *
 *  ⚪ TC-2: verifyErrorPropagation_byAckCmdWithError_expectFailedStatus  [ERROR-POLLING]
 *      @[Purpose]: Validate IOC_ackCMD with error descriptor propagates to command status FAILED
 *      @[Brief]: Executor sets error in descriptor before IOC_ackCMD, verify status→FAILED
 *      @[Strategy]: Service with LinkA1(Executor in polling mode) + Client-A1(Initiator)
 *                   → Client-A1 sends command via IOC_execCMD
 *                   → Executor calls IOC_waitCMD, receives command
 *                   → Executor sets pCmdDesc->status = IOC_CMD_STATUS_FAILED (5)
 *                   → Executor sets pCmdDesc->result = IOC_RESULT_CMD_EXEC_FAILED (-509)
 *                   → Executor calls IOC_ackCMD(LinkA1, pCmdDesc)
 *                   → Initiator queries status: expect FAILED (5), result expect -509
 *      @[Key Assertions]:
 *          • ASSERTION 1: IOC_waitCMD succeeds, returns command descriptor
 *          • ASSERTION 2: Executor explicitly sets status = FAILED (5) before ack
 *          • ASSERTION 3: Executor explicitly sets result = IOC_RESULT_CMD_EXEC_FAILED (-509)
 *          • ASSERTION 4: IOC_ackCMD propagates error to initiator
 *          • ASSERTION 5: Initiator queries status = FAILED, result = -509 ← ERROR PROPAGATED!
 *      @[Architecture Principle]: Polling mode explicit error setting before ack
 *      @[TDD Expectation]: Verify ackCMD error descriptor propagation
 *      @[Status]: TODO - Test error propagation path in polling mode
 *
 * [@AC-4,US-4] Mixed success/failure command independence (BOTH execution patterns)
 *  ⚪ TC-1: verifyMixedResults_bySequentialCallbacks_expectIndependentStates  [ISOLATION-CALLBACK]
 *      @[Purpose]: Validate commands with different outcomes maintain independent states (callback mode)
 *      @[Brief]: Two sequential commands on same link: Cmd1 succeeds, Cmd2 fails, verify isolation
 *      @[Strategy]: Service with LinkA1(Initiator) + Client-A1(Executor)
 *                   → Cmd1: Normal callback, returns IOC_RESULT_SUCCESS → expect SUCCESS state
 *                   → Cmd2: Error callback, returns IOC_RESULT_CMD_EXEC_FAILED → expect FAILED state
 *                   → Verify: Cmd1 status unaffected by Cmd2 failure (state independence)
 *      @[Key Assertions]:
 *          • ASSERTION 1: Cmd1 completes: status=SUCCESS (4), result=IOC_RESULT_SUCCESS (0)
 *          • ASSERTION 2: Cmd2 completes: status=FAILED (5), result=IOC_RESULT_CMD_EXEC_FAILED (-509)
 *          • ASSERTION 3: Cmd1 status unchanged after Cmd2 failure (isolation verified) ← KEY!
 *          • ASSERTION 4: Link state returns to Ready after both commands
 *          • ASSERTION 5: No cross-contamination between command descriptors
 *      @[Architecture Principle]: Command-level isolation prevents error propagation between commands
 *      @[TDD Expectation]: Verify independent command descriptor states in callback mode
 *      @[Status]: TODO - Verify callback mode command isolation
 *
 *  ⚪ TC-2: verifyMixedResults_byWaitAckCycle_expectIndependentStates  [ISOLATION-POLLING]
 *      @[Purpose]: Validate commands with different outcomes maintain independent states (polling mode)
 *      @[Brief]: Two wait/ack cycles on same link: Cmd1 succeeds, Cmd2 fails, verify isolation
 *      @[Strategy]: Service with LinkA1(Executor in polling mode) + Client-A1(Initiator)
 *                   → Cmd1: Executor waitCMD → ackCMD with SUCCESS → expect SUCCESS state
 *                   → Cmd2: Executor waitCMD → ackCMD with FAILED → expect FAILED state
 *                   → Verify: Cmd1 status unaffected by Cmd2 failure (state independence)
 *      @[Key Assertions]:
 *          • ASSERTION 1: Cmd1 completes: executor ackCMD(status=SUCCESS, result=0)
 *          • ASSERTION 2: Cmd2 completes: executor ackCMD(status=FAILED, result=-509)
 *          • ASSERTION 3: Cmd1 status unchanged after Cmd2 failure (isolation verified) ← KEY!
 *          • ASSERTION 4: Link state returns to Ready after both wait/ack cycles
 *          • ASSERTION 5: No cross-contamination between command descriptors
 *      @[Architecture Principle]: Command-level isolation in polling mode
 *      @[TDD Expectation]: Verify independent command descriptor states in polling mode
 *      @[Status]: TODO - Verify polling mode command isolation
 *
 * [@AC-5,US-4] Error recovery and state cleanup (BOTH execution patterns)
 *  ⚪ TC-1: verifyErrorRecovery_byCallbackSuccessAfterFailure_expectStateCleanup  [RECOVERY-CALLBACK]
 *      @[Purpose]: Validate system recovers from callback errors, subsequent operations succeed normally
 *      @[Brief]: Failed command followed by successful command (callback mode), verify state cleanup
 *      @[Strategy]: Service with LinkA1(Initiator) + Client-A1(Executor)
 *                   → Cmd1: Callback returns error → verify FAILED state
 *                   → Verify: Link returns to Ready (error cleanup)
 *                   → Cmd2: Normal callback returns success → verify SUCCESS state
 *                   → Verify: Cmd2 starts fresh (INITIALIZED→PENDING→PROCESSING→SUCCESS)
 *      @[Key Assertions]:
 *          • ASSERTION 1: Cmd1 fails: status=FAILED, result=IOC_RESULT_CMD_EXEC_FAILED
 *          • ASSERTION 2: After Cmd1: link state = CmdInitiatorReady (error cleaned up)
 *          • ASSERTION 3: Cmd2 initializes fresh: status=INITIALIZED (1) ← CLEAN STATE!
 *          • ASSERTION 4: Cmd2 succeeds: status=SUCCESS (4), result=IOC_RESULT_SUCCESS (0)
 *          • ASSERTION 5: No residual error state from Cmd1 affects Cmd2 (complete recovery)
 *      @[Architecture Principle]: Callback error recovery ensures system resilience
 *      @[TDD Expectation]: Verify state cleanup mechanism after callback errors
 *      @[Status]: TODO - Verify callback mode error recovery
 *
 *  ⚪ TC-2: verifyErrorRecovery_byPollingSuccessAfterFailure_expectStateCleanup  [RECOVERY-POLLING]
 *      @[Purpose]: Validate system recovers from polling errors, subsequent operations succeed normally
 *      @[Brief]: Failed command followed by successful command (polling mode), verify state cleanup
 *      @[Strategy]: Service with LinkA1(Executor in polling mode) + Client-A1(Initiator)
 *                   → Cmd1: Executor ackCMD with error → verify FAILED state
 *                   → Verify: Link returns to Ready (error cleanup)
 *                   → Cmd2: Executor ackCMD with success → verify SUCCESS state
 *                   → Verify: Cmd2 starts fresh (INITIALIZED→PENDING→PROCESSING→SUCCESS)
 *      @[Key Assertions]:
 *          • ASSERTION 1: Cmd1 fails: executor ackCMD(status=FAILED, result=-509)
 *          • ASSERTION 2: After Cmd1: link state = CmdExecutorReady (error cleaned up)
 *          • ASSERTION 3: Cmd2 initializes fresh: status=INITIALIZED (1) ← CLEAN STATE!
 *          • ASSERTION 4: Cmd2 succeeds: executor ackCMD(status=SUCCESS, result=0)
 *          • ASSERTION 5: No residual error state from Cmd1 affects Cmd2 (complete recovery)
 *      @[Architecture Principle]: Polling error recovery ensures system resilience
 *      @[TDD Expectation]: Verify state cleanup mechanism after polling errors
 *      @[Status]: TODO - Verify polling mode error recovery
 *
 **************************************************************************************************/
//======>END OF TEST CASES=========================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF AC-1 TC-1: DESCRIPTOR TIMEOUT IN CALLBACK MODE==================================

TEST(UT_CommandStateUS4, verifyCommandTimeout_byDescriptorTimeout_expectTimeoutStatus) {
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════════════════════════════════╗\n");
    printf("║  🧪 AC-1 TC-1: Descriptor Timeout in Callback Mode                                      ║\n");
    printf("║  Purpose: Validate pCmdDesc->TimeoutMs limits callback execution duration               ║\n");
    printf("║  Strategy: Set TimeoutMs=100ms, callback delays 200ms, verify TIMEOUT at ~100ms        ║\n");
    printf("╚══════════════════════════════════════════════════════════════════════════════════════════╝\n");

    IOC_Result_T ResultValue = IOC_RESULT_BUG;

    // ┌──────────────────────────────────────────────────────────────┐
    // │                    🏗️ SETUP PHASE                            │
    // └──────────────────────────────────────────────────────────────┘
    printf("🏗️ [SETUP] Creating Service with CmdInitiator capability\n");

    // Service URI configuration
    IOC_SrvURI_T SrvURI = {
        .pProtocol = IOC_SRV_PROTO_FIFO, .pHost = IOC_SRV_HOST_LOCAL_PROCESS, .pPath = "TimeoutTestService"};

    // Client-A1 configuration: CmdExecutor with slow callback (200ms delay)
    printf("🏗️ [SETUP] Client-A1 will act as Executor with 200ms callback delay\n");

    struct Client1PrivData {
        int callbackInvoked = 0;
        std::chrono::steady_clock::time_point callbackStartTime;
        std::chrono::steady_clock::time_point callbackEndTime;
    } client1PrivData;

    auto client1ExecutorCb = [](IOC_LinkID_T LinkID, IOC_CmdDesc_pT pCmdDesc, void* pPrivData) -> IOC_Result_T {
        auto* privData = static_cast<Client1PrivData*>(pPrivData);
        privData->callbackStartTime = std::chrono::steady_clock::now();
        privData->callbackInvoked++;

        IOC_CmdID_T cmdID = IOC_CmdDesc_getCmdID(pCmdDesc);
        printf("⏱️  [CALLBACK] Executor callback invoked (cmdID=%llu), delaying 200ms...\n", (unsigned long long)cmdID);

        // Simulate slow execution - EXCEEDS timeout!
        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        privData->callbackEndTime = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(privData->callbackEndTime -
                                                                             privData->callbackStartTime)
                           .count();
        printf("⏱️  [CALLBACK] Callback completed after %lldms\n", (long long)elapsed);

        return IOC_RESULT_SUCCESS;
    };

    IOC_CmdID_T supportedCmdIDs[] = {1};
    IOC_CmdUsageArgs_T client1CmdUsageArgs = {
        .CbExecCmd_F = client1ExecutorCb, .pCbPrivData = &client1PrivData, .CmdNum = 1, .pCmdIDs = supportedCmdIDs};

    // Service configuration: CmdInitiator capability
    IOC_SrvArgs_T srvArgs = {
        .SrvURI = SrvURI, .Flags = IOC_SRVFLAG_NONE, .UsageCapabilites = IOC_LinkUsageCmdInitiator};

    IOC_SrvID_T srvID = IOC_ID_INVALID;
    ResultValue = IOC_onlineService(&srvID, &srvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);
    ASSERT_NE(IOC_ID_INVALID, srvID);
    printf("🏗️ [SETUP] Service online: SrvID=%llu\n", (unsigned long long)srvID);

    // Client connection args
    IOC_ConnArgs_T client1ConnArgs = {
        .SrvURI = SrvURI, .Usage = IOC_LinkUsageCmdExecutor, .UsageArgs = {.pCmd = &client1CmdUsageArgs}};

    IOC_LinkID_T client1LinkID = IOC_ID_INVALID;
    std::thread client1Thread([&] {
        IOC_Result_T connResult = IOC_connectService(&client1LinkID, &client1ConnArgs, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, connResult);
        ASSERT_NE(IOC_ID_INVALID, client1LinkID);
    });

    // Service accepts Client1
    IOC_LinkID_T srvLinkID1 = IOC_ID_INVALID;
    ResultValue = IOC_acceptClient(srvID, &srvLinkID1, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);
    ASSERT_NE(IOC_ID_INVALID, srvLinkID1);

    if (client1Thread.joinable()) client1Thread.join();

    printf("🏗️ [SETUP] Link established: Service(LinkID=%llu) ←→ Client-A1(LinkID=%llu)\n",
           (unsigned long long)srvLinkID1, (unsigned long long)client1LinkID);

    // ┌──────────────────────────────────────────────────────────────┐
    // │                    📋 BEHAVIOR PHASE                         │
    // └──────────────────────────────────────────────────────────────┘
    printf("📋 [BEHAVIOR] Preparing command with 100ms timeout\n");

    // ⚠️ IMPORTANT: DON'T manually set Status! Protocol layer manages state transitions:
    //    • INITIALIZED (1) - After zero-init (implicit)
    //    • PROCESSING (3)  - Set by protocol BEFORE callback execution
    //    • TIMEOUT (6)     - Set by protocol when callback exceeds TimeoutMs
    IOC_CmdDesc_T cmdDesc = {};  // Status = INITIALIZED (1) - Let protocol manage transitions!
    cmdDesc.CmdID = 1;
    cmdDesc.TimeoutMs = 100;  // KEY: Set 100ms descriptor timeout (callback will take 200ms!)

    printf("📋 [BEHAVIOR] Command configured: CmdID=1, TimeoutMs=100ms (DESCRIPTOR TIMEOUT)\n");
    printf("📋 [BEHAVIOR] Executor callback will delay 200ms (exceeds timeout by 100ms)\n");
    printf("📋 [BEHAVIOR] IOC_execCMD called with pOption=NULL (NO API-level timeout)\n");
    printf(
        "📋 [BEHAVIOR] Protocol will manage state: INITIALIZED → PROCESSING → TIMEOUT\n");  //@KeyVerifyPoint-1: Initial
                                                                                            // status must be PENDING
                                                                                            // after IOC_execCMD
    printf("📋 [BEHAVIOR] Executing command (expecting timeout)...\n");
    auto execStartTime = std::chrono::steady_clock::now();

    ResultValue = IOC_execCMD(srvLinkID1, &cmdDesc, NULL);

    auto execEndTime = std::chrono::steady_clock::now();
    auto execDuration = std::chrono::duration_cast<std::chrono::milliseconds>(execEndTime - execStartTime).count();

    printf("📋 [BEHAVIOR] IOC_execCMD returned: result=%d, duration=%lldms\n", ResultValue, (long long)execDuration);
    printf("📋 [BEHAVIOR] Callback invoked: %d times\n", client1PrivData.callbackInvoked);

    // ┌──────────────────────────────────────────────────────────────┐
    // │                     ✅ VERIFY PHASE                          │
    // └──────────────────────────────────────────────────────────────┘

    //@KeyVerifyPoint-1: Command status must be TIMEOUT
    printf("✅ [VERIFY] ASSERTION 1: Command status transitions to TIMEOUT\n");
    IOC_CmdStatus_E actualStatus = IOC_CmdDesc_getStatus(&cmdDesc);
    printf("    • Command status: %d (expected: IOC_CMD_STATUS_TIMEOUT=%d)\n", actualStatus, IOC_CMD_STATUS_TIMEOUT);
    VERIFY_KEYPOINT_EQ(actualStatus, IOC_CMD_STATUS_TIMEOUT,
                       "Command must transition to TIMEOUT after exceeding TimeoutMs");

    //@KeyVerifyPoint-2: Command result must be IOC_RESULT_TIMEOUT
    printf("✅ [VERIFY] ASSERTION 2: Command result = IOC_RESULT_TIMEOUT\n");
    IOC_Result_T actualResult = IOC_CmdDesc_getResult(&cmdDesc);
    printf("    • Command result: %d (expected: IOC_RESULT_TIMEOUT=%d)\n", actualResult, IOC_RESULT_TIMEOUT);
    VERIFY_KEYPOINT_EQ(actualResult, IOC_RESULT_TIMEOUT, "Command result must reflect timeout condition");

    //@KeyVerifyPoint-3: Callback was invoked despite timeout
    printf("✅ [VERIFY] ASSERTION 3: Executor callback was invoked\n");
    printf("    • Callback invocations: %d (expected: 1)\n", client1PrivData.callbackInvoked);
    VERIFY_KEYPOINT_EQ(client1PrivData.callbackInvoked, 1, "Callback must be invoked even if it eventually times out");

    //@KeyVerifyPoint-4: Timeout enforced PRECISELY at ~100ms (NOT 150ms or 200ms!)
    printf("✅ [VERIFY] ASSERTION 4: IOC_execCMD returned at ~100ms (timeout enforcement)\n");
    printf("    • Actual execution duration: %lldms\n", (long long)execDuration);
    printf("    • Expected timeout: 100ms ± 10ms tolerance\n");
    // ⚠️ CRITICAL: Verify timeout enforced IMMEDIATELY at 100ms, not after callback completes (200ms)
    // This proves protocol layer ACTIVELY enforces timeout, not passively waiting for callback
    ASSERT_GE(execDuration, 90);   // Minimum: timeout - 10ms tolerance
    ASSERT_LE(execDuration, 120);  // Maximum: timeout + 20ms tolerance (pthread scheduling overhead)
    printf("    • ✅ Timeout enforced precisely! (IOC didn't wait for 200ms callback completion)\n");

    printf("\n");
    printf("✅ [RESULT] Descriptor timeout in callback mode verified:\n");
    printf("   • TimeoutMs=100ms configured (DESCRIPTOR TIMEOUT) ✅\n");
    printf("   • pOption=NULL (NO API-level timeout) ✅\n");
    printf("   • Callback delayed 200ms (BEHAVIOR) ✅\n");
    printf("   • Status = TIMEOUT (ASSERTION 1) ✅\n");
    printf("   • Result = IOC_RESULT_TIMEOUT (ASSERTION 2) ✅\n");
    printf("   • Callback was invoked (ASSERTION 3) ✅\n");
    printf("   • Timeout enforced at ~100ms (ASSERTION 4) ✅ ← CRITICAL!\n");
    printf("   • Descriptor timeout prevents indefinite callback execution (PRINCIPLE) ✅\n");

    // Cleanup
    if (client1LinkID != IOC_ID_INVALID) IOC_closeLink(client1LinkID);
    if (srvLinkID1 != IOC_ID_INVALID) IOC_closeLink(srvLinkID1);
    if (srvID != IOC_ID_INVALID) IOC_offlineService(srvID);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF AC-1 TC-2: WAITCMD OPTION TIMEOUT IN POLLING MODE===============================

TEST(UT_CommandStateUS4, verifyCommandTimeout_byWaitCmdOptionTimeout_expectTimeoutStatus) {
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════════════════════════════════╗\n");
    printf("║  🧪 AC-1 TC-2: waitCMD Option Timeout in Polling Mode                                   ║\n");
    printf("║  Purpose: Validate pOption->TimeoutUS prevents indefinite blocking in IOC_waitCMD       ║\n");
    printf("║  Strategy: Executor calls waitCMD with 100ms timeout, no command sent, verify timeout   ║\n");
    printf("╚══════════════════════════════════════════════════════════════════════════════════════════╝\n");

    IOC_Result_T ResultValue = IOC_RESULT_BUG;

    // ┌──────────────────────────────────────────────────────────────┐
    // │                    🏗️ SETUP PHASE                            │
    // └──────────────────────────────────────────────────────────────┘
    printf("🏗️ [SETUP] Creating Service with CmdExecutor capability (POLLING MODE)\n");

    // Service URI configuration
    IOC_SrvURI_T SrvURI = {
        .pProtocol = IOC_SRV_PROTO_FIFO, .pHost = IOC_SRV_HOST_LOCAL_PROCESS, .pPath = "WaitCmdTimeoutTestService"};

    // Client-A1 configuration: CmdInitiator (will NOT send command)
    printf("🏗️ [SETUP] Client-A1 will act as Initiator (but will NOT send command)\n");

    // Service configuration: CmdExecutor capability (POLLING MODE - NO callback!)
    IOC_SrvArgs_T srvArgs = {.SrvURI = SrvURI, .Flags = IOC_SRVFLAG_NONE, .UsageCapabilites = IOC_LinkUsageCmdExecutor};

    IOC_SrvID_T srvID = IOC_ID_INVALID;
    ResultValue = IOC_onlineService(&srvID, &srvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);
    ASSERT_NE(IOC_ID_INVALID, srvID);
    printf("🏗️ [SETUP] Service online: SrvID=%llu (Executor in polling mode)\n", (unsigned long long)srvID);

    // Client connection args: CmdInitiator (no callback setup needed)
    IOC_ConnArgs_T client1ConnArgs = {.SrvURI = SrvURI, .Usage = IOC_LinkUsageCmdInitiator, .UsageArgs = {}};

    IOC_LinkID_T client1LinkID = IOC_ID_INVALID;
    std::thread client1Thread([&] {
        IOC_Result_T connResult = IOC_connectService(&client1LinkID, &client1ConnArgs, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, connResult);
        ASSERT_NE(IOC_ID_INVALID, client1LinkID);
    });

    // Service accepts Client1
    IOC_LinkID_T srvLinkID1 = IOC_ID_INVALID;
    ResultValue = IOC_acceptClient(srvID, &srvLinkID1, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);
    ASSERT_NE(IOC_ID_INVALID, srvLinkID1);

    if (client1Thread.joinable()) client1Thread.join();

    printf("🏗️ [SETUP] Link established: Service(LinkID=%llu, Executor) ←→ Client-A1(LinkID=%llu, Initiator)\n",
           (unsigned long long)srvLinkID1, (unsigned long long)client1LinkID);

    // ┌──────────────────────────────────────────────────────────────┐
    // │                    📋 BEHAVIOR PHASE                         │
    // └──────────────────────────────────────────────────────────────┘
    printf("📋 [BEHAVIOR] Preparing to call IOC_waitCMD with 100ms API-level timeout\n");
    printf("📋 [BEHAVIOR] Client-A1 will NOT send command → Executor would wait indefinitely\n");
    printf("📋 [BEHAVIOR] pOption->TimeoutUS=100000 (100ms) should prevent indefinite blocking\n");

    // Define timeout option: 100ms = 100,000 microseconds
    IOC_Option_defineTimeout(waitOpt, 100000);
    printf("📋 [BEHAVIOR] Created pOption with TimeoutUS=100000us (100ms)\n");

    IOC_CmdDesc_T cmdDesc = {};  // Will NOT be populated (no command sent)
    printf("📋 [BEHAVIOR] Calling IOC_waitCMD (expecting timeout)...\n");

    auto waitStartTime = std::chrono::steady_clock::now();

    ResultValue = IOC_waitCMD(srvLinkID1, &cmdDesc, &waitOpt);

    auto waitEndTime = std::chrono::steady_clock::now();
    auto waitDuration = std::chrono::duration_cast<std::chrono::milliseconds>(waitEndTime - waitStartTime).count();

    printf("📋 [BEHAVIOR] IOC_waitCMD returned: result=%d, duration=%lldms\n", ResultValue, (long long)waitDuration);

    // ┌──────────────────────────────────────────────────────────────┐
    // │                     ✅ VERIFY PHASE                          │
    // └──────────────────────────────────────────────────────────────┘

    //@KeyVerifyPoint-1: IOC_waitCMD must return IOC_RESULT_TIMEOUT
    printf("✅ [VERIFY] ASSERTION 1: IOC_waitCMD returns IOC_RESULT_TIMEOUT\n");
    printf("    • Actual result: %d (expected: IOC_RESULT_TIMEOUT=%d)\n", ResultValue, IOC_RESULT_TIMEOUT);
    VERIFY_KEYPOINT_EQ(ResultValue, IOC_RESULT_TIMEOUT,
                       "IOC_waitCMD must return TIMEOUT when no command arrives within timeout");

    //@KeyVerifyPoint-2: Timeout enforced PRECISELY at ~100ms (NOT indefinite wait!)
    printf("✅ [VERIFY] ASSERTION 2: IOC_waitCMD returned at ~100ms (timeout enforcement)\n");
    printf("    • Actual wait duration: %lldms\n", (long long)waitDuration);
    printf("    • Expected timeout: 100ms ± 20ms tolerance\n");
    // ⚠️ CRITICAL: Verify timeout enforced at 100ms, proving pOption timeout works
    ASSERT_GE(waitDuration, 80);   // Minimum: timeout - 20ms tolerance
    ASSERT_LE(waitDuration, 130);  // Maximum: timeout + 30ms tolerance (pthread scheduling overhead)
    printf("    • ✅ Timeout enforced precisely! (IOC didn't wait indefinitely)\n");

    //@KeyVerifyPoint-3: Link state must be ExecutorReady (recovered)
    printf("✅ [VERIFY] ASSERTION 3: Link state = IOC_LinkSubStateCmdExecutorReady (recovery)\n");
    IOC_LinkState_T linkMainState = IOC_LinkStateUndefined;
    IOC_LinkSubState_T linkSubState = IOC_LinkSubStateDefault;
    ResultValue = IOC_getLinkState(srvLinkID1, &linkMainState, &linkSubState);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);
    printf("    • Link main state: %d (expected: IOC_LinkStateReady=%d)\n", linkMainState, IOC_LinkStateReady);
    printf("    • Link sub state: %d (expected: IOC_LinkSubStateCmdExecutorReady=%d)\n", linkSubState,
           IOC_LinkSubStateCmdExecutorReady);
    VERIFY_KEYPOINT_EQ(linkMainState, IOC_LinkStateReady, "Link main state must be Ready after timeout");
    VERIFY_KEYPOINT_EQ(linkSubState, IOC_LinkSubStateCmdExecutorReady,
                       "Link sub state must return to ExecutorReady after timeout (auto recovery)");

    //@KeyVerifyPoint-4: No command descriptor populated (timeout before arrival)
    printf("✅ [VERIFY] ASSERTION 4: No command descriptor populated (timeout before command arrival)\n");
    IOC_CmdID_T cmdID = IOC_CmdDesc_getCmdID(&cmdDesc);
    printf("    • Command ID: %llu (expected: 0 - no command)\n", (unsigned long long)cmdID);
    VERIFY_KEYPOINT_EQ(cmdID, 0ULL, "No command should be populated when timeout occurs before arrival");

    //@KeyVerifyPoint-5: Subsequent IOC_waitCMD succeeds (link operational)
    printf("✅ [VERIFY] ASSERTION 5: Subsequent IOC_waitCMD succeeds (link operational)\n");
    IOC_CmdDesc_T cmdDesc2 = {};
    IOC_Option_defineTimeout(waitOpt2, 50000);  // 50ms timeout for quick test
    printf("    • Calling IOC_waitCMD again with 50ms timeout...\n");

    auto wait2StartTime = std::chrono::steady_clock::now();
    ResultValue = IOC_waitCMD(srvLinkID1, &cmdDesc2, &waitOpt2);
    auto wait2EndTime = std::chrono::steady_clock::now();
    auto wait2Duration = std::chrono::duration_cast<std::chrono::milliseconds>(wait2EndTime - wait2StartTime).count();

    printf("    • Second IOC_waitCMD returned: result=%d, duration=%lldms\n", ResultValue, (long long)wait2Duration);
    VERIFY_KEYPOINT_EQ(ResultValue, IOC_RESULT_TIMEOUT, "Subsequent waitCMD should also timeout (link operational)");
    printf("    • ✅ Link remains operational after first timeout!\n");

    printf("\n");
    printf("✅ [RESULT] waitCMD pOption timeout in polling mode verified:\n");
    printf("   • pOption->TimeoutUS=100000us (100ms) configured ✅\n");
    printf("   • No command sent (would wait indefinitely without timeout) ✅\n");
    printf("   • IOC_waitCMD returned IOC_RESULT_TIMEOUT (ASSERTION 1) ✅\n");
    printf("   • Timeout enforced at ~%lldms (ASSERTION 2) ✅ ← CRITICAL!\n", (long long)waitDuration);
    printf("   • Link state = ExecutorReady (ASSERTION 3) ✅\n");
    printf("   • No command populated (ASSERTION 4) ✅\n");
    printf("   • Subsequent waitCMD succeeds (ASSERTION 5) ✅\n");
    printf("   • API option timeout prevents indefinite blocking (PRINCIPLE) ✅\n");

    // Cleanup
    if (client1LinkID != IOC_ID_INVALID) IOC_closeLink(client1LinkID);
    if (srvLinkID1 != IOC_ID_INVALID) IOC_closeLink(srvLinkID1);
    if (srvID != IOC_ID_INVALID) IOC_offlineService(srvID);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF AC-1 TC-3: EXECCMD OPTION TIMEOUT IN CALLBACK MODE==============================

TEST(UT_CommandStateUS4, verifyCommandTimeout_byExecCmdOptionTimeout_expectTimeoutStatus) {
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════════════════════════════════╗\n");
    printf("║  🧪 AC-1 TC-3: execCMD Option Timeout in Callback Mode                                  ║\n");
    printf("║  Purpose: Validate pOption->TimeoutUS limits entire execCMD operation duration          ║\n");
    printf("║  Strategy: Set TimeoutMs=0, pOption=100ms, callback delays 200ms, verify timeout        ║\n");
    printf("╚══════════════════════════════════════════════════════════════════════════════════════════╝\n");

    IOC_Result_T ResultValue = IOC_RESULT_BUG;

    // ┌──────────────────────────────────────────────────────────────┐
    // │                    🏗️ SETUP PHASE                            │
    // └──────────────────────────────────────────────────────────────┘
    printf("🏗️ [SETUP] Creating Service with CmdInitiator capability\n");

    // Service URI configuration
    IOC_SrvURI_T SrvURI = {.pProtocol = IOC_SRV_PROTO_FIFO,
                           .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
                           .pPath = "ExecCmdOptionTimeoutTestService"};

    // Client-A1 configuration: CmdExecutor with slow callback (200ms delay)
    printf("🏗️ [SETUP] Client-A1 will act as Executor with 200ms callback delay\n");

    struct Client1PrivData {
        int callbackInvoked = 0;
        std::chrono::steady_clock::time_point callbackStartTime;
        std::chrono::steady_clock::time_point callbackEndTime;
    } client1PrivData;

    auto client1ExecutorCb = [](IOC_LinkID_T LinkID, IOC_CmdDesc_pT pCmdDesc, void* pPrivData) -> IOC_Result_T {
        auto* privData = static_cast<Client1PrivData*>(pPrivData);
        privData->callbackStartTime = std::chrono::steady_clock::now();
        privData->callbackInvoked++;

        IOC_CmdID_T cmdID = IOC_CmdDesc_getCmdID(pCmdDesc);
        printf("⏱️  [CALLBACK] Executor callback invoked (cmdID=%llu), delaying 200ms...\n", (unsigned long long)cmdID);

        // Simulate slow execution - EXCEEDS pOption timeout!
        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        privData->callbackEndTime = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(privData->callbackEndTime -
                                                                             privData->callbackStartTime)
                           .count();
        printf("⏱️  [CALLBACK] Callback completed after %lldms\n", (long long)elapsed);

        return IOC_RESULT_SUCCESS;
    };

    IOC_CmdID_T supportedCmdIDs[] = {1};
    IOC_CmdUsageArgs_T client1CmdUsageArgs = {
        .CbExecCmd_F = client1ExecutorCb, .pCbPrivData = &client1PrivData, .CmdNum = 1, .pCmdIDs = supportedCmdIDs};

    // Service configuration: CmdInitiator capability
    IOC_SrvArgs_T srvArgs = {
        .SrvURI = SrvURI, .Flags = IOC_SRVFLAG_NONE, .UsageCapabilites = IOC_LinkUsageCmdInitiator};

    IOC_SrvID_T srvID = IOC_ID_INVALID;
    ResultValue = IOC_onlineService(&srvID, &srvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);
    ASSERT_NE(IOC_ID_INVALID, srvID);
    printf("🏗️ [SETUP] Service online: SrvID=%llu\n", (unsigned long long)srvID);

    // Client connection args
    IOC_ConnArgs_T client1ConnArgs = {
        .SrvURI = SrvURI, .Usage = IOC_LinkUsageCmdExecutor, .UsageArgs = {.pCmd = &client1CmdUsageArgs}};

    IOC_LinkID_T client1LinkID = IOC_ID_INVALID;
    std::thread client1Thread([&] {
        IOC_Result_T connResult = IOC_connectService(&client1LinkID, &client1ConnArgs, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, connResult);
        ASSERT_NE(IOC_ID_INVALID, client1LinkID);
    });

    // Service accepts Client1
    IOC_LinkID_T srvLinkID1 = IOC_ID_INVALID;
    ResultValue = IOC_acceptClient(srvID, &srvLinkID1, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);
    ASSERT_NE(IOC_ID_INVALID, srvLinkID1);

    if (client1Thread.joinable()) client1Thread.join();

    printf("🏗️ [SETUP] Link established: Service(LinkID=%llu) ←→ Client-A1(LinkID=%llu)\n",
           (unsigned long long)srvLinkID1, (unsigned long long)client1LinkID);

    // ┌──────────────────────────────────────────────────────────────┐
    // │                    📋 BEHAVIOR PHASE                         │
    // └──────────────────────────────────────────────────────────────┘
    printf("📋 [BEHAVIOR] Preparing command with NO descriptor timeout but WITH pOption timeout\n");

    // KEY DIFFERENCE from TC-1: TimeoutMs=0 (NO descriptor timeout)
    IOC_CmdDesc_T cmdDesc = {};
    cmdDesc.CmdID = 1;
    cmdDesc.TimeoutMs = 0;  // CRITICAL: NO descriptor-level timeout!

    printf("📋 [BEHAVIOR] Command configured: CmdID=1, TimeoutMs=0 (NO descriptor timeout)\n");
    printf("📋 [BEHAVIOR] Executor callback will delay 200ms (exceeds pOption timeout by 100ms)\n");

    // Define API-level timeout option: 100ms = 100,000 microseconds
    IOC_Option_defineTimeout(execOpt, 100000);
    printf("📋 [BEHAVIOR] Created pOption with TimeoutUS=100000us (100ms) - API-LEVEL TIMEOUT\n");
    printf("📋 [BEHAVIOR] IOC_execCMD called with pOption (API-level timeout enforcement)\n");
    printf("📋 [BEHAVIOR] Expected: pOption timeout enforces at ~100ms (NOT 200ms!)\n");
    printf("📋 [BEHAVIOR] Executing command (expecting API-level timeout)...\n");

    auto execStartTime = std::chrono::steady_clock::now();

    ResultValue = IOC_execCMD(srvLinkID1, &cmdDesc, &execOpt);

    auto execEndTime = std::chrono::steady_clock::now();
    auto execDuration = std::chrono::duration_cast<std::chrono::milliseconds>(execEndTime - execStartTime).count();

    printf("📋 [BEHAVIOR] IOC_execCMD returned: result=%d, duration=%lldms\n", ResultValue, (long long)execDuration);
    printf("📋 [BEHAVIOR] Callback invoked: %d times\n", client1PrivData.callbackInvoked);

    // ┌──────────────────────────────────────────────────────────────┐
    // │                     ✅ VERIFY PHASE                          │
    // └──────────────────────────────────────────────────────────────┘

    //@KeyVerifyPoint-1: Command status must be TIMEOUT
    printf("✅ [VERIFY] ASSERTION 1: Command status transitions to TIMEOUT\n");
    IOC_CmdStatus_E actualStatus = IOC_CmdDesc_getStatus(&cmdDesc);
    printf("    • Command status: %d (expected: IOC_CMD_STATUS_TIMEOUT=%d)\n", actualStatus, IOC_CMD_STATUS_TIMEOUT);
    VERIFY_KEYPOINT_EQ(actualStatus, IOC_CMD_STATUS_TIMEOUT,
                       "Command must transition to TIMEOUT when pOption timeout exceeded");

    //@KeyVerifyPoint-2: Command result must be IOC_RESULT_TIMEOUT
    printf("✅ [VERIFY] ASSERTION 2: Command result = IOC_RESULT_TIMEOUT\n");
    IOC_Result_T actualResult = IOC_CmdDesc_getResult(&cmdDesc);
    printf("    • Command result: %d (expected: IOC_RESULT_TIMEOUT=%d)\n", actualResult, IOC_RESULT_TIMEOUT);
    VERIFY_KEYPOINT_EQ(actualResult, IOC_RESULT_TIMEOUT, "Command result must reflect API option timeout");

    //@KeyVerifyPoint-3: pOption timeout enforced PRECISELY at ~100ms (NOT 200ms!)
    printf("✅ [VERIFY] ASSERTION 3: IOC_execCMD returned at ~100ms (pOption timeout enforcement)\n");
    printf("    • Actual execution duration: %lldms\n", (long long)execDuration);
    printf("    • Expected pOption timeout: 100ms ± 10ms tolerance\n");
    // ⚠️ CRITICAL: Verify pOption timeout enforced at 100ms, NOT descriptor (0ms) or callback (200ms)
    ASSERT_GE(execDuration, 90);   // Minimum: timeout - 10ms tolerance
    ASSERT_LE(execDuration, 120);  // Maximum: timeout + 20ms tolerance
    printf("    • ✅ pOption timeout enforced precisely! (API-level timeout at 100ms, NOT 200ms)\n");

    //@KeyVerifyPoint-4: Callback was invoked despite timeout
    printf("✅ [VERIFY] ASSERTION 4: Executor callback was invoked\n");
    printf("    • Callback invocations: %d (expected: 1)\n", client1PrivData.callbackInvoked);
    VERIFY_KEYPOINT_EQ(client1PrivData.callbackInvoked, 1, "Callback must be invoked even if pOption times out");

    printf("\n");
    printf("✅ [RESULT] execCMD pOption timeout in callback mode verified:\n");
    printf("   • TimeoutMs=0 (NO descriptor timeout) ✅\n");
    printf("   • pOption->TimeoutUS=100000us (100ms API-level timeout) ✅\n");
    printf("   • Callback delayed 200ms (BEHAVIOR) ✅\n");
    printf("   • Status = TIMEOUT (ASSERTION 1) ✅\n");
    printf("   • Result = IOC_RESULT_TIMEOUT (ASSERTION 2) ✅\n");
    printf("   • Timeout enforced at ~100ms (ASSERTION 3) ✅ ← CRITICAL!\n");
    printf("   • Callback was invoked (ASSERTION 4) ✅\n");
    printf("   • API option timeout prevents indefinite execCMD blocking (PRINCIPLE) ✅\n");

    // Cleanup
    if (client1LinkID != IOC_ID_INVALID) IOC_closeLink(client1LinkID);
    if (srvLinkID1 != IOC_ID_INVALID) IOC_closeLink(srvLinkID1);
    if (srvID != IOC_ID_INVALID) IOC_offlineService(srvID);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF AC-2 TC-1: LINK RECOVERY AFTER CALLBACK TIMEOUT=================================

TEST(UT_CommandStateUS4, verifyLinkRecovery_afterCallbackTimeout_expectReadyState) {
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════════════════════════════════╗\n");
    printf("║  🧪 AC-2 TC-1: Link Recovery After Callback Timeout                                     ║\n");
    printf("║  Purpose: Validate link returns to Ready state after callback timeout                   ║\n");
    printf("║  Strategy: Timeout command → verify recovery → send 2nd command successfully            ║\n");
    printf("╚══════════════════════════════════════════════════════════════════════════════════════════╝\n");

    IOC_Result_T ResultValue = IOC_RESULT_BUG;

    // ┌──────────────────────────────────────────────────────────────┐
    // │                    🏗️ SETUP PHASE                            │
    // └──────────────────────────────────────────────────────────────┘
    printf("🏗️ [SETUP] Creating Service with CmdInitiator capability\n");

    // Service URI configuration
    IOC_SrvURI_T SrvURI = {
        .pProtocol = IOC_SRV_PROTO_FIFO, .pHost = IOC_SRV_HOST_LOCAL_PROCESS, .pPath = "LinkRecoveryTestService"};

    // Client-A1 configuration: CmdExecutor with configurable callback delay
    printf("🏗️ [SETUP] Client-A1 will act as Executor with variable callback delay\n");

    struct Client1PrivData {
        int callbackInvoked = 0;
        int delayMs = 200;  // Can be changed for 2nd command
    } client1PrivData;

    auto client1ExecutorCb = [](IOC_LinkID_T LinkID, IOC_CmdDesc_pT pCmdDesc, void* pPrivData) -> IOC_Result_T {
        auto* privData = static_cast<Client1PrivData*>(pPrivData);
        privData->callbackInvoked++;

        IOC_CmdID_T cmdID = IOC_CmdDesc_getCmdID(pCmdDesc);
        printf("⏱️  [CALLBACK] Executor callback invoked (cmdID=%llu, invocation #%d), delaying %dms...\n",
               (unsigned long long)cmdID, privData->callbackInvoked, privData->delayMs);

        std::this_thread::sleep_for(std::chrono::milliseconds(privData->delayMs));

        printf("⏱️  [CALLBACK] Callback completed (cmdID=%llu)\n", (unsigned long long)cmdID);
        return IOC_RESULT_SUCCESS;
    };

    IOC_CmdID_T supportedCmdIDs[] = {1, 2};
    IOC_CmdUsageArgs_T client1CmdUsageArgs = {
        .CbExecCmd_F = client1ExecutorCb, .pCbPrivData = &client1PrivData, .CmdNum = 2, .pCmdIDs = supportedCmdIDs};

    // Service configuration: CmdInitiator capability
    IOC_SrvArgs_T srvArgs = {
        .SrvURI = SrvURI, .Flags = IOC_SRVFLAG_NONE, .UsageCapabilites = IOC_LinkUsageCmdInitiator};

    IOC_SrvID_T srvID = IOC_ID_INVALID;
    ResultValue = IOC_onlineService(&srvID, &srvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);
    ASSERT_NE(IOC_ID_INVALID, srvID);
    printf("🏗️ [SETUP] Service online: SrvID=%llu\n", (unsigned long long)srvID);

    // Client connection args
    IOC_ConnArgs_T client1ConnArgs = {
        .SrvURI = SrvURI, .Usage = IOC_LinkUsageCmdExecutor, .UsageArgs = {.pCmd = &client1CmdUsageArgs}};

    IOC_LinkID_T client1LinkID = IOC_ID_INVALID;
    std::thread client1Thread([&] {
        IOC_Result_T connResult = IOC_connectService(&client1LinkID, &client1ConnArgs, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, connResult);
        ASSERT_NE(IOC_ID_INVALID, client1LinkID);
    });

    // Service accepts Client1
    IOC_LinkID_T srvLinkID1 = IOC_ID_INVALID;
    ResultValue = IOC_acceptClient(srvID, &srvLinkID1, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);
    ASSERT_NE(IOC_ID_INVALID, srvLinkID1);

    if (client1Thread.joinable()) client1Thread.join();

    printf("🏗️ [SETUP] Link established: Service(LinkID=%llu) ←→ Client-A1(LinkID=%llu)\n",
           (unsigned long long)srvLinkID1, (unsigned long long)client1LinkID);

    // ┌──────────────────────────────────────────────────────────────┐
    // │                    📋 BEHAVIOR PHASE                         │
    // └──────────────────────────────────────────────────────────────┘
    printf("📋 [BEHAVIOR] Executing 1st command with timeout (will timeout)\n");

    // Cmd1: TimeoutMs=100ms, callback delays 200ms → timeout occurs
    IOC_CmdDesc_T cmdDesc1 = {};
    cmdDesc1.CmdID = 1;
    cmdDesc1.TimeoutMs = 100;       // 100ms timeout
    client1PrivData.delayMs = 200;  // Callback will take 200ms

    printf("📋 [BEHAVIOR] Cmd1 configured: CmdID=1, TimeoutMs=100ms, callback will delay 200ms\n");
    printf("📋 [BEHAVIOR] Expected: Timeout at ~100ms, link should auto-recover\n");

    ResultValue = IOC_execCMD(srvLinkID1, &cmdDesc1, NULL);

    printf("📋 [BEHAVIOR] Cmd1 returned: result=%d (expected: TIMEOUT=%d)\n", ResultValue, IOC_RESULT_TIMEOUT);
    ASSERT_EQ(IOC_RESULT_TIMEOUT, ResultValue);
    ASSERT_EQ(IOC_CMD_STATUS_TIMEOUT, IOC_CmdDesc_getStatus(&cmdDesc1));

    // ┌──────────────────────────────────────────────────────────────┐
    // │                     ✅ VERIFY PHASE                          │
    // └──────────────────────────────────────────────────────────────┘

    //@KeyVerifyPoint-1: Link substate after timeout = ExecutorReady (recovery)
    printf("✅ [VERIFY] ASSERTION 1: Link recovered to ExecutorReady after timeout\n");
    IOC_LinkState_T linkMainState = IOC_LinkStateUndefined;
    IOC_LinkSubState_T linkSubState = IOC_LinkSubStateDefault;
    ResultValue = IOC_getLinkState(client1LinkID, &linkMainState, &linkSubState);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);
    printf("    • Link main state: %d (expected: IOC_LinkStateReady=%d)\n", linkMainState, IOC_LinkStateReady);
    printf("    • Link sub state: %d (expected: IOC_LinkSubStateCmdExecutorReady=%d)\n", linkSubState,
           IOC_LinkSubStateCmdExecutorReady);
    VERIFY_KEYPOINT_EQ(linkMainState, IOC_LinkStateReady, "Link main state must be Ready after timeout");
    VERIFY_KEYPOINT_EQ(linkSubState, IOC_LinkSubStateCmdExecutorReady,
                       "Link sub state must return to ExecutorReady after timeout (auto recovery)");

    //@KeyVerifyPoint-2: Timeout isolated to command, didn't propagate to link failure
    printf("✅ [VERIFY] ASSERTION 2: Timeout isolated to command (no link failure)\n");
    printf("    • Command status: TIMEOUT (isolated to Cmd1) ✅\n");
    printf("    • Link state: Ready (NOT failed) ✅\n");
    printf("    • Architecture: Timeout doesn't cause link failure ✅\n");

    //@KeyVerifyPoint-3: Send 2nd command to verify link operational
    printf("✅ [VERIFY] ASSERTION 3: 2nd command executes successfully (link recovered)\n");

    IOC_CmdDesc_T cmdDesc2 = {};
    cmdDesc2.CmdID = 2;
    cmdDesc2.TimeoutMs = 200;      // Give enough time for 2nd command
    client1PrivData.delayMs = 50;  // Callback will take 50ms (well within timeout)

    printf("    • Executing Cmd2: CmdID=2, TimeoutMs=200ms, callback will delay 50ms\n");

    ResultValue = IOC_execCMD(srvLinkID1, &cmdDesc2, NULL);

    printf("    • Cmd2 returned: result=%d (expected: SUCCESS=%d)\n", ResultValue, IOC_RESULT_SUCCESS);
    VERIFY_KEYPOINT_EQ(ResultValue, IOC_RESULT_SUCCESS, "2nd command must succeed after link recovery");
    VERIFY_KEYPOINT_EQ(IOC_CmdDesc_getStatus(&cmdDesc2), IOC_CMD_STATUS_SUCCESS, "2nd command status must be SUCCESS");

    //@KeyVerifyPoint-4: Callback invoked twice (once per command)
    printf("✅ [VERIFY] ASSERTION 4: Callback invoked for both commands\n");
    printf("    • Callback invocations: %d (expected: 2)\n", client1PrivData.callbackInvoked);
    VERIFY_KEYPOINT_EQ(client1PrivData.callbackInvoked, 2, "Callback must be invoked twice (once per command)");

    //@KeyVerifyPoint-5: Final link state remains Ready
    printf("✅ [VERIFY] ASSERTION 5: Final link state remains Ready\n");
    ResultValue = IOC_getLinkState(client1LinkID, &linkMainState, &linkSubState);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);
    printf("    • Final link main state: %d (expected: IOC_LinkStateReady=%d)\n", linkMainState, IOC_LinkStateReady);
    printf("    • Final link sub state: %d (expected: IOC_LinkSubStateCmdExecutorReady=%d)\n", linkSubState,
           IOC_LinkSubStateCmdExecutorReady);
    VERIFY_KEYPOINT_EQ(linkMainState, IOC_LinkStateReady, "Final link main state must remain Ready");
    VERIFY_KEYPOINT_EQ(linkSubState, IOC_LinkSubStateCmdExecutorReady, "Final link sub state must be ExecutorReady");

    printf("\n");
    printf("✅ [RESULT] Link recovery after callback timeout verified:\n");
    printf("   • Cmd1 timeout occurred (TimeoutMs=100ms, callback=200ms) ✅\n");
    printf("   • Link recovered to ExecutorReady after Cmd1 timeout (ASSERTION 1) ✅\n");
    printf("   • Timeout isolated to command, no link failure (ASSERTION 2) ✅\n");
    printf("   • Cmd2 executed successfully after recovery (ASSERTION 3) ✅\n");
    printf("   • Both callbacks invoked (ASSERTION 4) ✅\n");
    printf("   • Final link state Ready (ASSERTION 5) ✅\n");
    printf("   • Architecture: Link resilience after timeout (PRINCIPLE) ✅\n");

    // Cleanup
    if (client1LinkID != IOC_ID_INVALID) IOC_closeLink(client1LinkID);
    if (srvLinkID1 != IOC_ID_INVALID) IOC_closeLink(srvLinkID1);
    if (srvID != IOC_ID_INVALID) IOC_offlineService(srvID);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF AC-2 TC-2: LINK RECOVERY AFTER POLLING TIMEOUT==================================

TEST(UT_CommandStateUS4, verifyLinkRecovery_afterPollingTimeout_expectReadyState) {
    // TODO: Implement polling timeout link recovery verification
    // Verify link state returns to Ready after IOC_waitCMD timeout

    GTEST_SKIP() << "AC-2 TC-2: Polling timeout link recovery testing pending implementation";
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF AC-3 TC-1: ERROR PROPAGATION IN CALLBACK MODE===================================

TEST(UT_CommandStateUS4, verifyErrorPropagation_byCallbackReturnError_expectFailedStatus) {
    // TODO: Implement callback return error propagation verification
    // Verify callback error code propagates to command status FAILED

    GTEST_SKIP() << "AC-3 TC-1: Callback error propagation testing pending implementation";
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF AC-3 TC-2: ERROR PROPAGATION IN POLLING MODE====================================

TEST(UT_CommandStateUS4, verifyErrorPropagation_byAckCmdWithError_expectFailedStatus) {
    // TODO: Implement IOC_ackCMD error descriptor propagation verification
    // Verify error in descriptor before ackCMD propagates to initiator

    GTEST_SKIP() << "AC-3 TC-2: Polling ackCMD error propagation testing pending implementation";
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF AC-4 TC-1: MIXED RESULTS IN CALLBACK MODE=======================================

TEST(UT_CommandStateUS4, verifyMixedResults_bySequentialCallbacks_expectIndependentStates) {
    // TODO: Implement callback mode mixed results verification
    // Verify sequential callback commands maintain independent states

    GTEST_SKIP() << "AC-4 TC-1: Callback mixed success/failure testing pending implementation";
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF AC-4 TC-2: MIXED RESULTS IN POLLING MODE========================================

TEST(UT_CommandStateUS4, verifyMixedResults_byWaitAckCycle_expectIndependentStates) {
    // TODO: Implement polling mode mixed results verification
    // Verify wait/ack cycle commands maintain independent states

    GTEST_SKIP() << "AC-4 TC-2: Polling mixed success/failure testing pending implementation";
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF AC-5 TC-1: ERROR RECOVERY IN CALLBACK MODE======================================

TEST(UT_CommandStateUS4, verifyErrorRecovery_byCallbackSuccessAfterFailure_expectStateCleanup) {
    // TODO: Implement callback mode error recovery verification
    // Verify system recovers from callback errors, subsequent callbacks succeed

    GTEST_SKIP() << "AC-5 TC-1: Callback error recovery testing pending implementation";
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF AC-5 TC-2: ERROR RECOVERY IN POLLING MODE=======================================

TEST(UT_CommandStateUS4, verifyErrorRecovery_byPollingSuccessAfterFailure_expectStateCleanup) {
    // TODO: Implement polling mode error recovery verification
    // Verify system recovers from polling errors, subsequent wait/ack cycles succeed

    GTEST_SKIP() << "AC-5 TC-2: Polling error recovery testing pending implementation";
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF IMPLEMENTATION SUMMARY===========================================================
/**
 * ╔══════════════════════════════════════════════════════════════════════════════════════════╗
 * ║                              📊 IMPLEMENTATION SUMMARY                                   ║
 * ╠══════════════════════════════════════════════════════════════════════════════════════════╣
 * ║ 🎯 PURPOSE: Command Timeout and Error State Verification - User Story 4                ║
 * ║                                                                                          ║
 * ║ 📋 FRAMEWORK STATUS: DESIGNED (1/11 tests - 9%)                                          ║
 * ║   • Error and timeout state verification framework DESIGNED for BOTH execution patterns║
 * ║   • 5 Acceptance criteria with 11 test specifications (AC-1 has 3 timeout tests)       ║
 * ║   • AC-1 TC-1 COMPLETE: Descriptor timeout (pCmdDesc->TimeoutMs) validated ✅          ║
 * ║   • AC-1 TC-2 PENDING: waitCMD API option timeout (pOption->TimeoutUS) ⚪              ║
 * ║   • AC-1 TC-3 PENDING: execCMD API option timeout (pOption->TimeoutUS) ⚪              ║
 * ║   • Test case placeholders created with comprehensive documentation                     ║
 * ║   • API discovery complete (all required enums/fields/options exist)                    ║
 * ║   • Ready for TDD implementation phase (both patterns + both timeout mechanisms)        ║
 * ║                                                                                          ║
 * ║ 🔧 DESIGN APPROACH:                                                                      ║
 * ║   • DUAL EXECUTION PATTERNS: Callback (CbExecCmd_F) + Polling (waitCMD/ackCMD)        ║
 * ║   • Dual-state error handling: command + link error state verification                 ║
 * ║   • Timeout state transitions: PENDING→PROCESSING→TIMEOUT (callback mode)              ║
 * ║   • Polling timeout: IOC_waitCMD timeout parameter enforcement                         ║
 * ║   • Error propagation: callback return vs ackCMD error descriptor                      ║
 * ║   • State recovery: link returns to Ready after error/timeout (both patterns)          ║
 * ║   • Command isolation: errors don't contaminate other commands                         ║
 * ║                                                                                          ║
 * ║ 💡 ERROR STATE INSIGHTS:                                                                ║
 * ║   • TWO TIMEOUT MECHANISMS IDENTIFIED:                                                  ║
 * ║     1. Descriptor Timeout (pCmdDesc->TimeoutMs) - Limits callback execution            ║
 * ║     2. API Option Timeout (pOption->Payload.TimeoutUS) - Limits API call blocking      ║
 * ║   • Descriptor timeout: Protocol enforces during callback thread execution             ║
 * ║   • API option timeout: Framework enforces on execCMD/waitCMD/ackCMD calls            ║
 * ║   • Both mechanisms can coexist - timeout occurs at whichever limit reached first      ║
 * ║   • IOC_CMD_STATUS_TIMEOUT (6) and IOC_CMD_STATUS_FAILED (5) exist                     ║
 * ║   • IOC_RESULT_TIMEOUT (-506) and IOC_RESULT_CMD_EXEC_FAILED (-509) available         ║
 * ║   • Protocol layer has timeout enforcement infrastructure                               ║
 * ║   • Proper error state handling prevents resource leaks                                ║
 * ║   • Different link substates: BusyExecCmd (9) vs BusyWaitCmd (10)                     ║
 * ║   • Timeout conditions require careful state cleanup                                   ║
 * ║   • Error isolation prevents failure propagation between commands                      ║
 * ║   • Recovery mechanisms ensure link availability after errors                          ║
 * ║                                                                                          ║
 * ║ 📋 IMPLEMENTATION REQUIREMENTS IDENTIFIED:                                               ║
 * ║   • AC-1: Command timeout state transition verification (THREE mechanisms)             ║
 * ║     - TC-1 ✅: Descriptor timeout (pCmdDesc->TimeoutMs=100) in callback mode          ║
 * ║     - TC-2 ⚪: API option timeout in IOC_waitCMD(pOption->TimeoutUS=100000)           ║
 * ║     - TC-3 ⚪: API option timeout in IOC_execCMD(pOption->TimeoutUS=100000)           ║
 * ║     - Verify status transitions INITIALIZED→PROCESSING→TIMEOUT (all mechanisms)       ║
 * ║     - Verify result = IOC_RESULT_TIMEOUT in all mechanisms                            ║
 * ║     - Verify timing precision ~100ms ± tolerance (all mechanisms)                     ║
 * ║                                                                                          ║
 * ║   • AC-2: Link state recovery after timeout (BOTH patterns)                            ║
 * ║     - CALLBACK: Query link state during timeout (expect BusyExecCmd-9)                ║
 * ║     - POLLING: Query link state during wait (expect BusyWaitCmd-10)                   ║
 * ║     - Query link state after timeout (expect Ready-8 - RECOVERY!)                     ║
 * ║     - Execute new command to verify link operational (both patterns)                   ║
 * ║                                                                                          ║
 * ║   • AC-3: Error state propagation verification (BOTH patterns)                         ║
 * ║     - CALLBACK: Executor callback returns IOC_RESULT_CMD_EXEC_FAILED                  ║
 * ║     - POLLING: Executor sets status=FAILED, result=-509 before IOC_ackCMD             ║
 * ║     - Verify command status = IOC_CMD_STATUS_FAILED (both patterns)                   ║
 * ║     - Verify command result = IOC_RESULT_CMD_EXEC_FAILED (both patterns)              ║
 * ║     - Verify link returns to Ready (resilience in both patterns)                      ║
 * ║                                                                                          ║
 * ║   • AC-4: Mixed success/failure independence (BOTH patterns)                           ║
 * ║     - CALLBACK: Sequential callbacks: Cmd1 success, Cmd2 failure                      ║
 * ║     - POLLING: Sequential wait/ack cycles: Cmd1 success, Cmd2 failure                 ║
 * ║     - Verify independent command descriptor states (both patterns)                     ║
 * ║     - Verify no cross-contamination (both patterns)                                    ║
 * ║                                                                                          ║
 * ║   • AC-5: Error recovery and state cleanup (BOTH patterns)                             ║
 * ║     - CALLBACK: Cmd1 callback error → verify link cleanup → Cmd2 callback success    ║
 * ║     - POLLING: Cmd1 ackCMD error → verify link cleanup → Cmd2 ackCMD success         ║
 * ║     - Verify fresh initialization in both patterns                                     ║
 * ║     - Verify no residual error state in both patterns                                  ║
 * ║                                                                                          ║
 * ║ 🎯 TDD EXPECTATIONS:                                                                     ║
 * ║   • CALLBACK MODE: Tests will likely REVEAL missing timeout enforcement in callback   ║
 * ║   • POLLING MODE: Tests will likely REVEAL missing timeout enforcement in waitCMD     ║
 * ║   • May discover gaps in error propagation for both patterns                          ║
 * ║   • Could identify missing state cleanup after error/timeout (both patterns)          ║
 * ║   • Opportunity to improve error handling robustness for both patterns                ║
 * ║   • TRUE TDD: Tests drive production code improvements for both execution patterns!   ║
 * ║                                                                                          ║
 * ║ 📊 TEST COVERAGE PLAN (EXPANDED FOR THREE TIMEOUT MECHANISMS):                          ║
 * ║   🟢 AC-1 (Timeout State):      3 tests (1✅/3 complete - 33%)                         ║
 * ║      • TC-1 ✅: Descriptor timeout (pCmdDesc->TimeoutMs) - COMPLETE                   ║
 * ║      • TC-2 ⚪: waitCMD option timeout (pOption->TimeoutUS) - TODO                    ║
 * ║      • TC-3 ⚪: execCMD option timeout (pOption->TimeoutUS) - TODO                    ║
 * ║   ⚪ AC-2 (Link Recovery):      2 tests - TC-1 Callback + TC-2 Polling                 ║
 * ║   ⚪ AC-3 (Error Propagation):  2 tests - TC-1 Callback + TC-2 Polling                 ║
 * ║   ⚪ AC-4 (Mixed Results):      2 tests - TC-1 Callback + TC-2 Polling                 ║
 * ║   ⚪ AC-5 (Error Recovery):     2 tests - TC-1 Callback + TC-2 Polling                 ║
 * ║   TOTAL: 11 tests planned (1 complete, 10 pending) - 3 timeout + 8 error tests        ║
 * ║                                                                                          ║
 * ║ 🚀 NEXT STEPS:                                                                           ║
 * ║   1. ✅ COMPLETE: AC-1 TC-1 - Descriptor timeout validated                             ║
 * ║   2. Implement AC-1 TC-2: waitCMD pOption timeout                                      ║
 * ║   3. Build → Expect COMPILATION SUCCESS (pOption APIs exist)                           ║
 * ║   4. Run → Discover if pOption timeout implemented in waitCMD                          ║
 * ║   5. Implement AC-1 TC-3: execCMD pOption timeout                                      ║
 * ║   6. Run → Discover if pOption timeout implemented in execCMD                          ║
 * ║   7. Repeat TDD cycle for remaining 8 tests (AC-2 through AC-5)                        ║
 * ╚══════════════════════════════════════════════════════════════════════════════════════════╝
 */
//======>END OF IMPLEMENTATION SUMMARY=============================================================
