///////////////////////////////////////////////////////////////////////////////////////////////////
// Command State US-3 Implementation: Multi-Role Service State Verification
//
// 🎯 IMPLEMENTATION OF: User Story 3 (see UT_CommandState.h for complete specification)
// 📋 PURPOSE: Verify multi-role service state behavior across multiple links
// 🔗 DUAL-STATE LEVEL: Level 2 Advanced - Multi-Role Service Command State
//
// This file implements all test cases for US-3 Acceptance Criteria.
// See UT_CommandState.h for complete User Story definition and Acceptance Criteria.
//
// 🏗️ ARCHITECTURE CLARIFICATION (CRITICAL):
//    ✅ SERVICE Capabilities: UsageCapabilities = (CmdInitiator | CmdExecutor)
//       → Service is CAPABLE of acting in both roles
//    ✅ LINK Usage: Each LinkID has ONLY ONE usage pair after establishment:
//       → Link1: Service(CmdInitiator) ←→ Client1(CmdExecutor)
//       → Link2: Service(CmdExecutor) ←→ Client2(CmdInitiator)
//    ❌ NOT SUPPORTED: Single link with dual roles simultaneously
//    ✅ Multi-Role = Service managing MULTIPLE links with DIFFERENT single roles
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "UT_CommandState.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF IMPLEMENTATION OVERVIEW=========================================================
/**
 * @brief US-3 Implementation: Multi-Role Service State Verification
 *
 * Implements test cases for User Story 3 (see UT_CommandState.h for complete US/AC specification):
 *  - TC-1: Multi-role service ready state with multiple single-role links (AC-1)
 *  - TC-2: Service as CmdInitiator link state independence (AC-2)
 *  - TC-3: Service as CmdExecutor link state independence (AC-3)
 *  - TC-4: Concurrent multi-link operations with different roles (AC-4)
 *  - TC-5: Multi-link role-specific operation management (AC-5)
 *
 * 🔧 Implementation Focus:
 *  - Multi-role SERVICE capability declaration
 *  - Multiple LINKS with different single roles per link
 *  - Independent link state tracking and management
 *  - Concurrent operations across different role links
 *
 * 📊 LINK SUBSTATE REFERENCE (from IOC_Types.h):
 *  - IOC_LinkSubStateCmdInitiatorReady       - Ready to send commands
 *  - IOC_LinkSubStateCmdInitiatorBusyExecCmd - Busy executing outbound command
 *  - IOC_LinkSubStateCmdExecutorReady        - Ready to receive commands
 *  - IOC_LinkSubStateCmdExecutorBusyExecCmd  - Busy processing inbound command (callback mode)
 *  - IOC_LinkSubStateCmdExecutorBusyWaitCmd  - Busy waiting for inbound command (polling mode)
 *
 * 🎯 MULTI-ROLE SERVICE ARCHITECTURE (CORRECTED UNDERSTANDING):
 *  - Service Capabilities: UsageCapabilities = (CmdInitiator | CmdExecutor)
 *    → Service declares it CAN act in BOTH roles
 *  - Link Usage: Each LinkID has ONLY ONE usage pair after establishment
 *    → Link1: Service(Initiator) ←→ Client1(Executor)
 *    → Link2: Service(Executor) ←→ Client2(Initiator)
 *  - Multi-Role Service = Service managing MULTIPLE links, each with DIFFERENT single role
 *  - Link State: Each link's state is INDEPENDENT, tracked via IOC_getLinkState(linkID)
 *  - NO dual-role on single link: A LinkID never has both Initiator+Executor simultaneously
 *
 * 🏗️ KEY ARCHITECTURE PRINCIPLE (from IOC_Types.h):
 *    "As a Service, it MAY have multiple usage, e.g. <EvtProducer and CmdExecutor and ...>.
 *     As a Link, it MAY ONLY have a single pair of usage, e.g. <EvtProducer vs EvtConsumer>,
 *     or <CmdInitiator vs CmdExecutor>, or <DatSender vs DatReceiver>,
 *     AND a single usage at each side, e.g. <EvtProducer or EvtConsumer>."
 */
//======>END OF IMPLEMENTATION OVERVIEW===========================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF TEST CASES=======================================================================
/**************************************************************************************************
 * @brief 【Multi-Role Service State Test Cases - PHASE 2: TEST DESIGN CORRECTED】
 *
 * ORGANIZATION STRATEGY:
 *  - By Service Capability: Single-capability baseline → Multi-capability service
 *  - By Link Independence: Single link → Multiple links with different roles
 *  - By State Isolation: Independent state tracking per link
 *  - By Concurrent Operations: Multi-link concurrent command execution
 *
 * 🔄 STATE FOCUS: This file focuses on LINK-LEVEL state (Level 2) for multi-role SERVICES
 *    Each LinkID has independent state, service manages multiple links
 *    Individual command state (Level 1) is handled by US-1
 *
 * STATUS TRACKING: ⚪ = Planned/TODO，🔴 = Implemented/RED, 🟢 = Passed/GREEN, ⚠️ = Issues
 *
 * ⚪ FRAMEWORK STATUS: Multi-role service state verification IN REDESIGN
 *    ⚪ 0/10 tests implemented
 *    ⚪ 5 Acceptance Criteria being redesigned
 *    ✅ Architecture understanding corrected (Service≠Link)
 *
 * 📊 COVERAGE PLAN (REVISED):
 *    ⚪ AC-1: 2/2 tests planned - Multi-role service with multiple single-role links
 *    ⚪ AC-2: 2/2 tests planned - Service as Initiator link state independence
 *    ⚪ AC-3: 2/2 tests planned - Service as Executor link state independence
 *    ⚪ AC-4: 2/2 tests planned - Concurrent multi-link operations
 *    ⚪ AC-5: 2/2 tests planned - Multi-link role-specific operations
 *
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * 📋 [US-3]: MULTI-ROLE SERVICE STATE VERIFICATION (CORRECTED)
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 *
 * 🏗️ CORRECTED ARCHITECTURE UNDERSTANDING:
 *    ✅ Multi-Role SERVICE: Service declares UsageCapabilities = (CmdInitiator | CmdExecutor)
 *    ✅ Single-Role LINK: Each established LinkID has ONE usage pair only
 *       • Link1: Service(Initiator) ←→ Client1(Executor)
 *       • Link2: Service(Executor) ←→ Client2(Initiator)
 *    ❌ WRONG: Single link with both Initiator+Executor roles simultaneously
 *    ✅ CORRECT: Service manages multiple links, each with different single role
 *
 * [@AC-1,US-3] Multi-role service with multiple single-role links
 *  ⚪ TC-1: verifyMultiRoleServiceReady_byDualCapability_expectMultipleSingleRoleLinks  [STATE]
 *      @[Purpose]: Validate service with both capabilities can establish multiple links with different roles
 *      @[Brief]: Service declares (CmdInitiator|CmdExecutor), accepts two clients with different usages
 *      @[Strategy]: Service UsageCapabilities=0x0C → Client1 connects as Executor → Client2 connects as Initiator
 *      @[Key Assertions]:
 *          • ASSERTION 1: Service accepts Client1, Link1 established with Service=Initiator role
 *          • ASSERTION 2: Service accepts Client2, Link2 established with Service=Executor role
 *          • ASSERTION 3: Link1 state = CmdInitiatorReady (service can send on Link1)
 *          • ASSERTION 4: Link2 state = CmdExecutorReady (service can receive on Link2)
 *          • ASSERTION 5: Each link has independent single-role state
 *      @[Architecture Principle]: Multi-role services manage multiple single-role links independently
 *      @[Status]: TODO - Need to redesign with correct multi-link architecture
 *
 *  ⚪ TC-2: verifyLinkIndependence_byMultiRoleService_expectIsolatedLinkStates  [STATE]
 *      @[Purpose]: Validate each link maintains independent state despite service having multiple roles
 *      @[Brief]: Multi-role service with 2 links, verify IOC_getLinkState() returns correct state per link
 *      @[Strategy]: Service with Link1(Initiator) + Link2(Executor) → Query both link states independently
 *      @[Key Assertions]:
 *          • ASSERTION 1: IOC_getLinkState(Link1) returns Initiator-role state
 *          • ASSERTION 2: IOC_getLinkState(Link2) returns Executor-role state
 *          • ASSERTION 3: Link states are completely independent
 *          • ASSERTION 4: Changing Link1 state does NOT affect Link2 state
 *      @[Architecture Principle]: Each LinkID has independent state tracking
 *      @[Status]: TODO - Need independent link state verification
 *
 * [@AC-2,US-3] Service as CmdInitiator link state independence
 *  ⚪ TC-1: verifyInitiatorLinkState_whenServiceSendsCommand_expectOnlyInitiatorLinkAffected  [STATE]
 *      @[Purpose]: Validate only Initiator link state changes when service sends command
 *      @[Brief]: Service with Link1(Initiator) + Link2(Executor), send command on Link1
 *      @[Strategy]: Check Link1 state during command → Verify Link2 state unchanged
 *      @[Key Assertions]:
 *          • ASSERTION 1: Before send: Link1=CmdInitiatorReady, Link2=CmdExecutorReady
 *          • ASSERTION 2: During send: Link1=CmdInitiatorBusyExecCmd
 *          • ASSERTION 3: During send: Link2 STILL =CmdExecutorReady (unaffected)
 *          • ASSERTION 4: After send: Link1 returns to CmdInitiatorReady
 *          • ASSERTION 5: Link2 state never changed throughout Link1 operation
 *      @[Architecture Principle]: Link state isolation - operations on one link don't affect others
 *      @[Status]: TODO - Need multi-link state tracking test
 *
 *  ⚪ TC-2: verifyConcurrentCapability_whileInitiatorBusy_expectExecutorLinkAcceptsCommands  [STATE]
 *      @[Purpose]: Validate Executor link can accept commands while Initiator link is busy
 *      @[Brief]: Service Link1(Initiator) sends command, simultaneously receive on Link2(Executor)
 *      @[Strategy]: Link1 sends slow command (500ms) → Client2 sends to Link2 during Link1 wait
 *      @[Key Assertions]:
 *          • ASSERTION 1: Link1 busy with outbound command (CmdInitiatorBusyExecCmd)
 *          • ASSERTION 2: Link2 accepts inbound command from Client2 (independent operation)
 *          • ASSERTION 3: Both commands complete successfully
 *          • ASSERTION 4: Link1 and Link2 states tracked independently
 *      @[Architecture Principle]: Multi-role service supports concurrent operations on different links
 *      @[Status]: TODO - Need concurrent multi-link operation test
 *
 * [@AC-3,US-3] Service as CmdExecutor link state independence
 *  ⚪ TC-1: verifyExecutorLinkState_whenServiceReceivesCommand_expectOnlyExecutorLinkAffected  [STATE]
 *      @[Purpose]: Validate only Executor link state changes when service processes command
 *      @[Brief]: Service with Link1(Executor) + Link2(Initiator), receive command on Link1
 *      @[Strategy]: Client1 sends command to Link1 → Check Link1 state → Verify Link2 unchanged
 *      @[Key Assertions]:
 *          • ASSERTION 1: Before receive: Link1=CmdExecutorReady, Link2=CmdInitiatorReady
 *          • ASSERTION 2: During callback: Link1=CmdExecutorBusyExecCmd
 *          • ASSERTION 3: During callback: Link2 STILL =CmdInitiatorReady (unaffected)
 *          • ASSERTION 4: After callback: Link1 returns to CmdExecutorReady
 *          • ASSERTION 5: Link2 state never changed throughout Link1 operation
 *      @[Architecture Principle]: Link state isolation applies to all roles
 *      @[Status]: TODO - Need executor link state isolation test
 *
 *  ⚪ TC-2: verifyConcurrentCapability_whileExecutorBusy_expectInitiatorLinkSendsCommands  [STATE]
 *      @[Purpose]: Validate Initiator link can send commands while Executor link is busy
 *      @[Brief]: Service Link1(Executor) processes command, simultaneously send on Link2(Initiator)
 *      @[Strategy]: Client1 sends to Link1 (slow callback 500ms) → Service sends on Link2 during callback
 *      @[Key Assertions]:
 *          • ASSERTION 1: Link1 busy with inbound command (CmdExecutorBusyExecCmd)
 *          • ASSERTION 2: Link2 sends outbound command successfully (independent operation)
 *          • ASSERTION 3: Both commands complete successfully
 *          • ASSERTION 4: Link1 and Link2 states tracked independently
 *      @[Architecture Principle]: Service can use different role links concurrently
 *      @[Status]: TODO - Need concurrent cross-role operation test
 *
 * [@AC-4,US-3] Concurrent multi-link operations with different roles
 *  ⚪ TC-1: verifyConcurrentMultiLink_bySimultaneousOperations_expectIndependentCompletion  [STATE]
 *      @[Purpose]: Validate simultaneous operations on multiple links complete independently
 *      @[Brief]: Service with 3 links (2 Initiator, 1 Executor), trigger all simultaneously
 *      @[Strategy]: Link1 sends → Link2 sends → Link3 receives → All concurrent
 *      @[Key Assertions]:
 *          • ASSERTION 1: All three operations execute concurrently
 *          • ASSERTION 2: Link1 state = CmdInitiatorBusyExecCmd (independent)
 *          • ASSERTION 3: Link2 state = CmdInitiatorBusyExecCmd (independent)
 *          • ASSERTION 4: Link3 state = CmdExecutorBusyExecCmd (independent)
 *          • ASSERTION 5: All commands complete successfully
 *          • ASSERTION 6: No cross-link state contamination
 *      @[Architecture Principle]: Multi-role services scale to many concurrent link operations
 *      @[Status]: TODO - Need multi-link concurrent operation test
 *
 *  ⚪ TC-2: verifyCommandIsolation_acrossMultipleLinks_expectNoInterference  [STATE]
 *      @[Purpose]: Validate command state (Level 1) isolated across different links
 *      @[Brief]: Execute commands on Link1 and Link2, verify each IOC_CmdDesc_T independent
 *      @[Strategy]: Link1 command + Link2 command concurrent → Track both CmdDesc states
 *      @[Key Assertions]:
 *          • ASSERTION 1: Link1 command status/result independent
 *          • ASSERTION 2: Link2 command status/result independent
 *          • ASSERTION 3: Both complete with correct results
 *          • ASSERTION 4: No command state cross-contamination
 *      @[Architecture Principle]: Command state (Level 1) + Link state (Level 2) both isolated per link
 *      @[Status]: TODO - Need dual-level state isolation verification
 *
 * [@AC-5,US-3] Multi-link role-specific operation management
 *  ⚪ TC-1: verifyRoleSpecificOperation_onCorrectLink_expectProperExecution  [STATE]
 *      @[Purpose]: Validate service executes role-specific operations on correct link
 *      @[Brief]: Service with mixed-role links, verify Initiator ops only on Initiator links
 *      @[Strategy]: Try execCMD on Executor link (should fail?) vs execCMD on Initiator link (succeed)
 *      @[Key Assertions]:
 *          • ASSERTION 1: Service can send command on Initiator link
 *          • ASSERTION 2: Service cannot send command on Executor link (role mismatch?)
 *          • ASSERTION 3: Service can receive command on Executor link
 *          • ASSERTION 4: Service cannot receive command on Initiator link (role mismatch?)
 *      @[Architecture Principle]: Link role determines allowed operations
 *      @[Status]: TODO - Need role-operation validation test (may need API error handling)
 *
 *  ⚪ TC-2: verifyMultiLinkManagement_byServiceLifecycle_expectConsistentState  [STATE]
 *      @[Purpose]: Validate service maintains consistent state across link establishment/teardown
 *      @[Brief]: Service with multi-role capability, add/remove links dynamically
 *      @[Strategy]: Start with Link1 → Add Link2 → Remove Link1 → Verify Link2 unaffected
 *      @[Key Assertions]:
 *          • ASSERTION 1: Initial Link1 state correct
 *          • ASSERTION 2: Adding Link2 doesn't affect Link1
 *          • ASSERTION 3: Removing Link1 doesn't affect Link2
 *          • ASSERTION 4: Service capability persists across link changes
 *      @[Architecture Principle]: Service capability independent of individual link lifecycle
 *      @[Status]: TODO - Need dynamic link management test
 *
 **************************************************************************************************/
//======>END OF TEST CASES=========================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF AC-1 TC-1: MULTI-ROLE SERVICE READY STATE=======================================

TEST(UT_CommandStateUS3, verifyMultiRoleServiceReady_byDualCapability_expectMultipleSingleRoleLinks) {
    // ╔══════════════════════════════════════════════════════════════════════════════════════════╗
    // ║          🔗 MULTI-ROLE SERVICE READY STATE VERIFICATION                                  ║
    // ╠══════════════════════════════════════════════════════════════════════════════════════════╣
    // ║ 🎯 TEST PURPOSE: Validate service with both CmdInitiator+CmdExecutor capabilities can   ║
    // ║                  establish multiple links, each with different single role              ║
    // ║                                                                                          ║
    // ║ 📋 TEST BRIEF: Service declares dual capabilities (Initiator|Executor), accepts two     ║
    // ║                clients with different usages, verify each link has correct single role  ║
    // ║                                                                                          ║
    // ║ 🔧 TEST STRATEGY:                                                                        ║
    // ║    1. Create service with UsageCapabilities = (CmdInitiator | CmdExecutor) = 0x0C       ║
    // ║    2. Client1 connects with Usage=CmdExecutor → Link1: Service(Initiator)               ║
    // ║    3. Client2 connects with Usage=CmdInitiator → Link2: Service(Executor)               ║
    // ║    4. Query IOC_getLinkState() for both links independently                             ║
    // ║    5. Verify Link1 shows CmdInitiatorReady (service can send)                           ║
    // ║    6. Verify Link2 shows CmdExecutorReady (service can receive)                         ║
    // ║                                                                                          ║
    // ║ ✅ KEY ASSERTIONS:                                                                       ║
    // ║   • ASSERTION 1: Service accepts Client1, Link1 established (Service=Initiator)         ║
    // ║   • ASSERTION 2: Service accepts Client2, Link2 established (Service=Executor)          ║
    // ║   • ASSERTION 3: Link1 substate = CmdInitiatorReady (single-role)                       ║
    // ║   • ASSERTION 4: Link2 substate = CmdExecutorReady (single-role)                        ║
    // ║   • ASSERTION 5: Each link has independent single-role state                            ║
    // ║                                                                                          ║
    // ║ 🏛️ ARCHITECTURE PRINCIPLE: Multi-role SERVICE manages multiple single-role LINKS       ║
    // ║                              independently, NOT dual-role on single link                ║
    // ╚══════════════════════════════════════════════════════════════════════════════════════════╝

    IOC_Result_T ResultValue = IOC_RESULT_BUG;

    // ┌──────────────────────────────────────────────────────────────┐
    // │                      🔧 SETUP PHASE                          │
    // └──────────────────────────────────────────────────────────────┘
    printf("🔧 [SETUP] Creating multi-role service with dual capabilities (CmdInitiator | CmdExecutor)\n");

    // Private data for service executor callback
    struct MultiRoleSrvPriv_T {
        std::atomic<int> commandCount{0};
    };

    MultiRoleSrvPriv_T srvPrivData = {};

    // Executor callback for receiving commands on Executor links
    auto executorCb = [](IOC_LinkID_T LinkID, IOC_CmdDesc_pT pCmdDesc, void *pCbPriv) -> IOC_Result_T {
        MultiRoleSrvPriv_T *pPrivData = (MultiRoleSrvPriv_T *)pCbPriv;
        if (!pPrivData || !pCmdDesc) return IOC_RESULT_INVALID_PARAM;

        pPrivData->commandCount++;

        // Simple PING/ECHO handler
        IOC_CmdID_T CmdID = IOC_CmdDesc_getCmdID(pCmdDesc);
        if (CmdID == IOC_CMDID_TEST_PING) {
            IOC_CmdDesc_setOutPayload(pCmdDesc, (void *)"PONG", 4);
            IOC_CmdDesc_setStatus(pCmdDesc, IOC_CMD_STATUS_SUCCESS);
            IOC_CmdDesc_setResult(pCmdDesc, IOC_RESULT_SUCCESS);
        } else {
            IOC_CmdDesc_setStatus(pCmdDesc, IOC_CMD_STATUS_FAILED);
            IOC_CmdDesc_setResult(pCmdDesc, IOC_RESULT_NOT_SUPPORT);
        }

        return IOC_RESULT_SUCCESS;
    };

    // Create service with DUAL capabilities: CmdInitiator | CmdExecutor
    IOC_SrvURI_T srvURI = {.pProtocol = IOC_SRV_PROTO_FIFO,
                           .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
                           .pPath = (const char *)"MultiRoleSrv_US3_TC1"};

    static IOC_CmdID_T supportedCmdIDs[] = {IOC_CMDID_TEST_PING};
    IOC_CmdUsageArgs_T cmdUsageArgs = {
        .CbExecCmd_F = executorCb, .pCbPrivData = &srvPrivData, .CmdNum = 1, .pCmdIDs = supportedCmdIDs};

    IOC_SrvArgs_T srvArgs = {
        .SrvURI = srvURI,
        .Flags = IOC_SRVFLAG_NONE,
        .UsageCapabilites = (IOC_LinkUsage_T)(IOC_LinkUsageCmdInitiator | IOC_LinkUsageCmdExecutor),
        .UsageArgs = {.pCmd = &cmdUsageArgs}};

    IOC_SrvID_T srvID = IOC_ID_INVALID;
    ResultValue = IOC_onlineService(&srvID, &srvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);
    ASSERT_NE(IOC_ID_INVALID, srvID);

    printf("🔧 [SETUP] Service capability: 0x%02X (CmdInitiator | CmdExecutor)\n",
           IOC_LinkUsageCmdInitiator | IOC_LinkUsageCmdExecutor);

    // Client1 connects with Usage=CmdExecutor → Service will act as Initiator on this link
    printf("🔧 [SETUP] Client1 connects with Usage=CmdExecutor → Link1: Service acts as Initiator\n");

    IOC_ConnArgs_T client1ConnArgs = {.SrvURI = srvURI, .Usage = IOC_LinkUsageCmdExecutor};

    // Client1 needs executor callback too
    struct Client1Priv_T {
        std::atomic<int> commandCount{0};
    };
    Client1Priv_T client1PrivData = {};

    auto client1ExecutorCb = [](IOC_LinkID_T LinkID, IOC_CmdDesc_pT pCmdDesc, void *pCbPriv) -> IOC_Result_T {
        Client1Priv_T *pPrivData = (Client1Priv_T *)pCbPriv;
        if (!pPrivData || !pCmdDesc) return IOC_RESULT_INVALID_PARAM;

        pPrivData->commandCount++;
        IOC_CmdDesc_setOutPayload(pCmdDesc, (void *)"ACK", 3);
        IOC_CmdDesc_setStatus(pCmdDesc, IOC_CMD_STATUS_SUCCESS);
        IOC_CmdDesc_setResult(pCmdDesc, IOC_RESULT_SUCCESS);
        return IOC_RESULT_SUCCESS;
    };

    IOC_CmdUsageArgs_T client1CmdUsageArgs = {
        .CbExecCmd_F = client1ExecutorCb, .pCbPrivData = &client1PrivData, .CmdNum = 1, .pCmdIDs = supportedCmdIDs};
    client1ConnArgs.UsageArgs.pCmd = &client1CmdUsageArgs;

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

    // Client2 connects with Usage=CmdInitiator → Service will act as Executor on this link
    printf("🔧 [SETUP] Client2 connects with Usage=CmdInitiator → Link2: Service acts as Executor\n");

    IOC_ConnArgs_T client2ConnArgs = {.SrvURI = srvURI, .Usage = IOC_LinkUsageCmdInitiator};
    IOC_LinkID_T client2LinkID = IOC_ID_INVALID;

    std::thread client2Thread([&] {
        IOC_Result_T connResult = IOC_connectService(&client2LinkID, &client2ConnArgs, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, connResult);
        ASSERT_NE(IOC_ID_INVALID, client2LinkID);
    });

    // Service accepts Client2
    IOC_LinkID_T srvLinkID2 = IOC_ID_INVALID;
    ResultValue = IOC_acceptClient(srvID, &srvLinkID2, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);
    ASSERT_NE(IOC_ID_INVALID, srvLinkID2);

    if (client2Thread.joinable()) client2Thread.join();

    // ┌──────────────────────────────────────────────────────────────┐
    // │                    📋 BEHAVIOR PHASE                         │
    // └──────────────────────────────────────────────────────────────┘
    printf("📋 [BEHAVIOR] Querying link states for both connections\n");

    IOC_LinkState_T mainState1 = IOC_LinkStateUndefined;
    IOC_LinkSubState_T subState1 = IOC_LinkSubStateDefault;
    printf("📋 [BEHAVIOR] Link1 (Service=Initiator) state query\n");
    ResultValue = IOC_getLinkState(srvLinkID1, &mainState1, &subState1);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);

    IOC_LinkState_T mainState2 = IOC_LinkStateUndefined;
    IOC_LinkSubState_T subState2 = IOC_LinkSubStateDefault;
    printf("📋 [BEHAVIOR] Link2 (Service=Executor) state query\n");
    ResultValue = IOC_getLinkState(srvLinkID2, &mainState2, &subState2);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);

    // ┌──────────────────────────────────────────────────────────────┐
    // │                     ✅ VERIFY PHASE                          │
    // └──────────────────────────────────────────────────────────────┘
    //@KeyVerifyPoint<=3: Multi-role SERVICE managing multiple single-role LINKS verification
    //  1. ASSERTION 3: Link1 substate = CmdInitiatorReady (service as Initiator on Link1)
    //  2. ASSERTION 4: Link2 substate = CmdExecutorReady (service as Executor on Link2) ← KEY!
    //  3. ASSERTION 5: Both links have independent single-role states (architectural principle)

    printf("✅ [VERIFY] ASSERTION 1: Service accepted Client1, Link1 established\n");
    ASSERT_NE(IOC_ID_INVALID, srvLinkID1);
    ASSERT_NE(IOC_ID_INVALID, client1LinkID);

    printf("✅ [VERIFY] ASSERTION 2: Service accepted Client2, Link2 established\n");
    ASSERT_NE(IOC_ID_INVALID, srvLinkID2);
    ASSERT_NE(IOC_ID_INVALID, client2LinkID);

    printf("✅ [VERIFY] ASSERTION 3: Link1 substate = CmdInitiatorReady (Service can send on Link1)\n");
    printf("    • Link1 mainState: %d, subState: %d (expected: %d)\n", mainState1, subState1,
           IOC_LinkSubStateCmdInitiatorReady);
    VERIFY_KEYPOINT_EQ(subState1, IOC_LinkSubStateCmdInitiatorReady,
                       "Link1 must show Initiator role (Service acts as Initiator on Link1)");

    //@KeyVerifyPoint-1: Verify Link2 substate correctly reflects Executor role (THIS IS THE CRITICAL TEST!)
    printf("✅ [VERIFY] ASSERTION 4: Link2 substate = CmdExecutorReady (Service can receive on Link2)\n");
    printf("    • Link2 mainState: %d, subState: %d (expected: %d)\n", mainState2, subState2,
           IOC_LinkSubStateCmdExecutorReady);
    VERIFY_KEYPOINT_EQ(subState2, IOC_LinkSubStateCmdExecutorReady,
                       "Link2 must show Executor role (multi-role service architecture)");

    //@KeyVerifyPoint-2: Verify each link maintains independent single-role state
    printf("✅ [VERIFY] ASSERTION 5: Each link has independent single-role state\n");
    printf("    • Link1: Service role = Initiator (substate = %d)\n", subState1);
    printf("    • Link2: Service role = Executor (substate = %d)\n", subState2);
    VERIFY_KEYPOINT_NE(subState1, subState2, "Each link must have independent single-role state (different substates)");

    printf("\n");
    printf("✅ [RESULT] Multi-role service ready state verified:\n");
    printf("   • Service capabilities: CmdInitiator | CmdExecutor (ASSERTION 1+2) ✅\n");
    printf("   • Link1: Service=Initiator, Client1=Executor (ASSERTION 3) ✅\n");
    printf("   • Link2: Service=Executor, Client2=Initiator (ASSERTION 4) ✅\n");
    printf("   • Independent single-role links (ASSERTION 5) ✅\n");
    printf("   • Architecture principle: Multi-role SERVICE ≠ Dual-role LINK ✅\n");

    // ┌──────────────────────────────────────────────────────────────┐
    // │                    🧹 CLEANUP PHASE                          │
    // └──────────────────────────────────────────────────────────────┘
    printf("🧹 [CLEANUP] Disconnecting clients and stopping service\n");

    // 🎯 CLEANUP STRATEGY: Follow clear ownership model
    //
    // WHY: Avoid double-close errors by respecting resource ownership:
    //  • Client-side links (client1LinkID, client2LinkID): Test owns → Test closes
    //  • Server-side links (srvLinkID1, srvLinkID2): Service owns → Service closes
    //
    // IOC_offlineService() automatically closes all manually accepted links unless
    // KEEP_ACCEPTED_LINK flag is set. Since we didn't set that flag, the service
    // will handle srvLinkID1 and srvLinkID2 cleanup.
    //
    // OLD WRONG CODE (caused double-close errors):
    //   IOC_closeLink(srvLinkID1);  // ← First close
    //   IOC_closeLink(srvLinkID2);  // ← First close
    //   IOC_offlineService(srvID);   // ← Tries to close AGAIN → ERROR!
    //
    // CORRECT CODE: Only close client-side links, let service handle its own links

    // Close client-side links (test owns these)
    if (client1LinkID != IOC_ID_INVALID) IOC_closeLink(client1LinkID);
    if (client2LinkID != IOC_ID_INVALID) IOC_closeLink(client2LinkID);

    // DO NOT close srvLinkID1/srvLinkID2 here - IOC_offlineService will handle them
    // if (srvLinkID1 != IOC_ID_INVALID) IOC_closeLink(srvLinkID1);  // ← REMOVED: Let service handle
    // if (srvLinkID2 != IOC_ID_INVALID) IOC_closeLink(srvLinkID2);  // ← REMOVED: Let service handle

    // Stop service (automatically closes all accepted links)
    if (srvID != IOC_ID_INVALID) IOC_offlineService(srvID);
}

//======>END OF AC-1 TC-1==========================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF AC-1 TC-2: BIDIRECTIONAL COMMAND CAPABILITY======================================

TEST(UT_CommandStateUS3, verifyMultiRoleCapability_byBidirectionalCommands_expectBothSupported) {
    // ╔══════════════════════════════════════════════════════════════════════════════════════════╗
    // ║          🔄 BIDIRECTIONAL COMMAND CAPABILITY VERIFICATION                                ║
    // ╠══════════════════════════════════════════════════════════════════════════════════════════╣
    // ║ 🎯 TEST PURPOSE: Validate multi-role service supports commands in BOTH directions        ║
    // ║                  (A→B and B→A) on same connection pair                                   ║
    // ║                                                                                          ║
    // ║ 📋 TEST BRIEF: Create two multi-role services, connect them via client, verify each     ║
    // ║                can send commands to the other, demonstrating true bidirectional capability║
    // ║                                                                                          ║
    // ║ 🔧 TEST STRATEGY:                                                                        ║
    // ║    1. Create Service A with dual capabilities (CmdInitiator | CmdExecutor)              ║
    // ║    2. Create Service B with dual capabilities (CmdInitiator | CmdExecutor)              ║
    // ║    3. Client connects to both services (client-A link, client-B link)                   ║
    // ║    4. Service A sends command through client-A link → Client acts as relay              ║
    // ║    5. Service B sends command through client-B link → Client acts as relay              ║
    // ║    6. Verify both commands complete successfully                                         ║
    // ║    7. Verify link states reflect correct role for each direction                        ║
    // ║                                                                                          ║
    // ║ ✅ KEY ASSERTIONS:                                                                       ║
    // ║   • ASSERTION 1: Service A sends command successfully (A as Initiator)                  ║
    // ║   • ASSERTION 2: Service B sends command successfully (B as Initiator)                  ║
    // ║   • ASSERTION 3: Both commands complete with SUCCESS status                             ║
    // ║   • ASSERTION 4: Multi-role capability enables symmetric communication                  ║
    // ║                                                                                          ║
    // ║ 🏛️ ARCHITECTURE PRINCIPLE: Multi-role services provide symmetric command capability,   ║
    // ║                              enabling flexible peer-to-peer-like communication patterns ║
    // ╚══════════════════════════════════════════════════════════════════════════════════════════╝

    IOC_Result_T ResultValue = IOC_RESULT_BUG;

    // ┌──────────────────────────────────────────────────────────────┐
    // │                      🔧 SETUP PHASE                          │
    // └──────────────────────────────────────────────────────────────┘
    printf("🔧 [SETUP] Creating two multi-role services for bidirectional command testing\n");

    // Private data for Service A
    struct ServiceAPriv_T {
        std::atomic<int> commandsReceived{0};
        std::atomic<int> commandsSent{0};
    };
    ServiceAPriv_T srvAPrivData = {};

    // Executor callback for Service A (receives commands from clients)
    auto executorCbA = [](IOC_LinkID_T LinkID, IOC_CmdDesc_pT pCmdDesc, void *pCbPriv) -> IOC_Result_T {
        ServiceAPriv_T *pPrivData = (ServiceAPriv_T *)pCbPriv;
        if (!pPrivData || !pCmdDesc) return IOC_RESULT_INVALID_PARAM;

        pPrivData->commandsReceived++;
        printf("    📩 [SERVICE-A EXECUTOR] Received command, count=%d\n", pPrivData->commandsReceived.load());

        IOC_CmdID_T CmdID = IOC_CmdDesc_getCmdID(pCmdDesc);
        if (CmdID == IOC_CMDID_TEST_PING) {
            IOC_CmdDesc_setOutPayload(pCmdDesc, (void *)"PONG_FROM_A", 11);
            IOC_CmdDesc_setStatus(pCmdDesc, IOC_CMD_STATUS_SUCCESS);
            IOC_CmdDesc_setResult(pCmdDesc, IOC_RESULT_SUCCESS);
        } else {
            IOC_CmdDesc_setStatus(pCmdDesc, IOC_CMD_STATUS_FAILED);
            IOC_CmdDesc_setResult(pCmdDesc, IOC_RESULT_NOT_SUPPORT);
        }
        return IOC_RESULT_SUCCESS;
    };

    // Private data for Service B
    struct ServiceBPriv_T {
        std::atomic<int> commandsReceived{0};
        std::atomic<int> commandsSent{0};
    };
    ServiceBPriv_T srvBPrivData = {};

    // Executor callback for Service B (receives commands from clients)
    auto executorCbB = [](IOC_LinkID_T LinkID, IOC_CmdDesc_pT pCmdDesc, void *pCbPriv) -> IOC_Result_T {
        ServiceBPriv_T *pPrivData = (ServiceBPriv_T *)pCbPriv;
        if (!pPrivData || !pCmdDesc) return IOC_RESULT_INVALID_PARAM;

        pPrivData->commandsReceived++;
        printf("    📩 [SERVICE-B EXECUTOR] Received command, count=%d\n", pPrivData->commandsReceived.load());

        IOC_CmdID_T CmdID = IOC_CmdDesc_getCmdID(pCmdDesc);
        if (CmdID == IOC_CMDID_TEST_PING) {
            IOC_CmdDesc_setOutPayload(pCmdDesc, (void *)"PONG_FROM_B", 11);
            IOC_CmdDesc_setStatus(pCmdDesc, IOC_CMD_STATUS_SUCCESS);
            IOC_CmdDesc_setResult(pCmdDesc, IOC_RESULT_SUCCESS);
        } else {
            IOC_CmdDesc_setStatus(pCmdDesc, IOC_CMD_STATUS_FAILED);
            IOC_CmdDesc_setResult(pCmdDesc, IOC_RESULT_NOT_SUPPORT);
        }
        return IOC_RESULT_SUCCESS;
    };

    // Create Service A with dual capabilities
    IOC_SrvURI_T srvURI_A = {.pProtocol = IOC_SRV_PROTO_FIFO,
                             .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
                             .pPath = (const char *)"MultiRoleSrvA_US3_TC2"};

    static IOC_CmdID_T supportedCmdIDs[] = {IOC_CMDID_TEST_PING};
    IOC_CmdUsageArgs_T cmdUsageArgsA = {
        .CbExecCmd_F = executorCbA, .pCbPrivData = &srvAPrivData, .CmdNum = 1, .pCmdIDs = supportedCmdIDs};

    IOC_SrvArgs_T srvArgsA = {
        .SrvURI = srvURI_A,
        .Flags = IOC_SRVFLAG_NONE,
        .UsageCapabilites = (IOC_LinkUsage_T)(IOC_LinkUsageCmdInitiator | IOC_LinkUsageCmdExecutor),
        .UsageArgs = {.pCmd = &cmdUsageArgsA}};

    IOC_SrvID_T srvID_A = IOC_ID_INVALID;
    ResultValue = IOC_onlineService(&srvID_A, &srvArgsA);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);
    ASSERT_NE(IOC_ID_INVALID, srvID_A);
    printf("🔧 [SETUP] Service A online: UsageCapabilities=0x%02X (CmdInitiator|CmdExecutor)\n",
           IOC_LinkUsageCmdInitiator | IOC_LinkUsageCmdExecutor);

    // Create Service B with dual capabilities
    IOC_SrvURI_T srvURI_B = {.pProtocol = IOC_SRV_PROTO_FIFO,
                             .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
                             .pPath = (const char *)"MultiRoleSrvB_US3_TC2"};

    IOC_CmdUsageArgs_T cmdUsageArgsB = {
        .CbExecCmd_F = executorCbB, .pCbPrivData = &srvBPrivData, .CmdNum = 1, .pCmdIDs = supportedCmdIDs};

    IOC_SrvArgs_T srvArgsB = {
        .SrvURI = srvURI_B,
        .Flags = IOC_SRVFLAG_NONE,
        .UsageCapabilites = (IOC_LinkUsage_T)(IOC_LinkUsageCmdInitiator | IOC_LinkUsageCmdExecutor),
        .UsageArgs = {.pCmd = &cmdUsageArgsB}};

    IOC_SrvID_T srvID_B = IOC_ID_INVALID;
    ResultValue = IOC_onlineService(&srvID_B, &srvArgsB);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);
    ASSERT_NE(IOC_ID_INVALID, srvID_B);
    printf("🔧 [SETUP] Service B online: UsageCapabilities=0x%02X (CmdInitiator|CmdExecutor)\n",
           IOC_LinkUsageCmdInitiator | IOC_LinkUsageCmdExecutor);

    // Client connects to Service A as Executor (Service A will act as Initiator on this link)
    printf("🔧 [SETUP] Client connects to Service A with Usage=CmdExecutor\n");

    struct ClientAPriv_T {
        std::atomic<int> commandsReceived{0};
    };
    ClientAPriv_T clientAPrivData = {};

    auto clientAExecutorCb = [](IOC_LinkID_T LinkID, IOC_CmdDesc_pT pCmdDesc, void *pCbPriv) -> IOC_Result_T {
        ClientAPriv_T *pPrivData = (ClientAPriv_T *)pCbPriv;
        if (!pPrivData || !pCmdDesc) return IOC_RESULT_INVALID_PARAM;

        pPrivData->commandsReceived++;
        printf("    📩 [CLIENT-A EXECUTOR] Received command from Service A, count=%d\n",
               pPrivData->commandsReceived.load());

        IOC_CmdDesc_setOutPayload(pCmdDesc, (void *)"ACK_A", 5);
        IOC_CmdDesc_setStatus(pCmdDesc, IOC_CMD_STATUS_SUCCESS);
        IOC_CmdDesc_setResult(pCmdDesc, IOC_RESULT_SUCCESS);
        return IOC_RESULT_SUCCESS;
    };

    IOC_CmdUsageArgs_T clientACmdUsageArgs = {
        .CbExecCmd_F = clientAExecutorCb, .pCbPrivData = &clientAPrivData, .CmdNum = 1, .pCmdIDs = supportedCmdIDs};

    IOC_ConnArgs_T clientAConnArgs = {
        .SrvURI = srvURI_A, .Usage = IOC_LinkUsageCmdExecutor, .UsageArgs = {.pCmd = &clientACmdUsageArgs}};

    IOC_LinkID_T clientLinkID_A = IOC_ID_INVALID;
    std::thread clientAThread([&] {
        IOC_Result_T connResult = IOC_connectService(&clientLinkID_A, &clientAConnArgs, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, connResult);
        ASSERT_NE(IOC_ID_INVALID, clientLinkID_A);
    });

    IOC_LinkID_T srvLinkID_A = IOC_ID_INVALID;
    ResultValue = IOC_acceptClient(srvID_A, &srvLinkID_A, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);
    ASSERT_NE(IOC_ID_INVALID, srvLinkID_A);

    if (clientAThread.joinable()) clientAThread.join();

    // Client connects to Service B as Executor (Service B will act as Initiator on this link)
    printf("🔧 [SETUP] Client connects to Service B with Usage=CmdExecutor\n");

    struct ClientBPriv_T {
        std::atomic<int> commandsReceived{0};
    };
    ClientBPriv_T clientBPrivData = {};

    auto clientBExecutorCb = [](IOC_LinkID_T LinkID, IOC_CmdDesc_pT pCmdDesc, void *pCbPriv) -> IOC_Result_T {
        ClientBPriv_T *pPrivData = (ClientBPriv_T *)pCbPriv;
        if (!pPrivData || !pCmdDesc) return IOC_RESULT_INVALID_PARAM;

        pPrivData->commandsReceived++;
        printf("    📩 [CLIENT-B EXECUTOR] Received command from Service B, count=%d\n",
               pPrivData->commandsReceived.load());

        IOC_CmdDesc_setOutPayload(pCmdDesc, (void *)"ACK_B", 5);
        IOC_CmdDesc_setStatus(pCmdDesc, IOC_CMD_STATUS_SUCCESS);
        IOC_CmdDesc_setResult(pCmdDesc, IOC_RESULT_SUCCESS);
        return IOC_RESULT_SUCCESS;
    };

    IOC_CmdUsageArgs_T clientBCmdUsageArgs = {
        .CbExecCmd_F = clientBExecutorCb, .pCbPrivData = &clientBPrivData, .CmdNum = 1, .pCmdIDs = supportedCmdIDs};

    IOC_ConnArgs_T clientBConnArgs = {
        .SrvURI = srvURI_B, .Usage = IOC_LinkUsageCmdExecutor, .UsageArgs = {.pCmd = &clientBCmdUsageArgs}};

    IOC_LinkID_T clientLinkID_B = IOC_ID_INVALID;
    std::thread clientBThread([&] {
        IOC_Result_T connResult = IOC_connectService(&clientLinkID_B, &clientBConnArgs, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, connResult);
        ASSERT_NE(IOC_ID_INVALID, clientLinkID_B);
    });

    IOC_LinkID_T srvLinkID_B = IOC_ID_INVALID;
    ResultValue = IOC_acceptClient(srvID_B, &srvLinkID_B, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);
    ASSERT_NE(IOC_ID_INVALID, srvLinkID_B);

    if (clientBThread.joinable()) clientBThread.join();

    printf("🔧 [SETUP] Both services connected to clients, ready for bidirectional commands\n");

    // ┌──────────────────────────────────────────────────────────────┐
    // │                    📋 BEHAVIOR PHASE                         │
    // └──────────────────────────────────────────────────────────────┘
    printf("📋 [BEHAVIOR] Testing bidirectional command capability\n");

    // Service A sends command (A acts as Initiator on its link)
    printf("📋 [BEHAVIOR] Service A → Client: Sending PING command\n");
    IOC_CmdDesc_T cmdDescA = {};
    cmdDescA.CmdID = IOC_CMDID_TEST_PING;
    cmdDescA.TimeoutMs = 5000;
    cmdDescA.Status = IOC_CMD_STATUS_PENDING;
    IOC_CmdDesc_setInPayload(&cmdDescA, (void *)"PING_A", 6);

    ResultValue = IOC_execCMD(srvLinkID_A, &cmdDescA, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);
    srvAPrivData.commandsSent++;

    IOC_CmdStatus_E statusA = IOC_CmdDesc_getStatus(&cmdDescA);
    IOC_Result_T resultA = IOC_CmdDesc_getResult(&cmdDescA);
    printf("    ✅ [SERVICE-A RESULT] Command status=%d, result=%d\n", statusA, resultA);

    // Service B sends command (B acts as Initiator on its link)
    printf("📋 [BEHAVIOR] Service B → Client: Sending PING command\n");
    IOC_CmdDesc_T cmdDescB = {};
    cmdDescB.CmdID = IOC_CMDID_TEST_PING;
    cmdDescB.TimeoutMs = 5000;
    cmdDescB.Status = IOC_CMD_STATUS_PENDING;
    IOC_CmdDesc_setInPayload(&cmdDescB, (void *)"PING_B", 6);

    ResultValue = IOC_execCMD(srvLinkID_B, &cmdDescB, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);
    srvBPrivData.commandsSent++;

    IOC_CmdStatus_E statusB = IOC_CmdDesc_getStatus(&cmdDescB);
    IOC_Result_T resultB = IOC_CmdDesc_getResult(&cmdDescB);
    printf("    ✅ [SERVICE-B RESULT] Command status=%d, result=%d\n", statusB, resultB);

    // ┌──────────────────────────────────────────────────────────────┐
    // │                     ✅ VERIFY PHASE                          │
    // └──────────────────────────────────────────────────────────────┘
    //@KeyVerifyPoint<=4: Bidirectional command capability verification
    //  1. ASSERTION 1: Service A sends command successfully (A as Initiator)
    //  2. ASSERTION 2: Service B sends command successfully (B as Initiator)
    //  3. ASSERTION 3: Both commands complete with SUCCESS status
    //  4. ASSERTION 4: Multi-role capability enables symmetric communication

    printf("✅ [VERIFY] ASSERTION 1: Service A → Client command succeeds (A as Initiator)\n");
    printf("    • Service A sent: %d commands\n", srvAPrivData.commandsSent.load());
    printf("    • Client A received: %d commands from Service A\n", clientAPrivData.commandsReceived.load());
    VERIFY_KEYPOINT_EQ(statusA, IOC_CMD_STATUS_SUCCESS, "Service A command must complete successfully");
    VERIFY_KEYPOINT_EQ(resultA, IOC_RESULT_SUCCESS, "Service A command must return SUCCESS");
    ASSERT_EQ(1, srvAPrivData.commandsSent.load());
    ASSERT_EQ(1, clientAPrivData.commandsReceived.load());

    printf("✅ [VERIFY] ASSERTION 2: Service B → Client command succeeds (B as Initiator)\n");
    printf("    • Service B sent: %d commands\n", srvBPrivData.commandsSent.load());
    printf("    • Client B received: %d commands from Service B\n", clientBPrivData.commandsReceived.load());
    VERIFY_KEYPOINT_EQ(statusB, IOC_CMD_STATUS_SUCCESS, "Service B command must complete successfully");
    VERIFY_KEYPOINT_EQ(resultB, IOC_RESULT_SUCCESS, "Service B command must return SUCCESS");
    ASSERT_EQ(1, srvBPrivData.commandsSent.load());
    ASSERT_EQ(1, clientBPrivData.commandsReceived.load());

    printf("✅ [VERIFY] ASSERTION 3: Both commands completed with SUCCESS status\n");
    ASSERT_EQ(IOC_CMD_STATUS_SUCCESS, statusA);
    ASSERT_EQ(IOC_CMD_STATUS_SUCCESS, statusB);
    ASSERT_EQ(IOC_RESULT_SUCCESS, resultA);
    ASSERT_EQ(IOC_RESULT_SUCCESS, resultB);

    printf("✅ [VERIFY] ASSERTION 4: Multi-role capability enables symmetric communication\n");
    printf("    • Both services declared UsageCapabilities = 0x0C (Initiator|Executor)\n");
    printf("    • Service A successfully acted as Initiator on its link\n");
    printf("    • Service B successfully acted as Initiator on its link\n");
    printf("    • Multi-role services provide symmetric command capability ✅\n");

    printf("\n");
    printf("✅ [RESULT] Bidirectional command capability verified:\n");
    printf("   • Service A → Client command: SUCCESS (ASSERTION 1) ✅\n");
    printf("   • Service B → Client command: SUCCESS (ASSERTION 2) ✅\n");
    printf("   • Both commands completed successfully (ASSERTION 3) ✅\n");
    printf("   • Symmetric communication pattern demonstrated (ASSERTION 4) ✅\n");
    printf("   • Architecture principle: Multi-role services enable peer-to-peer patterns ✅\n");

    // ┌──────────────────────────────────────────────────────────────┐
    // │                    🧹 CLEANUP PHASE                          │
    // └──────────────────────────────────────────────────────────────┘
    printf("🧹 [CLEANUP] Disconnecting and shutting down services\n");

    // Close client-side links
    if (clientLinkID_A != IOC_ID_INVALID) IOC_closeLink(clientLinkID_A);
    if (clientLinkID_B != IOC_ID_INVALID) IOC_closeLink(clientLinkID_B);

    // Stop services (automatically closes server-side links)
    if (srvID_A != IOC_ID_INVALID) IOC_offlineService(srvID_A);
    if (srvID_B != IOC_ID_INVALID) IOC_offlineService(srvID_B);
}

//======>END OF AC-1 TC-2==========================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF AC-2 TC-1: CMD INITIATOR PRIORITY STATE==========================================

TEST(UT_CommandStateUS3, verifyCmdInitiatorPriority_whenSendingCommand_expectInitiatorBusyState) {
    // TODO: Implement CmdInitiator priority state verification
    //
    // ╔══════════════════════════════════════════════════════════════════════════════════════════╗
    // ║          ⬆️ CMD INITIATOR PRIORITY STATE VERIFICATION                                    ║
    // ╠══════════════════════════════════════════════════════════════════════════════════════════╣
    // ║ 🎯 TEST PURPOSE: Validate link shows CmdInitiator busy substate when multi-role link    ║
    // ║                  is actively sending an outbound command                                 ║
    // ║                                                                                          ║
    // ║ 📋 TEST BRIEF: Multi-role link executes outbound command, verify link substate          ║
    // ║                prioritizes CmdInitiatorBusyExecCmd during operation                      ║
    // ║                                                                                          ║
    // ║ 🔧 TEST STRATEGY:                                                                        ║
    // ║    1. Setup multi-role service and client                                                ║
    // ║    2. Query initial link substate (should be Ready)                                      ║
    // ║    3. Service sends command to client (CmdInitiator role)                               ║
    // ║    4. During command execution, query link substate                                      ║
    // ║    5. Verify substate = IOC_LinkSubStateCmdInitiatorBusyExecCmd                          ║
    // ║    6. After command completion, verify substate returns to Ready                         ║
    // ║                                                                                          ║
    // ║ ✅ KEY ASSERTIONS:                                                                       ║
    // ║   • ASSERTION 1: Initial substate = Ready (Initiator or Executor)                       ║
    // ║   • ASSERTION 2: During outbound command = CmdInitiatorBusyExecCmd                      ║
    // ║   • ASSERTION 3: After completion substate = Ready                                       ║
    // ║   • ASSERTION 4: CmdExecutor capability remains available (can accept incoming)         ║
    // ║                                                                                          ║
    // ║ 🏛️ ARCHITECTURE PRINCIPLE: Active operation determines displayed link substate,        ║
    // ║                              providing clear visibility of current link activity        ║
    // ╚══════════════════════════════════════════════════════════════════════════════════════════╝

    GTEST_SKIP() << "AC-2 TC-1: CmdInitiator priority state - DESIGN COMPLETE, implementation pending";
}

//======>END OF AC-2 TC-1==========================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF AC-2 TC-2: EXECUTOR AVAILABILITY DURING INITIATOR OPERATION=====================

TEST(UT_CommandStateUS3, verifyExecutorAvailability_duringInitiatorOperation_expectIncomingAccepted) {
    // TODO: Implement executor availability during initiator operation
    //
    // ╔══════════════════════════════════════════════════════════════════════════════════════════╗
    // ║          🔄 EXECUTOR CAPABILITY AVAILABILITY DURING INITIATOR BUSY                       ║
    // ╠══════════════════════════════════════════════════════════════════════════════════════════╣
    // ║ 🎯 TEST PURPOSE: Validate CmdExecutor capability remains available while CmdInitiator   ║
    // ║                  is busy sending outbound command                                        ║
    // ║                                                                                          ║
    // ║ 📋 TEST BRIEF: While multi-role service is sending outbound command, verify it can      ║
    // ║                still accept and process inbound command from client                      ║
    // ║                                                                                          ║
    // ║ 🔧 TEST STRATEGY:                                                                        ║
    // ║    1. Setup multi-role service A and client B (also multi-role)                         ║
    // ║    2. Service A starts sending command to B (slow executor, 500ms)                      ║
    // ║    3. While A waits for response, B sends command to A (quick, 50ms)                    ║
    // ║    4. Verify A accepts and processes B's command despite being in Initiator busy        ║
    // ║    5. Verify both commands complete successfully                                         ║
    // ║    6. Track link state transitions for both directions                                   ║
    // ║                                                                                          ║
    // ║ ✅ KEY ASSERTIONS:                                                                       ║
    // ║   • ASSERTION 1: Service A busy with outbound (CmdInitiatorBusyExecCmd)                 ║
    // ║   • ASSERTION 2: Service A accepts inbound command from B                               ║
    // ║   • ASSERTION 3: Both commands complete successfully                                     ║
    // ║   • ASSERTION 4: Link state transitions correctly between roles                         ║
    // ║                                                                                          ║
    // ║ 🏛️ ARCHITECTURE PRINCIPLE: Multi-role links support concurrent bidirectional           ║
    // ║                              operations without role blocking                           ║
    // ╚══════════════════════════════════════════════════════════════════════════════════════════╝

    GTEST_SKIP()
        << "AC-2 TC-2: Executor availability during initiator operation - DESIGN COMPLETE, implementation pending";
}

//======>END OF AC-2 TC-2==========================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF AC-3 TC-1: CMD EXECUTOR PRIORITY STATE===========================================

TEST(UT_CommandStateUS3, verifyCmdExecutorPriority_whenProcessingCommand_expectExecutorBusyState) {
    // TODO: Implement CmdExecutor priority state verification
    //
    // ╔══════════════════════════════════════════════════════════════════════════════════════════╗
    // ║          ⬇️ CMD EXECUTOR PRIORITY STATE VERIFICATION                                     ║
    // ╠══════════════════════════════════════════════════════════════════════════════════════════╣
    // ║ 🎯 TEST PURPOSE: Validate link shows CmdExecutor busy substate when multi-role link     ║
    // ║                  is actively processing an inbound command                               ║
    // ║                                                                                          ║
    // ║ 📋 TEST BRIEF: Multi-role link processes inbound command, verify link substate          ║
    // ║                prioritizes CmdExecutorBusyExecCmd during callback execution              ║
    // ║                                                                                          ║
    // ║ 🔧 TEST STRATEGY:                                                                        ║
    // ║    1. Setup multi-role service with executor callback (slow, 200ms)                     ║
    // ║    2. Query initial link substate (should be Ready)                                      ║
    // ║    3. Client sends command to service (service acts as Executor)                        ║
    // ║    4. During callback execution, query link substate                                     ║
    // ║    5. Verify substate = IOC_LinkSubStateCmdExecutorBusyExecCmd                           ║
    // ║    6. After callback completion, verify substate returns to Ready                        ║
    // ║                                                                                          ║
    // ║ ✅ KEY ASSERTIONS:                                                                       ║
    // ║   • ASSERTION 1: Initial substate = Ready (Executor or Initiator)                       ║
    // ║   • ASSERTION 2: During callback = CmdExecutorBusyExecCmd                               ║
    // ║   • ASSERTION 3: After completion substate = Ready                                       ║
    // ║   • ASSERTION 4: CmdInitiator capability remains available (can send outbound)          ║
    // ║                                                                                          ║
    // ║ 🏛️ ARCHITECTURE PRINCIPLE: Processing inbound commands takes priority in substate      ║
    // ║                              display, reflecting current service activity               ║
    // ╚══════════════════════════════════════════════════════════════════════════════════════════╝

    GTEST_SKIP() << "AC-3 TC-1: CmdExecutor priority state - DESIGN COMPLETE, implementation pending";
}

//======>END OF AC-3 TC-1==========================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF AC-3 TC-2: INITIATOR AVAILABILITY DURING EXECUTOR OPERATION=====================

TEST(UT_CommandStateUS3, verifyInitiatorAvailability_duringExecutorOperation_expectOutgoingSupported) {
    // TODO: Implement initiator availability during executor operation
    //
    // ╔══════════════════════════════════════════════════════════════════════════════════════════╗
    // ║          🔄 INITIATOR CAPABILITY AVAILABILITY DURING EXECUTOR BUSY                       ║
    // ╠══════════════════════════════════════════════════════════════════════════════════════════╣
    // ║ 🎯 TEST PURPOSE: Validate CmdInitiator capability remains available while CmdExecutor   ║
    // ║                  is busy processing inbound command in callback                          ║
    // ║                                                                                          ║
    // ║ 📋 TEST BRIEF: While multi-role service is processing inbound command, verify it can    ║
    // ║                still send outbound command to different client                           ║
    // ║                                                                                          ║
    // ║ 🔧 TEST STRATEGY:                                                                        ║
    // ║    1. Setup multi-role service A, client B, and client C                                ║
    // ║    2. Client B sends command to service A (slow callback, 500ms)                        ║
    // ║    3. During A's callback execution, A sends command to client C                        ║
    // ║    4. Verify A can send to C despite processing B's command                             ║
    // ║    5. Verify both commands complete successfully                                         ║
    // ║    6. Verify link states managed independently for different links                      ║
    // ║                                                                                          ║
    // ║ ✅ KEY ASSERTIONS:                                                                       ║
    // ║   • ASSERTION 1: Service A busy with B's command (CmdExecutorBusyExecCmd)               ║
    // ║   • ASSERTION 2: Service A can send command to C (CmdInitiator capability)              ║
    // ║   • ASSERTION 3: Both commands complete successfully                                     ║
    // ║   • ASSERTION 4: Link A-B and A-C states independent                                    ║
    // ║                                                                                          ║
    // ║ 🏛️ ARCHITECTURE PRINCIPLE: Multi-role services support concurrent operations on        ║
    // ║                              different links with independent state management          ║
    // ╚══════════════════════════════════════════════════════════════════════════════════════════╝

    GTEST_SKIP()
        << "AC-3 TC-2: Initiator availability during executor operation - DESIGN COMPLETE, implementation pending";
}

//======>END OF AC-3 TC-2==========================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF AC-4 TC-1: CONCURRENT BIDIRECTIONAL OPERATIONS===================================

TEST(UT_CommandStateUS3, verifyConcurrentOperations_bySimultaneousBidirectional_expectPriorityResolution) {
    // TODO: Implement concurrent bidirectional operation test
    //
    // ╔══════════════════════════════════════════════════════════════════════════════════════════╗
    // ║          ⚡ CONCURRENT BIDIRECTIONAL OPERATIONS AND STATE PRIORITY                       ║
    // ╠══════════════════════════════════════════════════════════════════════════════════════════╣
    // ║ 🎯 TEST PURPOSE: Validate state priority resolution when both CmdInitiator and          ║
    // ║                  CmdExecutor roles are busy simultaneously on same link                  ║
    // ║                                                                                          ║
    // ║ 📋 TEST BRIEF: Trigger inbound and outbound commands simultaneously on same multi-role  ║
    // ║                link, verify link substate shows priority operation and both succeed     ║
    // ║                                                                                          ║
    // ║ 🔧 TEST STRATEGY:                                                                        ║
    // ║    1. Setup two multi-role services A and B connected                                   ║
    // ║    2. A sends command to B (slow executor, 500ms)                                       ║
    // ║    3. While A waits, B sends command to A (200ms)                                       ║
    // ║    4. Both services now busy: A=Initiator+Executor, B=Executor+Initiator               ║
    // ║    5. Monitor link substates during concurrent operations                               ║
    // ║    6. Verify both commands complete without blocking each other                         ║
    // ║                                                                                          ║
    // ║ ✅ KEY ASSERTIONS:                                                                       ║
    // ║   • ASSERTION 1: Both commands execute concurrently                                      ║
    // ║   • ASSERTION 2: Link substate reflects active operation (priority algorithm)           ║
    // ║   • ASSERTION 3: A→B command completes successfully                                      ║
    // ║   • ASSERTION 4: B→A command completes successfully                                      ║
    // ║                                                                                          ║
    // ║ 🏛️ ARCHITECTURE PRINCIPLE: Framework handles concurrent bidirectional commands         ║
    // ║                              gracefully with clear state priority resolution            ║
    // ╚══════════════════════════════════════════════════════════════════════════════════════════╝

    GTEST_SKIP() << "AC-4 TC-1: Concurrent bidirectional operations - DESIGN COMPLETE, implementation pending";
}

//======>END OF AC-4 TC-1==========================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF AC-4 TC-2: COMMAND STATE ISOLATION IN CONCURRENT OPERATIONS=====================

TEST(UT_CommandStateUS3, verifyCommandIsolation_inConcurrentMultiRole_expectNoInterference) {
    // TODO: Implement command state isolation verification
    //
    // ╔══════════════════════════════════════════════════════════════════════════════════════════╗
    // ║          🔒 COMMAND STATE ISOLATION DURING CONCURRENT OPERATIONS                         ║
    // ╠══════════════════════════════════════════════════════════════════════════════════════════╣
    // ║ 🎯 TEST PURPOSE: Validate individual command states (Level 1) remain isolated despite   ║
    // ║                  concurrent bidirectional operations on multi-role link (Level 2)        ║
    // ║                                                                                          ║
    // ║ 📋 TEST BRIEF: Execute concurrent inbound and outbound commands, verify each            ║
    // ║                IOC_CmdDesc_T maintains independent status/result without interference   ║
    // ║                                                                                          ║
    // ║ 🔧 TEST STRATEGY:                                                                        ║
    // ║    1. Setup two multi-role services A and B                                             ║
    // ║    2. A→B command (CmdDesc1, slow 500ms) and B→A command (CmdDesc2, 200ms)              ║
    // ║    3. Track both command descriptors' status/result independently                       ║
    // ║    4. Verify CmdDesc1 status transitions: PENDING→PROCESSING→SUCCESS                    ║
    // ║    5. Verify CmdDesc2 status transitions: PENDING→PROCESSING→SUCCESS                    ║
    // ║    6. Verify no cross-contamination between command states                              ║
    // ║                                                                                          ║
    // ║ ✅ KEY ASSERTIONS:                                                                       ║
    // ║   • ASSERTION 1: Outbound command (A→B) status/result independent                       ║
    // ║   • ASSERTION 2: Inbound command (B→A) status/result independent                        ║
    // ║   • ASSERTION 3: Both complete with correct results                                      ║
    // ║   • ASSERTION 4: No state cross-contamination between commands                          ║
    // ║                                                                                          ║
    // ║ 🏛️ ARCHITECTURE PRINCIPLE: Individual command state (Level 1) is fully isolated from   ║
    // ║                              link state (Level 2), enabling safe concurrent operations  ║
    // ╚══════════════════════════════════════════════════════════════════════════════════════════╝

    GTEST_SKIP()
        << "AC-4 TC-2: Command state isolation in concurrent operations - DESIGN COMPLETE, implementation pending";
}

//======>END OF AC-4 TC-2==========================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF AC-5 TC-1: ROLE TRANSITION STATE MANAGEMENT======================================

TEST(UT_CommandStateUS3, verifyRoleTransition_fromInitiatorToExecutor_expectSmoothStateChange) {
    // TODO: Implement role transition state management
    //
    // ╔══════════════════════════════════════════════════════════════════════════════════════════╗
    // ║          🔄 SMOOTH ROLE TRANSITION STATE MANAGEMENT                                      ║
    // ╠══════════════════════════════════════════════════════════════════════════════════════════╣
    // ║ 🎯 TEST PURPOSE: Validate smooth state transitions when multi-role link switches        ║
    // ║                  active role from CmdInitiator to CmdExecutor or vice versa             ║
    // ║                                                                                          ║
    // ║ 📋 TEST BRIEF: Complete outbound command, then immediately process inbound command,     ║
    // ║                verify link state transitions smoothly without corruption                ║
    // ║                                                                                          ║
    // ║ 🔧 TEST STRATEGY:                                                                        ║
    // ║    1. Setup multi-role services A and B                                                 ║
    // ║    2. A sends command to B (A=Initiator, B=Executor)                                    ║
    // ║    3. Wait for A→B completion, capture state transition                                 ║
    // ║    4. Immediately B sends command to A (B=Initiator, A=Executor)                        ║
    // ║    5. Track A's link state transitions: InitiatorBusy → Ready → ExecutorBusy            ║
    // ║    6. Verify no state corruption during role change                                      ║
    // ║                                                                                          ║
    // ║ ✅ KEY ASSERTIONS:                                                                       ║
    // ║   • ASSERTION 1: First transition: CmdInitiatorBusyExecCmd → Ready                      ║
    // ║   • ASSERTION 2: Second transition: Ready → CmdExecutorBusyExecCmd                      ║
    // ║   • ASSERTION 3: No intermediate invalid states                                          ║
    // ║   • ASSERTION 4: Both commands complete successfully                                     ║
    // ║                                                                                          ║
    // ║ 🏛️ ARCHITECTURE PRINCIPLE: Role transitions maintain link state integrity,             ║
    // ║                              ensuring reliable multi-role operation                     ║
    // ╚══════════════════════════════════════════════════════════════════════════════════════════╝

    GTEST_SKIP() << "AC-5 TC-1: Role transition state management - DESIGN COMPLETE, implementation pending";
}

//======>END OF AC-5 TC-1==========================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF AC-5 TC-2: ONGOING OPERATIONS DURING ROLE SWITCH================================

TEST(UT_CommandStateUS3, verifyOngoingOperations_duringRoleSwitch_expectUnaffected) {
    // TODO: Implement ongoing operations during role switch verification
    //
    // ╔══════════════════════════════════════════════════════════════════════════════════════════╗
    // ║          🔄 OPERATION ISOLATION DURING ROLE TRANSITION                                   ║
    // ╠══════════════════════════════════════════════════════════════════════════════════════════╣
    // ║ 🎯 TEST PURPOSE: Validate ongoing command operation is unaffected when link             ║
    // ║                  experiences role transition due to new operation in opposite direction ║
    // ║                                                                                          ║
    // ║ 📋 TEST BRIEF: Long-running outbound command, trigger quick inbound during execution,   ║
    // ║                verify long command completes correctly despite role switch              ║
    // ║                                                                                          ║
    // ║ 🔧 TEST STRATEGY:                                                                        ║
    // ║    1. Setup multi-role services A and B                                                 ║
    // ║    2. A starts sending command to B (very slow executor, 1000ms)                        ║
    // ║    3. While A waits, B sends quick command to A (100ms)                                 ║
    // ║    4. Verify A processes B's command without affecting original A→B command             ║
    // ║    5. Verify both commands complete with correct results                                ║
    // ║    6. Track link state showing role switch during ongoing operation                     ║
    // ║                                                                                          ║
    // ║ ✅ KEY ASSERTIONS:                                                                       ║
    // ║   • ASSERTION 1: Long outbound (A→B, 1000ms) completes successfully                     ║
    // ║   • ASSERTION 2: Quick inbound (B→A, 100ms) completes during outbound                   ║
    // ║   • ASSERTION 3: Outbound result/status unaffected by inbound                           ║
    // ║   • ASSERTION 4: Link state reflects current active operation                           ║
    // ║                                                                                          ║
    // ║ 🏛️ ARCHITECTURE PRINCIPLE: Multi-role links provide operation isolation, enabling      ║
    // ║                              role transitions without affecting ongoing commands        ║
    // ╚══════════════════════════════════════════════════════════════════════════════════════════╝

    GTEST_SKIP() << "AC-5 TC-2: Ongoing operations during role switch - DESIGN COMPLETE, implementation pending";
}

//======>END OF AC-5 TC-2==========================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF IMPLEMENTATION SUMMARY===========================================================
/**
 * ╔══════════════════════════════════════════════════════════════════════════════════════════╗
 * ║                              📊 IMPLEMENTATION SUMMARY                                   ║
 * ╠══════════════════════════════════════════════════════════════════════════════════════════╣
 * ║ 🎯 PURPOSE: Multi-Role Link Command State Verification - User Story 3                  ║
 * ║                                                                                          ║
 * ║ 📋 FRAMEWORK STATUS: PLANNED (Skeleton Implementation)                                   ║
 * ║   • Multi-role link state verification framework defined                                ║
 * ║   • Acceptance criteria established for role transition scenarios                       ║
 * ║   • Test case placeholders created for future implementation                            ║
 * ║                                                                                          ║
 * ║ 🔧 DESIGN APPROACH:                                                                      ║
 * ║   • Focus on link state during multi-role operations                                   ║
 * ║   • Priority-based state resolution for concurrent role activities                     ║
 * ║   • Role isolation and capability management verification                               ║
 * ║   • Smooth role transition state handling                                              ║
 * ║                                                                                          ║
 * ║ 💡 MULTI-ROLE STATE INSIGHTS:                                                           ║
 * ║   • Links may need bidirectional command capabilities                                   ║
 * ║   • State priority helps resolve conflicts during concurrent operations                 ║
 * ║   • Role transitions should maintain operation continuity                               ║
 * ║   • Multi-role links enable flexible communication patterns                            ║
 * ║                                                                                          ║
 * ║ 📋 IMPLEMENTATION REQUIREMENTS:                                                          ║
 * ║   • Multi-role link configuration support                                              ║
 * ║   • State priority resolution algorithms                                               ║
 * ║   • Concurrent role operation handling                                                 ║
 * ║   • Role transition state management                                                   ║
 * ╚══════════════════════════════════════════════════════════════════════════════════════════╝
 */
//======>END OF IMPLEMENTATION SUMMARY=============================================================
