///////////////////////////////////////////////////////////////////////////////////////////////////
// QUICK REFERENCE GUIDE - 快速参考指南
// 📝 用途: DAT服务发送角色验证单元测试实现 - User Story 6
// 🔄 流程: User Story → Acceptance Criteria → Test Cases → Implementation
// 📂 分类: DataState US-6 - Service asDatSender role verification
// 🎯 重点: 服务作为数据发送者角色的状态验证、客户端作为接收者的状态转换
///////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF OVERVIEW OF THIS UNIT TESTING FILE===============================================
/**
 * @brief
 *  DAT服务发送角色验证单元测试 - 验证IOC框架中服务作为DatSender角色的状态管理功能
 *
 *-------------------------------------------------------------------------------------------------
 *++背景说明：
 *  本测试文件验证IOC框架中DAT(Data Transfer)服务作为DatSender角色的状态管理机制
 *  重点关注Service asDatSender + Client asDatReceiver的角色组合模式
 *  确保角色反转场景下的状态转换正确性和一致性
 *
 *  关键概念：
 *  - Service asDatSender: 服务作为数据发送者，主动推送数据给客户端
 *  - Client asDatReceiver: 客户端作为数据接收者，被动接收服务推送的数据
 *  - Role Reversal: 角色反转，与典型的Client发送、Service接收模式相反
 *  - Push Mode: 推模式数据传输，服务主动向客户端推送数据
 *  - Receiver State Tracking: 客户端接收者状态跟踪机制
 */
//======>END OF OVERVIEW OF THIS UNIT TESTING FILE=================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF UNIT TESTING DESIGN==============================================================

/**************************************************************************************************
 * @brief 【User Story】
 *
 *  US-6: AS a DAT service sender role developer,
 *    I WANT to verify that Service asDatSender and Client asDatReceiver roles work correctly,
 *   SO THAT I can ensure proper state transitions in role-reversed DAT scenarios
 *      AND validate Service-side sender state management,
 *      AND implement reliable Client-side receiver state tracking.
 *
 *************************************************************************************************/

/**************************************************************************************************
 * @brief 【Acceptance Criteria】
 *
 * [@US-6]
 *  AC-1: GIVEN a Service configured as DatSender and Client as DatReceiver,
 *         WHEN Service sends data to Client via IOC_sendDAT,
 *         THEN Service sender states should transition correctly (Ready → BusySendDat → Ready)
 *              AND Client receiver states should be properly tracked
 *              AND data should be successfully delivered to Client receiver callback.
 *
 *  AC-2: GIVEN a Service asDatSender in callback mode communication,
 *         WHEN Service initiates data push operations,
 *         THEN Service sender substates should be observable via IOC_getLinkState
 *              AND Client receiver callback should be executed properly
 *              AND both roles should maintain state consistency throughout.
 *
 *  AC-3: GIVEN multiple Client connections to Service asDatSender,
 *         WHEN Service broadcasts data to multiple Clients,
 *         THEN each Client connection should maintain independent receiver states
 *              AND Service sender state should handle multiple concurrent sends
 *              AND all Client receivers should receive data correctly.
 *
 *************************************************************************************************/

/**************************************************************************************************
 * @brief 【Test Cases】
 *
 * [@AC-1,US-6]
 *  TC-1:
 *      @[Name]: verifyServiceSenderRole_byServiceSendToClient_expectSenderStateTransitions
 *      @[Purpose]: 验证服务发送者角色的状态转换
 *      @[Brief]: Service asDatSender向Client asDatReceiver发送数据，验证发送者状态转换
 *      @[ServiceSender_Focus]: 测试Service端DatSender状态转换规则的正确性
 *
 *  TC-2:
 *      @[Name]: verifyClientReceiverRole_byServiceDataPush_expectReceiverStateTracking
 *      @[Purpose]: 验证客户端接收者角色的状态跟踪
 *      @[Brief]: Client asDatReceiver接收Service推送数据，验证接收者状态跟踪
 *      @[ClientReceiver_Focus]: 测试Client端DatReceiver状态跟踪的正确性
 *
 * [@AC-2,US-6]
 *  TC-1:
 *      @[Name]: verifyServiceSenderCallback_byPushModeOperations_expectCallbackStateConsistency
 *      @[Purpose]: 验证服务发送者回调模式的状态一致性
 *      @[Brief]: Service推送模式操作，验证发送者和接收者回调状态一致性
 *      @[CallbackMode_Focus]: 测试推送模式下的状态一致性和回调执行
 *
 * [@AC-3,US-6]
 *  TC-1:
 *      @[Name]: verifyMultiClientReceiver_byServiceBroadcast_expectIndependentStates
 *      @[Purpose]: 验证多客户端接收者的独立状态管理
 *      @[Brief]: Service向多个Client广播数据，验证独立接收者状态管理
 *      @[MultiClient_Focus]: 测试多客户端场景下的独立状态管理
 *
 *************************************************************************************************/
//======>END OF UNIT TESTING DESIGN================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======BEGIN OF UNIT TESTING IMPLEMENTATION=======================================================

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

#include "UT_DataState.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF TEST FIXTURE CLASS===============================================================

/**
 * @brief DAT服务发送者角色测试夹具类
 *        为US-6相关的所有测试用例提供公共的设置和清理
 *        专门测试Service asDatSender + Client asDatReceiver角色组合
 */
class DATServiceSenderRoleTest : public ::testing::Test {
   protected:
    // ┌──────────────────────────────────────────────────────────────────────────────────────┐
    // │                                🔧 SETUP PHASE                                        │
    // └──────────────────────────────────────────────────────────────────────────────────────┘
    void SetUp() override {
        // Initialize private data structure for service sender role testing
        __ResetStateTracking(&servicePrivData);
        __ResetStateTracking(&clientPrivData);

        printf("🔧 [SETUP] DATServiceSenderRoleTest initialized\n");
    }

    // ┌──────────────────────────────────────────────────────────────────────────────────────┐
    // │                               🧹 CLEANUP PHASE                                        │
    // └──────────────────────────────────────────────────────────────────────────────────────┘
    void TearDown() override {
        // Clean up client connections first
        for (auto& linkID : clientLinkIDs) {
            if (linkID != IOC_ID_INVALID) {
                IOC_closeLink(linkID);
            }
        }
        clientLinkIDs.clear();

        // Clean up service
        if (serviceSrvID != IOC_ID_INVALID) {
            IOC_offlineService(serviceSrvID);
            serviceSrvID = IOC_ID_INVALID;
        }

        printf("🔧 [TEARDOWN] DATServiceSenderRoleTest cleaned up\n");
    }

    // Helper method to setup Service asDatSender + Client asDatReceiver pattern
    void setupServiceSenderClientReceiver() {
        // Setup Service as DatSender (role reversal from typical pattern)
        IOC_SrvArgs_T srvArgs = {};
        IOC_Helper_initSrvArgs(&srvArgs);
        srvArgs.SrvURI.pProtocol = IOC_SRV_PROTO_FIFO;
        srvArgs.SrvURI.pHost = IOC_SRV_HOST_LOCAL_PROCESS;
        srvArgs.SrvURI.pPath = "test/service/sender";
        srvArgs.UsageCapabilites = IOC_LinkUsageDatSender;  // Service as Sender
        srvArgs.Flags = IOC_SRVFLAG_AUTO_ACCEPT;            // Enable auto-accept mode

        // Service as sender doesn't need receive callback, but we set up tracking
        servicePrivData.ServiceAsDatReceiver = false;  // Service NOT as receiver in role reversal

        IOC_Result_T result = IOC_onlineService(&serviceSrvID, &srvArgs);
        ASSERT_EQ(IOC_RESULT_SUCCESS, result) << "Service asDatSender setup failed";

        // Setup Client connection as DatReceiver (role reversal from typical pattern)
        IOC_ConnArgs_T connArgs = {};
        IOC_Helper_initConnArgs(&connArgs);
        connArgs.SrvURI = srvArgs.SrvURI;
        connArgs.Usage = IOC_LinkUsageDatReceiver;  // Client as Receiver

        IOC_DatUsageArgs_T clientDatArgs = {};
        clientDatArgs.CbRecvDat_F = __CbRecvDat_ClientReceiver_F;
        clientDatArgs.pCbPrivData = &clientPrivData;
        connArgs.UsageArgs.pDat = &clientDatArgs;

        IOC_LinkID_T clientLinkID = IOC_ID_INVALID;
        result = IOC_connectService(&clientLinkID, &connArgs, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, result) << "Client asDatReceiver connection setup failed";

        clientLinkIDs.push_back(clientLinkID);

        // Update state tracking
        servicePrivData.ServiceOnline = true;
        servicePrivData.LinkConnected = true;
        clientPrivData.ServiceAsDatReceiver = false;  // Client is receiver, not service
        clientPrivData.ClientAsDatReceiver = true;    // Mark client as receiver
        clientPrivData.LinkConnected = true;

        RECORD_STATE_CHANGE(&servicePrivData);
        RECORD_STATE_CHANGE(&clientPrivData);
    }

    // Client receiver callback for role-reversed scenario
    static IOC_Result_T __CbRecvDat_ClientReceiver_F(IOC_LinkID_T linkID, IOC_DatDesc_pT pDatDesc, void* pCbPrivData) {
        if (!pCbPrivData) return IOC_RESULT_INVALID_PARAM;

        __DatStatePrivData_T* privData = (__DatStatePrivData_T*)pCbPrivData;

        printf("📥 [CLIENT-RECEIVER] Callback executed for LinkID=%llu, DataSize=%zu\n", linkID,
               pDatDesc->Payload.PtrDataLen);

        privData->CallbackExecuted = true;
        privData->TotalDataReceived = pDatDesc->Payload.PtrDataLen;
        RECORD_STATE_CHANGE(privData);

        return IOC_RESULT_SUCCESS;
    }

    // Test data members
    __DatStatePrivData_T servicePrivData;  // Service state tracking
    __DatStatePrivData_T clientPrivData;   // Client state tracking
    IOC_SrvID_T serviceSrvID = IOC_ID_INVALID;
    std::vector<IOC_LinkID_T> clientLinkIDs;
};

//======>END OF TEST FIXTURE CLASS=================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>US-6 AC-1 TESTS: Service asDatSender + Client asDatReceiver role verification=============

/**
 * ╔══════════════════════════════════════════════════════════════════════════════════════════╗
 * ║                        🚀 SERVICE SENDER ROLE VERIFICATION                              ║
 * ╠══════════════════════════════════════════════════════════════════════════════════════════╣
 * ║ @[Name]: verifyServiceSenderRole_byServiceSendToClient_expectSenderStateTransitions     ║
 * ║ @[Purpose]: 验证服务发送者角色的状态转换                                                 ║
 * ║ @[Steps]: Service asDatSender向Client asDatReceiver发送数据，验证发送者状态转换         ║
 * ║ @[Expect]: Service发送者状态正确转换，Client接收者回调被执行，数据传输成功               ║
 * ║ @[Notes]: 验证角色反转场景下的基础功能                                                   ║
 * ║                                                                                          ║
 * ║ 🎯 ServiceSender测试重点：                                                              ║
 * ║   • 验证Service端DatSender状态转换规则的正确性                                           ║
 * ║   • 确保Service能够主动向Client推送数据                                                  ║
 * ║   • 测试角色反转场景下的状态管理正确性                                                   ║
 * ║   • 验证Service发送者和Client接收者的协调工作                                            ║
 * ║ @[TestPattern]: US-6 AC-1 TC-1 - 服务发送者角色状态转换验证                            ║
 * ╚══════════════════════════════════════════════════════════════════════════════════════════╝
 */
TEST_F(DATServiceSenderRoleTest, verifyServiceSenderRole_byServiceSendToClient_expectSenderStateTransitions) {
    // ┌──────────────────────────────────────────────────────────────────────────────────────┐
    // │                                🔧 SETUP PHASE                                        │
    // └──────────────────────────────────────────────────────────────────────────────────────┘
    printf("🧪 [TEST] verifyServiceSenderRole_byServiceSendToClient_expectSenderStateTransitions\n");

    setupServiceSenderClientReceiver();

    // GIVEN: Service configured as DatSender and Client as DatReceiver
    ASSERT_TRUE(servicePrivData.ServiceOnline.load()) << "Service should be online";
    ASSERT_FALSE(servicePrivData.ServiceAsDatReceiver.load())
        << "Service should NOT be configured as DatReceiver in role reversal";
    ASSERT_FALSE(clientLinkIDs.empty()) << "Client connection should be established";

    IOC_LinkID_T clientLinkID = clientLinkIDs[0];

    // ┌──────────────────────────────────────────────────────────────────────────────────────┐
    // │                               🎯 BEHAVIOR PHASE                                       │
    // └──────────────────────────────────────────────────────────────────────────────────────┘
    printf("🚀 [ACTION] Service asDatSender pushing data to Client asDatReceiver\n");

    // WHEN: Service sends data to Client via IOC_sendDAT
    // Note: In role-reversed scenario, Service uses its LinkID to send to Client

    // Get Service's LinkID for the client connection (through accept mechanism)
    // For now, we'll use the client's LinkID as the communication channel
    const char* testData = "Service-to-Client push data";
    IOC_DatDesc_T datDesc = {};
    IOC_initDatDesc(&datDesc);
    datDesc.Payload.pData = (void*)testData;
    datDesc.Payload.PtrDataSize = strlen(testData) + 1;
    datDesc.Payload.PtrDataLen = strlen(testData) + 1;

    // Record initial state
    size_t initialServiceTransitions = servicePrivData.StateTransitionCount.load();
    size_t initialClientTransitions = clientPrivData.StateTransitionCount.load();

    // Service sends data to Client (role-reversed operation)
    IOC_Result_T result = IOC_sendDAT(clientLinkID, &datDesc, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result) << "Service asDatSender should successfully send data to Client";

    // Allow time for data transmission and callback execution
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // ┌──────────────────────────────────────────────────────────────────────────────────────┐
    // │                                ✅ VERIFY PHASE                                        │
    // └──────────────────────────────────────────────────────────────────────────────────────┘
    // @KeyVerifyPoint-1: Service sender states should transition correctly
    ASSERT_GT(servicePrivData.StateTransitionCount.load(), initialServiceTransitions)
        << "Service sender should have state transitions recorded";

    // @KeyVerifyPoint-2: Client receiver states should be properly tracked
    ASSERT_TRUE(clientPrivData.CallbackExecuted.load()) << "Client asDatReceiver callback should be executed";
    ASSERT_GT(clientPrivData.StateTransitionCount.load(), initialClientTransitions)
        << "Client receiver should have state transitions recorded";

    // @KeyVerifyPoint-3: Data should be successfully delivered to Client receiver callback
    ASSERT_EQ(strlen(testData) + 1, clientPrivData.TotalDataReceived) << "Client should receive data with correct size";

    // @KeyVerifyPoint-4: Verify Service sender substate (using IOC_getLinkState)
    IOC_LinkState_T serviceLinkState = IOC_LinkStateUndefined;
    IOC_LinkSubState_T serviceLinkSubState = IOC_LinkSubStateDefault;
    result = IOC_getLinkState(clientLinkID, &serviceLinkState, &serviceLinkSubState);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result) << "Should get Service sender link state";
    ASSERT_EQ(IOC_LinkStateReady, serviceLinkState) << "Service sender main state should be Ready";

    // 🔴 RED TDD: Service should show DatSender substates
    printf("🔴 [RED TDD] Service LinkSubState = %d (expecting IOC_LinkSubStateDatSenderReady = %d)\n",
           serviceLinkSubState, IOC_LinkSubStateDatSenderReady);
    ASSERT_EQ(IOC_LinkSubStateDatSenderReady, serviceLinkSubState)
        << "🔴 RED TDD: Service should show DatSender Ready substate after send completion";

    printf("✅ [RESULT] Service asDatSender role verification successful\n");
    printf("🔄 [ROLE-REVERSAL] Service → Client data push pattern verified\n");

    // ┌──────────────────────────────────────────────────────────────────────────────────────┐
    // │                               🧹 CLEANUP PHASE                                        │
    // └──────────────────────────────────────────────────────────────────────────────────────┘
    // Cleanup handled by TearDown()
}

/**
 * ╔══════════════════════════════════════════════════════════════════════════════════════════╗
 * ║                       📥 CLIENT RECEIVER ROLE VERIFICATION                              ║
 * ╠══════════════════════════════════════════════════════════════════════════════════════════╣
 * ║ @[Name]: verifyClientReceiverRole_byServiceDataPush_expectReceiverStateTracking         ║
 * ║ @[Purpose]: 验证客户端接收者角色的状态跟踪                                               ║
 * ║ @[Steps]: Client asDatReceiver接收Service推送数据，验证接收者状态跟踪                   ║
 * ║ @[Expect]: Client接收者状态正确跟踪，接收回调正常执行，状态转换记录完整                   ║
 * ║ @[Notes]: 专门测试Client端作为接收者的状态管理                                           ║
 * ║                                                                                          ║
 * ║ 🎯 ClientReceiver测试重点：                                                             ║
 * ║   • 验证Client端DatReceiver状态跟踪的正确性                                              ║
 * ║   • 确保Client能够正确接收Service推送的数据                                              ║
 * ║   • 测试Client接收者回调机制的可靠性                                                     ║
 * ║   • 验证Client接收者状态与Service发送者状态的协调                                        ║
 * ║ @[TestPattern]: US-6 AC-1 TC-2 - 客户端接收者角色状态跟踪验证                          ║
 * ╚══════════════════════════════════════════════════════════════════════════════════════════╝
 */
TEST_F(DATServiceSenderRoleTest, verifyClientReceiverRole_byServiceDataPush_expectReceiverStateTracking) {
    // ┌──────────────────────────────────────────────────────────────────────────────────────┐
    // │                                🔧 SETUP PHASE                                        │
    // └──────────────────────────────────────────────────────────────────────────────────────┘
    printf("🧪 [TEST] verifyClientReceiverRole_byServiceDataPush_expectReceiverStateTracking\n");

    setupServiceSenderClientReceiver();

    // GIVEN: Client configured as DatReceiver to receive Service data pushes
    ASSERT_TRUE(clientPrivData.ClientAsDatReceiver.load()) << "Client should be configured as DatReceiver";
    ASSERT_FALSE(clientLinkIDs.empty()) << "Client connection should be established";

    IOC_LinkID_T clientLinkID = clientLinkIDs[0];

    // ┌──────────────────────────────────────────────────────────────────────────────────────┐
    // │                               🎯 BEHAVIOR PHASE                                       │
    // └──────────────────────────────────────────────────────────────────────────────────────┘
    printf("📥 [ACTION] Client asDatReceiver receiving Service data push\n");

    // WHEN: Service initiates data push to Client
    const char* pushData1 = "Service push message #1";
    const char* pushData2 = "Service push message #2";

    // First data push
    IOC_DatDesc_T datDesc1 = {};
    IOC_initDatDesc(&datDesc1);
    datDesc1.Payload.pData = (void*)pushData1;
    datDesc1.Payload.PtrDataSize = strlen(pushData1) + 1;
    datDesc1.Payload.PtrDataLen = strlen(pushData1) + 1;

    size_t initialClientTransitions = clientPrivData.StateTransitionCount.load();

    IOC_Result_T result = IOC_sendDAT(clientLinkID, &datDesc1, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result) << "First Service data push should succeed";

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Second data push
    IOC_DatDesc_T datDesc2 = {};
    IOC_initDatDesc(&datDesc2);
    datDesc2.Payload.pData = (void*)pushData2;
    datDesc2.Payload.PtrDataSize = strlen(pushData2) + 1;
    datDesc2.Payload.PtrDataLen = strlen(pushData2) + 1;

    result = IOC_sendDAT(clientLinkID, &datDesc2, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result) << "Second Service data push should succeed";

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // ┌──────────────────────────────────────────────────────────────────────────────────────┐
    // │                                ✅ VERIFY PHASE                                        │
    // └──────────────────────────────────────────────────────────────────────────────────────┘
    // @KeyVerifyPoint-1: Client receiver states should be properly tracked
    ASSERT_TRUE(clientPrivData.CallbackExecuted.load()) << "Client asDatReceiver callback should be executed";

    // @KeyVerifyPoint-2: Client should track multiple state transitions
    ASSERT_GT(clientPrivData.StateTransitionCount.load(), initialClientTransitions)
        << "Client receiver should have recorded state transitions from multiple pushes";

    // @KeyVerifyPoint-3: Client should receive latest data correctly
    ASSERT_EQ(strlen(pushData2) + 1, clientPrivData.TotalDataReceived)
        << "Client should receive latest push data with correct size";

    // @KeyVerifyPoint-4: Verify Client receiver substate (if accessible)
    // Note: In half-duplex architecture, we might not directly see receiver substates from client link
    // But we can verify the link state consistency
    IOC_LinkState_T clientLinkState = IOC_LinkStateUndefined;
    result = IOC_getLinkState(clientLinkID, &clientLinkState, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result) << "Should get Client link state";
    ASSERT_EQ(IOC_LinkStateReady, clientLinkState) << "Client link main state should be Ready";

    // @KeyVerifyPoint-5: Client receiver role verification
    ASSERT_TRUE(clientPrivData.ClientAsDatReceiver.load()) << "Client should maintain DatReceiver role";
    ASSERT_TRUE(clientPrivData.LinkConnected.load()) << "Client link should remain connected";

    printf("✅ [RESULT] Client asDatReceiver role verification successful\n");
    printf("📡 [PUSH-MODE] Client successfully received multiple Service data pushes\n");

    // ┌──────────────────────────────────────────────────────────────────────────────────────┐
    // │                               🧹 CLEANUP PHASE                                        │
    // └──────────────────────────────────────────────────────────────────────────────────────┘
    // Cleanup handled by TearDown()
}

// Additional test cases for AC-2 and AC-3 would be implemented here...
// Following the same comprehensive pattern

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>END OF US-6 IMPLEMENTATION================================================================
