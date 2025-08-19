///////////////////////////////////////////////////////////////////////////////////////////////////
// DAT Typical Auto-Accept — US-2 tests (Service=DatSender, Client=DatReceiver)
//
// UT Design (mirrors the monolithic file’s commentary, adapted to US-2 role):
// - Story: As a DatSender service developer, I enable IOC_SRVFLAG_AUTO_ACCEPT so the service
//          auto-accepts receiver clients and I can start pushing data immediately.
// - Scope: P2P. Service Usage=IOC_LinkUsageDatSender. Client Usage=DatReceiver.
// - Discovery Model:
//     * Hook path: OnAutoAccepted_F(srv, link, priv) is invoked right after acceptance.
//     * Polling path: IOC_getServiceLinkIDs(srv, ...) lists new links to be used for send.
// - Invariants:
//     * No manual IOC_acceptClient().
//     * Coalescing allowed; assertions focus on totals and byte-wise integrity, not callback count.
//     * Send pattern encourages batching with IOC_flushDAT() to emit.
// - Acceptance Criteria:
//     AC-1  Hook path immediate send (enabled once callback is wired by SUT).
//     AC-1b Polling path discovery + send (validated here).
//     AC-2  Typical batched send; totals and ordering verified at client.
//     AC-3  Multi-client unicast; per-link isolation of payloads.
//     AC-4  Large payload integrity (~200KB class).
//     AC-5  Reconnect lifecycle resilience.
//     AC-6+ Offline/Capacity/Hook safety as needed.
// - Note: At time of split, hook wasn’t wired; polling tests proceed; hook tests to follow.
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

#include "_UT_IOC_Common.h"

// Shared receiver-side callback data (client acts as receiver in US-2)
typedef struct __US2_RecvPriv {
    std::atomic<int> ReceivedDataCnt{0};
    std::atomic<ULONG_T> TotalReceivedSize{0};
    std::atomic<bool> CallbackExecuted{false};
    char ReceivedContent[204800];
} __US2_RecvPriv_T;

static IOC_Result_T __US2_CbRecv(IOC_LinkID_T LinkID, IOC_DatDesc_pT pDatDesc, void *pPriv) {
    __US2_RecvPriv_T *priv = (__US2_RecvPriv_T *)pPriv;
    void *pData = nullptr;
    ULONG_T len = 0;
    IOC_Result_T r = IOC_getDatPayload(pDatDesc, &pData, &len);
    if (r != IOC_RESULT_SUCCESS) return r;
    int c = priv->ReceivedDataCnt.fetch_add(1) + 1;
    priv->CallbackExecuted = true;
    ULONG_T cur = priv->TotalReceivedSize.load();
    if (cur + len <= sizeof(priv->ReceivedContent)) memcpy(priv->ReceivedContent + cur, pData, len);
    priv->TotalReceivedSize.fetch_add(len);
    printf("US2 recv: Link=%llu, got %lu bytes, cnt=%d, total=%lu\n", LinkID, len, c, priv->TotalReceivedSize.load());
    return IOC_RESULT_SUCCESS;
}

// Helper to build a DatSender service (server) with AUTO_ACCEPT
static void __US2_buildDatSenderService(IOC_SrvURI_T SrvURI, IOC_SrvID_T *pSrvID,
                                        IOC_CbOnAutoAccepted_F onAccepted = NULL, void *pPriv = NULL) {
    IOC_SrvArgs_T args = {.SrvURI = SrvURI,
                          .Flags = IOC_SRVFLAG_AUTO_ACCEPT,
                          .UsageCapabilites = IOC_LinkUsageDatSender,
                          .UsageArgs = {.pDat = NULL},
                          .OnAutoAccepted_F = onAccepted,
                          .pSrvPriv = pPriv};
    IOC_Result_T r = IOC_onlineService(pSrvID, &args);
    ASSERT_EQ(IOC_RESULT_SUCCESS, r);
}

// AC-1 (hook): provide OnAutoAccepted_F to send immediately when link is accepted
typedef struct __US2_HookPriv {
    const char *pMsg;
    std::atomic<bool> Sent{false};
} __US2_HookPriv_T;
static void __US2_OnAcceptedHook(IOC_SrvID_T srv, IOC_LinkID_T link, void *pPriv) {
    __US2_HookPriv_T *hp = (__US2_HookPriv_T *)pPriv;
    if (!hp || !hp->pMsg) return;
    IOC_DatDesc_T d = {0};
    IOC_initDatDesc(&d);
    d.Payload.pData = (void *)hp->pMsg;
    d.Payload.PtrDataSize = strlen(hp->pMsg);
    d.Payload.PtrDataLen = strlen(hp->pMsg);
    if (IOC_sendDAT(link, &d, NULL) == IOC_RESULT_SUCCESS) {
        IOC_flushDAT(link, NULL);
        hp->Sent = true;
    }
}

TEST(UT_DataTypicalAutoAccept, US2_AcceptAndSend_byHook_expectImmediateDelivery) {
    IOC_SrvID_T srv = IOC_ID_INVALID;
    IOC_SrvURI_T uri = {.pProtocol = IOC_SRV_PROTO_FIFO,
                        .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
                        .pPath = (const char *)"US2_DatSender_Hook"};

    __US2_RecvPriv cliPriv = {};
    IOC_DatUsageArgs_T cliDatArgs = {.CbRecvDat_F = __US2_CbRecv, .pCbPrivData = &cliPriv};
    IOC_ConnArgs_T cliConn = {.SrvURI = uri, .Usage = IOC_LinkUsageDatReceiver, .UsageArgs = {.pDat = &cliDatArgs}};

    __US2_HookPriv_T hookPriv = {.pMsg = "US2-Hook: hi"};
    __US2_buildDatSenderService(uri, &srv, __US2_OnAcceptedHook, &hookPriv);
    std::this_thread::sleep_for(std::chrono::milliseconds(30));

    IOC_LinkID_T cliLink = IOC_ID_INVALID;
    ASSERT_EQ(IOC_RESULT_SUCCESS, IOC_connectService(&cliLink, &cliConn, NULL));

    // Wait for hook to send and client to receive
    for (int i = 0; i < 60; ++i) {
        if (hookPriv.Sent.load() && cliPriv.TotalReceivedSize.load() >= strlen(hookPriv.pMsg)) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    ASSERT_TRUE(hookPriv.Sent.load());
    ASSERT_TRUE(cliPriv.CallbackExecuted.load());
    ASSERT_EQ((ULONG_T)strlen(hookPriv.pMsg), cliPriv.TotalReceivedSize.load());

    if (cliLink != IOC_ID_INVALID) IOC_closeLink(cliLink);
    if (srv != IOC_ID_INVALID) IOC_offlineService(srv);
}

// AC-1b (polling): discover link via IOC_getServiceLinkIDs and send
TEST(UT_DataTypicalAutoAccept, US2_AcceptAndSend_byPolling_expectWorks) {
    IOC_SrvID_T srv = IOC_ID_INVALID;
    IOC_SrvURI_T uri = {.pProtocol = IOC_SRV_PROTO_FIFO,
                        .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
                        .pPath = (const char *)"US2_DatSender_Polling"};

    __US2_buildDatSenderService(uri, &srv, NULL, NULL);  // no hook (polling path)
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Client (receiver)
    __US2_RecvPriv cliPriv = {};
    IOC_DatUsageArgs_T cliDatArgs = {.CbRecvDat_F = __US2_CbRecv, .pCbPrivData = &cliPriv};
    IOC_ConnArgs_T cliConn = {.SrvURI = uri, .Usage = IOC_LinkUsageDatReceiver, .UsageArgs = {.pDat = &cliDatArgs}};

    IOC_LinkID_T cliLink = IOC_ID_INVALID;
    IOC_Result_T r = IOC_connectService(&cliLink, &cliConn, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, r);
    ASSERT_NE(IOC_ID_INVALID, cliLink);

    // Poll for service-side LinkID then send
    IOC_LinkID_T srvLinks[2] = {0};
    uint16_t actual = 0;
    for (int i = 0; i < 50; ++i) {  // up to 500ms
        r = IOC_getServiceLinkIDs(srv, srvLinks, 2, &actual);
        if (actual >= 1 && (r == IOC_RESULT_SUCCESS || r == IOC_RESULT_BUFFER_TOO_SMALL)) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    ASSERT_GE(actual, 1);
    IOC_LinkID_T srvLink = srvLinks[0];

    const char *msg = "US2-Poll: hello";
    IOC_DatDesc_T d = {0};
    IOC_initDatDesc(&d);
    d.Payload.pData = (void *)msg;
    d.Payload.PtrDataSize = strlen(msg);
    d.Payload.PtrDataLen = strlen(msg);
    r = IOC_sendDAT(srvLink, &d, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, r);
    IOC_flushDAT(srvLink, NULL);

    for (int i = 0; i < 60; ++i) {
        if (cliPriv.TotalReceivedSize.load() >= strlen(msg)) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    ASSERT_TRUE(cliPriv.CallbackExecuted.load());
    ASSERT_EQ((ULONG_T)strlen(msg), cliPriv.TotalReceivedSize.load());

    if (cliLink != IOC_ID_INVALID) IOC_closeLink(cliLink);
    if (srv != IOC_ID_INVALID) IOC_offlineService(srv);
}

// AC-2 (single client, typical data delivery with batching)
TEST(UT_DataTypicalAutoAccept, US2_TypicalSend_withBatchFlush_expectIntegrity) {
    IOC_SrvID_T srv = IOC_ID_INVALID;
    IOC_SrvURI_T uri = {.pProtocol = IOC_SRV_PROTO_FIFO,
                        .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
                        .pPath = (const char *)"US2_DatSender_Typical"};

    __US2_buildDatSenderService(uri, &srv, NULL, NULL);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    __US2_RecvPriv cliPriv = {};
    IOC_DatUsageArgs_T cliDatArgs = {.CbRecvDat_F = __US2_CbRecv, .pCbPrivData = &cliPriv};
    IOC_ConnArgs_T cliConn = {.SrvURI = uri, .Usage = IOC_LinkUsageDatReceiver, .UsageArgs = {.pDat = &cliDatArgs}};

    IOC_LinkID_T cliLink = IOC_ID_INVALID;
    IOC_Result_T r = IOC_connectService(&cliLink, &cliConn, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, r);

    // Discover server link
    IOC_LinkID_T srvLink = IOC_ID_INVALID;
    IOC_LinkID_T tmp[2] = {0};
    uint16_t n = 0;
    for (int i = 0; i < 50; ++i) {
        r = IOC_getServiceLinkIDs(srv, tmp, 2, &n);
        if (n >= 1) {
            srvLink = tmp[0];
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    ASSERT_NE(IOC_ID_INVALID, srvLink);

    // Send three chunks + single flush
    const char *c1 = "U2-Typ-1:";
    const char *c2 = "U2-Typ-2:";
    std::vector<char> c3(1024);
    for (int i = 0; i < 1024; ++i) c3[i] = (char)(i & 0xFF);
    auto sendOne = [&](const void *p, size_t sz) {
        IOC_DatDesc_T d = {0};
        IOC_initDatDesc(&d);
        d.Payload.pData = (void *)p;
        d.Payload.PtrDataSize = sz;
        d.Payload.PtrDataLen = sz;
        ASSERT_EQ(IOC_RESULT_SUCCESS, IOC_sendDAT(srvLink, &d, NULL));
    };
    sendOne(c1, strlen(c1));
    sendOne(c2, strlen(c2));
    sendOne(c3.data(), c3.size());
    IOC_flushDAT(srvLink, NULL);

    ULONG_T expect = (ULONG_T)strlen(c1) + (ULONG_T)strlen(c2) + (ULONG_T)c3.size();
    for (int i = 0; i < 80; ++i) {
        if (cliPriv.TotalReceivedSize.load() >= expect) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    ASSERT_TRUE(cliPriv.CallbackExecuted.load());
    ASSERT_EQ(expect, cliPriv.TotalReceivedSize.load());

    size_t off = 0;
    ASSERT_EQ(0, memcmp(cliPriv.ReceivedContent + off, c1, strlen(c1)));
    off += strlen(c1);
    ASSERT_EQ(0, memcmp(cliPriv.ReceivedContent + off, c2, strlen(c2)));
    off += strlen(c2);
    ASSERT_EQ(0, memcmp(cliPriv.ReceivedContent + off, c3.data(), c3.size()));

    if (cliLink != IOC_ID_INVALID) IOC_closeLink(cliLink);
    if (srv != IOC_ID_INVALID) IOC_offlineService(srv);
}

// Skeletons for more ACs (multi-client, large payload, reconnect); implement as needed
TEST(UT_DataTypicalAutoAccept, US2_MultiClientUnicast_placeholder) { GTEST_SKIP() << "Pending US-2 AC-3"; }
TEST(UT_DataTypicalAutoAccept, US2_LargePayload_placeholder) { GTEST_SKIP() << "Pending US-2 AC-4"; }
TEST(UT_DataTypicalAutoAccept, US2_ReconnectLifecycle_placeholder) { GTEST_SKIP() << "Pending US-2 AC-5"; }
