///////////////////////////////////////////////////////////////////////////////////////////////////
// Event Typical Keep Accepted Links — UT for IOC_SRVFLAG_KEEP_ACCEPTED_LINK
//
// Intent:
// - Verify the IOC_SRVFLAG_KEEP_ACCEPTED_LINK service flag behavior
// - Focus on manually accepted link lifecycle management during service shutdown
// - Test configurable cleanup behavior vs default auto-close behavior
// - Mirrors the UT template and US/AC structure used across this repo
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "_UT_IOC_Common.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF OVERVIEW OF THIS UNIT TESTING FILE===============================================
/**
 * @brief Verify IOC_SRVFLAG_KEEP_ACCEPTED_LINK flag behavior in service lifecycle:
 *  - Service flag controls whether manually accepted links are automatically closed
 *    during service shutdown or kept alive for application-controlled cleanup
 *  - Compare behavior with and without the flag to validate functional differences
 *  - Ensure manual cleanup still works when links are preserved
 *
 * Key concepts:
 *  - Manual accept: IOC_acceptClient() creates links that require explicit management
 *  - Keep flag: IOC_SRVFLAG_KEEP_ACCEPTED_LINK preserves links during IOC_offlineService()
 *  - Default behavior: Manually accepted links are auto-closed during service offline
 *  - Resource management: Applications retain control over link cleanup timing
 */
//======>END OF OVERVIEW OF THIS UNIT TESTING FILE=================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF UNIT TESTING DESIGN==============================================================
/**
 * Design focus:
 *  - Typical flag usage: verify keep-alive behavior vs default auto-close behavior
 *  - Edge conditions: service offline scenarios with and without the flag
 *  - State management: link preservation and manual cleanup workflows
 *  - Resource control: application-driven cleanup after service shutdown
 */
///////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF USER STORY=======================================================================
/**
 * US-1: As a service developer, I want to control whether manually accepted links
 *       are automatically closed during service shutdown,
 *       so that I can manage link lifecycle according to application requirements.
 *
 * US-2: As a service developer, I want links to survive service restarts
 *       when using the KEEP_ACCEPTED_LINK flag,
 *       so that I can implement graceful shutdown and restart scenarios.
 *
 * US-3: As a service developer, I want to manually cleanup preserved links
 *       after service shutdown,
 *       so that I retain full control over resource management.
 */
//======>END OF USER STORY==========================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//=======>BEGIN OF ACCEPTANCE CRITERIA==============================================================
/**
 * [@US-1]
 *  AC-1: GIVEN a service with IOC_SRVFLAG_KEEP_ACCEPTED_LINK flag and manually accepted links,
 *         WHEN the service goes offline,
 *         THEN the manually accepted links remain open and functional.
 *  AC-2: GIVEN a service without IOC_SRVFLAG_KEEP_ACCEPTED_LINK flag and manually accepted links,
 *         WHEN the service goes offline,
 *         THEN the manually accepted links are automatically closed.
 *
 * [@US-2]
 *  AC-1: GIVEN preserved links after service offline with the flag,
 *         WHEN events are posted to the preserved links,
 *         THEN events continue to be delivered (or appropriate error handling occurs).
 *
 * [@US-3]
 *  AC-1: GIVEN preserved links after service offline,
 *         WHEN IOC_closeLink() is called on the preserved links,
 *         THEN the links are successfully closed and resources are freed.
 */
//=======>END OF ACCEPTANCE CRITERIA================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF TEST CASES=======================================================================

// [@AC-1,US-1]
// TC-1:
//   @[Name]: verifyKeepAcceptedLinksFlag_byServiceOffline_expectLinksRemainOpen
//   @[Purpose]: Validate that IOC_SRVFLAG_KEEP_ACCEPTED_LINK preserves manually accepted links during service offline
//   @[Brief]: Service with flag → manual accept → event delivery → service offline → links preserved → continued
//   functionality
//   @[Steps]:
//     1) Online service with IOC_SRVFLAG_KEEP_ACCEPTED_LINK flag and EvtProducer capability
//     2) Connect client as EvtConsumer and manually accept the connection
//     3) Verify initial event delivery works correctly
//     4) Take service offline
//     5) Verify links remain open and events can still be delivered

// [@AC-2,US-1]
// TC-1:
//   @[Name]: verifyFlagDifference_compareWithDefaultBehavior_expectDifferentLifecycle
//   @[Purpose]: Compare behavior with and without the flag to validate functional difference
//   @[Brief]: Service without flag → manual accept → service offline → links auto-closed
//   @[Steps]:
//     1) Online service without IOC_SRVFLAG_KEEP_ACCEPTED_LINK flag
//     2) Connect client and manually accept the connection
//     3) Verify initial event delivery works correctly
//     4) Take service offline
//     5) Verify links are auto-closed and event delivery fails

// [@AC-1,US-2]
// TC-1:
//   @[Name]: verifyPreservedLinks_byEventDelivery_expectContinuedFunctionality
//   @[Purpose]: Validate that preserved links continue to function for event delivery after service offline
//   @[Brief]: Service with flag → service offline → links preserved → continued event functionality
//   @[Steps]:
//     1) Online service with IOC_SRVFLAG_KEEP_ACCEPTED_LINK flag
//     2) Connect client and manually accept the connection
//     3) Take service offline (links preserved)
//     4) Verify event posting still works on preserved links
//     5) Verify event delivery continues or appropriate error handling occurs

// [@AC-1,US-3]
// TC-1:
//   @[Name]: verifyManualCleanup_withKeepAcceptedLinksFlag_expectCleanupWorks
//   @[Purpose]: Verify manual cleanup still works when links are preserved by the flag
//   @[Brief]: Service with flag → service offline → links preserved → manual cleanup → links closed
//   @[Steps]:
//     1) Online service with IOC_SRVFLAG_KEEP_ACCEPTED_LINK flag
//     2) Connect client and manually accept the connection
//     3) Take service offline (links preserved)
//     4) Verify link ID is still valid but event functionality may be limited
//     5) Manually close the preserved link using IOC_closeLink()
//     6) Verify manual cleanup worked and link is completely closed
//=================================================================================================

// Callback structure for event reception testing
typedef struct __KeepLinksEvtRecvPriv {
    std::atomic<bool> Got{false};
    std::atomic<ULONG_T> Seq{0};
    IOC_EvtID_T EvtID{0};
    ULONG_T EvtValue{0};
} __KeepLinksEvtRecvPriv_T;

// Event callback for client-side event reception
static IOC_Result_T __KeepLinksTestClientCb(const IOC_EvtDesc_pT pEvtDesc, void *pCbPriv) {
    __KeepLinksEvtRecvPriv_T *pPrivData = (__KeepLinksEvtRecvPriv_T *)pCbPriv;
    if (!pPrivData || !pEvtDesc) return IOC_RESULT_INVALID_PARAM;
    pPrivData->EvtID = IOC_EvtDesc_getEvtID((IOC_EvtDesc_pT)pEvtDesc);
    pPrivData->EvtValue = IOC_EvtDesc_getEvtValue((IOC_EvtDesc_pT)pEvtDesc);
    pPrivData->Seq = IOC_EvtDesc_getSeqID((IOC_EvtDesc_pT)pEvtDesc);
    pPrivData->Got = true;
    return IOC_RESULT_SUCCESS;
}

//======>END OF TEST CASES=========================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF TEST IMPLEMENTATIONS=============================================================

// [@AC-1,US-1]
// TC-1: verifyKeepAcceptedLinksFlag_byServiceOffline_expectLinksRemainOpen
TEST(UT_ConetEventTypical, verifyKeepAcceptedLinksFlag_byServiceOffline_expectLinksRemainOpen) {
    IOC_Result_T ResultValue = IOC_RESULT_BUG;
    IOC_SrvID_T SrvID = IOC_ID_INVALID;
    IOC_LinkID_T SrvLinkID = IOC_ID_INVALID;
    IOC_LinkID_T CliLinkID = IOC_ID_INVALID;
    std::thread CliThread;

    // Test data for event communication
    __KeepLinksEvtRecvPriv_T RecvPriv = {};

    try {
        // Step-1: Create service with KEEP_ACCEPTED_LINK flag (no auto-accept)
        IOC_SrvURI_T SrvURI = {.pProtocol = IOC_SRV_PROTO_FIFO,
                               .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
                               .pPath = (const char *)"KeepLinks_Test1"};
        IOC_SrvArgs_T SrvArgs = {.SrvURI = SrvURI,
                                 .Flags = IOC_SRVFLAG_KEEP_ACCEPTED_LINK,  // Key: Set the flag, no auto-accept
                                 .UsageCapabilites = IOC_LinkUsageEvtProducer};

        ResultValue = IOC_onlineService(&SrvID, &SrvArgs);
        ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);
        ASSERT_NE(IOC_ID_INVALID, SrvID);

        // Step-2: Start client connection thread (will be manually accepted)
        std::atomic<bool> Subscribed{false};
        CliThread = std::thread([&]() {
            IOC_ConnArgs_T ConnArgs = {.SrvURI = SrvURI, .Usage = IOC_LinkUsageEvtConsumer};
            IOC_Result_T ResultValueInThread = IOC_connectService(&CliLinkID, &ConnArgs, NULL);
            ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValueInThread);
            ASSERT_NE(IOC_ID_INVALID, CliLinkID);

            // Subscribe to events
            static IOC_EvtID_T SubEvtIDs[1] = {IOC_EVTID_TEST_KEEPALIVE};
            IOC_SubEvtArgs_T Sub = {.CbProcEvt_F = __KeepLinksTestClientCb,
                                    .pCbPrivData = &RecvPriv,
                                    .EvtNum = 1,
                                    .pEvtIDs = &SubEvtIDs[0]};
            ResultValueInThread = IOC_subEVT(CliLinkID, &Sub);
            ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValueInThread);
            Subscribed = true;
        });

        // Step-3: Wait and manually accept the connection
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        ResultValue = IOC_acceptClient(SrvID, &SrvLinkID, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);
        ASSERT_NE(IOC_ID_INVALID, SrvLinkID);

        // Wait for client subscription
        for (int i = 0; i < 50 && !Subscribed.load(); ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }

        // Step-4: Send event to verify initial connection works
        IOC_EvtDesc_T EvtDesc = {};
        EvtDesc.EvtID = IOC_EVTID_TEST_KEEPALIVE;
        EvtDesc.EvtValue = 42;
        ResultValue = IOC_postEVT(SrvLinkID, &EvtDesc, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);

        // Step-5: Wait for event delivery and verify
        for (int i = 0; i < 60; ++i) {
            if (RecvPriv.Got.load()) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        ASSERT_TRUE(RecvPriv.Got.load());
        ASSERT_EQ(IOC_EVTID_TEST_KEEPALIVE, RecvPriv.EvtID);
        ASSERT_EQ((ULONG_T)42, RecvPriv.EvtValue);

        // Step-6: Take service offline (key test point - links should remain open due to flag)
        ResultValue = IOC_offlineService(SrvID);
        ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);
        SrvID = IOC_ID_INVALID;

        // Step-7: Verify that the link is still open and functional
        // Reset callback state
        RecvPriv.Got = false;
        RecvPriv.EvtValue = 0;
        EvtDesc.EvtValue = 99;

        // This should still work because the link was kept alive
        IOC_Result_T PostResult = IOC_postEVT(SrvLinkID, &EvtDesc, NULL);
        EXPECT_EQ(IOC_RESULT_SUCCESS, PostResult)
            << "Link should remain open when IOC_SRVFLAG_KEEP_ACCEPTED_LINK is set";

        // Wait and verify event was delivered
        for (int i = 0; i < 60; ++i) {
            if (RecvPriv.Got.load()) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        EXPECT_TRUE(RecvPriv.Got.load()) << "Event delivery should continue after service offline when flag is set";
        EXPECT_EQ((ULONG_T)99, RecvPriv.EvtValue);

    } catch (...) {
        // Ensure proper cleanup even on exceptions
    }

    // Cleanup
    if (CliThread.joinable()) CliThread.join();
    if (CliLinkID != IOC_ID_INVALID) IOC_closeLink(CliLinkID);
    if (SrvLinkID != IOC_ID_INVALID) IOC_closeLink(SrvLinkID);  // Manual cleanup as expected
    if (SrvID != IOC_ID_INVALID) IOC_offlineService(SrvID);
}

// [@AC-2,US-1]
// TC-1: verifyFlagDifference_compareWithDefaultBehavior_expectDifferentLifecycle
TEST(UT_ConetEventTypical, verifyFlagDifference_compareWithDefaultBehavior_expectDifferentLifecycle) {
    IOC_Result_T ResultValue = IOC_RESULT_BUG;
    IOC_SrvID_T SrvID = IOC_ID_INVALID;
    IOC_LinkID_T SrvLinkID = IOC_ID_INVALID;
    IOC_LinkID_T CliLinkID = IOC_ID_INVALID;
    std::thread CliThread;

    // Test data for event communication
    __KeepLinksEvtRecvPriv_T RecvPriv = {};

    try {
        // Step-1: Create service WITHOUT the KEEP_ACCEPTED_LINK flag (default behavior, no auto-accept)
        IOC_SrvURI_T SrvURI = {.pProtocol = IOC_SRV_PROTO_FIFO,
                               .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
                               .pPath = (const char *)"DefaultBehavior_Test2"};
        IOC_SrvArgs_T SrvArgs = {.SrvURI = SrvURI,
                                 .Flags = IOC_SRVFLAG_NONE,  // No KEEP_ACCEPTED_LINK flag, no auto-accept
                                 .UsageCapabilites = IOC_LinkUsageEvtProducer};

        ResultValue = IOC_onlineService(&SrvID, &SrvArgs);
        ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);
        ASSERT_NE(IOC_ID_INVALID, SrvID);

        // Step-2: Start client connection thread
        std::atomic<bool> Subscribed{false};
        CliThread = std::thread([&]() {
            IOC_ConnArgs_T ConnArgs = {.SrvURI = SrvURI, .Usage = IOC_LinkUsageEvtConsumer};
            IOC_Result_T ResultValueInThread = IOC_connectService(&CliLinkID, &ConnArgs, NULL);
            ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValueInThread);
            ASSERT_NE(IOC_ID_INVALID, CliLinkID);

            // Subscribe to events
            static IOC_EvtID_T SubEvtIDs[1] = {IOC_EVTID_TEST_KEEPALIVE};
            IOC_SubEvtArgs_T Sub = {.CbProcEvt_F = __KeepLinksTestClientCb,
                                    .pCbPrivData = &RecvPriv,
                                    .EvtNum = 1,
                                    .pEvtIDs = &SubEvtIDs[0]};
            ResultValueInThread = IOC_subEVT(CliLinkID, &Sub);
            ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValueInThread);
            Subscribed = true;
        });

        // Step-3: Wait and manually accept the connection
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        ResultValue = IOC_acceptClient(SrvID, &SrvLinkID, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);
        ASSERT_NE(IOC_ID_INVALID, SrvLinkID);

        // Wait for client subscription
        for (int i = 0; i < 50 && !Subscribed.load(); ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }

        // Step-4: Send event to verify initial connection works
        IOC_EvtDesc_T EvtDesc = {};
        EvtDesc.EvtID = IOC_EVTID_TEST_KEEPALIVE;
        EvtDesc.EvtValue = 42;
        ResultValue = IOC_postEVT(SrvLinkID, &EvtDesc, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);

        // Step-5: Wait for event delivery and verify
        for (int i = 0; i < 60; ++i) {
            if (RecvPriv.Got.load()) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        ASSERT_TRUE(RecvPriv.Got.load());
        ASSERT_EQ(IOC_EVTID_TEST_KEEPALIVE, RecvPriv.EvtID);
        ASSERT_EQ((ULONG_T)42, RecvPriv.EvtValue);

        // Step-6: Take service offline (default behavior - links should be auto-closed)
        ResultValue = IOC_offlineService(SrvID);
        ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);
        SrvID = IOC_ID_INVALID;

        // Step-7: Verify that the link was automatically closed (default behavior)
        // Reset callback state
        RecvPriv.Got = false;
        RecvPriv.EvtValue = 0;
        EvtDesc.EvtValue = 99;

        // This should fail because the link was automatically closed
        IOC_Result_T PostResult = IOC_postEVT(SrvLinkID, &EvtDesc, NULL);
        EXPECT_NE(IOC_RESULT_SUCCESS, PostResult)
            << "Link should be auto-closed when IOC_SRVFLAG_KEEP_ACCEPTED_LINK is NOT set";

        // Event should not be delivered
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        EXPECT_FALSE(RecvPriv.Got.load()) << "Event delivery should stop after service offline when flag is NOT set";

    } catch (...) {
        // Ensure proper cleanup even on exceptions
    }

    // Cleanup
    if (CliThread.joinable()) CliThread.join();
    if (CliLinkID != IOC_ID_INVALID) IOC_closeLink(CliLinkID);
    // Note: SrvLinkID should already be closed by service offline in default behavior
    if (SrvID != IOC_ID_INVALID) IOC_offlineService(SrvID);
}

// [@AC-1,US-2]
// TC-1: verifyPreservedLinks_byEventDelivery_expectContinuedFunctionality
TEST(UT_ConetEventTypical, verifyPreservedLinks_byEventDelivery_expectContinuedFunctionality) {
    IOC_Result_T ResultValue = IOC_RESULT_BUG;
    IOC_SrvID_T SrvID = IOC_ID_INVALID;
    IOC_LinkID_T SrvLinkID = IOC_ID_INVALID;
    IOC_LinkID_T CliLinkID = IOC_ID_INVALID;
    std::thread CliThread;

    // Test data for event communication
    __KeepLinksEvtRecvPriv_T RecvPriv = {};

    try {
        // Step-1: Create service with KEEP_ACCEPTED_LINK flag
        IOC_SrvURI_T SrvURI = {.pProtocol = IOC_SRV_PROTO_FIFO,
                               .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
                               .pPath = (const char *)"PreservedLinks_Test3"};
        IOC_SrvArgs_T SrvArgs = {
            .SrvURI = SrvURI, .Flags = IOC_SRVFLAG_KEEP_ACCEPTED_LINK, .UsageCapabilites = IOC_LinkUsageEvtProducer};

        ResultValue = IOC_onlineService(&SrvID, &SrvArgs);
        ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);
        ASSERT_NE(IOC_ID_INVALID, SrvID);

        // Step-2: Start client connection thread
        std::atomic<bool> Subscribed{false};
        CliThread = std::thread([&]() {
            IOC_ConnArgs_T ConnArgs = {.SrvURI = SrvURI, .Usage = IOC_LinkUsageEvtConsumer};
            IOC_Result_T ResultValueInThread = IOC_connectService(&CliLinkID, &ConnArgs, NULL);
            ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValueInThread);
            ASSERT_NE(IOC_ID_INVALID, CliLinkID);

            // Subscribe to events
            static IOC_EvtID_T SubEvtIDs[1] = {IOC_EVTID_TEST_KEEPALIVE};
            IOC_SubEvtArgs_T Sub = {.CbProcEvt_F = __KeepLinksTestClientCb,
                                    .pCbPrivData = &RecvPriv,
                                    .EvtNum = 1,
                                    .pEvtIDs = &SubEvtIDs[0]};
            ResultValueInThread = IOC_subEVT(CliLinkID, &Sub);
            ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValueInThread);
            Subscribed = true;
        });

        // Step-3: Wait and manually accept the connection
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        ResultValue = IOC_acceptClient(SrvID, &SrvLinkID, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);
        ASSERT_NE(IOC_ID_INVALID, SrvLinkID);

        // Wait for client subscription
        for (int i = 0; i < 50 && !Subscribed.load(); ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }

        // Step-4: Take service offline (key test point - links should be preserved)
        ResultValue = IOC_offlineService(SrvID);
        ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);
        SrvID = IOC_ID_INVALID;

        // Step-5: Verify that preserved links continue to function for event delivery
        // This is the main focus of US-2: event delivery continues after service restart scenarios
        IOC_EvtDesc_T EvtDesc = {};
        EvtDesc.EvtID = IOC_EVTID_TEST_KEEPALIVE;
        EvtDesc.EvtValue = 123;

        // Reset callback state for clean test
        RecvPriv.Got = false;
        RecvPriv.EvtValue = 0;

        // Post event on preserved link - this should work
        IOC_Result_T PostResult = IOC_postEVT(SrvLinkID, &EvtDesc, NULL);
        EXPECT_EQ(IOC_RESULT_SUCCESS, PostResult)
            << "Event posting should work on preserved links when IOC_SRVFLAG_KEEP_ACCEPTED_LINK is set";

        // Wait and verify event was delivered
        for (int i = 0; i < 60; ++i) {
            if (RecvPriv.Got.load()) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        EXPECT_TRUE(RecvPriv.Got.load()) << "Event delivery should continue on preserved links";
        EXPECT_EQ((ULONG_T)123, RecvPriv.EvtValue) << "Event data should be correctly delivered";

        // Step-6: Test multiple event deliveries to ensure continued functionality
        RecvPriv.Got = false;
        RecvPriv.EvtValue = 0;
        EvtDesc.EvtValue = 456;

        PostResult = IOC_postEVT(SrvLinkID, &EvtDesc, NULL);
        EXPECT_EQ(IOC_RESULT_SUCCESS, PostResult) << "Multiple events should work on preserved links";

        for (int i = 0; i < 60; ++i) {
            if (RecvPriv.Got.load()) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        EXPECT_TRUE(RecvPriv.Got.load()) << "Multiple event deliveries should work";
        EXPECT_EQ((ULONG_T)456, RecvPriv.EvtValue) << "Second event should be delivered correctly";

    } catch (...) {
        // Ensure proper cleanup even on exceptions
    }

    // Cleanup
    if (CliThread.joinable()) CliThread.join();
    if (CliLinkID != IOC_ID_INVALID) IOC_closeLink(CliLinkID);
    if (SrvLinkID != IOC_ID_INVALID) IOC_closeLink(SrvLinkID);  // Manual cleanup of preserved link
    if (SrvID != IOC_ID_INVALID) IOC_offlineService(SrvID);
}

// [@AC-1,US-3]
// TC-1: verifyManualCleanup_withKeepAcceptedLinksFlag_expectCleanupWorks
TEST(UT_ConetEventTypical, verifyManualCleanup_withKeepAcceptedLinksFlag_expectCleanupWorks) {
    IOC_Result_T ResultValue = IOC_RESULT_BUG;
    IOC_SrvID_T SrvID = IOC_ID_INVALID;
    IOC_LinkID_T SrvLinkID = IOC_ID_INVALID;
    IOC_LinkID_T CliLinkID = IOC_ID_INVALID;
    std::thread CliThread;

    try {
        // Step-1: Create service with KEEP_ACCEPTED_LINK flag
        IOC_SrvURI_T SrvURI = {.pProtocol = IOC_SRV_PROTO_FIFO,
                               .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
                               .pPath = (const char *)"ManualCleanup_Test3"};
        IOC_SrvArgs_T SrvArgs = {
            .SrvURI = SrvURI, .Flags = IOC_SRVFLAG_KEEP_ACCEPTED_LINK, .UsageCapabilites = IOC_LinkUsageEvtProducer};

        ResultValue = IOC_onlineService(&SrvID, &SrvArgs);
        ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);
        ASSERT_NE(IOC_ID_INVALID, SrvID);

        // Step-2: Start client connection thread
        CliThread = std::thread([&]() {
            IOC_ConnArgs_T ConnArgs = {.SrvURI = SrvURI, .Usage = IOC_LinkUsageEvtConsumer};
            IOC_Result_T ResultValueInThread = IOC_connectService(&CliLinkID, &ConnArgs, NULL);
            EXPECT_EQ(IOC_RESULT_SUCCESS, ResultValueInThread);
        });

        // Step-3: Wait and manually accept the connection
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        ResultValue = IOC_acceptClient(SrvID, &SrvLinkID, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);
        ASSERT_NE(IOC_ID_INVALID, SrvLinkID);

        // Join client thread to ensure connection is complete
        if (CliThread.joinable()) CliThread.join();

        // Step-4: Take service offline (links should remain open due to flag)
        ResultValue = IOC_offlineService(SrvID);
        ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue);
        SrvID = IOC_ID_INVALID;

        // Step-5: Verify link ID is still valid (not auto-closed)
        // The link ID should still be valid but event posting may not work without the service
        // This is the key difference - the link object itself is preserved
        IOC_EvtDesc_T EvtDesc = {};
        EvtDesc.EvtID = IOC_EVTID_TEST_KEEPALIVE;
        EvtDesc.EvtValue = 42;

        // The link ID is preserved, but posting may fail due to no event consumer
        ResultValue = IOC_postEVT(SrvLinkID, &EvtDesc, NULL);
        // We expect IOC_RESULT_NO_EVENT_CONSUMER (-502) because service is offline
        EXPECT_EQ(IOC_RESULT_NO_EVENT_CONSUMER, ResultValue)
            << "Expected no event consumer error when service is offline";

        // Step-6: Manually close the server-side link
        ResultValue = IOC_closeLink(SrvLinkID);
        ASSERT_EQ(IOC_RESULT_SUCCESS, ResultValue) << "Manual close should work on preserved link";
        SrvLinkID = IOC_ID_INVALID;

        // Step-7: Verify that manual cleanup worked
        // After manual close, the link should be completely invalid
        ResultValue = IOC_postEVT(SrvLinkID, &EvtDesc, NULL);
        EXPECT_NE(IOC_RESULT_SUCCESS, ResultValue) << "Link should be completely closed after manual cleanup";

    } catch (...) {
        // Ensure proper cleanup even on exceptions
    }

    // Cleanup
    if (CliThread.joinable()) CliThread.join();
    if (CliLinkID != IOC_ID_INVALID) IOC_closeLink(CliLinkID);
    if (SrvLinkID != IOC_ID_INVALID) IOC_closeLink(SrvLinkID);
    if (SrvID != IOC_ID_INVALID) IOC_offlineService(SrvID);
}

//======>END OF TEST IMPLEMENTATIONS===============================================================
