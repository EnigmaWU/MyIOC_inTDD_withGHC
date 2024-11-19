
///////////////////////////////////////////////////////////////////////////////////////////////////
//======BEGIN OF OVERVIEW OF THIS UNIT TESTING FILE================================================
/**
 * @brief
 *  Verify typical/classic usage/example of IOC's Service APIs.
 *
 *-------------------------------------------------------------------------------------------------
 * @note
 *     Service is identify by 'SrvURI' defined in IOC_SrvTypes.h,
 *         which is a combination of 'Protocol+Host+Port+Path' with some customized meaning.
 *     Service APIs are defined in IOC_SrvAPI.h, and it types are defined in IOC_SrvTypes.h.
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
 */

//======END OF UNIT TESTING DESIGN=================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======BEGIN OF UNIT TESTING IMPLEMENTATION=======================================================
#include "_UT_IOC_Common.h"

//===TEMPLATE OF UT CASE===
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

typedef struct {
    uint32_t KeepAliveEvtCnt;
} __US1AC1TC1_EvtConsumerPrivData_T;

static IOC_Result_T __US1AC1TC1_CbProcEvt_F(IOC_EvtDesc_T* pEvtDesc, void* pCbPrivData) {
    __US1AC1TC1_EvtConsumerPrivData_T* pEvtConsumerPrivData = (__US1AC1TC1_EvtConsumerPrivData_T*)pCbPrivData;

    if (IOC_EVT_NAME_TEST_KEEPALIVE == pEvtDesc->EvtID) {
        pEvtConsumerPrivData->KeepAliveEvtCnt++;
    } else {
        // ASSERT_EQ(0, 1);
    }

    return IOC_RESULT_SUCCESS;
}

TEST(UT_ServiceTypical, verifySingleServiceSingleClient_byPostEvtAtSrvSide) {
    IOC_Result_T Result = IOC_RESULT_BUG;
    IOC_SrvID_T EvtProducerSrvID = IOC_ID_INVALID;
    IOC_LinkID_T EvtProducerLinkID = IOC_ID_INVALID;
    IOC_LinkID_T EvtConsumerLinkID = IOC_ID_INVALID;

    IOC_SrvURI_T CSURI = {
        .pProtocol = IOC_SRV_PROTO_FIFO,
        .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
        .pPath = (const char*)"EvtProducer",
    };

    // Step-1
    IOC_SrvArgs_T SrvArgs = {
        .SrvURI = CSURI,
        .UsageCapabilites = IOC_LinkUsageEvtProducer,
    };

    Result = IOC_onlineService(&EvtProducerSrvID, &SrvArgs);
    EXPECT_EQ(IOC_RESULT_SUCCESS, Result);  // KeyVerifyPoint

    // Step-2:
    __US1AC1TC1_EvtConsumerPrivData_T EvtConsumerPrivData = {
        .KeepAliveEvtCnt = 0,
    };

    IOC_EvtID_T EvtIDs[] = {IOC_EVT_NAME_TEST_KEEPALIVE};
    IOC_SubEvtArgs_T SubEvtArgs = {
        .CbProcEvt_F = __US1AC1TC1_CbProcEvt_F,
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
        EXPECT_EQ(IOC_RESULT_SUCCESS, Result);  // KeyVerifyPoint

        Result = IOC_subEVT(EvtConsumerLinkID, &SubEvtArgs);
        EXPECT_EQ(IOC_RESULT_SUCCESS, Result);  // KeyVerifyPoint
    });

    // Step-3

    Result = IOC_acceptClient(EvtProducerSrvID, &EvtProducerLinkID, NULL);
    EXPECT_EQ(IOC_RESULT_SUCCESS, Result);  // KeyVerifyPoint

    EvtConsumerThread.join();

    // Step-4
    IOC_EvtDesc_T EvtDesc = {
        .EvtID = IOC_EVT_NAME_TEST_KEEPALIVE,
    };
    Result = IOC_postEVT(EvtProducerLinkID, &EvtDesc, NULL);
    EXPECT_EQ(IOC_RESULT_SUCCESS, Result);  // KeyVerifyPoint

    IOC_forceProcEVT();

    // Step-5
    IOC_UnsubEvtArgs_T UnsubEvtArgs = {
        .CbProcEvt_F = __US1AC1TC1_CbProcEvt_F,
        .pCbPrivData = &EvtConsumerPrivData,
    };
    Result = IOC_unsubEVT(EvtConsumerLinkID, &UnsubEvtArgs);
    EXPECT_EQ(IOC_RESULT_SUCCESS, Result);              // KeyVerifyPoint
    EXPECT_EQ(1, EvtConsumerPrivData.KeepAliveEvtCnt);  // KeyVerifyPoint

    // Step-6
    Result = IOC_postEVT(EvtProducerLinkID, &EvtDesc, NULL);
    EXPECT_EQ(IOC_RESULT_NO_EVENT_CONSUMER, Result);  // KeyVerifyPoint

    // Step-7
    Result = IOC_closeLink(EvtProducerLinkID);
    EXPECT_EQ(IOC_RESULT_SUCCESS, Result);  // KeyVerifyPoint

    Result = IOC_closeLink(EvtConsumerLinkID);
    EXPECT_EQ(IOC_RESULT_SUCCESS, Result);  // KeyVerifyPoint

    // Step-8
    Result = IOC_offlineService(EvtProducerSrvID);
    EXPECT_EQ(IOC_RESULT_SUCCESS, Result);  // KeyVerifyPoint
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
 *
 *
 *
 */