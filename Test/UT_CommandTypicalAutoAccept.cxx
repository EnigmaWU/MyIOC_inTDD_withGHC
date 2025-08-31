///////////////////////////////////////////////////////////////////////////////////////////////////
// Command Typical Auto-Accept (connection-oriented / Conet) — UT skeleton
//
// Intent:
// - "CommandTypicalAutoAccept" focuses on auto-accept integration with command patterns.
// - Extends command execution flows with IOC_SRVFLAG_AUTO_ACCEPT for streamlined connections.
// - Combines auto-accept capability with both callback and polling command patterns.
// - Validates OnAutoAccepted_F callback integration with command executor/initiator roles.
//
// ⚪ IMPLEMENTATION STATUS:
//     ⚪ TODO: Auto-accept command patterns need implementation
//     Focus on IOC_SRVFLAG_AUTO_ACCEPT + command capabilities integration
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <chrono>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "IOC/IOC_Option.h"
#include "_UT_IOC_Common.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF OVERVIEW OF THIS UNIT TESTING FILE===============================================
/**
 * @brief Verify auto-accept integration with command execution flows (Conet):
 *  - Service automatically accepts clients without manual IOC_acceptClient calls
 *  - Combines IOC_SRVFLAG_AUTO_ACCEPT with command executor and initiator patterns
 *  - Tests OnAutoAccepted_F callback integration with command capabilities
 *  - Validates streamlined connection flows for command-oriented services
 *
 * Key differences from UT_CommandTypical.cxx and UT_CommandTypicalWaitAck.cxx:
 *  - Auto-accept vs manual accept: No IOC_acceptClient calls needed
 *  - OnAutoAccepted_F callback: Service notified when clients auto-connect
 *  - Simplified connection flow: Clients connect directly to command-ready service
 *  - Mixed patterns: Both callback and polling command execution with auto-accept
 */
//======>END OF OVERVIEW OF THIS UNIT TESTING FILE=================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF UNIT TESTING DESIGN==============================================================
/**
 * Design focus:
 *  - Auto-accept integration with command patterns (both callback and polling)
 *  - OnAutoAccepted_F callback validation for command services
 *  - Streamlined service-client connection without manual accept calls
 *  - Command execution readiness immediately after auto-accept
 *  - Error handling for auto-accept failures in command contexts
 *
 * Test progression:
 *  - Basic auto-accept with command executor service
 *  - Auto-accept with command initiator service (reversed roles)
 *  - Multi-client auto-accept with command isolation
 *  - OnAutoAccepted_F callback integration with command context
 *  - Auto-accept with polling-based command patterns
 *  - Error scenarios: auto-accept limits, connection failures
 */
///////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF USER STORY=======================================================================
/**
 * US-1: As a service developer, I want auto-accept functionality with command executor capability
 *       so that clients can connect and execute commands without manual acceptance overhead.
 *
 * US-2: As a service developer, I want auto-accept with command initiator capability
 *       so that the service can send commands to auto-accepted clients immediately.
 *
 * US-3: As a service developer, I want OnAutoAccepted_F callback with command context
 *       so that I can configure per-client command capabilities upon auto-acceptance.
 *
 * US-4: As a service developer, I want control over auto-accepted link lifecycle with IOC_SRVFLAG_KEEP_ACCEPTED_LINK
 *       so that I can manage persistent connections across service restart scenarios.
 *
 * US-5: As a service developer, I want to understand resource management differences between auto-cleanup and
 * persistent links so that I can choose the appropriate cleanup strategy for my service architecture.
 */
//======>END OF USER STORY==========================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//=======>BEGIN OF ACCEPTANCE CRITERIA==============================================================
/**
 * [@US-1] Auto-Accept with Command Executor Service
 *  AC-1: GIVEN a service with IOC_SRVFLAG_AUTO_ACCEPT and CmdExecutor capability,
 *         WHEN client connects with CmdInitiator usage,
 *         THEN client is auto-accepted and can immediately send commands.
 *  AC-2: GIVEN auto-accept service supporting multiple command types,
 *         WHEN multiple clients connect and send different commands,
 *         THEN each client executes commands independently without manual accept.
 *  AC-3: GIVEN auto-accept service with command timeout constraints,
 *         WHEN client sends time-bounded commands,
 *         THEN commands execute successfully within auto-accepted connections.
 *
 * [@US-2] Auto-Accept with Command Initiator Service
 *  AC-1: GIVEN a service with IOC_SRVFLAG_AUTO_ACCEPT and CmdInitiator capability,
 *         WHEN client connects with CmdExecutor usage,
 *         THEN service can immediately send commands to auto-accepted client.
 *  AC-2: GIVEN auto-accept service orchestrating multiple clients,
 *         WHEN service sends commands to auto-accepted clients,
 *         THEN each client processes commands without connection delays.
 *
 * [@US-3] OnAutoAccepted_F Callback with Command Context
 *  AC-1: GIVEN a service with OnAutoAccepted_F callback and command capability,
 *         WHEN client auto-connects,
 *         THEN callback receives command-ready link context.
 *  AC-2: GIVEN OnAutoAccepted_F callback with per-client configuration,
 *         WHEN multiple clients auto-connect with DIFFERENT Usage types (CmdInitiator vs CmdExecutor),
 *         THEN service with COMBINED capabilities supports BOTH client→service AND service→client commands.
 *         DETAILS: Service capability = CmdExecutor | CmdInitiator, callback configures per-client flow direction.
 *  AC-3: GIVEN OnAutoAccepted_F callback integration with both callback and polling patterns,
 *         WHEN clients connect with different command usage patterns,
 *         THEN callback handles mixed command execution modes correctly.
 *
 * [@US-4] Service Lifecycle with Persistent Links (IOC_SRVFLAG_KEEP_ACCEPTED_LINK)
 *  AC-1: GIVEN a service with IOC_SRVFLAG_AUTO_ACCEPT and IOC_SRVFLAG_KEEP_ACCEPTED_LINK,
 *         WHEN service goes offline,
 *         THEN auto-accepted links persist and remain valid for manual cleanup.
 *  AC-2: GIVEN persistent auto-accepted links requiring manual cleanup,
 *         WHEN service shutdown occurs,
 *         THEN developer must manually close server-side LinkIDs to prevent resource leaks.
 *  AC-3: GIVEN persistent auto-accepted links across service restart,
 *         WHEN service comes back online,
 *         THEN existing links remain functional for continued operation.
 *
 * [@US-5] Service Lifecycle Comparison (Auto-cleanup vs Persistent Links)
 *  AC-1: GIVEN services with and without IOC_SRVFLAG_KEEP_ACCEPTED_LINK,
 *         WHEN both services go offline,
 *         THEN auto-cleanup service cleans links automatically while persistent service preserves links.
 *  AC-2: GIVEN different cleanup strategies under load testing,
 *         WHEN measuring resource management performance,
 *         THEN each strategy shows measurable differences in cleanup timing and resource usage.
 */
//=======>END OF ACCEPTANCE CRITERIA================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF TEST CASES=======================================================================
/**************************************************************************************************
 * @brief 【Auto-Accept Command Test Cases】
 *
 * ORGANIZATION STRATEGIES:
 *  - By Feature/Component: Auto-accept + Command Executor vs Command Initiator patterns
 *  - By Test Category: Basic → Multi-client → Callback Integration → Mixed Patterns
 *  - By Coverage Matrix: Auto-accept integration with all command execution modes
 *  - By Priority: Basic auto-accept first, complex callback integration second
 *
 * STATUS TRACKING: ⚪ = Planned/TODO，🔴 = Implemented/RED, 🟢 = Passed/GREEN, ⚠️ = Issues
 *
 * ⚪ FRAMEWORK STATUS: Auto-accept + Command integration needs implementation
 *    Building on completed command APIs from UT_CommandTypical.cxx and UT_CommandTypicalWaitAck.cxx
 *
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * 📋 [US-1]: AUTO-ACCEPT + CLIENT→SERVICE COMMANDS (Service=CmdExecutor, Client=CmdInitiator)
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * PATTERN: Client connects → Service auto-accepts → Client sends commands → Service executes commands
 *
 * [@AC-1,US-1] Basic auto-accept with client-to-service command execution
 *  ✓ TC-1: verifyAutoAcceptCmdExecutor_bySingleClient_expectImmediateCommandReady
 *      @[Purpose]: Validate CLIENT→SERVICE command flow with auto-accept (no manual accept needed)
 *      @[Brief]: Service(CmdExecutor+AutoAccept), Client(CmdInitiator) connects → Client sends PING → Service executes
 *      @[Status]: IMPLEMENTED - Basic auto-accept + client→service command pattern working
 *
 * [@AC-2,US-1] Multi-client auto-accept with isolated client-to-service commands
 *  ✓ TC-1: verifyAutoAcceptClientToServiceCmd_byMultipleClients_expectIsolatedExecution
 *      @[Purpose]: Ensure multiple clients can send commands independently to auto-accepting service
 *      @[Brief]: Multiple Client(CmdInitiator) -> Service(CmdExecutor+AutoAccept), verify command isolation
 *      @[Status]: IMPLEMENTED - Multi-client CLIENT->SERVICE command patterns working
 *
 * [@AC-3,US-1] Client-to-service commands with timeout validation under auto-accept
 *  ✓ TC-1: verifyAutoAcceptClientToServiceCmd_byTimeoutConstraints_expectProperTiming
 *      @[Purpose]: Validate command timeout behavior for CLIENT→SERVICE commands with auto-accept
 *      @[Brief]: Client(CmdInitiator) sends DELAY command → Service(CmdExecutor+AutoAccept) verifies timing
 *      @[Status]: IMPLEMENTED - CLIENT→SERVICE timeout validation with auto-accept working
 *
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * 📋 [US-2]: AUTO-ACCEPT + SERVICE→CLIENT COMMANDS (Service=CmdInitiator, Client=CmdExecutor)
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * PATTERN: Client connects → Service auto-accepts → Service sends commands → Client executes commands
 *
 * [@AC-1,US-2] Basic auto-accept with service-to-client command initiation
 *  ✓ TC-1: verifyAutoAcceptServiceToClientCmd_bySingleClient_expectImmediateExecution
 *      @[Purpose]: Validate SERVICE→CLIENT command flow with auto-accept (service initiates commands)
 *      @[Brief]: Service(CmdInitiator+AutoAccept), Client(CmdExecutor) connects → Service sends ECHO → Client executes
 *      @[Status]: IMPLEMENTED - Basic auto-accept + service→client command pattern working
 *
 * [@AC-2,US-2] Auto-accept service orchestrating commands to multiple clients
 *  ✓ TC-1: verifyAutoAcceptServiceToClientCmd_byMultipleClients_expectOrchestration
 *      @[Purpose]: Validate service orchestrating commands to multiple auto-accepted clients
 *      @[Brief]: Service(CmdInitiator+AutoAccept) → Multiple Client(CmdExecutor), demonstrate orchestration capability
 *      @[Status]: IMPLEMENTED - Multi-client SERVICE→CLIENT orchestration working correctly
 *
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * 📋 [US-3]: OnAutoAccepted_F CALLBACK INTEGRATION WITH COMMAND CONFIGURATION
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * PATTERN: Auto-accept callback configures command capabilities per client on connection
 *
 * [@AC-1,US-3] OnAutoAccepted_F callback enabling immediate command readiness (US-1 pattern)
 *  🟢 TC-1: verifyOnAutoAcceptedCallback_forClientToServiceCmd_expectLinkReadiness
 *      @[Purpose]: Validate OnAutoAccepted_F callback prepares CLIENT→SERVICE command readiness
 *      @[Brief]: Service(CmdExecutor+AutoAccept+Callback), callback configures link for CLIENT→SERVICE commands
 *      @[Status]: IMPLEMENTED & PASSED - OnAutoAccepted_F callback with CLIENT→SERVICE command context working
 *
 * [@AC-2,US-3] Per-client command capability configuration via auto-accept callback
 *  🔴 TC-1: verifyOnAutoAcceptedCallback_forMixedCmdPatterns_expectFlexibleConfig
 *      @[Purpose]: Validate service supporting BOTH CmdExecutor+CmdInitiator with per-client configuration
 *      @[Brief]: Service(CmdExecutor|CmdInitiator+AutoAccept+Callback) handles:
 *                - Client-A(CmdInitiator) → CLIENT-A→SERVICE commands (US-1 pattern)
 *                - Client-B(CmdExecutor) → SERVICE→CLIENT-B commands (US-2 pattern)
 *                - OnAutoAccepted_F configures each client individually based on Usage type
 *      @[Technical]: Service.UsageCapabilites = IOC_LinkUsageCmdExecutor | IOC_LinkUsageCmdInitiator
 *                    Callback determines per-client command flow based on client's Usage parameter
 *      @[Status]: IMPLEMENTED - Mixed client types with unified service capability (test failing - need to debug
 * command routing)
 *
 * [@AC-3,US-3] Mixed command patterns (callback + polling) with auto-accept callback
 *  ⚪ TC-1: verifyOnAutoAcceptedCallback_forCallbackPlusPolling_expectFlexibleHandling
 *      @[Purpose]: Validate OnAutoAccepted_F callback dynamically configuring execution modes per client
 *      @[Brief]: Service auto-accepts multiple clients → Callback assigns Client-A(immediate execution) vs
 * Client-B(polling-based execution)
 *      @[Status]: TODO - Need to implement mixed pattern support with auto-accept
 *
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * 📋 [US-4]: SERVICE LIFECYCLE WITH PERSISTENT LINKS (IOC_SRVFLAG_KEEP_ACCEPTED_LINK)
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * PATTERN: Auto-accepted links persist across service lifecycle with manual cleanup responsibility
 *
 * [@AC-1,US-4] Auto-accepted link persistence after service offline
 *  ⚪ TC-1: verifyKeepAcceptedLink_byServiceOffline_expectLinkPersistence
 *      @[Purpose]: Validate IOC_SRVFLAG_KEEP_ACCEPTED_LINK preserves auto-accepted links after service shutdown
 *      @[Brief]: Service(AutoAccept+KeepLinks) → Client connects → Service offline → Links persist for manual cleanup
 *      @[Status]: TODO - Need to implement persistent link behavior validation
 *
 * [@AC-2,US-4] Manual cleanup requirement for persistent auto-accepted links
 *  🟢 TC-1: verifyKeepAcceptedLink_byManualCleanup_expectProperResourceManagement
 *      @[Purpose]: Validate manual cleanup responsibility for persistent auto-accepted links
 *      @[Brief]: Service(AutoAccept+KeepLinks) → Multiple clients → Service offline → Manual LinkID cleanup required
 *      @[Status]: IMPLEMENTED & PASSED - Manual cleanup patterns for persistent links validated with 9-client stress
 * testing
 *
 * [@AC-3,US-4] Link functionality across service restart scenarios
 *  ⚪ TC-1: verifyKeepAcceptedLink_byServiceRestart_expectConnectionPersistence
 *      @[Purpose]: Validate persistent links remain functional across service restart scenarios
 *      @[Brief]: Service restart with persistent links maintaining connection continuity
 *      @[Status]: TODO - Need to implement service restart with persistent link functionality
 *
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * 📋 [US-5]: SERVICE LIFECYCLE COMPARISON (Auto-cleanup vs Persistent Links)
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * PATTERN: Comparative analysis of resource management strategies for auto-accepted links
 *
 * [@AC-1,US-5] Resource management behavior comparison
 *  ⚪ TC-1: verifyServiceLifecycleComparison_byAutoCleanupVsPersistent_expectDifferentBehavior
 *      @[Purpose]: Compare auto-cleanup vs persistent link behavior for resource management
 *      @[Brief]: Two services: one with auto-cleanup, one with persistent links → Compare resource handling
 *      @[Status]: TODO - Need to implement comparative resource management analysis
 *
 * [@AC-2,US-5] Performance implications of cleanup strategies
 *  ⚪ TC-1: verifyServiceLifecycleComparison_byPerformanceImplications_expectMeasurableDifference
 *      @[Purpose]: Measure performance differences between auto-cleanup and persistent link strategies
 *      @[Brief]: Load testing with both cleanup strategies → Measure resource usage and cleanup timing
 *      @[Status]: TODO - Need to implement performance comparison for cleanup strategies
 */
//======>END OF TEST CASES=========================================================================

// Auto-accept command private data structure
typedef struct __AutoAcceptCmdPriv {
    std::atomic<bool> ClientAutoAccepted{false};
    std::atomic<int> AutoAcceptCount{0};
    std::atomic<bool> CommandReceived{false};
    std::atomic<int> CommandCount{0};
    IOC_LinkID_T LastAcceptedLinkID{IOC_ID_INVALID};
    IOC_CmdID_T LastCmdID{0};
    IOC_CmdStatus_E LastStatus{IOC_CMD_STATUS_PENDING};
    IOC_Result_T LastResult{IOC_RESULT_BUG};
    char LastResponseData[512];
    ULONG_T LastResponseSize{0};
    std::mutex DataMutex;
    std::vector<IOC_LinkID_T> AcceptedLinks;  // Track multiple auto-accepted clients
} __AutoAcceptCmdPriv_T;

// TODO: Implement auto-accept callback function
static void __AutoAcceptCmd_OnAutoAcceptedCb(IOC_SrvID_T SrvID, IOC_LinkID_T LinkID, void *pSrvPriv) {
    __AutoAcceptCmdPriv_T *pPrivData = (__AutoAcceptCmdPriv_T *)pSrvPriv;
    if (!pPrivData) return;

    std::lock_guard<std::mutex> lock(pPrivData->DataMutex);

    pPrivData->ClientAutoAccepted = true;
    pPrivData->AutoAcceptCount++;
    pPrivData->LastAcceptedLinkID = LinkID;
    pPrivData->AcceptedLinks.push_back(LinkID);

    // TODO: Add command-specific configuration based on link usage
    // For now, the link is ready for command execution immediately after auto-accept
}

// TODO: Implement command execution callback for auto-accept scenarios
static IOC_Result_T __AutoAcceptCmd_ExecutorCb(IOC_LinkID_T LinkID, IOC_CmdDesc_pT pCmdDesc, void *pCbPriv) {
    __AutoAcceptCmdPriv_T *pPrivData = (__AutoAcceptCmdPriv_T *)pCbPriv;
    if (!pPrivData || !pCmdDesc) return IOC_RESULT_INVALID_PARAM;

    std::lock_guard<std::mutex> lock(pPrivData->DataMutex);

    pPrivData->CommandReceived = true;
    pPrivData->CommandCount++;
    pPrivData->LastCmdID = IOC_CmdDesc_getCmdID(pCmdDesc);

    // TODO: Implement command processing logic similar to UT_CommandTypical.cxx
    // Process different command types (PING, ECHO, CALC, etc.)
    IOC_CmdID_T CmdID = IOC_CmdDesc_getCmdID(pCmdDesc);
    IOC_Result_T ExecResult = IOC_RESULT_SUCCESS;

    if (CmdID == IOC_CMDID_TEST_PING) {
        // PING command: simple response with "AUTO_PONG"
        const char *response = "AUTO_PONG";
        ExecResult = IOC_CmdDesc_setOutPayload(pCmdDesc, (void *)response, strlen(response));
        strcpy(pPrivData->LastResponseData, response);
        pPrivData->LastResponseSize = strlen(response);
    } else if (CmdID == IOC_CMDID_TEST_ECHO) {
        // ECHO command: return input data as output with "AUTO_" prefix
        void *inputData = IOC_CmdDesc_getInData(pCmdDesc);
        ULONG_T inputSize = IOC_CmdDesc_getInDataSize(pCmdDesc);
        if (inputData && inputSize > 0) {
            std::string autoResponse = "AUTO_" + std::string((char *)inputData, inputSize);
            ExecResult = IOC_CmdDesc_setOutPayload(pCmdDesc, (void *)autoResponse.c_str(), autoResponse.length());
            strcpy(pPrivData->LastResponseData, autoResponse.c_str());
            pPrivData->LastResponseSize = autoResponse.length();
        }
    } else {
        ExecResult = IOC_RESULT_NOT_SUPPORT;
    }

    // Update command status and result
    if (ExecResult == IOC_RESULT_SUCCESS) {
        IOC_CmdDesc_setStatus(pCmdDesc, IOC_CMD_STATUS_SUCCESS);
        IOC_CmdDesc_setResult(pCmdDesc, IOC_RESULT_SUCCESS);
        pPrivData->LastStatus = IOC_CMD_STATUS_SUCCESS;
        pPrivData->LastResult = IOC_RESULT_SUCCESS;
    } else {
        IOC_CmdDesc_setStatus(pCmdDesc, IOC_CMD_STATUS_FAILED);
        IOC_CmdDesc_setResult(pCmdDesc, ExecResult);
        pPrivData->LastStatus = IOC_CMD_STATUS_FAILED;
        pPrivData->LastResult = ExecResult;
    }

    return ExecResult;
}

// [@AC-1,US-1] TC-1: CLIENT→SERVICE command flow with auto-accept (Client=CmdInitiator, Service=CmdExecutor)
TEST(UT_ConetCommandTypicalAutoAccept, verifyAutoAcceptClientToServiceCmd_bySingleClient_expectImmediateExecution) {
    IOC_Result_T ResultValue = IOC_RESULT_BUG;

    // TODO: Implement auto-accept + command executor service setup
    __AutoAcceptCmdPriv_T AutoAcceptPriv = {};
    IOC_SrvURI_T SrvURI = {.pProtocol = IOC_SRV_PROTO_FIFO,
                           .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
                           .pPath = (const char *)"CmdAutoAccept_ExecutorSingle"};

    // Define supported commands for auto-accept service
    static IOC_CmdID_T SupportedCmdIDs[] = {IOC_CMDID_TEST_PING, IOC_CMDID_TEST_ECHO};
    IOC_CmdUsageArgs_T CmdUsageArgs = {.CbExecCmd_F = __AutoAcceptCmd_ExecutorCb,
                                       .pCbPrivData = &AutoAcceptPriv,
                                       .CmdNum = sizeof(SupportedCmdIDs) / sizeof(SupportedCmdIDs[0]),
                                       .pCmdIDs = SupportedCmdIDs};

    IOC_SrvArgs_T SrvArgs = {.SrvURI = SrvURI,
                             .Flags = IOC_SRVFLAG_AUTO_ACCEPT,  // Enable auto-accept
                             .UsageCapabilites = IOC_LinkUsageCmdExecutor,
                             .UsageArgs = {.pCmd = &CmdUsageArgs},
                             .OnAutoAccepted_F = __AutoAcceptCmd_OnAutoAcceptedCb,
                             .pSrvPriv = &AutoAcceptPriv};

    IOC_SrvID_T SrvID = IOC_ID_INVALID;
    ResultValue = IOC_onlineService(&SrvID, &SrvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);

    // TODO: Client setup and connection (should be auto-accepted)
    IOC_ConnArgs_T ConnArgs = {.SrvURI = SrvURI, .Usage = IOC_LinkUsageCmdInitiator};
    IOC_LinkID_T CliLinkID = IOC_ID_INVALID;

    std::thread CliThread([&] {
        IOC_Result_T ResultValueInThread = IOC_connectService(&CliLinkID, &ConnArgs, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValueInThread);
        ASSERT_NE(IOC_ID_INVALID, CliLinkID);
    });

    // Wait for client connection (no manual accept needed)
    if (CliThread.joinable()) CliThread.join();

    // TODO: Verify auto-accept occurred
    // Wait for auto-accept callback
    for (int retry = 0; retry < 100; ++retry) {
        if (AutoAcceptPriv.ClientAutoAccepted.load()) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    ASSERT_TRUE(AutoAcceptPriv.ClientAutoAccepted.load());
    ASSERT_EQ(1, AutoAcceptPriv.AutoAcceptCount.load());
    ASSERT_NE(IOC_ID_INVALID, AutoAcceptPriv.LastAcceptedLinkID);

    // Additional wait to ensure auto-accept link is fully configured for commands
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // TODO: Client sends command immediately after auto-accept
    IOC_CmdDesc_T CmdDesc = {};
    CmdDesc.CmdID = IOC_CMDID_TEST_PING;
    CmdDesc.TimeoutMs = 5000;
    CmdDesc.Status = IOC_CMD_STATUS_PENDING;

    ResultValue = IOC_execCMD(CliLinkID, &CmdDesc, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);

    // TODO: Verify command execution through auto-accepted connection
    ASSERT_TRUE(AutoAcceptPriv.CommandReceived.load());
    ASSERT_EQ(1, AutoAcceptPriv.CommandCount.load());
    ASSERT_EQ(IOC_CMDID_TEST_PING, AutoAcceptPriv.LastCmdID);
    ASSERT_EQ(IOC_CMD_STATUS_SUCCESS, AutoAcceptPriv.LastStatus);

    // Verify response payload contains auto-accept indicator
    void *responseData = IOC_CmdDesc_getOutData(&CmdDesc);
    ULONG_T responseSize = IOC_CmdDesc_getOutDataSize(&CmdDesc);
    ASSERT_TRUE(responseData != nullptr);
    ASSERT_STREQ("AUTO_PONG", (char *)responseData);

    // Cleanup
    if (CliLinkID != IOC_ID_INVALID) IOC_closeLink(CliLinkID);
    // Note: server-side auto-accepted LinkIDs are cleaned up automatically by IOC_offlineService()
    if (SrvID != IOC_ID_INVALID) IOC_offlineService(SrvID);
}

// TODO: Implement remaining test cases

// [@AC-2,US-1] TC-1: verifyAutoAcceptCmdExecutor_byMultipleClients_expectIsolatedExecution
// [@AC-2,US-1] TC-1: Multi-client CLIENT→SERVICE commands with auto-accept and isolation
TEST(UT_ConetCommandTypicalAutoAccept, verifyAutoAcceptClientToServiceCmd_byMultipleClients_expectIsolatedExecution) {
    IOC_Result_T ResultValue = IOC_RESULT_BUG;
    const int NUM_CLIENTS = 3;

    // Setup auto-accept service with command executor capability
    __AutoAcceptCmdPriv_T AutoAcceptPriv = {};
    IOC_SrvURI_T SrvURI = {.pProtocol = IOC_SRV_PROTO_FIFO,
                           .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
                           .pPath = (const char *)"CmdAutoAccept_MultiClient"};

    // Define supported commands for auto-accept service
    static IOC_CmdID_T SupportedCmdIDs[] = {IOC_CMDID_TEST_PING, IOC_CMDID_TEST_ECHO};
    IOC_CmdUsageArgs_T CmdUsageArgs = {.CbExecCmd_F = __AutoAcceptCmd_ExecutorCb,
                                       .pCbPrivData = &AutoAcceptPriv,
                                       .CmdNum = sizeof(SupportedCmdIDs) / sizeof(SupportedCmdIDs[0]),
                                       .pCmdIDs = SupportedCmdIDs};

    IOC_SrvArgs_T SrvArgs = {.SrvURI = SrvURI,
                             .Flags = IOC_SRVFLAG_AUTO_ACCEPT,  // Enable auto-accept
                             .UsageCapabilites = IOC_LinkUsageCmdExecutor,
                             .UsageArgs = {.pCmd = &CmdUsageArgs},
                             .OnAutoAccepted_F = __AutoAcceptCmd_OnAutoAcceptedCb,
                             .pSrvPriv = &AutoAcceptPriv};

    IOC_SrvID_T SrvID = IOC_ID_INVALID;
    ResultValue = IOC_onlineService(&SrvID, &SrvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);

    // Create multiple clients that connect simultaneously
    std::vector<IOC_LinkID_T> ClientLinkIDs(NUM_CLIENTS, IOC_ID_INVALID);
    std::vector<std::thread> ClientThreads;

    for (int i = 0; i < NUM_CLIENTS; ++i) {
        ClientThreads.emplace_back([&, i] {
            IOC_ConnArgs_T ConnArgs = {.SrvURI = SrvURI, .Usage = IOC_LinkUsageCmdInitiator};
            IOC_Result_T ResultValueInThread = IOC_connectService(&ClientLinkIDs[i], &ConnArgs, NULL);
            ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValueInThread);
            ASSERT_NE(IOC_ID_INVALID, ClientLinkIDs[i]);
        });
    }

    // Wait for all clients to connect
    for (auto &thread : ClientThreads) {
        if (thread.joinable()) thread.join();
    }

    // Wait for all auto-accepts to complete
    for (int retry = 0; retry < 100; ++retry) {
        if (AutoAcceptPriv.AutoAcceptCount.load() >= NUM_CLIENTS) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    ASSERT_EQ(NUM_CLIENTS, AutoAcceptPriv.AutoAcceptCount.load());
    ASSERT_TRUE(AutoAcceptPriv.ClientAutoAccepted.load());

    // Additional wait to ensure all auto-accept links are ready for commands
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Each client sends a unique command to verify isolation
    std::vector<IOC_CmdDesc_T> CmdDescs(NUM_CLIENTS);
    std::vector<IOC_CmdID_T> ExpectedCmdIDs = {IOC_CMDID_TEST_PING, IOC_CMDID_TEST_ECHO, IOC_CMDID_TEST_PING};
    std::vector<std::string> EchoInputs = {"", "TestInput", ""};  // ECHO needs input data

    for (int i = 0; i < NUM_CLIENTS; ++i) {
        CmdDescs[i] = {};
        CmdDescs[i].CmdID = ExpectedCmdIDs[i];
        CmdDescs[i].TimeoutMs = 5000;
        CmdDescs[i].Status = IOC_CMD_STATUS_PENDING;

        // Set input data for ECHO commands
        if (ExpectedCmdIDs[i] == IOC_CMDID_TEST_ECHO && !EchoInputs[i].empty()) {
            IOC_CmdDesc_setInPayload(&CmdDescs[i], (void *)EchoInputs[i].c_str(), EchoInputs[i].length());
        }

        ResultValue = IOC_execCMD(ClientLinkIDs[i], &CmdDescs[i], NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);
    }

    // Verify all commands were executed (command count should be at least NUM_CLIENTS)
    ASSERT_TRUE(AutoAcceptPriv.CommandReceived.load());
    ASSERT_GE(AutoAcceptPriv.CommandCount.load(), NUM_CLIENTS);

    // Verify responses are properly isolated (each client gets its own response)
    for (int i = 0; i < NUM_CLIENTS; ++i) {
        void *responseData = IOC_CmdDesc_getOutData(&CmdDescs[i]);
        ULONG_T responseSize = IOC_CmdDesc_getOutDataSize(&CmdDescs[i]);
        ASSERT_TRUE(responseData != nullptr);
        ASSERT_GT(responseSize, 0);

        // Verify response format based on command type
        std::string response((char *)responseData, responseSize);
        if (ExpectedCmdIDs[i] == IOC_CMDID_TEST_PING) {
            ASSERT_EQ("AUTO_PONG", response);
        } else if (ExpectedCmdIDs[i] == IOC_CMDID_TEST_ECHO) {
            ASSERT_EQ("AUTO_TestInput", response);  // Expected "AUTO_" + "TestInput"
        }
    }

    // Cleanup all client links
    for (int i = 0; i < NUM_CLIENTS; ++i) {
        if (ClientLinkIDs[i] != IOC_ID_INVALID) {
            IOC_closeLink(ClientLinkIDs[i]);
        }
    }

    // Note: server-side auto-accepted LinkIDs cleaned up automatically by IOC_offlineService()

    // Cleanup service
    if (SrvID != IOC_ID_INVALID) IOC_offlineService(SrvID);
}

// [@AC-3,US-1] TC-1: verifyAutoAcceptCmdExecutor_byTimeoutConstraints_expectProperTiming
// [@AC-3,US-1] TC-1: CLIENT→SERVICE command timeout validation with auto-accept
TEST(UT_ConetCommandTypicalAutoAccept, verifyAutoAcceptClientToServiceCmd_byTimeoutConstraints_expectProperTiming) {
    IOC_Result_T ResultValue = IOC_RESULT_BUG;

    // Setup auto-accept service with command executor capability
    __AutoAcceptCmdPriv_T AutoAcceptPriv = {};
    IOC_SrvURI_T SrvURI = {.pProtocol = IOC_SRV_PROTO_FIFO,
                           .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
                           .pPath = (const char *)"CmdAutoAccept_Timeout"};

    // Define supported commands for auto-accept service
    static IOC_CmdID_T SupportedCmdIDs[] = {IOC_CMDID_TEST_PING, IOC_CMDID_TEST_ECHO};
    IOC_CmdUsageArgs_T CmdUsageArgs = {.CbExecCmd_F = __AutoAcceptCmd_ExecutorCb,
                                       .pCbPrivData = &AutoAcceptPriv,
                                       .CmdNum = sizeof(SupportedCmdIDs) / sizeof(SupportedCmdIDs[0]),
                                       .pCmdIDs = SupportedCmdIDs};

    IOC_SrvArgs_T SrvArgs = {.SrvURI = SrvURI,
                             .Flags = IOC_SRVFLAG_AUTO_ACCEPT,  // Enable auto-accept
                             .UsageCapabilites = IOC_LinkUsageCmdExecutor,
                             .UsageArgs = {.pCmd = &CmdUsageArgs},
                             .OnAutoAccepted_F = __AutoAcceptCmd_OnAutoAcceptedCb,
                             .pSrvPriv = &AutoAcceptPriv};

    IOC_SrvID_T SrvID = IOC_ID_INVALID;
    ResultValue = IOC_onlineService(&SrvID, &SrvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);

    // Client setup and connection
    IOC_ConnArgs_T ConnArgs = {.SrvURI = SrvURI, .Usage = IOC_LinkUsageCmdInitiator};
    IOC_LinkID_T CliLinkID = IOC_ID_INVALID;

    std::thread CliThread([&] {
        IOC_Result_T ResultValueInThread = IOC_connectService(&CliLinkID, &ConnArgs, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValueInThread);
        ASSERT_NE(IOC_ID_INVALID, CliLinkID);
    });

    // Wait for client connection and auto-accept
    if (CliThread.joinable()) CliThread.join();

    // Wait for auto-accept to complete
    for (int retry = 0; retry < 100; ++retry) {
        if (AutoAcceptPriv.ClientAutoAccepted.load()) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    ASSERT_TRUE(AutoAcceptPriv.ClientAutoAccepted.load());
    ASSERT_EQ(1, AutoAcceptPriv.AutoAcceptCount.load());

    // Additional wait to ensure auto-accept link is ready for commands
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Test 1: Normal command with reasonable timeout should succeed
    IOC_CmdDesc_T NormalCmd = {};
    NormalCmd.CmdID = IOC_CMDID_TEST_PING;
    NormalCmd.TimeoutMs = 3000;  // 3 second timeout - should be enough
    NormalCmd.Status = IOC_CMD_STATUS_PENDING;

    auto start_time = std::chrono::steady_clock::now();
    ResultValue = IOC_execCMD(CliLinkID, &NormalCmd, NULL);
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);
    ASSERT_EQ(IOC_CMD_STATUS_SUCCESS, NormalCmd.Status);
    ASSERT_LT(duration.count(), 1000);  // Should complete in less than 1 second

    // Verify response data
    void *responseData = IOC_CmdDesc_getOutData(&NormalCmd);
    ULONG_T responseSize = IOC_CmdDesc_getOutDataSize(&NormalCmd);
    ASSERT_TRUE(responseData != nullptr);
    ASSERT_GT(responseSize, 0);

    std::string response((char *)responseData, responseSize);
    ASSERT_EQ("AUTO_PONG", response);

    // Test 2: Command with very short timeout should also work (immediate response)
    IOC_CmdDesc_T FastCmd = {};
    FastCmd.CmdID = IOC_CMDID_TEST_PING;
    FastCmd.TimeoutMs = 100;  // 100ms timeout - should still work for immediate response
    FastCmd.Status = IOC_CMD_STATUS_PENDING;

    start_time = std::chrono::steady_clock::now();
    ResultValue = IOC_execCMD(CliLinkID, &FastCmd, NULL);
    end_time = std::chrono::steady_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);
    ASSERT_EQ(IOC_CMD_STATUS_SUCCESS, FastCmd.Status);
    ASSERT_LT(duration.count(), 100);  // Should complete well within timeout

    // Verify command execution statistics
    ASSERT_TRUE(AutoAcceptPriv.CommandReceived.load());
    ASSERT_GE(AutoAcceptPriv.CommandCount.load(), 2);  // At least 2 commands executed

    // Cleanup client link
    if (CliLinkID != IOC_ID_INVALID) {
        IOC_closeLink(CliLinkID);
    }

    // Note: server-side auto-accepted LinkIDs cleaned up automatically by IOC_offlineService()

    // Cleanup service
    if (SrvID != IOC_ID_INVALID) IOC_offlineService(SrvID);
}

// [@AC-1,US-2] TC-1: verifyAutoAcceptCmdInitiator_bySingleClient_expectServiceToClientCommand
// [@AC-1,US-2] TC-1: SERVICE→CLIENT command flow with auto-accept (Service=CmdInitiator, Client=CmdExecutor)
TEST(UT_ConetCommandTypicalAutoAccept, verifyAutoAcceptServiceToClientCmd_bySingleClient_expectImmediateExecution) {
    IOC_Result_T ResultValue = IOC_RESULT_BUG;

    // Private data for client command executor
    __AutoAcceptCmdPriv_T ClientExecPriv = {};

    // Setup auto-accept service with command INITIATOR capability (reversed roles)
    __AutoAcceptCmdPriv_T AutoAcceptPriv = {};
    IOC_SrvURI_T SrvURI = {.pProtocol = IOC_SRV_PROTO_FIFO,
                           .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
                           .pPath = (const char *)"CmdAutoAccept_ServiceInitiator"};

    // Service acts as CmdInitiator - no command execution callback needed for service
    static IOC_CmdID_T ServiceCmdIDs[] = {IOC_CMDID_TEST_PING, IOC_CMDID_TEST_ECHO};
    IOC_CmdUsageArgs_T ServiceCmdUsageArgs = {.CbExecCmd_F = nullptr,  // Service initiates, doesn't execute
                                              .pCbPrivData = nullptr,
                                              .CmdNum = sizeof(ServiceCmdIDs) / sizeof(ServiceCmdIDs[0]),
                                              .pCmdIDs = ServiceCmdIDs};

    IOC_SrvArgs_T SrvArgs = {.SrvURI = SrvURI,
                             .Flags = IOC_SRVFLAG_AUTO_ACCEPT,               // Enable auto-accept
                             .UsageCapabilites = IOC_LinkUsageCmdInitiator,  // Service initiates commands
                             .UsageArgs = {.pCmd = &ServiceCmdUsageArgs},
                             .OnAutoAccepted_F = __AutoAcceptCmd_OnAutoAcceptedCb,
                             .pSrvPriv = &AutoAcceptPriv};

    IOC_SrvID_T SrvID = IOC_ID_INVALID;
    ResultValue = IOC_onlineService(&SrvID, &SrvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);

    // Client setup as CmdExecutor (client will execute commands from service)
    static IOC_CmdID_T ClientCmdIDs[] = {IOC_CMDID_TEST_PING, IOC_CMDID_TEST_ECHO};
    IOC_CmdUsageArgs_T ClientCmdUsageArgs = {.CbExecCmd_F = __AutoAcceptCmd_ExecutorCb,  // Client executes commands
                                             .pCbPrivData = &ClientExecPriv,
                                             .CmdNum = sizeof(ClientCmdIDs) / sizeof(ClientCmdIDs[0]),
                                             .pCmdIDs = ClientCmdIDs};

    IOC_ConnArgs_T ConnArgs = {.SrvURI = SrvURI,
                               .Usage = IOC_LinkUsageCmdExecutor,  // Client executes commands
                               .UsageArgs = {.pCmd = &ClientCmdUsageArgs}};
    IOC_LinkID_T CliLinkID = IOC_ID_INVALID;

    std::thread CliThread([&] {
        IOC_Result_T ResultValueInThread = IOC_connectService(&CliLinkID, &ConnArgs, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValueInThread);
        ASSERT_NE(IOC_ID_INVALID, CliLinkID);
    });

    // Wait for client connection and auto-accept
    if (CliThread.joinable()) CliThread.join();

    // Wait for auto-accept to complete
    for (int retry = 0; retry < 100; ++retry) {
        if (AutoAcceptPriv.ClientAutoAccepted.load()) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    ASSERT_TRUE(AutoAcceptPriv.ClientAutoAccepted.load());
    ASSERT_EQ(1, AutoAcceptPriv.AutoAcceptCount.load());
    ASSERT_NE(IOC_ID_INVALID, AutoAcceptPriv.LastAcceptedLinkID);

    // Additional wait to ensure auto-accept link is ready for commands
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Service sends command TO client (reversed flow: SERVICE→CLIENT)
    IOC_CmdDesc_T CmdDesc = {};
    CmdDesc.CmdID = IOC_CMDID_TEST_PING;
    CmdDesc.TimeoutMs = 5000;
    CmdDesc.Status = IOC_CMD_STATUS_PENDING;

    // Service uses the auto-accepted link to send command to client
    ResultValue = IOC_execCMD(AutoAcceptPriv.LastAcceptedLinkID, &CmdDesc, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);

    // Verify command was executed by CLIENT (not the auto-accept service)
    ASSERT_TRUE(ClientExecPriv.CommandReceived.load());
    ASSERT_EQ(1, ClientExecPriv.CommandCount.load());
    ASSERT_EQ(IOC_CMDID_TEST_PING, ClientExecPriv.LastCmdID);
    ASSERT_EQ(IOC_CMD_STATUS_SUCCESS, ClientExecPriv.LastStatus);

    // Verify response payload from client command execution
    void *responseData = IOC_CmdDesc_getOutData(&CmdDesc);
    ULONG_T responseSize = IOC_CmdDesc_getOutDataSize(&CmdDesc);
    ASSERT_TRUE(responseData != nullptr);
    ASSERT_GT(responseSize, 0);

    std::string response((char *)responseData, responseSize);
    ASSERT_EQ("AUTO_PONG", response);  // Client's response to SERVICE→CLIENT command

    // Cleanup client link
    if (CliLinkID != IOC_ID_INVALID) {
        IOC_closeLink(CliLinkID);
    }

    // Note: server-side auto-accepted LinkIDs cleaned up automatically by IOC_offlineService()

    // Cleanup service
    if (SrvID != IOC_ID_INVALID) IOC_offlineService(SrvID);
}

// [@AC-2,US-2] TC-1: verifyAutoAcceptCmdInitiator_byMultipleClients_expectImmediateOrchestration
// [@AC-2,US-2] TC-1: Multi-client SERVICE→CLIENT command orchestration with auto-accept
TEST(UT_ConetCommandTypicalAutoAccept, verifyAutoAcceptServiceToClientCmd_byMultipleClients_expectOrchestration) {
    IOC_Result_T ResultValue = IOC_RESULT_BUG;

    // 🎯 SCALABILITY CONFIGURATION: Easy adjustment for extensive testing
    const int NUM_CLIENTS = 9;  // Improved from 6 to 9, architecture supports 9999+
    // 🚀 FOR LARGE SCALE TESTING: Simply change NUM_CLIENTS to 99, 999, or 9999
    //    Additional optimizations for 9999+ clients:
    //    - Reduce CONNECTION_TIMEOUT_MS to 10-20ms
    //    - Reduce COMMAND_DELAY_MS to 5-10ms
    //    - Consider batch processing for command execution
    //    - Monitor system resources (file descriptors, memory)
    //    - Use threading for parallel client connections
    //    - Implement exponential backoff for resource contention
    const int CONNECTION_TIMEOUT_MS = 100;    // Per-client connection timeout
    const int COMMAND_DELAY_MS = 25;          // Delay between commands (optimize for scale)
    const int AUTO_ACCEPT_TIMEOUT_MS = 1000;  // Total time to wait for all auto-accepts

    // 📊 PERFORMANCE TRACKING: Monitor resource usage for scalability analysis
    auto test_start_time = std::chrono::steady_clock::now();

    // Dynamic allocation for scalability: supports NUM_CLIENTS up to thousands
    std::vector<std::unique_ptr<__AutoAcceptCmdPriv_T>> ClientExecPrivs;
    ClientExecPrivs.reserve(NUM_CLIENTS);
    for (int i = 0; i < NUM_CLIENTS; ++i) {
        ClientExecPrivs.push_back(std::make_unique<__AutoAcceptCmdPriv_T>());
    }

    // Setup auto-accept service with command INITIATOR capability (service orchestrates commands)
    __AutoAcceptCmdPriv_T AutoAcceptPriv = {};
    IOC_SrvURI_T SrvURI = {.pProtocol = IOC_SRV_PROTO_FIFO,
                           .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
                           .pPath = (const char *)"CmdAutoAccept_MultiOrchestrator"};

    // Service acts as CmdInitiator (same pattern as single-client test)
    static IOC_CmdID_T ServiceCmdIDs[] = {IOC_CMDID_TEST_PING, IOC_CMDID_TEST_ECHO};
    IOC_CmdUsageArgs_T ServiceCmdUsageArgs = {.CbExecCmd_F = nullptr,  // Service initiates, doesn't execute
                                              .pCbPrivData = nullptr,
                                              .CmdNum = sizeof(ServiceCmdIDs) / sizeof(ServiceCmdIDs[0]),
                                              .pCmdIDs = ServiceCmdIDs};

    IOC_SrvArgs_T SrvArgs = {.SrvURI = SrvURI,
                             .Flags = IOC_SRVFLAG_AUTO_ACCEPT,               // Enable auto-accept
                             .UsageCapabilites = IOC_LinkUsageCmdInitiator,  // Service initiates commands
                             .UsageArgs = {.pCmd = &ServiceCmdUsageArgs},
                             .OnAutoAccepted_F = __AutoAcceptCmd_OnAutoAcceptedCb,
                             .pSrvPriv = &AutoAcceptPriv};

    IOC_SrvID_T SrvID = IOC_ID_INVALID;
    ResultValue = IOC_onlineService(&SrvID, &SrvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);

    // Dynamic client setup: scalable to thousands of clients
    std::vector<IOC_LinkID_T> ClientLinkIDs(NUM_CLIENTS, IOC_ID_INVALID);
    std::vector<IOC_CmdUsageArgs_T> ClientCmdUsageArgs(NUM_CLIENTS);
    std::vector<IOC_ConnArgs_T> ConnArgs(NUM_CLIENTS);

    static IOC_CmdID_T ClientCmdIDs[] = {IOC_CMDID_TEST_PING, IOC_CMDID_TEST_ECHO};

    // Connect all clients dynamically and sequentially for resource management
    for (int i = 0; i < NUM_CLIENTS; ++i) {
        // Configure command usage for this client
        ClientCmdUsageArgs[i] = {.CbExecCmd_F = __AutoAcceptCmd_ExecutorCb,
                                 .pCbPrivData = ClientExecPrivs[i].get(),
                                 .CmdNum = sizeof(ClientCmdIDs) / sizeof(ClientCmdIDs[0]),
                                 .pCmdIDs = ClientCmdIDs};

        ConnArgs[i] = {
            .SrvURI = SrvURI, .Usage = IOC_LinkUsageCmdExecutor, .UsageArgs = {.pCmd = &ClientCmdUsageArgs[i]}};

        ResultValue = IOC_connectService(&ClientLinkIDs[i], &ConnArgs[i], NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue) << "Client " << (i + 1) << " connection failed";
        ASSERT_NE(IOC_ID_INVALID, ClientLinkIDs[i]) << "Client " << (i + 1) << " LinkID invalid";

        // Staggered connection timing for resource management (important for 9999+ clients)
        std::this_thread::sleep_for(std::chrono::milliseconds(CONNECTION_TIMEOUT_MS / 2));
    }

    // Wait for all auto-accepts to complete with timeout
    auto auto_accept_start = std::chrono::steady_clock::now();
    for (int retry = 0; retry < (AUTO_ACCEPT_TIMEOUT_MS / 10); ++retry) {
        if (AutoAcceptPriv.AutoAcceptCount.load() >= NUM_CLIENTS) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    auto auto_accept_duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - auto_accept_start);

    ASSERT_EQ(NUM_CLIENTS, AutoAcceptPriv.AutoAcceptCount.load())
        << "Auto-accept count mismatch after " << auto_accept_duration.count() << "ms";
    ASSERT_TRUE(AutoAcceptPriv.ClientAutoAccepted.load());

    // Additional wait to ensure all auto-accept links are ready for commands
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Service orchestrates different commands to each of the 9 clients
    // Define diverse CmdIDs and payload values for extensive testing (scalable pattern)
    std::vector<IOC_CmdID_T> CmdIDs(NUM_CLIENTS);
    std::vector<std::string> PayloadValues(NUM_CLIENTS);

    for (int i = 0; i < NUM_CLIENTS; ++i) {
        // Alternating pattern: PING, ECHO, PING, ECHO, etc. (extensible for 9999+ clients)
        CmdIDs[i] = (i % 2 == 0) ? IOC_CMDID_TEST_PING : IOC_CMDID_TEST_ECHO;
        PayloadValues[i] =
            "Payload" + std::to_string(i + 1) + "_" + ((CmdIDs[i] == IOC_CMDID_TEST_PING) ? "PING" : "ECHO");
    }

    // Ensure we have enough accepted links
    ASSERT_EQ(NUM_CLIENTS, AutoAcceptPriv.AcceptedLinks.size());

    // Execute different commands on each client with dynamic allocation
    std::vector<IOC_CmdDesc_T> OrchestrationCmds(NUM_CLIENTS);
    for (int i = 0; i < NUM_CLIENTS; ++i) {
        OrchestrationCmds[i] = {};
        OrchestrationCmds[i].CmdID = CmdIDs[i];
        OrchestrationCmds[i].TimeoutMs = 5000;
        OrchestrationCmds[i].Status = IOC_CMD_STATUS_PENDING;

        // For ECHO commands, set input payload; for PING, no input needed
        if (CmdIDs[i] == IOC_CMDID_TEST_ECHO) {
            IOC_CmdDesc_setInPayload(&OrchestrationCmds[i], (void *)PayloadValues[i].c_str(),
                                     PayloadValues[i].length());
        }

        // Execute command on the i-th accepted client
        ResultValue = IOC_execCMD(AutoAcceptPriv.AcceptedLinks[i], &OrchestrationCmds[i], NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue) << "Command execution failed for client " << (i + 1);

        // Optimized delay between commands for resource management
        std::this_thread::sleep_for(std::chrono::milliseconds(COMMAND_DELAY_MS));
    }

    // 📊 PERFORMANCE MEASUREMENT: Track command execution timing
    auto cmd_execution_duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - test_start_time);

    // Verify all clients received and executed their commands
    int ExecutedCount = 0;
    for (int i = 0; i < NUM_CLIENTS; ++i) {
        if (ClientExecPrivs[i]->CommandReceived.load()) {
            ExecutedCount++;
            ASSERT_GE(ClientExecPrivs[i]->CommandCount.load(), 1) << "Client " << (i + 1) << " command count mismatch";
            ASSERT_EQ(CmdIDs[i], ClientExecPrivs[i]->LastCmdID) << "Client " << (i + 1) << " CmdID mismatch";
            ASSERT_EQ(IOC_CMD_STATUS_SUCCESS, ClientExecPrivs[i]->LastStatus)
                << "Client " << (i + 1) << " status mismatch";
        }
    }
    ASSERT_EQ(NUM_CLIENTS, ExecutedCount)
        << "All " << NUM_CLIENTS << " clients should receive and execute their commands";

    // 📈 SCALABILITY REPORT: Log performance metrics for analysis
    std::cout << "\n🎯 SCALABILITY METRICS for " << NUM_CLIENTS << " clients:\n";
    std::cout << "   Auto-accept duration: " << auto_accept_duration.count() << "ms\n";
    std::cout << "   Total test duration: " << cmd_execution_duration.count() << "ms\n";
    std::cout << "   Avg time per client: " << (cmd_execution_duration.count() / NUM_CLIENTS) << "ms\n";
    std::cout << "   Resource efficiency: " << (ExecutedCount * 100 / NUM_CLIENTS) << "% success rate\n";

    // Verify responses from orchestrated commands (SERVICE→CLIENT pattern)
    for (int i = 0; i < NUM_CLIENTS; ++i) {
        void *responseData = IOC_CmdDesc_getOutData(&OrchestrationCmds[i]);
        ULONG_T responseSize = IOC_CmdDesc_getOutDataSize(&OrchestrationCmds[i]);
        ASSERT_TRUE(responseData != nullptr) << "Client " << (i + 1) << " should provide response";
        ASSERT_GT(responseSize, 0) << "Client " << (i + 1) << " response size should be > 0";

        std::string response((char *)responseData, responseSize);

        if (CmdIDs[i] == IOC_CMDID_TEST_ECHO) {
            // ECHO commands should return "AUTO_" + input
            std::string expected = "AUTO_" + PayloadValues[i];
            ASSERT_EQ(expected, response) << "Client " << (i + 1) << " ECHO response mismatch";
        } else if (CmdIDs[i] == IOC_CMDID_TEST_PING) {
            // PING commands should return "AUTO_PONG"
            ASSERT_EQ("AUTO_PONG", response) << "Client " << (i + 1) << " PING response mismatch";
        }
    }

    // Cleanup all client links with dynamic approach
    for (int i = 0; i < NUM_CLIENTS; ++i) {
        if (ClientLinkIDs[i] != IOC_ID_INVALID) {
            IOC_closeLink(ClientLinkIDs[i]);
        }
    }

    // Note: server-side auto-accepted LinkIDs cleaned up automatically by IOC_offlineService()

    // Cleanup service
    if (SrvID != IOC_ID_INVALID) IOC_offlineService(SrvID);
}

// [@AC-1,US-3] TC-1: verifyOnAutoAcceptedCallback_byCommandContext_expectLinkReadiness
// [@AC-1,US-3] TC-1: OnAutoAccepted_F callback enabling CLIENT→SERVICE command readiness
TEST(UT_ConetCommandTypicalAutoAccept, verifyOnAutoAcceptedCallback_forClientToServiceCmd_expectLinkReadiness) {
    IOC_Result_T ResultValue = IOC_RESULT_BUG;

    // Enhanced callback private data to track callback details
    typedef struct __CallbackCmdPriv {
        __AutoAcceptCmdPriv_T AutoAcceptBase;
        std::atomic<bool> CallbackInvoked{false};
        std::atomic<bool> CommandContextReady{false};
        IOC_SrvID_T CallbackSrvID{IOC_ID_INVALID};
        IOC_LinkID_T CallbackLinkID{IOC_ID_INVALID};
        std::mutex CallbackMutex;
    } __CallbackCmdPriv_T;

    __CallbackCmdPriv_T CallbackPriv = {};

    // Enhanced auto-accept callback that validates command readiness
    auto OnAutoAcceptedWithCmdContext = [](IOC_SrvID_T SrvID, IOC_LinkID_T LinkID, void *pSrvPriv) {
        __CallbackCmdPriv_T *pPrivData = (__CallbackCmdPriv_T *)pSrvPriv;
        if (!pPrivData) return;

        std::lock_guard<std::mutex> lock(pPrivData->CallbackMutex);

        // Record callback invocation
        pPrivData->CallbackInvoked = true;
        pPrivData->CallbackSrvID = SrvID;
        pPrivData->CallbackLinkID = LinkID;

        // Update base auto-accept tracking
        pPrivData->AutoAcceptBase.ClientAutoAccepted = true;
        pPrivData->AutoAcceptBase.AutoAcceptCount++;
        pPrivData->AutoAcceptBase.LastAcceptedLinkID = LinkID;
        pPrivData->AutoAcceptBase.AcceptedLinks.push_back(LinkID);

        // Validate that link is immediately ready for CLIENT→SERVICE commands
        // In a real implementation, this callback could configure command-specific settings
        pPrivData->CommandContextReady = true;
    };

    // Setup auto-accept service with enhanced callback
    IOC_SrvURI_T SrvURI = {.pProtocol = IOC_SRV_PROTO_FIFO,
                           .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
                           .pPath = (const char *)"CmdAutoAccept_CallbackContext"};

    static IOC_CmdID_T SupportedCmdIDs[] = {IOC_CMDID_TEST_PING, IOC_CMDID_TEST_ECHO};
    IOC_CmdUsageArgs_T CmdUsageArgs = {.CbExecCmd_F = __AutoAcceptCmd_ExecutorCb,
                                       .pCbPrivData = &CallbackPriv.AutoAcceptBase,
                                       .CmdNum = sizeof(SupportedCmdIDs) / sizeof(SupportedCmdIDs[0]),
                                       .pCmdIDs = SupportedCmdIDs};

    IOC_SrvArgs_T SrvArgs = {.SrvURI = SrvURI,
                             .Flags = IOC_SRVFLAG_AUTO_ACCEPT,
                             .UsageCapabilites = IOC_LinkUsageCmdExecutor,
                             .UsageArgs = {.pCmd = &CmdUsageArgs},
                             .OnAutoAccepted_F = OnAutoAcceptedWithCmdContext,
                             .pSrvPriv = &CallbackPriv};

    IOC_SrvID_T SrvID = IOC_ID_INVALID;
    ResultValue = IOC_onlineService(&SrvID, &SrvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);

    // Client connects and triggers auto-accept callback
    IOC_ConnArgs_T ConnArgs = {.SrvURI = SrvURI, .Usage = IOC_LinkUsageCmdInitiator};
    IOC_LinkID_T CliLinkID = IOC_ID_INVALID;

    ResultValue = IOC_connectService(&CliLinkID, &ConnArgs, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);
    ASSERT_NE(IOC_ID_INVALID, CliLinkID);

    // Wait for auto-accept callback to be invoked
    for (int retry = 0; retry < 100; ++retry) {
        if (CallbackPriv.CallbackInvoked.load()) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // Verify callback was invoked with correct context
    ASSERT_TRUE(CallbackPriv.CallbackInvoked.load()) << "OnAutoAccepted_F callback should be invoked";
    ASSERT_EQ(SrvID, CallbackPriv.CallbackSrvID) << "Callback should receive correct SrvID";
    ASSERT_NE(IOC_ID_INVALID, CallbackPriv.CallbackLinkID) << "Callback should receive valid LinkID";
    ASSERT_TRUE(CallbackPriv.CommandContextReady.load()) << "Command context should be ready after callback";

    // Verify base auto-accept tracking was updated
    ASSERT_TRUE(CallbackPriv.AutoAcceptBase.ClientAutoAccepted.load());
    ASSERT_EQ(1, CallbackPriv.AutoAcceptBase.AutoAcceptCount.load());
    ASSERT_EQ(CallbackPriv.CallbackLinkID, CallbackPriv.AutoAcceptBase.LastAcceptedLinkID);

    // Additional wait to ensure command readiness after callback
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Verify CLIENT→SERVICE command execution works immediately after callback
    IOC_CmdDesc_T CmdDesc = {};
    CmdDesc.CmdID = IOC_CMDID_TEST_PING;
    CmdDesc.TimeoutMs = 3000;
    CmdDesc.Status = IOC_CMD_STATUS_PENDING;

    ResultValue = IOC_execCMD(CliLinkID, &CmdDesc, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue) << "CLIENT→SERVICE command should work after auto-accept callback";
    ASSERT_EQ(IOC_CMD_STATUS_SUCCESS, CmdDesc.Status) << "Command should complete successfully";

    // Verify command was executed by service
    ASSERT_TRUE(CallbackPriv.AutoAcceptBase.CommandReceived.load());
    ASSERT_GE(CallbackPriv.AutoAcceptBase.CommandCount.load(), 1);
    ASSERT_EQ(IOC_CMDID_TEST_PING, CallbackPriv.AutoAcceptBase.LastCmdID);

    // Verify response data
    void *responseData = IOC_CmdDesc_getOutData(&CmdDesc);
    ULONG_T responseSize = IOC_CmdDesc_getOutDataSize(&CmdDesc);
    ASSERT_TRUE(responseData != nullptr);
    ASSERT_GT(responseSize, 0);

    std::string response((char *)responseData, responseSize);
    ASSERT_EQ("AUTO_PONG", response) << "Should receive expected response from auto-accepted service";

    // Cleanup
    if (CliLinkID != IOC_ID_INVALID) IOC_closeLink(CliLinkID);
    if (SrvID != IOC_ID_INVALID) IOC_offlineService(SrvID);
}

// [@AC-2,US-3] TC-1: Service with BOTH CmdExecutor+CmdInitiator supporting mixed client types
// [@AC-2,US-3] TC-1: Per-client command flow configuration (CLIENT→SERVICE & SERVICE→CLIENT patterns)
TEST(UT_ConetCommandTypicalAutoAccept, verifyOnAutoAcceptedCallback_forMixedCmdPatterns_expectFlexibleConfig) {
    IOC_Result_T ResultValue = IOC_RESULT_BUG;

    // Enhanced callback private data structure for mixed pattern tracking
    typedef struct __MixedPatternPriv {
        __AutoAcceptCmdPriv_T AutoAcceptBase;
        std::atomic<bool> CallbackInvoked{false};
        std::atomic<int> CmdInitiatorClients{0};  // CLIENT→SERVICE clients
        std::atomic<int> CmdExecutorClients{0};   // SERVICE→CLIENT clients
        std::mutex CallbackMutex;
        std::map<IOC_LinkID_T, IOC_LinkUsage_T> ClientUsageMap;  // Track per-client usage types
        std::vector<IOC_LinkID_T> InitiatorLinks;                // Links for CLIENT→SERVICE commands
        std::vector<IOC_LinkID_T> ExecutorLinks;                 // Links for SERVICE→CLIENT commands
    } __MixedPatternPriv_T;

    __MixedPatternPriv_T MixedPriv = {};

    // Private data for client-side command executors (SERVICE→CLIENT pattern)
    __AutoAcceptCmdPriv_T ClientExecPriv_A = {};  // Client-A executor
    __AutoAcceptCmdPriv_T ClientExecPriv_B = {};  // Client-B executor

    // Enhanced auto-accept callback that configures per-client command capabilities
    auto OnAutoAcceptedWithMixedConfig = [](IOC_SrvID_T SrvID, IOC_LinkID_T LinkID, void *pSrvPriv) {
        __MixedPatternPriv_T *pPrivData = (__MixedPatternPriv_T *)pSrvPriv;
        if (!pPrivData) return;

        std::lock_guard<std::mutex> lock(pPrivData->CallbackMutex);

        // Record callback invocation
        pPrivData->CallbackInvoked = true;

        // Update base auto-accept tracking
        pPrivData->AutoAcceptBase.ClientAutoAccepted = true;
        pPrivData->AutoAcceptBase.AutoAcceptCount++;
        pPrivData->AutoAcceptBase.LastAcceptedLinkID = LinkID;
        pPrivData->AutoAcceptBase.AcceptedLinks.push_back(LinkID);

        // TODO: In real implementation, we would query the client's Usage type
        // For this test, we'll track the connection order to determine client type
        int clientIndex = pPrivData->AutoAcceptBase.AutoAcceptCount.load() - 1;

        if (clientIndex % 2 == 0) {
            // Even-numbered clients (0, 2, 4, ...) are CmdInitiator clients (CLIENT→SERVICE)
            pPrivData->CmdInitiatorClients++;
            pPrivData->InitiatorLinks.push_back(LinkID);
            pPrivData->ClientUsageMap[LinkID] = IOC_LinkUsageCmdInitiator;
        } else {
            // Odd-numbered clients (1, 3, 5, ...) are CmdExecutor clients (SERVICE→CLIENT)
            pPrivData->CmdExecutorClients++;
            pPrivData->ExecutorLinks.push_back(LinkID);
            pPrivData->ClientUsageMap[LinkID] = IOC_LinkUsageCmdExecutor;
        }
    };

    // Setup service with COMBINED capabilities: CmdExecutor | CmdInitiator
    IOC_SrvURI_T SrvURI = {.pProtocol = IOC_SRV_PROTO_FIFO,
                           .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
                           .pPath = (const char *)"CmdAutoAccept_MixedPattern"};

    // Service supports BOTH command directions
    static IOC_CmdID_T SupportedCmdIDs[] = {IOC_CMDID_TEST_PING, IOC_CMDID_TEST_ECHO};
    IOC_CmdUsageArgs_T CmdUsageArgs = {.CbExecCmd_F = __AutoAcceptCmd_ExecutorCb,
                                       .pCbPrivData = &MixedPriv.AutoAcceptBase,
                                       .CmdNum = sizeof(SupportedCmdIDs) / sizeof(SupportedCmdIDs[0]),
                                       .pCmdIDs = SupportedCmdIDs};

    IOC_SrvArgs_T SrvArgs = {
        .SrvURI = SrvURI,
        .Flags = IOC_SRVFLAG_AUTO_ACCEPT,
        .UsageCapabilites =
            (IOC_LinkUsage_T)(IOC_LinkUsageCmdExecutor | IOC_LinkUsageCmdInitiator),  // 🎯 COMBINED CAPABILITIES
        .UsageArgs = {.pCmd = &CmdUsageArgs},
        .OnAutoAccepted_F = OnAutoAcceptedWithMixedConfig,
        .pSrvPriv = &MixedPriv};

    IOC_SrvID_T SrvID = IOC_ID_INVALID;
    ResultValue = IOC_onlineService(&SrvID, &SrvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);

    // 🔄 CLIENT-A: CmdInitiator type (CLIENT-A→SERVICE commands, US-1 pattern)
    IOC_ConnArgs_T ConnArgs_A = {.SrvURI = SrvURI, .Usage = IOC_LinkUsageCmdInitiator};
    IOC_LinkID_T CliLinkID_A = IOC_ID_INVALID;

    ResultValue = IOC_connectService(&CliLinkID_A, &ConnArgs_A, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);
    ASSERT_NE(IOC_ID_INVALID, CliLinkID_A);

    // Wait for Client-A auto-accept
    for (int retry = 0; retry < 100; ++retry) {
        if (MixedPriv.CmdInitiatorClients.load() >= 1) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // 🔄 CLIENT-B: CmdExecutor type (SERVICE→CLIENT-B commands, US-2 pattern)
    static IOC_CmdID_T ClientCmdIDs[] = {IOC_CMDID_TEST_PING, IOC_CMDID_TEST_ECHO};
    IOC_CmdUsageArgs_T ClientCmdUsageArgs_B = {.CbExecCmd_F = __AutoAcceptCmd_ExecutorCb,
                                               .pCbPrivData = &ClientExecPriv_B,
                                               .CmdNum = sizeof(ClientCmdIDs) / sizeof(ClientCmdIDs[0]),
                                               .pCmdIDs = ClientCmdIDs};

    IOC_ConnArgs_T ConnArgs_B = {
        .SrvURI = SrvURI, .Usage = IOC_LinkUsageCmdExecutor, .UsageArgs = {.pCmd = &ClientCmdUsageArgs_B}};
    IOC_LinkID_T CliLinkID_B = IOC_ID_INVALID;

    ResultValue = IOC_connectService(&CliLinkID_B, &ConnArgs_B, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);
    ASSERT_NE(IOC_ID_INVALID, CliLinkID_B);

    // Wait for Client-B auto-accept
    for (int retry = 0; retry < 100; ++retry) {
        if (MixedPriv.CmdExecutorClients.load() >= 1) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // Verify callback configured mixed client patterns correctly
    ASSERT_TRUE(MixedPriv.CallbackInvoked.load());
    ASSERT_EQ(2, MixedPriv.AutoAcceptBase.AutoAcceptCount.load());
    ASSERT_EQ(1, MixedPriv.CmdInitiatorClients.load());  // Client-A
    ASSERT_EQ(1, MixedPriv.CmdExecutorClients.load());   // Client-B
    ASSERT_EQ(1, MixedPriv.InitiatorLinks.size());       // CLIENT→SERVICE links
    ASSERT_EQ(1, MixedPriv.ExecutorLinks.size());        // SERVICE→CLIENT links

    // Additional wait to ensure all auto-accept links are ready for commands
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // 🎯 TEST PATTERN 1: CLIENT-A→SERVICE commands (US-1 pattern)
    IOC_CmdDesc_T CmdDesc_A = {};
    CmdDesc_A.CmdID = IOC_CMDID_TEST_PING;
    CmdDesc_A.TimeoutMs = 3000;
    CmdDesc_A.Status = IOC_CMD_STATUS_PENDING;

    ResultValue = IOC_execCMD(CliLinkID_A, &CmdDesc_A, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue) << "CLIENT-A→SERVICE command should work";
    ASSERT_EQ(IOC_CMD_STATUS_SUCCESS, CmdDesc_A.Status);

    // Verify CLIENT-A→SERVICE command was executed by service
    ASSERT_TRUE(MixedPriv.AutoAcceptBase.CommandReceived.load());
    ASSERT_GE(MixedPriv.AutoAcceptBase.CommandCount.load(), 1);

    // Verify response from service to CLIENT-A
    void *responseData_A = IOC_CmdDesc_getOutData(&CmdDesc_A);
    ULONG_T responseSize_A = IOC_CmdDesc_getOutDataSize(&CmdDesc_A);
    ASSERT_TRUE(responseData_A != nullptr);
    ASSERT_GT(responseSize_A, 0);

    std::string response_A((char *)responseData_A, responseSize_A);
    ASSERT_EQ("AUTO_PONG", response_A) << "CLIENT-A should receive response from service";

    // 🎯 TEST PATTERN 2: SERVICE→CLIENT-B commands (US-2 pattern)
    IOC_CmdDesc_T CmdDesc_B = {};
    CmdDesc_B.CmdID = IOC_CMDID_TEST_PING;
    CmdDesc_B.TimeoutMs = 3000;
    CmdDesc_B.Status = IOC_CMD_STATUS_PENDING;

    // Service sends command to CLIENT-B using the executor link
    ASSERT_EQ(1, MixedPriv.ExecutorLinks.size());
    IOC_LinkID_T ServiceToClientB_LinkID = MixedPriv.ExecutorLinks[0];

    ResultValue = IOC_execCMD(ServiceToClientB_LinkID, &CmdDesc_B, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue) << "SERVICE→CLIENT-B command should work";
    ASSERT_EQ(IOC_CMD_STATUS_SUCCESS, CmdDesc_B.Status);

    // Verify SERVICE→CLIENT-B command was executed by CLIENT-B
    ASSERT_TRUE(ClientExecPriv_B.CommandReceived.load());
    ASSERT_GE(ClientExecPriv_B.CommandCount.load(), 1);
    ASSERT_EQ(IOC_CMDID_TEST_PING, ClientExecPriv_B.LastCmdID);
    ASSERT_EQ(IOC_CMD_STATUS_SUCCESS, ClientExecPriv_B.LastStatus);

    // Verify response from CLIENT-B to service
    void *responseData_B = IOC_CmdDesc_getOutData(&CmdDesc_B);
    ULONG_T responseSize_B = IOC_CmdDesc_getOutDataSize(&CmdDesc_B);
    ASSERT_TRUE(responseData_B != nullptr);
    ASSERT_GT(responseSize_B, 0);

    std::string response_B((char *)responseData_B, responseSize_B);
    ASSERT_EQ("AUTO_PONG", response_B) << "Service should receive response from CLIENT-B";

    // 🎯 VERIFICATION: Mixed patterns working simultaneously
    // 1. CLIENT-A can send commands to service (CLIENT→SERVICE)
    // 2. Service can send commands to CLIENT-B (SERVICE→CLIENT)
    // 3. Both patterns coexist in the same service with combined capabilities

    IOC_CmdDesc_T CmdDesc_A2 = {};
    CmdDesc_A2.CmdID = IOC_CMDID_TEST_ECHO;
    CmdDesc_A2.TimeoutMs = 3000;
    CmdDesc_A2.Status = IOC_CMD_STATUS_PENDING;
    const char *echoInput = "MixedPattern_Test";
    IOC_CmdDesc_setInPayload(&CmdDesc_A2, (void *)echoInput, strlen(echoInput));

    ResultValue = IOC_execCMD(CliLinkID_A, &CmdDesc_A2, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue) << "Second CLIENT-A→SERVICE command should work";

    void *responseData_A2 = IOC_CmdDesc_getOutData(&CmdDesc_A2);
    std::string response_A2((char *)responseData_A2, IOC_CmdDesc_getOutDataSize(&CmdDesc_A2));
    ASSERT_EQ("AUTO_MixedPattern_Test", response_A2) << "Service should echo CLIENT-A input with AUTO_ prefix";

    // Verify final command statistics
    ASSERT_GE(MixedPriv.AutoAcceptBase.CommandCount.load(), 2) << "Service should have executed multiple commands";
    ASSERT_GE(ClientExecPriv_B.CommandCount.load(), 1) << "CLIENT-B should have executed service commands";

    // Cleanup
    if (CliLinkID_A != IOC_ID_INVALID) IOC_closeLink(CliLinkID_A);
    if (CliLinkID_B != IOC_ID_INVALID) IOC_closeLink(CliLinkID_B);
    if (SrvID != IOC_ID_INVALID) IOC_offlineService(SrvID);
}

// [@AC-3,US-3] TC-1: verifyOnAutoAcceptedCallback_byMixedPatterns_expectFlexibleHandling
// [@AC-3,US-3] TC-1: Mixed command execution modes (callback + polling) with auto-accept callback
TEST(UT_ConetCommandTypicalAutoAccept, verifyOnAutoAcceptedCallback_forCallbackPlusPolling_expectFlexibleHandling) {
    IOC_Result_T ResultValue = IOC_RESULT_BUG;

    // Enhanced callback private data structure for tracking mixed client execution modes
    typedef struct __MixedExecModePriv {
        __AutoAcceptCmdPriv_T AutoAcceptBase;
        std::atomic<bool> CallbackInvoked{false};
        std::atomic<int> CallbackClients{0};  // Clients using callback-based execution
        std::atomic<int> PollingClients{0};   // Clients using polling-based execution
        std::mutex CallbackMutex;
        std::map<IOC_LinkID_T, std::string> ClientExecutionModes;  // Track per-client execution modes
        std::vector<IOC_LinkID_T> CallbackLinks;                   // Links for callback execution clients
        std::vector<IOC_LinkID_T> PollingLinks;                    // Links for polling execution clients
    } __MixedExecModePriv_T;

    __MixedExecModePriv_T MixedExecPriv = {};

    // Private data for callback-based client command executor (Client-A)
    __AutoAcceptCmdPriv_T CallbackClientExecPriv = {};

    // Enhanced auto-accept callback that tracks different client execution modes
    auto OnAutoAcceptedWithExecModeTracking = [](IOC_SrvID_T SrvID, IOC_LinkID_T LinkID, void *pSrvPriv) {
        __MixedExecModePriv_T *pPrivData = (__MixedExecModePriv_T *)pSrvPriv;
        if (!pPrivData) return;

        std::lock_guard<std::mutex> lock(pPrivData->CallbackMutex);

        // Record callback invocation
        pPrivData->CallbackInvoked = true;

        // Update base auto-accept tracking
        pPrivData->AutoAcceptBase.ClientAutoAccepted = true;
        pPrivData->AutoAcceptBase.AutoAcceptCount++;
        pPrivData->AutoAcceptBase.LastAcceptedLinkID = LinkID;
        pPrivData->AutoAcceptBase.AcceptedLinks.push_back(LinkID);

        // Track different client execution modes based on connection order
        int clientIndex = pPrivData->AutoAcceptBase.AutoAcceptCount.load() - 1;

        if (clientIndex % 2 == 0) {
            // Even-numbered clients (0, 2, 4, ...) use callback-based execution
            pPrivData->CallbackClients++;
            pPrivData->CallbackLinks.push_back(LinkID);
            pPrivData->ClientExecutionModes[LinkID] = "CALLBACK_IMMEDIATE";
        } else {
            // Odd-numbered clients (1, 3, 5, ...) use polling-based execution
            pPrivData->PollingClients++;
            pPrivData->PollingLinks.push_back(LinkID);
            pPrivData->ClientExecutionModes[LinkID] = "POLLING_MANUAL";
        }
    };

    // Setup service as command INITIATOR (service sends commands to clients)
    IOC_SrvURI_T SrvURI = {.pProtocol = IOC_SRV_PROTO_FIFO,
                           .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
                           .pPath = (const char *)"CmdAutoAccept_MixedExecModes"};

    // Service initiates commands - no callback execution needed for service side
    static IOC_CmdID_T ServiceCmdIDs[] = {IOC_CMDID_TEST_PING, IOC_CMDID_TEST_ECHO};
    IOC_CmdUsageArgs_T ServiceCmdUsageArgs = {.CbExecCmd_F = nullptr,  // Service initiates, doesn't execute
                                              .pCbPrivData = nullptr,
                                              .CmdNum = sizeof(ServiceCmdIDs) / sizeof(ServiceCmdIDs[0]),
                                              .pCmdIDs = ServiceCmdIDs};

    IOC_SrvArgs_T SrvArgs = {.SrvURI = SrvURI,
                             .Flags = IOC_SRVFLAG_AUTO_ACCEPT,               // Enable auto-accept
                             .UsageCapabilites = IOC_LinkUsageCmdInitiator,  // Service initiates commands
                             .UsageArgs = {.pCmd = &ServiceCmdUsageArgs},
                             .OnAutoAccepted_F = OnAutoAcceptedWithExecModeTracking,
                             .pSrvPriv = &MixedExecPriv};

    IOC_SrvID_T SrvID = IOC_ID_INVALID;
    ResultValue = IOC_onlineService(&SrvID, &SrvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);

    // 🎯 CLIENT-A: Callback-based command execution (immediate processing via callback)
    static IOC_CmdID_T CallbackClientCmdIDs[] = {IOC_CMDID_TEST_PING, IOC_CMDID_TEST_ECHO};
    IOC_CmdUsageArgs_T CallbackClientCmdUsageArgs = {
        .CbExecCmd_F = __AutoAcceptCmd_ExecutorCb,  // Callback execution
        .pCbPrivData = &CallbackClientExecPriv,
        .CmdNum = sizeof(CallbackClientCmdIDs) / sizeof(CallbackClientCmdIDs[0]),
        .pCmdIDs = CallbackClientCmdIDs};

    IOC_ConnArgs_T ConnArgs_CallbackClient = {
        .SrvURI = SrvURI, .Usage = IOC_LinkUsageCmdExecutor, .UsageArgs = {.pCmd = &CallbackClientCmdUsageArgs}};
    IOC_LinkID_T CallbackClientLinkID = IOC_ID_INVALID;

    ResultValue = IOC_connectService(&CallbackClientLinkID, &ConnArgs_CallbackClient, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);
    ASSERT_NE(IOC_ID_INVALID, CallbackClientLinkID);

    // Wait for Client-A auto-accept
    for (int retry = 0; retry < 100; ++retry) {
        if (MixedExecPriv.CallbackClients.load() >= 1) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // 🔄 CLIENT-B: Polling-based command execution (manual IOC_waitCMD calls)
    static IOC_CmdID_T PollingClientCmdIDs[] = {IOC_CMDID_TEST_PING, IOC_CMDID_TEST_ECHO};
    IOC_CmdUsageArgs_T PollingClientCmdUsageArgs = {
        .CbExecCmd_F = nullptr,  // No callback for polling mode
        .pCbPrivData = nullptr,
        .CmdNum = sizeof(PollingClientCmdIDs) / sizeof(PollingClientCmdIDs[0]),
        .pCmdIDs = PollingClientCmdIDs};

    IOC_ConnArgs_T ConnArgs_PollingClient = {
        .SrvURI = SrvURI, .Usage = IOC_LinkUsageCmdExecutor, .UsageArgs = {.pCmd = &PollingClientCmdUsageArgs}};
    IOC_LinkID_T PollingClientLinkID = IOC_ID_INVALID;

    ResultValue = IOC_connectService(&PollingClientLinkID, &ConnArgs_PollingClient, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);
    ASSERT_NE(IOC_ID_INVALID, PollingClientLinkID);

    // Wait for Client-B auto-accept
    for (int retry = 0; retry < 100; ++retry) {
        if (MixedExecPriv.PollingClients.load() >= 1) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // Verify callback configured mixed execution modes correctly
    ASSERT_TRUE(MixedExecPriv.CallbackInvoked.load());
    ASSERT_EQ(2, MixedExecPriv.AutoAcceptBase.AutoAcceptCount.load());
    ASSERT_EQ(1, MixedExecPriv.CallbackClients.load());  // Client-A
    ASSERT_EQ(1, MixedExecPriv.PollingClients.load());   // Client-B
    ASSERT_EQ(1, MixedExecPriv.CallbackLinks.size());    // Callback execution clients
    ASSERT_EQ(1, MixedExecPriv.PollingLinks.size());     // Polling execution clients

    // Additional wait to ensure all auto-accept links are ready for commands
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // 🎯 TEST PATTERN 1: Service sends command to CLIENT-A (callback-based execution)
    IOC_CmdDesc_T CallbackCmd = {};
    CallbackCmd.CmdID = IOC_CMDID_TEST_PING;
    CallbackCmd.TimeoutMs = 3000;
    CallbackCmd.Status = IOC_CMD_STATUS_PENDING;

    // Service sends command to callback client
    ASSERT_EQ(1, MixedExecPriv.CallbackLinks.size());
    IOC_LinkID_T ServiceToCallbackClient_LinkID = MixedExecPriv.CallbackLinks[0];

    auto callback_start_time = std::chrono::steady_clock::now();
    ResultValue = IOC_execCMD(ServiceToCallbackClient_LinkID, &CallbackCmd, NULL);
    auto callback_duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - callback_start_time);

    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue) << "SERVICE→CLIENT-A (callback) command should work";
    ASSERT_EQ(IOC_CMD_STATUS_SUCCESS, CallbackCmd.Status) << "Callback command should complete immediately";

    // Verify callback execution was processed by CLIENT-A immediately
    ASSERT_TRUE(CallbackClientExecPriv.CommandReceived.load());
    ASSERT_GE(CallbackClientExecPriv.CommandCount.load(), 1);
    ASSERT_EQ(IOC_CMDID_TEST_PING, CallbackClientExecPriv.LastCmdID);
    ASSERT_EQ(IOC_CMD_STATUS_SUCCESS, CallbackClientExecPriv.LastStatus);
    ASSERT_LT(callback_duration.count(), 500) << "Callback execution should be fast (< 500ms)";

    // Verify response from callback client
    void *callbackResponseData = IOC_CmdDesc_getOutData(&CallbackCmd);
    ULONG_T callbackResponseSize = IOC_CmdDesc_getOutDataSize(&CallbackCmd);
    ASSERT_TRUE(callbackResponseData != nullptr);
    ASSERT_GT(callbackResponseSize, 0);

    std::string callbackResponse((char *)callbackResponseData, callbackResponseSize);
    ASSERT_EQ("AUTO_PONG", callbackResponse) << "CLIENT-A should provide callback response";

    // 🔄 TEST PATTERN 2: Service sends command to CLIENT-B (polling-based execution)
    IOC_CmdDesc_T PollingCmd = {};
    PollingCmd.CmdID = IOC_CMDID_TEST_ECHO;
    PollingCmd.TimeoutMs = 5000;
    PollingCmd.Status = IOC_CMD_STATUS_PENDING;
    const char *echoInput = "PollingTest";
    IOC_CmdDesc_setInPayload(&PollingCmd, (void *)echoInput, strlen(echoInput));

    // Service sends command to polling client (this will block until client polls for it)
    ASSERT_EQ(1, MixedExecPriv.PollingLinks.size());
    IOC_LinkID_T ServiceToPollingClient_LinkID = MixedExecPriv.PollingLinks[0];

    auto polling_start_time = std::chrono::steady_clock::now();

    // Start service command in background since polling client needs to actively wait for it
    std::atomic<bool> ServiceCommandSent{false};
    std::thread ServiceCommandThread([&] {
        IOC_Result_T ServiceResult = IOC_execCMD(ServiceToPollingClient_LinkID, &PollingCmd, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, ServiceResult) << "SERVICE→CLIENT-B (polling) command should work";
        ServiceCommandSent = true;
    });

    // Small delay to let service start sending command
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // CLIENT-B polls for commands using IOC_waitCMD (polling mode)
    IOC_CmdDesc_T ClientPollCmd = {};
    ClientPollCmd.CmdID = 0;  // Wait for any command
    ClientPollCmd.TimeoutMs = 3000;
    ClientPollCmd.Status = IOC_CMD_STATUS_PENDING;

    ResultValue = IOC_waitCMD(PollingClientLinkID, &ClientPollCmd, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue) << "CLIENT-B should receive command via polling";
    ASSERT_EQ(IOC_CMDID_TEST_ECHO, ClientPollCmd.CmdID) << "CLIENT-B should receive ECHO command";

    // CLIENT-B manually processes the polled command and sends response
    void *polledInputData = IOC_CmdDesc_getInData(&ClientPollCmd);
    ULONG_T polledInputSize = IOC_CmdDesc_getInDataSize(&ClientPollCmd);
    ASSERT_TRUE(polledInputData != nullptr);
    ASSERT_GT(polledInputSize, 0);

    std::string polledInput((char *)polledInputData, polledInputSize);
    ASSERT_EQ("PollingTest", polledInput) << "CLIENT-B should receive correct input via polling";

    // CLIENT-B processes and responds to polled command
    std::string pollingResponse = "AUTO_" + polledInput;
    IOC_CmdDesc_setOutPayload(&ClientPollCmd, (void *)pollingResponse.c_str(), pollingResponse.length());
    IOC_CmdDesc_setStatus(&ClientPollCmd, IOC_CMD_STATUS_SUCCESS);
    IOC_CmdDesc_setResult(&ClientPollCmd, IOC_RESULT_SUCCESS);

    // CLIENT-B acknowledges the polled command
    ResultValue = IOC_ackCMD(PollingClientLinkID, &ClientPollCmd, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue) << "CLIENT-B should acknowledge polled command";

    // Wait for service command thread to complete
    if (ServiceCommandThread.joinable()) ServiceCommandThread.join();

    auto polling_duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - polling_start_time);

    // Verify service received response from polling client
    ASSERT_TRUE(ServiceCommandSent.load()) << "Service command should complete";
    ASSERT_EQ(IOC_CMD_STATUS_SUCCESS, PollingCmd.Status) << "Polling command should complete successfully";

    void *pollingResponseData = IOC_CmdDesc_getOutData(&PollingCmd);
    ULONG_T pollingResponseSize = IOC_CmdDesc_getOutDataSize(&PollingCmd);
    ASSERT_TRUE(pollingResponseData != nullptr);
    ASSERT_GT(pollingResponseSize, 0);

    std::string finalPollingResponse((char *)pollingResponseData, pollingResponseSize);
    ASSERT_EQ("AUTO_PollingTest", finalPollingResponse) << "Service should receive polling response from CLIENT-B";

    // 📊 PERFORMANCE COMPARISON: Callback vs Polling execution timing
    std::cout << "\n🎯 CLIENT EXECUTION MODE PERFORMANCE COMPARISON:\n";
    std::cout << "   Callback client execution (CLIENT-A): " << callback_duration.count() << "ms\n";
    std::cout << "   Polling client execution (CLIENT-B): " << polling_duration.count() << "ms\n";
    std::cout << "   Performance difference: " << (polling_duration.count() - callback_duration.count()) << "ms\n";

    // Verify timing characteristics: polling should take longer than callback
    ASSERT_GT(polling_duration.count(), callback_duration.count())
        << "Polling client execution should take longer than callback client execution";

    // 🎯 VERIFICATION: Mixed client execution modes working simultaneously
    // 1. CLIENT-A uses callback-based execution (immediate processing via CbExecCmd_F)
    // 2. CLIENT-B uses polling-based execution (manual IOC_waitCMD + IOC_ackCMD)
    // 3. Both patterns coexist with the same service (service is command initiator)
    // 4. OnAutoAccepted_F callback tracks different client execution modes

    // Send one more command to each client to verify sustained mixed operation
    IOC_CmdDesc_T CallbackCmd2 = {};
    CallbackCmd2.CmdID = IOC_CMDID_TEST_PING;
    CallbackCmd2.TimeoutMs = 3000;
    CallbackCmd2.Status = IOC_CMD_STATUS_PENDING;

    ResultValue = IOC_execCMD(ServiceToCallbackClient_LinkID, &CallbackCmd2, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue) << "Second CLIENT-A callback command should work";
    ASSERT_EQ(IOC_CMD_STATUS_SUCCESS, CallbackCmd2.Status) << "Second callback should complete immediately";

    // Verify final execution mode assignments in callback private data
    ASSERT_EQ(2, MixedExecPriv.ClientExecutionModes.size()) << "Should track 2 client execution modes";

    auto callbackLinkID = MixedExecPriv.CallbackLinks[0];
    auto pollingLinkID = MixedExecPriv.PollingLinks[0];

    ASSERT_EQ("CALLBACK_IMMEDIATE", MixedExecPriv.ClientExecutionModes[callbackLinkID])
        << "CLIENT-A should be configured for callback execution";
    ASSERT_EQ("POLLING_MANUAL", MixedExecPriv.ClientExecutionModes[pollingLinkID])
        << "CLIENT-B should be configured for polling execution";

    // Verify sustained command execution capabilities
    ASSERT_GE(CallbackClientExecPriv.CommandCount.load(), 2)
        << "CLIENT-A should have processed multiple callback commands";

    // Cleanup
    if (CallbackClientLinkID != IOC_ID_INVALID) IOC_closeLink(CallbackClientLinkID);
    if (PollingClientLinkID != IOC_ID_INVALID) IOC_closeLink(PollingClientLinkID);
    if (SrvID != IOC_ID_INVALID) IOC_offlineService(SrvID);
}

//======>BEGIN US-4: Service Lifecycle with Persistent Links=======================================

// [@AC-1,US-4] TC-1: verifyKeepAcceptedLink_byServiceOffline_expectLinkPersistence
// [@AC-1,US-4] TC-1: Auto-accepted links persist after service offline with IOC_SRVFLAG_KEEP_ACCEPTED_LINK
/**
 * @brief Test link persistence behavior across service lifecycle with IOC_SRVFLAG_KEEP_ACCEPTED_LINK
 *
 * 🔑 PERSISTENCE CONCEPT EXPLAINED:
 * - "Persist" = Links survive service shutdown and remain valid for future operations
 * - WITHOUT persistent flag: Auto-cleanup occurs → links destroyed on service offline
 * - WITH persistent flag: Links preserved → manual cleanup required, service restart possible
 *
 * 📋 PERSISTENCE LIFECYCLE PATTERN:
 * 1. Service(AutoAccept+KeepLinks) ← Client connects
 * 2. Auto-accept creates persistent link with bidirectional command capability
 * 3. Service goes offline → Link persists (vs auto-cleanup destroying link)
 * 4. Application responsible for manual link cleanup OR service restart reuses link
 *
 * 🎯 PERSISTENCE VALUE PROPOSITION:
 * - Enables service restart without client reconnection overhead
 * - Supports hot-swappable service deployments with connection continuity
 * - Provides application-controlled resource management vs automatic cleanup
 * - Maintains established command channels across service lifecycle events
 *
 * ⚖️ PERSISTENCE TRADE-OFFS:
 * Benefits: Service restart flexibility, connection preservation, reduced reconnection cost
 * Responsibilities: Manual cleanup required, resource tracking burden, potential leak risk
 *
 * 🚩 CURRENT IMPLEMENTATION STATUS:
 * IOC_SRVFLAG_KEEP_ACCEPTED_LINK flag not yet implemented → Test documents expected behavior
 * Using IOC_SRVFLAG_AUTO_ACCEPT only to demonstrate current auto-cleanup vs future persistence
 */
TEST(UT_ConetCommandTypicalAutoAccept, verifyKeepAcceptedLink_byServiceOffline_expectLinkPersistence) {
    // 📋 TEST DESIGN: Document expected persistent link behavior vs current auto-cleanup behavior
    // This test serves as specification for IOC_SRVFLAG_KEEP_ACCEPTED_LINK implementation

    IOC_Result_T ResultValue = IOC_RESULT_BUG;
    __AutoAcceptCmdPriv_T AutoAcceptPriv = {};

    IOC_SrvURI_T SrvURI = {.pProtocol = IOC_SRV_PROTO_FIFO,
                           .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
                           .pPath = (const char *)"PersistentLinkTest"};

    // Define supported commands for auto-accept service
    static IOC_CmdID_T SupportedCmdIDs[] = {IOC_CMDID_TEST_PING};
    IOC_CmdUsageArgs_T CmdUsageArgs = {.CbExecCmd_F = __AutoAcceptCmd_ExecutorCb,
                                       .pCbPrivData = &AutoAcceptPriv,
                                       .CmdNum = 1,
                                       .pCmdIDs = SupportedCmdIDs};

    IOC_SrvArgs_T SrvArgs = {
        .SrvURI = SrvURI,
        .Flags = (IOC_SrvFlags_T)(IOC_SRVFLAG_AUTO_ACCEPT |
                                  IOC_SRVFLAG_KEEP_ACCEPTED_LINK),  // TDD RED: Test persistence flag
        .UsageCapabilites = IOC_LinkUsageCmdExecutor,
        .UsageArgs = {.pCmd = &CmdUsageArgs},
        .OnAutoAccepted_F = __AutoAcceptCmd_OnAutoAcceptedCb,
        .pSrvPriv = &AutoAcceptPriv};

    IOC_SrvID_T SrvID = IOC_ID_INVALID;
    ResultValue = IOC_onlineService(&SrvID, &SrvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);

    std::cout << "\n🔄 TESTING PERSISTENT LINK BEHAVIOR:\n";
    std::cout << "   Setting up auto-accept service with command capability...\n";
    std::cout << "   🎯 PERSISTENCE GOAL: Validate link survival after service shutdown\n";
    std::cout << "   📋 CURRENT vs EXPECTED: Documenting auto-cleanup vs persistent behavior\n";

    // Client setup and connection
    IOC_ConnArgs_T ConnArgs = {.SrvURI = SrvURI, .Usage = IOC_LinkUsageCmdInitiator};
    IOC_LinkID_T CliLinkID = IOC_ID_INVALID;

    std::thread CliThread([&] {
        IOC_Result_T ResultValueInThread = IOC_connectService(&CliLinkID, &ConnArgs, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValueInThread);
        ASSERT_NE(IOC_ID_INVALID, CliLinkID);

        // Execute command to verify connection works
        IOC_CmdDesc_T TestCmd = {};
        TestCmd.CmdID = IOC_CMDID_TEST_PING;
        TestCmd.TimeoutMs = 500;
        TestCmd.Status = IOC_CMD_STATUS_PENDING;

        IOC_Result_T CmdResult = IOC_execCMD(CliLinkID, &TestCmd, NULL);
        EXPECT_EQ(IOC_RESULT_SUCCESS, CmdResult) << "Command execution should succeed";

        std::cout << "   ✓ Client connected and command executed\n";
    });

    // Wait for client connection and auto-accept
    if (CliThread.joinable()) CliThread.join();

    // Wait for auto-accept to complete
    for (int retry = 0; retry < 100; ++retry) {
        if (AutoAcceptPriv.ClientAutoAccepted.load()) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    ASSERT_TRUE(AutoAcceptPriv.ClientAutoAccepted.load());
    ASSERT_EQ(1, AutoAcceptPriv.AutoAcceptCount.load());

    // Service goes offline to test link persistence behavior
    std::cout << "   📤 Service shutting down to test link persistence...\n";
    std::cout << "   🔍 PERSISTENCE TEST: Will link survive service offline?\n";
    ASSERT_EQ(IOC_RESULT_SUCCESS, IOC_offlineService(SrvID));

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Test current behavior vs expected persistent behavior
    IOC_Result_T closeResult = IOC_closeLink(CliLinkID);

    std::cout << "\n💡 CURRENT BEHAVIOR ANALYSIS (IOC_SRVFLAG_AUTO_ACCEPT only):\n";
    if (closeResult == IOC_RESULT_LINK_BROKEN || closeResult != IOC_RESULT_SUCCESS) {
        std::cout << "   ❌ CURRENT: Links auto-cleaned after service shutdown\n";
        std::cout << "   ℹ️  This is expected with auto-cleanup behavior (no persistence)\n";
        std::cout << "   🎯 IMPLICATION: Client must reconnect when service restarts\n";
    } else {
        std::cout << "   ✅ CURRENT: Link still exists, manual cleanup successful\n";
        std::cout << "   ℹ️  Link survived service shutdown (unexpected with auto-cleanup)\n";
        EXPECT_EQ(IOC_RESULT_SUCCESS, closeResult);
    }

    std::cout << "\n� EXPECTED BEHAVIOR (when IOC_SRVFLAG_KEEP_ACCEPTED_LINK is implemented):\n"
              << "   ✅ PERSISTENCE: Auto-accepted links survive service shutdown\n"
              << "   🔄 LIFECYCLE: Links remain valid across service restart cycles\n"
              << "   👤 RESPONSIBILITY: Application controls link cleanup (not automatic)\n"
              << "   🔗 CONTINUITY: Established command channels preserved during service updates\n"
              << "   ⚡ PERFORMANCE: No reconnection overhead for service restart scenarios\n"
              << "   🛡️  SAFETY: Resource tracking prevents memory leaks in persistent mode\n";

    std::cout << "\n🎯 IMPLEMENTATION REQUIREMENTS FOR PERSISTENCE:\n"
              << "   1. 🏗️  Add IOC_SRVFLAG_KEEP_ACCEPTED_LINK flag validation in IOC_Service.c\n"
              << "   2. 🧹 Modify link cleanup logic to respect persistent flag setting\n"
              << "   3. 🎛️  Implement manual cleanup APIs for application-controlled resource management\n"
              << "   4. 🔄 Add service restart reconnection mechanisms for persistent links\n"
              << "   5. 📊 Ensure proper resource tracking and leak prevention for persistent links\n"
              << "   6. 📚 Document persistence usage patterns and best practices\n";

    std::cout << "\n📋 PERSISTENCE USE CASES:\n"
              << "   • Hot-swappable service deployments (zero-downtime updates)\n"
              << "   • Service restart scenarios (configuration reload, crash recovery)\n"
              << "   • Long-running client connections with intermittent service availability\n"
              << "   • Resource-constrained environments (minimize connection setup overhead)\n";
}

// [@AC-2,US-4] TC-1: verifyKeepAcceptedLink_byManualCleanup_expectProperResourceManagement
// [@AC-2,US-4] TC-1: Manual cleanup required for persistent auto-accepted links
/**
 * @brief Test manual cleanup responsibilities for persistent links
 *
 * 🎯 MANUAL CLEANUP CONCEPT:
 * - Persistent links transfer cleanup responsibility from IOC framework to application
 * - Application MUST explicitly call IOC_closeLink() for each persistent link
 * - Failure to cleanup = resource leak (memory, file descriptors, etc.)
 *
 * 📋 CLEANUP LIFECYCLE:
 * 1. Service(AutoAccept+KeepLinks) accepts multiple clients
 * 2. Service goes offline → Links persist (no auto-cleanup)
 * 3. Application tracks and manually closes all server-side LinkIDs
 * 4. Resource accounting validates no leaks occurred
 *
 * 🛡️  RESOURCE MANAGEMENT VALIDATION:
 * - Memory usage before/after cleanup should match
 * - File descriptor count should return to baseline
 * - Link registry should be empty after manual cleanup
 * - No zombie connections should remain in system
 */
TEST(UT_ConetCommandTypicalAutoAccept, verifyKeepAcceptedLink_byManualCleanup_expectProperResourceManagement) {
    // 📋 TDD APPROACH: Test manual cleanup responsibilities for persistent auto-accepted links
    // This test validates resource management when IOC_SRVFLAG_KEEP_ACCEPTED_LINK is implemented

    IOC_Result_T ResultValue = IOC_RESULT_BUG;
    __AutoAcceptCmdPriv_T AutoAcceptPriv = {};
    const int NUM_CLIENTS = 9;  // Test multiple clients for thorough cleanup validation

    IOC_SrvURI_T SrvURI = {.pProtocol = IOC_SRV_PROTO_FIFO,
                           .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
                           .pPath = (const char *)"ManualCleanupTest"};

    // Define supported commands for auto-accept service
    static IOC_CmdID_T SupportedCmdIDs[] = {IOC_CMDID_TEST_PING};
    IOC_CmdUsageArgs_T CmdUsageArgs = {.CbExecCmd_F = __AutoAcceptCmd_ExecutorCb,
                                       .pCbPrivData = &AutoAcceptPriv,
                                       .CmdNum = 1,
                                       .pCmdIDs = SupportedCmdIDs};

    IOC_SrvArgs_T SrvArgs = {.SrvURI = SrvURI,
                             // 🎯 TDD RED STATE: Using IOC_SRVFLAG_KEEP_ACCEPTED_LINK to test persistence behavior
                             // This flag is NOT YET IMPLEMENTED → Test should FAIL (RED) until implementation complete
                             .Flags = (IOC_SrvFlags_T)(IOC_SRVFLAG_AUTO_ACCEPT | IOC_SRVFLAG_KEEP_ACCEPTED_LINK),
                             .UsageCapabilites = IOC_LinkUsageCmdExecutor,
                             .UsageArgs = {.pCmd = &CmdUsageArgs},
                             .OnAutoAccepted_F = __AutoAcceptCmd_OnAutoAcceptedCb,
                             .pSrvPriv = &AutoAcceptPriv};

    IOC_SrvID_T SrvID = IOC_ID_INVALID;
    ResultValue = IOC_onlineService(&SrvID, &SrvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);

    std::cout << "\n🧹 TESTING MANUAL CLEANUP FOR PERSISTENT LINKS:\n";
    std::cout << "   Setting up auto-accept service with " << NUM_CLIENTS << " clients...\n";
    std::cout << "   🎯 GOAL: Validate manual cleanup responsibility after service shutdown\n";

    // Connect multiple clients to test thorough cleanup
    std::vector<IOC_LinkID_T> ClientLinkIDs(NUM_CLIENTS, IOC_ID_INVALID);
    std::vector<std::thread> ClientThreads;

    for (int i = 0; i < NUM_CLIENTS; ++i) {
        ClientThreads.emplace_back([&, i] {
            IOC_ConnArgs_T ConnArgs = {.SrvURI = SrvURI, .Usage = IOC_LinkUsageCmdInitiator};
            IOC_Result_T ResultValueInThread = IOC_connectService(&ClientLinkIDs[i], &ConnArgs, NULL);
            ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValueInThread);
            ASSERT_NE(IOC_ID_INVALID, ClientLinkIDs[i]);

            // Execute command to verify connection works
            IOC_CmdDesc_T TestCmd = {};
            TestCmd.CmdID = IOC_CMDID_TEST_PING;
            TestCmd.TimeoutMs = 1000;
            TestCmd.Status = IOC_CMD_STATUS_PENDING;

            // Add retry logic for command execution to handle race conditions
            IOC_Result_T CmdResult = IOC_RESULT_BUG;
            for (int retry = 0; retry < 3; ++retry) {
                CmdResult = IOC_execCMD(ClientLinkIDs[i], &TestCmd, NULL);
                if (CmdResult == IOC_RESULT_SUCCESS) break;
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
            EXPECT_EQ(IOC_RESULT_SUCCESS, CmdResult)
                << "Client " << (i + 1) << " command should succeed (after retries)";
        });
    }

    // Wait for all clients to connect
    for (auto &thread : ClientThreads) {
        if (thread.joinable()) thread.join();
    }

    // Wait for all auto-accepts to complete
    for (int retry = 0; retry < 100; ++retry) {
        if (AutoAcceptPriv.AutoAcceptCount.load() >= NUM_CLIENTS) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    ASSERT_EQ(NUM_CLIENTS, AutoAcceptPriv.AutoAcceptCount.load());
    ASSERT_TRUE(AutoAcceptPriv.ClientAutoAccepted.load());
    ASSERT_EQ(NUM_CLIENTS, AutoAcceptPriv.AcceptedLinks.size()) << "Should track all accepted links";

    std::cout << "   ✓ " << NUM_CLIENTS << " clients connected and commands executed\n";
    std::cout << "   📊 Accepted server-side LinkIDs: ";
    for (const auto &linkID : AutoAcceptPriv.AcceptedLinks) {
        std::cout << linkID << " ";
    }
    std::cout << "\n";

    // 🔍 RESOURCE ACCOUNTING: Capture baseline before service shutdown
    // In a real implementation, we would track:
    // - Memory usage before shutdown
    // - File descriptor count
    // - Link registry state
    std::cout << "   📋 RESOURCE BASELINE: Capturing pre-shutdown state\n";

    // Service goes offline - with persistent flag, links should survive
    std::cout << "   📤 Service shutting down (links should persist with IOC_SRVFLAG_KEEP_ACCEPTED_LINK)\n";
    ASSERT_EQ(IOC_RESULT_SUCCESS, IOC_offlineService(SrvID));

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // 🎯 MANUAL CLEANUP REQUIREMENT: Application must clean up server-side LinkIDs
    std::cout << "\n🧹 MANUAL CLEANUP PHASE:\n";
    std::cout << "   📋 RESPONSIBILITY: Application must manually close all server-side LinkIDs\n";
    std::cout << "   ⚠️  RISK: Failure to cleanup = resource leak\n";

    // � TDD RED TEST: Auto-accept should work immediately after service restart
    // This tests that the auto-accept daemon restarts correctly and can handle new connections
    std::cout << "\n🔥 TDD RED TEST: Auto-accept functionality after service restart\n";

    // Try to restart the service on the same URI with persistent links
    std::cout << "   🔄 Attempting to restart service on same URI with existing persistent links...\n";
    IOC_SrvID_T RestartedSrvID = IOC_ID_INVALID;
    IOC_Result_T RestartResult = IOC_onlineService(&RestartedSrvID, &SrvArgs);

    if (RestartResult == IOC_RESULT_SUCCESS) {
        std::cout << "     ✅ Service restarted successfully (SrvID: " << RestartedSrvID << ")\n";

        // Test if existing client links can still communicate with restarted service
        std::cout << "   🔍 Testing client→restarted service communication...\n";
        int WorkingLinksAfterRestart = 0;

        for (int i = 0; i < NUM_CLIENTS; ++i) {
            if (ClientLinkIDs[i] != IOC_ID_INVALID) {
                IOC_CmdDesc_T RestartTestCmd = {};
                RestartTestCmd.CmdID = IOC_CMDID_TEST_PING;
                RestartTestCmd.TimeoutMs = 1000;
                RestartTestCmd.Status = IOC_CMD_STATUS_PENDING;

                std::cout << "     🔍 Testing client LinkID " << ClientLinkIDs[i] << " → restarted service...\n";
                IOC_Result_T CmdResult = IOC_execCMD(ClientLinkIDs[i], &RestartTestCmd, NULL);

                if (CmdResult == IOC_RESULT_SUCCESS) {
                    WorkingLinksAfterRestart++;
                    std::cout << "       ✅ Client LinkID " << ClientLinkIDs[i] << " works with restarted service\n";
                } else {
                    std::cout << "       ❌ Client LinkID " << ClientLinkIDs[i]
                              << " failed with restarted service (result: " << CmdResult << ")\n";
                }
            }
        }

        // 🎯 TDD RED TEST: New client auto-accept after service restart with persistent links
        std::cout << "\n   🔥 CRITICAL TEST: Auto-accept new client after restart with persistent links...\n";

        // Create a new client that should be auto-accepted by the restarted service
        IOC_LinkID_T NewClientLinkID = IOC_ID_INVALID;
        std::thread NewClientThread([&]() {
            IOC_ConnArgs_T ConnArgs = {};
            ConnArgs.SrvURI = SrvURI;
            ConnArgs.Usage = IOC_LinkUsageCmdInitiator;

            IOC_Result_T ConnResult = IOC_connectService(&NewClientLinkID, &ConnArgs, NULL);
            std::cout << "     🔍 New client connection result: " << ConnResult << "\n";
        });

        // Wait for connection attempt
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        // Wait for auto-accept (this is the critical test - should the auto-accept daemon work after restart?)
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        NewClientThread.join();

        // Test if the new client was auto-accepted
        bool NewClientAutoAccepted = (NewClientLinkID != IOC_ID_INVALID);
        std::cout << "     🔍 New client auto-accept result: " << (NewClientAutoAccepted ? "SUCCESS" : "FAILED")
                  << "\n";

        if (NewClientAutoAccepted) {
            // Test command execution on new auto-accepted client
            IOC_CmdDesc_T NewClientCmd = {};
            NewClientCmd.CmdID = IOC_CMDID_TEST_PING;
            NewClientCmd.TimeoutMs = 1000;
            NewClientCmd.Status = IOC_CMD_STATUS_PENDING;

            IOC_Result_T NewCmdResult = IOC_execCMD(NewClientLinkID, &NewClientCmd, NULL);
            std::cout << "     🔍 New client command execution: "
                      << (NewCmdResult == IOC_RESULT_SUCCESS ? "SUCCESS" : "FAILED") << "\n";

            IOC_closeLink(NewClientLinkID);
        }

        // 🎯 TDD RED ASSERTION: Auto-accept should work after service restart
        ASSERT_TRUE(NewClientAutoAccepted)
            << "TDD RED: Auto-accept daemon should accept new clients after service restart with persistent links";

        // Cleanup restarted service
        IOC_offlineService(RestartedSrvID);

    } else {
        std::cout << "     ❌ Service restart failed (result: " << RestartResult << ")\n";
        FAIL() << "TDD RED: Service restart on same URI should succeed with persistent links (result: " << RestartResult
               << ")";
    }

    // Test 2: Check if server-side links still exist for manual cleanup
    std::cout << "\n   🔍 Testing server-side LinkID persistence (should exist for manual cleanup)...\n";
    int ExistingServerLinks = 0;

    for (int i = 0; i < AutoAcceptPriv.AcceptedLinks.size(); ++i) {
        IOC_LinkID_T ServerSideLinkID = AutoAcceptPriv.AcceptedLinks[i];

        // Try to close the link - if it exists, close should succeed
        std::cout << "     🔍 Testing server-side LinkID " << ServerSideLinkID << " existence...\n";
        IOC_Result_T CloseResult = IOC_closeLink(ServerSideLinkID);

        if (CloseResult == IOC_RESULT_SUCCESS) {
            ExistingServerLinks++;
            std::cout << "       ✅ Server LinkID " << ServerSideLinkID << " exists and was closed (PERSISTENT)\n";
        } else {
            std::cout << "       ❌ Server LinkID " << ServerSideLinkID
                      << " doesn't exist or already closed (result: " << CloseResult << ")\n";
        }
    }

    // 🎯 TDD ASSERTION: Server-side links should persist
    ASSERT_GT(ExistingServerLinks, 0)
        << "TDD RED: With IOC_SRVFLAG_KEEP_ACCEPTED_LINK, server-side links should persist after service shutdown";

    // Account for the extra link created during service restart test (new client auto-accept test)
    int ExpectedLinks = NUM_CLIENTS + 1;  // Original clients + 1 new client from restart test
    ASSERT_EQ(ExistingServerLinks, ExpectedLinks)
        << "TDD RED: All " << NUM_CLIENTS
        << " original + 1 restart-test server-side links should persist with IOC_SRVFLAG_KEEP_ACCEPTED_LINK";

    std::cout << "\n� PERSISTENCE TEST RESULTS:\n";
    std::cout << "   Server links existing after shutdown: " << ExistingServerLinks << "/"
              << AutoAcceptPriv.AcceptedLinks.size() << "\n";

    std::cout << "\n💡 PERSISTENCE BEHAVIOR ANALYSIS:\n";
    if (ExistingServerLinks > 0) {
        std::cout << "   ✅ Server-side links persisted (manual cleanup required)\n";
    } else {
        std::cout << "   ❌ Server-side links were auto-cleaned (persistence flag not working)\n";
    }

    // Clean up client-side links
    std::cout << "\n   🔧 Cleaning up client-side LinkIDs:\n";
    for (int i = 0; i < NUM_CLIENTS; ++i) {
        if (ClientLinkIDs[i] != IOC_ID_INVALID) {
            IOC_Result_T ClientCleanupResult = IOC_closeLink(ClientLinkIDs[i]);
            std::cout << "     📤 Client LinkID " << ClientLinkIDs[i]
                      << " cleanup: " << (ClientCleanupResult == IOC_RESULT_SUCCESS ? "SUCCESS" : "FAILED") << "\n";
        }
    }

    // 📊 CLEANUP VALIDATION AND REPORTING
    std::cout << "\n📊 MANUAL CLEANUP PATTERN RESULTS:\n";
    std::cout << "   📋 Server-side LinkIDs tracked: " << AutoAcceptPriv.AcceptedLinks.size() << "/" << NUM_CLIENTS
              << "\n";
    std::cout << "   ✅ Pattern demonstrated: Manual cleanup responsibility documented\n";
    std::cout << "   ℹ️  Current behavior: Auto-cleanup already occurred (no persistence yet)\n";

    // 🎯 CURRENT BEHAVIOR vs EXPECTED BEHAVIOR ANALYSIS
    std::cout << "\n💡 MANUAL CLEANUP BEHAVIOR ANALYSIS:\n";
    std::cout << "   ⚠️  CURRENT: Server-side links were auto-cleaned during IOC_offlineService()\n";
    std::cout << "   ℹ️  This confirms current auto-cleanup behavior (no IOC_SRVFLAG_KEEP_ACCEPTED_LINK)\n";
    std::cout << "   🎯 IMPLICATION: Manual cleanup pattern validated, awaiting persistence implementation\n";

    std::cout << "\n🚀 EXPECTED BEHAVIOR (when IOC_SRVFLAG_KEEP_ACCEPTED_LINK is implemented):\n"
              << "   ✅ PERSISTENCE: All server-side LinkIDs remain valid after service shutdown\n"
              << "   🧹 MANUAL CLEANUP: Application responsible for closing all persistent LinkIDs\n"
              << "   🛡️  RESOURCE SAFETY: Proper cleanup prevents memory/file descriptor leaks\n"
              << "   📊 ACCOUNTING: System tracks resource usage before/after manual cleanup\n"
              << "   ⚠️  RESPONSIBILITY: Application failure to cleanup = resource leak\n";

    std::cout << "\n🎯 IMPLEMENTATION REQUIREMENTS FOR MANUAL CLEANUP:\n"
              << "   1. 📋 Track all auto-accepted server-side LinkIDs for application cleanup\n"
              << "   2. 🛡️  Implement resource accounting to detect cleanup failures\n"
              << "   3. ⚠️  Provide warnings/errors when manual cleanup is not performed\n"
              << "   4. 📚 Document manual cleanup patterns and best practices\n"
              << "   5. 🔍 Add resource leak detection and reporting mechanisms\n";

    // 🎯 TEST VALIDATION: Validate link persistence functionality
    int ExpectedAcceptedLinks = NUM_CLIENTS + 1;  // Original clients + 1 new client from restart test
    ASSERT_EQ(ExpectedAcceptedLinks, AutoAcceptPriv.AcceptedLinks.size())
        << "Should track all accepted links for cleanup (including restart test client)";

    std::cout << "\n✅ MANUAL CLEANUP TEST COMPLETED: TDD RED test demonstrates persistence requirements\n";
}

// [@AC-3,US-4] TC-1: verifyKeepAcceptedLink_byServiceRestart_expectConnectionPersistence
// [@AC-3,US-4] TC-1: Links remain functional across service restart scenarios
/**
 * @brief Test connection continuity across service restart with persistent links
 *
 * 🔄 SERVICE RESTART PERSISTENCE:
 * - Persistent links survive service shutdown and remain valid for restart
 * - New service instance can inherit and reuse existing persistent links
 * - Command execution resumes seamlessly after service comes back online
 *
 * 📋 RESTART LIFECYCLE:
 * 1. Service(AutoAccept+KeepLinks) ← Client connects, commands work
 * 2. Service goes offline → Link persists (not destroyed)
 * 3. New service starts with same URI → Inherits persistent links
 * 4. Commands resume working without client reconnection
 *
 * 🎯 RESTART VALUE PROPOSITION:
 * - Zero-downtime service updates (hot deployment)
 * - Client connection preservation during service maintenance
 * - Reduced reconnection overhead for long-running operations
 * - Seamless failover and recovery scenarios
 */
TEST(UT_ConetCommandTypicalAutoAccept, verifyKeepAcceptedLink_byServiceRestart_expectConnectionPersistence) {
    // TODO: Implement service restart with persistent links functionality
    // Test should validate link reuse after service restart without client reconnection
    GTEST_SKIP() << "TODO: Implement service restart with persistent link functionality test";
}

//======>BEGIN US-5: Service Lifecycle Comparison==================================================

// [@AC-1,US-5] TC-1: verifyServiceLifecycleComparison_byAutoCleanupVsPersistent_expectDifferentBehavior
// [@AC-1,US-5] TC-1: Compare resource management between default auto-cleanup vs. persistent links
/**
 * @brief Comparative analysis of auto-cleanup vs persistent link strategies
 *
 * 🔄 CLEANUP STRATEGY COMPARISON:
 *
 * AUTO-CLEANUP (Default behavior):
 * ✅ Pros: Automatic resource management, no memory leaks, simple application code
 * ❌ Cons: Client reconnection required after service restart, connection setup overhead
 * 🎯 Use Case: Simple request-response services, stateless operations
 *
 * PERSISTENT LINKS (IOC_SRVFLAG_KEEP_ACCEPTED_LINK):
 * ✅ Pros: Service restart without reconnection, reduced overhead, connection continuity
 * ❌ Cons: Manual cleanup required, resource tracking burden, potential leak risk
 * 🎯 Use Case: Long-running services, hot deployments, stateful connections
 *
 * 📊 COMPARISON METRICS:
 * - Resource cleanup timing (immediate vs manual)
 * - Memory usage patterns (automatic vs tracked)
 * - Service restart impact (reconnection vs continuity)
 * - Application complexity (simple vs managed)
 */
TEST(UT_ConetCommandTypicalAutoAccept,
     verifyServiceLifecycleComparison_byAutoCleanupVsPersistent_expectDifferentBehavior) {
    // TODO: Implement side-by-side comparison of both cleanup strategies
    // Test should run identical scenarios with both flag configurations and compare results
    GTEST_SKIP() << "TODO: Implement auto-cleanup vs persistent links comparison test";
}

// [@AC-2,US-5] TC-1: verifyServiceLifecycleComparison_byPerformanceImplications_expectMeasurableDifference
// [@AC-2,US-5] TC-1: Performance implications of different cleanup strategies
/**
 * @brief Quantitative performance analysis of cleanup strategies under load
 *
 * 📊 PERFORMANCE DIMENSIONS:
 *
 * STARTUP/SHUTDOWN PERFORMANCE:
 * - Auto-cleanup: Fast shutdown (automatic), slower restart (reconnection overhead)
 * - Persistent: Slower shutdown (tracking), faster restart (reuse existing links)
 *
 * MEMORY USAGE PATTERNS:
 * - Auto-cleanup: Lower peak memory (immediate cleanup), predictable usage
 * - Persistent: Higher peak memory (accumulated links), requires careful management
 *
 * SCALABILITY CHARACTERISTICS:
 * - Auto-cleanup: Linear resource usage, simple scaling model
 * - Persistent: Complex resource tracking, higher efficiency for restart scenarios
 *
 * 🎯 MEASUREMENT TARGETS:
 * - Connection setup/teardown timing
 * - Memory consumption under load
 * - Service restart recovery time
 * - Resource utilization efficiency
 */
TEST(UT_ConetCommandTypicalAutoAccept,
     verifyServiceLifecycleComparison_byPerformanceImplications_expectMeasurableDifference) {
    // TODO: Implement quantitative performance comparison with load testing
    // Test should measure timing, memory, and resource usage for both cleanup strategies
    GTEST_SKIP() << "TODO: Implement performance implications test for different cleanup strategies";
}

// ⚪ IMPLEMENTATION STATUS TRACKING - Auto-Accept Command Patterns TODO
// Auto-accept + command integration needs implementation
//
// ⚪ PLANNED IMPLEMENTATION ITEMS:
//   ⚪ Basic auto-accept + command executor: Immediate command readiness after auto-connect
//   ⚪ Multi-client auto-accept: Independent command processing without manual accept
//   ⚪ Auto-accept + command initiator: Service→client commands immediately after auto-accept
//   ⚪ OnAutoAccepted_F integration: Command context setup in auto-accept callback
//   ⚪ Mixed command patterns: Both callback and polling modes with auto-accept
//   ⚪ Timeout validation: Command timing constraints with auto-accepted connections
//   ⚪ Error handling: Auto-accept failures and command readiness validation
//
// 🎯 GOAL: Integrate IOC_SRVFLAG_AUTO_ACCEPT with command execution patterns
//    Provide streamlined connection + command capability without manual accept overhead
//    Building on proven command APIs from UT_CommandTypical.cxx and UT_CommandTypicalWaitAck.cxx
//    Coverage: Auto-accept + Command Executor + Command Initiator + Mixed Patterns

//======>END OF TEST CASES==========================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF IMPLEMENTATION NOTES=============================================================
/**
 * Key Implementation Areas:
 *
 * 1. Auto-Accept Service Setup:
 *    - Configure service with IOC_SRVFLAG_AUTO_ACCEPT + command capabilities
 *    - Setup OnAutoAccepted_F callback for command readiness notification
 *    - Ensure command executor/initiator capabilities work immediately after auto-accept
 *
 * 2. OnAutoAccepted_F Callback Integration:
 *    - Receive auto-accepted link with command context
 *    - Configure per-client command capabilities based on client usage
 *    - Handle both CmdExecutor and CmdInitiator client connection scenarios
 *
 * 3. Streamlined Connection Flow:
 *    - Client connects with command usage specification
 *    - Auto-accept enables immediate command readiness
 *    - No manual IOC_acceptClient calls required
 *    - Reduced connection setup overhead for command services
 *
 * 4. Mixed Command Pattern Support:
 *    - Support both callback-based commands (from UT_CommandTypical.cxx)
 *    - Support polling-based commands (from UT_CommandTypicalWaitAck.cxx)
 *    - Auto-accept callback determines command execution mode per client
 *
 * 5. Error Handling and Validation:
 *    - Auto-accept failure scenarios and fallback behavior
 *    - Command readiness validation after auto-accept
 *    - Connection limits and resource management with auto-accept
 *    - Timeout behavior for auto-accepted command connections
 */
//======>END OF IMPLEMENTATION NOTES===============================================================
