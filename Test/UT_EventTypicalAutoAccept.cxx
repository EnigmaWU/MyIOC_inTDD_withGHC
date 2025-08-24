///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF OVERVIEW OF THIS UNIT TESTING FILE===============================================
/**
 * @brief
 *  VERIFICATIONS for Event Typical Auto-Accept (Conet) behaviors in IOC framework.
 *
 *-------------------------------------------------------------------------------------------------
 *++Context: Event communication with automatic client acceptance.
 *  This UTF focuses exclusively on AUTO_ACCEPT behaviors for Conet events,
 *  ensuring EventTypical (UT_EventTypical.cxx) remains free of AUTO_ACCEPT per repo guidance.
 *
 *  Key Concepts:
 *  - AUTO_ACCEPT flag enables automatic client acceptance without manual IOC_acceptClient
 *  - Service acts as event producer with automatic link management
 *  - Client acts as event consumer with subscription-based event reception
 *  - Polling mechanism for service to discover accepted client links
 *
 *  Core Functionality:
 *  - Service startup with AUTO_ACCEPT capability
 *  - Client connection and automatic acceptance
 *  - Event subscription and delivery verification
 *  - Link discovery and management
 */
//======>END OF OVERVIEW OF THIS UNIT TESTING FILE=================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF UNIT TESTING DESIGN==============================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF USER STORY=======================================================================
/**************************************************************************************************
 * @brief 【User Story】
 *
 *  US-1: AS a service developer using IOC framework,
 *    I WANT to enable automatic client acceptance for event services,
 *   SO THAT clients can connect and receive events without manual intervention.
 *
 *************************************************************************************************/
//======>END OF USER STORY=========================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//=======>BEGIN OF ACCEPTANCE CRITERIA=============================================================
/**************************************************************************************************
 * @brief 【Acceptance Criteria】
 *
 * [@US-1]
 *  AC-1: GIVEN a service with AUTO_ACCEPT flag and EvtProducer capability,
 *         WHEN a client connects as EvtConsumer and subscribes to events,
 *         THEN the service can discover the accepted link and deliver events successfully.
 *
 *************************************************************************************************/
//=======>END OF ACCEPTANCE CRITERIA================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF TEST CASES=======================================================================
/**************************************************************************************************
 * @brief 【Test Cases】
 *
 * [@AC-1,US-1]
 *  TC-1:
 *      @[Name]: verifyServiceAutoAccept_byPollingPath_expectEventDelivered
 *      @[Purpose]: Verify basic auto-accept functionality with polling-based link discovery
 *      @[Brief]: Service with AUTO_ACCEPT accepts client, discovers link via polling, and delivers events
 *
 *************************************************************************************************/
//======>END OF TEST CASES=========================================================================
//======>END OF UNIT TESTING DESIGN================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======BEGIN OF UNIT TESTING IMPLEMENTATION=======================================================
#include "_UT_IOC_Common.h"

// Minimal client-side receiver state for auto-accept testing
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

/**
 * ╔══════════════════════════════════════════════════════════════════════════════════════════╗
 * ║                                  📋 TEST CASE                                            ║
 * ╠══════════════════════════════════════════════════════════════════════════════════════════╣
 * ║ @[Name]: verifyServiceAutoAccept_byPollingPath_expectEventDelivered                      ║
 * ║ @[Steps]:                                                                                ║
 * ║   1) 🔧 Setup service with AUTO_ACCEPT flag and EvtProducer capability                  ║
 * ║   2) 🔧 Client connects as EvtConsumer and subscribes to KEEPALIVE events               ║
 * ║   3) 🎯 Service polls for accepted client links via IOC_getServiceLinkIDs               ║
 * ║   4) 🎯 Service posts KEEPALIVE event to discovered client link                         ║
 * ║   5) ✅ Verify client receives event with correct ID and value                          ║
 * ║   6) 🧹 Cleanup client link and offline service                                         ║
 * ║ @[Expect]: Client successfully receives event after auto-accept and polling discovery   ║
 * ║ @[Notes]: Tests polling-based link discovery pattern for auto-accepted clients          ║
 * ╚══════════════════════════════════════════════════════════════════════════════════════════╝
 */
TEST(UT_ConetEventTypical, verifyServiceAutoAccept_byPollingPath_expectEventDelivered) {
    // ┌──────────────────────────────────────────────────────────────────────────────────────┐
    // │                                🔧 SETUP PHASE                                        │
    // └──────────────────────────────────────────────────────────────────────────────────────┘
    printf("🔧 SETUP: Service with AUTO_ACCEPT and client connection\n");
    IOC_Result_T R = IOC_RESULT_BUG;

    // 1. Service online with AUTO_ACCEPT
    IOC_SrvURI_T SrvURI = {.pProtocol = IOC_SRV_PROTO_FIFO,
                           .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
                           .pPath = (const char *)"EvtTypicalAA_ProducerSingle"};
    IOC_SrvArgs_T SrvArgs = {.SrvURI = SrvURI,
                             .Flags = (IOC_SrvFlags_T)(IOC_SRVFLAG_AUTO_ACCEPT),
                             .UsageCapabilites = IOC_LinkUsageEvtProducer};
    IOC_SrvID_T SrvID = IOC_ID_INVALID;
    R = IOC_onlineService(&SrvID, &SrvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, R);

    // 2. Client connect as consumer and subscribe
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

    // ┌──────────────────────────────────────────────────────────────────────────────────────┐
    // │                               🎯 BEHAVIOR PHASE                                       │
    // └──────────────────────────────────────────────────────────────────────────────────────┘
    printf("🎯 BEHAVIOR: Service polling for auto-accepted client links\n");

    // 3. Poll for accepted link at service
    IOC_LinkID_T SrvLinks[2] = {IOC_ID_INVALID, IOC_ID_INVALID};
    uint16_t actual = 0;
    for (int i = 0; i < 100; ++i) {
        R = IOC_getServiceLinkIDs(SrvID, SrvLinks, 2, &actual);
        if ((R == IOC_RESULT_SUCCESS || R == IOC_RESULT_BUFFER_TOO_SMALL) && actual >= 1) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    ASSERT_GE(actual, 1);
    IOC_LinkID_T SrvLink = SrvLinks[0];

    // 4. Post event
    printf("📤 Service posting KEEPALIVE event to auto-accepted client\n");
    IOC_EvtDesc_T E = {};
    E.EvtID = IOC_EVTID_TEST_KEEPALIVE;
    E.EvtValue = 7;
    R = IOC_postEVT(SrvLink, &E, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, R);

    // ┌──────────────────────────────────────────────────────────────────────────────────────┐
    // │                                ✅ VERIFY PHASE                                        │
    // └──────────────────────────────────────────────────────────────────────────────────────┘
    printf("✅ VERIFY: Client receives event via auto-accept mechanism\n");

    // 5. Wait for callback
    for (int i = 0; i < 60; ++i) {
        if (RecvPriv.Got.load()) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    ASSERT_TRUE(RecvPriv.Got.load());
    ASSERT_EQ((ULONG_T)7, RecvPriv.EvtValue);
    ASSERT_EQ(IOC_EVTID_TEST_KEEPALIVE, RecvPriv.EvtID);

    printf("✅ AUTO-ACCEPT SUCCESS: Client received event (ID=%llu, Value=%lu)\n", (unsigned long long)RecvPriv.EvtID,
           (unsigned long)RecvPriv.EvtValue);

    // ┌──────────────────────────────────────────────────────────────────────────────────────┐
    // │                               🧹 CLEANUP PHASE                                        │
    // └──────────────────────────────────────────────────────────────────────────────────────┘
    if (CliLink != IOC_ID_INVALID) IOC_closeLink(CliLink);
    if (SrvID != IOC_ID_INVALID) IOC_offlineService(SrvID);
}
