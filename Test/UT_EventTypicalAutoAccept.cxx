///////////////////////////////////////////////////////////////////////////////////////////////////
// Event Typical Auto-Accept (Conet) — dedicated UT
//
// Scope:
// - Only AUTO_ACCEPT behaviors for Conet events (no manual IOC_acceptClient here).
// - Keep EventTypical (UT_EventTypical.cxx) free of AUTO_ACCEPT per repo guidance.
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "_UT_IOC_Common.h"

// Minimal client-side receiver state
typedef struct __EvtRecvPrivAA {
    std::atomic<bool> Got{false};
    IOC_EvtID_T EvtID{0};
    ULONG_T EvtValue{0};
} __EvtRecvPrivAA_T;

static IOC_Result_T __EvtAA_ClientCb(const IOC_EvtDesc_pT pEvtDesc, void *pCbPriv) {
    __EvtRecvPrivAA_T *P = (__EvtRecvPrivAA_T *)pCbPriv;
    if (!P || !pEvtDesc) return IOC_RESULT_INVALID_PARAM;
    P->EvtID = IOC_EvtDesc_getEvtID((IOC_EvtDesc_pT)pEvtDesc);
    P->EvtValue = IOC_EvtDesc_getEvtValue((IOC_EvtDesc_pT)pEvtDesc);
    P->Got = true;
    return IOC_RESULT_SUCCESS;
}

// [Polling-path]
// GIVEN service as EvtProducer with AUTO_ACCEPT,
// WHEN a client (EvtConsumer) connects and subscribes,
// THEN service can discover accepted link via IOC_getServiceLinkIDs and post an event successfully.
TEST(UT_EventTypicalAutoAccept, US1_ServiceAsProducer_pollingPath_singleClient_expectDelivered) {
    IOC_Result_T R = IOC_RESULT_BUG;

    // Service online with AUTO_ACCEPT
    IOC_SrvURI_T SrvURI = {.pProtocol = IOC_SRV_PROTO_FIFO,
                           .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
                           .pPath = (const char *)"EvtTypicalAA_ProducerSingle"};
    IOC_SrvArgs_T SrvArgs = {.SrvURI = SrvURI,
                             .Flags = (IOC_SrvFlags_T)(IOC_SRVFLAG_AUTO_ACCEPT),
                             .UsageCapabilites = IOC_LinkUsageEvtProducer};
    IOC_SrvID_T SrvID = IOC_ID_INVALID;
    R = IOC_onlineService(&SrvID, &SrvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, R);

    // Client connect as consumer and subscribe
    IOC_ConnArgs_T ConnArgs = {.SrvURI = SrvURI, .Usage = IOC_LinkUsageEvtConsumer};
    IOC_LinkID_T CliLink = IOC_ID_INVALID;
    R = IOC_connectService(&CliLink, &ConnArgs, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, R);

    __EvtRecvPrivAA_T RecvPriv = {};
    static IOC_EvtID_T EIDs[1] = {IOC_EVTID_TEST_KEEPALIVE};
    IOC_SubEvtArgs_T Sub = {
        .CbProcEvt_F = __EvtAA_ClientCb, .pCbPrivData = &RecvPriv, .EvtNum = 1, .pEvtIDs = &EIDs[0]};
    R = IOC_subEVT(CliLink, &Sub);
    ASSERT_EQ(IOC_RESULT_SUCCESS, R);

    // Poll for accepted link at service
    IOC_LinkID_T SrvLinks[2] = {IOC_ID_INVALID, IOC_ID_INVALID};
    uint16_t actual = 0;
    for (int i = 0; i < 100; ++i) {
        R = IOC_getServiceLinkIDs(SrvID, SrvLinks, 2, &actual);
        if ((R == IOC_RESULT_SUCCESS || R == IOC_RESULT_BUFFER_TOO_SMALL) && actual >= 1) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    ASSERT_GE(actual, 1);
    IOC_LinkID_T SrvLink = SrvLinks[0];

    // Post event
    IOC_EvtDesc_T E = {};
    E.EvtID = IOC_EVTID_TEST_KEEPALIVE;
    E.EvtValue = 7;
    R = IOC_postEVT(SrvLink, &E, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, R);

    // Wait for callback
    for (int i = 0; i < 60; ++i) {
        if (RecvPriv.Got.load()) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    ASSERT_TRUE(RecvPriv.Got.load());
    ASSERT_EQ((ULONG_T)7, RecvPriv.EvtValue);

    if (CliLink != IOC_ID_INVALID) IOC_closeLink(CliLink);
    if (SrvID != IOC_ID_INVALID) IOC_offlineService(SrvID);
}
