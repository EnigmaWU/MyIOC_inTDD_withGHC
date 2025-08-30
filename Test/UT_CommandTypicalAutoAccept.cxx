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
 *  ⚪ TC-1: verifyOnAutoAcceptedCallback_forMixedCmdPatterns_expectFlexibleConfig
 *      @[Purpose]: Validate service supporting BOTH CmdExecutor+CmdInitiator with per-client configuration
 *      @[Brief]: Service(CmdExecutor|CmdInitiator+AutoAccept+Callback) handles:
 *                - Client-A(CmdInitiator) → CLIENT-A→SERVICE commands (US-1 pattern)
 *                - Client-B(CmdExecutor) → SERVICE→CLIENT-B commands (US-2 pattern)
 *                - OnAutoAccepted_F configures each client individually based on Usage type
 *      @[Technical]: Service.UsageCapabilites = IOC_LinkUsageCmdExecutor | IOC_LinkUsageCmdInitiator
 *                    Callback determines per-client command flow based on client's Usage parameter
 *      @[Status]: TODO - Need to implement mixed client types with unified service capability
 *
 * [@AC-3,US-3] Mixed command patterns (callback + polling) with auto-accept callback
 *  ⚪ TC-1: verifyOnAutoAcceptedCallback_forCallbackPlusPolling_expectFlexibleHandling
 *      @[Purpose]: Validate auto-accept callback handling both callback-based and polling command modes
 *      @[Brief]: Callback configures some links for immediate commands, others for polling-based commands
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
 *  ⚪ TC-1: verifyKeepAcceptedLink_byManualCleanup_expectProperResourceManagement
 *      @[Purpose]: Validate manual cleanup responsibility for persistent auto-accepted links
 *      @[Brief]: Service(AutoAccept+KeepLinks) → Multiple clients → Service offline → Manual LinkID cleanup required
 *      @[Status]: TODO - Need to implement manual cleanup patterns for persistent links
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
    // TODO: Implement service with combined CmdExecutor|CmdInitiator capability
    // Service supports BOTH capabilities, callback configures per-client command direction:
    // - Client-A(CmdInitiator) → CLIENT-A→SERVICE commands (US-1 pattern)
    // - Client-B(CmdExecutor) → SERVICE→CLIENT-B commands (US-2 pattern)
    // Key: Service.UsageCapabilites = IOC_LinkUsageCmdExecutor | IOC_LinkUsageCmdInitiator
    GTEST_SKIP() << "TODO: Implement mixed client types with unified service capability test";
}

// [@AC-3,US-3] TC-1: verifyOnAutoAcceptedCallback_byMixedPatterns_expectFlexibleHandling
// [@AC-3,US-3] TC-1: Mixed command execution modes (callback + polling) with auto-accept callback
TEST(UT_ConetCommandTypicalAutoAccept, verifyOnAutoAcceptedCallback_forCallbackPlusPolling_expectFlexibleHandling) {
    // TODO: Implement mixed callback + polling patterns with auto-accept
    GTEST_SKIP() << "TODO: Implement mixed command patterns with auto-accept test";
}

//======>BEGIN US-4: Service Lifecycle with Persistent Links=======================================

// [@AC-1,US-4] TC-1: verifyKeepAcceptedLink_byServiceOffline_expectLinkPersistence
// [@AC-1,US-4] TC-1: Auto-accepted links persist after service offline with IOC_SRVFLAG_KEEP_ACCEPTED_LINK
TEST(UT_ConetCommandTypicalAutoAccept, verifyKeepAcceptedLink_byServiceOffline_expectLinkPersistence) {
    // TODO: Implement IOC_SRVFLAG_KEEP_ACCEPTED_LINK persistence validation
    GTEST_SKIP() << "TODO: Implement auto-accepted link persistence after service offline test";
}

// [@AC-2,US-4] TC-1: verifyKeepAcceptedLink_byManualCleanup_expectProperResourceManagement
// [@AC-2,US-4] TC-1: Manual cleanup required for persistent auto-accepted links
TEST(UT_ConetCommandTypicalAutoAccept, verifyKeepAcceptedLink_byManualCleanup_expectProperResourceManagement) {
    // TODO: Implement manual cleanup patterns for persistent auto-accepted links
    GTEST_SKIP() << "TODO: Implement manual cleanup requirement validation for persistent links test";
}

// [@AC-3,US-4] TC-1: verifyKeepAcceptedLink_byServiceRestart_expectConnectionPersistence
// [@AC-3,US-4] TC-1: Links remain functional across service restart scenarios
TEST(UT_ConetCommandTypicalAutoAccept, verifyKeepAcceptedLink_byServiceRestart_expectConnectionPersistence) {
    // TODO: Implement service restart with persistent links functionality
    GTEST_SKIP() << "TODO: Implement service restart with persistent link functionality test";
}

//======>BEGIN US-5: Service Lifecycle Comparison==================================================

// [@AC-1,US-5] TC-1: verifyServiceLifecycleComparison_byAutoCleanupVsPersistent_expectDifferentBehavior
// [@AC-1,US-5] TC-1: Compare resource management between default auto-cleanup vs. persistent links
TEST(UT_ConetCommandTypicalAutoAccept,
     verifyServiceLifecycleComparison_byAutoCleanupVsPersistent_expectDifferentBehavior) {
    // TODO: Implement comparative resource management analysis
    GTEST_SKIP() << "TODO: Implement auto-cleanup vs persistent links comparison test";
}

// [@AC-2,US-5] TC-1: verifyServiceLifecycleComparison_byPerformanceImplications_expectMeasurableDifference
// [@AC-2,US-5] TC-1: Performance implications of different cleanup strategies
TEST(UT_ConetCommandTypicalAutoAccept,
     verifyServiceLifecycleComparison_byPerformanceImplications_expectMeasurableDifference) {
    // TODO: Implement performance comparison between auto-cleanup vs manual cleanup strategies
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
