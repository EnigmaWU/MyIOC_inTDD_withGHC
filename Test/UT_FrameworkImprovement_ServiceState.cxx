///////////////////////////////////////////////////////////////////////////////////////////////////
// 🚀 FRAMEWORK IMPROVEMENT TEST: Service-Side State Access APIs
// 📝 Purpose: Test NEW IOC framework APIs for comprehensive service monitoring
// 🎯 Focus: IOC_getServiceLinkIDs() and IOC_getServiceState() - REAL framework improvements
// 💡 Value: Enables receiver-side substate testing and service management
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "UT_DataState.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>🚀 FRAMEWORK IMPROVEMENT TEST CLASS====================================================

class ServiceStateImprovementTest : public ::testing::Test {
   protected:
    IOC_SrvID_T testSrvID;
    IOC_LinkID_T testClientLinkID;
    __DatStatePrivData_T privData;

    void SetUp() override {
        printf("🔧 [SETUP] ServiceStateImprovementTest initialized - testing NEW framework APIs\n");
        __ResetStateTracking(&privData);

        testSrvID = IOC_ID_INVALID;
        testClientLinkID = IOC_ID_INVALID;
        setupServiceWithMultipleConnections();
    }

    void TearDown() override {
        if (testClientLinkID != IOC_ID_INVALID) {
            IOC_closeLink(testClientLinkID);
        }
        if (testSrvID != IOC_ID_INVALID) {
            IOC_offlineService(testSrvID);
        }
        printf("🔧 [TEARDOWN] ServiceStateImprovementTest cleaned up\n");
    }

   private:
    void setupServiceWithMultipleConnections() {
        // Setup service as DatReceiver with auto-accept
        IOC_SrvArgs_T srvArgs = {};
        IOC_Helper_initSrvArgs(&srvArgs);
        srvArgs.SrvURI.pProtocol = IOC_SRV_PROTO_FIFO;
        srvArgs.SrvURI.pHost = IOC_SRV_HOST_LOCAL_PROCESS;
        srvArgs.SrvURI.pPath = "framework/improvement/service/state";
        srvArgs.UsageCapabilites = IOC_LinkUsageDatReceiver;
        srvArgs.Flags = IOC_SRVFLAG_AUTO_ACCEPT;  // Enable auto-accept for multiple connections

        IOC_DatUsageArgs_T datArgs = {};
        datArgs.CbRecvDat_F = __CbRecvDat_ServiceReceiver_F;
        datArgs.pCbPrivData = &privData;
        srvArgs.UsageArgs.pDat = &datArgs;

        IOC_Result_T result = IOC_onlineService(&testSrvID, &srvArgs);
        ASSERT_EQ(IOC_RESULT_SUCCESS, result) << "Service should come online for state improvement testing";

        // Connect as client sender
        IOC_ConnArgs_T connArgs = {};
        IOC_Helper_initConnArgs(&connArgs);
        connArgs.SrvURI = srvArgs.SrvURI;
        connArgs.Usage = IOC_LinkUsageDatSender;

        result = IOC_connectService(&testClientLinkID, &connArgs, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, result) << "Client should connect for state improvement testing";

        std::this_thread::sleep_for(std::chrono::milliseconds(50));  // Allow connection setup
    }
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>🚀 NEW API TEST: IOC_getServiceLinkIDs=================================================

/**
 * ╔══════════════════════════════════════════════════════════════════════════════════════════╗
 * ║                    🚀 FRAMEWORK IMPROVEMENT: Service LinkID Access                      ║
 * ╠══════════════════════════════════════════════════════════════════════════════════════════╣
 * ║ @[NEW-API]: IOC_getServiceLinkIDs() - enables service-side LinkID enumeration           ║
 * ║ @[Problem-Solved]: Previously impossible to query receiver-side substates               ║
 * ║ @[Value]: Comprehensive service monitoring and receiver state inspection                 ║
 * ║ @[TDD-Impact]: Enables REAL receiver substate testing instead of architectural gaps     ║
 * ╚══════════════════════════════════════════════════════════════════════════════════════════╝
 */
TEST_F(ServiceStateImprovementTest, verifyNewServiceLinkIDsAPI_enablesReceiverStateAccess_expectServiceSideLinkIDs) {
    printf(
        "🧪 [FRAMEWORK-IMPROVEMENT] verifyNewServiceLinkIDsAPI_enablesReceiverStateAccess_expectServiceSideLinkIDs\n");

    // ===== TEST NEW API: IOC_getServiceLinkIDs() =====
    const uint16_t MaxLinks = 10;
    IOC_LinkID_T serviceLinkIDs[MaxLinks] = {0};
    uint16_t actualCount = 0;

    printf("🚀 [NEW-API] Testing IOC_getServiceLinkIDs() - previously impossible functionality\n");
    IOC_Result_T result = IOC_getServiceLinkIDs(testSrvID, serviceLinkIDs, MaxLinks, &actualCount);

    // ===== VERIFY NEW CAPABILITY =====
    printf("🔍 [API-RESULT] IOC_getServiceLinkIDs result=%d, actualCount=%d\n", result, actualCount);

    ASSERT_EQ(IOC_RESULT_SUCCESS, result) << "NEW API IOC_getServiceLinkIDs should work successfully";
    ASSERT_GT(actualCount, 0) << "Service should have at least one connected LinkID";

    // ===== DEMONSTRATE RECEIVER STATE ACCESS =====
    for (uint16_t i = 0; i < actualCount; i++) {
        IOC_LinkID_T serviceSideLinkID = serviceLinkIDs[i];
        printf("🔗 [SERVICE-LINK] Found service-side LinkID: %llu\n", serviceSideLinkID);

        // NOW WE CAN QUERY RECEIVER-SIDE STATES!
        IOC_LinkState_T mainState = IOC_LinkStateUndefined;
        IOC_LinkSubState_T subState = IOC_LinkSubStateDefault;
        IOC_Result_T stateResult = IOC_getLinkState(serviceSideLinkID, &mainState, &subState);

        printf("🔍 [RECEIVER-STATE] Service-side LinkID state: mainState=%d, subState=%d\n", mainState, subState);

        ASSERT_EQ(IOC_RESULT_SUCCESS, stateResult) << "Should be able to query service-side LinkID state";

        // Check if this is a receiver-side LinkID
        if (subState == IOC_LinkSubStateDatReceiverReady || subState == IOC_LinkSubStateDatReceiverBusyRecvDat ||
            subState == IOC_LinkSubStateDatReceiverBusyCbRecvDat) {
            printf("✅ [BREAKTHROUGH] Found RECEIVER substate (%d) on service-side LinkID!\n", subState);
            printf("🏆 [IMPROVEMENT] This was impossible before the framework improvement!\n");
        } else if (subState == IOC_LinkSubStateDatSenderReady) {
            printf(
                "ℹ️  [INFO] Found sender substate (%d) - this LinkID represents the service's view of client sender\n",
                subState);
        } else {
            printf("🔍 [UNKNOWN] Found substate (%d) - may be default or other state\n", subState);
        }
    }

    printf("🏆 [SUCCESS] NEW IOC_getServiceLinkIDs API enables comprehensive service monitoring!\n");
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>🚀 NEW API TEST: IOC_getServiceState===================================================

/**
 * ╔══════════════════════════════════════════════════════════════════════════════════════════╗
 * ║                    🚀 FRAMEWORK IMPROVEMENT: Service State Monitoring                   ║
 * ╠══════════════════════════════════════════════════════════════════════════════════════════╣
 * ║ @[NEW-API]: IOC_getServiceState() - provides comprehensive service status information   ║
 * ║ @[Problem-Solved]: Previously no way to monitor service connection count and health      ║
 * ║ @[Value]: Service management, monitoring, and debugging capabilities                     ║
 * ║ @[Future-Ready]: Extensible for additional service state information                     ║
 * ╚══════════════════════════════════════════════════════════════════════════════════════════╝
 */
TEST_F(ServiceStateImprovementTest, verifyNewServiceStateAPI_providesServiceMonitoring_expectConnectionCount) {
    printf("🧪 [FRAMEWORK-IMPROVEMENT] verifyNewServiceStateAPI_providesServiceMonitoring_expectConnectionCount\n");

    // ===== TEST NEW API: IOC_getServiceState() =====
    uint16_t connectedLinks = 0;

    printf("🚀 [NEW-API] Testing IOC_getServiceState() - comprehensive service monitoring\n");
    IOC_Result_T result = IOC_getServiceState(testSrvID, NULL, &connectedLinks);

    // ===== VERIFY SERVICE MONITORING CAPABILITY =====
    printf("🔍 [API-RESULT] IOC_getServiceState result=%d, connectedLinks=%d\n", result, connectedLinks);

    ASSERT_EQ(IOC_RESULT_SUCCESS, result) << "NEW API IOC_getServiceState should work successfully";
    EXPECT_GT(connectedLinks, 0) << "Service should report at least one connected link";

    // ===== DEMONSTRATE SERVICE HEALTH MONITORING =====
    printf("📊 [SERVICE-HEALTH] Service %llu has %d active connections\n", testSrvID, connectedLinks);
    printf("🏆 [MONITORING] This enables service health monitoring and management!\n");

    // ===== TEST ERROR HANDLING =====
    IOC_SrvID_T invalidSrvID = 12345;  // Use a non-existent but valid-range SrvID
    IOC_Result_T errorResult = IOC_getServiceState(invalidSrvID, NULL, &connectedLinks);
    ASSERT_EQ(IOC_RESULT_NOT_EXIST_SERVICE, errorResult) << "API should properly handle invalid service ID";

    printf("✅ [ERROR-HANDLING] API properly validates service ID existence\n");
    printf("🎯 [SUCCESS] NEW IOC_getServiceState API provides comprehensive service monitoring!\n");
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>🚀 INTEGRATION TEST: Combined Service State Management=================================

/**
 * ╔══════════════════════════════════════════════════════════════════════════════════════════╗
 * ║              🚀 FRAMEWORK IMPROVEMENT: Complete Service State Management                 ║
 * ╠══════════════════════════════════════════════════════════════════════════════════════════╣
 * ║ @[INTEGRATION]: Combines both new APIs for comprehensive service management              ║
 * ║ @[Real-World-Value]: Enables production-ready service monitoring and debugging          ║
 * ║ @[TDD-Evolution]: Shows how TDD RED→GREEN revealed framework gaps and drove improvements║
 * ╚══════════════════════════════════════════════════════════════════════════════════════════╝
 */
TEST_F(ServiceStateImprovementTest, integratedServiceStateManagement_combinesNewAPIs_expectCompleteMonitoring) {
    printf("🧪 [INTEGRATION] integratedServiceStateManagement_combinesNewAPIs_expectCompleteMonitoring\n");

    // ===== STEP 1: Get service overview =====
    uint16_t totalConnections = 0;
    IOC_Result_T result = IOC_getServiceState(testSrvID, NULL, &totalConnections);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result);
    printf("📊 [OVERVIEW] Service %llu: %d total connections\n", testSrvID, totalConnections);

    // ===== STEP 2: Get detailed LinkID information =====
    IOC_LinkID_T linkIDs[16] = {0};
    uint16_t actualCount = 0;
    result = IOC_getServiceLinkIDs(testSrvID, linkIDs, 16, &actualCount);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result);
    printf("🔗 [DETAILS] Found %d specific LinkIDs for detailed inspection\n", actualCount);

    // ===== STEP 3: Complete state inspection per LinkID =====
    uint16_t receiverStates = 0;
    uint16_t senderStates = 0;
    uint16_t otherStates = 0;

    for (uint16_t i = 0; i < actualCount; i++) {
        IOC_LinkState_T mainState;
        IOC_LinkSubState_T subState;
        result = IOC_getLinkState(linkIDs[i], &mainState, &subState);

        if (result == IOC_RESULT_SUCCESS) {
            printf("🔍 [LINK-%d] LinkID=%llu: mainState=%d, subState=%d\n", i, linkIDs[i], mainState, subState);

            if (subState >= IOC_LinkSubStateDatReceiverReady && subState <= IOC_LinkSubStateDatReceiverBusyCbRecvDat) {
                receiverStates++;
            } else if (subState >= IOC_LinkSubStateDatSenderReady && subState <= IOC_LinkSubStateDatSenderBusySendDat) {
                senderStates++;
            } else {
                otherStates++;
            }
        }
    }

    // ===== REPORT COMPREHENSIVE SERVICE STATE =====
    printf("════════════════════════════════════════════════════════════════════\n");
    printf("🏆 [COMPLETE-REPORT] Service %llu State Summary:\n", testSrvID);
    printf("├── Total Connections: %d\n", totalConnections);
    printf("├── Receiver States: %d\n", receiverStates);
    printf("├── Sender States: %d\n", senderStates);
    printf("├── Other States: %d\n", otherStates);
    printf("└── Monitoring Status: ✅ FULLY OPERATIONAL\n");
    printf("════════════════════════════════════════════════════════════════════\n");

    // ===== VERIFY MEANINGFUL IMPROVEMENT =====
    EXPECT_EQ(totalConnections, actualCount) << "Connection count should match LinkID count";
    EXPECT_GT(senderStates + receiverStates, 0) << "Should have meaningful DAT substates";

    printf("🎯 [FRAMEWORK-IMPROVEMENT] NEW APIs enable complete service state management!\n");
    printf("💡 [TDD-DRIVEN] This improvement was discovered through TDD RED→GREEN process!\n");
    printf("🚀 [PRODUCTION-READY] Framework now supports comprehensive service monitoring!\n");
}
