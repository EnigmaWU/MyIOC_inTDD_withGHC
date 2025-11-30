///////////////////////////////////////////////////////////////////////////////////////////////////
// UT_LinkStateOperation.cxx - Link Operation State Testing (Level 2)
//
// PURPOSE:
//   Test Link Operation State (Level 2) behavior for both ConetMode and ConlesMode.
//   This file verifies the Ready/Busy states and their transitions during operations.
//
// COVERAGE STRATEGY (CaTDD Methodology):
//   - Dimension 1: Operation Type (EVT/CMD/DAT)
//   - Dimension 2: Link Mode (ConetMode/ConlesMode)
//   - Dimension 3: State Type (Ready/Busy variations)
//
// RELATIONSHIP WITH OTHER TEST FILES:
//   - UT_LinkConnState.cxx: Connection State Level 1 (FOUNDATION - COMPLETED)
//   - UT_LinkConnStateTCP.cxx: TCP-specific Level 1 (FOUNDATION - COMPLETED)
//   - UT_LinkStateCorrelation.cxx: 3-Level correlation (NEXT PHASE)
//
// REFERENCE:
//   - README_ArchDesign-State.md "Link Operation States (Level 2)"
//   - README_ArchDesign-State.md "Understanding Link State Hierarchy"
//   - Doc/UserGuide_CMD.md "Command Execution States"
//   - Doc/UserGuide_EVT.md "Event Processing States"
//
// TDD WORKFLOW:
//   Design → Draft → Structure → Test (RED) → Code (GREEN) → Refactor → Repeat
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <atomic>
#include <chrono>
#include <thread>

#include "_UT_IOC_Common.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF OVERVIEW OF THIS UNIT TESTING FILE===============================================
/**
 * @brief
 *   [WHAT] This file verifies Link Operation State (Level 2) behavior
 *   [WHERE] in the IOC Link State Management subsystem for both connection modes
 *   [WHY] to ensure operations correctly transition between Ready and Busy states
 *
 * SCOPE:
 *   - In scope:
 *     • Ready state: Link available for new operations
 *     • Busy states: BusyCbProcEvt, BusySubEvt, BusyUnsubEvt
 *     • Busy with substates: During CMD/DAT operations (Level 3 correlation)
 *     • State transitions: Ready ↔ Busy during operation lifecycle
 *     • ConetMode: Operation state with connection established
 *     • ConlesMode: Operation state with auto-managed connection
 *   - Out of scope:
 *     • Connection State Level 1 (see UT_LinkConnState.cxx)
 *     • TCP-specific behavior (see UT_LinkConnStateTCP.cxx)
 *     • 3-level correlation (see UT_LinkStateCorrelation.cxx)
 *
 * KEY CONCEPTS:
 *   - Level 2 Independence: Operation state independent of connection state
 *   - Ready State: Link idle, available for new operations
 *   - Busy State: Link actively processing an operation
 *   - State Transition: Atomic transitions during operation start/complete
 *   - Mode Awareness: Both ConetMode and ConlesMode support Level 2 states
 *
 * RELATIONSHIPS:
 *   - Depends on: IOC_Service.c (link management), IOC_Command.c, IOC_Event.c
 *   - Related tests:
 *     • UT_LinkConnState.cxx (Level 1 - connection state)
 *     • UT_CommandTypical.cxx (command execution patterns)
 *     • UT_ConlesEventState.cxx (event state in ConlesMode)
 *   - Production code: Source/IOC_Service.c (state management)
 *   - Architecture: README_ArchDesign-State.md "3-Level Link State Hierarchy"
 */
//======>END OF OVERVIEW OF THIS UNIT TESTING FILE=================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF UNIT TESTING DESIGN==============================================================

/**************************************************************************************************
 * 📋 TEST CASE DESIGN ASPECTS/CATEGORIES
 *
 * PRIORITY FRAMEWORK (from CaTDD):
 *   P1 🥇 FUNCTIONAL:     Ready/Busy state verification (Typical + Boundary)
 *   P2 🥈 DESIGN-ORIENTED: State transitions, mode comparison
 *   P3 🥉 QUALITY-ORIENTED: Concurrent operations, performance
 *
 * CONTEXT-SPECIFIC ADJUSTMENT:
 *   - Level 2 Foundation Critical → All P1 tests must pass before Level 3
 *   - Mode Parity Important → ConetMode and ConlesMode behave consistently
 *************************************************************************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF USER STORY=======================================================================
/**
 * COVERAGE STRATEGY:
 *   Dimension 1: Link Mode (ConetMode / ConlesMode)
 *   Dimension 2: Operation Type (EVT / CMD / DAT)
 *   Dimension 3: State Type (Ready / Busy variations)
 *
 * COVERAGE MATRIX:
 * ┌──────────────────────┬────────────────┬──────────────────────┬─────────────────────────────┐
 * │ Link Mode            │ Operation      │ Expected State       │ Key Scenarios               │
 * ├──────────────────────┼────────────────┼──────────────────────┼─────────────────────────────┤
 * │ ConetMode (TCP)      │ None           │ Ready                │ US-1: After connection      │
 * │ ConlesMode           │ None           │ Ready                │ US-2: After initialization  │
 * │ Both                 │ Between ops    │ Ready                │ US-3: Between operations    │
 * │ Both                 │ During CbEvt   │ BusyCbProcEvt        │ US-4: Event callback        │
 * │ Both                 │ During SubEvt  │ BusySubEvt           │ US-5: Subscribe operation   │
 * │ Both                 │ During UnsubEvt│ BusyUnsubEvt         │ US-6: Unsubscribe operation │
 * │ Both                 │ During ExecCmd │ Busy + SubState      │ US-7: Command execution     │
 * │ Both                 │ During SendDat │ Busy + SubState      │ US-8: Data transmission     │
 * │ Both                 │ During RecvDat │ Busy + SubState      │ US-9: Data reception        │
 * │ Both                 │ Op complete    │ Ready                │ US-10: Transition back      │
 * └──────────────────────┴────────────────┴──────────────────────┴─────────────────────────────┘
 *
 * USER STORIES:
 *
 *  US-1: As a ConetMode application,
 *        I want newly connected links to show Ready state,
 *        So that I know the link is available for operations.
 *
 *  US-2: As a ConlesMode application,
 *        I want auto-managed links to show Ready state after init,
 *        So that I can immediately start operations.
 *
 *  US-3: As an operation scheduler,
 *        I want links to return to Ready state between operations,
 *        So that I can detect when link is available for next operation.
 *
 *  US-4: As an event processor,
 *        I want link state to show BusyCbProcEvt during callback,
 *        So that I can prevent concurrent operations on same link.
 *
 *  US-5: As a subscription manager,
 *        I want link state to show BusySubEvt during subscribe,
 *        So that I can track subscription operation progress.
 *
 *  US-6: As a subscription manager,
 *        I want link state to show BusyUnsubEvt during unsubscribe,
 *        So that I can track unsubscription operation progress.
 *
 *  US-7: As a command executor,
 *        I want link state to show Busy with appropriate substate during execCMD,
 *        So that I can monitor command execution progress (Level 3 detail).
 *
 *  US-8: As a data sender,
 *        I want link state to show Busy with appropriate substate during sendDAT,
 *        So that I can monitor transmission progress (Level 3 detail).
 *
 *  US-9: As a data receiver,
 *        I want link state to show Busy with appropriate substate during recvDAT,
 *        So that I can monitor reception progress (Level 3 detail).
 *
 *  US-10: As a state monitor,
 *         I want atomic state transitions during operation lifecycle,
 *         So that I never observe inconsistent intermediate states.
 */
//======>END OF USER STORY=========================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//=======>BEGIN OF ACCEPTANCE CRITERIA=============================================================
/**
 * [@US-1] ConetMode Ready state after connection
 *  AC-1: GIVEN ConetMode link successfully connected via IOC_connectService(),
 *         WHEN querying IOC_getLinkState(),
 *         THEN mainState returns Ready,
 *          AND subState returns Default (no operation in progress),
 *          AND query returns IOC_RESULT_SUCCESS.
 *
 * [@US-2] ConlesMode Ready state after initialization
 *  AC-1: GIVEN ConlesMode initialized via IOC_initConlesMode(),
 *         WHEN querying IOC_getLinkState() with IOC_CONLES_MODE_AUTO_LINK_ID,
 *         THEN mainState returns Ready,
 *          AND subState returns Default,
 *          AND query returns IOC_RESULT_SUCCESS.
 *
 * [@US-3] Ready state between operations
 *  AC-1: GIVEN operation completed successfully,
 *         WHEN no new operation is in progress,
 *         THEN IOC_getLinkState() returns Ready,
 *          AND link is available for new operations,
 *          AND state remains stable (not transient).
 *
 * [@US-4] BusyCbProcEvt during event callback
 *  AC-1: GIVEN event callback is executing (IOC_CbProcEvt_F),
 *         WHEN querying IOC_getLinkState() from within callback,
 *         THEN mainState returns BusyCbProcEvt,
 *          AND subState is Default (EVT has no substates per architecture),
 *          AND demonstrates Level 2 state during event processing.
 *
 * [@US-5] BusySubEvt during subscription
 *  AC-1: GIVEN IOC_subEVT() operation is in progress,
 *         WHEN querying IOC_getLinkState(),
 *         THEN mainState returns BusySubEvt,
 *          AND subState is Default,
 *          AND state transitions back to Ready after completion.
 *
 * [@US-6] BusyUnsubEvt during unsubscription
 *  AC-1: GIVEN IOC_unsubEVT() operation is in progress,
 *         WHEN querying IOC_getLinkState(),
 *         THEN mainState returns BusyUnsubEvt,
 *          AND subState is Default,
 *          AND state transitions back to Ready after completion.
 *
 * [@US-7] Busy with substate during command execution
 *  AC-1: GIVEN IOC_execCMD() is executing,
 *         WHEN querying IOC_getLinkState(),
 *         THEN mainState returns Busy (exact value TBD based on role),
 *          AND subState indicates CMD operation progress (Level 3),
 *          AND demonstrates Level 2 + Level 3 correlation.
 *
 * [@US-8] Busy with substate during data send
 *  AC-1: GIVEN IOC_sendDAT() is transmitting,
 *         WHEN querying IOC_getLinkState(),
 *         THEN mainState returns Busy,
 *          AND subState indicates DAT transmission progress (Level 3),
 *          AND state transitions atomically.
 *
 * [@US-9] Busy with substate during data receive
 *  AC-1: GIVEN IOC_recvDAT() is receiving,
 *         WHEN querying IOC_getLinkState(),
 *         THEN mainState returns Busy,
 *          AND subState indicates DAT reception progress (Level 3),
 *          AND state reflects actual operation progress.
 *
 * [@US-10] Atomic state transitions
 *  AC-1: GIVEN concurrent state queries during operation lifecycle,
 *         WHEN operation transitions Ready → Busy → Ready,
 *         THEN queries observe only stable states (no half-transitions),
 *          AND state changes are thread-safe,
 *          AND no race conditions in state reporting.
 */
//=======>END OF ACCEPTANCE CRITERIA================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF TEST CASES=======================================================================
/**
 * TEST ORGANIZATION: By Operation State Category → Mode
 *
 * PORT ALLOCATION STRATEGY:
 *   Base: 24000-24099 (Operation State tests)
 *   UT_LinkConnState.cxx uses 23000-23099 (Connection State Level 1)
 *   UT_LinkConnStateTCP.cxx uses 23100-23199 (TCP-specific Level 1)
 *   UT_LinkStateOperation.cxx uses 24000-24099 (Operation State Level 2)
 *
 * STATUS TRACKING:
 *  ⚪ = Planned/TODO     - Designed but not implemented
 *  🔴 = Implemented/RED  - Test written and failing (need prod code)
 *  🟢 = Passed/GREEN     - Test written and passing
 *
 * NAMING CONVENTION: verifyLinkState_by[Scenario]_expect[State]_[Mode]
 *
 * ═════════════════════════════════════════════════════════════════════════════════════════════
 * 📋 [CATEGORY: CAT-1] Ready State Verification
 * ═════════════════════════════════════════════════════════════════════════════════════════════
 *
 * [@AC-1,US-1] ConetMode Ready after connection
 *  ⚪ TC-1: verifyLinkState_afterConnect_expectReady_ConetMode
 *      @[Purpose]: Validate Ready state in ConetMode after successful connection
 *      @[Brief]: Connect TCP service, query state, expect Ready+Default
 *      @[Mode]: ConetMode
 *      @[Port]: 24000
 *      @[Status]: PLANNED
 *
 * [@AC-1,US-2] ConlesMode Ready after initialization
 *  ⚪ TC-2: verifyLinkState_afterInit_expectReady_ConlesMode
 *      @[Purpose]: Validate Ready state in ConlesMode after initialization
 *      @[Brief]: Initialize ConlesMode, query auto-link state, expect Ready+Default
 *      @[Mode]: ConlesMode
 *      @[Port]: N/A (ConlesMode)
 *      @[Status]: PLANNED
 *
 * [@AC-1,US-3] Ready state between operations
 *  ⚪ TC-3: verifyLinkState_betweenOperations_expectReady
 *      @[Purpose]: Validate state returns to Ready after each operation
 *      @[Brief]: Execute operation, wait for completion, query state, expect Ready
 *      @[Mode]: Both
 *      @[Port]: 24001
 *      @[Status]: PLANNED
 *
 * ═════════════════════════════════════════════════════════════════════════════════════════════
 * 📋 [CATEGORY: CAT-2] Busy State During EVT Operations
 * ═════════════════════════════════════════════════════════════════════════════════════════════
 *
 * [@AC-1,US-4] BusyCbProcEvt during event callback
 *  ⚪ TC-4: verifyLinkState_duringCbProcEvt_expectBusyCbProcEvt
 *      @[Purpose]: Validate BusyCbProcEvt state during event callback execution
 *      @[Brief]: Post event, query state from within callback, expect BusyCbProcEvt
 *      @[Mode]: Both
 *      @[Port]: 24002
 *      @[Status]: PLANNED - Requires callback that queries own state
 *
 * [@AC-1,US-5] BusySubEvt during subscription
 *  ⚪ TC-5: verifyLinkState_duringSubEvt_expectBusySubEvt
 *      @[Purpose]: Validate BusySubEvt state during IOC_subEVT() operation
 *      @[Brief]: Call subEVT (async), query state immediately, expect BusySubEvt or Ready
 *      @[Mode]: Both
 *      @[Port]: 24003
 *      @[Status]: PLANNED - May be too fast to observe (timing-dependent)
 *
 * [@AC-1,US-6] BusyUnsubEvt during unsubscription
 *  ⚪ TC-6: verifyLinkState_duringUnsubEvt_expectBusyUnsubEvt
 *      @[Purpose]: Validate BusyUnsubEvt state during IOC_unsubEVT() operation
 *      @[Brief]: Call unsubEVT (async), query state immediately, expect BusyUnsubEvt or Ready
 *      @[Mode]: Both
 *      @[Port]: 24004
 *      @[Status]: PLANNED - May be too fast to observe (timing-dependent)
 *
 * ═════════════════════════════════════════════════════════════════════════════════════════════
 * 📋 [CATEGORY: CAT-3] Busy State During CMD/DAT Operations
 * ═════════════════════════════════════════════════════════════════════════════════════════════
 *
 * [@AC-1,US-7] Busy with substate during execCMD
 *  ⚪ TC-7: verifyLinkState_duringExecCmd_expectBusyWithSubstate
 *      @[Purpose]: Validate Busy+SubState during command execution
 *      @[Brief]: Execute CMD, query state during execution, expect Busy+CmdSubstate
 *      @[Mode]: Both
 *      @[Port]: 24005
 *      @[Status]: PLANNED - Level 2+3 correlation test
 *
 * [@AC-1,US-8] Busy with substate during sendDAT
 *  ⚪ TC-8: verifyLinkState_duringSendDat_expectBusyWithSubstate
 *      @[Purpose]: Validate Busy+SubState during data transmission
 *      @[Brief]: Send large data, query state during send, expect Busy+DatSubstate
 *      @[Mode]: Both
 *      @[Port]: 24006
 *      @[Status]: PLANNED - Requires large data to slow down transmission
 *
 * [@AC-1,US-9] Busy with substate during recvDAT
 *  ⚪ TC-9: verifyLinkState_duringRecvDat_expectBusyWithSubstate
 *      @[Purpose]: Validate Busy+SubState during data reception
 *      @[Brief]: Receive large data, query state during recv, expect Busy+DatSubstate
 *      @[Mode]: Both
 *      @[Port]: 24007
 *      @[Status]: PLANNED - Requires large data and controlled timing
 *
 * ═════════════════════════════════════════════════════════════════════════════════════════════
 * 📋 [CATEGORY: CAT-4] State Transitions
 * ═════════════════════════════════════════════════════════════════════════════════════════════
 *
 * [@AC-1,US-10] Ready to Busy transition on operation start
 *  ⚪ TC-10: verifyStateTransition_ReadyToBusy_onOperation
 *      @[Purpose]: Validate atomic transition Ready → Busy when operation starts
 *      @[Brief]: Verify state before/after operation, no intermediate states
 *      @[Mode]: Both
 *      @[Port]: 24008
 *      @[Status]: PLANNED
 *
 * [@AC-1,US-10] Busy to Ready transition after operation completion
 *  ⚪ TC-11: verifyStateTransition_BusyToReady_afterCompletion
 *      @[Purpose]: Validate atomic transition Busy → Ready when operation completes
 *      @[Brief]: Monitor state during operation completion, verify clean transition
 *      @[Mode]: Both
 *      @[Port]: 24009
 *      @[Status]: PLANNED
 *
 * [@AC-1,US-10] State transition atomicity under concurrency
 *  ⚪ TC-12: verifyStateTransition_atomicity_underConcurrency
 *      @[Purpose]: Validate thread-safe state transitions under concurrent queries
 *      @[Brief]: Multiple threads query state during transition, expect consistent results
 *      @[Mode]: Both
 *      @[Port]: 24010
 *      @[Status]: PLANNED - Stress test for thread safety
 */
//======>END OF TEST CASES=========================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
// 🔴 RED PHASE: CAT-1 Ready State Verification (P1)
///////////////////////////////////////////////////////////////////////////////////////////////////

// Helper struct to capture server-side linkID from auto-accept callback
struct _TC1_AutoAcceptContext {
    std::atomic<IOC_LinkID_T> ServerLinkID{IOC_ID_INVALID};
    std::atomic<bool> CallbackInvoked{false};
};

static void _TC1_OnAutoAccepted(IOC_SrvID_T srvID, IOC_LinkID_T newLinkID, void *pPriv) {
    auto *pCtx = static_cast<_TC1_AutoAcceptContext *>(pPriv);
    pCtx->ServerLinkID = newLinkID;
    pCtx->CallbackInvoked = true;
}

/**
 * @[TDD Phase]: 🟢 GREEN (Updated to test BOTH sides)
 * @[RGR Cycle]: 1 of 12
 * @[Test]: verifyLinkState_afterConnect_expectReady_ConetMode
 * @[Purpose]: Validate Ready state in ConetMode after TCP connection FOR BOTH CLIENT AND SERVER
 * @[Cross-Reference]: README_ArchDesign-State.md "Link Operation States (Level 2)"
 *
 * @[Expected Behavior]:
 * - After IOC_connectService() succeeds, BOTH links exist (client + server)
 * - Client link (CmdInitiator): Operation state Ready with CmdInitiatorReady substate
 * - Server link (CmdExecutor): Operation state Ready with CmdExecutorReady substate
 * - BOTH links: Connection state (Level 1) = Connected
 *
 * @[Level 2 Independence]:
 * - Operation state Ready is independent of connection state Connected
 * - Demonstrates 3-level hierarchy: Connected (L1) + Ready (L2) + Role-specific (L3)
 *
 * @[Critical Insight]: TCP connection creates TWO links with DIFFERENT roles!
 */
TEST(UT_LinkStateOperation_Ready, TC1_verifyLinkState_afterConnect_expectReady_ConetMode) {
    //===SETUP: Create TCP service with auto-accept callback===
    _TC1_AutoAcceptContext autoAcceptCtx;
    IOC_SrvID_T srvID = IOC_ID_INVALID;
    const uint16_t TEST_PORT = 24000;

    IOC_SrvArgs_T srvArgs = {0};
    IOC_Helper_initSrvArgs(&srvArgs);
    srvArgs.SrvURI.pProtocol = IOC_SRV_PROTO_TCP;
    srvArgs.SrvURI.pHost = IOC_SRV_HOST_LOCAL_PROCESS;
    srvArgs.SrvURI.Port = TEST_PORT;
    srvArgs.SrvURI.pPath = "LinkStateOp_TC1";
    srvArgs.UsageCapabilites = IOC_LinkUsageCmdExecutor;
    srvArgs.Flags = IOC_SRVFLAG_AUTO_ACCEPT;
    srvArgs.OnAutoAccepted_F = _TC1_OnAutoAccepted;  // Capture server linkID
    srvArgs.pSrvPriv = &autoAcceptCtx;

    IOC_Result_T result = IOC_onlineService(&srvID, &srvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result);

    //===BEHAVIOR: Client connects (auto-accept creates server link)===
    IOC_LinkID_T linkID = IOC_ID_INVALID;  // Client-side link
    IOC_ConnArgs_T connArgs = {0};
    IOC_Helper_initConnArgs(&connArgs);
    connArgs.SrvURI.pProtocol = IOC_SRV_PROTO_TCP;
    connArgs.SrvURI.pHost = IOC_SRV_HOST_LOCAL_PROCESS;
    connArgs.SrvURI.Port = TEST_PORT;
    connArgs.SrvURI.pPath = "LinkStateOp_TC1";
    connArgs.Usage = IOC_LinkUsageCmdInitiator;

    result = IOC_connectService(&linkID, &connArgs, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result);
    ASSERT_NE(IOC_ID_INVALID, linkID);

    // Wait for auto-accept callback to capture server-side linkID
    for (int i = 0; i < 100 && !autoAcceptCtx.CallbackInvoked.load(); i++) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    ASSERT_TRUE(autoAcceptCtx.CallbackInvoked.load()) << "Auto-accept callback should be invoked";

    IOC_LinkID_T serverLinkID = autoAcceptCtx.ServerLinkID.load();
    ASSERT_NE(IOC_ID_INVALID, serverLinkID) << "Server link should be created";
    ASSERT_NE(linkID, serverLinkID) << "Client and server links should be different";

    //===BEHAVIOR: Query operation state (Level 2) for BOTH links===
    // CLIENT SIDE (CmdInitiator)
    IOC_LinkState_T mainState = IOC_LinkStateUndefined;
    IOC_LinkSubState_T subState = IOC_LinkSubStateDefault;

    result = IOC_getLinkState(linkID, &mainState, &subState);

    //===VERIFY CLIENT SIDE: CmdInitiator link state===
    ASSERT_EQ(IOC_RESULT_SUCCESS, result) << "Client link state query should succeed";

    EXPECT_EQ(IOC_LinkStateReady, mainState) << "Client link (CmdInitiator) should be Ready after connection";

    EXPECT_EQ(IOC_LinkSubStateCmdInitiatorReady, subState) << "Client link substate should be CmdInitiatorReady";

    // Additional verification: Connection state (Level 1) should be Connected
    IOC_LinkConnState_T connState;
    result = IOC_getLinkConnState(linkID, &connState);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result);
    EXPECT_EQ(IOC_LinkConnStateConnected, connState) << "Client link Level 1 should be Connected";

    //===VERIFY SERVER SIDE: CmdExecutor link state===
    IOC_LinkState_T serverMainState = IOC_LinkStateUndefined;
    IOC_LinkSubState_T serverSubState = IOC_LinkSubStateDefault;
    result = IOC_getLinkState(serverLinkID, &serverMainState, &serverSubState);

    ASSERT_EQ(IOC_RESULT_SUCCESS, result) << "Server link state query should succeed";
    EXPECT_EQ(IOC_LinkStateReady, serverMainState) << "Server link (CmdExecutor) should be Ready after auto-accept";
    EXPECT_EQ(IOC_LinkSubStateCmdExecutorReady, serverSubState)
        << "Server link substate should be CmdExecutorReady (not CmdInitiatorReady!)";

    IOC_LinkConnState_T serverConnState;
    result = IOC_getLinkConnState(serverLinkID, &serverConnState);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result);
    EXPECT_EQ(IOC_LinkConnStateConnected, serverConnState) << "Server link Level 1 should also be Connected";

    //===CLEANUP===
    IOC_closeLink(linkID);  // Close client link
    // NOTE: Don't close serverLinkID manually! It's owned by the service.
    // IOC_offlineService will auto-close it unless IOC_SRVFLAG_KEEP_ACCEPTED_LINK is set.
    IOC_offlineService(srvID);  // This will clean up server-side link
}

/**
 * @[TDD Phase]: 🔴 RED
 * @[RGR Cycle]: 2 of 12
 * @[Test]: verifyLinkState_afterInit_expectReady_ConlesMode
 * @[Purpose]: Validate Ready state in ConlesMode after initialization
 * @[Cross-Reference]: README_ArchDesign-State.md "ConlesMode Auto-Link Management"
 *
 * @[Expected Behavior]:
 * - ConlesMode uses IOC_CONLES_MODE_AUTO_LINK_ID (no explicit connection)
 * - Auto-link should be in Ready state
 * - SubState should be Default
 *
 * @[Mode Difference]:
 * - ConlesMode: No connection state (Level 1 not applicable)
 * - Only Level 2 (Operation) and Level 3 (SubState) are relevant
 *
 * @[Note]: ConlesMode is always available - no setup needed
 */
TEST(UT_LinkStateOperation_Ready, TC2_verifyLinkState_afterInit_expectReady_ConlesMode) {
    //===SETUP: No setup needed - ConlesMode is always available===

    //===BEHAVIOR: Query operation state using auto-link ID===
    IOC_LinkState_T mainState = IOC_LinkStateUndefined;
    IOC_LinkSubState_T subState = IOC_LinkSubStateDefault;

    IOC_Result_T result = IOC_getLinkState(IOC_CONLES_MODE_AUTO_LINK_ID, &mainState, &subState);

    //===VERIFY: Operation state should be Ready===
    ASSERT_EQ(IOC_RESULT_SUCCESS, result) << "IOC_getLinkState should succeed for ConlesMode auto-link";

    EXPECT_EQ(IOC_LinkStateReady, mainState) << "ConlesMode auto-link should always be Ready for operations";

    EXPECT_EQ(IOC_LinkSubStateDefault, subState) << "When Ready in ConlesMode, substate should be Default";

    // Verify connection state query is NOT applicable in ConlesMode
    IOC_LinkConnState_T connState;
    result = IOC_getLinkConnState(IOC_CONLES_MODE_AUTO_LINK_ID, &connState);
    EXPECT_NE(IOC_RESULT_SUCCESS, result)
        << "Connection state query should NOT be valid for ConlesMode (no connection phase)";

    //===CLEANUP: No cleanup needed===
}

/**
 * @[TDD Phase]: 🔴 RED
 * @[RGR Cycle]: 3 of 12
 * @[Test]: verifyLinkState_betweenOperations_expectReady
 * @[Purpose]: Validate state returns to Ready after operation completes
 * @[Cross-Reference]: README_ArchDesign-State.md "State Transition Patterns"
 *
 * @[Expected Behavior]:
 * - Before operation: Ready
 * - During operation: Busy (with appropriate substate)
 * - After operation: Ready (state restored)
 *
 * @[State Lifecycle]:
 * - Demonstrates Ready → Busy → Ready transition
 * - Validates state cleanup after operation completion
 */
TEST(UT_LinkStateOperation_Ready, TC3_verifyLinkState_betweenOperations_expectReady) {
    //===SETUP: Create service and establish connection===
    IOC_SrvID_T srvID = IOC_ID_INVALID;
    const uint16_t TEST_PORT = 24001;

    IOC_SrvArgs_T srvArgs = {0};
    IOC_Helper_initSrvArgs(&srvArgs);
    srvArgs.SrvURI.pProtocol = IOC_SRV_PROTO_TCP;
    srvArgs.SrvURI.pHost = IOC_SRV_HOST_LOCAL_PROCESS;
    srvArgs.SrvURI.Port = TEST_PORT;
    srvArgs.SrvURI.pPath = "LinkStateOp_TC3";
    srvArgs.UsageCapabilites = IOC_LinkUsageCmdExecutor;
    srvArgs.Flags = IOC_SRVFLAG_AUTO_ACCEPT;

    IOC_Result_T result = IOC_onlineService(&srvID, &srvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result);

    IOC_LinkID_T linkID = IOC_ID_INVALID;
    IOC_ConnArgs_T connArgs = {0};
    IOC_Helper_initConnArgs(&connArgs);
    connArgs.SrvURI.pProtocol = IOC_SRV_PROTO_TCP;
    connArgs.SrvURI.pHost = IOC_SRV_HOST_LOCAL_PROCESS;
    connArgs.SrvURI.Port = TEST_PORT;
    connArgs.SrvURI.pPath = "LinkStateOp_TC3";
    connArgs.Usage = IOC_LinkUsageCmdInitiator;

    result = IOC_connectService(&linkID, &connArgs, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result);
    ASSERT_NE(IOC_ID_INVALID, linkID);

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    //===VERIFY: Initial state is Ready===
    IOC_LinkState_T stateBefore = IOC_LinkStateUndefined;
    IOC_LinkSubState_T subStateBefore = IOC_LinkSubStateDefault;
    result = IOC_getLinkState(linkID, &stateBefore, &subStateBefore);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result);
    ASSERT_EQ(IOC_LinkStateReady, stateBefore) << "Initial state should be Ready";
    ASSERT_EQ(IOC_LinkSubStateCmdInitiatorReady, subStateBefore) << "SubState should be CmdInitiatorReady for CMD link";

    //===BEHAVIOR: Execute a simple operation (command with short timeout)===
    IOC_CmdDesc_T cmdDesc = {0};
    IOC_CmdDesc_initVar(&cmdDesc);
    cmdDesc.CmdID = 1;
    cmdDesc.TimeoutMs = 100;  // Short timeout

    // Execute command (will timeout since no executor, but that's OK for state testing)
    result = IOC_execCMD(linkID, &cmdDesc, NULL);
    // Don't check result - we're testing state, not command execution success

    //===VERIFY: State returns to Ready after operation===
    IOC_LinkState_T stateAfter = IOC_LinkStateUndefined;
    IOC_LinkSubState_T subStateAfter = IOC_LinkSubStateDefault;
    result = IOC_getLinkState(linkID, &stateAfter, &subStateAfter);

    ASSERT_EQ(IOC_RESULT_SUCCESS, result);
    EXPECT_EQ(IOC_LinkStateReady, stateAfter)
        << "After operation completes (or times out), state should return to Ready";

    EXPECT_EQ(IOC_LinkSubStateCmdInitiatorReady, subStateAfter)
        << "SubState should also return to CmdInitiatorReady when Ready (CMD link)";

    //===VERIFY: Multiple operations - state consistency===
    // Execute second operation
    cmdDesc.CmdID = 2;
    result = IOC_execCMD(linkID, &cmdDesc, NULL);

    // Check state again
    IOC_LinkState_T stateAfter2 = IOC_LinkStateUndefined;
    result = IOC_getLinkState(linkID, &stateAfter2, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result);
    EXPECT_EQ(IOC_LinkStateReady, stateAfter2) << "State should be Ready between multiple operations";

    //===CLEANUP===
    IOC_closeLink(linkID);
    IOC_offlineService(srvID);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF TODO/IMPLEMENTATION TRACKING SECTION============================================
// 🔴 IMPLEMENTATION STATUS TRACKING
//
// STATUS LEGEND:
//   ⚪ TODO/PLANNED:      Designed but not implemented yet
//   🔴 RED/FAILING:       Test written, production code missing
//   🟢 GREEN/PASSED:      Test written and passing
//
// PRIORITY LEVELS:
//   P1 🥇 FUNCTIONAL:     Ready/Busy state verification (Typical)
//   P2 🥈 DESIGN:         State transitions, EVT operations
//   P3 🥉 QUALITY:        Concurrent operations, performance
//
//===================================================================================================
// P1 🥇 FUNCTIONAL TESTING – Ready State (CAT-1)
//===================================================================================================
//
//   ✅ [@AC-1,US-1] TC-1: verifyLinkState_afterConnect_expectReady_ConetMode
//        - Category: Ready State (ValidFunc)
//        - Status: GREEN - Test PASSED
//        - Result: ConetMode link shows Ready with CmdInitiatorReady substate
//
//   ✅ [@AC-1,US-2] TC-2: verifyLinkState_afterInit_expectReady_ConlesMode
//        - Category: Ready State (ValidFunc)
//        - Status: GREEN - Test PASSED
//        - Result: ConlesMode auto-link is always Ready
//
//   ✅ [@AC-1,US-3] TC-3: verifyLinkState_betweenOperations_expectReady
//        - Category: Ready State (ValidFunc)
//        - Status: GREEN - Test PASSED
//        - Result: State correctly returns to Ready after operations
//
//===================================================================================================
// P2 🥈 DESIGN-ORIENTED TESTING – Busy States (CAT-2, CAT-3)
//===================================================================================================
//
//   ⚪ [@AC-1,US-4] TC-4: verifyLinkState_duringCbProcEvt_expectBusyCbProcEvt
//        - Category: EVT Busy State
//        - Status: PLANNED
//        - Estimated effort: 40 min (complex - requires callback state query)
//
//   ⚪ [@AC-1,US-5] TC-5: verifyLinkState_duringSubEvt_expectBusySubEvt
//        - Category: EVT Busy State
//        - Status: PLANNED
//        - Estimated effort: 30 min (timing-dependent)
//
//   ⚪ [@AC-1,US-6] TC-6: verifyLinkState_duringUnsubEvt_expectBusyUnsubEvt
//        - Category: EVT Busy State
//        - Status: PLANNED
//        - Estimated effort: 30 min (timing-dependent)
//
//   ⚪ [@AC-1,US-7] TC-7: verifyLinkState_duringExecCmd_expectBusyWithSubstate
//        - Category: CMD Busy State (Level 2+3)
//        - Status: PLANNED
//        - Estimated effort: 35 min
//
//   ⚪ [@AC-1,US-8] TC-8: verifyLinkState_duringSendDat_expectBusyWithSubstate
//        - Category: DAT Busy State (Level 2+3)
//        - Status: PLANNED
//        - Estimated effort: 40 min (requires large data)
//
//   ⚪ [@AC-1,US-9] TC-9: verifyLinkState_duringRecvDat_expectBusyWithSubstate
//        - Category: DAT Busy State (Level 2+3)
//        - Status: PLANNED
//        - Estimated effort: 40 min (requires large data)
//
//===================================================================================================
// P2 🥈 DESIGN-ORIENTED TESTING – State Transitions (CAT-4)
//===================================================================================================
//
//   ⚪ [@AC-1,US-10] TC-10: verifyStateTransition_ReadyToBusy_onOperation
//        - Category: State Transition
//        - Status: PLANNED
//        - Estimated effort: 30 min
//
//   ⚪ [@AC-1,US-10] TC-11: verifyStateTransition_BusyToReady_afterCompletion
//        - Category: State Transition
//        - Status: PLANNED
//        - Estimated effort: 30 min
//
//   ⚪ [@AC-1,US-10] TC-12: verifyStateTransition_atomicity_underConcurrency
//        - Category: State Transition (thread safety)
//        - Status: PLANNED
//        - Estimated effort: 50 min (complex - concurrent testing)
//
// 🚪 GATE P1: First 3 tests (Ready state) must be GREEN before proceeding to P2
//
// 📊 Progress Summary:
//   P1: 3/3 tests implemented (100%) ✅ GREEN - ALL PASSING
//   P2: 0/9 tests implemented (0%) - Busy states and transitions
//   Total: 3/12 tests implemented (25%)
//
// ✅ GREEN Phase Status:
//   ✅ TC-1: verifyLinkState_afterConnect_expectReady_ConetMode [PASSED]
//   ✅ TC-2: verifyLinkState_afterInit_expectReady_ConlesMode [PASSED]
//   ✅ TC-3: verifyLinkState_betweenOperations_expectReady [PASSED]
//
// 📝 Implementation Notes:
//   - P1 gate cleared: All Ready state tests passing
//   - Key learning: SubState reflects role (CmdInitiatorReady vs Default)
//   - ConlesMode auto-link always available (no setup needed)
//   - Operation state independent of connection state (as designed)
//   - Ready to proceed to P2: Busy states (EVT/CMD/DAT)
//
// 🎯 Next Steps:
//   1. Implement TC-4 (BusyCbProcEvt during event callback)
//   2. Implement TC-5 (BusySubEvt during subscription)
//   3. Implement TC-6 (BusyUnsubEvt during unsubscription)
//   4. Continue with CMD/DAT Busy states (TC-7, TC-8, TC-9)
//   5. Complete State Transition tests (TC-10, TC-11, TC-12)
//
///////////////////////////////////////////////////////////////////////////////////////////////////
//======>END OF TODO/IMPLEMENTATION TRACKING SECTION===============================================

// END OF FILE
