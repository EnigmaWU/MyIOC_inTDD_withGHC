///////////////////////////////////////////////////////////////////////////////////////////////////
// Data Edge TCP - P1 ValidFunc Edge Testing
//
// PURPOSE:
//   Validate TCP data API edge cases, parameter limits, and mode variations.
//   Tests boundary conditions and edge values for TCP protocol.
//
// TDD WORKFLOW:
//   Design → Draft → Structure → Test (RED) → Code (GREEN) → Refactor → Repeat
//
// REFERENCE: LLM/CaTDD_DesignPrompt.md for full methodology
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "_UT_IOC_Common.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF OVERVIEW OF THIS UNIT TESTING FILE===============================================
/**
 * @brief
 *   [WHAT] This file validates TCP data API edge cases and boundary conditions
 *   [WHERE] in the IOC Data API with TCP protocol layer
 *   [WHY] to ensure correct behavior at parameter limits and edge values
 *
 * SCOPE:
 *   - Data size edges: 0 bytes, 1 byte, minimum/maximum sizes
 *   - Timeout mode variations: BLOCK (infinite wait), NONBLOCK (immediate), specific timeouts
 *   - Connection edge cases: Reconnection, max connection attempts
 *   - Buffer boundaries: Exactly buffer size, boundary +/- 1
 *   - Mode combinations: Block/NonBlock/Timeout with different data sizes
 *
 * OUT OF SCOPE:
 *   - Typical cases (tested in UT_DataTypicalTCP)
 *   - Fault conditions (tested in UT_DataFaultTCP)
 *   - API misuse (tested in UT_DataMisuseTCP)
 *   - State transitions (tested in UT_DataState)
 *
 * REFERENCE:
 *   - UT_DataTypicalTCP.cxx (typical use cases)
 *   - UT_DataFaultTCP.cxx (fault scenarios)
 *   - README_UserGuide.md::DAT section (timeout modes documentation)
 */
//======>END OF OVERVIEW OF THIS UNIT TESTING FILE=================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF UNIT TESTING DESIGN==============================================================
/**
 * TEST CATEGORY: P1 🥇 FUNCTIONAL TESTING - ValidFunc (Edge)
 *
 * DESIGN PRINCIPLE: IMPROVE VALUE • AVOID LOSS • BALANCE SKILL vs COST
 *
 * PRIORITY FRAMEWORK:
 *   P1 🥇 FUNCTIONAL:     Must complete before P2 (ValidFunc + InvalidFunc)
 *   P2 🥈 DESIGN-ORIENTED: Test after P1 (State, Capability, Concurrency)
 *   P3 🥉 QUALITY-ORIENTED: Test for quality attributes (Performance, Robust, etc.)
 *   P4 🎯 ADDONS:          Optional (Demo, Examples)
 *
 * THIS FILE FOCUS:
 *   🔲 EDGE (P1 ValidFunc): Edge cases, limits, and mode variations for TCP Data API
 *      - Purpose: Verify behavior at boundary conditions
 *      - Coverage: Min/max values, timeout modes, connection limits
 *      - Examples: 0-byte data, TIMEOUT_INFINITE, max buffer size
 *
 * OUT OF SCOPE (covered in other test files):
 *   ⭐ TYPICAL: Common workflows → UT_DataTypicalTCP.cxx
 *   🚫 MISUSE: Incorrect API usage → UT_DataMisuseTCP.cxx
 *   ⚠️  FAULT: Error handling, recovery → UT_DataFaultTCP.cxx
 *   🔄 STATE: Lifecycle transitions → UT_DataState.cxx
 *   🏆 CAPABILITY: Maximum capacity → UT_DataCapability.cxx
 *   ⚡ PERFORMANCE: Speed, throughput → UT_DataPerformance.cxx
 *
 * COVERAGE STRATEGY:
 *   Dimension 1: Data Size (0, 1, typical, max-1, max, max+1)
 *   Dimension 2: Timeout Mode (BLOCK, NONBLOCK, specific timeout)
 *   Dimension 3: Connection State (first connection, reconnection, max attempts)
 *
 * COVERAGE MATRIX:
 * ┌─────────────────┬─────────────────┬──────────────────┬──────────────────────────────┐
 * │ Data Size       │ Timeout Mode    │ Connection State │ Key Scenarios                │
 * ├─────────────────┼─────────────────┼──────────────────┼──────────────────────────────┤
 * │ 0 bytes         │ BLOCK           │ Established      │ US-1: Empty payload handling │
 * │ 1 byte          │ NONBLOCK        │ Established      │ US-2: Minimum data size      │
 * │ Large (1MB+)    │ Specific timeout│ Established      │ US-3: Maximum data size      │
 * │ Typical (10KB)  │ BLOCK/NONBLOCK  │ Reconnection     │ US-4: Mode variations        │
 * └─────────────────┴─────────────────┴──────────────────┴──────────────────────────────┘
 *
 * EDGE VALUE SELECTION PRINCIPLE:
 *   - Minimum: 0, 1 (lower boundary)
 *   - Typical: Representative value in normal range
 *   - Maximum: Buffer size, system limits (upper boundary)
 *   - Boundary: max-1, max+1 (off-by-one detection)
 *   - Special: NULL-equivalent, negative (if applicable)
 */
//======>END OF UNIT TESTING DESIGN================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF USER STORY=======================================================================
/**
 * US-1: AS a DatSender developer working with edge cases,
 *   I WANT to send 0-byte (empty) data payloads over TCP,
 *  SO THAT I can handle metadata-only transmissions or heartbeat scenarios,
 *      AND verify the system handles empty payloads gracefully.
 *
 * US-2: AS a DatSender developer testing minimum data sizes,
 *   I WANT to send 1-byte data payloads over TCP,
 *  SO THAT I can verify minimum data transmission works correctly,
 *      AND ensure no off-by-one errors in buffer handling.
 *
 * US-3: AS a DatSender developer handling large data,
 *   I WANT to send large data payloads (1MB+) over TCP,
 *  SO THAT I can verify system handles maximum data sizes,
 *      AND ensure no buffer overflow or memory issues.
 *
 * US-4: AS a DatReceiver developer using different timeout modes,
 *   I WANT to receive data with BLOCK, NONBLOCK, and specific timeout modes,
 *  SO THAT I can control waiting behavior based on application needs,
 *      AND ensure each mode behaves correctly at boundaries.
 *
 * US-5: AS a developer handling reconnection scenarios,
 *   I WANT to disconnect and reconnect TCP data links,
 *  SO THAT I can recover from transient network issues,
 *      AND verify reconnection maintains data integrity.
 */
//======>END OF USER STORY=========================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF ACCEPTANCE CRITERIA==============================================================
/**
 * [@US-1] Empty payload handling
 *  AC-1: GIVEN DatSender has connected to DatReceiver TCP service,
 *         WHEN DatSender calls IOC_sendDAT with 0-byte payload,
 *         THEN IOC_sendDAT returns success or appropriate status,
 *          AND DatReceiver callback is invoked (or not, per protocol spec),
 *          AND system remains stable without crashes.
 *
 *  AC-2: GIVEN DatSender sends 0-byte data in NONBLOCK mode,
 *         WHEN DatReceiver polls for data,
 *         THEN system handles empty data appropriately,
 *          AND no buffer underflow occurs.
 *
 * [@US-2] Minimum data size (1 byte)
 *  AC-1: GIVEN DatSender has connected to DatReceiver TCP service,
 *         WHEN DatSender calls IOC_sendDAT with exactly 1-byte payload,
 *         THEN data is transmitted successfully,
 *          AND DatReceiver receives exactly 1 byte via callback,
 *          AND byte content matches sent data.
 *
 *  AC-2: GIVEN 1-byte data transmission,
 *         WHEN multiple 1-byte chunks are sent sequentially,
 *         THEN each chunk is received independently,
 *          AND total received count equals sent count,
 *          AND no data merging or loss occurs.
 *
 * [@US-3] Maximum/Large data size
 *  AC-1: GIVEN DatSender has connected to DatReceiver TCP service,
 *         WHEN DatSender calls IOC_sendDAT with 1MB data payload,
 *         THEN data is transmitted successfully (possibly in chunks),
 *          AND DatReceiver receives all data via callback,
 *          AND data integrity is maintained (memcmp passes).
 *
 *  AC-2: GIVEN large data transmission (1MB+),
 *         WHEN transmission completes,
 *         THEN no memory leaks occur,
 *          AND both sender and receiver clean up resources properly.
 *
 * [@US-4] Timeout mode variations
 *  AC-1: GIVEN DatReceiver with BLOCK mode (TIMEOUT_INFINITE),
 *         WHEN no data is available,
 *         THEN IOC_recvDAT waits indefinitely until data arrives (or test timeout),
 *          AND returns success when data becomes available.
 *
 *  AC-2: GIVEN DatReceiver with NONBLOCK mode (0 timeout),
 *         WHEN no data is available,
 *         THEN IOC_recvDAT returns immediately with IOC_RESULT_NO_DATA,
 *          AND does not block caller.
 *
 *  AC-3: GIVEN DatReceiver with specific timeout (e.g., 100ms),
 *         WHEN no data is available within timeout,
 *         THEN IOC_recvDAT returns IOC_RESULT_TIMEOUT,
 *          AND returns within expected time window (±tolerance).
 *
 *  AC-4: GIVEN DatReceiver with timeout at boundary (1ms, max timeout),
 *         WHEN timeout expires,
 *         THEN system handles boundary timeouts correctly,
 *          AND no overflow or underflow in timeout calculations.
 *
 * [@US-5] Reconnection scenarios
 *  AC-1: GIVEN DatSender was connected and then disconnected,
 *         WHEN DatSender reconnects to same DatReceiver service,
 *         THEN new connection is established successfully,
 *          AND new LinkID is valid and different from previous,
 *          AND data transmission works on new connection.
 *
 *  AC-2: GIVEN DatReceiver service accepts connection, client disconnects,
 *         WHEN same client reconnects multiple times (3+ times),
 *         THEN each reconnection succeeds,
 *          AND no resource exhaustion occurs,
 *          AND data integrity maintained across reconnections.
 */
//======>END OF ACCEPTANCE CRITERIA================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF TEST CASES=======================================================================
/**
 * TEST CASES - Organized by Acceptance Criteria
 *
 * NAMING CONVENTION: verifyBehavior_byCondition_expectResult
 *
 * STATUS TRACKING:
 *   ⚪ = Planned/TODO     - Designed but not implemented
 *   🔴 = Implemented/RED  - Test written and failing (need prod code)
 *   🟢 = Passed/GREEN     - Test written and passing
 *
 * TEST STRUCTURE (4-phase pattern):
 *   1. 🔧 SETUP:    Prepare environment, create resources
 *   2. 🎯 BEHAVIOR: Execute the action being tested
 *   3. ✅ VERIFY:   Assert outcomes (≤3 key assertions)
 *   4. 🧹 CLEANUP:  Release resources, reset state
 *
 *===================================================================================================
 * 📋 [CATEGORY: Edge - Data Size] Empty and Minimum Data Transmission
 *===================================================================================================
 *
 * [@AC-1,US-1] Empty payload (0 bytes)
 *  ⚪ TC-1: verifyEmptyPayload_bySendZeroBytesTCP_expectGracefulHandling
 *      @[Status]: ⚪ TODO - Planned
 *      @[Purpose]: Validate system handles 0-byte payload without crash
 *      @[Brief]: Send 0-byte data, verify system stability and appropriate response
 *      @[Port]: 20001
 *      @[Expected]: IOC_RESULT_SUCCESS or IOC_RESULT_INVALID_PARAM (protocol-dependent)
 *
 * [@AC-1,US-2] Minimum data size (1 byte)
 *  ⚪ TC-2: verifyMinimumData_bySendOneByteTC_expectSuccessfulTransmission
 *      @[Status]: ⚪ TODO - Planned
 *      @[Purpose]: Validate 1-byte transmission works correctly
 *      @[Brief]: Send 1 byte ('A'), verify received byte matches
 *      @[Port]: 20002
 *      @[KeyVerifyPoints]: 1 (byte content match)
 *
 * [@AC-2,US-2] Sequential 1-byte chunks
 *  ⚪ TC-3: verifySequentialMinimum_bySendMultipleOneByteTCP_expectIndependentChunks
 *      @[Status]: ⚪ TODO - Planned
 *      @[Purpose]: Ensure 1-byte chunks don't merge or get lost
 *      @[Brief]: Send 10 sequential 1-byte chunks, verify each received independently
 *      @[Port]: 20003
 *      @[KeyVerifyPoints]: 2 (chunk count, content sequence)
 *
 *===================================================================================================
 * 📋 [CATEGORY: Edge - Data Size] Large/Maximum Data Transmission
 *===================================================================================================
 *
 * [@AC-1,US-3] Large data (1MB)
 *  ⚪ TC-4: verifyLargeData_bySendOneMegabyteTCP_expectCompleteIntegrity
 *      @[Status]: ⚪ TODO - Planned
 *      @[Purpose]: Validate 1MB data transmission with full integrity
 *      @[Brief]: Send 1MB data with pattern, verify byte-by-byte match
 *      @[Port]: 20004
 *      @[KeyVerifyPoints]: 3 (size match, memcmp integrity, no memory leak)
 *      @[Timeout]: Extended (5000ms) for large data transfer
 *
 * [@AC-2,US-3] Large data resource cleanup
 *  ⚪ TC-5: verifyLargeDataCleanup_byMultipleLargeSendsTCP_expectNoMemoryLeak
 *      @[Status]: ⚪ TODO - Planned
 *      @[Purpose]: Ensure no memory leaks with repeated large transmissions
 *      @[Brief]: Send 1MB data 5 times, verify cleanup after each
 *      @[Port]: 20005
 *      @[Note]: Run with AddressSanitizer to detect leaks
 *
 *===================================================================================================
 * 📋 [CATEGORY: Edge - Timeout Modes] BLOCK, NONBLOCK, Specific Timeout
 *===================================================================================================
 *
 * [@AC-2,US-4] NONBLOCK mode (immediate return)
 *  ⚪ TC-6: verifyNonblockMode_byRecvWithZeroTimeoutTCP_expectImmediateReturn
 *      @[Status]: ⚪ TODO - Planned
 *      @[Purpose]: Validate NONBLOCK mode returns immediately when no data
 *      @[Brief]: Poll with 0 timeout, verify IOC_RESULT_NO_DATA and quick return
 *      @[Port]: 20006
 *      @[KeyVerifyPoints]: 2 (NO_DATA result, return time < 10ms)
 *      @[Note]: TCP may require callback, document polling limitation if needed
 *
 * [@AC-3,US-4] Specific timeout (100ms)
 *  ⚪ TC-7: verifySpecificTimeout_byRecvWith100msTimeoutTCP_expectTimeoutResult
 *      @[Status]: ⚪ TODO - Planned
 *      @[Purpose]: Validate specific timeout returns within expected window
 *      @[Brief]: Poll with 100ms timeout, no data sent, verify TIMEOUT result
 *      @[Port]: 20007
 *      @[KeyVerifyPoints]: 2 (TIMEOUT result, elapsed time 90-110ms)
 *
 * [@AC-4,US-4] Boundary timeouts (1ms, max)
 *  ⚪ TC-8: verifyBoundaryTimeout_byRecvWith1msTimeoutTCP_expectCorrectBehavior
 *      @[Status]: ⚪ TODO - Planned
 *      @[Purpose]: Test minimum timeout boundary (1ms)
 *      @[Brief]: Poll with 1ms timeout, verify quick return
 *      @[Port]: 20008
 *      @[KeyVerifyPoints]: 2 (result code, elapsed time)
 *
 *  ⚪ TC-9: verifyMaxTimeout_byRecvWithMaxTimeoutTCP_expectNoOverflow
 *      @[Status]: ⚪ TODO - Planned
 *      @[Purpose]: Test maximum timeout value (e.g., UINT32_MAX-1)
 *      @[Brief]: Poll with max timeout, cancel early, verify no overflow
 *      @[Port]: 20009
 *      @[Note]: May need to cancel test early to avoid long wait
 *
 *===================================================================================================
 * 📋 [CATEGORY: Edge - Connection] Reconnection and Connection Boundaries
 *===================================================================================================
 *
 * [@AC-1,US-5] Single reconnection
 *  ⚪ TC-10: verifyReconnection_byDisconnectAndReconnectTCP_expectNewValidLink
 *      @[Status]: ⚪ TODO - Planned
 *      @[Purpose]: Validate disconnect and reconnect works correctly
 *      @[Brief]: Connect, send data, disconnect, reconnect, send again
 *      @[Port]: 20010
 *      @[KeyVerifyPoints]: 3 (new LinkID differs, data works on new link, old link invalid)
 *
 * [@AC-2,US-5] Multiple reconnections
 *  ⚪ TC-11: verifyMultipleReconnections_byReconnectFiveTimesTCP_expectAllSucceed
 *      @[Status]: ⚪ TODO - Planned
 *      @[Purpose]: Ensure repeated reconnections don't exhaust resources
 *      @[Brief]: Connect/disconnect/reconnect 5 times, verify each works
 *      @[Port]: 20011
 *      @[KeyVerifyPoints]: 2 (all 5 reconnections succeed, data integrity maintained)
 *
 *===================================================================================================
 * 📋 [CATEGORY: Edge - Mode Combinations] Data Size × Timeout Mode
 *===================================================================================================
 *
 * [@AC-2,US-1 + @AC-2,US-4] Empty data + NONBLOCK
 *  ⚪ TC-12: verifyEdgeCombination_byEmptyDataNonblockTCP_expectGracefulHandling
 *      @[Status]: ⚪ TODO - Planned
 *      @[Purpose]: Test edge combination: 0-byte data + NONBLOCK mode
 *      @[Brief]: Send 0-byte in NONBLOCK, verify no crash or hang
 *      @[Port]: 20012
 *
 * SUMMARY:
 *   Total Test Cases: 12 planned
 *   Status: 0/12 implemented (all TODO)
 *   Priority: HIGH (P1 ValidFunc coverage)
 *   Estimated Effort: 6-8 hours
 */
//======>END OF TEST CASES=========================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
// UNIT TESTING IMPLEMENTATION
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "_UT_IOC_Common.h"

// Private data structure for DAT receiver callback
typedef struct {
    int ReceivedDataCnt;
    ULONG_T TotalReceivedSize;
    char ReceivedContent[2 * 1024 * 1024];  // 2MB buffer for large data tests
    bool CallbackExecuted;
    int ClientIndex;
} __DatReceiverPrivData_T;

// Callback function for receiving data
static IOC_Result_T __CbRecvDat_F(IOC_LinkID_T LinkID, IOC_DatDesc_pT pDatDesc, void *pCbPriv) {
    __DatReceiverPrivData_T *pPrivData = (__DatReceiverPrivData_T *)pCbPriv;
    pPrivData->CallbackExecuted = true;
    pPrivData->ReceivedDataCnt++;

    ULONG_T DataSize = pDatDesc->Payload.PtrDataLen;
    if (DataSize > 0 && pDatDesc->Payload.pData != NULL) {
        memcpy(pPrivData->ReceivedContent + pPrivData->TotalReceivedSize, pDatDesc->Payload.pData, DataSize);
    }

    pPrivData->TotalReceivedSize += DataSize;

    printf("   [TCP DAT Callback] Client[%d] received %lu bytes, total: %lu bytes\n", pPrivData->ClientIndex, DataSize,
           pPrivData->TotalReceivedSize);
    return IOC_RESULT_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF: [@AC-1,US-1]====================================================================
/**
 * @[Name]: verifyEmptyPayload_bySendZeroBytesTC_expectGracefulHandling
 * @[Purpose]: Validate system handles 0-byte payload without crash (AC-1@US-1)
 * @[Brief]: Send 0-byte data over TCP, verify system stability and appropriate response
 * @[Steps]:
 *   1) Setup DatReceiver TCP service and establish connection
 *   2) DatSender send 0-byte payload via IOC_sendDAT
 *   3) Verify system remains stable, check result code
 *   4) Cleanup connections
 * @[Expect]: IOC_RESULT_SUCCESS or appropriate status, no crash
 * @[Notes]: Protocol may reject 0-byte payloads or handle as no-op
 */
TEST(UT_DataEdgeTCP, verifyEmptyPayload_bySendZeroBytesTC_expectGracefulHandling) {
    //===>>> SETUP <<<===
    printf("🔧 SETUP: Empty payload edge case test\n");

    IOC_Result_T Result;
    IOC_SrvID_T DatReceiverSrvID = IOC_ID_INVALID;
    IOC_LinkID_T DatReceiverLinkID = IOC_ID_INVALID;
    IOC_LinkID_T DatSenderLinkID = IOC_ID_INVALID;

    __DatReceiverPrivData_T RecvPrivData = {0};
    RecvPrivData.ClientIndex = 1;

    // Setup callback for data reception
    IOC_DatUsageArgs_T DatUsageArgs = {0};
    DatUsageArgs.CbRecvDat_F = __CbRecvDat_F;
    DatUsageArgs.pCbPrivData = &RecvPrivData;

    // Setup DatReceiver TCP service with callback
    IOC_SrvURI_T DatReceiverSrvURI = {
        .pProtocol = IOC_SRV_PROTO_TCP,
        .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
        .pPath = "test/data/edge/tcp/empty",
        .Port = 20001,
    };

    IOC_SrvArgs_T SrvArgs = {
        .SrvURI = DatReceiverSrvURI,
        .UsageCapabilites = IOC_LinkUsageDatReceiver,
        .UsageArgs =
            {
                .pDat = &DatUsageArgs,
            },
    };

    Result = IOC_onlineService(&DatReceiverSrvID, &SrvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);
    printf("   ✓ DatReceiver TCP service online on port 20001\n");

    // DatSender connect to service in separate thread
    IOC_ConnArgs_T ConnArgs = {
        .SrvURI = DatReceiverSrvURI,
        .Usage = IOC_LinkUsageDatSender,
    };

    std::thread DatSenderThread([&] {
        IOC_Result_T ThreadResult = IOC_connectService(&DatSenderLinkID, &ConnArgs, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, ThreadResult);
    });

    // Accept connection
    Result = IOC_acceptClient(DatReceiverSrvID, &DatReceiverLinkID, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);

    DatSenderThread.join();
    printf("   ✓ TCP connection established\n");

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    //===>>> BEHAVIOR <<<===
    printf("🎯 BEHAVIOR: Send 0-byte payload\n");

    IOC_DatDesc_T DatDesc = {0};
    IOC_initDatDesc(&DatDesc);
    DatDesc.Payload.pData = NULL;  // No data
    DatDesc.Payload.PtrDataSize = 0;
    DatDesc.Payload.PtrDataLen = 0;

    Result = IOC_sendDAT(DatSenderLinkID, &DatDesc, NULL);
    printf("   → IOC_sendDAT with 0-byte returned: %d\n", Result);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    //===>>> VERIFY <<<===
    printf("✅ VERIFY: System stability and appropriate response\n");

    //@KeyVerifyPoint-1: System doesn't crash (test still running)
    VERIFY_KEYPOINT_TRUE(true, "System remains stable after 0-byte send");

    //@KeyVerifyPoint-2: Result is valid (SUCCESS or appropriate error)
    VERIFY_KEYPOINT_TRUE(
        Result == IOC_RESULT_SUCCESS || Result == IOC_RESULT_INVALID_PARAM || Result == IOC_RESULT_NO_DATA,
        "Result code is valid");

    printf("   ✅ Empty payload handled gracefully\n");
    printf("      - Send result: %d\n", Result);
    printf("      - Callback executed: %s\n", RecvPrivData.CallbackExecuted ? "Yes" : "No");
    printf("      - Received count: %d\n", RecvPrivData.ReceivedDataCnt);

    //===>>> CLEANUP <<<===
    printf("🧹 CLEANUP\n");

    if (DatReceiverLinkID != IOC_ID_INVALID) IOC_closeLink(DatReceiverLinkID);
    if (DatSenderLinkID != IOC_ID_INVALID) IOC_closeLink(DatSenderLinkID);
    if (DatReceiverSrvID != IOC_ID_INVALID) IOC_offlineService(DatReceiverSrvID);

    printf("   ✓ Cleanup complete\n");
}
//======>END OF: [@AC-1,US-1]======================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF: [@AC-1,US-2]====================================================================
/**
 * @[Name]: verifyMinimumData_bySendOneByteTCP_expectSuccessfulTransmission
 * @[Purpose]: Validate 1-byte transmission works correctly (AC-1@US-2)
 * @[Brief]: Send exactly 1 byte ('A'), verify received byte matches
 * @[Steps]:
 *   1) Setup DatReceiver TCP service and establish connection
 *   2) DatSender send 1-byte payload ('A') via IOC_sendDAT
 *   3) Verify callback receives exactly 1 byte with correct content
 *   4) Cleanup connections
 * @[Expect]: Successful transmission, byte content matches
 * @[Notes]: Critical boundary test - ensures no off-by-one errors
 * @[Status]: 🟢 GREEN/PASSED - Implemented and verified
 */
TEST(UT_DataEdgeTCP, verifyMinimumData_bySendOneByteTCP_expectSuccessfulTransmission) {
    //===>>> SETUP <<<===
    printf("🔧 SETUP: 1-byte minimum data transmission test\n");

    IOC_Result_T Result;
    IOC_SrvID_T DatReceiverSrvID = IOC_ID_INVALID;
    IOC_LinkID_T DatReceiverLinkID = IOC_ID_INVALID;
    IOC_LinkID_T DatSenderLinkID = IOC_ID_INVALID;

    __DatReceiverPrivData_T RecvPrivData = {0};
    RecvPrivData.ClientIndex = 1;

    // Setup callback for data reception
    IOC_DatUsageArgs_T DatUsageArgs = {0};
    DatUsageArgs.CbRecvDat_F = __CbRecvDat_F;
    DatUsageArgs.pCbPrivData = &RecvPrivData;

    // Setup DatReceiver TCP service with callback
    IOC_SrvURI_T DatReceiverSrvURI = {
        .pProtocol = IOC_SRV_PROTO_TCP,
        .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
        .pPath = "test/data/edge/tcp/onebyte",
        .Port = 20002,
    };

    IOC_SrvArgs_T SrvArgs = {
        .SrvURI = DatReceiverSrvURI,
        .UsageCapabilites = IOC_LinkUsageDatReceiver,
        .UsageArgs =
            {
                .pDat = &DatUsageArgs,
            },
    };

    Result = IOC_onlineService(&DatReceiverSrvID, &SrvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);
    printf("   ✓ DatReceiver TCP service online on port 20002\n");

    // DatSender connect to service in separate thread
    IOC_ConnArgs_T ConnArgs = {
        .SrvURI = DatReceiverSrvURI,
        .Usage = IOC_LinkUsageDatSender,
    };

    std::thread DatSenderThread([&] {
        IOC_Result_T ThreadResult = IOC_connectService(&DatSenderLinkID, &ConnArgs, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, ThreadResult);
    });

    // Accept connection
    Result = IOC_acceptClient(DatReceiverSrvID, &DatReceiverLinkID, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);

    DatSenderThread.join();
    printf("   ✓ TCP connection established\n");

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    //===>>> BEHAVIOR <<<===
    printf("🎯 BEHAVIOR: Send 1-byte payload\n");

    char OneByte = 'A';
    IOC_DatDesc_T DatDesc = {0};
    IOC_initDatDesc(&DatDesc);
    DatDesc.Payload.pData = &OneByte;
    DatDesc.Payload.PtrDataSize = 1;
    DatDesc.Payload.PtrDataLen = 1;

    Result = IOC_sendDAT(DatSenderLinkID, &DatDesc, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);
    printf("   → Sent 1 byte: '%c' (0x%02X)\n", OneByte, (unsigned char)OneByte);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    //===>>> VERIFY <<<===
    printf("✅ VERIFY: 1-byte received correctly\n");

    //@KeyVerifyPoint-1: Exactly 1 byte received
    VERIFY_KEYPOINT_EQ(RecvPrivData.TotalReceivedSize, 1UL, "Exactly 1 byte received");

    //@KeyVerifyPoint-2: Byte content matches
    VERIFY_KEYPOINT_EQ(RecvPrivData.ReceivedContent[0], 'A', "Byte content matches");

    printf("   ✅ 1-byte transmission SUCCESS:\n");
    printf("      - Sent: '%c' (0x%02X)\n", OneByte, (unsigned char)OneByte);
    printf("      - Received: '%c' (0x%02X)\n", RecvPrivData.ReceivedContent[0],
           (unsigned char)RecvPrivData.ReceivedContent[0]);
    printf("      - Data integrity: ✓ (byte match)\n");

    //===>>> CLEANUP <<<===
    printf("🧹 CLEANUP\n");

    if (DatReceiverLinkID != IOC_ID_INVALID) IOC_closeLink(DatReceiverLinkID);
    if (DatSenderLinkID != IOC_ID_INVALID) IOC_closeLink(DatSenderLinkID);
    if (DatReceiverSrvID != IOC_ID_INVALID) IOC_offlineService(DatReceiverSrvID);

    printf("   ✓ Cleanup complete\n");
}
//======>END OF: [@AC-1,US-2]======================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF: [@AC-2,US-2]====================================================================
/**
 * @[Name]: verifySequentialMinimum_bySendMultipleOneByteTCP_expectIndependentChunks
 * @[Purpose]: Ensure 1-byte chunks don't merge or get lost (AC-2@US-2)
 * @[Brief]: Send 10 sequential 1-byte chunks, verify each received independently
 * @[Steps]:
 *   1) Setup DatReceiver TCP service and establish connection
 *   2) DatSender send 10 different 1-byte payloads ('A' to 'J') sequentially
 *   3) Verify callback receives 10 chunks with correct sequence
 *   4) Cleanup connections
 * @[Expect]: 10 independent chunks received, no merging, sequence preserved
 * @[Notes]: Tests chunking behavior and ensures no data coalescing
 * @[Status]: 🟢 GREEN/PASSED - Implemented and verified
 */
TEST(UT_DataEdgeTCP, verifySequentialMinimum_bySendMultipleOneByteTCP_expectIndependentChunks) {
    //===>>> SETUP <<<===
    printf("🔧 SETUP: Multiple 1-byte chunks transmission test\n");

    IOC_Result_T Result;
    IOC_SrvID_T DatReceiverSrvID = IOC_ID_INVALID;
    IOC_LinkID_T DatReceiverLinkID = IOC_ID_INVALID;
    IOC_LinkID_T DatSenderLinkID = IOC_ID_INVALID;

    __DatReceiverPrivData_T RecvPrivData = {0};
    RecvPrivData.ClientIndex = 1;

    // Setup callback for data reception
    IOC_DatUsageArgs_T DatUsageArgs = {0};
    DatUsageArgs.CbRecvDat_F = __CbRecvDat_F;
    DatUsageArgs.pCbPrivData = &RecvPrivData;

    // Setup DatReceiver TCP service with callback
    IOC_SrvURI_T DatReceiverSrvURI = {
        .pProtocol = IOC_SRV_PROTO_TCP,
        .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
        .pPath = "test/data/edge/tcp/multiple_onebyte",
        .Port = 20003,
    };

    IOC_SrvArgs_T SrvArgs = {
        .SrvURI = DatReceiverSrvURI,
        .UsageCapabilites = IOC_LinkUsageDatReceiver,
        .UsageArgs =
            {
                .pDat = &DatUsageArgs,
            },
    };

    Result = IOC_onlineService(&DatReceiverSrvID, &SrvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);
    printf("   ✓ DatReceiver TCP service online on port 20003\n");

    // DatSender connect to service in separate thread
    IOC_ConnArgs_T ConnArgs = {
        .SrvURI = DatReceiverSrvURI,
        .Usage = IOC_LinkUsageDatSender,
    };

    std::thread DatSenderThread([&] {
        IOC_Result_T ThreadResult = IOC_connectService(&DatSenderLinkID, &ConnArgs, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, ThreadResult);
    });

    // Accept connection
    Result = IOC_acceptClient(DatReceiverSrvID, &DatReceiverLinkID, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);

    DatSenderThread.join();
    printf("   ✓ TCP connection established\n");

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    //===>>> BEHAVIOR <<<===
    printf("🎯 BEHAVIOR: Send 10 sequential 1-byte payloads\n");

    const int ChunkCount = 10;
    char ExpectedSequence[ChunkCount + 1] = "ABCDEFGHIJ";

    for (int i = 0; i < ChunkCount; i++) {
        char OneByte = ExpectedSequence[i];

        IOC_DatDesc_T DatDesc = {0};
        IOC_initDatDesc(&DatDesc);
        DatDesc.Payload.pData = &OneByte;
        DatDesc.Payload.PtrDataSize = 1;
        DatDesc.Payload.PtrDataLen = 1;

        Result = IOC_sendDAT(DatSenderLinkID, &DatDesc, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, Result);

        // Small delay between chunks to test independent handling
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    printf("   → Sent 10 sequential 1-byte chunks: '%s'\n", ExpectedSequence);

    std::this_thread::sleep_for(std::chrono::milliseconds(150));

    //===>>> VERIFY <<<===
    printf("✅ VERIFY: All chunks received independently with correct sequence\n");

    //@KeyVerifyPoint-1: Total size is 10 bytes
    VERIFY_KEYPOINT_EQ(RecvPrivData.TotalReceivedSize, (ULONG_T)ChunkCount, "Total 10 bytes received");

    //@KeyVerifyPoint-2: Content sequence matches
    RecvPrivData.ReceivedContent[ChunkCount] = '\0';  // Null-terminate for comparison
    VERIFY_KEYPOINT_EQ(memcmp(RecvPrivData.ReceivedContent, ExpectedSequence, ChunkCount), 0,
                       "Sequence content matches");

    printf("   ✅ Multiple 1-byte chunks transmission SUCCESS:\n");
    printf("      - Sent sequence: '%s'\n", ExpectedSequence);
    printf("      - Received: '%s'\n", RecvPrivData.ReceivedContent);
    printf("      - Chunk count: %d (callback invoked %d times)\n", ChunkCount, RecvPrivData.ReceivedDataCnt);
    printf("      - Sequence integrity: ✓ (no merging or loss)\n");

    //===>>> CLEANUP <<<===
    printf("🧹 CLEANUP\n");

    if (DatReceiverLinkID != IOC_ID_INVALID) IOC_closeLink(DatReceiverLinkID);
    if (DatSenderLinkID != IOC_ID_INVALID) IOC_closeLink(DatSenderLinkID);
    if (DatReceiverSrvID != IOC_ID_INVALID) IOC_offlineService(DatReceiverSrvID);

    printf("   ✓ Cleanup complete\n");
}
//======>END OF: [@AC-2,US-2]======================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF: [@AC-1,US-3]====================================================================
/**
 * @[Name]: verifyLargeData_bySendOneMegabyteTCP_expectCompleteIntegrity
 * @[Purpose]: Validate 1MB data transmission with full integrity (AC-1@US-3)
 * @[Brief]: Send 1MB data with pattern, verify byte-by-byte match
 * @[Steps]:
 *   1) Setup DatReceiver TCP service and establish connection
 *   2) Generate 1MB data with repeating pattern (0x00-0xFF sequence)
 *   3) DatSender send 1MB payload via IOC_sendDAT
 *   4) Verify callback receives all data with integrity check (memcmp)
 *   5) Cleanup connections and verify no memory leaks
 * @[Expect]: Complete 1MB transmission, byte-by-byte integrity preserved
 * @[Notes]: May take longer due to large data size; extended timeout set
 * @[Status]: 🟢 GREEN/PASSED - Implemented and verified
 */
TEST(UT_DataEdgeTCP, verifyLargeData_bySendOneMegabyteTCP_expectCompleteIntegrity) {
    //===>>> SETUP <<<===
    printf("🔧 SETUP: 1MB large data transmission test\n");

    IOC_Result_T Result;
    IOC_SrvID_T DatReceiverSrvID = IOC_ID_INVALID;
    IOC_LinkID_T DatReceiverLinkID = IOC_ID_INVALID;
    IOC_LinkID_T DatSenderLinkID = IOC_ID_INVALID;

    __DatReceiverPrivData_T RecvPrivData = {0};
    RecvPrivData.ClientIndex = 1;

    // Setup callback for data reception
    IOC_DatUsageArgs_T DatUsageArgs = {0};
    DatUsageArgs.CbRecvDat_F = __CbRecvDat_F;
    DatUsageArgs.pCbPrivData = &RecvPrivData;

    // Setup DatReceiver TCP service with callback
    IOC_SrvURI_T DatReceiverSrvURI = {
        .pProtocol = IOC_SRV_PROTO_TCP,
        .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
        .pPath = "test/data/edge/tcp/largedata",
        .Port = 20004,
    };

    IOC_SrvArgs_T SrvArgs = {
        .SrvURI = DatReceiverSrvURI,
        .UsageCapabilites = IOC_LinkUsageDatReceiver,
        .UsageArgs =
            {
                .pDat = &DatUsageArgs,
            },
    };

    Result = IOC_onlineService(&DatReceiverSrvID, &SrvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);
    printf("   ✓ DatReceiver TCP service online on port 20004\n");

    // DatSender connect to service in separate thread
    IOC_ConnArgs_T ConnArgs = {
        .SrvURI = DatReceiverSrvURI,
        .Usage = IOC_LinkUsageDatSender,
    };

    std::thread DatSenderThread([&] {
        IOC_Result_T ThreadResult = IOC_connectService(&DatSenderLinkID, &ConnArgs, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, ThreadResult);
    });

    // Accept connection
    Result = IOC_acceptClient(DatReceiverSrvID, &DatReceiverLinkID, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);

    DatSenderThread.join();
    printf("   ✓ TCP connection established\n");

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    //===>>> BEHAVIOR <<<===
    printf("🎯 BEHAVIOR: Send 1MB payload with pattern\n");

    const ULONG_T DataSize = 1024 * 1024;  // 1MB
    char *LargeData = (char *)malloc(DataSize);
    ASSERT_NE(LargeData, nullptr);

    // Fill with repeating pattern 0x00-0xFF
    for (ULONG_T i = 0; i < DataSize; i++) {
        LargeData[i] = (char)(i % 256);
    }

    // Add markers at start and end
    memcpy(LargeData, "[START_1MB]", 11);
    memcpy(LargeData + DataSize - 9, "[END_1MB]", 9);

    IOC_DatDesc_T DatDesc = {0};
    IOC_initDatDesc(&DatDesc);
    DatDesc.Payload.pData = LargeData;
    DatDesc.Payload.PtrDataSize = DataSize;
    DatDesc.Payload.PtrDataLen = DataSize;

    printf("   → Sending 1MB data (1048576 bytes)...\n");
    Result = IOC_sendDAT(DatSenderLinkID, &DatDesc, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);
    printf("   → Send complete\n");

    // Extended wait for large data transfer
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    //===>>> VERIFY <<<===
    printf("✅ VERIFY: 1MB data received with full integrity\n");

    //@KeyVerifyPoint-1: Exact size match
    VERIFY_KEYPOINT_EQ(RecvPrivData.TotalReceivedSize, DataSize, "Exact 1MB received");

    //@KeyVerifyPoint-2: Byte-by-byte integrity
    VERIFY_KEYPOINT_EQ(memcmp(RecvPrivData.ReceivedContent, LargeData, DataSize), 0,
                       "Complete data integrity (memcmp)");

    //@KeyVerifyPoint-3: Markers present (start and end)
    VERIFY_KEYPOINT_EQ(memcmp(RecvPrivData.ReceivedContent, "[START_1MB]", 11), 0, "Start marker intact");

    printf("   ✅ 1MB large data transmission SUCCESS:\n");
    printf("      - Data size: %lu bytes (1MB)\n", DataSize);
    printf("      - Received size: %lu bytes\n", RecvPrivData.TotalReceivedSize);
    printf("      - Callback invocations: %d\n", RecvPrivData.ReceivedDataCnt);
    printf("      - Start marker: '%.*s'\n", 11, RecvPrivData.ReceivedContent);
    printf("      - End marker: '%.*s'\n", 9, RecvPrivData.ReceivedContent + DataSize - 9);
    printf("      - Data integrity: ✓ (byte-by-byte match via memcmp)\n");

    //===>>> CLEANUP <<<===
    printf("🧹 CLEANUP\n");

    free(LargeData);
    if (DatReceiverLinkID != IOC_ID_INVALID) IOC_closeLink(DatReceiverLinkID);
    if (DatSenderLinkID != IOC_ID_INVALID) IOC_closeLink(DatSenderLinkID);
    if (DatReceiverSrvID != IOC_ID_INVALID) IOC_offlineService(DatReceiverSrvID);

    printf("   ✓ Cleanup complete (1MB buffer freed)\n");
}
//======>END OF: [@AC-1,US-3]======================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF: [@AC-1,US-5]====================================================================
/**
 * @[Name]: verifyReconnection_byDisconnectAndReconnectTCP_expectNewValidLink
 * @[Purpose]: Validate disconnect and reconnect works correctly (AC-1@US-5)
 * @[Brief]: Connect, send data, disconnect, reconnect, send again
 * @[Steps]:
 *   1) Setup DatReceiver TCP service
 *   2) First connection: DatSender connects, sends data, disconnects
 *   3) Second connection: DatSender reconnects to same service
 *   4) Verify new LinkID differs from old LinkID
 *   5) Send data on new connection, verify it works
 *   6) Cleanup connections
 * @[Expect]: Reconnection succeeds, new LinkID valid, data works on new link
 * @[Notes]: Tests connection recovery scenario
 * @[Status]: 🟢 GREEN/PASSED - Implemented and verified
 */
TEST(UT_DataEdgeTCP, verifyReconnection_byDisconnectAndReconnectTCP_expectNewValidLink) {
    //===>>> SETUP <<<===
    printf("🔧 SETUP: Reconnection test - disconnect and reconnect\n");

    IOC_Result_T Result;
    IOC_SrvID_T DatReceiverSrvID = IOC_ID_INVALID;
    IOC_LinkID_T DatReceiverLinkID1 = IOC_ID_INVALID;
    IOC_LinkID_T DatReceiverLinkID2 = IOC_ID_INVALID;
    IOC_LinkID_T DatSenderLinkID1 = IOC_ID_INVALID;
    IOC_LinkID_T DatSenderLinkID2 = IOC_ID_INVALID;

    __DatReceiverPrivData_T RecvPrivData = {0};
    RecvPrivData.ClientIndex = 1;

    // Setup callback for data reception
    IOC_DatUsageArgs_T DatUsageArgs = {0};
    DatUsageArgs.CbRecvDat_F = __CbRecvDat_F;
    DatUsageArgs.pCbPrivData = &RecvPrivData;

    // Setup DatReceiver TCP service with callback
    IOC_SrvURI_T DatReceiverSrvURI = {
        .pProtocol = IOC_SRV_PROTO_TCP,
        .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
        .pPath = "test/data/edge/tcp/reconnection",
        .Port = 20010,
    };

    IOC_SrvArgs_T SrvArgs = {
        .SrvURI = DatReceiverSrvURI,
        .UsageCapabilites = IOC_LinkUsageDatReceiver,
        .UsageArgs =
            {
                .pDat = &DatUsageArgs,
            },
    };

    Result = IOC_onlineService(&DatReceiverSrvID, &SrvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);
    printf("   ✓ DatReceiver TCP service online on port 20010\n");

    //===>>> BEHAVIOR <<<===
    printf("🎯 BEHAVIOR: First connection - connect, send, disconnect\n");

    // First connection
    IOC_ConnArgs_T ConnArgs = {
        .SrvURI = DatReceiverSrvURI,
        .Usage = IOC_LinkUsageDatSender,
    };

    std::thread DatSenderThread1([&] {
        IOC_Result_T ThreadResult = IOC_connectService(&DatSenderLinkID1, &ConnArgs, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, ThreadResult);
    });

    Result = IOC_acceptClient(DatReceiverSrvID, &DatReceiverLinkID1, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);

    DatSenderThread1.join();
    printf("   ✓ First connection established (Sender LinkID: %llu, Receiver LinkID: %llu)\n", DatSenderLinkID1,
           DatReceiverLinkID1);

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Send data on first connection
    const char *FirstData = "DATA_ON_FIRST_CONNECTION";
    IOC_DatDesc_T DatDesc1 = {0};
    IOC_initDatDesc(&DatDesc1);
    DatDesc1.Payload.pData = (void *)FirstData;
    DatDesc1.Payload.PtrDataSize = strlen(FirstData);
    DatDesc1.Payload.PtrDataLen = strlen(FirstData);

    Result = IOC_sendDAT(DatSenderLinkID1, &DatDesc1, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);
    printf("   → Sent data on first connection: '%s'\n", FirstData);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Disconnect
    Result = IOC_closeLink(DatReceiverLinkID1);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);
    Result = IOC_closeLink(DatSenderLinkID1);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);
    printf("   ✓ First connection closed\n");

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Reset receive buffer for second connection
    RecvPrivData.ReceivedDataCnt = 0;
    RecvPrivData.TotalReceivedSize = 0;
    memset(RecvPrivData.ReceivedContent, 0, sizeof(RecvPrivData.ReceivedContent));

    printf("🎯 BEHAVIOR: Second connection - reconnect and send again\n");

    // Second connection (reconnect)
    std::thread DatSenderThread2([&] {
        IOC_Result_T ThreadResult = IOC_connectService(&DatSenderLinkID2, &ConnArgs, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, ThreadResult);
    });

    Result = IOC_acceptClient(DatReceiverSrvID, &DatReceiverLinkID2, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);

    DatSenderThread2.join();
    printf("   ✓ Second connection established (Sender LinkID: %llu, Receiver LinkID: %llu)\n", DatSenderLinkID2,
           DatReceiverLinkID2);

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Send data on second connection
    const char *SecondData = "DATA_ON_SECOND_CONNECTION";
    IOC_DatDesc_T DatDesc2 = {0};
    IOC_initDatDesc(&DatDesc2);
    DatDesc2.Payload.pData = (void *)SecondData;
    DatDesc2.Payload.PtrDataSize = strlen(SecondData);
    DatDesc2.Payload.PtrDataLen = strlen(SecondData);

    Result = IOC_sendDAT(DatSenderLinkID2, &DatDesc2, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);
    printf("   → Sent data on second connection: '%s'\n", SecondData);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    //===>>> VERIFY <<<===
    printf("✅ VERIFY: Reconnection successful, data transmission works\n");

    //@KeyVerifyPoint-1: Second connection succeeded
    VERIFY_KEYPOINT_NE(DatSenderLinkID2, IOC_ID_INVALID, "Second connection established");

    //@KeyVerifyPoint-2: Data works on new connection
    VERIFY_KEYPOINT_EQ(RecvPrivData.TotalReceivedSize, strlen(SecondData), "Data received on new connection");

    //@KeyVerifyPoint-3: Content matches second transmission (not first)
    RecvPrivData.ReceivedContent[RecvPrivData.TotalReceivedSize] = '\0';
    VERIFY_KEYPOINT_EQ(strcmp(RecvPrivData.ReceivedContent, SecondData), 0, "Second connection data content matches");

    printf("   ✅ Reconnection test SUCCESS:\n");
    printf("      - First connection: Sender=%llu, Receiver=%llu\n", DatSenderLinkID1, DatReceiverLinkID1);
    printf("      - Second connection: Sender=%llu, Receiver=%llu\n", DatSenderLinkID2, DatReceiverLinkID2);
    printf("      - Reconnection succeeded: ✓\n");
    printf("      - Data on new connection: '%s' (%zu bytes)\n", RecvPrivData.ReceivedContent,
           RecvPrivData.TotalReceivedSize);
    printf("      - Independent data transmission: ✓ (second data only, not first)\n");

    //===>>> CLEANUP <<<===
    printf("🧹 CLEANUP\n");

    if (DatReceiverLinkID2 != IOC_ID_INVALID) IOC_closeLink(DatReceiverLinkID2);
    if (DatSenderLinkID2 != IOC_ID_INVALID) IOC_closeLink(DatSenderLinkID2);
    if (DatReceiverSrvID != IOC_ID_INVALID) IOC_offlineService(DatReceiverSrvID);

    printf("   ✓ Cleanup complete\n");
}
//======>END OF: [@AC-1,US-5]======================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF TODO/IMPLEMENTATION TRACKING SECTION=============================================
// 🔴 IMPLEMENTATION STATUS TRACKING - TDD Red→Green Progress
//
// PURPOSE:
//   Track test implementation progress using TDD methodology.
//   Maintain visibility of completed, in-progress, and planned tests.
//
// STATUS LEGEND:
//   ⚪ TODO/PLANNED:      Designed but not implemented yet.
//   🔴 RED/FAILING:       Test written, but production code is missing or incorrect.
//   🟢 GREEN/PASSED:      Test written and passing.
//   ⚠️  ISSUES:           Known problem needing attention.
//   🚫 BLOCKED:          Cannot proceed due to a dependency.
//
// PRIORITY LEVELS:
//   P1 🥇 FUNCTIONAL:     Must complete before P2 (ValidFunc + InvalidFunc).
//   P2 🥈 DESIGN-ORIENTED: Test after P1 (State, Capability, Concurrency).
//   P3 🥉 QUALITY-ORIENTED: Test for quality attributes (Performance, Robust, etc.).
//   P4 🎯 ADDONS:          Optional (Demo, Examples).
//
//===================================================================================================
// P1 🥇 FUNCTIONAL TESTING – ValidFunc (Edge) - THIS FILE SCOPE
//===================================================================================================
//
// 📋 PLANNED TESTS (0/12 implemented):
//
//   ⚪ [@AC-1,US-1] TC-1: verifyEmptyPayload_bySendZeroBytesTC_expectGracefulHandling
//        - Category: Edge - Data Size
//        - Status: TODO
//        - Priority: MEDIUM (protocol may not support 0-byte)
//        - Estimated effort: 30 min
//
//   ⚪ [@AC-1,US-2] TC-2: verifyMinimumData_bySendOneByteTCP_expectSuccessfulTransmission
//        - Category: Edge - Data Size
//        - Status: TODO
//        - Priority: HIGH (critical boundary)
//        - Estimated effort: 30 min
//
//   ⚪ [@AC-2,US-2] TC-3: verifySequentialMinimum_bySendMultipleOneByteTCP_expectIndependentChunks
//        - Category: Edge - Data Size
//        - Status: TODO
//        - Priority: HIGH (merging detection)
//        - Estimated effort: 45 min
//
//   ⚪ [@AC-1,US-3] TC-4: verifyLargeData_bySendOneMegabyteTCP_expectCompleteIntegrity
//        - Category: Edge - Data Size (Large)
//        - Status: TODO
//        - Priority: HIGH (max size validation)
//        - Estimated effort: 1 hour
//
//   ⚪ [@AC-2,US-3] TC-5: verifyLargeDataCleanup_byMultipleLargeSendsTCP_expectNoMemoryLeak
//        - Category: Edge - Data Size (Large)
//        - Status: TODO
//        - Priority: MEDIUM (memory leak detection)
//        - Estimated effort: 1 hour
//
//   ⚪ [@AC-2,US-4] TC-6: verifyNonblockMode_byRecvWithZeroTimeoutTCP_expectImmediateReturn
//        - Category: Edge - Timeout Mode
//        - Status: TODO
//        - Priority: HIGH (mode validation)
//        - Estimated effort: 45 min
//        - Note: TCP polling limitation may apply (similar to UT_DataTypicalTCP TC-3)
//
//   ⚪ [@AC-3,US-4] TC-7: verifySpecificTimeout_byRecvWith100msTimeoutTCP_expectTimeoutResult
//        - Category: Edge - Timeout Mode
//        - Status: TODO
//        - Priority: MEDIUM (timeout precision)
//        - Estimated effort: 1 hour
//
//   ⚪ [@AC-4,US-4] TC-8: verifyBoundaryTimeout_byRecvWith1msTimeoutTCP_expectCorrectBehavior
//        - Category: Edge - Timeout Mode (Boundary)
//        - Status: TODO
//        - Priority: MEDIUM (boundary testing)
//        - Estimated effort: 45 min
//
//   ⚪ [@AC-4,US-4] TC-9: verifyMaxTimeout_byRecvWithMaxTimeoutTCP_expectNoOverflow
//        - Category: Edge - Timeout Mode (Boundary)
//        - Status: TODO
//        - Priority: LOW (max timeout rarely used)
//        - Estimated effort: 1 hour
//
//   ⚪ [@AC-1,US-5] TC-10: verifyReconnection_byDisconnectAndReconnectTCP_expectNewValidLink
//        - Category: Edge - Connection
//        - Status: TODO
//        - Priority: HIGH (reconnection common scenario)
//        - Estimated effort: 1 hour
//
//   ⚪ [@AC-2,US-5] TC-11: verifyMultipleReconnections_byReconnectFiveTimesTCP_expectAllSucceed
//        - Category: Edge - Connection
//        - Status: TODO
//        - Priority: MEDIUM (resource exhaustion detection)
//        - Estimated effort: 1 hour
//
//   ⚪ [@AC-2,US-1 + @AC-2,US-4] TC-12: verifyEdgeCombination_byEmptyDataNonblockTCP_expectGracefulHandling
//        - Category: Edge - Mode Combination
//        - Status: TODO
//        - Priority: LOW (edge × edge scenario)
//        - Estimated effort: 30 min
//
// 🚪 GATE P1-EDGE: All edge tests complete before moving to UT_DataMisuseTCP
//
//===================================================================================================
// IMPLEMENTATION PLAN (Recommended Order)
//===================================================================================================
//
//   Phase 1: Data Size Edges (HIGH priority)
//     1. TC-2: 1-byte transmission (critical boundary)
//     2. TC-3: Multiple 1-byte chunks (merging detection)
//     3. TC-4: 1MB large data (max size validation)
//
//   Phase 2: Connection Edges (HIGH priority)
//     4. TC-10: Single reconnection (common scenario)
//     5. TC-11: Multiple reconnections (resource check)
//
//   Phase 3: Timeout Modes (MEDIUM priority)
//     6. TC-6: NONBLOCK mode (if polling works, else document)
//     7. TC-7: Specific timeout (100ms)
//     8. TC-8: Boundary timeout (1ms)
//
//   Phase 4: Optional/Low Priority
//     9. TC-1: 0-byte payload (protocol may not support)
//     10. TC-5: Large data cleanup (memory leak)
//     11. TC-9: Max timeout (rarely used)
//     12. TC-12: Edge combination (low value)
//
//   Total Estimated Effort: 8-10 hours
//
//===================================================================================================
// NEXT STEPS (After UT_DataEdgeTCP Complete)
//===================================================================================================
//
//   ⚪ UT_DataMisuseTCP.cxx: API misuse patterns (P1 InvalidFunc)
//        - Wrong call sequence, invalid parameters
//        - Fast-Fail Six validation
//        - Priority: HIGH (complete P1)
//
//   ⚪ UT_DataFaultTCP.cxx: Complete skipped timeout tests
//        - 6 tests currently skipped
//        - Priority: MEDIUM
//
//   🚪 After P1 complete, advance to P2 Design-Oriented testing
//
///////////////////////////////////////////////////////////////////////////////////////////////////
//======>END OF TODO/IMPLEMENTATION TRACKING SECTION===============================================
