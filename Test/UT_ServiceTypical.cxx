///////////////////////////////////////////////////////////////////////////////////////////////////
//======BEGIN OF OVERVIEW OF THIS UNIT TESTING FILE================================================
/**
 * @brief ValidFunc-Typical Tests: Verify typical/classic scenarios where APIs WORK correctly.
 *
 *-------------------------------------------------------------------------------------------------
 * @category ValidFunc-Typical (Common Scenarios That Work - APIs Function Correctly)
 *
 * Part of Test Design Formula:
 *   Service's Functional Test = ValidFunc(Typical + Boundary) + InValidFunc(Misuse)
 *                                         ^^^^^^^^
 *                                    (Normal cases WORK!)
 *
 * ValidFunc = API WORKS from caller's viewpoint (successful operation)
 *  - Typical: Common scenarios in normal range - happy path success flows
 *  - Boundary: Edge cases (min/max, limits) but still work correctly
 *
 * This file covers: Typical/classic usage scenarios with expected success
 *  - Single and multiple services with single/multiple clients
 *  - Event posting, subscribing, and unsubscribing workflows
 *  - Service producer/consumer role variations
 *  - Dynamic resubscription patterns
 *  - All operations complete successfully as designed
 *
 * Test Philosophy - KEY DISTINCTION:
 *  - ValidFunc (Typical + Boundary): API WORKS correctly (success or graceful error)
 *  - InValidFunc (Misuse): API usage FAILS (wrong sequence, double calls, violations)
 *  - Focus: Verify common real-world scenarios execute successfully
 *  - All inputs are valid, all sequences are correct, all operations succeed
 *
 * Related Test Files:
 *  - UT_ServiceBoundary.cxx: ValidFunc-Boundary (edge cases that still work)
 *  - UT_ServiceMisuse.cxx: InValidFunc-Misuse (wrong usage that fails)
 *  - See: Test/UT_ServiceTestDesign.md for complete test taxonomy
 *
 *-------------------------------------------------------------------------------------------------
 * @note API Overview
 *     Service is identified by 'SrvURI' defined in IOC_SrvTypes.h,
 *         which is a combination of 'Protocol+Host+Port+Path' with some customized meaning.
 *     Service APIs are defined in IOC_SrvAPI.h, and types are defined in IOC_SrvTypes.h.
 *     On the server side, we call:
 *         IOC_onlineService() to online a service, IOC_offlineService() to offline a service,
 *         IOC_acceptLink() to accept a link from client,
 *         IOC_closeLink() to close a link.
 *     On the client side, we call:
 *         IOC_connectService() to connect to a service,
 *         IOC_closeLink() to close a link.
 *     On both sides, we can call:
 *         IOC_postEVT() to post an event, IOC_CbProcEvt_F() to process an event.
 *         IOC_execCMD() to execute a command, IOC_CbExecCmd_F() to execute a command.
 *         IOC_sendDAT() to send data, IOC_CbRecvDat_F() to receive data.
 */
//======END OF OVERVIEW OF THIS UNIT TESTING FILE==================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======BEGIN OF UNIT TESTING DESIGN===============================================================
/**
 * At least one User Story(a.k.a US),
 *    and at least one Acceptance Criteria(a.k.a AC) for each US,
 *      and at least one Test Case(a.k.a TC) for each AC.
 *
 * US takes VALUE from USR perspective.
 * AC clear CONDITIONS may relate to the USR.
 * TC details each condition's STEPS to verify.
 */
//-------------------------------------------------------------------------------------------------
/**
 * @brief 【User Story】
 *
 *  US-1: <a>AS a EvtProducer,
 *      I WANT to online one or many service with custom URI,
 *      SO THAT EvtConsumers can connect to my service,
 *          AND EACH can subscribe all or part events what I published on connected pair Links,
 *          AND ANY EvtConsumer can unsubscribe the event what it subscribed at any time.
 *  US-2: OR <b>AS a EvtConsumer,
 *      I ALSO WANT to online a service,
 *      SO THAT EvtProducer can connect to my service,
 *          AND publish events on connected pair Links.
 *      SAME FOR CmdInitiator and CmdExecutor:
 *  US-3:   <c>CmdInitiator online a service.
 *  US-4:   <d>CmdExecutor online a service.
 *      SAME FOR DatSender and DatReceiver:
 *  US-5:   <e>DatSender online a service.
 *  US-6:   <f>DatReceiver online a service.
 *  US-7:   <g>AS a EvtProducer,
 *      I WANT to online a service with BROADCAST flag,
 *      SO THAT when many EvtConsumers connect to my service,
 *          I CAN post events to all connected pair Links by a single API call.
 */

//-------------------------------------------------------------------------------------------------
/**
 * @brief 【Acceptance Criteria】
 *
 * [@US-1.a]
 *      AC-1: GIVEN a service is onlined by EvtProducer,
 *          WHEN EvtConsumer connects to the service and establish a pair Link,
 *          THEN EvtConsumer can subscribe events,
 *              AND EvtProducer can post events, EvtConsumer can process them.
 *          WHEN EvtConsumer unsubscribe the event,
 *          THEN EvtProducer post events will get NO_EVENT_CONSUMER result,
 *              AND EvtConsumer will NOT process the event.
 *      AC-2: GIVEN a service is onlined by EvtProducer,
 *          WHEN MANY EvtConsumers connects to the service and EACH establish a pair Link,
 *          THEN EACH EvtConsumer can subscribe different events on each's pair Link,
 *              WHEN EvtProducer post events to all pair Links,
 *              THEN EACH EvtConsumer will process what it subscribed events only.
 *      AC-3: GIVEN many services with different URI are onlined by EvtProducer,
 *          WHEN EvtConsumer connects to each service and establish a pair Link,
 *          THEN EvtConsumer can subscribe events on each pair Link,
 *              AND EvtProducer can post events to each pair Link, EvtConsumer can process them.
 *
 * [@US-2.b]
 *      AC-1: GIVEN MANY services with different URI are onlined by EvtConsumer,
 *          WHEN MANY EvtProducer connects to each service and establish a pair Link,
 *          THEN EvtProducer can post events to each pair Link, EvtConsumer can process them same as AC-3 of US-1.a.
 *
 */

//-------------------------------------------------------------------------------------------------
/**
 * @brief 【Test Cases】
 *
 * [@AC-1 of US-1.a]
 * TC-1:
 *  @[Name]: verifySingleServiceSingleClient_byPostEvtAtSrvSide
 *  @[Purpose]: verify simple but still typical scenario of one EvtProducer as server, one EvtConsumer as client.
 *
 * [@AC-2 of US-1.a]
 * TC-2:
 *  @[Name]: verifySingleServiceMultiClients_byPostEvtAtSrvSide_bySubDiffEvtAtCliSide
 *  @[Purpose]: verify multiple EvtConsumers can subscribe different events on each's pair Link.
 *
 * [@AC-3 of US-1.a]
 * TC-3:
 *  @[Name]: verifyMultiServiceMultiClient_byPostEvtAtSrvSide_bySubDiffEvtAtCliSide
 *  @[Purpose]: verify different Services with distinguish URI can be onlined by same EvtProducer,
 *      and each EvtConsumer can connect to each service, then sub&post&proc different events.
 *
 * [@AC-1 of US-2.b]
 *  TC-4:
 *  @[Name]: verifyMultiServiceMultiClient_byPostEvtAtCliSide_bySubDiffEvtAtSrvSide
 *  @[Purpose]: verify different Services with distinguish URI can be onlined by same EvtConsumer,
 *      and each EvtProducer can connect to each service, then post different events.
 *  @[Brief]: EvtConsumerA/B each online a service,
 *      EvtProducer1 connect to EvtConsumerA, and post MOVE_STARTED/KEEPING/STOPPED events,
 *      EvtProducer2 connect to EvtConsumerB, and post PULL_STARTED/KEEPING/STOPPED events,
 *      EvtProducer3 connect to EvtConsumerA/B, and post PUSH_STARTED/KEEPING/STOPPED events.
 *
 * [@AC-1 of US-1.a]
 * TC-5:
 *  @[Name]: verifyConsumerResubscribeEvent
 *  @[Purpose]: verify that an EvtConsumer can resubscribe to an event after unsubscribing.
 *  @[Brief]: EvtProducer online a service,
 *      EvtConsumer connect to the service, subscribe to an event, unsubscribe from the event,
 *      resubscribe to the event, and verify that the event is processed correctly.
 */

//======END OF UNIT TESTING DESIGN=================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======BEGIN OF UNIT TESTING IMPLEMENTATION=======================================================
#include "_UT_IOC_Common.h"

// 定义公共的事件消费者私有数据结构体
typedef struct {
    uint32_t KeepAliveEvtCnt;
    uint32_t StartedEvtCnt;
    uint32_t KeepingEvtCnt;
    uint32_t StoppedEvtCnt;
} __EvtConsumerPrivData_T;

// 定义公共的事件处理回调函数
static IOC_Result_T __CbProcEvt_F(IOC_EvtDesc_T* pEvtDesc, void* pCbPrivData) {
    __EvtConsumerPrivData_T* pEvtConsumerPrivData = (__EvtConsumerPrivData_T*)pCbPrivData;

    switch (pEvtDesc->EvtID) {
        case IOC_EVTID_TEST_KEEPALIVE:
            pEvtConsumerPrivData->KeepAliveEvtCnt++;
            break;
        case IOC_EVTID_TEST_MOVE_STARTED:
        case IOC_EVTID_TEST_PULL_STARTED:
        case IOC_EVTID_TEST_PUSH_STARTED:
            pEvtConsumerPrivData->StartedEvtCnt++;
            break;
        case IOC_EVTID_TEST_MOVE_KEEPING:
        case IOC_EVTID_TEST_PULL_KEEPING:
        case IOC_EVTID_TEST_PUSH_KEEPING:
            pEvtConsumerPrivData->KeepingEvtCnt++;
            break;
        case IOC_EVTID_TEST_MOVE_STOPPED:
        case IOC_EVTID_TEST_PULL_STOPPED:
        case IOC_EVTID_TEST_PUSH_STOPPED:
            pEvtConsumerPrivData->StoppedEvtCnt++;
            break;
        default:
            EXPECT_TRUE(false) << "Unknown EvtID: " << pEvtDesc->EvtID;
            break;
    }

    return IOC_RESULT_SUCCESS;
}

/**
 * @[Name]: <US1-AC1-TC1>verifySingleServiceSingleClient_byPostEvtAtSrvSide
 * @[Steps]:
 *   1) EvtProducer call IOC_onlineService() to online a service AS VERIFY.
 *      |-> SrvArgs.UsageCapabilites = IOC_LinkUsageEvtProducer
 *      |-> SrvArgs.SrvURI
 *          |-> pProtocol = IOC_SRV_PROTO_FIFO
 *          |-> pHost = IOC_SRV_HOST_LOCAL_PROCESS
 *          |-> pPath = "EvtProducer"
 *   2) EvtConsumer call IOC_connectLink() in standalone thread to the service AS VERIFY.
 *          |-> ConnArgs.Usage = IOC_LinkUsageEvtConsumer
 *          |-> ConnArgs.SrvURI = SrvArgs.SrvURI
 *      a) Call IOC_subEVT() to subscribe an event AS BEHAVIOR.
 *          |-> SubEvtArgs.EvtIDs = {IOC_EVT_NAME_TEST_KEEPALIVE}
 *   3) EvtProducer call IOC_acceptLink() to accept the link AS VERIFY.
 *   4) EvtProducer call IOC_postEVT() to post an event AS BEHAVIOR.
 *      |-> EvtDesc.EvtID = IOC_EVT_NAME_TEST_KEEPALIVE
 *      |-> call IOC_forceProcEVT() to process the event immediately.
 *   5) EvtConsumer call IOC_unsubEVT() to unsubscribe the event AS BEHAVIOR.
 *      |-> EvtConsumerPrivData.KeepAliveEvtCnt = 1 AS VERIFY.
 *   6) EvtProducer call IOC_postEVT() to post another event AS BEHAVIOR.
 *      |-> EvtDesc.EvtID = IOC_EVT_NAME_TEST_KEEPALIVE
 *      |-> get IOC_RESULT_NO_EVENT_CONSUMER AS VERIFY.
 *   7) EvtProducer/EvtConsumer call IOC_closeLink() to close the link AS VERIFY&CLEANUP.
 *   8) EvtProducer call IOC_offlineService() to offline the service AS VERIFY&CLEANUP.
 * @[Expect]: all steps are passed.
 * @[Notes]:
 */

TEST(UT_ServiceTypical, verifySingleServiceSingleClient_byPostEvtAtSrvSide) {
    IOC_Result_T Result = IOC_RESULT_BUG;
    IOC_SrvID_T EvtProducerSrvID = IOC_ID_INVALID;
    IOC_LinkID_T EvtProducerLinkID = IOC_ID_INVALID;
    IOC_LinkID_T EvtConsumerLinkID = IOC_ID_INVALID;

    IOC_SrvURI_T CSURI = {
        .pProtocol = IOC_SRV_PROTO_FIFO,
        .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
        .pPath = (const char*)"verifySingleServiceSingleClient",
    };

    // Step-1
    IOC_SrvArgs_T SrvArgs = {
        .SrvURI = CSURI,
        .UsageCapabilites = IOC_LinkUsageEvtProducer,
    };

    Result = IOC_onlineService(&EvtProducerSrvID, &SrvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // KeyVerifyPoint

    // Step-2:
    __EvtConsumerPrivData_T EvtConsumerPrivData = {0};

    IOC_EvtID_T EvtIDs[] = {IOC_EVTID_TEST_KEEPALIVE};
    IOC_SubEvtArgs_T SubEvtArgs = {
        .CbProcEvt_F = __CbProcEvt_F,
        .pCbPrivData = &EvtConsumerPrivData,
        .EvtNum = IOC_calcArrayElmtCnt(EvtIDs),
        .pEvtIDs = EvtIDs,
    };

    IOC_ConnArgs_T ConnArgs = {
        .SrvURI = CSURI,
        .Usage = IOC_LinkUsageEvtConsumer,
    };

    std::thread EvtConsumerThread([&] {
        IOC_Result_T Result = IOC_connectService(&EvtConsumerLinkID, &ConnArgs, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // KeyVerifyPoint

        Result = IOC_subEVT(EvtConsumerLinkID, &SubEvtArgs);
        ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // KeyVerifyPoint
    });

    // Step-3
    Result = IOC_acceptClient(EvtProducerSrvID, &EvtProducerLinkID, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // KeyVerifyPoint

    EvtConsumerThread.join();

    // Step-4
    IOC_EvtDesc_T EvtDesc = {.EvtID = IOC_EVTID_TEST_KEEPALIVE};
    Result = IOC_postEVT(EvtProducerLinkID, &EvtDesc, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // KeyVerifyPoint

    IOC_forceProcEVT();

    // Step-5
    IOC_UnsubEvtArgs_T UnsubEvtArgs = {
        .CbProcEvt_F = __CbProcEvt_F,
        .pCbPrivData = &EvtConsumerPrivData,
    };
    Result = IOC_unsubEVT(EvtConsumerLinkID, &UnsubEvtArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);              // KeyVerifyPoint
    ASSERT_EQ(1, EvtConsumerPrivData.KeepAliveEvtCnt);  // KeyVerifyPoint

    // Step-6
    Result = IOC_postEVT(EvtProducerLinkID, &EvtDesc, NULL);
    ASSERT_EQ(IOC_RESULT_NO_EVENT_CONSUMER, Result);  // KeyVerifyPoint

    // Step-7
    Result = IOC_closeLink(EvtProducerLinkID);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // KeyVerifyPoint

    Result = IOC_closeLink(EvtConsumerLinkID);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // KeyVerifyPoint

    // Step-8
    Result = IOC_offlineService(EvtProducerSrvID);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // KeyVerifyPoint
}

/**
 * @[Name]:
 * <US1-AC2-TC2>verifySingleServiceMultiClients_byPostEvtAtSrvSide_bySubDiffEvtAtCliSide
 * @[Steps]:
 *   1) EvtProducer call IOC_onlineService() to online a service AS BEHAVIOR.
 *      |-> SrvArgs.UsageCapabilites = IOC_LinkUsageEvtProducer
 *      |-> SrvArgs.SrvURI = {IOC_SRV_PROTO_FIFO, IOC_SRV_HOST_LOCAL_PROCESS, "EvtProducer"}
 *   2) EvtConsumerA call IOC_connectLink() in standalone thread to the service AS BEHAVIOR.
 *          |-> ConnArgs.Usage = IOC_LinkUsageEvtConsumer
 *          |-> ConnArgs.SrvURI = SrvArgs.SrvURI
 *      a) Call IOC_subEVT(EVT_TEST_MOVE_STARTED/_KEEPING/_STOPPED) to subscribe an event AS BEHAVIOR.
 *   3) EvtConsumerB call IOC_connectLink() in standalone thread to the service AS BEHAVIOR.
 *          |-> ConnArgs.Usage = IOC_LinkUsageEvtConsumer
 *          |-> ConnArgs.SrvURI = SrvArgs.SrvURI
 *      a) Call IOC_subEVT(EVT_TEST_PULL_STARTED/_KEEPING/_STOPPED) to subscribe an event AS BEHAVIOR.
 *   4) EvtProducer call IOC_acceptLink() to accept the link AS BEHAVIOR.
 *          |-> EvtProducerLinkID4Consumer1, EvtProducerLinkID4Consumer2
 *   5) EvtProducer call IOC_postEVT() to post an event AS BEHAVIOR.
 *      |-> EvtDesc.EvtID = 1xEVT_TEST_MOVE_STARTED, nxEVT_TEST_MOVE_KEEPING, 1xEVT_TEST_MOVE_STOPPED
 *      |-> both to EvtProducerLinkID4Consumer1 and EvtProducerLinkID4Consumer2
 *      |-> call IOC_forceProcEVT() to process the event immediately.
 *      |-> ONLY EvtConsumerA will process the event as VERIFY.
 *          |-> EvtConsumerAPrivData.[Started|Stopped]EvtCnt = 1, [Keeping]EvtCnt = n AS VERIFY.
 *          |-> EvtConsumerBPrivData.[Started|Stopped]EvtCnt = 0, [Keeping]EvtCnt = 0 AS VERIFY.
 *   6) EvtProducer call IOC_postEVT() to post another event AS BEHAVIOR.
 *      |-> EvtDesc.EvtID = 1xEVT_TEST_PULL_STARTED, mxEVT_TEST_PULL_KEEPING, 1xEVT_TEST_PULL_STOPPED
 *      |-> both to EvtProducerLinkID4Consumer1 and EvtProducerLinkID4Consumer2
 *      |-> call IOC_forceProcEVT() to process the event immediately.
 *      |-> ONLY EvtConsumerB will process the event as VERIFY.
 *          |-> EvtConsumerAPrivData.[Started|Stopped]EvtCnt = 1, [Keeping]EvtCnt = n AS VERIFY.
 *          |-> EvtConsumerBPrivData.[Started|Stopped]EvtCnt = 1, [Keeping]EvtCnt = m AS VERIFY.
 *   7) EvtConsumerA/EvtConsumerB call IOC_unsubEVT() to unsubscribe the event AS BEHAVIOR.
 *   8) EvtProducer call IOC_postEVT() to post another event AS BEHAVIOR.
 *      |-> EvtDesc.EvtID = IOC_EVT_NAME_TEST_SLEEP_KEEPALIVE
 *      |-> get IOC_RESULT_NO_EVENT_CONSUMER AS VERIFY.
 *   9) EvtProducer/EvtConsumerA/EvtConsumerB call IOC_closeLink() to close the link AS BEHAVIOR.
 *   10) EvtProducer call IOC_offlineService() to offline the service AS BEHAVIOR.
 * @[Expect]:
 *    Sleep9MsEvtCnt=1, Sleep99MsEvtCnt=1, and NO_EVENT_CONSUMER are got.
 * @[Notes]:
 */

TEST(UT_ServiceTypical, verifySingleServiceMultiClients_byPostEvtAtSrvSide_bySubDiffEvtAtCliSide) {
    IOC_Result_T Result = IOC_RESULT_BUG;
    IOC_SrvID_T SrvID_Producer = IOC_ID_INVALID;

    IOC_LinkID_T CliLinkID_ConsumerA2Producer = IOC_ID_INVALID;
    IOC_LinkID_T SrvLinkID_Producer2ConsumerA = IOC_ID_INVALID;

    IOC_LinkID_T CliLinkID_ConsumerB2Producer = IOC_ID_INVALID;
    IOC_LinkID_T SrvLinkID_Producer2ConsumerB = IOC_ID_INVALID;

    IOC_SrvURI_T CSURI = {
        .pProtocol = IOC_SRV_PROTO_FIFO,
        .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
        .pPath = (const char*)"verifySingleServiceMultiClients",
    };

    // Step-1
    IOC_SrvArgs_T SrvArgs = {
        .SrvURI = CSURI,
        .UsageCapabilites = IOC_LinkUsageEvtProducer,
    };

    Result = IOC_onlineService(&SrvID_Producer, &SrvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

    // Step-2:
    IOC_ConnArgs_T ConnArgs = {
        .SrvURI = CSURI,
        .Usage = IOC_LinkUsageEvtConsumer,
    };

    __EvtConsumerPrivData_T ConsumerAPrivData = {0};

    std::thread EvtConsumerAThread([&] {
        IOC_EvtID_T EvtIDs4ConsumerA[] = {IOC_EVTID_TEST_MOVE_STARTED, IOC_EVTID_TEST_MOVE_KEEPING,
                                          IOC_EVTID_TEST_MOVE_STOPPED};
        IOC_SubEvtArgs_T SubEvtArgs4ConsumerA = {
            .CbProcEvt_F = __CbProcEvt_F,
            .pCbPrivData = &ConsumerAPrivData,
            .EvtNum = IOC_calcArrayElmtCnt(EvtIDs4ConsumerA),
            .pEvtIDs = EvtIDs4ConsumerA,
        };

        IOC_Result_T Result = IOC_connectService(&CliLinkID_ConsumerA2Producer, &ConnArgs, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

        Result = IOC_subEVT(CliLinkID_ConsumerA2Producer, &SubEvtArgs4ConsumerA);
        ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint
    });

    // Step-3:
    Result = IOC_acceptClient(SrvID_Producer, &SrvLinkID_Producer2ConsumerA, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

    EvtConsumerAThread.join();

    // Step-4
    __EvtConsumerPrivData_T ConsumerBPrivData = {0};

    std::thread EvtConsumerBThread([&] {
        IOC_EvtID_T EvtIDs4ConsumerB[] = {IOC_EVTID_TEST_PULL_STARTED, IOC_EVTID_TEST_PULL_KEEPING,
                                          IOC_EVTID_TEST_PULL_STOPPED};
        IOC_SubEvtArgs_T SubEvtArgs4ConsumerB = {
            .CbProcEvt_F = __CbProcEvt_F,
            .pCbPrivData = &ConsumerBPrivData,
            .EvtNum = IOC_calcArrayElmtCnt(EvtIDs4ConsumerB),
            .pEvtIDs = EvtIDs4ConsumerB,
        };

        IOC_Result_T Result = IOC_connectService(&CliLinkID_ConsumerB2Producer, &ConnArgs, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

        Result = IOC_subEVT(CliLinkID_ConsumerB2Producer, &SubEvtArgs4ConsumerB);
        ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint
    });

    Result = IOC_acceptClient(SrvID_Producer, &SrvLinkID_Producer2ConsumerB, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

    EvtConsumerBThread.join();

    // Step-5
    IOC_EvtDesc_T EvtDesc = {
        .EvtID = IOC_EVTID_TEST_MOVE_STARTED,
    };
    Result = IOC_postEVT(SrvLinkID_Producer2ConsumerA, &EvtDesc, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

#define _N_MOVE_KEEPING 3
    EvtDesc.EvtID = IOC_EVTID_TEST_MOVE_KEEPING;
    for (int i = 0; i < _N_MOVE_KEEPING; i++) {
        Result = IOC_postEVT(SrvLinkID_Producer2ConsumerA, &EvtDesc, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint
    }

    EvtDesc.EvtID = IOC_EVTID_TEST_MOVE_STOPPED;
    Result = IOC_postEVT(SrvLinkID_Producer2ConsumerA, &EvtDesc, NULL);

    IOC_forceProcEVT();
    ASSERT_EQ(1, ConsumerAPrivData.StartedEvtCnt);                // KeyVerifyPoint
    ASSERT_EQ(_N_MOVE_KEEPING, ConsumerAPrivData.KeepingEvtCnt);  // KeyVerifyPoint
    ASSERT_EQ(1, ConsumerAPrivData.StoppedEvtCnt);                // KeyVerifyPoint
    ASSERT_EQ(0, ConsumerBPrivData.StartedEvtCnt);                // KeyVerifyPoint
    ASSERT_EQ(0, ConsumerBPrivData.KeepingEvtCnt);                // KeyVerifyPoint
    ASSERT_EQ(0, ConsumerBPrivData.StoppedEvtCnt);                // KeyVerifyPoint

    // Step-6
    EvtDesc.EvtID = IOC_EVTID_TEST_PULL_STARTED;
    Result = IOC_postEVT(SrvLinkID_Producer2ConsumerB, &EvtDesc, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // KeyVerifyPoint

#define _M_PULL_KEEPING 5
    EvtDesc.EvtID = IOC_EVTID_TEST_PULL_KEEPING;
    for (int i = 0; i < _M_PULL_KEEPING; i++) {
        Result = IOC_postEVT(SrvLinkID_Producer2ConsumerB, &EvtDesc, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // KeyVerifyPoint
    }

    EvtDesc.EvtID = IOC_EVTID_TEST_PULL_STOPPED;
    Result = IOC_postEVT(SrvLinkID_Producer2ConsumerB, &EvtDesc, NULL);

    IOC_forceProcEVT();
    ASSERT_EQ(1, ConsumerAPrivData.StartedEvtCnt);                // KeyVerifyPoint
    ASSERT_EQ(_N_MOVE_KEEPING, ConsumerAPrivData.KeepingEvtCnt);  // KeyVerifyPoint
    ASSERT_EQ(1, ConsumerAPrivData.StoppedEvtCnt);                // KeyVerifyPoint
    ASSERT_EQ(1, ConsumerBPrivData.StartedEvtCnt);                // KeyVerifyPoint
    ASSERT_EQ(_M_PULL_KEEPING, ConsumerBPrivData.KeepingEvtCnt);  // KeyVerifyPoint
    ASSERT_EQ(1, ConsumerBPrivData.StoppedEvtCnt);                // KeyVerifyPoint

    // Step-7
    IOC_UnsubEvtArgs_T UnsubEvtArgs = {
        .CbProcEvt_F = __CbProcEvt_F,
        .pCbPrivData = &ConsumerAPrivData,
    };
    Result = IOC_unsubEVT(CliLinkID_ConsumerA2Producer, &UnsubEvtArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

    UnsubEvtArgs.pCbPrivData = &ConsumerBPrivData;
    Result = IOC_unsubEVT(CliLinkID_ConsumerB2Producer, &UnsubEvtArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

    // Step-8
    EvtDesc.EvtID = IOC_EVTID_TEST_KEEPALIVE;
    Result = IOC_postEVT(SrvLinkID_Producer2ConsumerA, &EvtDesc, NULL);
    ASSERT_EQ(IOC_RESULT_NO_EVENT_CONSUMER, Result);  // VerifyPoint

    Result = IOC_postEVT(SrvLinkID_Producer2ConsumerB, &EvtDesc, NULL);
    ASSERT_EQ(IOC_RESULT_NO_EVENT_CONSUMER, Result);  // VerifyPoint

    // Step-9
    Result = IOC_closeLink(CliLinkID_ConsumerA2Producer);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

    Result = IOC_closeLink(CliLinkID_ConsumerB2Producer);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

    Result = IOC_closeLink(SrvLinkID_Producer2ConsumerA);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

    Result = IOC_closeLink(SrvLinkID_Producer2ConsumerB);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

    // Step-10
    Result = IOC_offlineService(SrvID_Producer);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint
}

/**
 * @[Name]: <US1-AC3-TC3>verifyMultiServiceMultiClient_byPostEvtAtSrvSide_bySubDiffEvtAtCliSide
 * @[Steps]:
 *   1) EvtProducer1 call IOC_onlineService() to online a service AS BEHAVIOR.
 *      |-> SrvArgs.UsageCapabilites = IOC_LinkUsageEvtProducer
 *      |-> SrvArgs.SrvURI = {IOC_SRV_PROTO_FIFO, IOC_SRV_HOST_LOCAL_PROCESS, "EvtProducer1"}
 *   2) EvtProducer2 call IOC_onlineService() to online another service AS BEHAVIOR.
 *      |-> SrvArgs.UsageCapabilites = IOC_LinkUsageEvtProducer
 *      |-> SrvArgs.SrvURI = {IOC_SRV_PROTO_FIFO, IOC_SRV_HOST_LOCAL_PROCESS, "EvtProducer2"}
 *  3) EvtConsumerA call IOC_connectLink() in standalone thread to the service AS BEHAVIOR.
 *          |-> ConnArgs.Usage = IOC_LinkUsageEvtConsumer
 *          |-> ConnArgs.SrvURI = SrvArgs.SrvURI={IOC_SRV_PROTO_FIFO, IOC_SRV_HOST_LOCAL_PROCESS, "EvtProducer1"}
 *      a) Call IOC_subEVT(EVT_TEST_MOVE_STARTED/_KEEPING/_STOPPED) to subscribe an event AS BEHAVIOR.
 *  4) EvtConsumerB call IOC_connectLink() in standalone thread to the service AS BEHAVIOR.
 *          |-> ConnArgs.Usage = IOC_LinkUsageEvtConsumer
 *          |-> ConnArgs.SrvURI = SrvArgs.SrvURI={IOC_SRV_PROTO_FIFO, IOC_SRV_HOST_LOCAL_PROCESS, "EvtProducer2"}
 *      a) Call IOC_subEVT(EVT_TEST_PULL_STARTED/_KEEPING/_STOPPED) to subscribe an event AS BEHAVIOR.
 *  5) EvtConsumerC call IOC_connectLink() in standalone thread to the service AS BEHAVIOR.
 *         |-> ConnArgs.Usage = IOC_LinkUsageEvtConsumer
 *         |-> ConnArgs.SrvURI = SrvArgs.SrvURI={IOC_SRV_PROTO_FIFO, IOC_SRV_HOST_LOCAL_PROCESS, "EvtProducer1"}
 *         |-> ConnArgs.SrvURI = SrvArgs.SrvURI={IOC_SRV_PROTO_FIFO, IOC_SRV_HOST_LOCAL_PROCESS, "EvtProducer2"}
 *     a) Call IOC_subEVT(EVT_TEST_MOVE_STARTED/_KEEPING/_STOPPED) to subscribe an event AS BEHAVIOR.
 *     b) Call IOC_subEVT(EVT_TEST_PULL_STARTED/_KEEPING/_STOPPED) to subscribe an event AS BEHAVIOR.
 *  6) EvtProducer call IOC_acceptLink() to accept the link AS BEHAVIOR.
 *          |-> 1) EvtProducerLinkID4ConsumerA, EvtProducerLinkID4ConsumerC
 *          |-> 2) EvtProducerLinkID4ConsumerB, EvtProducerLinkID4ConsumerC
 *  7) EvtProducer1 call IOC_postEVT() to post an event AS BEHAVIOR.
 *      |-> EvtDesc.EvtID = 1xEVT_TEST_MOVE_STARTED, nxEVT_TEST_MOVE_KEEPING, 1xEVT_TEST_MOVE_STOPPED
 *      |-> ALL to EvtProducerLinkID4ConsumerA/C
 *      |-> call IOC_forceProcEVT() to process the event immediately.
 *      |-> ONLY EvtConsumerA/C will process the event as VERIFY.
 *          |-> EvtConsumer[A,C]PrivData.[Started|Stopped]EvtCnt = 1, [Keeping]EvtCnt = n AS VERIFY.
 *          |-> EvtConsumer[B]PrivData.[Started|Stopped]EvtCnt = 0, [Keeping]EvtCnt = 0 AS VERIFY.
 *  8) EvtProducer2 call IOC_postEVT() to post another event AS BEHAVIOR.
 *      |-> EvtDesc.EvtID = 1xEVT_TEST_PULL_STARTED, mxEVT_TEST_PULL_KEEPING, 1xEVT_TEST_PULL_STOPPED
 *      |-> ALL to EvtProducerLinkID4ConsumerB/C
 *      |-> call IOC_forceProcEVT() to process the event immediately.
 *      |-> ONLY EvtConsumerB/C will process the event as VERIFY.
 *          |-> EvtConsumer[A]PrivData.[Started|Stopped]EvtCnt = 1, [Keeping]EvtCnt = n AS VERIFY.
 *          |-> EvtConsumer[B]PrivData.[Started|Stopped]EvtCnt = 1, [Keeping]EvtCnt = m AS VERIFY.
 *          |-> EvtConsumer[C]PrivData.[Started|Stopped]EvtCnt = 1+1, [Keeping]EvtCnt = n+m AS VERIFY.
 *  9) EvtProducer1/2 call IOC_postEVT() to post another event AS BEHAVIOR.
 *     |-> EvtDesc.EvtID = 1xEVT_TEST_PUSH_STARTED, pxEVT_TEST_PUSH_KEEPING, 1xEVT_TEST_PUSH_STOPPED
 *     |-> ALL to EvtProducerLinkID4Consumer[A/B,B/C]
 *     |-> call IOC_forceProcEVT() to process the event immediately.
 *     |-> NONE of EvtConsumer[A,B,C] will process the event as VERIFY.
 *         |-> EvtConsumer[A]PrivData.[Started|Stopped]EvtCnt = 1, [Keeping]EvtCnt = n AS VERIFY.
 *         |-> EvtConsumer[B]PrivData.[Started|Stopped]EvtCnt = 1, [Keeping]EvtCnt = m AS VERIFY.
 *         |-> EvtConsumer[C]PrivData.[Started|Stopped]EvtCnt = 1+1, [Keeping]EvtCnt = n+m AS VERIFY.
 *  10) EvtConsumerA/EvtConsumerB/EvtConsumerC call IOC_unsubEVT() to unsubscribe the event AS BEHAVIOR.
 *  11) EvtProducer call IOC_postEVT() to post another event AS BEHAVIOR.
 *   |-> EvtDesc.EvtID = IOC_EVT_NAME_TEST_SLEEP_KEEPALIVE
 *   |-> get IOC_RESULT_NO_EVENT_CONSUMER AS VERIFY.
 *  12) EvtProducer[1,2]/EvtConsumerA/EvtConsumerB/EvtConsumerC call IOC_closeLink() to close the link AS BEHAVIOR.
 *  13) EvtProducer[1,2] call IOC_offlineService() to offline the service AS BEHAVIOR.
 * @[Expect]:
 *    EvtConsumer[A,B,C] will process the events as expected.
 * @[Notes]:
 */

TEST(UT_ServiceTypical, verifyMultiServiceMultiClient_byPostEvtAtSrvSide_bySubDiffEvtAtCliSide) {
    IOC_Result_T Result = IOC_RESULT_BUG;
    IOC_SrvID_T SrvID_Producer1 = IOC_ID_INVALID;
    IOC_SrvID_T SrvID_Producer2 = IOC_ID_INVALID;
    IOC_LinkID_T CliLinkID_ConsumerA_toProducer1 = IOC_ID_INVALID;
    IOC_LinkID_T CliLinkID_ConsumerB_toProducer2 = IOC_ID_INVALID;
    IOC_LinkID_T CliLinkID_ConsumerC_toProducer1 = IOC_ID_INVALID;
    IOC_LinkID_T CliLinkID_ConsumerC_toProducer2 = IOC_ID_INVALID;

    IOC_SrvURI_T CSURI1 = {
        .pProtocol = IOC_SRV_PROTO_FIFO,
        .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
        .pPath = (const char*)"verifyMultiServiceMultiClient1",
    };

    IOC_SrvURI_T CSURI2 = {
        .pProtocol = IOC_SRV_PROTO_FIFO,
        .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
        .pPath = (const char*)"verifyMultiServiceMultiClient2",
    };

    // Step-1: online the first service Producer1
    IOC_SrvArgs_T SrvArgs1 = {.SrvURI = CSURI1, .UsageCapabilites = IOC_LinkUsageEvtProducer};

    Result = IOC_onlineService(&SrvID_Producer1, &SrvArgs1);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

    // Step-2: online the second service Producer2
    IOC_SrvArgs_T SrvArgs2 = {.SrvURI = CSURI2, .UsageCapabilites = IOC_LinkUsageEvtProducer};

    Result = IOC_onlineService(&SrvID_Producer2, &SrvArgs2);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

    // Step-3: ConsumerA connect to the first service Producer1
    __EvtConsumerPrivData_T PrivDataConsumerA = {.StartedEvtCnt = 0, .KeepingEvtCnt = 0, .StoppedEvtCnt = 0};
    IOC_EvtID_T EvtIDs4ConsumerA[] = {IOC_EVTID_TEST_MOVE_STARTED, IOC_EVTID_TEST_MOVE_KEEPING,
                                      IOC_EVTID_TEST_MOVE_STOPPED};
    IOC_SubEvtArgs_T SubEvtArgs4ConsumerA = {.CbProcEvt_F = __CbProcEvt_F,
                                             .pCbPrivData = &PrivDataConsumerA,
                                             .EvtNum = IOC_calcArrayElmtCnt(EvtIDs4ConsumerA),
                                             .pEvtIDs = EvtIDs4ConsumerA};

    IOC_ConnArgs_T ConnArgs = {.Usage = IOC_LinkUsageEvtConsumer};

    std::thread Thread_ConsumerA_connectProducer1([&] {
        ConnArgs.SrvURI = CSURI1;
        IOC_Result_T Result = IOC_connectService(&CliLinkID_ConsumerA_toProducer1, &ConnArgs, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

        Result = IOC_subEVT(CliLinkID_ConsumerA_toProducer1, &SubEvtArgs4ConsumerA);
        ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint
    });

    IOC_LinkID_T SrvLinkID_Producer1_fromConsumerA = IOC_ID_INVALID;
    Result = IOC_acceptClient(SrvID_Producer1, &SrvLinkID_Producer1_fromConsumerA, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint
    Thread_ConsumerA_connectProducer1.join();

    // Step-4: ConsumerB connect to the second service Producer2
    __EvtConsumerPrivData_T PrivDataConsumerB = {.StartedEvtCnt = 0, .KeepingEvtCnt = 0, .StoppedEvtCnt = 0};
    IOC_EvtID_T EvtIDs4ConsumerB[] = {IOC_EVTID_TEST_PULL_STARTED, IOC_EVTID_TEST_PULL_KEEPING,
                                      IOC_EVTID_TEST_PULL_STOPPED};
    IOC_SubEvtArgs_T SubEvtArgs4ConsumerB = {.CbProcEvt_F = __CbProcEvt_F,
                                             .pCbPrivData = &PrivDataConsumerB,
                                             .EvtNum = IOC_calcArrayElmtCnt(EvtIDs4ConsumerB),
                                             .pEvtIDs = EvtIDs4ConsumerB};

    std::thread Thread_ConsumerB_connectProducer2([&] {
        ConnArgs.SrvURI = CSURI2;
        IOC_Result_T Result = IOC_connectService(&CliLinkID_ConsumerB_toProducer2, &ConnArgs, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

        Result = IOC_subEVT(CliLinkID_ConsumerB_toProducer2, &SubEvtArgs4ConsumerB);
        ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint
    });

    IOC_LinkID_T SrvLinkID_Producer2_fromConsumerB = IOC_ID_INVALID;
    Result = IOC_acceptClient(SrvID_Producer2, &SrvLinkID_Producer2_fromConsumerB, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint
    Thread_ConsumerB_connectProducer2.join();

    // Step-5: ConsumerC connect to Producer1 and Producer2
    __EvtConsumerPrivData_T PrivDataConsumerC = {.StartedEvtCnt = 0, .KeepingEvtCnt = 0, .StoppedEvtCnt = 0};

    IOC_EvtID_T EvtIDs4ConsumerC[] = {IOC_EVTID_TEST_MOVE_STARTED, IOC_EVTID_TEST_MOVE_KEEPING,
                                      IOC_EVTID_TEST_MOVE_STOPPED, IOC_EVTID_TEST_PULL_STARTED,
                                      IOC_EVTID_TEST_PULL_KEEPING, IOC_EVTID_TEST_PULL_STOPPED};
    IOC_SubEvtArgs_T SubEvtArgs4ConsumerC = {.CbProcEvt_F = __CbProcEvt_F,
                                             .pCbPrivData = &PrivDataConsumerC,
                                             .EvtNum = IOC_calcArrayElmtCnt(EvtIDs4ConsumerC),
                                             .pEvtIDs = EvtIDs4ConsumerC};

    std::thread Thread_ConsumerC_connectProducer1([&] {
        ConnArgs.SrvURI = CSURI1;
        IOC_Result_T Result = IOC_connectService(&CliLinkID_ConsumerC_toProducer1, &ConnArgs, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

        Result = IOC_subEVT(CliLinkID_ConsumerC_toProducer1, &SubEvtArgs4ConsumerC);
        ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint
    });

    IOC_LinkID_T SrvLinkID_Producer1_fromConsumerC = IOC_ID_INVALID;
    Result = IOC_acceptClient(SrvID_Producer1, &SrvLinkID_Producer1_fromConsumerC, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint
    Thread_ConsumerC_connectProducer1.join();

    std::thread Thread_ConsumerC_connectProducer2([&] {
        ConnArgs.SrvURI = CSURI2;
        IOC_Result_T Result = IOC_connectService(&CliLinkID_ConsumerC_toProducer2, &ConnArgs, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

        Result = IOC_subEVT(CliLinkID_ConsumerC_toProducer2, &SubEvtArgs4ConsumerC);
        ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint
    });

    IOC_LinkID_T SrvLinkID_Producer2_fromConsumerC = IOC_ID_INVALID;
    Result = IOC_acceptClient(SrvID_Producer2, &SrvLinkID_Producer2_fromConsumerC, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint
    Thread_ConsumerC_connectProducer2.join();

    // Step-7: post events to ConsumerA
    IOC_EvtDesc_T EvtDesc = {.EvtID = IOC_EVTID_TEST_MOVE_STARTED};
    Result = IOC_postEVT(SrvLinkID_Producer1_fromConsumerA, &EvtDesc, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

#define _N_MOVE_KEEPING 3
    EvtDesc.EvtID = IOC_EVTID_TEST_MOVE_KEEPING;
    for (int i = 0; i < _N_MOVE_KEEPING; i++) {
        Result = IOC_postEVT(SrvLinkID_Producer1_fromConsumerA, &EvtDesc, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint
    }

    EvtDesc.EvtID = IOC_EVTID_TEST_MOVE_STOPPED;
    Result = IOC_postEVT(SrvLinkID_Producer1_fromConsumerA, &EvtDesc, NULL);

    IOC_forceProcEVT();
    ASSERT_EQ(1, PrivDataConsumerA.StartedEvtCnt);                // KeyVerifyPoint
    ASSERT_EQ(_N_MOVE_KEEPING, PrivDataConsumerA.KeepingEvtCnt);  // KeyVerifyPoint
    ASSERT_EQ(1, PrivDataConsumerA.StoppedEvtCnt);                // KeyVerifyPoint
    ASSERT_EQ(0, PrivDataConsumerB.StartedEvtCnt);                // KeyVerifyPoint
    ASSERT_EQ(0, PrivDataConsumerB.KeepingEvtCnt);                // KeyVerifyPoint
    ASSERT_EQ(0, PrivDataConsumerB.StoppedEvtCnt);                // KeyVerifyPoint
    ASSERT_EQ(0, PrivDataConsumerC.StartedEvtCnt);                // KeyVerifyPoint
    ASSERT_EQ(0, PrivDataConsumerC.KeepingEvtCnt);                // KeyVerifyPoint
    ASSERT_EQ(0, PrivDataConsumerC.StoppedEvtCnt);                // KeyVerifyPoint

    // Step-8: post events to ConsumerB
    EvtDesc.EvtID = IOC_EVTID_TEST_PULL_STARTED;
    Result = IOC_postEVT(SrvLinkID_Producer2_fromConsumerB, &EvtDesc, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // KeyVerifyPoint

#define _M_PULL_KEEPING 5
    EvtDesc.EvtID = IOC_EVTID_TEST_PULL_KEEPING;
    for (int i = 0; i < _M_PULL_KEEPING; i++) {
        Result = IOC_postEVT(SrvLinkID_Producer2_fromConsumerB, &EvtDesc, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // KeyVerifyPoint
    }

    EvtDesc.EvtID = IOC_EVTID_TEST_PULL_STOPPED;
    Result = IOC_postEVT(SrvLinkID_Producer2_fromConsumerB, &EvtDesc, NULL);

    IOC_forceProcEVT();
    ASSERT_EQ(1, PrivDataConsumerA.StartedEvtCnt);                // KeyVerifyPoint
    ASSERT_EQ(_N_MOVE_KEEPING, PrivDataConsumerA.KeepingEvtCnt);  // KeyVerifyPoint
    ASSERT_EQ(1, PrivDataConsumerA.StoppedEvtCnt);                // KeyVerifyPoint
    ASSERT_EQ(1, PrivDataConsumerB.StartedEvtCnt);                // KeyVerifyPoint
    ASSERT_EQ(_M_PULL_KEEPING, PrivDataConsumerB.KeepingEvtCnt);  // KeyVerifyPoint
    ASSERT_EQ(1, PrivDataConsumerB.StoppedEvtCnt);                // KeyVerifyPoint
    ASSERT_EQ(0, PrivDataConsumerC.StartedEvtCnt);                // KeyVerifyPoint
    ASSERT_EQ(0, PrivDataConsumerC.KeepingEvtCnt);                // KeyVerifyPoint
    ASSERT_EQ(0, PrivDataConsumerC.StoppedEvtCnt);                // KeyVerifyPoint

    // Step-9: post events to ConsumerC
    EvtDesc.EvtID = IOC_EVTID_TEST_MOVE_STARTED;
    Result = IOC_postEVT(SrvLinkID_Producer1_fromConsumerC, &EvtDesc, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

    for (int i = 0; i < _N_MOVE_KEEPING; i++) {
        EvtDesc.EvtID = IOC_EVTID_TEST_MOVE_KEEPING;
        Result = IOC_postEVT(SrvLinkID_Producer1_fromConsumerC, &EvtDesc, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint
    }

    EvtDesc.EvtID = IOC_EVTID_TEST_MOVE_STOPPED;
    Result = IOC_postEVT(SrvLinkID_Producer1_fromConsumerC, &EvtDesc, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

    EvtDesc.EvtID = IOC_EVTID_TEST_PULL_STARTED;
    Result = IOC_postEVT(SrvLinkID_Producer2_fromConsumerC, &EvtDesc, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

    for (int i = 0; i < _M_PULL_KEEPING; i++) {
        EvtDesc.EvtID = IOC_EVTID_TEST_PULL_KEEPING;
        Result = IOC_postEVT(SrvLinkID_Producer2_fromConsumerC, &EvtDesc, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint
    }

    EvtDesc.EvtID = IOC_EVTID_TEST_PULL_STOPPED;
    Result = IOC_postEVT(SrvLinkID_Producer2_fromConsumerC, &EvtDesc, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

    IOC_forceProcEVT();
    ASSERT_EQ(1, PrivDataConsumerA.StartedEvtCnt);                                  // KeyVerifyPoint
    ASSERT_EQ(_N_MOVE_KEEPING, PrivDataConsumerA.KeepingEvtCnt);                    // KeyVerifyPoint
    ASSERT_EQ(1, PrivDataConsumerA.StoppedEvtCnt);                                  // KeyVerifyPoint
    ASSERT_EQ(1, PrivDataConsumerB.StartedEvtCnt);                                  // KeyVerifyPoint
    ASSERT_EQ(_M_PULL_KEEPING, PrivDataConsumerB.KeepingEvtCnt);                    // KeyVerifyPoint
    ASSERT_EQ(1, PrivDataConsumerB.StoppedEvtCnt);                                  // KeyVerifyPoint
    ASSERT_EQ(1 + 1, PrivDataConsumerC.StartedEvtCnt);                              // KeyVerifyPoint
    ASSERT_EQ(_N_MOVE_KEEPING + _M_PULL_KEEPING, PrivDataConsumerC.KeepingEvtCnt);  // KeyVerifyPoint
    ASSERT_EQ(1 + 1, PrivDataConsumerC.StoppedEvtCnt);                              // KeyVerifyPoint

    // Step-9.1: post TEST_PUSH events to Consumer[A,B,C]
    EvtDesc.EvtID = IOC_EVTID_TEST_PUSH_STARTED;
    IOC_postEVT(SrvLinkID_Producer1_fromConsumerA, &EvtDesc, NULL);
    IOC_postEVT(SrvLinkID_Producer2_fromConsumerB, &EvtDesc, NULL);
    IOC_postEVT(SrvLinkID_Producer1_fromConsumerC, &EvtDesc, NULL);
    IOC_postEVT(SrvLinkID_Producer2_fromConsumerC, &EvtDesc, NULL);

    EvtDesc.EvtID = IOC_EVTID_TEST_PUSH_KEEPING;
#define _P_PUSH_KEEPING 7
    for (int i = 0; i < _P_PUSH_KEEPING; i++) {
        IOC_postEVT(SrvLinkID_Producer1_fromConsumerA, &EvtDesc, NULL);
        IOC_postEVT(SrvLinkID_Producer2_fromConsumerB, &EvtDesc, NULL);
        IOC_postEVT(SrvLinkID_Producer1_fromConsumerC, &EvtDesc, NULL);
        IOC_postEVT(SrvLinkID_Producer2_fromConsumerC, &EvtDesc, NULL);
    }

    IOC_EVTID_TEST_PUSH_STOPPED;
    IOC_postEVT(SrvLinkID_Producer1_fromConsumerA, &EvtDesc, NULL);
    IOC_postEVT(SrvLinkID_Producer2_fromConsumerB, &EvtDesc, NULL);
    IOC_postEVT(SrvLinkID_Producer1_fromConsumerC, &EvtDesc, NULL);
    IOC_postEVT(SrvLinkID_Producer2_fromConsumerC, &EvtDesc, NULL);

    IOC_forceProcEVT();
    ASSERT_EQ(1, PrivDataConsumerA.StartedEvtCnt);                                  // KeyVerifyPoint
    ASSERT_EQ(_N_MOVE_KEEPING, PrivDataConsumerA.KeepingEvtCnt);                    // KeyVerifyPoint
    ASSERT_EQ(1, PrivDataConsumerA.StoppedEvtCnt);                                  // KeyVerifyPoint
    ASSERT_EQ(1, PrivDataConsumerB.StartedEvtCnt);                                  // KeyVerifyPoint
    ASSERT_EQ(_M_PULL_KEEPING, PrivDataConsumerB.KeepingEvtCnt);                    // KeyVerifyPoint
    ASSERT_EQ(1, PrivDataConsumerB.StoppedEvtCnt);                                  // KeyVerifyPoint
    ASSERT_EQ(1 + 1, PrivDataConsumerC.StartedEvtCnt);                              // KeyVerifyPoint
    ASSERT_EQ(_N_MOVE_KEEPING + _M_PULL_KEEPING, PrivDataConsumerC.KeepingEvtCnt);  // KeyVerifyPoint
    ASSERT_EQ(1 + 1, PrivDataConsumerC.StoppedEvtCnt);                              // KeyVerifyPoint

    // Step-10
    IOC_UnsubEvtArgs_T UnsubEvtArgs = {.CbProcEvt_F = __CbProcEvt_F, .pCbPrivData = &PrivDataConsumerA};
    Result = IOC_unsubEVT(CliLinkID_ConsumerA_toProducer1, &UnsubEvtArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

    UnsubEvtArgs.pCbPrivData = &PrivDataConsumerB;
    Result = IOC_unsubEVT(CliLinkID_ConsumerB_toProducer2, &UnsubEvtArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

    UnsubEvtArgs.pCbPrivData = &PrivDataConsumerC;
    Result = IOC_unsubEVT(CliLinkID_ConsumerC_toProducer1, &UnsubEvtArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

    Result = IOC_unsubEVT(CliLinkID_ConsumerC_toProducer2, &UnsubEvtArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

    // Step-11
    EvtDesc.EvtID = IOC_EVTID_TEST_KEEPALIVE;
    Result = IOC_postEVT(SrvLinkID_Producer1_fromConsumerA, &EvtDesc, NULL);
    ASSERT_EQ(IOC_RESULT_NO_EVENT_CONSUMER, Result);  // VerifyPoint

    Result = IOC_postEVT(SrvLinkID_Producer2_fromConsumerB, &EvtDesc, NULL);
    ASSERT_EQ(IOC_RESULT_NO_EVENT_CONSUMER, Result);  // VerifyPoint

    Result = IOC_postEVT(SrvLinkID_Producer1_fromConsumerC, &EvtDesc, NULL);
    ASSERT_EQ(IOC_RESULT_NO_EVENT_CONSUMER, Result);  // VerifyPoint

    Result = IOC_postEVT(SrvLinkID_Producer2_fromConsumerC, &EvtDesc, NULL);
    ASSERT_EQ(IOC_RESULT_NO_EVENT_CONSUMER, Result);  // VerifyPoint

    // Step-12: close all server and client links
    Result = IOC_closeLink(CliLinkID_ConsumerA_toProducer1);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

    Result = IOC_closeLink(CliLinkID_ConsumerB_toProducer2);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

    Result = IOC_closeLink(CliLinkID_ConsumerC_toProducer1);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

    Result = IOC_closeLink(CliLinkID_ConsumerC_toProducer2);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

    Result = IOC_closeLink(SrvLinkID_Producer1_fromConsumerA);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

    Result = IOC_closeLink(SrvLinkID_Producer1_fromConsumerC);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

    Result = IOC_closeLink(SrvLinkID_Producer2_fromConsumerB);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

    Result = IOC_closeLink(SrvLinkID_Producer2_fromConsumerC);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

    // Step-13: offline all services
    Result = IOC_offlineService(SrvID_Producer1);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

    Result = IOC_offlineService(SrvID_Producer2);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint
}

/**
 * @[Name]: <US2-AC1-TC4>verifyMultiServiceMultiClient_byPostEvtAtCliSide_bySubDiffEvtAtSrvSide
 * @[Steps]:
 *  1) EvtConsumerA call IOC_onlineService() to online a service AS BEHAVIOR.
 *      |-> SrvArgs.UsageCapabilites = IOC_LinkUsageEvtConsumer
 *      |-> SrvArgs.SrvURI = {IOC_SRV_PROTO_FIFO, IOC_SRV_HOST_LOCAL_PROCESS, "EvtConsumerA"}
 *  2) EvtConsumerB call IOC_onlineService() to online another service AS BEHAVIOR.
 *      |-> SrvArgs.UsageCapabilites = IOC_LinkUsageEvtConsumer
 *      |-> SrvArgs.SrvURI = {IOC_SRV_PROTO_FIFO, IOC_SRV_HOST_LOCAL_PROCESS, "EvtConsumerB"}
 *  3) EvtProducer1 call IOC_connectLink() in standalone thread to the service AS BEHAVIOR.
 *          |-> ConnArgs.Usage = IOC_LinkUsageEvtProducer
 *          |-> ConnArgs.SrvURI = SrvArgs.SrvURI={IOC_SRV_PROTO_FIFO, IOC_SRV_HOST_LOCAL_PROCESS, "EvtConsumerA"}
 *          |-> CliLinkID_Producer1_toConsumerA
 *     a) EvtConsumerA call IOC_acceptLink() to accept the link AS BEHAVIOR.
 *          |-> SrvLinkID_ConsumerA_fromProducer1
 *          |-> call IOC_subEVT(EVT_TEST_MOVE_STARTED/_KEEPING/_STOPPED) to subscribe an event AS BEHAVIOR.
 *  4) EvtProducer2 call IOC_connectLink() in standalone thread to the service AS BEHAVIOR.
 *          |-> ConnArgs.Usage = IOC_LinkUsageEvtProducer
 *          |-> ConnArgs.SrvURI = SrvArgs.SrvURI={IOC_SRV_PROTO_FIFO, IOC_SRV_HOST_LOCAL_PROCESS, "EvtConsumerB"}
 *          |-> CliLinkID_Producer2_toConsumerB
 *     a) EvtConsumerB call IOC_acceptLink() to accept the link AS BEHAVIOR.
 *          |-> SrvLinkID_ConsumerB_fromProducer2
 *          |-> call IOC_subEVT(EVT_TEST_PULL_STARTED/_KEEPING/_STOPPED) to subscribe an event AS BEHAVIOR.
 *  5) EvtProducer3 call IOC_connectLink() in standalone thread to the service AS BEHAVIOR.
 *          |-> ConnArgs.Usage = IOC_LinkUsageEvtProducer
 *          |-> ConnArgs.SrvURI = SrvArgs.SrvURI={IOC_SRV_PROTO_FIFO, IOC_SRV_HOST_LOCAL_PROCESS, "EvtConsumerA"}
 *          |-> ConnArgs.SrvURI = SrvArgs.SrvURI={IOC_SRV_PROTO_FIFO, IOC_SRV_HOST_LOCAL_PROCESS, "EvtConsumerB"}
 *          |-> CliLinkID_Producer3_toConsumerA, CliLinkID_Producer3_toConsumerB
 *     a) EvtConsumerA call IOC_acceptLink() to accept the link AS BEHAVIOR.
 *          |-> SrvLinkID_ConsumerA_fromProducer3
 *          |-> call IOC_subEVT(EVT_TEST_MOVE_STARTED/_KEEPING/_STOPPED) to subscribe an event AS BEHAVIOR.
 *     b) EvtConsumerB call IOC_acceptLink() to accept the link AS BEHAVIOR.
 *          |-> SrvLinkID_ConsumerB_fromProducer3
 *          |-> call IOC_subEVT(EVT_TEST_PULL_STARTED/_KEEPING/_STOPPED) to subscribe an event AS BEHAVIOR.
 *  6) EvtProducer1 call IOC_postEVT() to post an event AS BEHAVIOR.
 *      |-> EvtDesc.EvtID = 1xEVT_TEST_MOVE_STARTED, nxEVT_TEST_MOVE_KEEPING, 1xEVT_TEST_MOVE_STOPPED
 *      |-> ALL to CliLinkID_Producer1_toConsumerA
 *      |-> call IOC_forceProcEVT() to process the event immediately.
 *      |-> ONLY EvtConsumerA will process the event as VERIFY.
 *      |-> EvtConsumerAPrivData.[Started|Stopped]EvtCnt = 1, [Keeping]EvtCnt = n AS VERIFY.
 *  7) EvtProducer2 call IOC_postEVT() to post another event AS BEHAVIOR.
 *      |-> EvtDesc.EvtID = 1xEVT_TEST_PULL_STARTED, mxEVT_TEST_PULL_KEEPING, 1xEVT_TEST_PULL_STOPPED
 *      |-> ALL to CliLinkID_Producer2_toConsumerB
 *      |-> call IOC_forceProcEVT() to process the event immediately.
 *      |-> ONLY EvtConsumerB will process the event as VERIFY.
 *      |-> EvtConsumerBPrivData.[Started|Stopped]EvtCnt = 1, [Keeping]EvtCnt = m AS VERIFY.
 *  8) EvtProducer3 call IOC_postEVT() to post another event AS BEHAVIOR.
 *     |-> EvtDesc.EvtID = 1xEVT_TEST_PUSH_STARTED, pxEVT_TEST_PUSH_KEEPING, 1xEVT_TEST_PUSH_STOPPED
 *     |-> ALL to CliLinkID_Producer3_toConsumer[A,B]
 *     |-> call IOC_forceProcEVT() to process the event immediately.
 *     |-> EvtConsumerAPrivData.[Started|Stopped]EvtCnt = 1, [Keeping]EvtCnt = n AS VERIFY.
 *     |-> EvtConsumerBPrivData.[Started|Stopped]EvtCnt = 1, [Keeping]EvtCnt = m AS VERIFY.
 *  9) EvtConsumer[A,B] call IOC_unsubEVT() to unsubscribe the event AS BEHAVIOR.
 *  10) EvtProducer[1,2,3]/EvtConsumer[A,B] call IOC_closeLink() to close the link AS BEHAVIOR.
 *  11) EvtConsumer[A,B] call IOC_offlineService() to offline the service AS BEHAVIOR.
 * @[Expect]:
 *    EvtConsumer[A,B] will process the events as expected.
 * @[Notes]:
 */

TEST(UT_ServiceTypical, verifyMultiServiceMultiClient_byPostEvtAtCliSide_bySubDiffEvtAtSrvSide) {
    IOC_Result_T Result = IOC_RESULT_BUG;
    IOC_SrvID_T SrvID_ConsumerA = IOC_ID_INVALID;
    IOC_SrvID_T SrvID_ConsumerB = IOC_ID_INVALID;
    IOC_LinkID_T CliLinkID_Producer1_toConsumerA = IOC_ID_INVALID;
    IOC_LinkID_T CliLinkID_Producer2_toConsumerB = IOC_ID_INVALID;
    IOC_LinkID_T CliLinkID_Producer3_toConsumerA = IOC_ID_INVALID;
    IOC_LinkID_T CliLinkID_Producer3_toConsumerB = IOC_ID_INVALID;

    IOC_SrvURI_T CSURI1 = {
        .pProtocol = IOC_SRV_PROTO_FIFO,
        .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
        .pPath = (const char*)"EvtConsumerA",
    };

    IOC_SrvURI_T CSURI2 = {
        .pProtocol = IOC_SRV_PROTO_FIFO,
        .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
        .pPath = (const char*)"EvtConsumerB",
    };

    // Step-1: online the first service ConsumerA
    IOC_SrvArgs_T SrvArgs1 = {.SrvURI = CSURI1, .UsageCapabilites = IOC_LinkUsageEvtConsumer};

    Result = IOC_onlineService(&SrvID_ConsumerA, &SrvArgs1);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

    // Step-2: online the second service ConsumerB
    IOC_SrvArgs_T SrvArgs2 = {.SrvURI = CSURI2, .UsageCapabilites = IOC_LinkUsageEvtConsumer};

    Result = IOC_onlineService(&SrvID_ConsumerB, &SrvArgs2);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

    // Step-3: Producer1 connect to ConsumerA
    IOC_ConnArgs_T ConnArgs = {.Usage = IOC_LinkUsageEvtProducer};

    std::thread Thread_Producer1_connectConsumerA([&] {
        ConnArgs.SrvURI = CSURI1;
        IOC_Result_T Result = IOC_connectService(&CliLinkID_Producer1_toConsumerA, &ConnArgs, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint
    });

    IOC_LinkID_T SrvLinkID_ConsumerA_fromProducer1 = IOC_ID_INVALID;

    Result = IOC_acceptClient(SrvID_ConsumerA, &SrvLinkID_ConsumerA_fromProducer1, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

    __EvtConsumerPrivData_T PrivDataConsumerA_fromProducer1 = {
        .StartedEvtCnt = 0, .KeepingEvtCnt = 0, .StoppedEvtCnt = 0};
    IOC_EvtID_T EvtIDs4ConsumerA[] = {IOC_EVTID_TEST_MOVE_STARTED, IOC_EVTID_TEST_MOVE_KEEPING,
                                      IOC_EVTID_TEST_MOVE_STOPPED};
    IOC_SubEvtArgs_T SubEvtArgs4ConsumerA = {.CbProcEvt_F = __CbProcEvt_F,
                                             .pCbPrivData = &PrivDataConsumerA_fromProducer1,
                                             .EvtNum = IOC_calcArrayElmtCnt(EvtIDs4ConsumerA),
                                             .pEvtIDs = EvtIDs4ConsumerA};

    Result = IOC_subEVT(SrvLinkID_ConsumerA_fromProducer1, &SubEvtArgs4ConsumerA);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

    Thread_Producer1_connectConsumerA.join();

    // Step-4: Producer2 connect to ConsumerB
    std::thread Thread_Producer2_connectConsumerB([&] {
        ConnArgs.SrvURI = CSURI2;
        IOC_Result_T Result = IOC_connectService(&CliLinkID_Producer2_toConsumerB, &ConnArgs, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint
    });

    IOC_LinkID_T SrvLinkID_ConsumerB_fromProducer2 = IOC_ID_INVALID;

    Result = IOC_acceptClient(SrvID_ConsumerB, &SrvLinkID_ConsumerB_fromProducer2, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

    __EvtConsumerPrivData_T PrivDataConsumerB_fromProducer2 = {
        .StartedEvtCnt = 0, .KeepingEvtCnt = 0, .StoppedEvtCnt = 0};
    IOC_EvtID_T EvtIDs4ConsumerB[] = {IOC_EVTID_TEST_PULL_STARTED, IOC_EVTID_TEST_PULL_KEEPING,
                                      IOC_EVTID_TEST_PULL_STOPPED};
    IOC_SubEvtArgs_T SubEvtArgs4ConsumerB = {.CbProcEvt_F = __CbProcEvt_F,
                                             .pCbPrivData = &PrivDataConsumerB_fromProducer2,
                                             .EvtNum = IOC_calcArrayElmtCnt(EvtIDs4ConsumerB),
                                             .pEvtIDs = EvtIDs4ConsumerB};

    Result = IOC_subEVT(SrvLinkID_ConsumerB_fromProducer2, &SubEvtArgs4ConsumerB);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

    Thread_Producer2_connectConsumerB.join();

    // Step-5: Producer3 connect to ConsumerA and ConsumerB
    std::thread Thread_Producer3_connectConsumerA([&] {
        ConnArgs.SrvURI = CSURI1;
        IOC_Result_T Result = IOC_connectService(&CliLinkID_Producer3_toConsumerA, &ConnArgs, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

        ConnArgs.SrvURI = CSURI2;
        Result = IOC_connectService(&CliLinkID_Producer3_toConsumerB, &ConnArgs, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint
    });

    IOC_LinkID_T SrvLinkID_ConsumerA_fromProducer3 = IOC_ID_INVALID;
    Result = IOC_acceptClient(SrvID_ConsumerA, &SrvLinkID_ConsumerA_fromProducer3, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

    IOC_LinkID_T SrvLinkID_ConsumerB_fromProducer3 = IOC_ID_INVALID;
    Result = IOC_acceptClient(SrvID_ConsumerB, &SrvLinkID_ConsumerB_fromProducer3, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

    __EvtConsumerPrivData_T PrivDataConsumerA_formProducer3 = {
        .StartedEvtCnt = 0, .KeepingEvtCnt = 0, .StoppedEvtCnt = 0};
    // sub MOVE/PULL_EVT, but not PUSH_EVT, while post PUSH_EVT
    IOC_EvtID_T EvtIDs4ConsumerC[] = {IOC_EVTID_TEST_MOVE_STARTED, IOC_EVTID_TEST_MOVE_KEEPING,
                                      IOC_EVTID_TEST_MOVE_STOPPED, IOC_EVTID_TEST_PULL_STARTED,
                                      IOC_EVTID_TEST_PULL_KEEPING, IOC_EVTID_TEST_PULL_STOPPED};
    IOC_SubEvtArgs_T SubEvtArgs4ConsumerC = {.CbProcEvt_F = __CbProcEvt_F,
                                             .pCbPrivData = &PrivDataConsumerA_formProducer3,
                                             .EvtNum = IOC_calcArrayElmtCnt(EvtIDs4ConsumerC),
                                             .pEvtIDs = EvtIDs4ConsumerC};
    Result = IOC_subEVT(SrvLinkID_ConsumerA_fromProducer3, &SubEvtArgs4ConsumerC);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

    __EvtConsumerPrivData_T PrivDataConsumerB_fromProducer3 = {
        .StartedEvtCnt = 0, .KeepingEvtCnt = 0, .StoppedEvtCnt = 0};
    SubEvtArgs4ConsumerC.pCbPrivData = &PrivDataConsumerB_fromProducer3;
    Result = IOC_subEVT(SrvLinkID_ConsumerB_fromProducer3, &SubEvtArgs4ConsumerC);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

    Thread_Producer3_connectConsumerA.join();

    // Step-6: post events to ConsumerA
    IOC_EvtDesc_T EvtDesc = {.EvtID = IOC_EVTID_TEST_MOVE_STARTED};
    Result = IOC_postEVT(CliLinkID_Producer1_toConsumerA, &EvtDesc, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

#define _N_MOVE_KEEPING 3
    EvtDesc.EvtID = IOC_EVTID_TEST_MOVE_KEEPING;
    for (int i = 0; i < _N_MOVE_KEEPING; i++) {
        Result = IOC_postEVT(CliLinkID_Producer1_toConsumerA, &EvtDesc, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint
    }

    EvtDesc.EvtID = IOC_EVTID_TEST_MOVE_STOPPED;
    Result = IOC_postEVT(CliLinkID_Producer1_toConsumerA, &EvtDesc, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

    IOC_forceProcEVT();
    ASSERT_EQ(1, PrivDataConsumerA_fromProducer1.StartedEvtCnt);                // KeyVerifyPoint
    ASSERT_EQ(_N_MOVE_KEEPING, PrivDataConsumerA_fromProducer1.KeepingEvtCnt);  // KeyVerifyPoint
    ASSERT_EQ(1, PrivDataConsumerA_fromProducer1.StoppedEvtCnt);                // KeyVerifyPoint
    ASSERT_EQ(0, PrivDataConsumerB_fromProducer2.StartedEvtCnt);                // KeyVerifyPoint
    ASSERT_EQ(0, PrivDataConsumerB_fromProducer2.KeepingEvtCnt);                // KeyVerifyPoint
    ASSERT_EQ(0, PrivDataConsumerB_fromProducer2.StoppedEvtCnt);                // KeyVerifyPoint
    ASSERT_EQ(0, PrivDataConsumerA_formProducer3.StartedEvtCnt);                // KeyVerifyPoint
    ASSERT_EQ(0, PrivDataConsumerA_formProducer3.KeepingEvtCnt);                // KeyVerifyPoint
    ASSERT_EQ(0, PrivDataConsumerA_formProducer3.StoppedEvtCnt);                // KeyVerifyPoint
    ASSERT_EQ(0, PrivDataConsumerB_fromProducer3.StartedEvtCnt);                // KeyVerifyPoint
    ASSERT_EQ(0, PrivDataConsumerB_fromProducer3.KeepingEvtCnt);                // KeyVerifyPoint
    ASSERT_EQ(0, PrivDataConsumerB_fromProducer3.StoppedEvtCnt);                // KeyVerifyPoint

    // Step-7: post events to ConsumerB
    EvtDesc.EvtID = IOC_EVTID_TEST_PULL_STARTED;
    Result = IOC_postEVT(CliLinkID_Producer2_toConsumerB, &EvtDesc, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // KeyVerifyPoint

#define _M_PULL_KEEPING 5
    EvtDesc.EvtID = IOC_EVTID_TEST_PULL_KEEPING;
    for (int i = 0; i < _M_PULL_KEEPING; i++) {
        Result = IOC_postEVT(CliLinkID_Producer2_toConsumerB, &EvtDesc, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // KeyVerifyPoint
    }

    EvtDesc.EvtID = IOC_EVTID_TEST_PULL_STOPPED;
    Result = IOC_postEVT(CliLinkID_Producer2_toConsumerB, &EvtDesc, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // KeyVerifyPoint

    IOC_forceProcEVT();
    ASSERT_EQ(1, PrivDataConsumerA_fromProducer1.StartedEvtCnt);                // KeyVerifyPoint
    ASSERT_EQ(_N_MOVE_KEEPING, PrivDataConsumerA_fromProducer1.KeepingEvtCnt);  // KeyVerifyPoint
    ASSERT_EQ(1, PrivDataConsumerA_fromProducer1.StoppedEvtCnt);                // KeyVerifyPoint
    ASSERT_EQ(1, PrivDataConsumerB_fromProducer2.StartedEvtCnt);                // KeyVerifyPoint
    ASSERT_EQ(_M_PULL_KEEPING, PrivDataConsumerB_fromProducer2.KeepingEvtCnt);  // KeyVerifyPoint
    ASSERT_EQ(1, PrivDataConsumerB_fromProducer2.StoppedEvtCnt);                // KeyVerifyPoint
    ASSERT_EQ(0, PrivDataConsumerA_formProducer3.StartedEvtCnt);                // KeyVerifyPoint
    ASSERT_EQ(0, PrivDataConsumerA_formProducer3.KeepingEvtCnt);                // KeyVerifyPoint
    ASSERT_EQ(0, PrivDataConsumerA_formProducer3.StoppedEvtCnt);                // KeyVerifyPoint
    ASSERT_EQ(0, PrivDataConsumerB_fromProducer3.StartedEvtCnt);                // KeyVerifyPoint
    ASSERT_EQ(0, PrivDataConsumerB_fromProducer3.KeepingEvtCnt);                // KeyVerifyPoint
    ASSERT_EQ(0, PrivDataConsumerB_fromProducer3.StoppedEvtCnt);                // KeyVerifyPoint

    // Step-8: post TEST_PUSH events to Consumer[A,B]
    EvtDesc.EvtID = IOC_EVTID_TEST_PUSH_STARTED;
    IOC_postEVT(CliLinkID_Producer3_toConsumerA, &EvtDesc, NULL);
    IOC_postEVT(CliLinkID_Producer3_toConsumerB, &EvtDesc, NULL);

    EvtDesc.EvtID = IOC_EVTID_TEST_PUSH_KEEPING;
#define _P_PUSH_KEEPING 7
    for (int i = 0; i < _P_PUSH_KEEPING; i++) {
        IOC_postEVT(CliLinkID_Producer3_toConsumerA, &EvtDesc, NULL);
        IOC_postEVT(CliLinkID_Producer3_toConsumerB, &EvtDesc, NULL);
    }

    EvtDesc.EvtID = IOC_EVTID_TEST_PUSH_STOPPED;
    IOC_postEVT(CliLinkID_Producer3_toConsumerA, &EvtDesc, NULL);
    IOC_postEVT(CliLinkID_Producer3_toConsumerB, &EvtDesc, NULL);

    IOC_forceProcEVT();
    ASSERT_EQ(1, PrivDataConsumerA_fromProducer1.StartedEvtCnt);                // KeyVerifyPoint
    ASSERT_EQ(_N_MOVE_KEEPING, PrivDataConsumerA_fromProducer1.KeepingEvtCnt);  // KeyVerifyPoint
    ASSERT_EQ(1, PrivDataConsumerA_fromProducer1.StoppedEvtCnt);                // KeyVerifyPoint
    ASSERT_EQ(1, PrivDataConsumerB_fromProducer2.StartedEvtCnt);                // KeyVerifyPoint
    ASSERT_EQ(_M_PULL_KEEPING, PrivDataConsumerB_fromProducer2.KeepingEvtCnt);  // KeyVerifyPoint
    ASSERT_EQ(1, PrivDataConsumerB_fromProducer2.StoppedEvtCnt);                // KeyVerifyPoint
    ASSERT_EQ(0, PrivDataConsumerA_formProducer3.StartedEvtCnt);                // KeyVerifyPoint
    ASSERT_EQ(0, PrivDataConsumerA_formProducer3.KeepingEvtCnt);                // KeyVerifyPoint
    ASSERT_EQ(0, PrivDataConsumerA_formProducer3.StoppedEvtCnt);                // KeyVerifyPoint
    ASSERT_EQ(0, PrivDataConsumerB_fromProducer3.StartedEvtCnt);                // KeyVerifyPoint
    ASSERT_EQ(0, PrivDataConsumerB_fromProducer3.KeepingEvtCnt);                // KeyVerifyPoint
    ASSERT_EQ(0, PrivDataConsumerB_fromProducer3.StoppedEvtCnt);                // KeyVerifyPoint

    // Step-9
    IOC_UnsubEvtArgs_T UnsubEvtArgs = {.CbProcEvt_F = __CbProcEvt_F, .pCbPrivData = &PrivDataConsumerA_formProducer3};
    Result = IOC_unsubEVT(SrvLinkID_ConsumerA_fromProducer3, &UnsubEvtArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

    UnsubEvtArgs.pCbPrivData = &PrivDataConsumerB_fromProducer3;
    Result = IOC_unsubEVT(SrvLinkID_ConsumerB_fromProducer3, &UnsubEvtArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

    UnsubEvtArgs.pCbPrivData = &PrivDataConsumerA_fromProducer1;
    Result = IOC_unsubEVT(SrvLinkID_ConsumerA_fromProducer1, &UnsubEvtArgs);

    UnsubEvtArgs.pCbPrivData = &PrivDataConsumerB_fromProducer2;
    Result = IOC_unsubEVT(SrvLinkID_ConsumerB_fromProducer2, &UnsubEvtArgs);

    // Step-10
    Result = IOC_closeLink(CliLinkID_Producer1_toConsumerA);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

    Result = IOC_closeLink(CliLinkID_Producer2_toConsumerB);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

    Result = IOC_closeLink(CliLinkID_Producer3_toConsumerA);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

    Result = IOC_closeLink(CliLinkID_Producer3_toConsumerB);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

    Result = IOC_closeLink(SrvLinkID_ConsumerA_fromProducer1);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

    Result = IOC_closeLink(SrvLinkID_ConsumerA_fromProducer3);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

    Result = IOC_closeLink(SrvLinkID_ConsumerB_fromProducer2);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

    Result = IOC_closeLink(SrvLinkID_ConsumerB_fromProducer3);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

    // Step-11
    Result = IOC_offlineService(SrvID_ConsumerA);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

    Result = IOC_offlineService(SrvID_ConsumerB);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint
}

/**
 * @[Name]: <US1-AC1-TC5>verifyConsumerResubscribeEvent
 * @[Steps]:
 *   1) EvtProducer call IOC_onlineService() to online a service AS VERIFY.
 *      |-> SrvArgs.UsageCapabilites = IOC_LinkUsageEvtProducer
 *      |-> SrvArgs.SrvURI
 *          |-> pProtocol = IOC_SRV_PROTO_FIFO
 *          |-> pHost = IOC_SRV_HOST_LOCAL_PROCESS
 *          |-> pPath = "EvtProducer"
 *   2) EvtConsumer call IOC_connectLink() in standalone thread to the service AS VERIFY.
 *          |-> ConnArgs.Usage = IOC_LinkUsageEvtConsumer
 *          |-> ConnArgs.SrvURI = SrvArgs.SrvURI
 *      a) Call IOC_subEVT() to subscribe an event AS BEHAVIOR.
 *          |-> SubEvtArgs.EvtIDs = {IOC_EVT_NAME_TEST_KEEPALIVE}
 *   3) EvtProducer call IOC_acceptLink() to accept the link AS VERIFY.
 *   4) EvtProducer call IOC_postEVT() to post an event AS BEHAVIOR.
 *      |-> EvtDesc.EvtID = IOC_EVT_NAME_TEST_KEEPALIVE
 *      |-> call IOC_forceProcEVT() to process the event immediately.
 *      |-> EvtConsumerPrivData.KeepAliveEvtCnt = 1 AS VERIFY.
 *   5) EvtConsumer call IOC_unsubEVT() to unsubscribe the event AS BEHAVIOR.
 *   6) EvtProducer call IOC_postEVT() to post another event AS BEHAVIOR.
 *      |-> EvtDesc.EvtID = IOC_EVT_NAME_TEST_KEEPALIVE
 *      |-> get IOC_RESULT_NO_EVENT_CONSUMER AS VERIFY.
 *   7) EvtConsumer call IOC_subEVT() to resubscribe the event AS BEHAVIOR.
 *   8) EvtProducer call IOC_postEVT() to post another event AS BEHAVIOR.
 *      |-> EvtDesc.EvtID = IOC_EVT_NAME_TEST_KEEPALIVE
 *      |-> call IOC_forceProcEVT() to process the event immediately.
 *      |-> EvtConsumerPrivData.KeepAliveEvtCnt = 2 AS VERIFY.
 *   9) EvtProducer/EvtConsumer call IOC_closeLink() to close the link AS VERIFY&CLEANUP.
 *   10) EvtProducer call IOC_offlineService() to offline the service AS VERIFY&CLEANUP.
 * @[Expect]: all steps are passed.
 * @[Notes]:
 */

TEST(UT_ServiceTypical, verifyConsumerResubscribeEvent) {
    IOC_Result_T Result = IOC_RESULT_BUG;
    IOC_SrvID_T EvtProducerSrvID = IOC_ID_INVALID;
    IOC_LinkID_T EvtProducerLinkID = IOC_ID_INVALID;
    IOC_LinkID_T EvtConsumerLinkID = IOC_ID_INVALID;

    IOC_SrvURI_T CSURI = {
        .pProtocol = IOC_SRV_PROTO_FIFO,
        .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
        .pPath = (const char*)"verifyConsumerResubscribeEvent",
    };

    // Step-1
    IOC_SrvArgs_T SrvArgs = {
        .SrvURI = CSURI,
        .UsageCapabilites = IOC_LinkUsageEvtProducer,
    };

    Result = IOC_onlineService(&EvtProducerSrvID, &SrvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

    // Step-2:
    __EvtConsumerPrivData_T EvtConsumerPrivData = {0};

    IOC_EvtID_T EvtIDs[] = {IOC_EVTID_TEST_KEEPALIVE};
    IOC_SubEvtArgs_T SubEvtArgs = {
        .CbProcEvt_F = __CbProcEvt_F,
        .pCbPrivData = &EvtConsumerPrivData,
        .EvtNum = IOC_calcArrayElmtCnt(EvtIDs),
        .pEvtIDs = EvtIDs,
    };

    IOC_ConnArgs_T ConnArgs = {
        .SrvURI = CSURI,
        .Usage = IOC_LinkUsageEvtConsumer,
    };

    std::thread EvtConsumerThread([&] {
        IOC_Result_T Result = IOC_connectService(&EvtConsumerLinkID, &ConnArgs, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

        Result = IOC_subEVT(EvtConsumerLinkID, &SubEvtArgs);
        ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint
    });

    // Step-3
    Result = IOC_acceptClient(EvtProducerSrvID, &EvtProducerLinkID, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

    EvtConsumerThread.join();

    // Step-4
    IOC_EvtDesc_T EvtDesc = {.EvtID = IOC_EVTID_TEST_KEEPALIVE};
    Result = IOC_postEVT(EvtProducerLinkID, &EvtDesc, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // KeyVerifyPoint

    IOC_forceProcEVT();
    ASSERT_EQ(1, EvtConsumerPrivData.KeepAliveEvtCnt);  // KeyVerifyPoint

    // Step-5
    IOC_UnsubEvtArgs_T UnsubEvtArgs = {
        .CbProcEvt_F = __CbProcEvt_F,
        .pCbPrivData = &EvtConsumerPrivData,
    };
    Result = IOC_unsubEVT(EvtConsumerLinkID, &UnsubEvtArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // KeyVerifyPoint

    // Step-6
    Result = IOC_postEVT(EvtProducerLinkID, &EvtDesc, NULL);
    ASSERT_EQ(IOC_RESULT_NO_EVENT_CONSUMER, Result);  // KeyVerifyPoint

    // Step-7
    Result = IOC_subEVT(EvtConsumerLinkID, &SubEvtArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // KeyVerifyPoint

    // Step-8
    Result = IOC_postEVT(EvtProducerLinkID, &EvtDesc, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // KeyVerifyPoint

    IOC_forceProcEVT();
    ASSERT_EQ(2, EvtConsumerPrivData.KeepAliveEvtCnt);  // KeyVerifyPoint

    // Step-9
    Result = IOC_closeLink(EvtProducerLinkID);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

    Result = IOC_unsubEVT(EvtConsumerLinkID, &UnsubEvtArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

    Result = IOC_closeLink(EvtConsumerLinkID);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

    // Step-10
    Result = IOC_offlineService(EvtProducerSrvID);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint
}
