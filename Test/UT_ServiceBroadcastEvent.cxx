///////////////////////////////////////////////////////////////////////////////////////////////////
//======BEGIN OF OVERVIEW OF THIS UNIT TESTING FILE================================================
/**
 * @brief
 *  Reference: 'SrvID as EvtProducer' in README_UserGuide.md
 *
 *-------------------------------------------------------------------------------------------------
 * Key APIs and DataTypes:
 *  IOC_SrvTypes: IOC_SrvArgs_T, IOC_SrvFlags_T::IOC_SRVFLAG_BROADCAST_EVENT
 *  IOC_SrvAPI.h: IOC_onlineService, IOC_offlineService, IOC_acceptClient
 *  IOC_EvtAPI.h: IOC_broadcastEVT, IOC_subEVT, IOC_unsubEVT
 */
//======END OF OVERVIEW OF THIS UNIT TESTING FILE==================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======BEGIN OF UNIT TESTING DESIGN===============================================================
//-------------------------------------------------------------------------------------------------
/**
 * @brief 【User Story】
 *
 *  US-1: AS AN EvtProducer,
 *      I WANT to postEVT to SrvID after onlineService, without acceptClient and knowing the LinkID,
 *      SO THAT what ever how many EvtConsumer connect to my service,
 *          I CAN post events to all connected pair Links by a single API call.
 *
 *
 */

//-------------------------------------------------------------------------------------------------
/**
 * @brief 【Acceptance Criteria】
 *
 * [@US-1]
 *      AC-1: GIVEN EvtProducer online a service, but NO EvtConsumer connect to the service,
 *          WHEN EvtProducer postEVT to the service,
 *          THEN get IOC_RESULT_NO_EVENT_CONSUMER.
 *      AC-2: GIVEN EvtProducer online a service with SrvFlag=IOC_SRVFLAG_BROADCAST_EVENT got SrvID,
 *              AND MANY EvtConsumer connect to the service,
 *          WHEN EvtProducer postEVT to the SrvID,
 *          THEN each EvtConsumer will process the subbed event.
 *      AC-3: GIVEN EvtProducer online a service without SrvFlag=IOC_SRVFLAG_BROADCAST_EVENT,
 *              AND MANY EvtConsumer connect to the service,
 *          WHEN EvtProducer postEVT to the SrvID,
 *          THEN NO EvtConsumer will autoAccept by the service,
 *              which means all EvtConsumer will blocked on IOC_connectService(),
 *          AND THEN get IOC_RESULT_NOT_SUPPORT_BROADCAST_EVENT on server side.
 *
 */

//-------------------------------------------------------------------------------------------------
/**
 * @brief 【Test Cases】
 *
 * [@AC-1, US-1]
 *  TC-1:
 *      [@Name]: verifyPostEvtToSrvID_willGetNoEvtConsumer_whenNoEvtConsumerConnected
 *      [@Purpose]: verify postEVT to SrvID will get NO_EVENT_CONSUMER when no EvtConsumer connected.
 *
 * [@AC-2, US-1]
 *  TC-2:
 *      [@Name]: verifyPostEvtToSrvID_willLetAllConnectedEvtConsumersProcessEvt
 *      [@Purpose]: verify postEVT to SrvID will get EvtConsumer process event when many EvtConsumer connected.
 *      [@brief]: EvtConsumerA connect to SrvID, subEvt(MOVE_STARTED/KEEPING/STOPPED),
 *          EvtConsumerB connect to SrvID, subEvt(PULL_STARTED/KEEPING/STOPPED),
 *          EvtConsumerC connect to SrvID, subEvt(PUSH_STARTED/KEEPING/STOPPED),
 *          EvtProducer postEVT([MOVE,PULL,PUSH]_STARTED/KEEPING/STOPPED) to SrvID,
 *
 */

//======END OF UNIT TESTING DESIGN=================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======BEGIN OF UNIT TESTING IMPLEMENTATION=======================================================
#include "_UT_IOC_Common.h"
/**
 * @[Name]: <US1AC1TC1>verifyPostEvtToSrvID_willGetNoEvtConsumer_whenNoEvtConsumerConnected
 * @[Steps]:
 *    1) EvtProducer call IOC_onlineService() to online a service AS SETUP.
 *        |-> SrvArgs.UsageCapabilites = IOC_LinkUsageEvtProducer
 *        |-> SrvArgs.SrvURI = {IOC_SRV_PROTO_FIFO, IOC_SRV_HOST_LOCAL_PROCESS, "EvtProducer"}
 *        |-> SrvArgs.Flags = IOC_SRVFLAG_BROADCAST_EVENT
 *    2) EvtProducer call IOC_broadcastEVT() to post an event AS BEHAVIOR.
 *        |-> EvtDesc.EvtID = IOC_EVT_NAME_TEST_KEEPALIVE
 *        |-> get IOC_RESULT_NO_EVENT_CONSUMER AS VERIFY.
 * @[Expect]:
 *    Get IOC_RESULT_NO_EVENT_CONSUMER.
 * @[Notes]:
 */
TEST(UT_ServiceBroadcastEvent, verifyPostEvtToSrvID_willGetNoEvtConsumer_whenNoEvtConsumerConnected) {
    IOC_Result_T Result = IOC_RESULT_BUG;

    // Step-1
    IOC_SrvArgs_T SrvArgs = {
        .SrvURI = {"fifo", "localprocess", "EvtProducer"},
        .Flags = IOC_SRVFLAG_BROADCAST_EVENT,
        .UsageCapabilites = IOC_LinkUsageEvtProducer,
    };
    IOC_SrvID_T SrvID = IOC_INVALID_SRV_ID;
    Result = IOC_onlineService(&SrvID, &SrvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);

    // Step-2
    IOC_EvtDesc_T EvtDesc = {IOC_EVTID_TEST_KEEPALIVE};
    Result = IOC_broadcastEVT(SrvID, &EvtDesc, NULL);
    ASSERT_EQ(IOC_RESULT_NO_EVENT_CONSUMER, Result);  // KeyVerifyPoint

    // Step-3
    Result = IOC_offlineService(SrvID);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);
}

/**
 * @[Name]: <US1AC2TC2>verifyPostEvtToSrvID_willLetAllConnectedEvtConsumersProcessEvt
 * @[Steps]:
 *      1) EvtProducer call IOC_onlineService() to online a service got SrvID_EvtProducer AS SETUP.
 *          |-> SrvArgs.UsageCapabilites = IOC_LinkUsageEvtProducer
 *          |-> SrvArgs.SrvURI = {IOC_SRV_PROTO_FIFO, IOC_SRV_HOST_LOCAL_PROCESS, "EvtPostFromSrvID"}
 *          |-> SrvArgs.Flags = IOC_SRVFLAG_BROADCAST_EVENT
 *      2) EvtConsumerA call IOC_connectService() the service got LinkID_EvtConsumerA_toEvtProducer AS SETUP.
 *          |-> SrvArgs.UsageCapabilites = IOC_LinkUsageEvtConsumer
 *          |-> SrvArgs.SrvURI = {IOC_SRV_PROTO_FIFO, IOC_SRV_HOST_LOCAL_PROCESS, "EvtPostFromSrvID"}
 *          a) EvtProducer call IOC_acceptClient() got LinkID_EvtProducer_fromEvtConsumerA AS SETUP.
 *          b) EvtConsumerA call IOC_subEVT() to subEvt(MOVE_STARTED/KEEPING/STOPPED) AS SETUP.
 *     3) EvtConsumerB call IOC_connectService() the service got LinkID_EvtConsumerB_toEvtProducer AS SETUP.
 *          |-> SrvArgs.UsageCapabilites = IOC_LinkUsageEvtConsumer
 *          |-> SrvArgs.SrvURI = {IOC_SRV_PROTO_FIFO, IOC_SRV_HOST_LOCAL_PROCESS, "EvtPostFromSrvID"}
 *          a) EvtProducer call IOC_acceptClient() got LinkID_EvtProducer_fromEvtConsumerB AS SETUP.
 *          b) EvtConsumerB call IOC_subEVT() to subEvt(PULL_STARTED/KEEPING/STOPPED) AS SETUP.
 *     4) EvtConsumerC call IOC_connectService() the service got LinkID_EvtConsumerC_toEvtProducer AS SETUP.
 *          |-> SrvArgs.UsageCapabilites = IOC_LinkUsageEvtConsumer
 *          |-> SrvArgs.SrvURI = {IOC_SRV_PROTO_FIFO, IOC_SRV_HOST_LOCAL_PROCESS, "EvtPostFromSrvID"}
 *          a) EvtProducer call IOC_acceptClient() got LinkID_EvtProducer_fromEvtConsumerC AS SETUP.
 *          b) EvtConsumerC call IOC_subEVT() to subEvt(PUSH_STARTED/KEEPING/STOPPED) AS SETUP.
 *     5) EvtProducer call IOC_broadcastEVT() to post an event AS BEHAVIOR.
 *          |-> R1: MOVE_STARTEDx1, MOVE_KEEPINGxn, MOVE_STOPPEDx1  // EvtConsumerA processes MOVE events
 *          |-> R2: PULL_STARTEDx1, PULL_KEEPINGxm, PULL_STOPPEDx1  // EvtConsumerB processes PULL events
 *          |-> R3: PUSH_STARTEDx1, PUSH_KEEPINGxp, PUSH_STOPPEDx1  // EvtConsumerC processes PUSH events
 *     6) EvtConsumerA, EvtConsumerB, EvtConsumerC process the subbed event AS VERIFY.
 *          |-> R1: EvtConsumerA process MOVE_STARTEDx1, MOVE_KEEPINGxn, MOVE_STOPPEDx1
 *          |-> R2: EvtConsumerB process PULL_STARTEDx1, PULL_KEEPINGxm, PULL_STOPPEDx1
 *          |-> R3: EvtConsumerC process PUSH_STARTEDx1, PUSH_KEEPINGxp, PUSH_STOPPEDx1
 *     7) EvtConsumerA, EvtConsumerB, EvtConsumerC call IOC_unsubEVT() AS CLEANUP.
 *     8) EvtConsumerA, EvtConsumerB, EvtConsumerC call IOC_closeLink() AS CLEANUP.
 *     9) EvtProducer call IOC_offlineService() AS CLEANUP.
 * @[Expect]:
 *     EvtConsumerA processes MOVE_STARTED, MOVE_KEEPING, MOVE_STOPPED events.
 *     EvtConsumerB processes PULL_STARTED, PULL_KEEPING, PULL_STOPPED events.
 *     EvtConsumerC processes PUSH_STARTED, PUSH_KEEPING, PUSH_STOPPED events.
 */

//======END OF UNIT TESTING IMPLEMENTATION=========================================================
///////////////////////////////////////////////////////////////////////////////////////////////////
