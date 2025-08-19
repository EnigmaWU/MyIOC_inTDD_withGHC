///////////////////////////////////////////////////////////////////////////////////////////////////
// DAT Typical Auto-Accept — US-1 tests (Service=DatReceiver, Client=DatSender)
//
// UT Design (extracted from the original UT_DataTypicalAutoAccept.cxx):
// - Story: As a DatReceiver service developer, I enable IOC_SRVFLAG_AUTO_ACCEPT to accept
//          senders automatically and process data via the configured CbRecvDat_F.
// - Scope: P2P (not broadcast). Service Usage=IOC_LinkUsageDatReceiver. Client Usage=DatSender.
// - Invariants:
//     * No explicit IOC_acceptClient(); accepted links are discoverable via service internals.
//     * Coalescing is allowed: multiple sends may arrive in fewer callbacks; assertions tolerate it.
//     * Flush semantics: callers may batch multiple IOC_sendDAT() then IOC_flushDAT().
// - What we verify:
//     AC-1  Connection auto-accept and first data arrives in callback (basic smoke).
//     AC-2  Typical data processing using callback-driven flow; totals and ordering checked.
//     AC-3  Multi-client acceptance: all clients accepted; per-link delivery covered best-effort.
//     AC-4  Common data types (string/packed/binary) handled transparently.
//     AC-5  Large payload integrity (~128KB+) under possible coalescing.
//     AC-6  Reconnect lifecycle; continued service health across link closes.
// - Notes:
//     * Performance is out-of-scope here (covered by DataPerformance US files).
//     * Threading: service runs an accept loop; callbacks must remain non-blocking.
//
// Rationale for split: keep tests focused and readable while preserving the UT design above.
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <algorithm>
#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>
#include <vector>

#include "_UT_IOC_Common.h"

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

// === [@AC-1,US-1]
TEST(UT_DataTypicalAutoAccept, verifyAutoAcceptConnection_byDatReceiverService_expectAutomaticAcceptance) {
    IOC_Result_T Result = IOC_RESULT_BUG;
    IOC_SrvID_T AutoAcceptSrvID = IOC_ID_INVALID;
    IOC_LinkID_T DatSenderLinkID = IOC_ID_INVALID;

    __AutoAcceptDatReceiverPrivData_T AutoAcceptPrivData = {};
    AutoAcceptPrivData.ClientIndex = 1;

    IOC_SrvURI_T AutoAcceptSrvURI = {
        .pProtocol = IOC_SRV_PROTO_FIFO,
        .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
        .pPath = (const char *)"AutoAccept_DatReceiver",
    };

    IOC_DatUsageArgs_T AutoAcceptDatUsageArgs = {
        .CbRecvDat_F = __AutoAcceptCbRecvDat_F,
        .pCbPrivData = &AutoAcceptPrivData,
    };

    IOC_SrvArgs_T AutoAcceptSrvArgs = {
        .SrvURI = AutoAcceptSrvURI,
        .Flags = IOC_SRVFLAG_AUTO_ACCEPT,
        .UsageCapabilites = IOC_LinkUsageDatReceiver,
        .UsageArgs = {.pDat = &AutoAcceptDatUsageArgs},
    };

    Result = IOC_onlineService(&AutoAcceptSrvID, &AutoAcceptSrvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    IOC_ConnArgs_T ConnArgs = {.SrvURI = AutoAcceptSrvURI, .Usage = IOC_LinkUsageDatSender};
    Result = IOC_connectService(&DatSenderLinkID, &ConnArgs, NULL);

    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);
    ASSERT_NE(IOC_ID_INVALID, DatSenderLinkID);

    const char *TestMessage = "AutoAccept Test Message";
    IOC_DatDesc_T TestDatDesc = {0};
    IOC_initDatDesc(&TestDatDesc);
    TestDatDesc.Payload.pData = (void *)TestMessage;
    TestDatDesc.Payload.PtrDataSize = strlen(TestMessage);
    TestDatDesc.Payload.PtrDataLen = strlen(TestMessage);

    Result = IOC_sendDAT(DatSenderLinkID, &TestDatDesc, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);
    IOC_flushDAT(DatSenderLinkID, NULL);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    ASSERT_TRUE(AutoAcceptPrivData.CallbackExecuted.load());
    ASSERT_TRUE(AutoAcceptPrivData.ConnectionAccepted.load());

    if (DatSenderLinkID != IOC_ID_INVALID) IOC_closeLink(DatSenderLinkID);
    if (AutoAcceptSrvID != IOC_ID_INVALID) IOC_offlineService(AutoAcceptSrvID);
}

// === [@AC-2,US-1]
TEST(UT_DataTypicalAutoAccept, verifyAutoDataProcessing_byCallbackDriven_expectSeamlessProcessing) {
    IOC_Result_T Result = IOC_RESULT_BUG;
    IOC_SrvID_T SrvID = IOC_ID_INVALID;
    IOC_LinkID_T SenderLinkID = IOC_ID_INVALID;

    __AutoAcceptDatReceiverPrivData_T Priv = {};
    Priv.ClientIndex = 2;

    IOC_SrvURI_T SrvURI = {.pProtocol = IOC_SRV_PROTO_FIFO,
                           .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
                           .pPath = (const char *)"AutoAccept_CallbackProcessing"};

    IOC_DatUsageArgs_T DatArgs = {.CbRecvDat_F = __AutoAcceptCbRecvDat_F, .pCbPrivData = &Priv};
    IOC_SrvArgs_T SrvArgs = {.SrvURI = SrvURI,
                             .Flags = IOC_SRVFLAG_AUTO_ACCEPT,
                             .UsageCapabilites = IOC_LinkUsageDatReceiver,
                             .UsageArgs = {.pDat = &DatArgs}};

    Result = IOC_onlineService(&SrvID, &SrvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    IOC_ConnArgs_T ConnArgs = {.SrvURI = SrvURI, .Usage = IOC_LinkUsageDatSender};
    Result = IOC_connectService(&SenderLinkID, &ConnArgs, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);
    ASSERT_NE(IOC_ID_INVALID, SenderLinkID);

    const char *Chunk1 = "AC2-Chunk1: Hello AutoAccept";
    const char *Chunk2 = "AC2-Chunk2: Lorem ipsum dolor sit amet";
    const int Chunk3Size = 2048;
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
    };

    sendChunk(Chunk1, strlen(Chunk1));
    sendChunk(Chunk2, strlen(Chunk2));
    sendChunk(Chunk3.data(), Chunk3.size());
    IOC_flushDAT(SenderLinkID, NULL);

    ULONG_T expectedTotal = (ULONG_T)strlen(Chunk1) + (ULONG_T)strlen(Chunk2) + (ULONG_T)Chunk3.size();
    for (int i = 0; i < 60; ++i) {
        if (Priv.TotalReceivedSize.load() >= expectedTotal) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    ASSERT_TRUE(Priv.CallbackExecuted.load());
    ASSERT_GE(Priv.ReceivedDataCnt.load(), 1);
    ASSERT_LE(Priv.ReceivedDataCnt.load(), 3);
    ASSERT_EQ(expectedTotal, Priv.TotalReceivedSize.load());

    size_t offset = 0;
    ASSERT_GE(Priv.TotalReceivedSize.load(), (ULONG_T)(strlen(Chunk1)));
    ASSERT_EQ(0, memcmp(Priv.ReceivedContent + offset, Chunk1, strlen(Chunk1)));
    offset += strlen(Chunk1);
    ASSERT_GE(Priv.TotalReceivedSize.load(), (ULONG_T)(offset + strlen(Chunk2)));
    ASSERT_EQ(0, memcmp(Priv.ReceivedContent + offset, Chunk2, strlen(Chunk2)));
    offset += strlen(Chunk2);
    ASSERT_GE(Priv.TotalReceivedSize.load(), (ULONG_T)(offset + Chunk3.size()));
    ASSERT_EQ(0, memcmp(Priv.ReceivedContent + offset, Chunk3.data(), Chunk3.size()));

    if (SenderLinkID != IOC_ID_INVALID) IOC_closeLink(SenderLinkID);
    if (SrvID != IOC_ID_INVALID) IOC_offlineService(SrvID);
}

// === [@AC-3,US-1]
TEST(UT_DataTypicalAutoAccept, verifyMultiClientAutoAccept_byConcurrentConnections_expectAllAccepted) {
    IOC_Result_T Result = IOC_RESULT_BUG;
    IOC_SrvID_T SrvID = IOC_ID_INVALID;

    __AutoAcceptDatReceiverPrivData_T Priv = {};
    Priv.ClientIndex = 3;

    IOC_SrvURI_T SrvURI = {.pProtocol = IOC_SRV_PROTO_FIFO,
                           .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
                           .pPath = (const char *)"AutoAccept_MultiClient"};

    IOC_DatUsageArgs_T DatArgs = {.CbRecvDat_F = __AutoAcceptCbRecvDat_F, .pCbPrivData = &Priv};
    IOC_SrvArgs_T SrvArgs = {.SrvURI = SrvURI,
                             .Flags = IOC_SRVFLAG_AUTO_ACCEPT,
                             .UsageCapabilites = IOC_LinkUsageDatReceiver,
                             .UsageArgs = {.pDat = &DatArgs}};

    Result = IOC_onlineService(&SrvID, &SrvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

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

    for (int i = 0; i < 60; ++i) {
        if (Priv.TotalReceivedSize.load() >= expectedTotal) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    ASSERT_TRUE(Priv.CallbackExecuted.load());
    ASSERT_GE(Priv.ReceivedDataCnt.load(), kClients);
    ASSERT_EQ(expectedTotal, Priv.TotalReceivedSize.load());
    ASSERT_GE(Priv.UniqueLinkCnt.load(), kClients);

    const char *bufBegin = Priv.ReceivedContent;
    const char *bufEnd = Priv.ReceivedContent + Priv.TotalReceivedSize.load();
    for (int i = 0; i < kClients; ++i) {
        const char *m = Msgs[i];
        size_t ml = strlen(m);
        auto it = std::search(bufBegin, bufEnd, m, m + ml);
        ASSERT_NE(it, bufEnd);
    }

    if (SrvID != IOC_ID_INVALID) IOC_offlineService(SrvID);
}

// === [@AC-4,US-1]
TEST(UT_DataTypicalAutoAccept, verifyAutoAcceptDataTypes_byTypicalTypes_expectTransparentHandling) {
    IOC_Result_T Result = IOC_RESULT_BUG;
    IOC_SrvID_T SrvID = IOC_ID_INVALID;
    IOC_LinkID_T LinkID = IOC_ID_INVALID;

    __AutoAcceptDatReceiverPrivData_T Priv = {};
    Priv.ClientIndex = 4;

    IOC_SrvURI_T SrvURI = {.pProtocol = IOC_SRV_PROTO_FIFO,
                           .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
                           .pPath = (const char *)"AutoAccept_DataTypes"};

    IOC_DatUsageArgs_T DatArgs = {.CbRecvDat_F = __AutoAcceptCbRecvDat_F, .pCbPrivData = &Priv};
    IOC_SrvArgs_T SrvArgs = {.SrvURI = SrvURI,
                             .Flags = IOC_SRVFLAG_AUTO_ACCEPT,
                             .UsageCapabilites = IOC_LinkUsageDatReceiver,
                             .UsageArgs = {.pDat = &DatArgs}};

    Result = IOC_onlineService(&SrvID, &SrvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    IOC_ConnArgs_T ConnArgs = {.SrvURI = SrvURI, .Usage = IOC_LinkUsageDatSender};
    Result = IOC_connectService(&LinkID, &ConnArgs, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);
    ASSERT_NE(IOC_ID_INVALID, LinkID);

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

    ASSERT_TRUE(Priv.CallbackExecuted.load());
    ASSERT_EQ(expectedTotal, Priv.TotalReceivedSize.load());

    size_t offset = 0;
    ASSERT_EQ(0, memcmp(Priv.ReceivedContent + offset, Str, strlen(Str)));
    offset += strlen(Str);
    ASSERT_EQ(0, memcmp(Priv.ReceivedContent + offset, &S, sizeof(S)));
    offset += sizeof(S);
    ASSERT_EQ(0, memcmp(Priv.ReceivedContent + offset, Bin.data(), Bin.size()));

    if (LinkID != IOC_ID_INVALID) IOC_closeLink(LinkID);
    if (SrvID != IOC_ID_INVALID) IOC_offlineService(SrvID);
}

// === [@AC-5,US-1]
TEST(UT_DataTypicalAutoAccept, verifyAutoAcceptLargePayload_bySingleSend_expectIntegrity) {
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

    IOC_ConnArgs_T ConnArgs = {.SrvURI = SrvURI, .Usage = IOC_LinkUsageDatSender};
    Result = IOC_connectService(&LinkID, &ConnArgs, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);
    ASSERT_NE(IOC_ID_INVALID, LinkID);

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

    for (int i = 0; i < 400; ++i) {
        if (Priv.TotalReceivedSize.load() >= (ULONG_T)data.size()) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    ASSERT_TRUE(Priv.CallbackExecuted.load());
    ASSERT_EQ((ULONG_T)data.size(), Priv.TotalReceivedSize.load());
    ASSERT_EQ(0, memcmp(Priv.ReceivedContent, data.data(), data.size()));

    if (LinkID != IOC_ID_INVALID) IOC_closeLink(LinkID);
    if (SrvID != IOC_ID_INVALID) IOC_offlineService(SrvID);
}

// === [@AC-6,US-1]
TEST(UT_DataTypicalAutoAccept, verifyAutoAcceptReconnectLifecycle_byCloseAndReconnect_expectContinuedService) {
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

    const char *Msg1 = "RC-First: Hello";
    const char *Msg2 = "RC-Second: Again";
    connect_and_send(Msg1);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    connect_and_send(Msg2);

    ULONG_T expected = (ULONG_T)strlen(Msg1) + (ULONG_T)strlen(Msg2);
    for (int i = 0; i < 100; ++i) {
        if (Priv.TotalReceivedSize.load() >= expected) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    ASSERT_TRUE(Priv.CallbackExecuted.load());
    ASSERT_GE(Priv.UniqueLinkCnt.load(), 1);
    ASSERT_EQ(expected, Priv.TotalReceivedSize.load());

    const char *bufBegin = Priv.ReceivedContent;
    const char *bufEnd = Priv.ReceivedContent + Priv.TotalReceivedSize.load();
    auto it1 = std::search(bufBegin, bufEnd, Msg1, Msg1 + strlen(Msg1));
    auto it2 = std::search(bufBegin, bufEnd, Msg2, Msg2 + strlen(Msg2));
    ASSERT_NE(it1, bufEnd);
    ASSERT_NE(it2, bufEnd);

    if (LinkID != IOC_ID_INVALID) IOC_closeLink(LinkID);
    if (SrvID != IOC_ID_INVALID) IOC_offlineService(SrvID);
}
