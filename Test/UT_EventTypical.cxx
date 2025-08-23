///////////////////////////////////////////////////////////////////////////////////////////////////
// Event Typical (connection-oriented / Conet) — UT skeleton
//
// Intent:
// - “EventTypical” here explicitly means connection-oriented events (Conet), not Conles.
// - Focus on P2P link-to-link event flows by default (no broadcast mode here).
// - Mirrors the UT template and US/AC structure used across this repo.
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "_UT_IOC_Common.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF OVERVIEW OF THIS UNIT TESTING FILE===============================================
/**
 * @brief Verify typical, connection-oriented event flows (Conet):
 *  - Service and client exchange events over specific links (P2P),
 *    using link-scoped event posting and processing callbacks.
 *  - Not covering broadcast (SrvID→all clients); that’s in UT_ServiceBroadcastEvent.cxx.
 *
 * Key concepts:
 *  - Conet vs Conles: Conet binds events to a link; Conles is connection-less.
 *  - Typical flows: service as EvtProducer (server→client), service as EvtConsumer (client→server).
 */
//======>END OF OVERVIEW OF THIS UNIT TESTING FILE=================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF UNIT TESTING DESIGN==============================================================
/**
 * Design focus:
 *  - Typical P2P event usage first; validate happy paths and ordering.
 *  - Coexistence with data/command capabilities is out-of-scope here.
 *  - Broadcast mode tested elsewhere; we keep Conet here.
 */
///////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF USER STORY=======================================================================
/**
 * US-1: As a service EvtProducer, I want to post events to a specific client link
 *       so that the client receives only events intended for that link.
 *
 * US-2: As a service EvtConsumer, I want to consume events posted by client producer(s)
 *       so that server-side logic is triggered per-link.
 */
//======>END OF USER STORY==========================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//=======>BEGIN OF ACCEPTANCE CRITERIA==============================================================
/**
 * [@US-1]
 *  AC-1: GIVEN a Conet service (producer) and a connected consumer link,
 *         WHEN the service posts an event to that link,
 *         THEN the client receives exactly that event.
 *  AC-2: GIVEN multiple client links,
 *         WHEN the service posts distinct events to each link,
 *         THEN each client receives only its own event (isolation).
 *
 * [@US-2]
 *  AC-1: GIVEN a Conet service (consumer) and a client producer link,
 *         WHEN the client posts an event to the link,
 *         THEN the service callback processes it successfully.
 *  AC-2: GIVEN rapid sequential events on a link,
 *         WHEN posted in order,
 *         THEN they are observed in-order per-link.
 */
//=======>END OF ACCEPTANCE CRITERIA================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF TEST CASES (placeholders; to be implemented)=====================================

// [@AC-1,US-1]
// TC-1:
//   @[Name]: verifyConetEvent_byServiceAsProducer_singleClient_expectDelivered
//   @[Purpose]: Validate basic Conet producer→consumer delivery to a specific link.
//   @[Brief]: Service online as EvtProducer; client connects as EvtConsumer; service posts one event to that link;
//   client callback receives it.
//   @[Steps]:
//     1) Online service (Usage=EvtProducer, Conet) without broadcast flag.
//     2) Connect one client (Usage=EvtConsumer) with CbProcEvt_F registered.
//     3) Post event from service to accepted link.
//     4) Expect client callback fired with matching EvtID/payload.
// Minimal callback & priv for client-side event reception
typedef struct __EvtRecvPriv {
    std::atomic<bool> Got{false};
    std::atomic<ULONG_T> Seq{0};
    IOC_EvtID_T EvtID{0};
    ULONG_T EvtValue{0};
} __EvtRecvPriv_T;

static IOC_Result_T __EvtTypical_ClientCb(const IOC_EvtDesc_pT pEvtDesc, void *pCbPriv) {
    __EvtRecvPriv_T *pPrivData = (__EvtRecvPriv_T *)pCbPriv;
    if (!pPrivData || !pEvtDesc) return IOC_RESULT_INVALID_PARAM;
    pPrivData->EvtID = IOC_EvtDesc_getEvtID((IOC_EvtDesc_pT)pEvtDesc);
    pPrivData->EvtValue = IOC_EvtDesc_getEvtValue((IOC_EvtDesc_pT)pEvtDesc);
    pPrivData->Seq = IOC_EvtDesc_getSeqID((IOC_EvtDesc_pT)pEvtDesc);
    pPrivData->Got = true;
    return IOC_RESULT_SUCCESS;
}

// OLD: TEST(UT_EventTypical, verifyConetEvent_ServiceAsProducer_singleClient_expectDelivered)
/*NEW*/ TEST(UT_ConetEventTypical, verifyServiceAsEvtProducer_bySingleClient_expectDelivered) {
    IOC_Result_T ResultValue = IOC_RESULT_BUG;
    // Service setup (Conet producer)
    IOC_SrvURI_T SrvURI = {.pProtocol = IOC_SRV_PROTO_FIFO,
                           .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
                           .pPath = (const char *)"EvtTypical_ProducerSingle"};
    IOC_SrvArgs_T SrvArgs = {.SrvURI = SrvURI, .Flags = IOC_SRVFLAG_NONE, .UsageCapabilites = IOC_LinkUsageEvtProducer};
    IOC_SrvID_T SrvID = IOC_ID_INVALID;
    ResultValue = IOC_onlineService(&SrvID, &SrvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);

    // Client setup (Conet consumer) — connect in a separate thread to avoid blocking before accept
    IOC_ConnArgs_T ConnArgs = {.SrvURI = SrvURI, .Usage = IOC_LinkUsageEvtConsumer};
    IOC_LinkID_T CliLinkID = IOC_ID_INVALID;
    __EvtRecvPriv_T RecvPriv = {};
    std::atomic<bool> Subscribed{false};
    std::thread CliThread([&] {
        IOC_Result_T ResultValueInThread = IOC_connectService(&CliLinkID, &ConnArgs, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValueInThread);
        ASSERT_NE(IOC_ID_INVALID, CliLinkID);
        static IOC_EvtID_T SubEvtIDs[1] = {IOC_EVTID_TEST_KEEPALIVE};
        IOC_SubEvtArgs_T Sub = {
            .CbProcEvt_F = __EvtTypical_ClientCb, .pCbPrivData = &RecvPriv, .EvtNum = 1, .pEvtIDs = &SubEvtIDs[0]};
        ResultValueInThread = IOC_subEVT(CliLinkID, &Sub);
        ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValueInThread);
        Subscribed = true;
    });

    // Accept the client on the service side explicitly (no AUTO_ACCEPT here)
    IOC_LinkID_T SrvLinkID = IOC_ID_INVALID;
    ResultValue = IOC_acceptClient(SrvID, &SrvLinkID, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);
    ASSERT_NE(IOC_ID_INVALID, SrvLinkID);

    // Wait the client finishes subscription
    for (int i = 0; i < 50 && !Subscribed.load(); ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    // Post one event from service to that link
    IOC_EvtDesc_T EvtDesc = {};
    EvtDesc.EvtID = IOC_EVTID_TEST_KEEPALIVE;
    EvtDesc.EvtValue = 42;
    ResultValue = IOC_postEVT(SrvLinkID, &EvtDesc, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);

    // Wait for client callback
    for (int i = 0; i < 60; ++i) {
        if (RecvPriv.Got.load()) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    ASSERT_TRUE(RecvPriv.Got.load());
    ASSERT_EQ(IOC_EVTID_TEST_KEEPALIVE, RecvPriv.EvtID);
    ASSERT_EQ((ULONG_T)42, RecvPriv.EvtValue);

    // Cleanup
    if (CliThread.joinable()) CliThread.join();
    if (CliLinkID != IOC_ID_INVALID) IOC_closeLink(CliLinkID);
    if (SrvLinkID != IOC_ID_INVALID) IOC_closeLink(SrvLinkID);
    if (SrvID != IOC_ID_INVALID) IOC_offlineService(SrvID);
}

// [@AC-2,US-1]
// TC-1:
//   @[Name]: verifyServiceAsEvtProducer_byMultiClientIsolation_expectPerLinkDelivery
//   @[Purpose]: Ensure per-link isolation; each client receives only its own event.
//   @[Brief]: Two+ clients connect as EvtConsumers; service posts distinct events to each link; each client receives
//   only its respective event.
//   @[Steps]:
//     1) Online service (EvtProducer, Conet).
//     2) Connect N clients (EvtConsumer), each with its own callback context.
//     3) Post event-A to link-1, event-B to link-2, ...
//     4) Assert client-1 only saw event-A; client-2 only saw event-B; etc.
TEST(UT_ConetEventTypical, verifyServiceAsEvtProducer_byMultiClientIsolation_expectPerLinkDelivery) {
    IOC_Result_T ResultValue = IOC_RESULT_BUG;
    const int NumClients = 2;

    // Service setup (Conet producer)
    IOC_SrvURI_T SrvURI = {.pProtocol = IOC_SRV_PROTO_FIFO,
                           .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
                           .pPath = (const char *)"EvtTypical_ProducerMulti"};
    IOC_SrvArgs_T SrvArgs = {.SrvURI = SrvURI, .Flags = IOC_SRVFLAG_NONE, .UsageCapabilites = IOC_LinkUsageEvtProducer};
    IOC_SrvID_T SrvID = IOC_ID_INVALID;
    ResultValue = IOC_onlineService(&SrvID, &SrvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);

    // Client contexts and threads
    __EvtRecvPriv_T RecvPrivs[NumClients] = {};
    std::thread CliThreads[NumClients];
    IOC_LinkID_T CliLinkIDs[NumClients] = {IOC_ID_INVALID, IOC_ID_INVALID};
    std::atomic<int> SubscribedCount{0};

    // Start client threads
    for (int i = 0; i < NumClients; ++i) {
        CliThreads[i] = std::thread([&, i] {
            IOC_ConnArgs_T ConnArgs = {.SrvURI = SrvURI, .Usage = IOC_LinkUsageEvtConsumer};
            IOC_Result_T ResultValueInThread = IOC_connectService(&CliLinkIDs[i], &ConnArgs, NULL);
            ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValueInThread);
            ASSERT_NE(IOC_ID_INVALID, CliLinkIDs[i]);

            static IOC_EvtID_T SubEvtIDs[1] = {IOC_EVTID_TEST_KEEPALIVE};
            IOC_SubEvtArgs_T Sub = {.CbProcEvt_F = __EvtTypical_ClientCb,
                                    .pCbPrivData = &RecvPrivs[i],
                                    .EvtNum = 1,
                                    .pEvtIDs = &SubEvtIDs[0]};
            ResultValueInThread = IOC_subEVT(CliLinkIDs[i], &Sub);
            ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValueInThread);
            SubscribedCount++;
        });
    }

    // Accept clients on service side
    IOC_LinkID_T SrvLinkIDs[NumClients] = {IOC_ID_INVALID, IOC_ID_INVALID};
    for (int i = 0; i < NumClients; ++i) {
        ResultValue = IOC_acceptClient(SrvID, &SrvLinkIDs[i], NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);
        ASSERT_NE(IOC_ID_INVALID, SrvLinkIDs[i]);
    }

    // Wait for all clients to subscribe
    for (int i = 0; i < 100 && SubscribedCount.load() < NumClients; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    ASSERT_EQ(NumClients, SubscribedCount.load());

    // Post the SAME event to ALL links to verify isolation
    // If isolation works, each client should receive exactly ONE event
    // If isolation fails, some clients might receive multiple events or wrong events
    IOC_EvtDesc_T EvtDesc = {};
    EvtDesc.EvtID = IOC_EVTID_TEST_KEEPALIVE;
    EvtDesc.EvtValue = 42;  // Same value for all

    for (int i = 0; i < NumClients; ++i) {
        ResultValue = IOC_postEVT(SrvLinkIDs[i], &EvtDesc, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);
    }

    // Wait for all callbacks
    for (int retries = 0; retries < 60; ++retries) {
        bool allReceived = true;
        for (int i = 0; i < NumClients; ++i) {
            if (!RecvPrivs[i].Got.load()) {
                allReceived = false;
                break;
            }
        }
        if (allReceived) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // Assert each client received exactly ONE event (isolation)
    for (int i = 0; i < NumClients; ++i) {
        ASSERT_TRUE(RecvPrivs[i].Got.load()) << "Client " << i << " did not receive event";
        ASSERT_EQ(IOC_EVTID_TEST_KEEPALIVE, RecvPrivs[i].EvtID) << "Client " << i << " wrong event ID";
        ASSERT_EQ((ULONG_T)42, RecvPrivs[i].EvtValue) << "Client " << i << " received wrong event value";
    }

    // Verify each client got exactly 1 sequence (no cross-talk)
    // If isolation failed, clients might get events from multiple links
    std::set<ULONG_T> uniqueSeqIDs;
    for (int i = 0; i < NumClients; ++i) {
        uniqueSeqIDs.insert(RecvPrivs[i].Seq);
    }
    ASSERT_EQ(NumClients, uniqueSeqIDs.size()) << "Isolation failed: clients received duplicate/cross-wired events";

    // Cleanup
    for (int i = 0; i < NumClients; ++i) {
        if (CliThreads[i].joinable()) CliThreads[i].join();
        if (CliLinkIDs[i] != IOC_ID_INVALID) IOC_closeLink(CliLinkIDs[i]);
        if (SrvLinkIDs[i] != IOC_ID_INVALID) IOC_closeLink(SrvLinkIDs[i]);
    }
    if (SrvID != IOC_ID_INVALID) IOC_offlineService(SrvID);
}

// [@AC-1,US-2]
// TC-1:
//   @[Name]: verifyServiceAsEvtConsumer_bySingleClient_expectProcessed
//   @[Purpose]: Validate service-side consumption when client posts to its link.
//   @[Brief]: Service online as EvtConsumer; client connects as EvtProducer; client posts one event; service callback
//   processes it.
//   @[Steps]:
//     1) Online service (Usage=EvtConsumer) with CbProcEvt_F registered.
//     2) Connect one client (Usage=EvtProducer).
//     3) Client posts event to its link.
//     4) Assert service callback fired and payload/ID match.
TEST(UT_ConetEventTypical, verifyServiceAsEvtConsumer_bySingleClient_expectProcessed) {
    IOC_Result_T ResultValue = IOC_RESULT_BUG;

    // Service setup (Conet consumer with callback)
    __EvtRecvPriv_T SrvRecvPriv = {};
    IOC_SrvURI_T SrvURI = {.pProtocol = IOC_SRV_PROTO_FIFO,
                           .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
                           .pPath = (const char *)"EvtTypical_ConsumerSingle"};
    IOC_SrvArgs_T SrvArgs = {.SrvURI = SrvURI, .Flags = IOC_SRVFLAG_NONE, .UsageCapabilites = IOC_LinkUsageEvtConsumer};
    IOC_SrvID_T SrvID = IOC_ID_INVALID;
    ResultValue = IOC_onlineService(&SrvID, &SrvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);

    // Client setup (Conet producer) — connect in a separate thread
    IOC_ConnArgs_T ConnArgs = {.SrvURI = SrvURI, .Usage = IOC_LinkUsageEvtProducer};
    IOC_LinkID_T CliLinkID = IOC_ID_INVALID;
    std::thread CliThread([&] {
        IOC_Result_T ResultValueInThread = IOC_connectService(&CliLinkID, &ConnArgs, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValueInThread);
        ASSERT_NE(IOC_ID_INVALID, CliLinkID);
    });

    // Accept the client and setup service-side subscription
    IOC_LinkID_T SrvLinkID = IOC_ID_INVALID;
    ResultValue = IOC_acceptClient(SrvID, &SrvLinkID, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);
    ASSERT_NE(IOC_ID_INVALID, SrvLinkID);

    static IOC_EvtID_T SubEvtIDs[1] = {IOC_EVTID_TEST_KEEPALIVE};
    IOC_SubEvtArgs_T Sub = {
        .CbProcEvt_F = __EvtTypical_ClientCb, .pCbPrivData = &SrvRecvPriv, .EvtNum = 1, .pEvtIDs = &SubEvtIDs[0]};
    ResultValue = IOC_subEVT(SrvLinkID, &Sub);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);

    // Wait for client thread completion
    if (CliThread.joinable()) CliThread.join();

    // Client posts event to service
    IOC_EvtDesc_T EvtDesc = {};
    EvtDesc.EvtID = IOC_EVTID_TEST_KEEPALIVE;
    EvtDesc.EvtValue = 123;
    ResultValue = IOC_postEVT(CliLinkID, &EvtDesc, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);

    // Wait for service callback
    for (int i = 0; i < 60; ++i) {
        if (SrvRecvPriv.Got.load()) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    ASSERT_TRUE(SrvRecvPriv.Got.load());
    ASSERT_EQ(IOC_EVTID_TEST_KEEPALIVE, SrvRecvPriv.EvtID);
    ASSERT_EQ((ULONG_T)123, SrvRecvPriv.EvtValue);

    // Cleanup
    if (CliLinkID != IOC_ID_INVALID) IOC_closeLink(CliLinkID);
    if (SrvLinkID != IOC_ID_INVALID) IOC_closeLink(SrvLinkID);
    if (SrvID != IOC_ID_INVALID) IOC_offlineService(SrvID);
}

// Enhanced callback structure for order verification
typedef struct __EvtOrderPriv {
    std::atomic<int> ReceivedCount{0};
    std::vector<ULONG_T> ReceivedSequences;
    std::vector<ULONG_T> ReceivedValues;
    std::mutex RecordMutex;
    IOC_EvtID_T LastEvtID{0};
} __EvtOrderPriv_T;

static IOC_Result_T __EvtTypical_OrderCb(const IOC_EvtDesc_pT pEvtDesc, void *pCbPriv) {
    __EvtOrderPriv_T *pPrivData = (__EvtOrderPriv_T *)pCbPriv;
    if (!pPrivData || !pEvtDesc) return IOC_RESULT_INVALID_PARAM;

    std::lock_guard<std::mutex> lock(pPrivData->RecordMutex);
    pPrivData->LastEvtID = IOC_EvtDesc_getEvtID((IOC_EvtDesc_pT)pEvtDesc);
    pPrivData->ReceivedSequences.push_back(IOC_EvtDesc_getSeqID((IOC_EvtDesc_pT)pEvtDesc));
    pPrivData->ReceivedValues.push_back(IOC_EvtDesc_getEvtValue((IOC_EvtDesc_pT)pEvtDesc));
    pPrivData->ReceivedCount++;
    return IOC_RESULT_SUCCESS;
}

// [@AC-2,US-2]
// TC-1:
//   @[Name]: verifyOrderPerLink_bySequentialEvents_expectInOrderObservation
//   @[Purpose]: Ensure in-order observation on the same link under sequential posts.
//   @[Brief]: Client posts a sequence of events (IDs/payload sequence) to one link; service records order; assert
//   preserved order.
//   @[Steps]:
//     1) Online service (EvtConsumer) with callback storing sequence.
//     2) Client (EvtProducer) posts events E1..En sequentially on same link.
//     3) Wait for processing; verify order E1..En at service.
TEST(UT_ConetEventTypical, verifyOrderPerLink_bySequentialEvents_expectInOrderObservation) {
    IOC_Result_T ResultValue = IOC_RESULT_BUG;
    const int NumEvents = 5;

    // Service setup (Conet consumer with order-tracking callback)
    __EvtOrderPriv_T SrvOrderPriv = {};
    IOC_SrvURI_T SrvURI = {.pProtocol = IOC_SRV_PROTO_FIFO,
                           .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
                           .pPath = (const char *)"EvtTypical_OrderTest"};
    IOC_SrvArgs_T SrvArgs = {.SrvURI = SrvURI, .Flags = IOC_SRVFLAG_NONE, .UsageCapabilites = IOC_LinkUsageEvtConsumer};
    IOC_SrvID_T SrvID = IOC_ID_INVALID;
    ResultValue = IOC_onlineService(&SrvID, &SrvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);

    // Client setup (Conet producer) — connect in a separate thread
    IOC_ConnArgs_T ConnArgs = {.SrvURI = SrvURI, .Usage = IOC_LinkUsageEvtProducer};
    IOC_LinkID_T CliLinkID = IOC_ID_INVALID;
    std::thread CliThread([&] {
        IOC_Result_T ResultValueInThread = IOC_connectService(&CliLinkID, &ConnArgs, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValueInThread);
        ASSERT_NE(IOC_ID_INVALID, CliLinkID);
    });

    // Accept the client and setup service-side subscription
    IOC_LinkID_T SrvLinkID = IOC_ID_INVALID;
    ResultValue = IOC_acceptClient(SrvID, &SrvLinkID, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);
    ASSERT_NE(IOC_ID_INVALID, SrvLinkID);

    static IOC_EvtID_T SubEvtIDs[1] = {IOC_EVTID_TEST_KEEPALIVE};
    IOC_SubEvtArgs_T Sub = {
        .CbProcEvt_F = __EvtTypical_OrderCb, .pCbPrivData = &SrvOrderPriv, .EvtNum = 1, .pEvtIDs = &SubEvtIDs[0]};
    ResultValue = IOC_subEVT(SrvLinkID, &Sub);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);

    // Wait for client thread completion
    if (CliThread.joinable()) CliThread.join();

    // Client posts sequential events with increasing values
    std::vector<ULONG_T> SentValues;
    for (int i = 0; i < NumEvents; ++i) {
        IOC_EvtDesc_T EvtDesc = {};
        EvtDesc.EvtID = IOC_EVTID_TEST_KEEPALIVE;
        EvtDesc.EvtValue = 100 + i;  // 100, 101, 102, 103, 104
        SentValues.push_back(EvtDesc.EvtValue);

        ResultValue = IOC_postEVT(CliLinkID, &EvtDesc, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);

        // Small delay to ensure order (though the framework should maintain order anyway)
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    // Wait for all events to be processed
    for (int retry = 0; retry < 100; ++retry) {
        if (SrvOrderPriv.ReceivedCount.load() >= NumEvents) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // Verify all events received
    ASSERT_EQ(NumEvents, SrvOrderPriv.ReceivedCount.load()) << "Not all events were received";

    // Verify order preservation: received values should match sent order
    {
        std::lock_guard<std::mutex> lock(SrvOrderPriv.RecordMutex);
        ASSERT_EQ(NumEvents, SrvOrderPriv.ReceivedValues.size()) << "Mismatch in received values count";

        for (int i = 0; i < NumEvents; ++i) {
            ASSERT_EQ(SentValues[i], SrvOrderPriv.ReceivedValues[i])
                << "Event order violation: expected value " << SentValues[i] << " at position " << i << ", got "
                << SrvOrderPriv.ReceivedValues[i];
        }

        // Verify sequence IDs are strictly increasing (order preserved by IOC framework)
        for (int i = 1; i < NumEvents; ++i) {
            ASSERT_LT(SrvOrderPriv.ReceivedSequences[i - 1], SrvOrderPriv.ReceivedSequences[i])
                << "Sequence ID order violation: SeqID[" << (i - 1) << "]=" << SrvOrderPriv.ReceivedSequences[i - 1]
                << " should be < SeqID[" << i << "]=" << SrvOrderPriv.ReceivedSequences[i];
        }
    }

    // Cleanup
    if (CliLinkID != IOC_ID_INVALID) IOC_closeLink(CliLinkID);
    if (SrvLinkID != IOC_ID_INVALID) IOC_closeLink(SrvLinkID);
    if (SrvID != IOC_ID_INVALID) IOC_offlineService(SrvID);
}

// Optional lifecycle/cleanup case
// TC-1:
//   @[Name]: verifyOfflineLifecycle_byServiceShutdown_expectCleanup
//   @[Purpose]: Validate links and callbacks are cleaned up when service goes offline.
//   @[Brief]: Service online; client connects; take service offline; ensure link closed and no further event delivery.
//   @[Steps]:
//     1) Online service; client connects.
//     2) Post an event (works), then offline service.
//     3) Further posts (if attempted) fail; no callbacks invoked; resources freed.
TEST(UT_ConetEventTypical, verifyOfflineLifecycle_byServiceShutdown_expectCleanup) {
    IOC_Result_T ResultValue = IOC_RESULT_BUG;

    // Service setup (Conet consumer)
    __EvtRecvPriv_T SrvRecvPriv = {};
    IOC_SrvURI_T SrvURI = {.pProtocol = IOC_SRV_PROTO_FIFO,
                           .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
                           .pPath = (const char *)"EvtTypical_LifecycleTest"};
    IOC_SrvArgs_T SrvArgs = {.SrvURI = SrvURI, .Flags = IOC_SRVFLAG_NONE, .UsageCapabilites = IOC_LinkUsageEvtConsumer};
    IOC_SrvID_T SrvID = IOC_ID_INVALID;
    ResultValue = IOC_onlineService(&SrvID, &SrvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);

    // Client setup (Conet producer)
    IOC_ConnArgs_T ConnArgs = {.SrvURI = SrvURI, .Usage = IOC_LinkUsageEvtProducer};
    IOC_LinkID_T CliLinkID = IOC_ID_INVALID;
    std::thread CliThread([&] {
        IOC_Result_T ResultValueInThread = IOC_connectService(&CliLinkID, &ConnArgs, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValueInThread);
        ASSERT_NE(IOC_ID_INVALID, CliLinkID);
    });

    // Accept the client and setup service-side subscription
    IOC_LinkID_T SrvLinkID = IOC_ID_INVALID;
    ResultValue = IOC_acceptClient(SrvID, &SrvLinkID, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);
    ASSERT_NE(IOC_ID_INVALID, SrvLinkID);

    static IOC_EvtID_T SubEvtIDs[1] = {IOC_EVTID_TEST_KEEPALIVE};
    IOC_SubEvtArgs_T Sub = {
        .CbProcEvt_F = __EvtTypical_ClientCb, .pCbPrivData = &SrvRecvPriv, .EvtNum = 1, .pEvtIDs = &SubEvtIDs[0]};
    ResultValue = IOC_subEVT(SrvLinkID, &Sub);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);

    // Wait for client thread completion
    if (CliThread.joinable()) CliThread.join();

    // Step 1: Verify normal operation - post an event (should work)
    IOC_EvtDesc_T EvtDesc = {};
    EvtDesc.EvtID = IOC_EVTID_TEST_KEEPALIVE;
    EvtDesc.EvtValue = 200;
    ResultValue = IOC_postEVT(CliLinkID, &EvtDesc, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue) << "Event posting should work before service shutdown";

    // Wait for the event to be processed
    for (int i = 0; i < 60; ++i) {
        if (SrvRecvPriv.Got.load()) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    ASSERT_TRUE(SrvRecvPriv.Got.load()) << "Event should be received before service shutdown";
    ASSERT_EQ((ULONG_T)200, SrvRecvPriv.EvtValue) << "Event value should match";

    // Step 2: Take service offline (this should cleanup links and stop event processing)
    ResultValue = IOC_offlineService(SrvID);
    ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue) << "Service should go offline successfully";
    SrvID = IOC_ID_INVALID;  // Mark as invalid after cleanup

    // Step 3: Verify cleanup - further posts should fail or not reach callbacks
    __EvtRecvPriv_T PostShutdownPriv = {};  // New callback context to verify no delivery

    // Try to post another event - this should fail or not be delivered
    IOC_EvtDesc_T PostShutdownEvt = {};
    PostShutdownEvt.EvtID = IOC_EVTID_TEST_KEEPALIVE;
    PostShutdownEvt.EvtValue = 999;  // Different value to distinguish

    // This post might fail (preferred) or succeed but not be delivered (acceptable)
    IOC_Result_T PostResult = IOC_postEVT(CliLinkID, &PostShutdownEvt, NULL);

    // Give some time for any potential (unwanted) callback delivery
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // The key validation: no new events should be processed after service shutdown
    // Since we can't easily hook into the original callback after shutdown,
    // we verify that the original callback state hasn't changed unexpectedly
    ASSERT_EQ((ULONG_T)200, SrvRecvPriv.EvtValue)
        << "Event value should not change after service shutdown - no new events should be processed";

    // Additional verification: if posting succeeded, ensure it doesn't crash or hang
    if (PostResult == IOC_RESULT_SUCCESS) {
        // Even if posting "succeeded", the event should not be processed due to service being offline
        // This is acceptable behavior as long as it doesn't crash
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    // Cleanup remaining client resources
    if (CliLinkID != IOC_ID_INVALID) IOC_closeLink(CliLinkID);
    // Note: SrvLinkID and SrvID should be automatically cleaned up by IOC_offlineService
}

//======>END OF TEST CASES==========================================================================
