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
// 🟢 IMPLEMENTATION STATUS:
//     🟢 GREEN: Comprehensive polling-based command patterns implemented and tested
//     🟢 All 7 test cases implemented: 7/7 PASSED - Complete success! 🎉
//     🟢 IOC_waitCMD, IOC_ackCMD workflows fully implemented vs CbExecCmd_F callback patterns
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
 *  🟢 TC-1: verifyServiceOrchestration_byPollingClients_expectReliableCollection
 *      @[Purpose]: Validate service orchestration of multiple clients using polling patterns
 *      @[Brief]: Service coordinates multiple client operations via polling-based command execution
 *      @[Status]: ✅ PASSED - Multi-client polling orchestration implemented and working
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
// 🟢 COMPLETED: Basic polling pattern implemented and tested
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
        // Generate expected response based on command type using generic multi-client format
        if (MultiPriv.Clients[i].SentCmdID == IOC_CMDID_TEST_PING) {
            MultiPriv.Clients[i].ExpectedResponse = "PONG_MULTI";
        } else {
            MultiPriv.Clients[i].ExpectedResponse = "ECHO_MULTI";
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

    // Accept all client connections into a service link pool
    std::vector<IOC_LinkID_T> ServiceLinkPool;
    for (int i = 0; i < MultiPriv.ExpectedCommands.load(); i++) {
        IOC_LinkID_T SrvLinkID = IOC_ID_INVALID;
        ResultValue = IOC_acceptClient(SrvID, &SrvLinkID, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);
        ASSERT_NE(IOC_ID_INVALID, SrvLinkID);

        // Store each service link ID in a pool (order-independent)
        ServiceLinkPool.push_back(SrvLinkID);
    }

    // Wait for all client threads to complete connection
    for (auto &thread : ClientThreads) {
        if (thread.joinable()) thread.join();
    }
    ClientThreads.clear();

    // Start service polling thread using service link pool approach
    std::thread SrvPollingThread([&] {
        MultiPriv.PollingStarted = true;

        printf("[DEBUG] Multi-client polling started, waiting for %d commands\n", MultiPriv.ExpectedCommands.load());

        while (MultiPriv.TotalCommandsProcessed.load() < MultiPriv.ExpectedCommands.load() &&
               !MultiPriv.ShouldStop.load()) {
            // Poll all service links in pool for commands
            bool CommandFound = false;

            for (IOC_LinkID_T poolLinkID : ServiceLinkPool) {
                IOC_CMDDESC_DECLARE_VAR(CmdDesc);

                // Use non-blocking polling for each pool link
                IOC_Option_defineNonBlock(NonBlockOption);
                IOC_Result_T PollingResult = IOC_waitCMD(poolLinkID, &CmdDesc, &NonBlockOption);

                if (PollingResult == IOC_RESULT_SUCCESS) {
                    CommandFound = true;
                    auto ReceiveTime = std::chrono::steady_clock::now();
                    IOC_CmdID_T CmdID = IOC_CmdDesc_getCmdID(&CmdDesc);

                    printf("[DEBUG] Service received command %llu on pool link %llu\n", CmdID, poolLinkID);

                    // Generate response using client identification approach
                    std::string responseStr;
                    if (CmdID == IOC_CMDID_TEST_PING) {
                        // PING commands from Client 0 should get PONG_0 response
                        responseStr = "PONG_MULTI";
                    } else if (CmdID == IOC_CMDID_TEST_ECHO) {
                        // ECHO commands from Client 1,2 should get ECHO_MULTI response
                        responseStr = "ECHO_MULTI";
                    } else {
                        responseStr = "UNKNOWN_MULTI";
                    }

                    IOC_Result_T AckResult =
                        IOC_CmdDesc_setOutPayload(&CmdDesc, (void *)responseStr.c_str(), responseStr.length());
                    EXPECT_EQ(IOC_RESULT_SUCCESS, AckResult);

                    // Set command status to success
                    IOC_CmdDesc_setStatus(&CmdDesc, IOC_CMD_STATUS_SUCCESS);
                    IOC_CmdDesc_setResult(&CmdDesc, IOC_RESULT_SUCCESS);

                    // Send acknowledgment on the same pool link
                    IOC_Result_T SendResult = IOC_ackCMD(poolLinkID, &CmdDesc, NULL);
                    EXPECT_EQ(IOC_RESULT_SUCCESS, SendResult);

                    MultiPriv.TotalCommandsProcessed++;

                    printf("[DEBUG] Service acknowledged command %llu (total: %d/%d)\n", CmdID,
                           MultiPriv.TotalCommandsProcessed.load(), MultiPriv.ExpectedCommands.load());

                    // Find which client received this response by parsing the response
                    for (int clientIdx = 0; clientIdx < MultiPriv.ExpectedCommands.load(); clientIdx++) {
                        if (MultiPriv.Clients[clientIdx].SentCmdID == CmdID &&
                            !MultiPriv.Clients[clientIdx].CommandReceived.load()) {
                            MultiPriv.Clients[clientIdx].CommandReceived = true;
                            MultiPriv.Clients[clientIdx].ReceivedCmdID = CmdID;
                            MultiPriv.Clients[clientIdx].ReceiveTime = ReceiveTime;
                            MultiPriv.Clients[clientIdx].CommandAcknowledged = true;
                            MultiPriv.Clients[clientIdx].AckTime = std::chrono::steady_clock::now();
                            break;
                        }
                    }
                } else if (PollingResult != IOC_RESULT_NO_CMD_PENDING) {
                    printf("[ERROR] Polling failed on pool link %llu with result: %d\n", poolLinkID, PollingResult);
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
    }
    for (IOC_LinkID_T srvLinkID : ServiceLinkPool) {
        if (srvLinkID != IOC_ID_INVALID) {
            IOC_closeLink(srvLinkID);
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

// Service orchestration data structure for multiple polling clients
typedef struct __ServiceOrchestrationPriv {
    std::atomic<bool> OrchestrationStarted{false};
    std::atomic<bool> OrchestrationComplete{false};
    std::atomic<bool> ShouldStop{false};
    std::atomic<int> TotalClientsOrchestrated{0};
    std::atomic<int> ExpectedClients{0};

    // Per-client orchestration tracking
    struct ClientOrchInfo {
        IOC_LinkID_T SrvLinkID{IOC_ID_INVALID};  // Service-side link for sending commands
        IOC_LinkID_T CliLinkID{IOC_ID_INVALID};  // Client-side link for polling

        // Client polling thread tracking
        std::atomic<bool> ClientPollingStarted{false};
        std::atomic<bool> ClientPollingComplete{false};
        std::atomic<bool> CommandReceived{false};
        std::atomic<bool> CommandAcknowledged{false};

        // Service orchestration tracking
        std::atomic<bool> CommandSent{false};
        std::atomic<IOC_Result_T> ServiceResult{IOC_RESULT_BUG};

        // Command and response tracking
        IOC_CmdID_T AssignedCmdID{0};    // Command assigned by service orchestrator
        IOC_CmdID_T ReceivedCmdID{0};    // Command received by client
        std::string ExpectedResponse;    // Response expected by service
        std::string ActualResponse;      // Response actually received by service
        std::string ClientSentResponse;  // Response sent by client

        // Timing tracking
        std::chrono::steady_clock::time_point ServiceSendTime;
        std::chrono::steady_clock::time_point ClientReceiveTime;
        std::chrono::steady_clock::time_point ClientAckTime;
        std::chrono::steady_clock::time_point ServiceResponseTime;

        int ClientIndex{0};
        std::thread ClientPollingThread;
        std::mutex ClientProcessingMutex;
        std::condition_variable ClientProcessingCV;
    };

    static const int MAX_ORCHESTRATED_CLIENTS = 4;
    ClientOrchInfo Clients[MAX_ORCHESTRATED_CLIENTS];

    std::mutex OrchestrationMutex;
    std::condition_variable OrchestrationCV;
} __ServiceOrchestrationPriv_T;

// Implement service orchestration test with multiple polling clients
TEST(UT_ConetCommandWaitAck, verifyServiceOrchestration_byPollingClients_expectReliableCollection) {
    IOC_Result_T ResultValue = IOC_RESULT_BUG;

    // Private data for tracking orchestration state
    __ServiceOrchestrationPriv_T OrchPriv = {};
    OrchPriv.ExpectedClients = 4;  // Test with 4 clients for comprehensive orchestration

    // Service setup as CmdInitiator (orchestrator role)
    IOC_SrvURI_T SrvURI = {.pProtocol = IOC_SRV_PROTO_FIFO,
                           .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
                           .pPath = (const char *)"CmdWaitAck_ServiceOrchestration"};

    // Service configured as CmdInitiator (orchestrates multiple clients)
    IOC_SrvArgs_T SrvArgs = {.SrvURI = SrvURI,
                             .Flags = IOC_SRVFLAG_NONE,
                             .UsageCapabilites = IOC_LinkUsageCmdInitiator,  // Service as orchestrator
                             .UsageArgs = {.pCmd = nullptr}};                // No command processing needed
    IOC_SrvID_T SrvID = IOC_ID_INVALID;
    ResultValue = IOC_onlineService(&SrvID, &SrvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);

    // Setup multiple clients with different command assignments for orchestration testing
    std::vector<std::thread> ClientConnectionThreads;

    // Define orchestration plan: different commands for different clients
    IOC_CmdID_T OrchestrationCmds[] = {
        IOC_CMDID_TEST_PING,  // Client 0: PING command
        IOC_CMDID_TEST_ECHO,  // Client 1: ECHO command
        IOC_CMDID_TEST_PING,  // Client 2: PING command (duplicate for testing)
        IOC_CMDID_TEST_ECHO   // Client 3: ECHO command (duplicate for testing)
    };

    // Expected responses for orchestration validation
    std::string ExpectedResponses[] = {
        "ORCH_PING_0",  // Client 0 specialized response
        "ORCH_ECHO_1",  // Client 1 specialized response
        "ORCH_PING_2",  // Client 2 specialized response
        "ORCH_ECHO_3"   // Client 3 specialized response
    };

    // Client setup with polling capability (each as CmdExecutor with polling)
    static IOC_CmdID_T ClientSupportedCmdIDs[] = {IOC_CMDID_TEST_PING, IOC_CMDID_TEST_ECHO};
    IOC_CmdUsageArgs_T ClientCmdUsageArgs = {.CbExecCmd_F = nullptr,  // NO CALLBACK - polling mode
                                             .pCbPrivData = nullptr,
                                             .CmdNum = 2,  // PING and ECHO commands
                                             .pCmdIDs = ClientSupportedCmdIDs};

    for (int i = 0; i < OrchPriv.ExpectedClients.load(); i++) {
        OrchPriv.Clients[i].ClientIndex = i;
        OrchPriv.Clients[i].AssignedCmdID = OrchestrationCmds[i];
        OrchPriv.Clients[i].ExpectedResponse = ExpectedResponses[i];

        ClientConnectionThreads.emplace_back([&, i] {
            IOC_ConnArgs_T ConnArgs = {.SrvURI = SrvURI,
                                       .Usage = IOC_LinkUsageCmdExecutor,  // Client as executor
                                       .UsageArgs = {.pCmd = &ClientCmdUsageArgs}};

            IOC_Result_T ResultValueInThread = IOC_connectService(&OrchPriv.Clients[i].CliLinkID, &ConnArgs, NULL);
            ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValueInThread);
            ASSERT_NE(IOC_ID_INVALID, OrchPriv.Clients[i].CliLinkID);
        });
    }

    // Service accepts all client connections into a pool
    std::vector<IOC_LinkID_T> ServiceLinkPool;
    for (int i = 0; i < OrchPriv.ExpectedClients.load(); i++) {
        IOC_LinkID_T SrvLinkID = IOC_ID_INVALID;
        ResultValue = IOC_acceptClient(SrvID, &SrvLinkID, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);
        ASSERT_NE(IOC_ID_INVALID, SrvLinkID);
        ServiceLinkPool.push_back(SrvLinkID);
    }

    // Wait for all client connections to complete
    for (auto &thread : ClientConnectionThreads) {
        if (thread.joinable()) thread.join();
    }
    ClientConnectionThreads.clear();

    // Start client polling threads (each client runs independent polling)
    for (int i = 0; i < OrchPriv.ExpectedClients.load(); i++) {
        OrchPriv.Clients[i].ClientPollingThread = std::thread([&, i] {
            OrchPriv.Clients[i].ClientPollingStarted = true;

            printf("[DEBUG] Client %d polling started, waiting for orchestration commands\n", i);

            while (!OrchPriv.ShouldStop.load()) {
                IOC_CMDDESC_DECLARE_VAR(CmdDesc);

                // Use non-blocking polling to avoid infinite wait
                IOC_Option_defineNonBlock(NonBlockOption);
                IOC_Result_T PollingResult = IOC_waitCMD(OrchPriv.Clients[i].CliLinkID, &CmdDesc, &NonBlockOption);

                if (PollingResult == IOC_RESULT_SUCCESS) {
                    // Command received from service orchestrator
                    OrchPriv.Clients[i].ClientReceiveTime = std::chrono::steady_clock::now();
                    OrchPriv.Clients[i].CommandReceived = true;
                    OrchPriv.Clients[i].ReceivedCmdID = IOC_CmdDesc_getCmdID(&CmdDesc);

                    printf("[DEBUG] Client %d received orchestration command %llu\n", i,
                           OrchPriv.Clients[i].ReceivedCmdID);

                    // Process the orchestration command (client-side specialized logic)
                    std::string response;
                    if (OrchPriv.Clients[i].ReceivedCmdID == IOC_CMDID_TEST_PING) {
                        response = "ORCH_PING_" + std::to_string(i);
                    } else if (OrchPriv.Clients[i].ReceivedCmdID == IOC_CMDID_TEST_ECHO) {
                        response = "ORCH_ECHO_" + std::to_string(i);
                    } else {
                        response = "ORCH_UNKNOWN_" + std::to_string(i);
                    }

                    OrchPriv.Clients[i].ClientSentResponse = response;

                    IOC_Result_T AckResult =
                        IOC_CmdDesc_setOutPayload(&CmdDesc, (void *)response.c_str(), response.length());
                    EXPECT_EQ(IOC_RESULT_SUCCESS, AckResult);

                    // Set command status to success
                    IOC_CmdDesc_setStatus(&CmdDesc, IOC_CMD_STATUS_SUCCESS);
                    IOC_CmdDesc_setResult(&CmdDesc, IOC_RESULT_SUCCESS);

                    // Send acknowledgment back to service orchestrator
                    OrchPriv.Clients[i].ClientAckTime = std::chrono::steady_clock::now();
                    IOC_Result_T SendResult = IOC_ackCMD(OrchPriv.Clients[i].CliLinkID, &CmdDesc, NULL);
                    EXPECT_EQ(IOC_RESULT_SUCCESS, SendResult);

                    OrchPriv.Clients[i].CommandAcknowledged = true;

                    printf("[DEBUG] Client %d acknowledged orchestration command with response: %s\n", i,
                           response.c_str());

                    // Signal processing complete
                    {
                        std::lock_guard<std::mutex> lock(OrchPriv.Clients[i].ClientProcessingMutex);
                        OrchPriv.Clients[i].ClientProcessingCV.notify_one();
                    }

                    break;  // Exit after processing orchestration command
                } else if (PollingResult != IOC_RESULT_NO_CMD_PENDING) {
                    printf("[ERROR] Client %d polling failed with result: %d\n", i, PollingResult);
                    break;
                }

                // Brief wait before next polling attempt
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }

            OrchPriv.Clients[i].ClientPollingComplete = true;
            printf("[DEBUG] Client %d polling completed\n", i);
        });
    }

    // Wait for all client polling to start
    for (int i = 0; i < OrchPriv.ExpectedClients.load(); i++) {
        while (!OrchPriv.Clients[i].ClientPollingStarted.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(200));  // Let all clients be ready for orchestration

    // Service orchestration: send different commands to different clients
    printf("[DEBUG] Service starting orchestration of %d polling clients\n", OrchPriv.ExpectedClients.load());

    OrchPriv.OrchestrationStarted = true;

    // Orchestrate clients using the service link pool (distribute commands across available links)
    std::vector<std::thread> OrchestrationThreads;
    std::atomic<int> LinkPoolIndex{0};

    for (int i = 0; i < OrchPriv.ExpectedClients.load(); i++) {
        OrchestrationThreads.emplace_back([&, i] {
            // Stagger orchestration commands slightly for testing independent processing
            std::this_thread::sleep_for(std::chrono::milliseconds(i * 50));

            // Get next available service link from pool
            int poolIdx = LinkPoolIndex.fetch_add(1);
            IOC_LinkID_T selectedServiceLink = ServiceLinkPool[poolIdx];

            IOC_CMDDESC_DECLARE_VAR(ServiceCmdDesc);
            ServiceCmdDesc.CmdID = OrchPriv.Clients[i].AssignedCmdID;

            printf("[DEBUG] Service orchestrating command slot %d (cmd=%llu) using service link %llu\n", i,
                   ServiceCmdDesc.CmdID, selectedServiceLink);

            OrchPriv.Clients[i].ServiceSendTime = std::chrono::steady_clock::now();
            OrchPriv.Clients[i].CommandSent = true;

            // Service executes orchestration command via selected service link (SYNC operation)
            IOC_Result_T ResultValueInThread = IOC_execCMD(selectedServiceLink, &ServiceCmdDesc, NULL);

            OrchPriv.Clients[i].ServiceResponseTime = std::chrono::steady_clock::now();
            OrchPriv.Clients[i].ServiceResult = ResultValueInThread;

            printf("[DEBUG] Service orchestration slot %d completed with result: %d\n", i, ResultValueInThread);

            if (ResultValueInThread == IOC_RESULT_SUCCESS) {
                // Extract response from orchestrated client
                void *serviceResponseData = IOC_CmdDesc_getOutData(&ServiceCmdDesc);
                ULONG_T serviceResponseSize = IOC_CmdDesc_getOutDataSize(&ServiceCmdDesc);

                if (serviceResponseData != nullptr) {
                    std::string response((char *)serviceResponseData, serviceResponseSize);

                    // Parse the response to determine which client actually responded
                    // Format: "ORCH_PING_X" or "ORCH_ECHO_X" where X is the client index
                    int respondingClientIndex = -1;
                    if (response.find("ORCH_PING_") == 0 || response.find("ORCH_ECHO_") == 0) {
                        size_t underscorePos = response.find_last_of('_');
                        if (underscorePos != std::string::npos && underscorePos + 1 < response.length()) {
                            respondingClientIndex = std::stoi(response.substr(underscorePos + 1));
                        }
                    }

                    if (respondingClientIndex >= 0 && respondingClientIndex < OrchPriv.ExpectedClients.load()) {
                        OrchPriv.Clients[respondingClientIndex].ActualResponse = response;
                    } else {
                        printf("[WARNING] Service received unidentifiable response: %s\n", response.c_str());
                    }
                }

                OrchPriv.TotalClientsOrchestrated++;
                printf("[DEBUG] Service collected response from command slot %d: result=%d (total: %d/%d)\n", i,
                       ResultValueInThread, OrchPriv.TotalClientsOrchestrated.load(), OrchPriv.ExpectedClients.load());
            }
        });
    }

    // Wait for all orchestration to complete
    for (auto &thread : OrchestrationThreads) {
        if (thread.joinable()) thread.join();
    }

    // Wait for all client processing to complete
    for (int i = 0; i < OrchPriv.ExpectedClients.load(); i++) {
        std::unique_lock<std::mutex> lock(OrchPriv.Clients[i].ClientProcessingMutex);
        OrchPriv.Clients[i].ClientProcessingCV.wait_for(
            lock, std::chrono::seconds(5), [&, i] { return OrchPriv.Clients[i].CommandAcknowledged.load(); });
    }

    OrchPriv.ShouldStop = true;
    OrchPriv.OrchestrationComplete = true;

    // Wait for all client polling threads to complete
    for (int i = 0; i < OrchPriv.ExpectedClients.load(); i++) {
        if (OrchPriv.Clients[i].ClientPollingThread.joinable()) {
            OrchPriv.Clients[i].ClientPollingThread.join();
        }
    }

    // Verify orchestration results
    printf("[DEBUG] Verifying service orchestration results:\n");

    // Verify overall orchestration success
    ASSERT_EQ(OrchPriv.ExpectedClients.load(), OrchPriv.TotalClientsOrchestrated.load())
        << "Service should have successfully orchestrated all clients";

    ASSERT_TRUE(OrchPriv.OrchestrationStarted.load()) << "Orchestration should have started";
    ASSERT_TRUE(OrchPriv.OrchestrationComplete.load()) << "Orchestration should have completed";

    // Verify each client's orchestrated processing with pool-based command distribution
    int TotalPINGCmds = 0;
    int TotalECHOCmds = 0;
    int TotalResponsesCollected = 0;

    for (int i = 0; i < OrchPriv.ExpectedClients.load(); i++) {
        printf("[DEBUG] Client %d: Sent=%s, Received=%s, Acknowledged=%s, Response=%s\n", i,
               OrchPriv.Clients[i].CommandSent.load() ? "true" : "false",
               OrchPriv.Clients[i].CommandReceived.load() ? "true" : "false",
               OrchPriv.Clients[i].CommandAcknowledged.load() ? "true" : "false",
               OrchPriv.Clients[i].ActualResponse.c_str());

        // Verify individual client orchestration completion
        ASSERT_TRUE(OrchPriv.Clients[i].CommandReceived.load())
            << "Client " << i << " should have received orchestration command via polling";
        ASSERT_TRUE(OrchPriv.Clients[i].CommandAcknowledged.load())
            << "Client " << i << " should have acknowledged orchestration command";

        // Count command types received and responses collected
        if (OrchPriv.Clients[i].ReceivedCmdID == IOC_CMDID_TEST_PING) TotalPINGCmds++;
        if (OrchPriv.Clients[i].ReceivedCmdID == IOC_CMDID_TEST_ECHO) TotalECHOCmds++;
        if (!OrchPriv.Clients[i].ActualResponse.empty()) TotalResponsesCollected++;

        // Verify response format is correct for the command type received
        if (OrchPriv.Clients[i].ReceivedCmdID == IOC_CMDID_TEST_PING) {
            ASSERT_EQ(OrchPriv.Clients[i].ActualResponse, "ORCH_PING_" + std::to_string(i))
                << "Client " << i << " should send PING response with correct client index";
        } else if (OrchPriv.Clients[i].ReceivedCmdID == IOC_CMDID_TEST_ECHO) {
            ASSERT_EQ(OrchPriv.Clients[i].ActualResponse, "ORCH_ECHO_" + std::to_string(i))
                << "Client " << i << " should send ECHO response with correct client index";
        } else {
            FAIL() << "Client " << i << " received unexpected command ID: " << OrchPriv.Clients[i].ReceivedCmdID;
        }

        // Verify client polling threads completed properly
        ASSERT_TRUE(OrchPriv.Clients[i].ClientPollingStarted.load())
            << "Client " << i << " polling should have started";
        ASSERT_TRUE(OrchPriv.Clients[i].ClientPollingComplete.load())
            << "Client " << i << " polling should have completed";
    }

    // Verify orchestration distribution (planned commands were distributed correctly)
    ASSERT_EQ(OrchPriv.ExpectedClients.load(), TotalResponsesCollected)
        << "Service should have collected responses from all clients";

    // Verify command type distribution (we planned 2 PING + 2 ECHO commands)
    ASSERT_EQ(2, TotalPINGCmds) << "Exactly 2 clients should have received PING commands";
    ASSERT_EQ(2, TotalECHOCmds) << "Exactly 2 clients should have received ECHO commands";

    // Verify timing independence (orchestration processed concurrently)
    auto MinSendTime = OrchPriv.Clients[0].ServiceSendTime;
    auto MaxResponseTime = OrchPriv.Clients[0].ServiceResponseTime;

    for (int i = 1; i < OrchPriv.ExpectedClients.load(); i++) {
        if (OrchPriv.Clients[i].ServiceSendTime < MinSendTime) {
            MinSendTime = OrchPriv.Clients[i].ServiceSendTime;
        }
        if (OrchPriv.Clients[i].ServiceResponseTime > MaxResponseTime) {
            MaxResponseTime = OrchPriv.Clients[i].ServiceResponseTime;
        }
    }

    auto TotalOrchestrationTime = std::chrono::duration_cast<std::chrono::milliseconds>(MaxResponseTime - MinSendTime);
    printf("[DEBUG] Total orchestration time: %lld ms\n", TotalOrchestrationTime.count());

    // Should orchestrate efficiently, not take excessively long
    ASSERT_LE(TotalOrchestrationTime.count(), 3000)  // Allow up to 3 seconds for orchestration
        << "Service orchestration took too long: " << TotalOrchestrationTime.count() << "ms";

    // Verify orchestration response diversity (different clients gave different specialized responses)
    std::set<std::string> UniqueResponses;
    for (int i = 0; i < OrchPriv.ExpectedClients.load(); i++) {
        UniqueResponses.insert(OrchPriv.Clients[i].ActualResponse);
    }
    ASSERT_EQ(OrchPriv.ExpectedClients.load(), UniqueResponses.size())
        << "Service should receive distinct responses from different clients";

    printf("[DEBUG] Service orchestration test completed successfully\n");
    printf("[DEBUG] Orchestrated %d clients with %zu unique responses in %lld ms\n",
           OrchPriv.TotalClientsOrchestrated.load(), UniqueResponses.size(), TotalOrchestrationTime.count());

    // Cleanup
    for (int i = 0; i < OrchPriv.ExpectedClients.load(); i++) {
        if (OrchPriv.Clients[i].CliLinkID != IOC_ID_INVALID) {
            IOC_closeLink(OrchPriv.Clients[i].CliLinkID);
        }
    }
    for (IOC_LinkID_T srvLinkID : ServiceLinkPool) {
        if (srvLinkID != IOC_ID_INVALID) {
            IOC_closeLink(srvLinkID);
        }
    }
    if (SrvID != IOC_ID_INVALID) IOC_offlineService(SrvID);
}

// 🟢 IMPLEMENTATION STATUS TRACKING - Polling Patterns COMPLETED
// 🟢 IOC_waitCMD + IOC_ackCMD polling patterns fully implemented and tested
//
// 🟢 COMPLETED IMPLEMENTATION ITEMS:
//   🟢 Basic polling detection: IOC_waitCMD command detection loops
//   🟢 Manual acknowledgment: IOC_ackCMD response sending patterns
//   🟢 Delayed response processing: Delayed acknowledgment within SYNC execution framework
//   🟢 Polling timeouts: IOC_waitCMD timeout behavior validation
//   🟢 Multi-client polling: Independent command tracking with SYNC guarantees (FIXED!)
//   🟢 Service-to-client polling: Service as CmdInitiator with polling client executors
//   🟢 Client orchestration: Multi-client coordination via polling patterns
//
// 🎯 GOAL ACHIEVED: Complete bidirectional polling-based command patterns implemented
//    ✅ Alternative command processing model with manual response control
//    ✅ CMD's SYNC+MAYBLOCK+NODROP architectural constraints maintained
//    ✅ Coverage: Client→Service polling (US-1) + Service→Client polling (US-2)
//    📊 TEST RESULTS: 7/7 PASSED (100% success rate) 🎉

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
