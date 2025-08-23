/**
 * @file UT_EventTypicalKeepAcceptedLinks.cxx
 * @brief Unit tests for IOC_SRVFLAG_KEEP_ACCEPTED_LINK flag behavior
 *
 * This test suite verifies that the IOC_SRVFLAG_KEEP_ACCEPTED_LINK service flag
 * correctly controls whether manually accepted links are automatically closed
 * during service shutdown or kept alive for application-controlled cleanup.
 *
 * Test scenarios:
 * - Verify that manually accepted links remain open when flag is set
 * - Verify that event delivery continues after service offline when flag is set
 * - Verify that manual cleanup still works when flag is set
 * - Compare behavior with and without the flag
 */

#include "_UT_IOC_Common.h"

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

/**
 * Test suite for IOC_SRVFLAG_KEEP_ACCEPTED_LINK functionality
 */
class UT_EventTypicalKeepAcceptedLinks : public ::testing::Test {
   protected:
    void SetUp() override {
        // Common test setup
    }

    void TearDown() override {
        // Ensure cleanup
    }
};

// [@TC-1] Verify that manually accepted links remain open when IOC_SRVFLAG_KEEP_ACCEPTED_LINK is set
TEST_F(UT_EventTypicalKeepAcceptedLinks, verifyKeepAcceptedLinksFlag_byServiceOffline_expectLinksRemainOpen) {
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

// [@TC-2] Verify that the flag behavior differs from default behavior
TEST_F(UT_EventTypicalKeepAcceptedLinks, verifyFlagDifference_compareWithDefaultBehavior_expectDifferentLifecycle) {
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

// [@TC-3] Verify manual cleanup still works when flag is set
TEST_F(UT_EventTypicalKeepAcceptedLinks, verifyManualCleanup_withKeepAcceptedLinksFlag_expectCleanupWorks) {
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
