///////////////////////////////////////////////////////////////////////////////////////////////////
// Command Typical WaitAck (connection-oriented / Conet) — UT skeleton
//
// Intent:
// - Focus on IOC_waitCMD + IOC_ackCMD polling patterns vs callback-based execution
// - Test manual command detection and explicit response acknowledgment workflows
// - Complement UT_CommandTypical.cxx which focuses on callback-based automatic processing
// - Validate polling-based command handling with manual acknowledgment control
//
// Key CMD Properties (SYNC+MAYBLOCK+NODROP):
// - SYNC: IOC_execCMD always waits synchronously for final result (immutable)
// - MAYBLOCK: Command operations may block until completion (configurable to NONBLOCK)
// - NODROP: Guaranteed response delivery - initiator always gets result or failure reason
//
// ⚪ IMPLEMENTATION STATUS:
//     🔴 RED: Skeleton created, implementation needed for polling-based command patterns
//     Focus on IOC_waitCMD, IOC_ackCMD workflows vs CbExecCmd_F callback patterns
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <chrono>
#include <thread>

#include "IOC/IOC_Option.h"
#include "_UT_IOC_Common.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF OVERVIEW OF THIS UNIT TESTING FILE===============================================
/**
 * @brief Verify connection-oriented command execution with explicit polling patterns (Conet):
 *  - Service and client exchange commands using IOC_waitCMD + IOC_ackCMD patterns
 *  - Focus on manual command detection vs automatic callback processing
 *  - Test polling-based command workflows with explicit acknowledgment control
 *  - Complement callback-based patterns tested in UT_CommandTypical.cxx
 *
 * Key differences from UT_CommandTypical.cxx:
 *  - Uses IOC_waitCMD for command detection instead of CbExecCmd_F callbacks
 *  - Uses IOC_ackCMD for explicit response sending vs automatic callback returns
 *  - Enables delayed response patterns within synchronous command execution
 *  - Tests polling-based command detection and manual response control
 *
 * CMD Properties (SYNC+MAYBLOCK+NODROP):
 *  - SYNC: Command execution remains synchronous - IOC_execCMD waits for final result
 *  - MAYBLOCK: Operations may block until completion (can configure NONBLOCK with timeout)
 *  - NODROP: Guaranteed delivery - initiator always receives result or failure reason
 */
//======>END OF OVERVIEW OF THIS UNIT TESTING FILE=================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF UNIT TESTING DESIGN==============================================================
/**
 * Design focus:
 *  - Polling-based command detection using IOC_waitCMD (not CbExecCmd_F callbacks)
 *  - Explicit response sending using IOC_ackCMD (not automatic callback returns)
 *  - Delayed response patterns within synchronous command execution framework
 *  - Manual control over command response timing and content
 *  - Validation of polling timeouts and command queue behaviors
 *
 * Key API patterns tested:
 *  - IOC_waitCMD: Polling for incoming commands with timeout control
 *  - IOC_ackCMD: Manual command acknowledgment with response data
 *  - Delayed workflows: Commands received, processed later, acknowledged separately
 *  - SYNC constraint: IOC_execCMD caller waits synchronously for final result
 *
 * CMD Architecture Compliance:
 *  - SYNC: All command execution remains synchronous from initiator perspective
 *  - MAYBLOCK: Test both blocking and NONBLOCK modes with timeout handling
 *  - NODROP: Validate guaranteed response delivery in all scenarios
 */
///////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF USER STORY=======================================================================
/**
 * US-1: As a service executor using polling patterns, I want to detect incoming commands
 *       via IOC_waitCMD and respond via IOC_ackCMD so that I can control response timing
 *       while maintaining synchronous command execution semantics.
 *
 * US-2: As a service initiator using polling patterns, I want to send commands to connected
 *       client executors that use IOC_waitCMD + IOC_ackCMD so that I can orchestrate
 *       client-side polling operations and collect results reliably.
 */
//======>END OF USER STORY==========================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//=======>BEGIN OF ACCEPTANCE CRITERIA==============================================================
/**
 * [@US-1] Service as Polling CmdExecutor using IOC_waitCMD + IOC_ackCMD
 *  AC-1: GIVEN a service using IOC_waitCMD for command detection,
 *         WHEN client sends command via IOC_execCMD,
 *         THEN service detects command via polling and can respond via IOC_ackCMD.
 *  AC-2: GIVEN service wants to process commands with delayed response,
 *         WHEN command is received via IOC_waitCMD,
 *         THEN service can process later and acknowledge when ready via IOC_ackCMD
 *         while client's IOC_execCMD waits synchronously for final result.
 *  AC-3: GIVEN service uses polling with timeout constraints,
 *         WHEN no commands arrive within timeout period,
 *         THEN IOC_waitCMD returns timeout result appropriately.
 *  AC-4: GIVEN service processes multiple commands with delayed responses,
 *         WHEN commands arrive from different clients,
 *         THEN service can track and acknowledge each command independently
 *         while each client waits synchronously for their respective result.
 *
 * [@US-2] Service as CmdInitiator, Client as CmdExecutor (reversed server→client polling commands)
 *  AC-1: GIVEN a Conet service (CmdInitiator) and client with polling CmdExecutor capability,
 *         WHEN service sends command to client via IOC_execCMD,
 *         THEN client detects command via IOC_waitCMD, processes, and acknowledges via IOC_ackCMD
 *         while service receives result through normal IOC_execCMD completion.
 *  AC-2: GIVEN service needs to orchestrate multiple client polling operations,
 *         WHEN service sends different commands to different connected clients using polling,
 *         THEN each client processes commands via polling patterns independently
 *         while service collects all results synchronously.
 */
//=======>END OF ACCEPTANCE CRITERIA================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF TEST CASES=======================================================================
/**************************************************************************************************
 * @brief 【Command WaitAck Test Cases】
 *
 * ORGANIZATION STRATEGIES:
 *  - By Pattern: Polling vs Callback command detection patterns
 *  - By Timing: Immediate vs Delayed acknowledgment patterns
 *  - By Control: Manual response timing vs automatic callback responses
 *  - By Integration: Service-side polling vs Client-side interaction with polling services
 *
 * STATUS TRACKING: ⚪ = Planned/TODO，🔴 = Implemented/RED, 🟢 = Passed/GREEN, ⚠️ = Issues
 *
 * ⚪ FRAMEWORK STATUS: IOC_waitCMD + IOC_ackCMD polling patterns need implementation
 *
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * 📋 [US-1]: Service Polling Command Detection (IOC_waitCMD + IOC_ackCMD)
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 *
 * [@AC-1,US-1] Basic polling command detection and acknowledgment
 *  🟢 TC-1: verifyServicePolling_bySingleClient_expectWaitAckPattern
 *      @[Purpose]: Validate basic IOC_waitCMD detection and IOC_ackCMD response pattern
 *      @[Brief]: Service polls for PING command, processes, and acknowledges with PONG
 *      @[Status]: ✅ PASSED - Basic polling pattern with IOC_waitCMD + IOC_ackCMD implemented and working
 *
 * [@AC-2,US-1] Delayed response processing with manual acknowledgment
 *  🟢 TC-1: verifyServiceAsyncProcessing_byDelayedAck_expectControlledTiming
 *      @[Purpose]: Validate delayed response processing with manual acknowledgment control
 *      @[Brief]: Service receives command, processes in background, acknowledges when complete
 *      @[Status]: ✅ PASSED - Delayed acknowledgment with controlled timing implemented and working
 *
 * [@AC-3,US-1] Polling timeout behavior and queue management
 *  🟢 TC-1: verifyServicePollingTimeout_byEmptyQueue_expectTimeoutHandling
 *      @[Purpose]: Validate IOC_waitCMD timeout behavior when no commands available
 *      @[Brief]: Service polls with timeout, handles empty queue gracefully
 *      @[Status]: ✅ PASSED - Polling timeout validation implemented and working
 *
 *  🟢 TC-2: verifyServicePollingNonblock_byEmptyQueue_expectImmediateReturn
 *      @[Purpose]: Validate IOC_waitCMD non-blocking behavior when no commands available
 *      @[Brief]: Service polls with NONBLOCK mode, returns immediately with NO_CMD_PENDING
 *      @[Status]: ✅ PASSED - Non-blocking polling validation implemented and working
 *
 * [@AC-4,US-1] Multi-client polling and independent acknowledgment tracking
 *  � TC-1: verifyServiceMultiClientPolling_byIndependentAck_expectProperTracking
 *      @[Purpose]: Validate independent command tracking and acknowledgment for multiple clients
 *      @[Brief]: Multiple clients send commands, service polls and tracks each independently
 *      @[Status]: ✅ PASSED - Multi-client polling with independent tracking implemented and working
 *
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * 📋 [US-2]: Service as CmdInitiator (Server→Client Polling Command Patterns)
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 *
 * [@AC-1,US-2] Service interaction with polling-based client executors
 *  🟢 TC-1: verifyServiceToPollingClient_byStandardFlow_expectProperResponse
 *      @[Purpose]: Validate service command sending to polling-based client
 *      @[Brief]: Service sends command, client polls and acknowledges, service receives response
 *      @[Status]: ✅ PASSED - Service-to-polling-client integration implemented and working
 *
 * [@AC-2,US-2] Service orchestration of multiple polling clients
 *  ⚪ TC-1: verifyServiceOrchestration_byPollingClients_expectReliableCollection
 *      @[Purpose]: Validate service orchestration of multiple clients using polling patterns
 *      @[Brief]: Service coordinates multiple client operations via polling-based command execution
 *      @[Status]: TODO - Implement multi-client polling orchestration patterns
 */
//======>END OF TEST CASES=========================================================================

// Command polling private data structure
typedef struct __CmdPollingPriv {
    std::atomic<bool> CommandDetected{false};
    std::atomic<int> CommandCount{0};
    IOC_CmdID_T LastCmdID{0};
    IOC_CmdDesc_T LastCmdDesc{};
    std::atomic<bool> ShouldStop{false};
    std::mutex PollingMutex;
    int ClientIndex{0};  // For multi-client scenarios
} __CmdPollingPriv_T;

// Extended structure for delayed processing
typedef struct __CmdDelayedProcessingPriv {
    std::atomic<bool> CommandDetected{false};
    std::atomic<int> CommandCount{0};
    IOC_CmdID_T LastCmdID{0};
    IOC_CmdDesc_T LastCmdDesc{};
    std::atomic<bool> ShouldStop{false};
    std::mutex ProcessingMutex;
    std::condition_variable ProcessingCV;
    std::atomic<bool> ProcessingComplete{false};
    std::chrono::steady_clock::time_point CommandReceiveTime;
    std::chrono::steady_clock::time_point CommandAckTime;
    std::atomic<int> DelayMs{500};           // Configurable delay
    IOC_LinkID_T SrvLinkID{IOC_ID_INVALID};  // For delayed ack
} __CmdDelayedProcessingPriv_T;

// Structure for timeout testing
typedef struct __CmdTimeoutTestPriv {
    std::atomic<bool> PollingStarted{false};
    std::atomic<bool> PollingComplete{false};
    std::atomic<IOC_Result_T> PollingResult{IOC_RESULT_BUG};
    std::chrono::steady_clock::time_point PollingStartTime;
    std::chrono::steady_clock::time_point PollingEndTime;
    std::atomic<int> TimeoutMs{1000};  // Configurable timeout
    std::atomic<int> PollingAttempts{0};
    IOC_LinkID_T SrvLinkID{IOC_ID_INVALID};
} __CmdTimeoutTestPriv_T;

// TODO: Implement basic polling pattern test
TEST(UT_ConetCommandWaitAck, verifyServicePolling_bySingleClient_expectWaitAckPattern) {
    IOC_Result_T ResultValue = IOC_RESULT_BUG;

    // Service setup (Conet CmdExecutor with polling - NO CALLBACK)
    __CmdPollingPriv_T SrvPollingPriv = {};
    IOC_SrvURI_T SrvURI = {.pProtocol = IOC_SRV_PROTO_FIFO,
                           .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
                           .pPath = (const char *)"CmdWaitAck_PollingBasic"};

    // Define supported commands for polling
    static IOC_CmdID_T SupportedCmdIDs[] = {IOC_CMDID_TEST_PING};
    IOC_CmdUsageArgs_T CmdUsageArgs = {.CbExecCmd_F = nullptr,  // NO CALLBACK - polling mode
                                       .pCbPrivData = nullptr,
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

    // Start service polling thread FIRST - uses IOC_waitCMD + IOC_ackCMD
    std::atomic<bool> PollingStarted{false};
    std::thread SrvPollingThread([&] {
        PollingStarted = true;             // Signal that polling has started
        IOC_CMDDESC_DECLARE_VAR(CmdDesc);  // Better initialization
        IOC_Result_T PollingResult = IOC_waitCMD(SrvLinkID, &CmdDesc, NULL);

        if (PollingResult == IOC_RESULT_SUCCESS) {
            // Command detected via polling
            SrvPollingPriv.CommandDetected = true;
            SrvPollingPriv.CommandCount++;
            SrvPollingPriv.LastCmdID = IOC_CmdDesc_getCmdID(&CmdDesc);

            // Process PING command - simple response with "PONG"
            if (SrvPollingPriv.LastCmdID == IOC_CMDID_TEST_PING) {
                const char *response = "PONG";
                IOC_Result_T AckResult = IOC_CmdDesc_setOutPayload(&CmdDesc, (void *)response, strlen(response));
                EXPECT_EQ(IOC_RESULT_SUCCESS, AckResult);

                // Set command status to success
                IOC_CmdDesc_setStatus(&CmdDesc, IOC_CMD_STATUS_SUCCESS);
                IOC_CmdDesc_setResult(&CmdDesc, IOC_RESULT_SUCCESS);

                // Explicit acknowledgment via IOC_ackCMD
                IOC_Result_T SendResult = IOC_ackCMD(SrvLinkID, &CmdDesc, NULL);
                EXPECT_EQ(IOC_RESULT_SUCCESS, SendResult);
            }
        }
    });

    // Wait for polling to start before sending command
    while (!PollingStarted.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));  // Extra time for polling setup

    // Client command sending - using new initialization macro
    IOC_CMDDESC_DECLARE_VAR(CmdDesc);
    CmdDesc.CmdID = IOC_CMDID_TEST_PING;
    // Status already initialized to IOC_CMD_STATUS_PENDING by macro

    printf("[DEBUG] About to call IOC_execCMD with CliLinkID=%llu, CmdID=%llu\n", CliLinkID, CmdDesc.CmdID);
    ResultValue = IOC_execCMD(CliLinkID, &CmdDesc, NULL);
    printf("[DEBUG] IOC_execCMD returned: %d\n", ResultValue);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);

    // Verify command status
    IOC_CmdStatus_E Status = IOC_CmdDesc_getStatus(&CmdDesc);
    ASSERT_EQ(IOC_CMD_STATUS_SUCCESS, Status);

    IOC_Result_T CmdResult = IOC_CmdDesc_getResult(&CmdDesc);
    ASSERT_EQ(IOC_RESULT_SUCCESS, CmdResult);

    // Verify response payload
    void *responseData = IOC_CmdDesc_getOutData(&CmdDesc);
    ULONG_T responseSize = IOC_CmdDesc_getOutDataSize(&CmdDesc);
    ASSERT_TRUE(responseData != nullptr);
    ASSERT_EQ(4, responseSize);  // "PONG" length
    ASSERT_STREQ("PONG", (char *)responseData);

    // Wait for polling thread to complete
    if (SrvPollingThread.joinable()) SrvPollingThread.join();

    // Verify polling detection worked
    ASSERT_TRUE(SrvPollingPriv.CommandDetected);
    ASSERT_EQ(1, SrvPollingPriv.CommandCount.load());
    ASSERT_EQ(IOC_CMDID_TEST_PING, SrvPollingPriv.LastCmdID);

    // Cleanup
    if (CliLinkID != IOC_ID_INVALID) IOC_closeLink(CliLinkID);
    if (SrvLinkID != IOC_ID_INVALID) IOC_closeLink(SrvLinkID);
    if (SrvID != IOC_ID_INVALID) IOC_offlineService(SrvID);
}

// Implement delayed response processing test
TEST(UT_ConetCommandWaitAck, verifyServiceAsyncProcessing_byDelayedAck_expectControlledTiming) {
    IOC_Result_T ResultValue = IOC_RESULT_BUG;

    // Service setup with delayed processing capabilities
    __CmdDelayedProcessingPriv_T SrvDelayedPriv = {};
    IOC_SrvURI_T SrvURI = {.pProtocol = IOC_SRV_PROTO_FIFO,
                           .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
                           .pPath = (const char *)"CmdWaitAck_DelayedProcessing"};

    // Define supported commands for delayed processing
    static IOC_CmdID_T SupportedCmdIDs[] = {IOC_CMDID_TEST_PING};
    IOC_CmdUsageArgs_T CmdUsageArgs = {.CbExecCmd_F = nullptr,  // NO CALLBACK - polling mode
                                       .pCbPrivData = nullptr,
                                       .CmdNum = 1,  // Only PING command
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

    SrvDelayedPriv.SrvLinkID = SrvLinkID;  // Store for delayed ack

    // Start service polling thread with delayed processing
    std::atomic<bool> PollingStarted{false};
    std::thread SrvPollingThread([&] {
        PollingStarted = true;
        IOC_CMDDESC_DECLARE_VAR(CmdDesc);
        IOC_Result_T PollingResult = IOC_waitCMD(SrvLinkID, &CmdDesc, NULL);

        if (PollingResult == IOC_RESULT_SUCCESS) {
            // Command detected via polling - record receive time
            SrvDelayedPriv.CommandReceiveTime = std::chrono::steady_clock::now();
            SrvDelayedPriv.CommandDetected = true;
            SrvDelayedPriv.CommandCount++;
            SrvDelayedPriv.LastCmdID = IOC_CmdDesc_getCmdID(&CmdDesc);

            // Copy command for delayed processing
            SrvDelayedPriv.LastCmdDesc = CmdDesc;  // Save command descriptor

            // Signal that command is received and processing can start
            {
                std::lock_guard<std::mutex> lock(SrvDelayedPriv.ProcessingMutex);
                SrvDelayedPriv.ProcessingCV.notify_one();
            }
        }
    });

    // Start delayed processing thread
    std::thread DelayedProcessingThread([&] {
        std::unique_lock<std::mutex> lock(SrvDelayedPriv.ProcessingMutex);

        // Wait for command to arrive
        SrvDelayedPriv.ProcessingCV.wait(lock, [&] { return SrvDelayedPriv.CommandDetected.load(); });

        // Simulate processing delay
        std::this_thread::sleep_for(std::chrono::milliseconds(SrvDelayedPriv.DelayMs.load()));

        // Process the command with delay
        if (SrvDelayedPriv.LastCmdID == IOC_CMDID_TEST_PING) {
            const char *response = "DELAYED_PONG";
            IOC_Result_T AckResult =
                IOC_CmdDesc_setOutPayload(&SrvDelayedPriv.LastCmdDesc, (void *)response, strlen(response));
            EXPECT_EQ(IOC_RESULT_SUCCESS, AckResult);
        }

        // Set command status to success
        IOC_CmdDesc_setStatus(&SrvDelayedPriv.LastCmdDesc, IOC_CMD_STATUS_SUCCESS);
        IOC_CmdDesc_setResult(&SrvDelayedPriv.LastCmdDesc, IOC_RESULT_SUCCESS);

        // Record acknowledgment time
        SrvDelayedPriv.CommandAckTime = std::chrono::steady_clock::now();

        // Send delayed acknowledgment
        IOC_Result_T SendResult = IOC_ackCMD(SrvLinkID, &SrvDelayedPriv.LastCmdDesc, NULL);
        EXPECT_EQ(IOC_RESULT_SUCCESS, SendResult);

        SrvDelayedPriv.ProcessingComplete = true;
    });

    // Wait for polling to start
    while (!PollingStarted.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Record client command start time
    auto CommandStartTime = std::chrono::steady_clock::now();

    // Client sends command - this should block until delayed ack arrives
    IOC_CMDDESC_DECLARE_VAR(CmdDesc);
    CmdDesc.CmdID = IOC_CMDID_TEST_PING;

    printf("[DEBUG] Client sending command with delayed processing...\n");
    ResultValue = IOC_execCMD(CliLinkID, &CmdDesc, NULL);

    auto CommandEndTime = std::chrono::steady_clock::now();
    printf("[DEBUG] Client command completed after delay\n");

    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);

    // Verify command status
    IOC_CmdStatus_E Status = IOC_CmdDesc_getStatus(&CmdDesc);
    ASSERT_EQ(IOC_CMD_STATUS_SUCCESS, Status);

    IOC_Result_T CmdResult = IOC_CmdDesc_getResult(&CmdDesc);
    ASSERT_EQ(IOC_RESULT_SUCCESS, CmdResult);

    // Verify delayed response payload
    void *responseData = IOC_CmdDesc_getOutData(&CmdDesc);
    ULONG_T responseSize = IOC_CmdDesc_getOutDataSize(&CmdDesc);
    ASSERT_TRUE(responseData != nullptr);
    ASSERT_EQ(12, responseSize);  // "DELAYED_PONG" length
    ASSERT_STREQ("DELAYED_PONG", (char *)responseData);

    // Wait for threads to complete
    if (SrvPollingThread.joinable()) SrvPollingThread.join();
    if (DelayedProcessingThread.joinable()) DelayedProcessingThread.join();

    // Verify timing constraints - delayed processing should take at least the configured delay
    auto TotalDuration = std::chrono::duration_cast<std::chrono::milliseconds>(CommandEndTime - CommandStartTime);
    auto ProcessingDuration = std::chrono::duration_cast<std::chrono::milliseconds>(SrvDelayedPriv.CommandAckTime -
                                                                                    SrvDelayedPriv.CommandReceiveTime);

    printf("[DEBUG] Total command duration: %lld ms\n", TotalDuration.count());
    printf("[DEBUG] Processing duration: %lld ms\n", ProcessingDuration.count());

    // Verify the delay was respected (allow some timing variance)
    ASSERT_GE(TotalDuration.count(), SrvDelayedPriv.DelayMs.load() - 50);  // At least delay minus variance
    ASSERT_GE(ProcessingDuration.count(), SrvDelayedPriv.DelayMs.load() - 10);

    // Verify delayed processing detection worked
    ASSERT_TRUE(SrvDelayedPriv.CommandDetected);
    ASSERT_TRUE(SrvDelayedPriv.ProcessingComplete);
    ASSERT_EQ(1, SrvDelayedPriv.CommandCount.load());
    ASSERT_EQ(IOC_CMDID_TEST_PING, SrvDelayedPriv.LastCmdID);

    // Cleanup
    if (CliLinkID != IOC_ID_INVALID) IOC_closeLink(CliLinkID);
    if (SrvLinkID != IOC_ID_INVALID) IOC_closeLink(SrvLinkID);
    if (SrvID != IOC_ID_INVALID) IOC_offlineService(SrvID);
}

// Implement polling timeout test
TEST(UT_ConetCommandWaitAck, verifyServicePollingTimeout_byEmptyQueue_expectTimeoutHandling) {
    IOC_Result_T ResultValue = IOC_RESULT_BUG;

    // Service setup with timeout testing capabilities
    __CmdTimeoutTestPriv_T SrvTimeoutPriv = {};
    IOC_SrvURI_T SrvURI = {.pProtocol = IOC_SRV_PROTO_FIFO,
                           .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
                           .pPath = (const char *)"CmdWaitAck_TimeoutTest"};

    // Define supported commands for timeout testing
    static IOC_CmdID_T SupportedCmdIDs[] = {IOC_CMDID_TEST_PING};
    IOC_CmdUsageArgs_T CmdUsageArgs = {.CbExecCmd_F = nullptr,  // NO CALLBACK - polling mode
                                       .pCbPrivData = nullptr,
                                       .CmdNum = 1,  // Only PING command
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

    SrvTimeoutPriv.SrvLinkID = SrvLinkID;

    // Start service polling thread with timeout testing
    std::thread SrvTimeoutPollingThread([&] {
        SrvTimeoutPriv.PollingStarted = true;

        // Create timeout option with specific timeout value (convert ms to us)
        IOC_Options_T TimeoutOption;
        TimeoutOption.IDs = IOC_OPTID_TIMEOUT;
        TimeoutOption.Payload.TimeoutUS = SrvTimeoutPriv.TimeoutMs.load() * 1000;  // Convert ms to us

        IOC_CMDDESC_DECLARE_VAR(CmdDesc);

        // Record polling start time
        SrvTimeoutPriv.PollingStartTime = std::chrono::steady_clock::now();

        printf("[DEBUG] Starting polling with timeout %d ms\n", SrvTimeoutPriv.TimeoutMs.load());

        // Attempt polling with timeout - should timeout because no commands will be sent
        IOC_Result_T PollingResult = IOC_waitCMD(SrvLinkID, &CmdDesc, &TimeoutOption);

        // Record polling end time
        SrvTimeoutPriv.PollingEndTime = std::chrono::steady_clock::now();

        // Store the result for verification
        SrvTimeoutPriv.PollingResult = PollingResult;
        SrvTimeoutPriv.PollingAttempts++;
        SrvTimeoutPriv.PollingComplete = true;

        printf("[DEBUG] Polling completed with result: %d\n", PollingResult);
    });

    // Wait for polling to start
    while (!SrvTimeoutPriv.PollingStarted.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    printf("[DEBUG] Polling started, waiting for timeout...\n");

    // Wait for polling to complete (should timeout)
    while (!SrvTimeoutPriv.PollingComplete.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Join the polling thread
    if (SrvTimeoutPollingThread.joinable()) SrvTimeoutPollingThread.join();

    // Verify timeout behavior
    auto ActualDuration = std::chrono::duration_cast<std::chrono::milliseconds>(SrvTimeoutPriv.PollingEndTime -
                                                                                SrvTimeoutPriv.PollingStartTime);

    printf("[DEBUG] Actual polling duration: %lld ms (expected ~%d ms)\n", ActualDuration.count(),
           SrvTimeoutPriv.TimeoutMs.load());

    // Verify that polling returned with timeout result
    IOC_Result_T ExpectedTimeoutResult = IOC_RESULT_TIMEOUT;  // or whatever timeout result is expected
    ASSERT_EQ(ExpectedTimeoutResult, SrvTimeoutPriv.PollingResult.load())
        << "Expected timeout result, got: " << SrvTimeoutPriv.PollingResult.load();

    // Verify that the timeout was respected (allow some variance for timing)
    int ExpectedTimeoutMs = SrvTimeoutPriv.TimeoutMs.load();
    ASSERT_GE(ActualDuration.count(), ExpectedTimeoutMs - 100)  // At least timeout minus variance
        << "Polling completed too early: " << ActualDuration.count() << "ms < " << (ExpectedTimeoutMs - 100) << "ms";

    ASSERT_LE(ActualDuration.count(), ExpectedTimeoutMs + 200)  // At most timeout plus variance
        << "Polling took too long: " << ActualDuration.count() << "ms > " << (ExpectedTimeoutMs + 200) << "ms";

    // Verify polling attempts and completion state
    ASSERT_EQ(1, SrvTimeoutPriv.PollingAttempts.load());
    ASSERT_TRUE(SrvTimeoutPriv.PollingComplete.load());

    // Test multiple timeout cycles to ensure consistency
    printf("[DEBUG] Testing second timeout cycle...\n");

    // Reset for second test
    SrvTimeoutPriv.PollingStarted = false;
    SrvTimeoutPriv.PollingComplete = false;
    SrvTimeoutPriv.PollingResult = IOC_RESULT_BUG;
    SrvTimeoutPriv.TimeoutMs = 500;  // Shorter timeout for second test

    std::thread SrvSecondTimeoutThread([&] {
        SrvTimeoutPriv.PollingStarted = true;

        IOC_Options_T TimeoutOption;
        TimeoutOption.IDs = IOC_OPTID_TIMEOUT;
        TimeoutOption.Payload.TimeoutUS = SrvTimeoutPriv.TimeoutMs.load() * 1000;  // Convert ms to us

        IOC_CMDDESC_DECLARE_VAR(CmdDesc);

        SrvTimeoutPriv.PollingStartTime = std::chrono::steady_clock::now();
        IOC_Result_T PollingResult = IOC_waitCMD(SrvLinkID, &CmdDesc, &TimeoutOption);
        SrvTimeoutPriv.PollingEndTime = std::chrono::steady_clock::now();

        SrvTimeoutPriv.PollingResult = PollingResult;
        SrvTimeoutPriv.PollingAttempts++;
        SrvTimeoutPriv.PollingComplete = true;
    });

    // Wait for second polling cycle
    while (!SrvTimeoutPriv.PollingStarted.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    while (!SrvTimeoutPriv.PollingComplete.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    if (SrvSecondTimeoutThread.joinable()) SrvSecondTimeoutThread.join();

    // Verify second timeout
    auto SecondDuration = std::chrono::duration_cast<std::chrono::milliseconds>(SrvTimeoutPriv.PollingEndTime -
                                                                                SrvTimeoutPriv.PollingStartTime);

    printf("[DEBUG] Second polling duration: %lld ms (expected ~%d ms)\n", SecondDuration.count(),
           SrvTimeoutPriv.TimeoutMs.load());

    ASSERT_EQ(ExpectedTimeoutResult, SrvTimeoutPriv.PollingResult.load());
    ASSERT_EQ(2, SrvTimeoutPriv.PollingAttempts.load());

    // Verify second timeout timing
    int SecondExpectedTimeoutMs = SrvTimeoutPriv.TimeoutMs.load();
    ASSERT_GE(SecondDuration.count(), SecondExpectedTimeoutMs - 100);
    ASSERT_LE(SecondDuration.count(), SecondExpectedTimeoutMs + 200);

    printf("[DEBUG] Timeout test completed successfully\n");

    // Cleanup
    if (CliLinkID != IOC_ID_INVALID) IOC_closeLink(CliLinkID);
    if (SrvLinkID != IOC_ID_INVALID) IOC_closeLink(SrvLinkID);
    if (SrvID != IOC_ID_INVALID) IOC_offlineService(SrvID);
}

// Implement non-blocking polling test
TEST(UT_ConetCommandWaitAck, verifyServicePollingNonblock_byEmptyQueue_expectImmediateReturn) {
    IOC_Result_T ResultValue = IOC_RESULT_BUG;

    // Service setup with non-blocking polling capabilities
    __CmdTimeoutTestPriv_T SrvNonblockPriv = {};
    IOC_SrvURI_T SrvURI = {.pProtocol = IOC_SRV_PROTO_FIFO,
                           .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
                           .pPath = (const char *)"CmdWaitAck_NonblockTest"};

    // Define supported commands for non-blocking testing
    static IOC_CmdID_T SupportedCmdIDs[] = {IOC_CMDID_TEST_PING};
    IOC_CmdUsageArgs_T CmdUsageArgs = {.CbExecCmd_F = nullptr,  // NO CALLBACK - polling mode
                                       .pCbPrivData = nullptr,
                                       .CmdNum = 1,  // Only PING command
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

    SrvNonblockPriv.SrvLinkID = SrvLinkID;

    // Start service non-blocking polling thread
    std::thread SrvNonblockPollingThread([&] {
        SrvNonblockPriv.PollingStarted = true;

        // Create non-blocking option using macro
        IOC_Option_defineNonBlock(NonBlockOption);

        IOC_CMDDESC_DECLARE_VAR(CmdDesc);

        // Record polling start time
        SrvNonblockPriv.PollingStartTime = std::chrono::steady_clock::now();

        printf("[DEBUG] Starting non-blocking polling (should return immediately)\n");

        // Attempt non-blocking polling - should return immediately with NO_DATA
        IOC_Result_T PollingResult = IOC_waitCMD(SrvLinkID, &CmdDesc, &NonBlockOption);

        // Record polling end time
        SrvNonblockPriv.PollingEndTime = std::chrono::steady_clock::now();

        // Store the result for verification
        SrvNonblockPriv.PollingResult = PollingResult;
        SrvNonblockPriv.PollingAttempts++;
        SrvNonblockPriv.PollingComplete = true;

        printf("[DEBUG] Non-blocking polling completed with result: %d\n", PollingResult);
    });

    // Wait for polling to start
    while (!SrvNonblockPriv.PollingStarted.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    printf("[DEBUG] Non-blocking polling started, should complete immediately...\n");

    // Wait for polling to complete (should be very quick)
    while (!SrvNonblockPriv.PollingComplete.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // Join the polling thread
    if (SrvNonblockPollingThread.joinable()) SrvNonblockPollingThread.join();

    // Verify non-blocking behavior
    auto ActualDuration = std::chrono::duration_cast<std::chrono::milliseconds>(SrvNonblockPriv.PollingEndTime -
                                                                                SrvNonblockPriv.PollingStartTime);

    printf("[DEBUG] Actual non-blocking polling duration: %lld ms (expected ~0 ms)\n", ActualDuration.count());

    // Verify that polling returned with NO_CMD_PENDING result (since no commands were sent)
    // Note: The IOC framework currently uses IOC_RESULT_NOT_EXIST for command polling non-blocking mode
    IOC_Result_T ExpectedNonblockResult = IOC_RESULT_NO_CMD_PENDING;
    ASSERT_EQ(ExpectedNonblockResult, SrvNonblockPriv.PollingResult.load())
        << "Expected NO_CMD_PENDING result for non-blocking call, got: " << SrvNonblockPriv.PollingResult.load();

    // Verify that the polling returned immediately (should be < 50ms)
    ASSERT_LE(ActualDuration.count(), 50)
        << "Non-blocking polling took too long: " << ActualDuration.count() << "ms > 50ms";

    // Verify polling attempts and completion state
    ASSERT_EQ(1, SrvNonblockPriv.PollingAttempts.load());
    ASSERT_TRUE(SrvNonblockPriv.PollingComplete.load());

    // Test multiple non-blocking cycles to ensure consistency
    printf("[DEBUG] Testing second non-blocking cycle...\n");

    // Reset for second test
    SrvNonblockPriv.PollingStarted = false;
    SrvNonblockPriv.PollingComplete = false;
    SrvNonblockPriv.PollingResult = IOC_RESULT_BUG;

    std::thread SrvSecondNonblockThread([&] {
        SrvNonblockPriv.PollingStarted = true;

        IOC_Option_defineNonBlock(NonBlockOption);
        IOC_CMDDESC_DECLARE_VAR(CmdDesc);

        SrvNonblockPriv.PollingStartTime = std::chrono::steady_clock::now();
        IOC_Result_T PollingResult = IOC_waitCMD(SrvLinkID, &CmdDesc, &NonBlockOption);
        SrvNonblockPriv.PollingEndTime = std::chrono::steady_clock::now();

        SrvNonblockPriv.PollingResult = PollingResult;
        SrvNonblockPriv.PollingAttempts++;
        SrvNonblockPriv.PollingComplete = true;
    });

    // Wait for second polling cycle
    while (!SrvNonblockPriv.PollingStarted.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    while (!SrvNonblockPriv.PollingComplete.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    if (SrvSecondNonblockThread.joinable()) SrvSecondNonblockThread.join();

    // Verify second non-blocking call
    auto SecondDuration = std::chrono::duration_cast<std::chrono::milliseconds>(SrvNonblockPriv.PollingEndTime -
                                                                                SrvNonblockPriv.PollingStartTime);

    printf("[DEBUG] Second non-blocking polling duration: %lld ms (expected ~0 ms)\n", SecondDuration.count());

    ASSERT_EQ(ExpectedNonblockResult, SrvNonblockPriv.PollingResult.load());
    ASSERT_EQ(2, SrvNonblockPriv.PollingAttempts.load());

    // Verify second non-blocking timing
    ASSERT_LE(SecondDuration.count(), 50);

    printf("[DEBUG] Non-blocking test completed successfully\n");

    // Cleanup
    if (CliLinkID != IOC_ID_INVALID) IOC_closeLink(CliLinkID);
    if (SrvLinkID != IOC_ID_INVALID) IOC_closeLink(SrvLinkID);
    if (SrvID != IOC_ID_INVALID) IOC_offlineService(SrvID);
}

// Multi-client polling data structure
typedef struct __CmdMultiClientPollingPriv {
    std::atomic<bool> PollingStarted{false};
    std::atomic<bool> PollingComplete{false};
    std::atomic<bool> ShouldStop{false};
    std::atomic<int> TotalCommandsProcessed{0};
    std::atomic<int> ExpectedCommands{0};

    // Per-client tracking
    struct ClientInfo {
        IOC_LinkID_T LinkID{IOC_ID_INVALID};     // Client-side link ID
        IOC_LinkID_T SrvLinkID{IOC_ID_INVALID};  // Server-side P2P link ID
        std::atomic<bool> CommandSent{false};
        std::atomic<bool> CommandReceived{false};
        std::atomic<bool> CommandAcknowledged{false};
        IOC_CmdID_T SentCmdID{0};
        IOC_CmdID_T ReceivedCmdID{0};
        std::string ExpectedResponse;
        std::string ActualResponse;
        std::chrono::steady_clock::time_point SendTime;
        std::chrono::steady_clock::time_point ReceiveTime;
        std::chrono::steady_clock::time_point AckTime;
        std::atomic<IOC_Result_T> CommandResult{IOC_RESULT_BUG};
        int ClientIndex{0};
    };

    static const int MAX_CLIENTS = 3;
    ClientInfo Clients[MAX_CLIENTS];

    std::mutex PollingMutex;
    std::condition_variable PollingCV;
} __CmdMultiClientPollingPriv_T;

// Implement multi-client polling test with SYNC guarantees
TEST(UT_ConetCommandWaitAck, verifyServiceMultiClientPolling_byIndependentAck_expectProperTracking) {
    IOC_Result_T ResultValue = IOC_RESULT_BUG;

    // Service setup with multi-client polling capabilities
    __CmdMultiClientPollingPriv_T MultiPriv = {};
    MultiPriv.ExpectedCommands = 3;  // Test with 3 clients

    IOC_SrvURI_T SrvURI = {.pProtocol = IOC_SRV_PROTO_FIFO,
                           .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
                           .pPath = (const char *)"CmdWaitAck_MultiClientTest"};

    // Define supported commands for multi-client testing
    static IOC_CmdID_T SupportedCmdIDs[] = {IOC_CMDID_TEST_PING, IOC_CMDID_TEST_ECHO};
    IOC_CmdUsageArgs_T CmdUsageArgs = {.CbExecCmd_F = nullptr,  // NO CALLBACK - polling mode
                                       .pCbPrivData = nullptr,
                                       .CmdNum = 2,  // PING and ECHO commands
                                       .pCmdIDs = SupportedCmdIDs};

    IOC_SrvArgs_T SrvArgs = {.SrvURI = SrvURI,
                             .Flags = IOC_SRVFLAG_NONE,
                             .UsageCapabilites = IOC_LinkUsageCmdExecutor,
                             .UsageArgs = {.pCmd = &CmdUsageArgs}};
    IOC_SrvID_T SrvID = IOC_ID_INVALID;
    ResultValue = IOC_onlineService(&SrvID, &SrvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);

    // Setup multiple clients
    std::vector<std::thread> ClientThreads;

    for (int i = 0; i < MultiPriv.ExpectedCommands.load(); i++) {
        MultiPriv.Clients[i].ClientIndex = i;
        MultiPriv.Clients[i].SentCmdID = (i == 0) ? IOC_CMDID_TEST_PING : IOC_CMDID_TEST_ECHO;
        // Generate expected response based on command type and client index
        if (MultiPriv.Clients[i].SentCmdID == IOC_CMDID_TEST_PING) {
            MultiPriv.Clients[i].ExpectedResponse = "PONG_" + std::to_string(i);
        } else {
            MultiPriv.Clients[i].ExpectedResponse = "ECHO_" + std::to_string(i);
        }

        ClientThreads.emplace_back([&, i] {
            IOC_ConnArgs_T ConnArgs = {.SrvURI = SrvURI, .Usage = IOC_LinkUsageCmdInitiator};
            IOC_LinkID_T CliLinkID = IOC_ID_INVALID;
            IOC_Result_T ResultValueInThread = IOC_connectService(&CliLinkID, &ConnArgs, NULL);
            ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValueInThread);
            ASSERT_NE(IOC_ID_INVALID, CliLinkID);

            MultiPriv.Clients[i].LinkID = CliLinkID;
        });
    }

    // Accept all client connections - each creates a dedicated P2P SrvLinkID
    std::vector<IOC_LinkID_T> SrvLinkIDs;
    for (int i = 0; i < MultiPriv.ExpectedCommands.load(); i++) {
        IOC_LinkID_T SrvLinkID = IOC_ID_INVALID;
        ResultValue = IOC_acceptClient(SrvID, &SrvLinkID, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);
        ASSERT_NE(IOC_ID_INVALID, SrvLinkID);

        // Store each service link ID for individual polling
        SrvLinkIDs.push_back(SrvLinkID);
        MultiPriv.Clients[i].SrvLinkID = SrvLinkID;  // Add SrvLinkID to ClientInfo
    }

    // Wait for all client threads to complete connection
    for (auto &thread : ClientThreads) {
        if (thread.joinable()) thread.join();
    }
    ClientThreads.clear();

    // Start service polling thread that handles multiple P2P connections
    std::thread SrvPollingThread([&] {
        MultiPriv.PollingStarted = true;

        printf("[DEBUG] Multi-client polling started, waiting for %d commands\n", MultiPriv.ExpectedCommands.load());

        while (MultiPriv.TotalCommandsProcessed.load() < MultiPriv.ExpectedCommands.load() &&
               !MultiPriv.ShouldStop.load()) {
            // Poll each P2P service link for commands
            bool CommandFound = false;

            for (int clientIdx = 0; clientIdx < MultiPriv.ExpectedCommands.load(); clientIdx++) {
                // Skip clients that have already been processed
                if (MultiPriv.Clients[clientIdx].CommandAcknowledged.load()) {
                    continue;
                }

                IOC_LinkID_T currentSrvLinkID = MultiPriv.Clients[clientIdx].SrvLinkID;

                IOC_CMDDESC_DECLARE_VAR(CmdDesc);

                // Use non-blocking polling for each link
                IOC_Option_defineNonBlock(NonBlockOption);
                IOC_Result_T PollingResult = IOC_waitCMD(currentSrvLinkID, &CmdDesc, &NonBlockOption);

                if (PollingResult == IOC_RESULT_SUCCESS) {
                    CommandFound = true;
                    auto ReceiveTime = std::chrono::steady_clock::now();
                    IOC_CmdID_T CmdID = IOC_CmdDesc_getCmdID(&CmdDesc);

                    printf("[DEBUG] Service received command %llu from client %d on SrvLinkID %llu\n", CmdID, clientIdx,
                           currentSrvLinkID);

                    // Mark command as received for this specific client (correct mapping)
                    MultiPriv.Clients[clientIdx].CommandReceived = true;
                    MultiPriv.Clients[clientIdx].ReceivedCmdID = CmdID;
                    MultiPriv.Clients[clientIdx].ReceiveTime = ReceiveTime;

                    // Process command based on command ID, not client index assumption
                    std::string responseStr;
                    if (CmdID == IOC_CMDID_TEST_PING) {
                        responseStr = "PONG_" + std::to_string(clientIdx);
                    } else if (CmdID == IOC_CMDID_TEST_ECHO) {
                        responseStr = "ECHO_" + std::to_string(clientIdx);
                    } else {
                        responseStr = "UNKNOWN_" + std::to_string(clientIdx);
                    }

                    IOC_Result_T AckResult =
                        IOC_CmdDesc_setOutPayload(&CmdDesc, (void *)responseStr.c_str(), responseStr.length());
                    EXPECT_EQ(IOC_RESULT_SUCCESS, AckResult);

                    // Set command status to success
                    IOC_CmdDesc_setStatus(&CmdDesc, IOC_CMD_STATUS_SUCCESS);
                    IOC_CmdDesc_setResult(&CmdDesc, IOC_RESULT_SUCCESS);

                    // Send acknowledgment on the same P2P link
                    IOC_Result_T SendResult = IOC_ackCMD(currentSrvLinkID, &CmdDesc, NULL);
                    EXPECT_EQ(IOC_RESULT_SUCCESS, SendResult);

                    // Mark command as acknowledged
                    MultiPriv.Clients[clientIdx].CommandAcknowledged = true;
                    MultiPriv.Clients[clientIdx].AckTime = std::chrono::steady_clock::now();
                    MultiPriv.TotalCommandsProcessed++;

                    printf("[DEBUG] Service acknowledged command %llu for client %d (total: %d/%d)\n", CmdID, clientIdx,
                           MultiPriv.TotalCommandsProcessed.load(), MultiPriv.ExpectedCommands.load());
                } else if (PollingResult != IOC_RESULT_NO_CMD_PENDING) {
                    printf("[ERROR] Polling failed on client %d link with result: %d\n", clientIdx, PollingResult);
                }
            }

            if (!CommandFound) {
                // No commands available on any link, brief wait before retry
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
        }

        MultiPriv.PollingComplete = true;
        printf("[DEBUG] Multi-client polling completed\n");
    });

    // Wait for polling to start
    while (!MultiPriv.PollingStarted.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));  // Let polling be ready

    // Send commands from multiple clients concurrently
    for (int i = 0; i < MultiPriv.ExpectedCommands.load(); i++) {
        ClientThreads.emplace_back([&, i] {
            // Add minimal staggered timing to test independent processing
            std::this_thread::sleep_for(std::chrono::milliseconds(i * 20));

            IOC_CMDDESC_DECLARE_VAR(CmdDesc);
            CmdDesc.CmdID = MultiPriv.Clients[i].SentCmdID;

            printf("[DEBUG] Client %d sending command %llu\n", i, CmdDesc.CmdID);

            MultiPriv.Clients[i].SendTime = std::chrono::steady_clock::now();
            MultiPriv.Clients[i].CommandSent = true;

            IOC_Result_T ResultValueInThread = IOC_execCMD(MultiPriv.Clients[i].LinkID, &CmdDesc, NULL);

            MultiPriv.Clients[i].CommandResult = ResultValueInThread;

            // Verify command execution success
            EXPECT_EQ(IOC_RESULT_SUCCESS, ResultValueInThread);
            EXPECT_EQ(IOC_CMD_STATUS_SUCCESS, IOC_CmdDesc_getStatus(&CmdDesc));
            EXPECT_EQ(IOC_RESULT_SUCCESS, IOC_CmdDesc_getResult(&CmdDesc));

            // Verify response payload
            void *responseData = IOC_CmdDesc_getOutData(&CmdDesc);
            ULONG_T responseSize = IOC_CmdDesc_getOutDataSize(&CmdDesc);
            if (responseData != nullptr) {
                MultiPriv.Clients[i].ActualResponse = std::string((char *)responseData, responseSize);
                EXPECT_STREQ(MultiPriv.Clients[i].ExpectedResponse.c_str(),
                             MultiPriv.Clients[i].ActualResponse.c_str());
            }

            printf("[DEBUG] Client %d completed command with response: %s\n", i,
                   MultiPriv.Clients[i].ActualResponse.c_str());
        });
    }

    // Wait for all client command executions to complete
    for (auto &thread : ClientThreads) {
        if (thread.joinable()) thread.join();
    }

    // Wait for polling to complete processing all commands
    while (!MultiPriv.PollingComplete.load() &&
           MultiPriv.TotalCommandsProcessed.load() < MultiPriv.ExpectedCommands.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    MultiPriv.ShouldStop = true;
    if (SrvPollingThread.joinable()) SrvPollingThread.join();

    // Verify multi-client processing results
    ASSERT_EQ(MultiPriv.ExpectedCommands.load(), MultiPriv.TotalCommandsProcessed.load());

    printf("[DEBUG] Verifying independent client processing:\n");
    for (int i = 0; i < MultiPriv.ExpectedCommands.load(); i++) {
        printf("[DEBUG] Client %d: Sent=%s, Received=%s, Acknowledged=%s\n", i,
               MultiPriv.Clients[i].CommandSent.load() ? "true" : "false",
               MultiPriv.Clients[i].CommandReceived.load() ? "true" : "false",
               MultiPriv.Clients[i].CommandAcknowledged.load() ? "true" : "false");

        // Verify each client's command flow
        ASSERT_TRUE(MultiPriv.Clients[i].CommandSent.load()) << "Client " << i << " command was not sent";
        ASSERT_TRUE(MultiPriv.Clients[i].CommandReceived.load())
            << "Client " << i << " command was not received by service";
        ASSERT_TRUE(MultiPriv.Clients[i].CommandAcknowledged.load())
            << "Client " << i << " command was not acknowledged by service";

        // Verify command IDs match
        ASSERT_EQ(MultiPriv.Clients[i].SentCmdID, MultiPriv.Clients[i].ReceivedCmdID)
            << "Client " << i << " command ID mismatch";

        // Verify response content
        ASSERT_EQ(MultiPriv.Clients[i].ExpectedResponse, MultiPriv.Clients[i].ActualResponse)
            << "Client " << i << " response mismatch";

        // Verify SYNC guarantee: command was successful
        ASSERT_EQ(IOC_RESULT_SUCCESS, MultiPriv.Clients[i].CommandResult.load())
            << "Client " << i << " command execution failed";
    }

    // Verify timing independence (commands processed concurrently, not sequentially)
    auto MinSendTime = MultiPriv.Clients[0].SendTime;
    auto MaxAckTime = MultiPriv.Clients[0].AckTime;

    for (int i = 1; i < MultiPriv.ExpectedCommands.load(); i++) {
        if (MultiPriv.Clients[i].SendTime < MinSendTime) {
            MinSendTime = MultiPriv.Clients[i].SendTime;
        }
        if (MultiPriv.Clients[i].AckTime > MaxAckTime) {
            MaxAckTime = MultiPriv.Clients[i].AckTime;
        }
    }

    auto TotalProcessingTime = std::chrono::duration_cast<std::chrono::milliseconds>(MaxAckTime - MinSendTime);
    printf("[DEBUG] Total multi-client processing time: %lld ms\n", TotalProcessingTime.count());

    // Should process concurrently, not take 3x longer than single command
    ASSERT_LE(TotalProcessingTime.count(), 1000)  // Allow up to 1 second for concurrent processing
        << "Multi-client processing took too long: " << TotalProcessingTime.count() << "ms";

    printf("[DEBUG] Multi-client polling test completed successfully\n");

    // Cleanup
    for (int i = 0; i < MultiPriv.ExpectedCommands.load(); i++) {
        if (MultiPriv.Clients[i].LinkID != IOC_ID_INVALID) {
            IOC_closeLink(MultiPriv.Clients[i].LinkID);
        }
        if (MultiPriv.Clients[i].SrvLinkID != IOC_ID_INVALID) {
            IOC_closeLink(MultiPriv.Clients[i].SrvLinkID);
        }
    }
    if (SrvID != IOC_ID_INVALID) IOC_offlineService(SrvID);
}

// Service-to-polling-client data structure
typedef struct __ServiceToPollingClientPriv {
    std::atomic<bool> ClientPollingStarted{false};
    std::atomic<bool> ClientPollingComplete{false};
    std::atomic<bool> ShouldStop{false};
    std::atomic<int> CommandsProcessed{0};

    // Client-side tracking
    std::atomic<bool> CommandReceived{false};
    std::atomic<bool> CommandAcknowledged{false};
    IOC_CmdID_T ReceivedCmdID{0};
    IOC_CmdDesc_T ReceivedCmdDesc{};
    std::string ActualResponse;
    std::chrono::steady_clock::time_point ReceiveTime;
    std::chrono::steady_clock::time_point AckTime;

    // Service-side tracking
    std::atomic<bool> CommandSent{false};
    std::atomic<IOC_Result_T> ServiceResult{IOC_RESULT_BUG};
    IOC_CmdID_T SentCmdID{0};
    std::string ExpectedResponse{"CLIENT_PONG"};
    std::string ServiceReceivedResponse;
    std::chrono::steady_clock::time_point SendTime;
    std::chrono::steady_clock::time_point ResponseTime;

    IOC_LinkID_T SrvLinkID{IOC_ID_INVALID};  // Service-side link for command sending
    IOC_LinkID_T CliLinkID{IOC_ID_INVALID};  // Client-side link for polling

    std::mutex ProcessingMutex;
    std::condition_variable ProcessingCV;
} __ServiceToPollingClientPriv_T;

// Implement service-to-polling-client test
TEST(UT_ConetCommandWaitAck, verifyServiceToPollingClient_byStandardFlow_expectProperResponse) {
    IOC_Result_T ResultValue = IOC_RESULT_BUG;

    // Private data for tracking test state
    __ServiceToPollingClientPriv_T TestPriv = {};
    TestPriv.SentCmdID = IOC_CMDID_TEST_PING;

    // Service setup as CmdInitiator (reversed role)
    IOC_SrvURI_T SrvURI = {.pProtocol = IOC_SRV_PROTO_FIFO,
                           .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
                           .pPath = (const char *)"CmdWaitAck_ServiceToPollingClient"};

    // Service configured as CmdInitiator (NO command support, only sends commands)
    IOC_SrvArgs_T SrvArgs = {.SrvURI = SrvURI,
                             .Flags = IOC_SRVFLAG_NONE,
                             .UsageCapabilites = IOC_LinkUsageCmdInitiator,  // Service as initiator
                             .UsageArgs = {.pCmd = nullptr}};                // No command processing needed
    IOC_SrvID_T SrvID = IOC_ID_INVALID;
    ResultValue = IOC_onlineService(&SrvID, &SrvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);

    // Client setup with polling capability (CmdExecutor with polling)
    static IOC_CmdID_T ClientSupportedCmdIDs[] = {IOC_CMDID_TEST_PING, IOC_CMDID_TEST_ECHO};
    IOC_CmdUsageArgs_T ClientCmdUsageArgs = {.CbExecCmd_F = nullptr,  // NO CALLBACK - polling mode
                                             .pCbPrivData = nullptr,
                                             .CmdNum = 2,  // PING and ECHO commands
                                             .pCmdIDs = ClientSupportedCmdIDs};

    IOC_ConnArgs_T ConnArgs = {.SrvURI = SrvURI,
                               .Usage = IOC_LinkUsageCmdExecutor,  // Client as executor
                               .UsageArgs = {.pCmd = &ClientCmdUsageArgs}};

    // Client connection thread
    std::thread ClientThread([&] {
        IOC_Result_T ResultValueInThread = IOC_connectService(&TestPriv.CliLinkID, &ConnArgs, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValueInThread);
        ASSERT_NE(IOC_ID_INVALID, TestPriv.CliLinkID);
    });

    // Service accepts client connection
    ResultValue = IOC_acceptClient(SrvID, &TestPriv.SrvLinkID, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);
    ASSERT_NE(IOC_ID_INVALID, TestPriv.SrvLinkID);

    if (ClientThread.joinable()) ClientThread.join();

    // Start client polling thread (client side uses IOC_waitCMD + IOC_ackCMD)
    std::thread ClientPollingThread([&] {
        TestPriv.ClientPollingStarted = true;

        printf("[DEBUG] Client polling started, waiting for commands from service\n");

        while (!TestPriv.ShouldStop.load()) {
            IOC_CMDDESC_DECLARE_VAR(CmdDesc);

            // Use non-blocking polling to avoid infinite wait
            IOC_Option_defineNonBlock(NonBlockOption);
            IOC_Result_T PollingResult = IOC_waitCMD(TestPriv.CliLinkID, &CmdDesc, &NonBlockOption);

            if (PollingResult == IOC_RESULT_SUCCESS) {
                // Command received from service
                TestPriv.ReceiveTime = std::chrono::steady_clock::now();
                TestPriv.CommandReceived = true;
                TestPriv.ReceivedCmdID = IOC_CmdDesc_getCmdID(&CmdDesc);
                TestPriv.ReceivedCmdDesc = CmdDesc;  // Save command descriptor

                printf("[DEBUG] Client received command %llu from service\n", TestPriv.ReceivedCmdID);

                // Process the command (client-side logic)
                if (TestPriv.ReceivedCmdID == IOC_CMDID_TEST_PING) {
                    // Client responds with "CLIENT_PONG"
                    const char *response = TestPriv.ExpectedResponse.c_str();
                    IOC_Result_T AckResult =
                        IOC_CmdDesc_setOutPayload(&TestPriv.ReceivedCmdDesc, (void *)response, strlen(response));
                    EXPECT_EQ(IOC_RESULT_SUCCESS, AckResult);

                    TestPriv.ActualResponse = std::string(response);
                } else {
                    // Handle other commands if needed
                    const char *response = "CLIENT_UNKNOWN";
                    IOC_CmdDesc_setOutPayload(&TestPriv.ReceivedCmdDesc, (void *)response, strlen(response));
                    TestPriv.ActualResponse = std::string(response);
                }

                // Set command status to success
                IOC_CmdDesc_setStatus(&TestPriv.ReceivedCmdDesc, IOC_CMD_STATUS_SUCCESS);
                IOC_CmdDesc_setResult(&TestPriv.ReceivedCmdDesc, IOC_RESULT_SUCCESS);

                // Send acknowledgment back to service
                TestPriv.AckTime = std::chrono::steady_clock::now();
                IOC_Result_T SendResult = IOC_ackCMD(TestPriv.CliLinkID, &TestPriv.ReceivedCmdDesc, NULL);
                EXPECT_EQ(IOC_RESULT_SUCCESS, SendResult);

                TestPriv.CommandAcknowledged = true;
                TestPriv.CommandsProcessed++;

                printf("[DEBUG] Client acknowledged command %llu with response: %s\n", TestPriv.ReceivedCmdID,
                       TestPriv.ActualResponse.c_str());

                // Signal processing complete
                {
                    std::lock_guard<std::mutex> lock(TestPriv.ProcessingMutex);
                    TestPriv.ProcessingCV.notify_one();
                }

                break;  // Exit after processing one command
            } else if (PollingResult != IOC_RESULT_NO_CMD_PENDING) {
                printf("[ERROR] Client polling failed with result: %d\n", PollingResult);
                break;
            }

            // Brief wait before next polling attempt
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        TestPriv.ClientPollingComplete = true;
        printf("[DEBUG] Client polling completed\n");
    });

    // Wait for client polling to start
    while (!TestPriv.ClientPollingStarted.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));  // Let client polling be ready

    // Service sends command to polling client (service as CmdInitiator)
    printf("[DEBUG] Service sending command to polling client\n");

    IOC_CMDDESC_DECLARE_VAR(ServiceCmdDesc);
    ServiceCmdDesc.CmdID = TestPriv.SentCmdID;

    TestPriv.SendTime = std::chrono::steady_clock::now();
    TestPriv.CommandSent = true;

    // Service executes command on client (SYNC operation)
    ResultValue = IOC_execCMD(TestPriv.SrvLinkID, &ServiceCmdDesc, NULL);

    TestPriv.ResponseTime = std::chrono::steady_clock::now();
    TestPriv.ServiceResult = ResultValue;

    printf("[DEBUG] Service command execution completed with result: %d\n", ResultValue);

    // Wait for client processing to complete
    {
        std::unique_lock<std::mutex> lock(TestPriv.ProcessingMutex);
        TestPriv.ProcessingCV.wait_for(lock, std::chrono::seconds(5),
                                       [&] { return TestPriv.CommandAcknowledged.load(); });
    }

    TestPriv.ShouldStop = true;
    if (ClientPollingThread.joinable()) ClientPollingThread.join();

    // Verify service command execution results
    ASSERT_EQ(IOC_RESULT_SUCCESS, TestPriv.ServiceResult.load()) << "Service command execution should succeed";

    // Verify command status from service perspective
    IOC_CmdStatus_E ServiceCmdStatus = IOC_CmdDesc_getStatus(&ServiceCmdDesc);
    ASSERT_EQ(IOC_CMD_STATUS_SUCCESS, ServiceCmdStatus) << "Service should receive successful command status";

    IOC_Result_T ServiceCmdResult = IOC_CmdDesc_getResult(&ServiceCmdDesc);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ServiceCmdResult) << "Service should receive successful command result";

    // Verify response payload received by service
    void *serviceResponseData = IOC_CmdDesc_getOutData(&ServiceCmdDesc);
    ULONG_T serviceResponseSize = IOC_CmdDesc_getOutDataSize(&ServiceCmdDesc);
    ASSERT_TRUE(serviceResponseData != nullptr) << "Service should receive response data";
    ASSERT_EQ(TestPriv.ExpectedResponse.length(), serviceResponseSize) << "Service response size should match expected";

    TestPriv.ServiceReceivedResponse = std::string((char *)serviceResponseData, serviceResponseSize);
    ASSERT_STREQ(TestPriv.ExpectedResponse.c_str(), TestPriv.ServiceReceivedResponse.c_str())
        << "Service should receive expected response content";

    // Verify client-side processing
    ASSERT_TRUE(TestPriv.CommandReceived.load()) << "Client should have received command from service";
    ASSERT_TRUE(TestPriv.CommandAcknowledged.load()) << "Client should have acknowledged the command";
    ASSERT_EQ(TestPriv.SentCmdID, TestPriv.ReceivedCmdID)
        << "Client should receive the same command ID that service sent";
    ASSERT_EQ(1, TestPriv.CommandsProcessed.load()) << "Client should have processed exactly one command";

    // Verify response consistency
    ASSERT_EQ(TestPriv.ExpectedResponse, TestPriv.ActualResponse) << "Client response should match expected";
    ASSERT_EQ(TestPriv.ActualResponse, TestPriv.ServiceReceivedResponse)
        << "Service should receive the same response that client sent";

    // Verify timing (SYNC guarantee)
    auto TotalDuration =
        std::chrono::duration_cast<std::chrono::milliseconds>(TestPriv.ResponseTime - TestPriv.SendTime);
    auto ProcessingDuration =
        std::chrono::duration_cast<std::chrono::milliseconds>(TestPriv.AckTime - TestPriv.ReceiveTime);

    printf("[DEBUG] Total service command duration: %lld ms\n", TotalDuration.count());
    printf("[DEBUG] Client processing duration: %lld ms\n", ProcessingDuration.count());

    // SYNC guarantee: service waits for client processing to complete
    ASSERT_GE(TotalDuration.count(), ProcessingDuration.count() - 50)
        << "Service should wait for client processing (SYNC guarantee)";

    // Reasonable timing bounds
    ASSERT_LE(TotalDuration.count(), 5000) << "Service command should complete within reasonable time";

    printf("[DEBUG] Service-to-polling-client test completed successfully\n");

    // Cleanup
    if (TestPriv.CliLinkID != IOC_ID_INVALID) IOC_closeLink(TestPriv.CliLinkID);
    if (TestPriv.SrvLinkID != IOC_ID_INVALID) IOC_closeLink(TestPriv.SrvLinkID);
    if (SrvID != IOC_ID_INVALID) IOC_offlineService(SrvID);
}

// TODO: Implement service orchestration test with polling clients
TEST(UT_ConetCommandWaitAck, verifyServiceOrchestration_byPollingClients_expectReliableCollection) {
    // TODO: Implement service setup as CmdInitiator for multiple clients
    // TODO: Implement multiple clients with polling capability
    // TODO: Implement service orchestration of different commands to different clients
    // TODO: Verify each client processes via polling patterns independently
    // TODO: Validate service collects all results reliably
    GTEST_SKIP() << "TODO: Implement multi-client polling orchestration patterns";
}

// ⚪ IMPLEMENTATION STATUS TRACKING - Polling Patterns TODO
// IOC_waitCMD + IOC_ackCMD polling patterns need full implementation
//
// ⚪ PLANNED IMPLEMENTATION ITEMS:
//   ⚪ Basic polling detection: IOC_waitCMD command detection loops
//   ⚪ Manual acknowledgment: IOC_ackCMD response sending patterns
//   ⚪ Delayed response processing: Delayed acknowledgment within SYNC execution framework
//   ⚪ Polling timeouts: IOC_waitCMD timeout behavior validation
//   ⚪ Multi-client polling: Independent command tracking with SYNC guarantees
//   ⚪ Service-to-client polling: Service as CmdInitiator with polling client executors
//   ⚪ Client orchestration: Multi-client coordination via polling patterns
//
// 🎯 GOAL: Implement complete bidirectional polling-based command patterns
//    Provide alternative command processing model with manual response control
//    while maintaining CMD's SYNC+MAYBLOCK+NODROP architectural constraints
//    Coverage: Client→Service polling (US-1) + Service→Client polling (US-2)

//======>END OF TEST CASES==========================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF IMPLEMENTATION NOTES=============================================================
/**
 * Key Implementation Areas:
 *
 * 1. Service Polling Setup:
 *    - Configure service WITHOUT CbExecCmd_F callback
 *    - Enable command queuing for IOC_waitCMD detection
 *    - Setup polling loops with proper timeout handling
 *
 * 2. IOC_waitCMD Patterns:
 *    - Blocking vs non-blocking polling modes
 *    - Timeout configuration and handling
 *    - Command queue management and detection
 *
 * 3. IOC_ackCMD Patterns:
 *    - Immediate vs delayed acknowledgment
 *    - Response data preparation and sending
 *    - Acknowledgment tracking and client notification
 *
 * 4. Delayed Response Processing:
 *    - Background command processing threads
 *    - Command state tracking between detection and acknowledgment
 *    - Proper resource cleanup and response timing within SYNC framework
 *    - SYNC constraint: IOC_execCMD caller waits for final result despite delayed processing
 *
 * 5. Comparison Studies:
 *    - Performance: Polling vs Callback patterns
 *    - Flexibility: Manual vs Automatic response control
 *    - Use Cases: When to use polling vs callback approaches
 *    - Architecture: SYNC+MAYBLOCK+NODROP compliance in all patterns
 */
//======>END OF IMPLEMENTATION NOTES===============================================================
