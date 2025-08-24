///////////////////////////////////////////////////////////////////////////////////////////////////
// Command Typical (connection-oriented / Conet) — UT skeleton
//
// Intent:
// - "CommandTypical" here explicitly means connection-oriented commands (Conet), not Conles.
// - Focus on P2P link-to-link command execution flows by default (no broadcast mode here).
// - Mirrors the UT template and US/AC structure used across this repo.
// - Default mode: accept+callback (IOC_acceptClient + CbExecCmd_F)
// - Consider: UT_CommandTypicalWaitAck.cxx, UT_CommandTypicalAutoAccept.cxx as future extensions
//
// ⚠️  IMPLEMENTATION STATUS:
//     Command API is DEFINED but NOT YET IMPLEMENTED in the IOC framework.
//     Functions like IOC_execCMD, IOC_waitCMD, IOC_ackCMD are declared but missing implementation.
//     This skeleton provides the COMPLETE TEST DESIGN ready for implementation.
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "_UT_IOC_Common.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF OVERVIEW OF THIS UNIT TESTING FILE===============================================
/**
 * @brief Verify typical, connection-oriented command execution flows (Conet):
 *  - Service and client exchange commands over specific links (P2P),
 *    using link-scoped command execution with callback processing by default.
 *  - Not covering broadcast (SrvID→all clients); that's in UT_ServiceBroadcastCommand.cxx.
 *  - Not covering polling (IOC_waitCMD); that's in UT_CommandTypicalWaitAck.cxx.
 *  - Not covering auto-accept; that's in UT_CommandTypicalAutoAccept.cxx.
 *
 * Key concepts:
 *  - Conet vs Conles: Conet binds commands to a link; Conles is connection-less.
 *  - Typical flows: service as CmdExecutor (client→server), service as CmdInitiator (server→client).
 *  - Default execution mode: CbExecCmd_F callback for immediate command processing.
 */
//======>END OF OVERVIEW OF THIS UNIT TESTING FILE=================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF UNIT TESTING DESIGN==============================================================
/**
 * Design focus:
 *  - Typical P2P command usage with accept+callback mode first.
 *  - Validate happy paths and typical request-response patterns.
 *  - Command execution via CbExecCmd_F callback mechanism (not polling).
 *  - Manual IOC_acceptClient (not auto-accept mode).
 *  - Coexistence with data/event capabilities is out-of-scope here.
 *  - Broadcast mode tested elsewhere; we keep Conet here.
 *
 * Future considerations:
 *  - UT_CommandTypicalWaitAck.cxx: IOC_waitCMD + IOC_ackCMD polling patterns
 *  - UT_CommandTypicalAutoAccept.cxx: IOC_SRVFLAG_AUTO_ACCEPT integration
 *  - UT_ServiceBroadcastCommand.cxx: SrvID→all clients command broadcast
 */
///////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF USER STORY=======================================================================
/**
 * US-1: As a service CmdExecutor, I want to receive and execute commands from client initiators
 *       so that clients can invoke server-side operations via command-response patterns.
 *
 * US-2: As a service CmdInitiator, I want to send commands to connected client executors
 *       so that server can orchestrate client-side operations and collect results.
 */
//======>END OF USER STORY==========================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//=======>BEGIN OF ACCEPTANCE CRITERIA==============================================================
/**
 * [@US-1] Service as CmdExecutor, Client as CmdInitiator (typical client→server commands)
 *  AC-1: GIVEN a Conet service (CmdExecutor) with CbExecCmd_F callback,
 *         WHEN client connects and sends command via IOC_execCMD,
 *         THEN service callback processes command and returns result synchronously.
 *  AC-2: GIVEN service supports multiple command types (PING, ECHO, CALC),
 *         WHEN client sends different command types with appropriate payloads,
 *         THEN service executes each command correctly and returns expected results.
 *  AC-3: GIVEN multiple clients connected to the same CmdExecutor service,
 *         WHEN clients send commands independently,
 *         THEN each command is processed correctly without interference.
 *  AC-4: GIVEN CmdExecutor service with command timeout configuration,
 *         WHEN client sends command that takes expected processing time,
 *         THEN command completes successfully within timeout constraints.
 *
 * [@US-2] Service as CmdInitiator, Client as CmdExecutor (reversed server→client commands)
 *  AC-1: GIVEN a Conet service (CmdInitiator) and client with CmdExecutor capability,
 *         WHEN service sends command to client via IOC_execCMD,
 *         THEN client callback processes command and service receives result.
 *  AC-2: GIVEN service needs to orchestrate multiple client operations,
 *         WHEN service sends different commands to different connected clients,
 *         THEN each client executes its assigned command independently.
 *  AC-3: GIVEN service requires command result aggregation,
 *         WHEN service sends same command to multiple clients,
 *         THEN service collects all results for processing.
 */
//=======>END OF ACCEPTANCE CRITERIA================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF TEST CASES=======================================================================
/**************************************************************************************************
 * @brief 【Command Test Cases】
 *
 * ORGANIZATION STRATEGIES:
 *  - By Feature/Component: Service as CmdExecutor vs CmdInitiator patterns
 *  - By Test Category: Typical → Boundary → State → Error → Performance
 *  - By Coverage Matrix: Systematic coverage of command execution flows
 *  - By Priority: Basic P2P commands first, complex orchestration second
 *
 * STATUS TRACKING: ⚪ = Planned/TODO，� = Implemented/RED, 🟢 = Passed/GREEN, ⚠️ = Issues
 *
 * ⚠️ FRAMEWORK STATUS: Command APIs (IOC_execCMD, IOC_waitCMD, IOC_ackCMD) are DECLARED but NOT IMPLEMENTED
 *    These tests provide complete design specification ready for implementation.
 *
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * 📋 [US-1]: Service as CmdExecutor (Client→Server Command Patterns)
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 *
 * [@AC-1,US-1] Basic P2P command execution with callback processing
 *  � TC-1: verifyServiceAsCmdExecutor_bySingleClient_expectSynchronousResponse
 *      @[Purpose]: Validate fundamental Conet CmdExecutor→callback execution from client initiator
 *      @[Brief]: Service accepts client, processes PING command via callback, returns PONG response
 *      @[Status]: IMPLEMENTED/RED 🔴 - Test code complete but blocked by missing IOC command implementation
 *
 * [@AC-2,US-1] Multiple command type support and payload handling
 *  � TC-1: verifyServiceAsCmdExecutor_byMultipleCommandTypes_expectProperExecution
 *      @[Purpose]: Ensure service can handle different command types with appropriate payloads
 *      @[Brief]: Tests PING (no payload), ECHO (text), CALC (numeric) commands sequentially
 *      @[Status]: IMPLEMENTED/RED 🔴 - Comprehensive command type coverage, awaiting framework
 *
 * [@AC-3,US-1] Multi-client isolation and concurrent command processing
 *  � TC-1: verifyServiceAsCmdExecutor_byMultipleClients_expectIsolatedExecution
 *      @[Purpose]: Validate command isolation between multiple clients without interference
 *      @[Brief]: 3 clients send unique ECHO commands concurrently, verify response isolation
 *      @[Status]: IMPLEMENTED/RED 🔴 - Thread-safe multi-client testing ready for framework
 *
 * [@AC-4,US-1] Command timeout and timing constraint validation
 *  ⚪ TC-1: verifyServiceAsCmdExecutor_byTimeoutConstraints_expectProperTiming
 *      @[Purpose]: Validate command timeout behavior for time-bounded operations
 *      @[Brief]: Test DELAY command with timeouts, verify completion and timeout scenarios
 *      @[Status]: PLANNED/TODO ⚪ - Need DELAY command implementation and timeout testing logic
 *
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * 📋 [US-2]: Service as CmdInitiator (Server→Client Command Patterns)
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 *
 * [@AC-1,US-2] Reversed command flow from service to client
 *  � TC-1: verifyServiceAsCmdInitiator_bySingleClient_expectClientExecution
 *      @[Purpose]: Validate reversed command flow from service to client executor
 *      @[Brief]: Service sends PING to client, client processes via callback, service gets PONG
 *      @[Status]: IMPLEMENTED/RED 🔴 - Complete bidirectional command flow, awaiting framework
 *
 * [@AC-2,US-2] Service orchestrating multiple client operations
 *  ⚪ TC-1: verifyServiceAsCmdInitiator_byMultipleClients_expectOrchestration
 *      @[Purpose]: Validate service orchestrating commands across multiple clients
 *      @[Brief]: Service sends different commands to different clients independently
 *      @[Status]: PLANNED/TODO ⚪ - Multi-client orchestration pattern needs implementation
 *
 * [@AC-3,US-2] Command result aggregation from multiple clients
 *  ⚪ TC-1: verifyServiceAsCmdInitiator_byResultAggregation_expectCompleteCollection
 *      @[Purpose]: Validate service collecting results from multiple clients for same command
 *      @[Brief]: Service sends GET_STATUS to all clients, aggregates responses
 *      @[Status]: PLANNED/TODO ⚪ - Result aggregation logic and GET_STATUS command type needed
 */
//======>END OF TEST CASES=========================================================================

// Command execution callback private data structure
typedef struct __CmdExecPriv {
    std::atomic<bool> CommandReceived{false};
    std::atomic<int> CommandCount{0};
    IOC_CmdID_T LastCmdID{0};
    IOC_CmdStatus_E LastStatus{IOC_CMD_STATUS_PENDING};
    IOC_Result_T LastResult{IOC_RESULT_BUG};
    char LastResponseData[512];
    ULONG_T LastResponseSize{0};
    std::mutex DataMutex;
    int ClientIndex{0};  // For multi-client scenarios
} __CmdExecPriv_T;

// Command execution callback function (service-side CmdExecutor)
static IOC_Result_T __CmdTypical_ExecutorCb(IOC_LinkID_T LinkID, IOC_CmdDesc_pT pCmdDesc, void *pCbPriv) {
    __CmdExecPriv_T *pPrivData = (__CmdExecPriv_T *)pCbPriv;
    if (!pPrivData || !pCmdDesc) return IOC_RESULT_INVALID_PARAM;

    std::lock_guard<std::mutex> lock(pPrivData->DataMutex);

    pPrivData->CommandReceived = true;
    pPrivData->CommandCount++;
    pPrivData->LastCmdID = IOC_CmdDesc_getCmdID(pCmdDesc);

    // Process different command types
    IOC_CmdID_T CmdID = IOC_CmdDesc_getCmdID(pCmdDesc);
    IOC_Result_T ExecResult = IOC_RESULT_SUCCESS;

    if (CmdID == IOC_CMDID_TEST_PING) {
        // PING command: simple response with "PONG"
        const char *response = "PONG";
        ExecResult = IOC_CmdDesc_setOutPayload(pCmdDesc, (void *)response, strlen(response));
        strcpy(pPrivData->LastResponseData, response);
        pPrivData->LastResponseSize = strlen(response);
    } else if (CmdID == IOC_CMDID_TEST_ECHO) {
        // ECHO command: return input data as output
        void *inputData = IOC_CmdDesc_getInData(pCmdDesc);
        ULONG_T inputSize = IOC_CmdDesc_getInDataSize(pCmdDesc);
        if (inputData && inputSize > 0) {
            ExecResult = IOC_CmdDesc_setOutPayload(pCmdDesc, inputData, inputSize);
            memcpy(pPrivData->LastResponseData, inputData,
                   std::min(inputSize, (ULONG_T)sizeof(pPrivData->LastResponseData) - 1));
            pPrivData->LastResponseSize = inputSize;
        }
    } else if (CmdID == IOC_CMDID_TEST_CALC) {
        // CALC command: simple arithmetic (add 1 to input number)
        void *inputData = IOC_CmdDesc_getInData(pCmdDesc);
        if (inputData) {
            int inputValue = *(int *)inputData;
            int resultValue = inputValue + 1;
            ExecResult = IOC_CmdDesc_setOutPayload(pCmdDesc, &resultValue, sizeof(resultValue));
            snprintf(pPrivData->LastResponseData, sizeof(pPrivData->LastResponseData), "%d", resultValue);
            pPrivData->LastResponseSize = sizeof(resultValue);
        }
    } else if (CmdID == IOC_CMDID_TEST_DELAY) {
        // DELAY command: simulate processing time
        void *inputData = IOC_CmdDesc_getInData(pCmdDesc);
        if (inputData) {
            int delayMs = *(int *)inputData;
            std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
            const char *response = "DELAY_COMPLETED";
            ExecResult = IOC_CmdDesc_setOutPayload(pCmdDesc, (void *)response, strlen(response));
            strcpy(pPrivData->LastResponseData, response);
            pPrivData->LastResponseSize = strlen(response);
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

// [@AC-1,US-1] TC-1: verifyCmdExecutorService_byClientInitiator_expectCallbackExecution
TEST(UT_ConetCommandTypical, verifyServiceAsCmdExecutor_bySingleClient_expectSynchronousResponse) {
    IOC_Result_T ResultValue = IOC_RESULT_BUG;

    // Service setup (Conet CmdExecutor with callback)
    __CmdExecPriv_T SrvExecPriv = {};
    IOC_SrvURI_T SrvURI = {.pProtocol = IOC_SRV_PROTO_FIFO,
                           .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
                           .pPath = (const char *)"CmdTypical_ExecutorSingle"};

    // Define supported commands
    static IOC_CmdID_T SupportedCmdIDs[] = {IOC_CMDID_TEST_PING, IOC_CMDID_TEST_ECHO, IOC_CMDID_TEST_CALC};
    IOC_CmdUsageArgs_T CmdUsageArgs = {.CbExecCmd_F = __CmdTypical_ExecutorCb,
                                       .pCbPrivData = &SrvExecPriv,
                                       .CmdNum = sizeof(SupportedCmdIDs) / sizeof(SupportedCmdIDs[0]),
                                       .pCmdIDs = SupportedCmdIDs};

    IOC_SrvArgs_T SrvArgs = {.SrvURI = SrvURI,
                             .Flags = IOC_SRVFLAG_NONE,
                             .UsageCapabilites = IOC_LinkUsageCmdExecutor,
                             .UsageArgs = {.pCmd = &CmdUsageArgs}};
    IOC_SrvID_T SrvID = IOC_ID_INVALID;
    ResultValue = IOC_onlineService(&SrvID, &SrvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);

    // Client setup (Conet CmdInitiator) — connect in a separate thread
    IOC_ConnArgs_T ConnArgs = {.SrvURI = SrvURI, .Usage = IOC_LinkUsageCmdInitiator};
    IOC_LinkID_T CliLinkID = IOC_ID_INVALID;
    std::thread CliThread([&] {
        IOC_Result_T ResultValueInThread = IOC_connectService(&CliLinkID, &ConnArgs, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValueInThread);
        ASSERT_NE(IOC_ID_INVALID, CliLinkID);
    });

    // Accept the client on the service side explicitly
    IOC_LinkID_T SrvLinkID = IOC_ID_INVALID;
    ResultValue = IOC_acceptClient(SrvID, &SrvLinkID, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);
    ASSERT_NE(IOC_ID_INVALID, SrvLinkID);

    // Wait for client thread completion
    if (CliThread.joinable()) CliThread.join();

    // Client sends PING command to service
    IOC_CmdDesc_T CmdDesc = {};
    CmdDesc.CmdID = IOC_CMDID_TEST_PING;
    CmdDesc.TimeoutMs = 5000;  // 5 second timeout
    CmdDesc.Status = IOC_CMD_STATUS_PENDING;

    ResultValue = IOC_execCMD(CliLinkID, &CmdDesc, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);

    // Verify command execution results
    ASSERT_TRUE(SrvExecPriv.CommandReceived.load());
    ASSERT_EQ(1, SrvExecPriv.CommandCount.load());
    ASSERT_EQ(IOC_CMDID_TEST_PING, SrvExecPriv.LastCmdID);
    ASSERT_EQ(IOC_CMD_STATUS_SUCCESS, SrvExecPriv.LastStatus);
    ASSERT_EQ(IOC_RESULT_SUCCESS, SrvExecPriv.LastResult);

    // Verify response payload
    void *responseData = IOC_CmdDesc_getOutData(&CmdDesc);
    ULONG_T responseSize = IOC_CmdDesc_getOutDataSize(&CmdDesc);
    ASSERT_TRUE(responseData != nullptr);
    ASSERT_EQ(4, responseSize);  // "PONG" length
    ASSERT_STREQ("PONG", (char *)responseData);

    // Cleanup
    if (CliLinkID != IOC_ID_INVALID) IOC_closeLink(CliLinkID);
    if (SrvLinkID != IOC_ID_INVALID) IOC_closeLink(SrvLinkID);
    if (SrvID != IOC_ID_INVALID) IOC_offlineService(SrvID);
}

// [@AC-2,US-1] TC-1: verifyCmdMultipleTypes_byTestCommands_expectCorrectResults
TEST(UT_ConetCommandTypical, verifyServiceAsCmdExecutor_byMultipleCommandTypes_expectProperExecution) {
    IOC_Result_T ResultValue = IOC_RESULT_BUG;

    // Service setup (Conet CmdExecutor supporting multiple command types)
    __CmdExecPriv_T SrvExecPriv = {};
    IOC_SrvURI_T SrvURI = {.pProtocol = IOC_SRV_PROTO_FIFO,
                           .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
                           .pPath = (const char *)"CmdTypical_MultipleTypes"};

    static IOC_CmdID_T SupportedCmdIDs[] = {IOC_CMDID_TEST_PING, IOC_CMDID_TEST_ECHO, IOC_CMDID_TEST_CALC};
    IOC_CmdUsageArgs_T CmdUsageArgs = {.CbExecCmd_F = __CmdTypical_ExecutorCb,
                                       .pCbPrivData = &SrvExecPriv,
                                       .CmdNum = sizeof(SupportedCmdIDs) / sizeof(SupportedCmdIDs[0]),
                                       .pCmdIDs = SupportedCmdIDs};

    IOC_SrvArgs_T SrvArgs = {.SrvURI = SrvURI,
                             .Flags = IOC_SRVFLAG_NONE,
                             .UsageCapabilites = IOC_LinkUsageCmdExecutor,
                             .UsageArgs = {.pCmd = &CmdUsageArgs}};
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

    IOC_LinkID_T SrvLinkID = IOC_ID_INVALID;
    ResultValue = IOC_acceptClient(SrvID, &SrvLinkID, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);
    ASSERT_NE(IOC_ID_INVALID, SrvLinkID);

    if (CliThread.joinable()) CliThread.join();

    // Test 1: PING command (no payload)
    {
        IOC_CmdDesc_T CmdDesc = {};
        CmdDesc.CmdID = IOC_CMDID_TEST_PING;
        CmdDesc.TimeoutMs = 3000;
        CmdDesc.Status = IOC_CMD_STATUS_PENDING;

        ResultValue = IOC_execCMD(CliLinkID, &CmdDesc, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);

        void *responseData = IOC_CmdDesc_getOutData(&CmdDesc);
        ASSERT_STREQ("PONG", (char *)responseData);
    }

    // Test 2: ECHO command (text payload)
    {
        IOC_CmdDesc_T CmdDesc = {};
        CmdDesc.CmdID = IOC_CMDID_TEST_ECHO;
        CmdDesc.TimeoutMs = 3000;
        CmdDesc.Status = IOC_CMD_STATUS_PENDING;

        const char *testText = "Hello Command System";
        ResultValue = IOC_CmdDesc_setInPayload(&CmdDesc, (void *)testText, strlen(testText));
        ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);

        ResultValue = IOC_execCMD(CliLinkID, &CmdDesc, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);

        void *responseData = IOC_CmdDesc_getOutData(&CmdDesc);
        ULONG_T responseSize = IOC_CmdDesc_getOutDataSize(&CmdDesc);
        ASSERT_EQ(strlen(testText), responseSize);
        ASSERT_EQ(0, memcmp(testText, responseData, responseSize));
    }

    // Test 3: CALC command (numeric payload)
    {
        IOC_CmdDesc_T CmdDesc = {};
        CmdDesc.CmdID = IOC_CMDID_TEST_CALC;
        CmdDesc.TimeoutMs = 3000;
        CmdDesc.Status = IOC_CMD_STATUS_PENDING;

        int inputValue = 42;
        ResultValue = IOC_CmdDesc_setInPayload(&CmdDesc, &inputValue, sizeof(inputValue));
        ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);

        ResultValue = IOC_execCMD(CliLinkID, &CmdDesc, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);

        void *responseData = IOC_CmdDesc_getOutData(&CmdDesc);
        ULONG_T responseSize = IOC_CmdDesc_getOutDataSize(&CmdDesc);
        ASSERT_EQ(sizeof(int), responseSize);
        ASSERT_EQ(43, *(int *)responseData);  // 42 + 1 = 43
    }

    // Verify total command count
    ASSERT_EQ(3, SrvExecPriv.CommandCount.load());

    // Cleanup
    if (CliLinkID != IOC_ID_INVALID) IOC_closeLink(CliLinkID);
    if (SrvLinkID != IOC_ID_INVALID) IOC_closeLink(SrvLinkID);
    if (SrvID != IOC_ID_INVALID) IOC_offlineService(SrvID);
}

// [@AC-3,US-1] TC-1: verifyCmdMultipleClients_byIndependentExecution_expectNoInterference
TEST(UT_ConetCommandTypical, verifyServiceAsCmdExecutor_byMultipleClients_expectIsolatedExecution) {
    IOC_Result_T ResultValue = IOC_RESULT_BUG;
    const int NumClients = 3;

    // Service setup (Conet CmdExecutor)
    __CmdExecPriv_T SrvExecPriv = {};
    IOC_SrvURI_T SrvURI = {.pProtocol = IOC_SRV_PROTO_FIFO,
                           .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
                           .pPath = (const char *)"CmdTypical_MultiClient"};

    static IOC_CmdID_T SupportedCmdIDs[] = {IOC_CMDID_TEST_ECHO};
    IOC_CmdUsageArgs_T CmdUsageArgs = {
        .CbExecCmd_F = __CmdTypical_ExecutorCb, .pCbPrivData = &SrvExecPriv, .CmdNum = 1, .pCmdIDs = SupportedCmdIDs};

    IOC_SrvArgs_T SrvArgs = {.SrvURI = SrvURI,
                             .Flags = IOC_SRVFLAG_NONE,
                             .UsageCapabilites = IOC_LinkUsageCmdExecutor,
                             .UsageArgs = {.pCmd = &CmdUsageArgs}};
    IOC_SrvID_T SrvID = IOC_ID_INVALID;
    ResultValue = IOC_onlineService(&SrvID, &SrvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);

    // Client contexts and threads
    std::thread CliThreads[NumClients];
    IOC_LinkID_T CliLinkIDs[NumClients] = {IOC_ID_INVALID, IOC_ID_INVALID, IOC_ID_INVALID};
    std::string ClientMessages[NumClients] = {"Client_0_Message", "Client_1_Message", "Client_2_Message"};
    std::string ReceivedResponses[NumClients];
    std::atomic<int> CompletedClients{0};

    // Start client threads
    for (int i = 0; i < NumClients; ++i) {
        CliThreads[i] = std::thread([&, i] {
            IOC_ConnArgs_T ConnArgs = {.SrvURI = SrvURI, .Usage = IOC_LinkUsageCmdInitiator};
            IOC_Result_T ResultValueInThread = IOC_connectService(&CliLinkIDs[i], &ConnArgs, NULL);
            ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValueInThread);
            ASSERT_NE(IOC_ID_INVALID, CliLinkIDs[i]);

            // Send ECHO command with unique message
            IOC_CmdDesc_T CmdDesc = {};
            CmdDesc.CmdID = IOC_CMDID_TEST_ECHO;
            CmdDesc.TimeoutMs = 5000;
            CmdDesc.Status = IOC_CMD_STATUS_PENDING;

            ResultValueInThread =
                IOC_CmdDesc_setInPayload(&CmdDesc, (void *)ClientMessages[i].c_str(), ClientMessages[i].length());
            ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValueInThread);

            ResultValueInThread = IOC_execCMD(CliLinkIDs[i], &CmdDesc, NULL);
            ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValueInThread);

            // Store response for verification
            void *responseData = IOC_CmdDesc_getOutData(&CmdDesc);
            ULONG_T responseSize = IOC_CmdDesc_getOutDataSize(&CmdDesc);
            ReceivedResponses[i] = std::string((char *)responseData, responseSize);

            CompletedClients++;
        });
    }

    // Accept clients on service side
    IOC_LinkID_T SrvLinkIDs[NumClients] = {IOC_ID_INVALID, IOC_ID_INVALID, IOC_ID_INVALID};
    for (int i = 0; i < NumClients; ++i) {
        ResultValue = IOC_acceptClient(SrvID, &SrvLinkIDs[i], NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);
        ASSERT_NE(IOC_ID_INVALID, SrvLinkIDs[i]);
    }

    // Wait for all clients to complete
    for (int i = 0; i < NumClients; ++i) {
        if (CliThreads[i].joinable()) CliThreads[i].join();
    }

    // Wait for command processing completion
    for (int retry = 0; retry < 100; ++retry) {
        if (CompletedClients.load() >= NumClients) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // Verify isolation: each client received exactly its own message back
    ASSERT_EQ(NumClients, CompletedClients.load());
    for (int i = 0; i < NumClients; ++i) {
        ASSERT_EQ(ClientMessages[i], ReceivedResponses[i])
            << "Client " << i << " response mismatch - isolation violation";
    }

    // Verify total command processing count
    ASSERT_EQ(NumClients, SrvExecPriv.CommandCount.load());

    // Cleanup
    for (int i = 0; i < NumClients; ++i) {
        if (CliLinkIDs[i] != IOC_ID_INVALID) IOC_closeLink(CliLinkIDs[i]);
        if (SrvLinkIDs[i] != IOC_ID_INVALID) IOC_closeLink(SrvLinkIDs[i]);
    }
    if (SrvID != IOC_ID_INVALID) IOC_offlineService(SrvID);
}

// [@AC-1,US-2] TC-1: verifyCmdInitiatorService_byClientExecutor_expectServerToClientCommand
TEST(UT_ConetCommandTypical, verifyServiceAsCmdInitiator_bySingleClient_expectClientExecution) {
    IOC_Result_T ResultValue = IOC_RESULT_BUG;

    // Service setup (Conet CmdInitiator)
    IOC_SrvURI_T SrvURI = {.pProtocol = IOC_SRV_PROTO_FIFO,
                           .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
                           .pPath = (const char *)"CmdTypical_InitiatorService"};

    IOC_SrvArgs_T SrvArgs = {
        .SrvURI = SrvURI, .Flags = IOC_SRVFLAG_NONE, .UsageCapabilites = IOC_LinkUsageCmdInitiator};
    IOC_SrvID_T SrvID = IOC_ID_INVALID;
    ResultValue = IOC_onlineService(&SrvID, &SrvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);

    // Client setup (Conet CmdExecutor with callback) — connect in a separate thread
    __CmdExecPriv_T CliExecPriv = {};
    static IOC_CmdID_T SupportedCmdIDs[] = {IOC_CMDID_TEST_PING, IOC_CMDID_TEST_ECHO};
    IOC_CmdUsageArgs_T CmdUsageArgs = {
        .CbExecCmd_F = __CmdTypical_ExecutorCb, .pCbPrivData = &CliExecPriv, .CmdNum = 2, .pCmdIDs = SupportedCmdIDs};

    IOC_ConnArgs_T ConnArgs = {
        .SrvURI = SrvURI, .Usage = IOC_LinkUsageCmdExecutor, .UsageArgs = {.pCmd = &CmdUsageArgs}};
    IOC_LinkID_T CliLinkID = IOC_ID_INVALID;
    std::thread CliThread([&] {
        IOC_Result_T ResultValueInThread = IOC_connectService(&CliLinkID, &ConnArgs, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValueInThread);
        ASSERT_NE(IOC_ID_INVALID, CliLinkID);
    });

    // Accept the client on the service side
    IOC_LinkID_T SrvLinkID = IOC_ID_INVALID;
    ResultValue = IOC_acceptClient(SrvID, &SrvLinkID, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);
    ASSERT_NE(IOC_ID_INVALID, SrvLinkID);

    // Wait for client thread completion
    if (CliThread.joinable()) CliThread.join();

    // Service sends command to client (reversed flow)
    IOC_CmdDesc_T CmdDesc = {};
    CmdDesc.CmdID = IOC_CMDID_TEST_PING;
    CmdDesc.TimeoutMs = 5000;
    CmdDesc.Status = IOC_CMD_STATUS_PENDING;

    ResultValue = IOC_execCMD(SrvLinkID, &CmdDesc, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);

    // Verify client executed the command
    ASSERT_TRUE(CliExecPriv.CommandReceived.load());
    ASSERT_EQ(1, CliExecPriv.CommandCount.load());
    ASSERT_EQ(IOC_CMDID_TEST_PING, CliExecPriv.LastCmdID);
    ASSERT_EQ(IOC_CMD_STATUS_SUCCESS, CliExecPriv.LastStatus);

    // Verify service received response
    void *responseData = IOC_CmdDesc_getOutData(&CmdDesc);
    ASSERT_TRUE(responseData != nullptr);
    ASSERT_STREQ("PONG", (char *)responseData);

    // Cleanup
    if (CliLinkID != IOC_ID_INVALID) IOC_closeLink(CliLinkID);
    if (SrvLinkID != IOC_ID_INVALID) IOC_closeLink(SrvLinkID);
    if (SrvID != IOC_ID_INVALID) IOC_offlineService(SrvID);
}

// 🔴 IMPLEMENTATION STATUS TRACKING - Organized by Priority and Category
// These will be implemented following the same pattern with increasing complexity
//
// 🔴 CURRENT RED TESTS (Implemented but blocked by framework):
//   4 tests complete, awaiting IOC_execCMD/IOC_waitCMD/IOC_ackCMD implementation
//
// ⚪ PLANNED IMPLEMENTATION ROADMAP:
//   1. AC-4,US-1 TC-1: Command timeout testing (need DELAY command support)
//   2. AC-2,US-2 TC-1: Multi-client orchestration patterns
//   3. AC-3,US-2 TC-1: Result aggregation mechanisms
//   4. Performance testing: Command throughput and latency
//   5. Boundary testing: Max payload sizes, concurrent limits
//   6. Error scenarios: Network failures, invalid commands
//
// 🟢 FUTURE GREEN STATE: Tests will turn green once framework command APIs are implemented

//======>END OF TEST CASES==========================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF FUTURE EXTENSION CONSIDERATIONS==================================================
/**
 * Future UT files to consider based on this foundation:
 *
 * 1. UT_CommandTypicalWaitAck.cxx:
 *    - Focus on IOC_waitCMD + IOC_ackCMD polling patterns
 *    - Explicit response handling vs automatic callback responses
 *    - Asynchronous command processing workflows
 *
 * 2. UT_CommandTypicalAutoAccept.cxx:
 *    - Integration with IOC_SRVFLAG_AUTO_ACCEPT
 *    - Automatic client acceptance for command services
 *    - OnAutoAccepted_F callback integration with command capabilities
 *
 * 3. UT_ServiceBroadcastCommand.cxx:
 *    - IOC_SRVFLAG_BROADCAST_COMMAND scenarios
 *    - Service→all connected clients command distribution
 *    - Command result aggregation from multiple clients
 *
 * 4. UT_CommandBoundary.cxx:
 *    - Boundary conditions: maximum payload sizes, timeout edge cases
 *    - Error scenarios: command not supported, executor busy, timeout
 *    - Resource limits: maximum concurrent commands, memory constraints
 *
 * 5. UT_CommandConcurrency.cxx:
 *    - Thread safety of command execution
 *    - Concurrent command processing
 *    - Race conditions and synchronization
 */
//======>END OF FUTURE EXTENSION CONSIDERATIONS====================================================
