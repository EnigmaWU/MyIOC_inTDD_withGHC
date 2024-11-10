
///////////////////////////////////////////////////////////////////////////////////////////////////
//======BEGIN OF OVERVIEW OF THIS UNIT TESTING FILE================================================
/**
 * @brief
 *  Verify typical/classic usage/example of IOC's Service APIs.
 *
 *-------------------------------------------------------------------------------------------------
 * @note
 *     Service APIs are defiend in IOC_SrvAPI.h, and it types are defined in IOC_SrvTypes.h.
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
 *      I WANT to online a service,
 *      SO THAT EvtConsumer can connect to my service,
 *          AND subscribe events what I published over connected pair Links.
 *      OR <b>AS a EvtConsumer,
 *      I ALSO WANT to online a service,
 *      SO THAT EvtProducer can connect to my service,
 *          AND publish events over connected pair Links.
 *      SAME FOR CmdInitiator and CmdExecutor:
 *          <c>CmdInitiator online a service.
 *          <d>CmdExecutor online a service.
 *      SAME FOR DatSender and DatReceiver:
 *          <e>DatSender online a service.
 *          <f>DatReceiver online a service.
 *
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
 *      AC-2: GIVEN a service is onlined by EvtConsumer,
 *          WHEN MANY EvtProducers connects to the service and EACH establish a pair Link,
 *          THEN EACH EvtConsumer can subscribe different events over each's pair Link,
 *              AND EvtProducer can post events to each EvtConsumer,
 *              AND EACH EvtConsumer will process what it subscribed events only.
 * [@US-1.b]
 *      AC-1: GIVEN a service is onlined by EvtConsumer,
 *          WHEN EvtProducer connects to the service and establish a pair Link,
 *          THEN EvtConsumer can subscribe events over the pair Link,
 *              AND EvtProducer can post events, EvtConsumer can process them.
 *
 */

//-------------------------------------------------------------------------------------------------
/**
 * @brief 【Test Cases】
 *
 * [@AC-1 of US-1.a]
 * TC-1:
 *  @[Name]:verifySingleServiceOnePairLink_byEvtProducerAtServerSide_andEvtConsumerAtClientSide
 *  @[Purpose]: verify simple but still typical scenario of one EvtProducer as server, one EvtConsumer as client.
 *
 */

//======END OF UNIT TESTING DESIGN=================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======BEGIN OF UNIT TESTING IMPLEMENTATION=======================================================
#include "_UT_IOC_Common.h"

//===TEMPLATE OF UT CASE===
/**
 * @[Name]: <TC1>verifySingleServiceOnePairLink_byEvtProducerAtServerSide_andEvtConsumerAtClientSide
 * @[Steps]:
 *   1) EvtProducer call IOC_onlineService() to online a service AS VERIFY.
 *      |-> SrvArgs.UsageCapabilites = IOC_LinkUsageEvtProducer
 *      |-> SrvArgs.SrvURI = "auto://localprocess/EvtProducer"
 *   2) EvtConsumer call IOC_connectLink() in standalone thread to the service AS VERIFY.
 *          |-> ConnArgs.Usage = IOC_LinkUsageEvtConsumer
 *          |-> ConnArgs.SrvURI = "auto://localprocess/EvtProducer"
 *      a) Call IOC_subEVT() to subscribe an event AS BEHAVIOR.
 *          |-> SubEvtArgs.EvtIDs = {IOC_EVT_NAME_TEST_KEEPALIVE}
 *   3) EvtProducer call IOC_acceptLink() to accept the link AS VERIFY.
 *   4) EvtProducer call IOC_postEVT() to post an event AS BEHAVIOR.
 *      |-> EvtDesc.EvtID = IOC_EVT_NAME_TEST_KEEPALIVE
 *      |-> call IOC_forceProcEVT() to process the event immediately.
 *   5) EvtConsumer call IOC_unsubEVT() to unsubscribe the event AS BEHAVIOR.
 *      |-> EvtConsumerPrivData.KeepAliveEvtCnt = 1 AS VERIFY.
 *   6) EvtProducer/EvtConsumer call IOC_closeLink() to close the link AS VERIFY&CLEANUP.
 *   7) EvtProducer call IOC_offlineService() to offline the service AS VERIFY&CLEANUP.
 * @[Expect]: all steps are passed.
 * @[Notes]:
 */

typedef struct {
    uint32_t KeepAliveEvtCnt;
} __TC1_EvtConsumerPrivData_T;

static IOC_Result_T __TC1_CbProcEvt_F(IOC_EvtDesc_T* pEvtDesc, void* pCbPrivData) {
    __TC1_EvtConsumerPrivData_T* pEvtConsumerPrivData = (__TC1_EvtConsumerPrivData_T*)pCbPrivData;

    if (IOC_EVT_NAME_TEST_KEEPALIVE == pEvtDesc->EvtID) {
        pEvtConsumerPrivData->KeepAliveEvtCnt++;
    } else {
        // ASSERT_EQ(0, 1);
    }

    return IOC_RESULT_SUCCESS;
}

TEST(UT_ServiceTypical, verifySingleServiceOnePairLink_byEvtProducerAtServerSide_andEvtConsumerAtClientSide) {
    IOC_Result_T Result            = IOC_RESULT_BUG;
    IOC_SrvID_T EvtProducerSrvID   = IOC_ID_INVALID;
    IOC_LinkID_T EvtProducerLinkID = IOC_ID_INVALID;
    IOC_LinkID_T EvtConsumerLinkID = IOC_ID_INVALID;

    IOC_SrvURI_T CSURI = {
        .pProtocol = IOC_SRV_PROTO_AUTO,
        .pHost     = IOC_SRV_HOST_LOCAL_PROCESS,
        .pPath     = (const char*)"EvtProducer",
    };

    // Step-1
    IOC_SrvArgs_T SrvArgs = {
        .SrvURI           = CSURI,
        .UsageCapabilites = IOC_LinkUsageEvtProducer,
    };

    Result = IOC_onlineService(&EvtProducerSrvID, &SrvArgs);
    EXPECT_EQ(IOC_RESULT_SUCCESS, Result);  // KeyVerifyPoint

    // Step-2:
    __TC1_EvtConsumerPrivData_T EvtConsumerPrivData = {
        .KeepAliveEvtCnt = 0,
    };

    IOC_EvtID_T EvtIDs[]        = {IOC_EVT_NAME_TEST_KEEPALIVE};
    IOC_SubEvtArgs_T SubEvtArgs = {
        .CbProcEvt_F = __TC1_CbProcEvt_F,
        .pCbPrivData = &EvtConsumerPrivData,
        .EvtNum      = IOC_calcArrayElmtCnt(EvtIDs),
        .pEvtIDs     = EvtIDs,
    };

    IOC_ConnArgs_T ConnArgs = {
        .SrvURI = CSURI,
        .Usage  = IOC_LinkUsageEvtConsumer,
    };

    std::thread EvtConsumerThread([&] {
        IOC_Result_T Result = IOC_connectService(&EvtConsumerLinkID, &ConnArgs, NULL);
        EXPECT_EQ(IOC_RESULT_SUCCESS, Result);  // KeyVerifyPoint

        Result = IOC_subEVT(EvtConsumerLinkID, &SubEvtArgs);
        EXPECT_EQ(IOC_RESULT_SUCCESS, Result);  // KeyVerifyPoint
    });

    // Step-3

    Result = IOC_acceptLink(EvtProducerSrvID, &EvtProducerLinkID, NULL);
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
        .CbProcEvt_F = __TC1_CbProcEvt_F,
        .pCbPrivData = &EvtConsumerPrivData,
    };
    Result = IOC_unsubEVT(EvtConsumerLinkID, &UnsubEvtArgs);
    EXPECT_EQ(IOC_RESULT_SUCCESS, Result);              // KeyVerifyPoint
    EXPECT_EQ(1, EvtConsumerPrivData.KeepAliveEvtCnt);  // KeyVerifyPoint

    // Step-6
    Result = IOC_closeLink(EvtProducerLinkID);
    EXPECT_EQ(IOC_RESULT_SUCCESS, Result);  // KeyVerifyPoint

    Result = IOC_closeLink(EvtConsumerLinkID);
    EXPECT_EQ(IOC_RESULT_SUCCESS, Result);  // KeyVerifyPoint

    // Step-7
    Result = IOC_offlineService(EvtProducerSrvID);
    EXPECT_EQ(IOC_RESULT_SUCCESS, Result);  // KeyVerifyPoint
}