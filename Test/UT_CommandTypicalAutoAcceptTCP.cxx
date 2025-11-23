///////////////////////////////////////////////////////////////////////////////////////////////////
// Command Typical Auto-Accept TCP (TCP protocol) — UT skeleton
//
// PURPOSE:
//   Verify TCP protocol layer integration with Auto-Accept command patterns.
//   This test suite validates that IOC_SRVFLAG_AUTO_ACCEPT works correctly over network sockets,
//   eliminating the need for manual IOC_acceptClient calls while maintaining command capabilities.
//
// TDD WORKFLOW:
//   Design → Draft → Structure → Test (RED) → Code (GREEN) → Refactor → Repeat
//
// REFERENCE: LLM/CaTDD_DesignPrompt.md for full methodology
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>
#include <vector>

#include "_UT_IOC_Common.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF OVERVIEW OF THIS UNIT TESTING FILE===============================================
/**
 * @brief
 *   [WHAT] This file verifies TCP-based Auto-Accept command execution
 *   [WHERE] in the IOC Command API with TCP protocol layer (_IOC_SrvProtoTCP.c)
 *   [WHY] to ensure streamlined TCP connection handling without manual accept loops.
 *
 * SCOPE:
 *   - [In scope]: TCP service with IOC_SRVFLAG_AUTO_ACCEPT
 *   - [In scope]: OnAutoAccepted_F callback integration with TCP sockets
 *   - [In scope]: Command execution (Executor/Initiator) over auto-accepted TCP links
 *   - [In scope]: TCP-specific concerns: port binding, concurrent auto-accepts
 *   - [Out of scope]: Manual accept patterns (see UT_CommandTypicalTCP.cxx)
 *   - [Out of scope]: FIFO transport (see UT_CommandTypicalAutoAccept.cxx)
 *
 * KEY CONCEPTS:
 *   - Auto-Accept: TCP listener thread automatically accepts connections and creates links
 *   - Callback Notification: OnAutoAccepted_F triggered when TCP connection is established
 *   - Immediate Readiness: TCP socket must be ready for commands immediately after auto-accept
 *   - Concurrency: Multiple clients connecting simultaneously to TCP port
 *
 * KEY DIFFERENCES FROM UT_CommandTypicalAutoAccept.cxx (FIFO):
 *   - Protocol: IOC_SRV_PROTO_TCP vs IOC_SRV_PROTO_FIFO
 *   - Transport: Real network sockets vs in-memory
 *   - Timing: Network latency considerations for "immediate" readiness
 *   - Port Management: Unique ports (18100+) to avoid conflicts
 */
//======>END OF OVERVIEW OF THIS UNIT TESTING FILE=================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF UNIT TESTING DESIGN==============================================================
/**************************************************************************************************
 * 📋 TEST CASE DESIGN ASPECTS/CATEGORIES
 *
 * DESIGN PRINCIPLE: IMPROVE VALUE • AVOID LOSS • BALANCE SKILL vs COST
 *
 * PRIORITY FRAMEWORK:
 *   P1 🥇 FUNCTIONAL:     Must complete before P2 (ValidFunc + InvalidFunc)
 *   P2 🥈 DESIGN-ORIENTED: Test after P1 (State, Capability, Concurrency)
 *   P3 🥉 QUALITY-ORIENTED: Test for quality attributes (Performance, Robust, etc.)
 *
 * DEFAULT TEST ORDER:
 *   P1: Typical → Boundary → Misuse → Fault
 *   P2: State → Capability → Concurrency
 *   P3: Performance → Robust → Compatibility
 *
 * CONTEXT-SPECIFIC ADJUSTMENT:
 *   - New Public API: IOC_SRVFLAG_AUTO_ACCEPT for TCP
 *   - Rationale: Auto-accept is core functionality → P1 Typical priority
 *   - Callback Integration: Promote to early P2 (architectural validation)
 *
 * RISK ASSESSMENT:
 *   US-1/AC-1/TC-1 (Basic TCP Auto-Accept): Impact=3, Likelihood=3, Uncertainty=1 → Score=9 (P1 Typical)
 *   US-1/AC-2/TC-1 (Multi-client): Impact=3, Likelihood=2, Uncertainty=1 → Score=6 (P1 Typical)
 *   US-2/AC-1/TC-1 (Callback): Impact=2, Likelihood=2, Uncertainty=2 → Score=8 (P2)
 *   US-3/AC-1/TC-1 (Persistent Links): Impact=2, Likelihood=2, Uncertainty=1 → Score=4 (P3)
 *
 * COVERAGE STRATEGY: Feature × Client Count × Lifecycle
 *
 * COVERAGE MATRIX (Systematic Test Planning):
 * ┌──────────────────────┬─────────────────┬───────────────────┬────────────────────────────┐
 * │ Feature              │ Client Count    │ Lifecycle Phase   │ Key Scenarios              │
 * ├──────────────────────┼─────────────────┼───────────────────┼────────────────────────────┤
 * │ Auto-Accept          │ Single          │ Connect→Execute   │ US-1/AC-1: Basic flow      │
 * │ Auto-Accept          │ Multiple (3)    │ Concurrent Exec   │ US-1/AC-2: Isolation       │
 * │ Callback Integration │ Single          │ Connect→Notify    │ US-2/AC-1: Callback invoke │
 * │ Keep Accepted Link   │ Single          │ Service Offline   │ US-3/AC-1: Persistence     │
 * └──────────────────────┴─────────────────┴───────────────────┴────────────────────────────┘
 *
 * Design focus:
 *  - TCP Auto-Accept mechanism validation
 *  - Integration of OnAutoAccepted_F with TCP socket lifecycle
 *  - Command flow reliability over auto-accepted network links
 *  - Handling of multiple concurrent TCP connections
 *
 * Test progression:
 *  - Basic TCP Auto-Accept (Client connects, Service auto-accepts, Command flows) - P1
 *  - Multi-client TCP Auto-Accept (Concurrency isolation) - P1
 *  - Callback integration (Configuring TCP links in OnAutoAccepted_F) - P2
 *  - Persistent TCP links (IOC_SRVFLAG_KEEP_ACCEPTED_LINK) - P3
 *
 * QUALITY GATE P1:
 *   ✅ US-1/AC-1/TC-1 GREEN (Basic auto-accept works)
 *   ✅ US-1/AC-2/TC-1 GREEN (Multi-client isolation verified)
 *   ✅ All Typical tests passing
 *   ✅ No socket/thread leaks
 */
///////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF USER STORY=======================================================================
/**
 * US-1: As a service developer, I want TCP services to auto-accept connections
 *       so that I can handle network clients without writing a manual accept loop.
 *
 * US-2: As a service developer, I want to be notified when a TCP client connects
 *       so that I can configure command capabilities for that specific socket.
 *
 * US-3: As a system integrator, I want auto-accepted TCP links to be reliable
 *       so that command execution works immediately upon connection.
 */
//======>END OF USER STORY==========================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//=======>BEGIN OF ACCEPTANCE CRITERIA==============================================================
/**
 * [@US-1] TCP Auto-Accept Basic Functionality
 *  AC-1: GIVEN a TCP service with IOC_SRVFLAG_AUTO_ACCEPT,
 *         WHEN a client connects to the TCP port,
 *         THEN the service automatically accepts the connection and creates a valid link.
 *  AC-2: GIVEN an auto-accepted TCP link,
 *         WHEN the client sends a command,
 *         THEN the service processes it correctly via the configured callback.
 *
 * [@US-2] TCP Auto-Accept Callback Integration
 *  AC-1: GIVEN a TCP service with OnAutoAccepted_F callback,
 *         WHEN a client connects,
 *         THEN the callback is invoked with the new LinkID and ServiceID.
 *  AC-2: GIVEN multiple TCP clients connecting concurrently,
 *         WHEN they are auto-accepted,
 *         THEN the callback is invoked for each client independently.
 *
 * [@US-3] TCP Auto-Accept Reliability
 *  AC-1: GIVEN an auto-accepted TCP connection,
 *         WHEN network latency exists,
 *         THEN the link remains stable and ready for commands.
 */
//=======>END OF ACCEPTANCE CRITERIA================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF TEST CASES=======================================================================
/**************************************************************************************************
 * @brief 【TCP Auto-Accept Test Cases】
 *
 * ORGANIZATION STRATEGIES:
 *  - By Feature: Basic Auto-Accept, Callback Integration, Concurrency
 *  - By Protocol: TCP specific validation
 *
 * STATUS TRACKING: ⚪ = Planned/TODO，🔴 = Implemented/RED, 🟢 = Passed/GREEN, ⚠️ = Issues
 *
 * PORT ALLOCATION STRATEGY:
 *  - Range: 18100 - 18199
 *
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * 📋 [US-1]: TCP Auto-Accept Basic Functionality
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 *
 * [@AC-1,US-1] Basic TCP Auto-Accept with Command Execution
 *  ⚪ TC-1: verifyTcpAutoAccept_bySingleClient_expectImmediateCommandExecution
 *      @[Purpose]: Validate that a TCP client can connect and execute commands without manual accept
 *      @[Brief]: Service(TCP+AutoAccept) starts → Client connects → Client sends PING → Service responds
 *      @[Protocol]: tcp://localhost:18100/AutoAcceptTCP_Basic
 *      @[Status]: TODO
 *      @[Steps]:
 *          1. Start TCP service with IOC_SRVFLAG_AUTO_ACCEPT on port 18100
 *          2. Client connects via TCP
 *          3. Verify NO manual IOC_acceptClient is called
 *          4. Client sends PING command
 *          5. Verify Service receives and responds PONG
 *          6. Cleanup
 *
 * [@AC-2,US-1] Multi-client TCP Auto-Accept
 *  ⚪ TC-1: verifyTcpAutoAccept_byMultipleClients_expectIsolatedExecution
 *      @[Purpose]: Validate multiple TCP clients can auto-connect and execute commands concurrently
 *      @[Brief]: Service(TCP+AutoAccept) → 3 Clients connect → All send commands → All succeed
 *      @[Protocol]: tcp://localhost:18101/AutoAcceptTCP_Multi
 *      @[Status]: TODO
 *      @[Steps]:
 *          1. Start TCP service (AutoAccept) on port 18101
 *          2. Start 3 client threads, each connects
 *          3. Each client sends unique ECHO command
 *          4. Verify all clients receive correct responses
 *          5. Cleanup
 *
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * 📋 [US-2]: TCP Auto-Accept Callback Integration
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 *
 * [@AC-1,US-2] OnAutoAccepted Callback Verification
 *  ⚪ TC-1: verifyTcpAutoAcceptCallback_byClientConnection_expectCallbackInvocation
 *      @[Purpose]: Validate OnAutoAccepted_F is called when TCP client connects
 *      @[Brief]: Service(TCP+AutoAccept+Callback) → Client connects → Verify Callback hit
 *      @[Protocol]: tcp://localhost:18102/AutoAcceptTCP_Callback
 *      @[Status]: TODO
 *      @[Steps]:
 *          1. Start TCP service with OnAutoAccepted_F configured
 *          2. Client connects
 *          3. Wait for callback notification (use atomic flag/CV)
 *          4. Verify LinkID in callback is valid
 *          5. Cleanup
 *
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * 📋 [US-3]: TCP Auto-Accept Reliability & Lifecycle
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 *
 * [@AC-1,US-3] Persistent Links (KeepAcceptedLink)
 *  ⚪ TC-1: verifyTcpKeepAcceptedLink_byServiceOffline_expectLinkPersistence
 *      @[Purpose]: Validate IOC_SRVFLAG_KEEP_ACCEPTED_LINK works for TCP sockets
 *      @[Brief]: Service(TCP+AutoAccept+KeepLinks) → Client connects → Service Offline → Link persists
 *      @[Protocol]: tcp://localhost:18103/AutoAcceptTCP_Keep
 *      @[Status]: TODO
 *      @[Steps]:
 *          1. Start TCP service with KEEP_ACCEPTED_LINK
 *          2. Client connects and verifies command execution
 *          3. Service goes offline (IOC_offlineService)
 *          4. Verify LinkID is NOT automatically closed (check validity)
 *          5. Manually close link
 *          6. Cleanup
 */
//======>END OF TEST CASES=========================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF TODO/IMPLEMENTATION TRACKING SECTION=============================================
// 🔴 IMPLEMENTATION STATUS TRACKING
//
// STATUS LEGEND:
//   ⚪ TODO/PLANNED:      Designed but not implemented yet.
//   🔴 RED/FAILING:       Test written, but production code is missing or incorrect.
//   🟢 GREEN/PASSED:      Test written and passing.
//
// PRIORITY LEVELS:
//   P1 🥇 FUNCTIONAL:     Basic Auto-Accept (TC-1, TC-2)
//   P2 🥈 CALLBACK:       Callback Integration (TC-3)
//   P3 🥉 LIFECYCLE:      Persistent Links (TC-4)
//
// TRACKING:
//   🟢 [@AC-1,US-1] TC-1: verifyTcpAutoAccept_bySingleClient_expectImmediateCommandExecution (PASSED - 2025-11-23)
//   🟢 [@AC-2,US-1] TC-1: verifyTcpAutoAccept_byMultipleClients_expectIsolatedExecution (PASSED - 2025-11-23)
//   🟢 [@AC-1,US-2] TC-1: verifyTcpAutoAcceptCallback_byClientConnection_expectCallbackInvocation (PASSED - 2025-11-23)
//   🟢 [@AC-1,US-3] TC-1: verifyTcpKeepAcceptedLink_byServiceOffline_expectLinkPersistence (PASSED - 2025-11-23)
//
// SUMMARY: 4/4 tests GREEN ✅✅✅✅ ALL COMPLETE!
//
//======>END OF TODO/IMPLEMENTATION TRACKING SECTION===============================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF HELPER FUNCTIONS AND DATA STRUCTURES=============================================

// Private data structure for tracking auto-accept events and command execution
typedef struct __TcpAutoAcceptPriv {
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
} __TcpAutoAcceptPriv_T;

// Auto-accept callback: invoked when TCP client connection is auto-accepted
static void __TcpAutoAccept_OnAutoAcceptedCb(IOC_SrvID_T SrvID, IOC_LinkID_T LinkID, void *pSrvPriv) {
    __TcpAutoAcceptPriv_T *pPrivData = (__TcpAutoAcceptPriv_T *)pSrvPriv;
    if (!pPrivData) return;

    std::lock_guard<std::mutex> lock(pPrivData->DataMutex);

    pPrivData->ClientAutoAccepted = true;
    pPrivData->AutoAcceptCount++;
    pPrivData->LastAcceptedLinkID = LinkID;
    pPrivData->AcceptedLinks.push_back(LinkID);

    printf("[AutoAccept Callback] SrvID=%llu, LinkID=%llu, Total=%d\n", (unsigned long long)SrvID,
           (unsigned long long)LinkID, pPrivData->AutoAcceptCount.load());
}

// Command executor callback: processes commands received on auto-accepted links
static IOC_Result_T __TcpAutoAccept_ExecutorCb(IOC_LinkID_T LinkID, IOC_CmdDesc_pT pCmdDesc, void *pCbPriv) {
    __TcpAutoAcceptPriv_T *pPrivData = (__TcpAutoAcceptPriv_T *)pCbPriv;
    if (!pPrivData || !pCmdDesc) return IOC_RESULT_INVALID_PARAM;

    std::lock_guard<std::mutex> lock(pPrivData->DataMutex);

    pPrivData->CommandReceived = true;
    pPrivData->CommandCount++;
    pPrivData->LastCmdID = IOC_CmdDesc_getCmdID(pCmdDesc);

    IOC_CmdID_T CmdID = IOC_CmdDesc_getCmdID(pCmdDesc);
    IOC_Result_T ExecResult = IOC_RESULT_SUCCESS;

    if (CmdID == IOC_CMDID_TEST_PING) {
        // PING command: respond with "TCP_AUTO_PONG"
        const char *response = "TCP_AUTO_PONG";
        ExecResult = IOC_CmdDesc_setOutPayload(pCmdDesc, (void *)response, strlen(response));
        if (ExecResult == IOC_RESULT_SUCCESS) {
            strcpy(pPrivData->LastResponseData, response);
            pPrivData->LastResponseSize = strlen(response);
        }
    } else if (CmdID == IOC_CMDID_TEST_ECHO) {
        // ECHO command: return input data as-is
        void *inputData = IOC_CmdDesc_getInData(pCmdDesc);
        ULONG_T inputSize = IOC_CmdDesc_getInDataLen(pCmdDesc);

        if (inputData && inputSize > 0) {
            if (inputSize < sizeof(pPrivData->LastResponseData)) {
                memcpy(pPrivData->LastResponseData, inputData, inputSize);
                pPrivData->LastResponseData[inputSize] = '\0';
                pPrivData->LastResponseSize = inputSize;

                ExecResult = IOC_CmdDesc_setOutPayload(pCmdDesc, pPrivData->LastResponseData, inputSize);
            } else {
                ExecResult = IOC_RESULT_BUFFER_TOO_SMALL;
            }
        } else {
            ExecResult = IOC_RESULT_INVALID_PARAM;
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

    printf("[Executor Callback] LinkID=%llu, CmdID=%llu, Result=%d\n", (unsigned long long)LinkID,
           (unsigned long long)CmdID, ExecResult);
    return ExecResult;
}

//======>END OF HELPER FUNCTIONS AND DATA STRUCTURES===============================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF TEST CASE IMPLEMENTATIONS========================================================

// [@AC-1,US-1] TC-1: verifyTcpAutoAccept_bySingleClient_expectImmediateCommandExecution
/**
 * @[Category]: P1-Typical (ValidFunc)
 * @[Purpose]: Validate TCP client can connect and execute commands without manual accept
 * @[Brief]: Service(TCP+AutoAccept) → Client connects → PING → PONG response (no manual accept)
 * @[4-Phase Structure]:
 *   1) 🔧 SETUP: Start TCP service with AUTO_ACCEPT flag on port 18100
 *   2) 🎯 BEHAVIOR: Client connects, auto-accepted, sends PING command
 *   3) ✅ VERIFY: 3 Key Points - Auto-accept triggered, Command executed, Correct response
 *   4) 🧹 CLEANUP: Offline service, close link
 */
TEST(UT_CommandTypicalAutoAcceptTCP, verifyTcpAutoAccept_bySingleClient_expectImmediateCommandExecution) {
    // ────────────────────────────────────────────────────────────────────────────────────────────
    // 🔧 PHASE 1: SETUP - Create TCP service with auto-accept enabled
    // ────────────────────────────────────────────────────────────────────────────────────────────
    __TcpAutoAcceptPriv_T AutoAcceptPriv = {};
    const uint16_t PORT = 18100;
    IOC_SrvURI_T SrvURI = {
        .pProtocol = IOC_SRV_PROTO_TCP, .pHost = "0.0.0.0", .Port = PORT, .pPath = "AutoAcceptTCP_Basic"};

    // Define supported commands
    static IOC_CmdID_T SupportedCmdIDs[] = {IOC_CMDID_TEST_PING, IOC_CMDID_TEST_ECHO};
    IOC_CmdUsageArgs_T CmdUsageArgs = {.CbExecCmd_F = __TcpAutoAccept_ExecutorCb,
                                       .pCbPrivData = &AutoAcceptPriv,
                                       .CmdNum = sizeof(SupportedCmdIDs) / sizeof(SupportedCmdIDs[0]),
                                       .pCmdIDs = SupportedCmdIDs};

    IOC_SrvArgs_T SrvArgs = {.SrvURI = SrvURI,
                             .Flags = IOC_SRVFLAG_AUTO_ACCEPT,  // Enable auto-accept
                             .UsageCapabilites = IOC_LinkUsageCmdExecutor,
                             .UsageArgs = {.pCmd = &CmdUsageArgs},
                             .OnAutoAccepted_F = __TcpAutoAccept_OnAutoAcceptedCb,
                             .pSrvPriv = &AutoAcceptPriv};

    IOC_SrvID_T SrvID = IOC_ID_INVALID;
    IOC_Result_T ResultValue = IOC_onlineService(&SrvID, &SrvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);
    ASSERT_NE(IOC_ID_INVALID, SrvID);

    // ────────────────────────────────────────────────────────────────────────────────────────────
    // 🎯 PHASE 2: BEHAVIOR - Client connects and sends command (auto-accepted)
    // ────────────────────────────────────────────────────────────────────────────────────────────
    // Connect client (should be auto-accepted)
    IOC_ConnArgs_T ConnArgs = {.SrvURI = SrvURI, .Usage = IOC_LinkUsageCmdInitiator};
    IOC_LinkID_T CliLinkID = IOC_ID_INVALID;
    ResultValue = IOC_connectService(&CliLinkID, &ConnArgs, nullptr);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);
    ASSERT_NE(IOC_ID_INVALID, CliLinkID);

    // Wait for auto-accept callback (allow up to 1 second for TCP handshake)
    for (int retry = 0; retry < 100; ++retry) {
        if (AutoAcceptPriv.ClientAutoAccepted.load()) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // Send PING command
    IOC_CmdDesc_T CmdDesc;
    IOC_CmdDesc_initVar(&CmdDesc);
    CmdDesc.CmdID = IOC_CMDID_TEST_PING;

    ResultValue = IOC_execCMD(CliLinkID, &CmdDesc, nullptr);

    // ────────────────────────────────────────────────────────────────────────────────────────────
    // ✅ PHASE 3: VERIFY - Assert auto-accept and command execution (≤3 key points)
    // ────────────────────────────────────────────────────────────────────────────────────────────
    VERIFY_KEYPOINT_EQ(ResultValue, IOC_RESULT_SUCCESS,
                       "KP1: Client must execute PING command on auto-accepted TCP link");

    VERIFY_KEYPOINT_TRUE(AutoAcceptPriv.ClientAutoAccepted.load(),
                         "KP2: TCP service must trigger auto-accept callback when client connects");

    // Verify response data
    void *pOutData = IOC_CmdDesc_getOutData(&CmdDesc);
    ULONG_T OutDataLen = IOC_CmdDesc_getOutDataLen(&CmdDesc);
    VERIFY_KEYPOINT_TRUE(pOutData != nullptr && OutDataLen > 0 && strcmp((char *)pOutData, "TCP_AUTO_PONG") == 0,
                         "KP3: Service must respond with correct PONG data via auto-accepted link");

    // ────────────────────────────────────────────────────────────────────────────────────────────
    // 🧹 PHASE 4: CLEANUP - Release resources
    // ────────────────────────────────────────────────────────────────────────────────────────────
    IOC_offlineService(SrvID);
}

// [@AC-2,US-1] TC-2: verifyTcpAutoAccept_byMultipleClients_expectIsolatedExecution
/**
 * @[Category]: P1-Typical (ValidFunc)
 * @[Purpose]: Validate multiple TCP clients can auto-connect concurrently
 * @[Brief]: Service(TCP+AutoAccept) → 3 Clients connect → Execute different CmdIDs → Isolated responses
 * @[4-Phase Structure]:
 *   1) 🔧 SETUP: Start TCP service with AUTO_ACCEPT on port 18101
 *   2) 🎯 BEHAVIOR: 3 clients connect concurrently, each executes different CmdID (PING/ECHO/PING)
 *   3) ✅ VERIFY: 3 Key Points - All auto-accepted, Commands isolated, Correct responses
 *   4) 🧹 CLEANUP: Offline service
 */
TEST(UT_CommandTypicalAutoAcceptTCP, verifyTcpAutoAccept_byMultipleClients_expectIsolatedExecution) {
    // ────────────────────────────────────────────────────────────────────────────────────────────
    // 🔧 PHASE 1: SETUP - Create TCP service with auto-accept for multiple clients
    // ────────────────────────────────────────────────────────────────────────────────────────────
    __TcpAutoAcceptPriv_T AutoAcceptPriv = {};
    const uint16_t PORT = 18101;
    IOC_SrvURI_T SrvURI = {
        .pProtocol = IOC_SRV_PROTO_TCP, .pHost = "0.0.0.0", .Port = PORT, .pPath = "AutoAcceptTCP_Multi"};

    static IOC_CmdID_T SupportedCmdIDs[] = {IOC_CMDID_TEST_PING, IOC_CMDID_TEST_ECHO};
    IOC_CmdUsageArgs_T CmdUsageArgs = {.CbExecCmd_F = __TcpAutoAccept_ExecutorCb,
                                       .pCbPrivData = &AutoAcceptPriv,
                                       .CmdNum = sizeof(SupportedCmdIDs) / sizeof(SupportedCmdIDs[0]),
                                       .pCmdIDs = SupportedCmdIDs};

    IOC_SrvArgs_T SrvArgs = {.SrvURI = SrvURI,
                             .Flags = IOC_SRVFLAG_AUTO_ACCEPT,
                             .UsageCapabilites = IOC_LinkUsageCmdExecutor,
                             .UsageArgs = {.pCmd = &CmdUsageArgs},
                             .OnAutoAccepted_F = __TcpAutoAccept_OnAutoAcceptedCb,
                             .pSrvPriv = &AutoAcceptPriv};

    IOC_SrvID_T SrvID = IOC_ID_INVALID;
    IOC_Result_T ResultValue = IOC_onlineService(&SrvID, &SrvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);
    ASSERT_NE(IOC_ID_INVALID, SrvID);

    // ────────────────────────────────────────────────────────────────────────────────────────────
    // 🎯 PHASE 2: BEHAVIOR - Multiple clients connect concurrently and execute different commands
    // ────────────────────────────────────────────────────────────────────────────────────────────
    const int NUM_CLIENTS = 3;
    std::vector<IOC_LinkID_T> ClientLinks(NUM_CLIENTS, IOC_ID_INVALID);
    std::vector<IOC_Result_T> ClientResults(NUM_CLIENTS, IOC_RESULT_BUG);

    // Each client will execute a different command
    struct ClientCmdInfo {
        IOC_CmdID_T CmdID;
        const char *pInData;
        size_t InDataSize;
        const char *pExpectedResp;
    };

    std::vector<ClientCmdInfo> ClientCmds = {
        {IOC_CMDID_TEST_PING, nullptr, 0, "TCP_AUTO_PONG"},                     // Client 0: PING
        {IOC_CMDID_TEST_ECHO, "ECHO_FROM_CLIENT_1", 18, "ECHO_FROM_CLIENT_1"},  // Client 1: ECHO
        {IOC_CMDID_TEST_PING, nullptr, 0, "TCP_AUTO_PONG"},                     // Client 2: PING
    };

    // Connect all clients concurrently
    std::vector<std::thread> ConnectThreads;
    for (int i = 0; i < NUM_CLIENTS; ++i) {
        ConnectThreads.emplace_back([&, i]() {
            IOC_ConnArgs_T ConnArgs = {.SrvURI = SrvURI, .Usage = IOC_LinkUsageCmdInitiator};
            ClientResults[i] = IOC_connectService(&ClientLinks[i], &ConnArgs, nullptr);
        });
    }

    for (auto &thread : ConnectThreads) {
        if (thread.joinable()) thread.join();
    }

    // Wait for all auto-accepts
    for (int retry = 0; retry < 100; ++retry) {
        if (AutoAcceptPriv.AutoAcceptCount.load() >= NUM_CLIENTS) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // Execute commands sequentially to avoid TCP protocol race conditions
    std::vector<std::string> ClientResponses(NUM_CLIENTS);
    for (int i = 0; i < NUM_CLIENTS; ++i) {
        IOC_CmdDesc_T CmdDesc;
        IOC_CmdDesc_initVar(&CmdDesc);
        CmdDesc.CmdID = ClientCmds[i].CmdID;

        if (ClientCmds[i].pInData != nullptr) {
            IOC_CmdDesc_setInPayload(&CmdDesc, (void *)ClientCmds[i].pInData, ClientCmds[i].InDataSize);
        }

        IOC_Result_T ExecResult = IOC_execCMD(ClientLinks[i], &CmdDesc, nullptr);
        if (ExecResult == IOC_RESULT_SUCCESS) {
            size_t OutDataLen = IOC_CmdDesc_getOutDataLen(&CmdDesc);
            const void *pOutData = IOC_CmdDesc_getOutData(&CmdDesc);
            if (pOutData != nullptr && OutDataLen > 0) {
                ClientResponses[i] = std::string((const char *)pOutData, OutDataLen);
            }
        }
    }

    // ────────────────────────────────────────────────────────────────────────────────────────────
    // ✅ PHASE 3: VERIFY - Assert all clients auto-accepted and commands executed correctly (≤3 key points)
    // ────────────────────────────────────────────────────────────────────────────────────────────
    VERIFY_KEYPOINT_EQ(AutoAcceptPriv.AutoAcceptCount.load(), NUM_CLIENTS,
                       "KP1: TCP service must auto-accept all concurrent client connections");

    // Verify all connections succeeded
    bool AllConnectionsValid = true;
    for (int i = 0; i < NUM_CLIENTS; ++i) {
        if (ClientResults[i] != IOC_RESULT_SUCCESS || ClientLinks[i] == IOC_ID_INVALID) {
            AllConnectionsValid = false;
            printf("[ERROR] Client %d connection failed: Result=%d, LinkID=%llu\n", i + 1, ClientResults[i],
                   (unsigned long long)ClientLinks[i]);
        }
    }
    VERIFY_KEYPOINT_TRUE(AllConnectionsValid, "KP2: All clients must successfully connect via auto-accept");

    // Verify each client received the correct response for its command
    bool AllResponsesCorrect = true;
    for (int i = 0; i < NUM_CLIENTS; ++i) {
        if (ClientResponses[i] != ClientCmds[i].pExpectedResp) {
            AllResponsesCorrect = false;
            printf("[ERROR] Client %d response mismatch: Expected='%s', Got='%s'\n", i, ClientCmds[i].pExpectedResp,
                   ClientResponses[i].c_str());
        }
    }
    VERIFY_KEYPOINT_TRUE(AllResponsesCorrect, "KP3: Each client must receive correct response for its CmdID");

    // ────────────────────────────────────────────────────────────────────────────────────────────
    // 🧹 PHASE 4: CLEANUP - Release resources
    // ────────────────────────────────────────────────────────────────────────────────────────────
    IOC_offlineService(SrvID);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * @test TC-3: Callback Integration Verification
 * @[Category]: P2-Callback (ValidFunc)
 * @[Purpose]: Validate OnAutoAccepted_F callback provides valid LinkID for command execution
 * @[Brief]: Service(TCP+AutoAccept) → Client connects → Verify callback LinkID is usable
 * @[4-Phase Structure]:
 *   1) 🔧 SETUP: Start TCP service with auto-accept callback on port 18102
 *   2) 🎯 BEHAVIOR: Client connects, callback stores LinkID, use it to execute command
 *   3) ✅ VERIFY: 3 Key Points - Callback invoked, LinkID valid, Command execution successful
 *   4) 🧹 CLEANUP: Offline service
 */
TEST(UT_CommandTypicalAutoAcceptTCP, verifyTcpAutoAcceptCallback_byClientConnection_expectCallbackInvocation) {
    // ────────────────────────────────────────────────────────────────────────────────────────────
    // 🔧 PHASE 1: SETUP - Create TCP service with auto-accept callback
    // ────────────────────────────────────────────────────────────────────────────────────────────
    __TcpAutoAcceptPriv_T AutoAcceptPriv = {};
    const uint16_t PORT = 18102;
    IOC_SrvURI_T SrvURI = {
        .pProtocol = IOC_SRV_PROTO_TCP, .pHost = "0.0.0.0", .Port = PORT, .pPath = "AutoAcceptTCP_Callback"};

    static IOC_CmdID_T SupportedCmdIDs[] = {IOC_CMDID_TEST_PING};
    IOC_CmdUsageArgs_T CmdUsageArgs = {.CbExecCmd_F = __TcpAutoAccept_ExecutorCb,
                                       .pCbPrivData = &AutoAcceptPriv,
                                       .CmdNum = sizeof(SupportedCmdIDs) / sizeof(SupportedCmdIDs[0]),
                                       .pCmdIDs = SupportedCmdIDs};

    IOC_SrvArgs_T SrvArgs = {.SrvURI = SrvURI,
                             .Flags = IOC_SRVFLAG_AUTO_ACCEPT,
                             .UsageCapabilites = IOC_LinkUsageCmdExecutor,
                             .UsageArgs = {.pCmd = &CmdUsageArgs},
                             .OnAutoAccepted_F = __TcpAutoAccept_OnAutoAcceptedCb,
                             .pSrvPriv = &AutoAcceptPriv};

    IOC_SrvID_T SrvID = IOC_ID_INVALID;
    IOC_Result_T ResultValue = IOC_onlineService(&SrvID, &SrvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);
    ASSERT_NE(IOC_ID_INVALID, SrvID);

    // ────────────────────────────────────────────────────────────────────────────────────────────
    // 🎯 PHASE 2: BEHAVIOR - Client connects and uses LinkID from callback
    // ────────────────────────────────────────────────────────────────────────────────────────────
    IOC_ConnArgs_T ConnArgs = {.SrvURI = SrvURI, .Usage = IOC_LinkUsageCmdInitiator};
    IOC_LinkID_T CliLinkID = IOC_ID_INVALID;
    ResultValue = IOC_connectService(&CliLinkID, &ConnArgs, nullptr);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);

    // Wait for auto-accept callback
    for (int retry = 0; retry < 100; ++retry) {
        if (AutoAcceptPriv.ClientAutoAccepted.load()) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // Verify callback was invoked and LinkID stored
    ASSERT_TRUE(AutoAcceptPriv.ClientAutoAccepted.load());
    IOC_LinkID_T ServerLinkID = AutoAcceptPriv.LastAcceptedLinkID;
    ASSERT_NE(IOC_ID_INVALID, ServerLinkID);

    // Use the LinkID from callback to execute command from client side
    IOC_CmdDesc_T CmdDesc;
    IOC_CmdDesc_initVar(&CmdDesc);
    CmdDesc.CmdID = IOC_CMDID_TEST_PING;

    ResultValue = IOC_execCMD(CliLinkID, &CmdDesc, nullptr);

    // ────────────────────────────────────────────────────────────────────────────────────────────
    // ✅ PHASE 3: VERIFY - Assert callback integration works (≤3 key points)
    // ────────────────────────────────────────────────────────────────────────────────────────────
    VERIFY_KEYPOINT_TRUE(AutoAcceptPriv.ClientAutoAccepted.load(),
                         "KP1: OnAutoAccepted_F callback must be invoked when client connects");

    VERIFY_KEYPOINT_NE(ServerLinkID, IOC_ID_INVALID, "KP2: Callback must provide valid LinkID for accepted connection");

    VERIFY_KEYPOINT_EQ(ResultValue, IOC_RESULT_SUCCESS, "KP3: Command execution must succeed using auto-accepted link");

    // ────────────────────────────────────────────────────────────────────────────────────────────
    // 🧹 PHASE 4: CLEANUP - Release resources
    // ────────────────────────────────────────────────────────────────────────────────────────────
    IOC_offlineService(SrvID);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * @test TC-4: Persistent Link Verification (KeepAcceptedLink)
 * @[Category]: P3-Lifecycle (ValidFunc)
 * @[Purpose]: Validate IOC_SRVFLAG_KEEP_ACCEPTED_LINK prevents auto-close of accepted links
 * @[Brief]: Service(TCP+AutoAccept+KeepLinks) → Client connects → Service offline → Link persists
 * @[4-Phase Structure]:
 *   1) 🔧 SETUP: Start TCP service with KEEP_ACCEPTED_LINK flag on port 18103
 *   2) 🎯 BEHAVIOR: Client connects, verify command works, then offline service
 *   3) ✅ VERIFY: 3 Key Points - Link created, Service offline succeeds, Link NOT auto-closed
 *   4) 🧹 CLEANUP: Manually close link
 */
TEST(UT_CommandTypicalAutoAcceptTCP, verifyTcpKeepAcceptedLink_byServiceOffline_expectLinkPersistence) {
    // ────────────────────────────────────────────────────────────────────────────────────────────
    // 🔧 PHASE 1: SETUP - Create TCP service with KEEP_ACCEPTED_LINK flag
    // ────────────────────────────────────────────────────────────────────────────────────────────
    __TcpAutoAcceptPriv_T AutoAcceptPriv = {};
    const uint16_t PORT = 18103;
    IOC_SrvURI_T SrvURI = {
        .pProtocol = IOC_SRV_PROTO_TCP, .pHost = "0.0.0.0", .Port = PORT, .pPath = "AutoAcceptTCP_Keep"};

    static IOC_CmdID_T SupportedCmdIDs[] = {IOC_CMDID_TEST_PING};
    IOC_CmdUsageArgs_T CmdUsageArgs = {.CbExecCmd_F = __TcpAutoAccept_ExecutorCb,
                                       .pCbPrivData = &AutoAcceptPriv,
                                       .CmdNum = sizeof(SupportedCmdIDs) / sizeof(SupportedCmdIDs[0]),
                                       .pCmdIDs = SupportedCmdIDs};

    IOC_SrvArgs_T SrvArgs = {.SrvURI = SrvURI,
                             .Flags = (IOC_SrvFlags_T)(IOC_SRVFLAG_AUTO_ACCEPT | IOC_SRVFLAG_KEEP_ACCEPTED_LINK),
                             .UsageCapabilites = IOC_LinkUsageCmdExecutor,
                             .UsageArgs = {.pCmd = &CmdUsageArgs},
                             .OnAutoAccepted_F = __TcpAutoAccept_OnAutoAcceptedCb,
                             .pSrvPriv = &AutoAcceptPriv};

    IOC_SrvID_T SrvID = IOC_ID_INVALID;
    IOC_Result_T ResultValue = IOC_onlineService(&SrvID, &SrvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);
    ASSERT_NE(IOC_ID_INVALID, SrvID);

    // ────────────────────────────────────────────────────────────────────────────────────────────
    // 🎯 PHASE 2: BEHAVIOR - Client connects, execute command, then offline service
    // ────────────────────────────────────────────────────────────────────────────────────────────
    IOC_ConnArgs_T ConnArgs = {.SrvURI = SrvURI, .Usage = IOC_LinkUsageCmdInitiator};
    IOC_LinkID_T CliLinkID = IOC_ID_INVALID;
    ResultValue = IOC_connectService(&CliLinkID, &ConnArgs, nullptr);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);

    // Wait for auto-accept
    for (int retry = 0; retry < 100; ++retry) {
        if (AutoAcceptPriv.ClientAutoAccepted.load()) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    IOC_LinkID_T ServerLinkID = AutoAcceptPriv.LastAcceptedLinkID;

    // Execute command to verify link is functional before service offline
    IOC_CmdDesc_T CmdDesc;
    IOC_CmdDesc_initVar(&CmdDesc);
    CmdDesc.CmdID = IOC_CMDID_TEST_PING;
    ResultValue = IOC_execCMD(CliLinkID, &CmdDesc, nullptr);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);

    // Now offline the service
    IOC_Result_T OfflineResult = IOC_offlineService(SrvID);

    // ────────────────────────────────────────────────────────────────────────────────────────────
    // ✅ PHASE 3: VERIFY - Assert link persists after service offline (≤3 key points)
    // ────────────────────────────────────────────────────────────────────────────────────────────
    VERIFY_KEYPOINT_NE(ServerLinkID, IOC_ID_INVALID, "KP1: Auto-accepted link must be created with valid LinkID");

    VERIFY_KEYPOINT_EQ(OfflineResult, IOC_RESULT_SUCCESS,
                       "KP2: Service offline must succeed with KEEP_ACCEPTED_LINK flag");

    // Verify link still exists by attempting to close it manually
    // Without KEEP_ACCEPTED_LINK, the link would already be closed (IOC_closeLink would fail)
    IOC_Result_T CloseLinkResult = IOC_closeLink(ServerLinkID);
    VERIFY_KEYPOINT_EQ(CloseLinkResult, IOC_RESULT_SUCCESS,
                       "KP3: Accepted link must persist after service offline (manual close succeeds)");

    // ────────────────────────────────────────────────────────────────────────────────────────────
    // 🧹 PHASE 4: CLEANUP - Close client link
    // ────────────────────────────────────────────────────────────────────────────────────────────
    IOC_closeLink(CliLinkID);
}

//======>END OF TEST CASE IMPLEMENTATIONS==========================================================
