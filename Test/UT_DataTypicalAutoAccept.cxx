///////////////////////////////////////////////////////////////////////////////////////////////////
// QUICK REFERENCE GUIDE - 快速参考指南
// 📝 用途: DAT（数据传输）典型自动接受连接场景单元测试
// 🔄 流程: User Story → Acceptance Criteria → Test Cases → Implementation
// 📂 分类: DataTypicalAutoAccept - 专注于IOC_SRVFLAG_AUTO_ACCEPT的典型使用场景
// 🎯 重点: 典型的自动接受连接模式和常见自动化使用方法
// Reference Unit Testing Templates in UT_FreelyDrafts.cxx when needed.
///////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF OVERVIEW OF THIS UNIT TESTING FILE===============================================
/**
 * @brief
 *  验证IOC框架中DAT（数据传输）使用IOC_SRVFLAG_AUTO_ACCEPT标志的典型场景，
 *  专注于最常见、最标准的自动接受连接模式。
 *
 *-------------------------------------------------------------------------------------------------
 *++IOC_SRVFLAG_AUTO_ACCEPT是IOC框架中用于自动接受客户端连接的便利特性：
 *
 *  典型使用场景：
 *  - 自动接受连接的DatReceiver服务（无需手动IOC_acceptClient）
 *  - 自动接受连接的DatSender服务（服务端推送数据模式）
 *  - 简化的连接管理流程（减少手动连接处理）
 *  - 典型的回调驱动自动化处理
 *      - 🤖 US-1: DatReceiver服务启用AUTO_ACCEPT，自动接受DatSender连接
 *      - 🤖 US-2: DatSender服务启用AUTO_ACCEPT，自动接受DatReceiver连接
 *
 *  🆕 AUTO_ACCEPT 核心设计理念:
 *  - 简化连接建立流程，减少手动IOC_acceptClient调用
 *  - 适用于需要自动处理多客户端连接的服务场景
 *  - 必须配合回调模式使用（CbRecvDat_F等）
 *  - 提供更流畅的开发体验和更简洁的代码结构
 *
 *  包括：
 *  - 自动连接接受的标准流程
 *  - 回调驱动的数据处理
 *  - 典型的多客户端自动服务场景
 *  - 简化的连接生命周期管理
 *
 *  不包括：
 *  - 手动连接接受测试（已在UT_DataTypical.cxx中覆盖）
 *  - 复杂的状态管理（属于UT_DataState范畴）
 *  - 性能优化场景（属于UT_DataPerformance范畴）
 *  - 错误处理和边界条件（属于UT_DataEdge范畴）
 *
 *  参考文档：
 *  - IOC_SrvTypes.h::IOC_SRVFLAG_AUTO_ACCEPT定义
 *  - README_UserGuide.md::ConetData自动接受示例
 */
//======>END OF OVERVIEW OF THIS UNIT TESTING FILE=================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF UNIT TESTING DESIGN==============================================================

/**************************************************************************************************
 * 📋 DAT TYPICAL AUTO-ACCEPT TEST FOCUS - DAT典型自动接受测试重点
 *
 * 🎯 DESIGN PRINCIPLE: 只验证AUTO_ACCEPT最常见、最标准的使用模式
 * 🔄 PRIORITY: 自动化流程 → 回调驱动 → 简化代码 → 典型场景
 *
 * ✅ TYPICAL AUTO-ACCEPT SCENARIOS INCLUDED (包含的典型自动接受场景):
 *    🤖 Auto Connection Accept: 服务自动接受客户端连接
 *    📞 Callback-Driven Processing: 自动回调驱动的数据处理
 *    🔗 Simplified Connection Flow: 简化的连接建立流程
 *    📦 Common Data Types: 常见数据类型的自动处理
 *    🏢 Multi-Client Service: 多客户端自动服务模式
 *
 * ❌ NON-TYPICAL AUTO-ACCEPT SCENARIOS EXCLUDED (排除的非典型场景):
 *    🔧 手动连接管理（已在UT_DataTypical.cxx覆盖）
 *    🚫 错误处理和异常场景（属于UT_DataEdge范畴）
 *    ⚡ 性能优化和压力测试（属于UT_DataPerformance范畴）
 *    🔄 复杂状态管理（属于UT_DataState范畴）
 *    🚫 轮询模式（AUTO_ACCEPT要求回调模式）
 *************************************************************************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF USER STORY=======================================================================
/**************************************************************************************************
 * @brief 【User Story】
 *
 *  US-1: AS a DatReceiver service developer,
 *    I WANT to enable IOC_SRVFLAG_AUTO_ACCEPT when onlining my service,
 *   SO THAT incoming DatSender connections are automatically accepted without manual IOC_acceptClient calls,
 *       AND I can focus on data processing logic in my CbRecvDat_F callback,
 *       AND the connection management is simplified and automated.
 *
 *  US-2: AS a DatSender service developer,
 *    I WANT to enable IOC_SRVFLAG_AUTO_ACCEPT when onlining my data push service,
 *   SO THAT incoming DatReceiver connections are automatically accepted,
 *      THEN I can immediately start sending data to connected receivers,
 *       AND receivers can process data through their CbRecvDat_F callbacks automatically.
 *
 *************************************************************************************************/
//======>END OF USER STORY=========================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//=======>BEGIN OF ACCEPTANCE CRITERIA=============================================================
/**************************************************************************************************
 * @brief 【Acceptance Criteria】
 *
 * 🎯 专注于 DAT TYPICAL AUTO-ACCEPT 测试 - 只验证最常见的自动接受连接使用模式
 *
 * [@US-1] AS a DatReceiver service developer, I WANT to enable IOC_SRVFLAG_AUTO_ACCEPT,
 *         SO THAT incoming DatSender connections are automatically accepted.
 *
 * [@US-2] AS a DatSender service developer, I WANT to enable IOC_SRVFLAG_AUTO_ACCEPT,
 *         SO THAT incoming DatReceiver connections are automatically accepted.
 *
 * ⭐ TYPICAL AUTO-ACCEPT SCENARIOS ONLY - 典型自动接受场景验收标准:
 *
 *  AC-1@US-1: GIVEN DatReceiver service onlined with IOC_SRVFLAG_AUTO_ACCEPT and CbRecvDat_F callback,
 *         WHEN DatSender calls IOC_connectService to connect,
 *         THEN connection is automatically accepted without manual IOC_acceptClient,
 *          AND DatSender gets IOC_RESULT_SUCCESS and valid LinkID,
 *          AND automatic connection establishment is transparent to DatSender.
 *
 *  AC-2@US-1: GIVEN auto-accept DatReceiver service with established connection,
 *         WHEN DatSender sends typical data using IOC_sendDAT,
 *         THEN DatReceiver automatically processes data via CbRecvDat_F callback,
 *          AND data integrity is maintained in automatic processing workflow,
 *          AND no manual intervention required for data reception.
 *
 *  AC-3@US-1: GIVEN auto-accept DatReceiver service ready to serve multiple clients,
 *         WHEN multiple DatSenders connect simultaneously,
 *         THEN all connections are automatically accepted in order,
 *          AND each DatSender can independently send data,
 *          AND DatReceiver processes all data streams via callback automatically.
 *
 *  AC-4@US-1: GIVEN auto-accept DatReceiver service handling typical data types,
 *         WHEN DatSenders transmit various data types (string, binary, struct),
 *         THEN all data types are automatically processed via callback,
 *          AND data type handling is transparent in auto-accept mode,
 *          AND typical application scenarios work seamlessly.
 *
 *  TODO:AC-5@US-1: ... (其他典型自动接受场景验收标准)
 *--------------------------------------------------------------------------------------------------
 *  AC-1@US-2: GIVEN DatSender service onlined with IOC_SRVFLAG_AUTO_ACCEPT (server role),
 *         WHEN DatReceiver calls IOC_connectService with CbRecvDat_F callback,
 *         THEN connection is automatically accepted without manual IOC_acceptClient,
 *          AND DatReceiver gets IOC_RESULT_SUCCESS and valid LinkID,
 *          AND automatic server-side connection acceptance works transparently.
 *
 *  AC-2@US-2: GIVEN auto-accept DatSender service with connected DatReceiver,
 *         WHEN DatSender sends typical data using IOC_sendDAT,
 *         THEN DatReceiver automatically processes data via CbRecvDat_F callback,
 *          AND server-to-client data flow works automatically,
 *          AND typical push-service scenarios are simplified.
 *
 *  AC-3@US-2: GIVEN auto-accept DatSender service serving multiple DatReceivers,
 *         WHEN DatSender broadcasts data to all connected clients,
 *         THEN all DatReceivers automatically process data via their callbacks,
 *          AND multi-client push service works seamlessly,
 *          AND typical broadcast scenarios are automated.
 *
 *  TODO:AC-4@US-2: ... (其他典型服务端自动接受场景验收标准)
 *
 */
//=======>END OF ACCEPTANCE CRITERIA================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF TEST CASES=======================================================================
/**************************************************************************************************
 * @brief 【Test Cases】
 *
 * 🎯 专注于 DAT TYPICAL AUTO-ACCEPT 测试用例 - 基于自动接受连接的典型使用模式
 *
 * [@AC-1,US-1] - Automatic Connection Acceptance for DatReceiver Service
 *  TC-1:
 *      @[Name]: verifyAutoAcceptConnection_byDatReceiverService_expectAutomaticAcceptance
 *      @[Purpose]: Verify AC-1@US-1 complete functionality - Auto-accept DatReceiver service
 *          automatically accepts DatSender connections without manual intervention
 *      @[Brief]: DatReceiver online service with IOC_SRVFLAG_AUTO_ACCEPT and CbRecvDat_F,
 *          DatSender connect, verify automatic acceptance and no IOC_acceptClient needed
 *
 *  TODO:TC-2...
 *
 *-------------------------------------------------------------------------------------------------
 * [@AC-2,US-1] - Automatic Data Processing with Auto-Accept
 *  TC-1:
 *      @[Name]: verifyAutoDataProcessing_byCallbackDriven_expectSeamlessProcessing
 *      @[Purpose]: Verify AC-2@US-1 complete functionality - Auto-accept service automatically
 *          processes incoming data via callback without manual connection management
 *      @[Brief]: Establish auto-accept connection, DatSender send typical data,
 *          verify automatic callback-driven processing and data integrity
 *
 *  TODO:TC-2...
 *
 *-------------------------------------------------------------------------------------------------
 * [@AC-3,US-1] - Multi-Client Auto-Accept Service
 *  TC-1:
 *      @[Name]: verifyMultiClientAutoAccept_byConcurrentConnections_expectAllAccepted
 *      @[Purpose]: Verify AC-3@US-1 complete functionality - Auto-accept service handles
 *          multiple simultaneous client connections automatically
 *      @[Brief]: Multiple DatSenders connect to auto-accept DatReceiver service concurrently,
 *          verify all connections automatically accepted and data processed independently
 *
 *  TODO:TC-2...
 *
 *-------------------------------------------------------------------------------------------------
 * [@AC-4,US-1] - Auto-Accept with Multiple Data Types
 *  TC-1:
 *      @[Name]: verifyAutoAcceptDataTypes_byTypicalTypes_expectTransparentHandling
 *      @[Purpose]: Verify AC-4@US-1 complete functionality - Auto-accept service transparently
 *          handles various typical data types through automatic callback processing
 *      @[Brief]: Auto-accept connection established, send string/binary/struct data types,
 *          verify automatic processing of all types via callback mechanism
 *
 *  TODO:TC-2...
 *
 *-------------------------------------------------------------------------------------------------
 * [@AC-1,US-2] - DatSender Service Auto-Accept (Server Role)
 *  TC-1:
 *      @[Name]: verifyDatSenderAutoAccept_byServerRole_expectAutomaticClientAcceptance
 *      @[Purpose]: Verify AC-1@US-2 complete functionality - DatSender service with auto-accept
 *          automatically accepts DatReceiver client connections in server role
 *      @[Brief]: DatSender online service with IOC_SRVFLAG_AUTO_ACCEPT, DatReceiver connect,
 *          verify automatic server-side acceptance and immediate data push capability
 *
 *  TODO:TC-2...
 *
 *-------------------------------------------------------------------------------------------------
 * [@AC-2,US-2] - Auto-Accept Server Data Push
 *  TC-1:
 *      @[Name]: verifyAutoAcceptDataPush_byServerToClient_expectAutomaticProcessing
 *      @[Purpose]: Verify AC-2@US-2 complete functionality - Auto-accept DatSender service
 *          pushes data to connected DatReceiver with automatic processing
 *      @[Brief]: Auto-accept DatSender service accept DatReceiver connection, push typical data,
 *          verify automatic client-side callback processing in server-to-client flow
 *
 *  TODO:TC-2...
 *
 *-------------------------------------------------------------------------------------------------
 * [@AC-3,US-2] - Multi-Client Push Service Auto-Accept
 *  TC-1:
 *      @[Name]: verifyMultiClientPushService_byAutoAccept_expectBroadcastProcessing
 *      @[Purpose]: Verify AC-3@US-2 complete functionality - Auto-accept DatSender service
 *          serves multiple DatReceiver clients with automatic broadcast processing
 *      @[Brief]: Auto-accept DatSender service accept multiple DatReceiver connections,
 *          broadcast data to all clients, verify automatic processing by all receivers
 *
 *  TODO:TC-2...
 *
 *************************************************************************************************/
//======>END OF TEST CASES=========================================================================
//======>END OF UNIT TESTING DESIGN================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======BEGIN OF UNIT TESTING IMPLEMENTATION=======================================================
#include <algorithm>
#include <atomic>  // For std::atomic
#include <chrono>  // For std::chrono::milliseconds
#include <mutex>
#include <thread>  // For std::this_thread::sleep_for
#include <vector>

#include "_UT_IOC_Common.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF: [@AC-1,US-1]====================================================================

// Private data structure for auto-accept DAT receiver callback (TDD Design)
typedef struct __AutoAcceptDatReceiverPrivData {
    std::atomic<int> ReceivedDataCnt{0};
    std::atomic<ULONG_T> TotalReceivedSize{0};
    std::atomic<bool> CallbackExecuted{false};
    std::atomic<bool> ConnectionAccepted{false};
    char ReceivedContent[204800];  // Buffer for 200KB+ data
    int ClientIndex;               // Client identifier for multi-client scenarios
    // For AC-3/AC-6: track unique link IDs observed on callbacks (best-effort)
    std::mutex LinksMu;
    IOC_LinkID_T UniqueLinks[16] = {0};
    std::atomic<int> UniqueLinkCnt{0};
} __AutoAcceptDatReceiverPrivData_T;

// Auto-accept callback function for receiving DAT data (TDD Design)
static IOC_Result_T __AutoAcceptCbRecvDat_F(IOC_LinkID_T LinkID, IOC_DatDesc_pT pDatDesc, void *pCbPriv) {
    __AutoAcceptDatReceiverPrivData_T *pPrivData = (__AutoAcceptDatReceiverPrivData_T *)pCbPriv;

    // Signal that connection was automatically accepted (callback indicates successful auto-accept)
    pPrivData->ConnectionAccepted = true;

    // Extract data from DatDesc
    void *pData;
    ULONG_T DataSize;
    IOC_Result_T result = IOC_getDatPayload(pDatDesc, &pData, &DataSize);
    if (result != IOC_RESULT_SUCCESS) {
        printf("AutoAccept Callback: Failed to get data payload, result=%d\n", result);
        return result;
    }

    int currentCount = pPrivData->ReceivedDataCnt.fetch_add(1) + 1;
    pPrivData->CallbackExecuted = true;

    // Track unique LinkIDs (best-effort)
    {
        std::lock_guard<std::mutex> g(pPrivData->LinksMu);
        bool found = false;
        int cnt = pPrivData->UniqueLinkCnt.load();
        for (int i = 0; i < cnt; ++i) {
            if (pPrivData->UniqueLinks[i] == LinkID) {
                found = true;
                break;
            }
        }
        if (!found && cnt < (int)(sizeof(pPrivData->UniqueLinks) / sizeof(pPrivData->UniqueLinks[0]))) {
            pPrivData->UniqueLinks[cnt] = LinkID;
            pPrivData->UniqueLinkCnt.store(cnt + 1);
        }
    }

    // Copy received data to buffer for verification (if space available)
    ULONG_T currentTotalSize = pPrivData->TotalReceivedSize.load();
    if (currentTotalSize + DataSize <= sizeof(pPrivData->ReceivedContent)) {
        memcpy(pPrivData->ReceivedContent + currentTotalSize, pData, DataSize);
    }

    // Always update TotalReceivedSize for accurate tracking
    pPrivData->TotalReceivedSize.fetch_add(DataSize);

    printf("AutoAccept DAT Callback: Client[%d], LinkID=%llu, received %lu bytes, count: %d, total: %lu bytes\n",
           pPrivData->ClientIndex, LinkID, DataSize, currentCount, pPrivData->TotalReceivedSize.load());
    return IOC_RESULT_SUCCESS;
}

/**
 * @[Name]: verifyAutoAcceptConnection_byDatReceiverService_expectAutomaticAcceptance
 * @[Steps]:
 *   1) Setup DatReceiver service with IOC_SRVFLAG_AUTO_ACCEPT and CbRecvDat_F callback AS SETUP.
 *      |-> DatReceiver online service with IOC_LinkUsageDatReceiver + IOC_SRVFLAG_AUTO_ACCEPT
 *      |-> Configure DatUsageArgs with __AutoAcceptCbRecvDat_F callback
 *      |-> No manual IOC_acceptClient setup needed (auto-accept handles this)
 *   2) DatSender connect to the auto-accept service AS BEHAVIOR.
 *      |-> DatSender calls IOC_connectService with typical parameters
 *      |-> Auto-accept service automatically accepts connection in background
 *   3) Verify automatic connection acceptance AS VERIFY.
 *      |-> IOC_connectService() returns IOC_RESULT_SUCCESS for DatSender
 *      |-> DatSender gets valid LinkID without manual IOC_acceptClient call
 *      |-> Connection is ready for immediate data transmission
 *   4) Cleanup: close connection and offline service AS CLEANUP.
 * @[Expect]: Connection established automatically without manual acceptance, demonstrating typical auto-accept usage.
 * @[Notes]: 验证AC-1@US-1 - 自动接受连接的基本功能，无需手动IOC_acceptClient调用。
 */
TEST(UT_DataTypicalAutoAccept, verifyAutoAcceptConnection_byDatReceiverService_expectAutomaticAcceptance) {
    //===SETUP===
    IOC_Result_T Result = IOC_RESULT_BUG;
    IOC_SrvID_T AutoAcceptSrvID = IOC_ID_INVALID;
    IOC_LinkID_T DatSenderLinkID = IOC_ID_INVALID;

    // Private data for auto-accept callback
    __AutoAcceptDatReceiverPrivData_T AutoAcceptPrivData = {};
    AutoAcceptPrivData.ClientIndex = 1;  // For debugging output

    // Standard SrvURI for auto-accept DAT communication
    IOC_SrvURI_T AutoAcceptSrvURI = {
        .pProtocol = IOC_SRV_PROTO_FIFO,
        .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
        .pPath = (const char *)"AutoAccept_DatReceiver",
    };

    // Step-1: Setup DatReceiver service with AUTO_ACCEPT flag and callback
    IOC_DatUsageArgs_T AutoAcceptDatUsageArgs = {
        .CbRecvDat_F = __AutoAcceptCbRecvDat_F,
        .pCbPrivData = &AutoAcceptPrivData,
    };

    IOC_SrvArgs_T AutoAcceptSrvArgs = {
        .SrvURI = AutoAcceptSrvURI,
        .Flags = IOC_SRVFLAG_AUTO_ACCEPT,  // 🤖 Enable automatic connection acceptance
        .UsageCapabilites = IOC_LinkUsageDatReceiver,
        .UsageArgs =
            {
                .pDat = &AutoAcceptDatUsageArgs,
            },
    };

    Result = IOC_onlineService(&AutoAcceptSrvID, &AutoAcceptSrvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint: Auto-accept service online success
    printf("AutoAccept Service: Online with auto-accept capability\n");

    // Brief pause to ensure auto-accept daemon thread is ready
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    //===BEHAVIOR===
    printf("BEHAVIOR: verifyAutoAcceptConnection_byDatReceiverService_expectAutomaticAcceptance\n");

    // Step-2: DatSender connect to auto-accept service (no manual accept needed)
    IOC_ConnArgs_T ConnArgs = {
        .SrvURI = AutoAcceptSrvURI,
        .Usage = IOC_LinkUsageDatSender,
    };

    // Connect directly - auto-accept service will handle acceptance automatically
    Result = IOC_connectService(&DatSenderLinkID, &ConnArgs, NULL);

    //===VERIFY===
    // KeyVerifyPoint-1: Connection should succeed automatically
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result) << "DatSender connection to auto-accept service should succeed automatically";

    // KeyVerifyPoint-2: Valid DatSender LinkID should be returned
    ASSERT_NE(IOC_ID_INVALID, DatSenderLinkID) << "DatSender should get valid LinkID from auto-accept connection";

    // KeyVerifyPoint-3: Connection should be ready for immediate use (send a test data)
    const char *TestMessage = "AutoAccept Test Message";
    IOC_DatDesc_T TestDatDesc = {0};
    IOC_initDatDesc(&TestDatDesc);
    TestDatDesc.Payload.pData = (void *)TestMessage;
    TestDatDesc.Payload.PtrDataSize = strlen(TestMessage);
    TestDatDesc.Payload.PtrDataLen = strlen(TestMessage);

    Result = IOC_sendDAT(DatSenderLinkID, &TestDatDesc, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result) << "Data transmission should work immediately after auto-accept connection";

    IOC_flushDAT(DatSenderLinkID, NULL);

    // Brief wait for callback processing
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // KeyVerifyPoint-4: Auto-accept callback should be triggered
    ASSERT_TRUE(AutoAcceptPrivData.CallbackExecuted.load())
        << "Auto-accept callback should be executed when data is received";

    ASSERT_TRUE(AutoAcceptPrivData.ConnectionAccepted.load())
        << "Connection acceptance should be signaled through callback execution";

    printf("TDD VERIFY: Auto-accept connection established and tested successfully\n");

    //===CLEANUP===
    // Close connection
    if (DatSenderLinkID != IOC_ID_INVALID) {
        IOC_closeLink(DatSenderLinkID);
    }
    // Offline auto-accept service
    if (AutoAcceptSrvID != IOC_ID_INVALID) {
        IOC_offlineService(AutoAcceptSrvID);
    }
}

//======>END OF: [@AC-1,US-1]======================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF: [@AC-2,US-1]====================================================================
/**
 * @[Name]: verifyAutoDataProcessing_byCallbackDriven_expectSeamlessProcessing
 * @[Steps]:
 *   1) Setup auto-accept DatReceiver service with callback AS SETUP.
 *   2) Establish connection and send typical data AS BEHAVIOR.
 *   3) Verify automatic callback-driven data processing AS VERIFY.
 *   4) Cleanup AS CLEANUP.
 * @[Expect]: Data automatically processed via callback without manual intervention.
 * @[Notes]: 验证AC-2@US-1 - 自动数据处理功能，展示回调驱动的无缝处理。
 */
TEST(UT_DataTypicalAutoAccept, verifyAutoDataProcessing_byCallbackDriven_expectSeamlessProcessing) {
    //===SETUP===
    IOC_Result_T Result = IOC_RESULT_BUG;
    IOC_SrvID_T SrvID = IOC_ID_INVALID;
    IOC_LinkID_T SenderLinkID = IOC_ID_INVALID;

    __AutoAcceptDatReceiverPrivData_T Priv = {};
    Priv.ClientIndex = 2;

    IOC_SrvURI_T SrvURI = {
        .pProtocol = IOC_SRV_PROTO_FIFO,
        .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
        .pPath = (const char *)"AutoAccept_CallbackProcessing",
    };

    IOC_DatUsageArgs_T DatArgs = {
        .CbRecvDat_F = __AutoAcceptCbRecvDat_F,
        .pCbPrivData = &Priv,
    };

    IOC_SrvArgs_T SrvArgs = {
        .SrvURI = SrvURI,
        .Flags = IOC_SRVFLAG_AUTO_ACCEPT,
        .UsageCapabilites = IOC_LinkUsageDatReceiver,
        .UsageArgs = {.pDat = &DatArgs},
    };

    Result = IOC_onlineService(&SrvID, &SrvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    //===BEHAVIOR===
    printf("BEHAVIOR: verifyAutoDataProcessing_byCallbackDriven_expectSeamlessProcessing\n");

    IOC_ConnArgs_T ConnArgs = {
        .SrvURI = SrvURI,
        .Usage = IOC_LinkUsageDatSender,
    };

    Result = IOC_connectService(&SenderLinkID, &ConnArgs, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);
    ASSERT_NE(IOC_ID_INVALID, SenderLinkID);

    // Prepare and send 3 typical chunks
    const char *Chunk1 = "AC2-Chunk1: Hello AutoAccept";
    const char *Chunk2 = "AC2-Chunk2: Lorem ipsum dolor sit amet";
    const int Chunk3Size = 2048;  // 2KB binary
    std::vector<char> Chunk3(Chunk3Size);
    for (int i = 0; i < Chunk3Size; ++i) Chunk3[i] = (char)(i % 256);

    auto sendChunk = [&](const void *data, size_t size) {
        IOC_DatDesc_T d = {0};
        IOC_initDatDesc(&d);
        d.Payload.pData = (void *)data;
        d.Payload.PtrDataSize = size;
        d.Payload.PtrDataLen = size;
        IOC_Result_T r = IOC_sendDAT(SenderLinkID, &d, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, r);
        // NOTE: Avoid per-chunk flush which can block depending on transport/backpressure.
        // We'll batch flush after all sends to prevent potential deadlocks.
    };

    sendChunk(Chunk1, strlen(Chunk1));
    sendChunk(Chunk2, strlen(Chunk2));
    sendChunk(Chunk3.data(), Chunk3Size);

    // Single flush after batching sends to push all data through.
    IOC_flushDAT(SenderLinkID, NULL);

    // Expected total length for all chunks (coalescing by transport is allowed)
    ULONG_T expectedTotal = (ULONG_T)strlen(Chunk1) + (ULONG_T)strlen(Chunk2) + (ULONG_T)Chunk3Size;

    // Wait for total received bytes to reach expected amount (up to ~600ms)
    for (int i = 0; i < 60; ++i) {
        if (Priv.TotalReceivedSize.load() >= expectedTotal) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    //===VERIFY===
    ASSERT_TRUE(Priv.CallbackExecuted.load());
    // Depending on transport, multiple sends may be coalesced into fewer callback deliveries.
    // We only require at least one callback and the correct total/content.
    ASSERT_GE(Priv.ReceivedDataCnt.load(), 1);
    ASSERT_LE(Priv.ReceivedDataCnt.load(), 3);
    ASSERT_EQ(expectedTotal, Priv.TotalReceivedSize.load());

    // Verify content integrity in order, allowing that chunks may arrive combined
    size_t offset = 0;
    ASSERT_GE(Priv.TotalReceivedSize.load(), (ULONG_T)(strlen(Chunk1)));
    ASSERT_EQ(0, memcmp(Priv.ReceivedContent + offset, Chunk1, strlen(Chunk1)));
    offset += strlen(Chunk1);

    ASSERT_GE(Priv.TotalReceivedSize.load(), (ULONG_T)(offset + strlen(Chunk2)));
    ASSERT_EQ(0, memcmp(Priv.ReceivedContent + offset, Chunk2, strlen(Chunk2)));
    offset += strlen(Chunk2);

    ASSERT_GE(Priv.TotalReceivedSize.load(), (ULONG_T)(offset + Chunk3Size));
    ASSERT_EQ(0, memcmp(Priv.ReceivedContent + offset, Chunk3.data(), Chunk3Size));

    //===CLEANUP===
    if (SenderLinkID != IOC_ID_INVALID) IOC_closeLink(SenderLinkID);
    if (SrvID != IOC_ID_INVALID) IOC_offlineService(SrvID);
}

//======>END OF: [@AC-2,US-1]======================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF: [@AC-3,US-1]====================================================================
/**
 * @[Name]: verifyMultiClientAutoAccept_byConcurrentConnections_expectAllAccepted
 * @[Steps]:
 *   1) Start one auto-accept DatReceiver service.
 *   2) Spawn multiple DatSender clients concurrently; each connects and sends one message.
 *   3) Verify all messages are processed via callback automatically.
 */
TEST(UT_DataTypicalAutoAccept, verifyMultiClientAutoAccept_byConcurrentConnections_expectAllAccepted) {
    //===SETUP===
    IOC_Result_T Result = IOC_RESULT_BUG;
    IOC_SrvID_T SrvID = IOC_ID_INVALID;

    __AutoAcceptDatReceiverPrivData_T Priv = {};
    Priv.ClientIndex = 3;

    IOC_SrvURI_T SrvURI = {
        .pProtocol = IOC_SRV_PROTO_FIFO,
        .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
        .pPath = (const char *)"AutoAccept_MultiClient",
    };

    IOC_DatUsageArgs_T DatArgs = {
        .CbRecvDat_F = __AutoAcceptCbRecvDat_F,
        .pCbPrivData = &Priv,
    };

    IOC_SrvArgs_T SrvArgs = {
        .SrvURI = SrvURI,
        .Flags = IOC_SRVFLAG_AUTO_ACCEPT,
        .UsageCapabilites = IOC_LinkUsageDatReceiver,
        .UsageArgs = {.pDat = &DatArgs},
    };

    Result = IOC_onlineService(&SrvID, &SrvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    //===BEHAVIOR===
    const int kClients = 3;
    const char *Msgs[kClients] = {"MC-Client-1: Hello", "MC-Client-2: World", "MC-Client-3: AutoAccept"};
    ULONG_T expectedTotal = 0;
    for (int i = 0; i < kClients; ++i) expectedTotal += (ULONG_T)strlen(Msgs[i]);

    std::vector<std::thread> threads;
    threads.reserve(kClients);
    for (int i = 0; i < kClients; ++i) {
        threads.emplace_back([&, i]() {
            IOC_ConnArgs_T ConnArgs = {.SrvURI = SrvURI, .Usage = IOC_LinkUsageDatSender};
            IOC_LinkID_T link = IOC_ID_INVALID;
            IOC_Result_T r = IOC_connectService(&link, &ConnArgs, NULL);
            ASSERT_EQ(IOC_RESULT_SUCCESS, r);
            ASSERT_NE(IOC_ID_INVALID, link);

            IOC_DatDesc_T d = {0};
            IOC_initDatDesc(&d);
            d.Payload.pData = (void *)Msgs[i];
            d.Payload.PtrDataSize = strlen(Msgs[i]);
            d.Payload.PtrDataLen = strlen(Msgs[i]);
            r = IOC_sendDAT(link, &d, NULL);
            ASSERT_EQ(IOC_RESULT_SUCCESS, r);
            IOC_flushDAT(link, NULL);
            IOC_closeLink(link);
        });
    }

    for (auto &t : threads) t.join();

    // Wait for all messages (up to ~600ms)
    for (int i = 0; i < 60; ++i) {
        if (Priv.TotalReceivedSize.load() >= expectedTotal) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    //===VERIFY===
    ASSERT_TRUE(Priv.CallbackExecuted.load());
    // Allow fragmentation; we expect at least kClients callbacks overall.
    ASSERT_GE(Priv.ReceivedDataCnt.load(), kClients);
    ASSERT_EQ(expectedTotal, Priv.TotalReceivedSize.load());

    // Per-client evidence: unique link IDs observed should be >= clients
    ASSERT_GE(Priv.UniqueLinkCnt.load(), kClients);

    // Each client message should appear in the received content (order-agnostic)
    const char *bufBegin = Priv.ReceivedContent;
    const char *bufEnd = Priv.ReceivedContent + Priv.TotalReceivedSize.load();
    for (int i = 0; i < kClients; ++i) {
        const char *m = Msgs[i];
        size_t ml = strlen(m);
        auto it = std::search(bufBegin, bufEnd, m, m + ml);
        ASSERT_NE(it, bufEnd) << "Missing client message in buffer index=" << i;
    }

    //===CLEANUP===
    if (SrvID != IOC_ID_INVALID) IOC_offlineService(SrvID);
}

//======>END OF: [@AC-3,US-1]======================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF: [@AC-4,US-1]====================================================================
/**
 * @[Name]: verifyAutoAcceptDataTypes_byTypicalTypes_expectTransparentHandling
 * @[Steps]:
 *   1) Start auto-accept DatReceiver service.
 *   2) Connect a DatSender and send string, binary, and struct data.
 *   3) Verify data integrity and total size; allow coalescing.
 */
TEST(UT_DataTypicalAutoAccept, verifyAutoAcceptDataTypes_byTypicalTypes_expectTransparentHandling) {
    //===SETUP===
    IOC_Result_T Result = IOC_RESULT_BUG;
    IOC_SrvID_T SrvID = IOC_ID_INVALID;
    IOC_LinkID_T LinkID = IOC_ID_INVALID;

    __AutoAcceptDatReceiverPrivData_T Priv = {};
    Priv.ClientIndex = 4;

    IOC_SrvURI_T SrvURI = {
        .pProtocol = IOC_SRV_PROTO_FIFO,
        .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
        .pPath = (const char *)"AutoAccept_DataTypes",
    };

    IOC_DatUsageArgs_T DatArgs = {.CbRecvDat_F = __AutoAcceptCbRecvDat_F, .pCbPrivData = &Priv};
    IOC_SrvArgs_T SrvArgs = {.SrvURI = SrvURI,
                             .Flags = IOC_SRVFLAG_AUTO_ACCEPT,
                             .UsageCapabilites = IOC_LinkUsageDatReceiver,
                             .UsageArgs = {.pDat = &DatArgs}};

    Result = IOC_onlineService(&SrvID, &SrvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    //===BEHAVIOR===
    IOC_ConnArgs_T ConnArgs = {.SrvURI = SrvURI, .Usage = IOC_LinkUsageDatSender};
    Result = IOC_connectService(&LinkID, &ConnArgs, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);
    ASSERT_NE(IOC_ID_INVALID, LinkID);

    // Prepare typical data types
    const char *Str = "DT-String: Quick brown fox";
    struct Packed {
        int a;
        float b;
        uint8_t c[8];
    } __attribute__((packed));
    Packed S = {42, 3.14f, {1, 2, 3, 4, 5, 6, 7, 8}};
    const int BinSize = 1024;
    std::vector<uint8_t> Bin(BinSize);
    for (int i = 0; i < BinSize; ++i) Bin[i] = (uint8_t)(i & 0xFF);

    auto sendChunk = [&](const void *data, size_t size) {
        IOC_DatDesc_T d = {0};
        IOC_initDatDesc(&d);
        d.Payload.pData = (void *)data;
        d.Payload.PtrDataSize = size;
        d.Payload.PtrDataLen = size;
        IOC_Result_T r = IOC_sendDAT(LinkID, &d, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, r);
    };

    sendChunk(Str, strlen(Str));
    sendChunk(&S, sizeof(S));
    sendChunk(Bin.data(), Bin.size());

    IOC_flushDAT(LinkID, NULL);

    ULONG_T expectedTotal = (ULONG_T)strlen(Str) + (ULONG_T)sizeof(S) + (ULONG_T)Bin.size();
    for (int i = 0; i < 80; ++i) {
        if (Priv.TotalReceivedSize.load() >= expectedTotal) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    //===VERIFY===
    ASSERT_TRUE(Priv.CallbackExecuted.load());
    ASSERT_EQ(expectedTotal, Priv.TotalReceivedSize.load());

    size_t offset = 0;
    ASSERT_EQ(0, memcmp(Priv.ReceivedContent + offset, Str, strlen(Str)));
    offset += strlen(Str);
    ASSERT_EQ(0, memcmp(Priv.ReceivedContent + offset, &S, sizeof(S)));
    offset += sizeof(S);
    ASSERT_EQ(0, memcmp(Priv.ReceivedContent + offset, Bin.data(), Bin.size()));

    //===CLEANUP===
    if (LinkID != IOC_ID_INVALID) IOC_closeLink(LinkID);
    if (SrvID != IOC_ID_INVALID) IOC_offlineService(SrvID);
}

//======>END OF: [@AC-4,US-1]======================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF: [@AC-5,US-1]====================================================================
/**
 * @[Name]: verifyAutoAcceptLargePayload_bySingleSend_expectIntegrity
 * @[Purpose]: Typical large (not perf) single payload goes through auto-accept path.
 */
TEST(UT_DataTypicalAutoAccept, verifyAutoAcceptLargePayload_bySingleSend_expectIntegrity) {
    //===SETUP===
    IOC_Result_T Result = IOC_RESULT_BUG;
    IOC_SrvID_T SrvID = IOC_ID_INVALID;
    IOC_LinkID_T LinkID = IOC_ID_INVALID;

    __AutoAcceptDatReceiverPrivData_T Priv = {};
    Priv.ClientIndex = 5;

    IOC_SrvURI_T SrvURI = {.pProtocol = IOC_SRV_PROTO_FIFO,
                           .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
                           .pPath = (const char *)"AutoAccept_LargePayload"};
    IOC_DatUsageArgs_T DatArgs = {.CbRecvDat_F = __AutoAcceptCbRecvDat_F, .pCbPrivData = &Priv};
    IOC_SrvArgs_T SrvArgs = {.SrvURI = SrvURI,
                             .Flags = IOC_SRVFLAG_AUTO_ACCEPT,
                             .UsageCapabilites = IOC_LinkUsageDatReceiver,
                             .UsageArgs = {.pDat = &DatArgs}};

    Result = IOC_onlineService(&SrvID, &SrvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    //===BEHAVIOR===
    IOC_ConnArgs_T ConnArgs = {.SrvURI = SrvURI, .Usage = IOC_LinkUsageDatSender};
    Result = IOC_connectService(&LinkID, &ConnArgs, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);
    ASSERT_NE(IOC_ID_INVALID, LinkID);

    // 128 KB payload within buffer budget (200 KB)
    const size_t kSize = 128 * 1024;
    std::vector<uint8_t> data(kSize);
    for (size_t i = 0; i < kSize; ++i) data[i] = (uint8_t)((i * 131) & 0xFF);

    IOC_DatDesc_T d = {0};
    IOC_initDatDesc(&d);
    d.Payload.pData = data.data();
    d.Payload.PtrDataSize = data.size();
    d.Payload.PtrDataLen = data.size();
    Result = IOC_sendDAT(LinkID, &d, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);
    IOC_flushDAT(LinkID, NULL);

    // Wait for full reception
    for (int i = 0; i < 400; ++i) {
        if (Priv.TotalReceivedSize.load() >= (ULONG_T)data.size()) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    //===VERIFY===
    ASSERT_TRUE(Priv.CallbackExecuted.load());
    ASSERT_EQ((ULONG_T)data.size(), Priv.TotalReceivedSize.load());
    ASSERT_EQ(0, memcmp(Priv.ReceivedContent, data.data(), data.size()));

    //===CLEANUP===
    if (LinkID != IOC_ID_INVALID) IOC_closeLink(LinkID);
    if (SrvID != IOC_ID_INVALID) IOC_offlineService(SrvID);
}

//======>END OF: [@AC-5,US-1]======================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF: [@AC-6,US-1]====================================================================
/**
 * @[Name]: verifyAutoAcceptReconnectLifecycle_byCloseAndReconnect_expectContinuedService
 * @[Purpose]: Typical lifecycle — close a client and reconnect on same service; auto-accept keeps working.
 */
TEST(UT_DataTypicalAutoAccept, verifyAutoAcceptReconnectLifecycle_byCloseAndReconnect_expectContinuedService) {
    //===SETUP===
    IOC_Result_T Result = IOC_RESULT_BUG;
    IOC_SrvID_T SrvID = IOC_ID_INVALID;
    IOC_LinkID_T LinkID = IOC_ID_INVALID;

    __AutoAcceptDatReceiverPrivData_T Priv = {};
    Priv.ClientIndex = 6;

    IOC_SrvURI_T SrvURI = {.pProtocol = IOC_SRV_PROTO_FIFO,
                           .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
                           .pPath = (const char *)"AutoAccept_Reconnect"};
    IOC_DatUsageArgs_T DatArgs = {.CbRecvDat_F = __AutoAcceptCbRecvDat_F, .pCbPrivData = &Priv};
    IOC_SrvArgs_T SrvArgs = {.SrvURI = SrvURI,
                             .Flags = IOC_SRVFLAG_AUTO_ACCEPT,
                             .UsageCapabilites = IOC_LinkUsageDatReceiver,
                             .UsageArgs = {.pDat = &DatArgs}};

    Result = IOC_onlineService(&SrvID, &SrvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    auto connect_and_send = [&](const char *msg) {
        IOC_ConnArgs_T ConnArgs = {.SrvURI = SrvURI, .Usage = IOC_LinkUsageDatSender};
        IOC_LinkID_T l = IOC_ID_INVALID;
        IOC_Result_T r = IOC_connectService(&l, &ConnArgs, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, r);
        ASSERT_NE(IOC_ID_INVALID, l);
        IOC_DatDesc_T d = {0};
        IOC_initDatDesc(&d);
        d.Payload.pData = (void *)msg;
        d.Payload.PtrDataSize = strlen(msg);
        d.Payload.PtrDataLen = strlen(msg);
        r = IOC_sendDAT(l, &d, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, r);
        IOC_flushDAT(l, NULL);
        IOC_closeLink(l);
    };

    //===BEHAVIOR===
    const char *Msg1 = "RC-First: Hello";
    const char *Msg2 = "RC-Second: Again";
    connect_and_send(Msg1);
    // Wait a moment to ensure first close is processed
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    connect_and_send(Msg2);

    ULONG_T expected = (ULONG_T)strlen(Msg1) + (ULONG_T)strlen(Msg2);
    for (int i = 0; i < 100; ++i) {
        if (Priv.TotalReceivedSize.load() >= expected) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    //===VERIFY===
    ASSERT_TRUE(Priv.CallbackExecuted.load());
    ASSERT_GE(Priv.UniqueLinkCnt.load(), 1);  // could be 2 if LinkIDs differ
    ASSERT_EQ(expected, Priv.TotalReceivedSize.load());
    // Ensure both messages are present (order not guaranteed)
    const char *bufBegin = Priv.ReceivedContent;
    const char *bufEnd = Priv.ReceivedContent + Priv.TotalReceivedSize.load();
    auto it1 = std::search(bufBegin, bufEnd, Msg1, Msg1 + strlen(Msg1));
    auto it2 = std::search(bufBegin, bufEnd, Msg2, Msg2 + strlen(Msg2));
    ASSERT_NE(it1, bufEnd);
    ASSERT_NE(it2, bufEnd);

    //===CLEANUP===
    if (LinkID != IOC_ID_INVALID) IOC_closeLink(LinkID);
    if (SrvID != IOC_ID_INVALID) IOC_offlineService(SrvID);
}

//======>END OF: [@AC-6,US-1]======================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF: [@AC-1,US-2]====================================================================
/**
 * @[Name]: verifyDatSenderAutoAccept_byServerRole_expectAutomaticClientAcceptance
 * @[Steps]:
 *   1) Setup DatSender service with IOC_SRVFLAG_AUTO_ACCEPT (server role) AS SETUP.
 *   2) DatReceiver connect to auto-accept DatSender service AS BEHAVIOR.
 *   3) Verify automatic server-side connection acceptance AS VERIFY.
 *   4) Cleanup AS CLEANUP.
 * @[Expect]: Server-side auto-accept works for DatSender service accepting DatReceiver clients.
 * @[Notes]: 验证AC-1@US-2 - 服务端自动接受功能，DatSender作为服务器自动接受DatReceiver客户端。
 */
TEST(UT_DataTypicalAutoAccept, verifyDatSenderAutoAccept_byServerRole_expectAutomaticClientAcceptance) {
    // TODO: Implement this test case
    // This test will verify auto-accept functionality when DatSender acts as server
    // and automatically accepts DatReceiver client connections
    GTEST_SKIP() << "Test case implementation pending - designed for server-side auto-accept verification";
}

//======>END OF: [@AC-1,US-2]======================================================================

//======END OF UNIT TESTING IMPLEMENTATION=========================================================
///////////////////////////////////////////////////////////////////////////////////////////////////
#include <gtest/gtest.h>

// Legacy monolithic AutoAccept tests have been split into US1/US2 files.
// This placeholder keeps the target but avoids duplicate test cases.
TEST(UT_DataTypicalAutoAccept_Legacy, Placeholder) {
    GTEST_SKIP() << "Legacy tests replaced by DataTypicalAutoAcceptUS1/US2";
}
