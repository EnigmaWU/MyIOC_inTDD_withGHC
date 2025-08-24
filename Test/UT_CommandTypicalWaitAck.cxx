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
 *  ⚪ TC-1: verifyServicePollingTimeout_byEmptyQueue_expectTimeoutHandling
 *      @[Purpose]: Validate IOC_waitCMD timeout behavior when no commands available
 *      @[Brief]: Service polls with timeout, handles empty queue gracefully
 *      @[Status]: TODO - Implement polling timeout validation
 *
 * [@AC-4,US-1] Multi-client polling and independent acknowledgment tracking
 *  ⚪ TC-1: verifyServiceMultiClientPolling_byIndependentAck_expectProperTracking
 *      @[Purpose]: Validate independent command tracking and acknowledgment for multiple clients
 *      @[Brief]: Multiple clients send commands, service polls and tracks each independently
 *      @[Status]: TODO - Implement multi-client polling patterns with SYNC guarantees
 *
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * 📋 [US-2]: Service as CmdInitiator (Server→Client Polling Command Patterns)
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 *
 * [@AC-1,US-2] Service interaction with polling-based client executors
 *  ⚪ TC-1: verifyServiceToPollingClient_byStandardFlow_expectProperResponse
 *      @[Purpose]: Validate service command sending to polling-based client
 *      @[Brief]: Service sends command, client polls and acknowledges, service receives response
 *      @[Status]: TODO - Implement service-to-polling-client integration
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

// TODO: Implement polling timeout test
TEST(UT_ConetCommandWaitAck, verifyServicePollingTimeout_byEmptyQueue_expectTimeoutHandling) {
    // TODO: Implement service polling with no incoming commands
    // TODO: Verify IOC_waitCMD timeout behavior
    // TODO: Test different timeout values and queue states
    GTEST_SKIP() << "TODO: Implement polling timeout validation";
}

// TODO: Implement multi-client polling test with SYNC guarantees
TEST(UT_ConetCommandWaitAck, verifyServiceMultiClientPolling_byIndependentAck_expectProperTracking) {
    // TODO: Implement multi-client command sending with synchronous wait
    // TODO: Implement service polling loop for multiple command sources
    // TODO: Verify independent command tracking and acknowledgment with SYNC guarantees
    // TODO: Validate each client gets proper response through IOC_execCMD completion
    GTEST_SKIP() << "TODO: Implement multi-client polling patterns with SYNC guarantees";
}

// TODO: Implement service-to-polling-client test
TEST(UT_ConetCommandWaitAck, verifyServiceToPollingClient_byStandardFlow_expectProperResponse) {
    // TODO: Implement service setup as CmdInitiator
    // TODO: Implement client setup with polling capability (IOC_waitCMD + IOC_ackCMD)
    // TODO: Implement service command sending via IOC_execCMD
    // TODO: Verify client polls, processes, and acknowledges correctly
    // TODO: Validate service receives proper response through SYNC completion
    GTEST_SKIP() << "TODO: Implement service-to-polling-client integration";
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
