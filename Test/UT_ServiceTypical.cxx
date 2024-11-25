
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
        ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // KeyVerifyPoint

        Result = IOC_subEVT(EvtConsumerLinkID, &SubEvtArgs);
        ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // KeyVerifyPoint
    });

    // Step-3
    Result = IOC_acceptClient(EvtProducerSrvID, &EvtProducerLinkID, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // KeyVerifyPoint

    EvtConsumerThread.join();

    // Step-4
    IOC_EvtDesc_T EvtDesc = {
        .EvtID = IOC_EVT_NAME_TEST_KEEPALIVE,
    };
    Result = IOC_postEVT(EvtProducerLinkID, &EvtDesc, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // KeyVerifyPoint

    IOC_forceProcEVT();

    // Step-5
    IOC_UnsubEvtArgs_T UnsubEvtArgs = {
        .CbProcEvt_F = __US1AC1TC1_CbProcEvt_F,
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

typedef struct {
    uint32_t StartedEvtCnt;
    uint32_t KeepingEvtCnt;
    uint32_t StoppedEvtCnt;
} __US1AC2TC2_EvtConsumerPrivData_T;

static IOC_Result_T __US1AC2TC2_CbProcEvt_F(IOC_EvtDesc_T* pEvtDesc, void* pCbPrivData) {
    __US1AC2TC2_EvtConsumerPrivData_T* pEvtConsumerPrivData = (__US1AC2TC2_EvtConsumerPrivData_T*)pCbPrivData;

    if ((IOC_EVTID_TEST_MOVE_STARTED == pEvtDesc->EvtID) || (IOC_EVTID_TEST_PULL_STARTED == pEvtDesc->EvtID)) {
        pEvtConsumerPrivData->StartedEvtCnt++;
    } else if ((IOC_EVTID_TEST_MOVE_KEEPING == pEvtDesc->EvtID) || (IOC_EVTID_TEST_PULL_KEEPING == pEvtDesc->EvtID)) {
        pEvtConsumerPrivData->KeepingEvtCnt++;
    } else if ((IOC_EVTID_TEST_MOVE_STOPPED == pEvtDesc->EvtID) || (IOC_EVTID_TEST_PULL_STOPPED == pEvtDesc->EvtID)) {
        pEvtConsumerPrivData->StoppedEvtCnt++;
    } else {
        EXPECT_TRUE(false) << "Unknown EvtID: " << pEvtDesc->EvtID;
    }

    return IOC_RESULT_SUCCESS;
}

TEST(UT_ServiceTypical, verifySingleServiceMultiClients_byPostEvtAtSrvSide_bySubDiffEvtAtCliSide) {
    IOC_Result_T Result = IOC_RESULT_BUG;
    IOC_SrvID_T EvtProducerSrvID = IOC_ID_INVALID;
    IOC_LinkID_T EvtProducerLinkID4Consumer1 = IOC_ID_INVALID;
    IOC_LinkID_T EvtProducerLinkID4Consumer2 = IOC_ID_INVALID;

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

    Result = IOC_onlineService(&EvtProducerSrvID, &SrvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

    // Step-2:
    __US1AC2TC2_EvtConsumerPrivData_T ConsumerAPrivData = {
        .StartedEvtCnt = 0,
        .KeepingEvtCnt = 0,
        .StoppedEvtCnt = 0,
    };

    IOC_EvtID_T EvtIDs4ConsumerA[] = {IOC_EVTID_TEST_MOVE_STARTED, IOC_EVTID_TEST_MOVE_KEEPING,
                                      IOC_EVTID_TEST_MOVE_STOPPED};
    IOC_SubEvtArgs_T SubEvtArgs4ConsumerA = {
        .CbProcEvt_F = __US1AC2TC2_CbProcEvt_F,
        .pCbPrivData = &ConsumerAPrivData,
        .EvtNum = IOC_calcArrayElmtCnt(EvtIDs4ConsumerA),
        .pEvtIDs = EvtIDs4ConsumerA,
    };

    IOC_ConnArgs_T ConnArgs = {
        .SrvURI = CSURI,
        .Usage = IOC_LinkUsageEvtConsumer,
    };

    std::thread EvtConsumerAThread([&] {
        IOC_Result_T Result = IOC_connectService(&EvtProducerLinkID4Consumer1, &ConnArgs, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

        Result = IOC_subEVT(EvtProducerLinkID4Consumer1, &SubEvtArgs4ConsumerA);
        ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint
    });

    // Step-3:
    __US1AC2TC2_EvtConsumerPrivData_T ConsumerBPrivData = {
        .StartedEvtCnt = 0,
        .KeepingEvtCnt = 0,
        .StoppedEvtCnt = 0,
    };

    IOC_EvtID_T EvtIDs4ConsumerB[] = {IOC_EVTID_TEST_PULL_STARTED, IOC_EVTID_TEST_PULL_KEEPING,
                                      IOC_EVTID_TEST_PULL_STOPPED};
    IOC_SubEvtArgs_T SubEvtArgs4ConsumerB = {
        .CbProcEvt_F = __US1AC2TC2_CbProcEvt_F,
        .pCbPrivData = &ConsumerBPrivData,
        .EvtNum = IOC_calcArrayElmtCnt(EvtIDs4ConsumerB),
        .pEvtIDs = EvtIDs4ConsumerB,
    };

    std::thread EvtConsumerBThread([&] {
        IOC_Result_T Result = IOC_connectService(&EvtProducerLinkID4Consumer2, &ConnArgs, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

        Result = IOC_subEVT(EvtProducerLinkID4Consumer2, &SubEvtArgs4ConsumerB);
        ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint
    });

    // Step-4
    Result = IOC_acceptClient(EvtProducerSrvID, &EvtProducerLinkID4Consumer1, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

    Result = IOC_acceptClient(EvtProducerSrvID, &EvtProducerLinkID4Consumer2, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

    EvtConsumerAThread.join();
    EvtConsumerBThread.join();

    // Step-5
    IOC_EvtDesc_T EvtDesc = {
        .EvtID = IOC_EVTID_TEST_MOVE_STARTED,
    };
    Result = IOC_postEVT(EvtProducerLinkID4Consumer1, &EvtDesc, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

#define _N_MOVE_KEEPING 3
    EvtDesc.EvtID = IOC_EVTID_TEST_MOVE_KEEPING;
    for (int i = 0; i < _N_MOVE_KEEPING; i++) {
        Result = IOC_postEVT(EvtProducerLinkID4Consumer1, &EvtDesc, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint
    }

    EvtDesc.EvtID = IOC_EVTID_TEST_MOVE_STOPPED;
    Result = IOC_postEVT(EvtProducerLinkID4Consumer1, &EvtDesc, NULL);

    IOC_forceProcEVT();
    ASSERT_EQ(1, ConsumerAPrivData.StartedEvtCnt);                // KeyVerifyPoint
    ASSERT_EQ(_N_MOVE_KEEPING, ConsumerAPrivData.KeepingEvtCnt);  // KeyVerifyPoint
    ASSERT_EQ(1, ConsumerAPrivData.StoppedEvtCnt);                // KeyVerifyPoint
    ASSERT_EQ(0, ConsumerBPrivData.StartedEvtCnt);                // KeyVerifyPoint
    ASSERT_EQ(0, ConsumerBPrivData.KeepingEvtCnt);                // KeyVerifyPoint
    ASSERT_EQ(0, ConsumerBPrivData.StoppedEvtCnt);                // KeyVerifyPoint

    // Step-6
    EvtDesc.EvtID = IOC_EVTID_TEST_PULL_STARTED;
    Result = IOC_postEVT(EvtProducerLinkID4Consumer2, &EvtDesc, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // KeyVerifyPoint

#define _M_PULL_KEEPING 5
    EvtDesc.EvtID = IOC_EVTID_TEST_PULL_KEEPING;
    for (int i = 0; i < _M_PULL_KEEPING; i++) {
        Result = IOC_postEVT(EvtProducerLinkID4Consumer2, &EvtDesc, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // KeyVerifyPoint
    }

    EvtDesc.EvtID = IOC_EVTID_TEST_PULL_STOPPED;
    Result = IOC_postEVT(EvtProducerLinkID4Consumer2, &EvtDesc, NULL);

    IOC_forceProcEVT();
    ASSERT_EQ(1, ConsumerAPrivData.StartedEvtCnt);                // KeyVerifyPoint
    ASSERT_EQ(_N_MOVE_KEEPING, ConsumerAPrivData.KeepingEvtCnt);  // KeyVerifyPoint
    ASSERT_EQ(1, ConsumerAPrivData.StoppedEvtCnt);                // KeyVerifyPoint
    ASSERT_EQ(1, ConsumerBPrivData.StartedEvtCnt);                // KeyVerifyPoint
    ASSERT_EQ(_M_PULL_KEEPING, ConsumerBPrivData.KeepingEvtCnt);  // KeyVerifyPoint
    ASSERT_EQ(1, ConsumerBPrivData.StoppedEvtCnt);                // KeyVerifyPoint

    // Step-7
    IOC_UnsubEvtArgs_T UnsubEvtArgs = {
        .CbProcEvt_F = __US1AC2TC2_CbProcEvt_F,
        .pCbPrivData = &ConsumerAPrivData,
    };
    Result = IOC_unsubEVT(EvtProducerLinkID4Consumer1, &UnsubEvtArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

    UnsubEvtArgs.pCbPrivData = &ConsumerBPrivData;
    Result = IOC_unsubEVT(EvtProducerLinkID4Consumer2, &UnsubEvtArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

    // Step-8
    EvtDesc.EvtID = IOC_EVTID_TEST_KEEPALIVE;
    Result = IOC_postEVT(EvtProducerLinkID4Consumer1, &EvtDesc, NULL);
    ASSERT_EQ(IOC_RESULT_NO_EVENT_CONSUMER, Result);  // VerifyPoint

    // Step-9
    Result = IOC_closeLink(EvtProducerLinkID4Consumer1);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

    Result = IOC_closeLink(EvtProducerLinkID4Consumer2);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

    // Step-10
    Result = IOC_offlineService(EvtProducerSrvID);
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

typedef struct {
    uint32_t StartedEvtCnt;
    uint32_t KeepingEvtCnt;
    uint32_t StoppedEvtCnt;
} __US1AC3TC3_EvtConsumerPrivData_T;

static IOC_Result_T __US1AC3TC3_CbProcEvt_F(IOC_EvtDesc_T* pEvtDesc, void* pCbPrivData) {
    __US1AC3TC3_EvtConsumerPrivData_T* pEvtConsumerPrivData = (__US1AC3TC3_EvtConsumerPrivData_T*)pCbPrivData;

    if ((IOC_EVTID_TEST_MOVE_STARTED == pEvtDesc->EvtID) || (IOC_EVTID_TEST_PULL_STARTED == pEvtDesc->EvtID) ||
        (IOC_EVTID_TEST_PUSH_STARTED == pEvtDesc->EvtID)) {
        pEvtConsumerPrivData->StartedEvtCnt++;
    } else if ((IOC_EVTID_TEST_MOVE_KEEPING == pEvtDesc->EvtID) || (IOC_EVTID_TEST_PULL_KEEPING == pEvtDesc->EvtID) ||
               (IOC_EVTID_TEST_PUSH_KEEPING == pEvtDesc->EvtID)) {
        pEvtConsumerPrivData->KeepingEvtCnt++;
    } else if ((IOC_EVTID_TEST_MOVE_STOPPED == pEvtDesc->EvtID) || (IOC_EVTID_TEST_PULL_STOPPED == pEvtDesc->EvtID) ||
               (IOC_EVTID_TEST_PUSH_STOPPED == pEvtDesc->EvtID)) {
        pEvtConsumerPrivData->StoppedEvtCnt++;
    } else {
        EXPECT_TRUE(false) << "Unknown EvtID: " << pEvtDesc->EvtID;
    }

    return IOC_RESULT_SUCCESS;
}

TEST(UT_ServiceTypical, verifyMultiServiceMultiClient_byPostEvtAtSrvSide_bySubDiffEvtAtCliSide) {
    IOC_Result_T Result = IOC_RESULT_BUG;
    IOC_SrvID_T SrvID_Producer1 = IOC_ID_INVALID;
    IOC_SrvID_T SrvID_Producer2 = IOC_ID_INVALID;
    IOC_LinkID_T LinkID_ConsumerA_toSrvID1 = IOC_ID_INVALID;
    IOC_LinkID_T LinkID_ConsumerB_toSrvID2 = IOC_ID_INVALID;
    IOC_LinkID_T LinkID_ConsumerC_toSrvID1 = IOC_ID_INVALID;
    IOC_LinkID_T LinkID_ConsumerC_toSrvID2 = IOC_ID_INVALID;

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

    // Step-1
    IOC_SrvArgs_T SrvArgs1 = {
        .SrvURI = CSURI1,
        .UsageCapabilites = IOC_LinkUsageEvtProducer,
    };

    Result = IOC_onlineService(&SrvID_Producer1, &SrvArgs1);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

    // Step-2
    IOC_SrvArgs_T SrvArgs2 = {
        .SrvURI = CSURI2,
        .UsageCapabilites = IOC_LinkUsageEvtProducer,
    };

    Result = IOC_onlineService(&SrvID_Producer2, &SrvArgs2);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

    // Step-3:
    __US1AC3TC3_EvtConsumerPrivData_T PrivDataConsumerA = {
        .StartedEvtCnt = 0,
        .KeepingEvtCnt = 0,
        .StoppedEvtCnt = 0,
    };

    IOC_EvtID_T EvtIDs4ConsumerA[] = {IOC_EVTID_TEST_MOVE_STARTED, IOC_EVTID_TEST_MOVE_KEEPING,
                                      IOC_EVTID_TEST_MOVE_STOPPED};
    IOC_SubEvtArgs_T SubEvtArgs4ConsumerA = {
        .CbProcEvt_F = __US1AC3TC3_CbProcEvt_F,
        .pCbPrivData = &PrivDataConsumerA,
        .EvtNum = IOC_calcArrayElmtCnt(EvtIDs4ConsumerA),
        .pEvtIDs = EvtIDs4ConsumerA,
    };

    IOC_ConnArgs_T ConnArgs = {
        .Usage = IOC_LinkUsageEvtConsumer,
    };

    ConnArgs.SrvURI = CSURI1;
    std::thread EvtConsumerAThread([&] {
        IOC_Result_T Result = IOC_connectService(&LinkID_ConsumerA_toSrvID1, &ConnArgs, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

        Result = IOC_subEVT(LinkID_ConsumerA_toSrvID1, &SubEvtArgs4ConsumerA);
        ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint
    });

    // Step-4:
    __US1AC3TC3_EvtConsumerPrivData_T PrivDataConsumerB = {
        .StartedEvtCnt = 0,
        .KeepingEvtCnt = 0,
        .StoppedEvtCnt = 0,
    };

    IOC_EvtID_T EvtIDs4ConsumerB[] = {IOC_EVTID_TEST_PULL_STARTED, IOC_EVTID_TEST_PULL_KEEPING,
                                      IOC_EVTID_TEST_PULL_STOPPED};
    IOC_SubEvtArgs_T SubEvtArgs4ConsumerB = {
        .CbProcEvt_F = __US1AC3TC3_CbProcEvt_F,
        .pCbPrivData = &PrivDataConsumerB,
        .EvtNum = IOC_calcArrayElmtCnt(EvtIDs4ConsumerB),
        .pEvtIDs = EvtIDs4ConsumerB,
    };

    ConnArgs.SrvURI = CSURI2;

    std::thread EvtConsumerBThread([&] {
        IOC_Result_T Result = IOC_connectService(&LinkID_ConsumerB_toSrvID2, &ConnArgs, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

        Result = IOC_subEVT(LinkID_ConsumerB_toSrvID2, &SubEvtArgs4ConsumerB);
        ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint
    });

    // Step-5:
    __US1AC3TC3_EvtConsumerPrivData_T PrivDataConsumerC = {
        .StartedEvtCnt = 0,
        .KeepingEvtCnt = 0,
        .StoppedEvtCnt = 0,
    };

    IOC_EvtID_T EvtIDs4ConsumerC[] = {IOC_EVTID_TEST_MOVE_STARTED, IOC_EVTID_TEST_MOVE_KEEPING,
                                      IOC_EVTID_TEST_MOVE_STOPPED, IOC_EVTID_TEST_PULL_STARTED,
                                      IOC_EVTID_TEST_PULL_KEEPING, IOC_EVTID_TEST_PULL_STOPPED};
    IOC_SubEvtArgs_T SubEvtArgs4ConsumerC = {
        .CbProcEvt_F = __US1AC3TC3_CbProcEvt_F,
        .pCbPrivData = &PrivDataConsumerC,
        .EvtNum = IOC_calcArrayElmtCnt(EvtIDs4ConsumerC),
        .pEvtIDs = EvtIDs4ConsumerC,
    };

    ConnArgs.SrvURI = CSURI1;

    std::thread EvtConsumerCThread([&] {
        IOC_Result_T Result = IOC_connectService(&LinkID_ConsumerC_toSrvID1, &ConnArgs, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

        Result = IOC_subEVT(LinkID_ConsumerC_toSrvID1, &SubEvtArgs4ConsumerC);
        ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint
    });

    IOC_ConnArgs_T ConnArgs2 = {
        .Usage = IOC_LinkUsageEvtConsumer,
    };
    ConnArgs2.SrvURI = CSURI2;

    std::thread EvtConsumerCThread2([&] {
        IOC_Result_T Result = IOC_connectService(&LinkID_ConsumerC_toSrvID2, &ConnArgs2, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

        Result = IOC_subEVT(LinkID_ConsumerC_toSrvID2, &SubEvtArgs4ConsumerC);
        ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint
    });

    // Step-6
    IOC_LinkID_T LinkID_Producer1_fromConsumerA = IOC_ID_INVALID;
    IOC_LinkID_T LinkID_Producer1_fromConsumerC = IOC_ID_INVALID;
    IOC_LinkID_T LinkID_Producer2_fromConsumerB = IOC_ID_INVALID;
    IOC_LinkID_T LinkID_Producer2_fromConsumerC = IOC_ID_INVALID;

    Result = IOC_acceptClient(SrvID_Producer1, &LinkID_Producer1_fromConsumerA, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

    Result = IOC_acceptClient(SrvID_Producer1, &LinkID_Producer1_fromConsumerC, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

    Result = IOC_acceptClient(SrvID_Producer2, &LinkID_Producer2_fromConsumerB, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

    Result = IOC_acceptClient(SrvID_Producer2, &LinkID_Producer2_fromConsumerC, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

    EvtConsumerAThread.join();
    EvtConsumerBThread.join();
    EvtConsumerCThread.join();
    EvtConsumerCThread2.join();

    // Step-7
    IOC_EvtDesc_T EvtDesc = {
        .EvtID = IOC_EVTID_TEST_MOVE_STARTED,
    };
    Result = IOC_postEVT(LinkID_Producer1_fromConsumerA, &EvtDesc, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

#define _N_MOVE_KEEPING 3
    EvtDesc.EvtID = IOC_EVTID_TEST_MOVE_KEEPING;
    for (int i = 0; i < _N_MOVE_KEEPING; i++) {
        Result = IOC_postEVT(LinkID_Producer1_fromConsumerA, &EvtDesc, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint
    }

    EvtDesc.EvtID = IOC_EVTID_TEST_MOVE_STOPPED;
    Result = IOC_postEVT(LinkID_Producer1_fromConsumerA, &EvtDesc, NULL);

    IOC_forceProcEVT();
    ASSERT_EQ(1, PrivDataConsumerA.StartedEvtCnt);                // KeyVerifyPoint
    ASSERT_EQ(_N_MOVE_KEEPING, PrivDataConsumerA.KeepingEvtCnt);  // KeyVerifyPoint
    ASSERT_EQ(1, PrivDataConsumerA.StoppedEvtCnt);                // KeyVerifyPoint
    ASSERT_EQ(0, PrivDataConsumerB.StartedEvtCnt);                // KeyVerifyPoint
    ASSERT_EQ(0, PrivDataConsumerB.KeepingEvtCnt);                // KeyVerifyPoint
    ASSERT_EQ(0, PrivDataConsumerB.StoppedEvtCnt);                // KeyVerifyPoint
    ASSERT_EQ(1, PrivDataConsumerC.StartedEvtCnt);                // KeyVerifyPoint
    ASSERT_EQ(_N_MOVE_KEEPING, PrivDataConsumerC.KeepingEvtCnt);  // KeyVerifyPoint
    ASSERT_EQ(1, PrivDataConsumerC.StoppedEvtCnt);                // KeyVerifyPoint

    // Step-8
    EvtDesc.EvtID = IOC_EVTID_TEST_PULL_STARTED;
    Result = IOC_postEVT(LinkID_Producer2_fromConsumerB, &EvtDesc, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // KeyVerifyPoint

#define _M_PULL_KEEPING 5
    EvtDesc.EvtID = IOC_EVTID_TEST_PULL_KEEPING;
    for (int i = 0; i < _M_PULL_KEEPING; i++) {
        Result = IOC_postEVT(LinkID_Producer2_fromConsumerB, &EvtDesc, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // KeyVerifyPoint
    }

    EvtDesc.EvtID = IOC_EVTID_TEST_PULL_STOPPED;
    Result = IOC_postEVT(LinkID_Producer2_fromConsumerB, &EvtDesc, NULL);

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

    // Step-9
    EvtDesc.EvtID = IOC_EVTID_TEST_PUSH_STARTED;
    Result = IOC_postEVT(LinkID_Producer1_fromConsumerC, &EvtDesc, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // KeyVerifyPoint

#define _P_PUSH_KEEPING 7
    EvtDesc.EvtID = IOC_EVTID_TEST_PUSH_KEEPING;
    for (int i = 0; i < _P_PUSH_KEEPING; i++) {
        Result = IOC_postEVT(LinkID_Producer1_fromConsumerC, &EvtDesc, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // KeyVerifyPoint
    }

    EvtDesc.EvtID = IOC_EVTID_TEST_PUSH_STOPPED;
    Result = IOC_postEVT(LinkID_Producer1_fromConsumerC, &EvtDesc, NULL);

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
    IOC_UnsubEvtArgs_T UnsubEvtArgs = {
        .CbProcEvt_F = __US1AC3TC3_CbProcEvt_F,
        .pCbPrivData = &PrivDataConsumerA,
    };
    Result = IOC_unsubEVT(LinkID_Producer1_fromConsumerA, &UnsubEvtArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

    UnsubEvtArgs.pCbPrivData = &PrivDataConsumerB;
    Result = IOC_unsubEVT(LinkID_Producer2_fromConsumerB, &UnsubEvtArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

    UnsubEvtArgs.pCbPrivData = &PrivDataConsumerC;
    Result = IOC_unsubEVT(LinkID_Producer1_fromConsumerC, &UnsubEvtArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

    Result = IOC_unsubEVT(LinkID_Producer2_fromConsumerC, &UnsubEvtArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

    // Step-11
    EvtDesc.EvtID = IOC_EVTID_TEST_KEEPALIVE;
    Result = IOC_postEVT(LinkID_Producer1_fromConsumerA, &EvtDesc, NULL);
    ASSERT_EQ(IOC_RESULT_NO_EVENT_CONSUMER, Result);  // VerifyPoint

    // Step-12
    Result = IOC_closeLink(LinkID_Producer1_fromConsumerA);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

    Result = IOC_closeLink(LinkID_Producer1_fromConsumerC);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

    Result = IOC_closeLink(LinkID_Producer2_fromConsumerB);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

    Result = IOC_closeLink(LinkID_Producer2_fromConsumerC);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

    // Step-13
    Result = IOC_offlineService(SrvID_Producer1);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint

    Result = IOC_offlineService(SrvID_Producer2);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint
}
