
///////////////////////////////////////////////////////////////////////////////////////////////////
//======BEGIN OF OVERVIEW OF THIS UNIT TESTING FILE================================================
/**
 * @brief
 *  Use this unit testing file to verify the capability of IOC's Service, such as:
 *      - MAX NUMBER of services may be online at the same time.
 *      - MAX NUMBER of clients may be connected to a service at the same time.
 *
 *-------------------------------------------------------------------------------------------------
 * @Usage
 *  IOC_getCapability(CapDesc.CapID=IOC_CAPID_CONET_MODE) to get the capability of ConetMode.
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
 *  US-1: AS A USR of IOC's service, such as EvtProducer,
 *          I WANT to know how many MAX services may be onlined at the same time,
 *          SO THAT I can design my system to meet the requirement,
 *            OR I can catch the out-of-capability exception and handle it properly.
 *  US-2: AS A USR of IOC's client, such as EvtConsumer,
 *          I WANT to know how many MAX clients may be connected to a service at the same time,
 *          SO THAT I can design my system to meet the requirement,
 *
 *
 */

//-------------------------------------------------------------------------------------------------
/**
 * @brief 【Acceptance Criteria】
 *
 * [@US-1]
 *  AC-1: GIVEN USR know the MAX NUMBER of services may be onlined at the same time BY IOC_getCapability,
 *    WHEN USR online services less than the MAX NUMBER,
 *      THEN USR will get IOC_RESULT_SUCCESS result.
 *    WHEN USR online services more than the MAX NUMBER,
 *      THEN USR will get IOC_RESULT_TOO_MANY_SERVICES result.
 *    WHEN USR offline a service and online a new service,
 *      THEN USR will get IOC_RESULT_SUCCESS result.
 *    AND upper steps is REPEATABLE.
 *
 * [@US-2]
 *  AC-1: GIVEN USR know the MAX NUMBER of clients may be connected to a service at the same time BY IOC_getCapability,
 *    WHEN USR connect clients less than the MAX NUMBER to a service,
 *      THEN USR will get IOC_RESULT_SUCCESS result.
 *    WHEN USR connect clients more than the MAX NUMBER to a service,
 *      THEN USR will get IOC_RESULT_TOO_MANY_CLIENTS result.
 *    WHEN USR disconnect a client and connect a new client to the service,
 *      THEN USR will get IOC_RESULT_SUCCESS result.
 *    AND upper steps is REPEATABLE.
 *    AND above behaviors should be same for DIFFERENT services.
 *
 */

//-------------------------------------------------------------------------------------------------
/**
 * @brief 【Test Cases】
 *
 * [@AC-1,US-1]
 *  TC-1:
 *      @[Name]: verifyOnlineMoreThanCapabilityServices_shouldGetTooManyServices_andRepeatable
 *      @[Purpose]: verify US-1,AC-1
 *      @[Brief]: Repeat NxTimes of online from 0 to MAX_NUMBER+1 services, then offline one and retry online again.
 *
 * [@AC-1,US-2]
 *  TC-2:
 *      @[Name]: verifyConnectMoreThanCapabilityClients_shouldGetTooManyClients_andRepeatableOnDifferentServices
 *      @[Purpose]: verify US-2,AC-1
 *      @[Brief]: For each service in test, repeat NxTimes of connect from 0 to MAX_CLIENT_NUM+1 clients,
 *              then disconnect one and retry connect again. Test this behavior on multiple different services.
 *
 */

//======END OF UNIT TESTING DESIGN=================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======BEGIN OF UNIT TESTING IMPLEMENTATION=======================================================
#include "_UT_IOC_Common.h"

/**
 * @[Name]: <US-1,AC-1,TC-1>verifyOnlineMoreThanCapabilityServices_shouldGetTooManyServices_andRepeatable
 * @[Steps]:
 *   1) Get the MAX_SRV_NUM by IOC_getCapability(CAPID_CONET_MODE) as SETUP.
 *   2) Repeat NxTimes:
 *        a) Online from [0,MAX_SRV_NUM) services as BEHAVIOR.
 *            |-> SrvURI = {IOC_SRV_PROTO_FIFO, IOC_SRV_HOST_LOCAL_PROCESS, "SrvName(%d)"}
 *            |-> get IOC_RESULT_SUCCESS as VERIFY.
 *        b) Online the MAX_SRV_NUMth service as BEHAVIOR.
 *            |-> get IOC_RESULT_TOO_MANY_SERVICES as VERIFY.
 *        c) Offline first onlined service and retry online the MAX_SRV_NUMth service as BEHAVIOR.
 *            |-> get IOC_RESULT_SUCCESS as VERIFY.
 *        d) Offline all services as BEHAVIOR.
 * @[Expect]:
 *    1) get IOC_RESULT_SUCCESS as VERIFY.
 *    2) get IOC_RESULT_TOO_MANY_SERVICES as VERIFY.
 * @[Notes]:
 */
TEST(UT_ServiceCapability, verifyOnlineMoreThanCapabilityServices_shouldGetTooManyServices_andRepeatable) {
    //===SETUP===
    IOC_CapabilityDescription_T CapDesc = {.CapID = IOC_CAPID_CONET_MODE};
    IOC_Result_T Result = IOC_getCapability(&CapDesc);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // CheckPoint

    printf("MaxSrvNum: %d\n", CapDesc.ConetMode.MaxSrvNum);
    ASSERT_TRUE(CapDesc.ConetMode.MaxSrvNum > 0);

//===BEHAVIOR===
#define _NxTimes 3
    for (int RptCnt = 0; RptCnt < _NxTimes; RptCnt++) {
        // define and initialize an array to store the online service IDs
        IOC_SrvID_T OnlinedSrvIDs[CapDesc.ConetMode.MaxSrvNum + 1];
        for (int SrvIdx = 0; SrvIdx <= CapDesc.ConetMode.MaxSrvNum; SrvIdx++) {
            OnlinedSrvIDs[SrvIdx] = IOC_ID_INVALID;
        }

        // Online from [0,MAX_SRV_NUM] services
        for (int SrvIdx = 0; SrvIdx <= CapDesc.ConetMode.MaxSrvNum; SrvIdx++) {
            char SrvPath[32] = {0};
            snprintf(SrvPath, sizeof(SrvPath), "SrvName(%d)", SrvIdx);
            IOC_SrvURI_T SrvURI = {
                .pProtocol = IOC_SRV_PROTO_FIFO, .pHost = IOC_SRV_HOST_LOCAL_PROCESS, .pPath = SrvPath};
            IOC_SrvArgs_T SrvArgs = {.SrvURI = SrvURI, .UsageCapabilites = IOC_LinkUsageEvtProducer};

            Result = IOC_onlineService(&OnlinedSrvIDs[SrvIdx], &SrvArgs);
            if (SrvIdx < CapDesc.ConetMode.MaxSrvNum) {
                ASSERT_EQ(IOC_RESULT_SUCCESS, Result);
            } else {
                ASSERT_EQ(IOC_RESULT_TOO_MANY_SERVICES, Result);  // KeyVerifyPoint

                // Offline first onlined service and retry online the MAX_SRV_NUMth service
                Result = IOC_offlineService(OnlinedSrvIDs[0]);
                ASSERT_EQ(IOC_RESULT_SUCCESS, Result);

                Result = IOC_onlineService(&OnlinedSrvIDs[SrvIdx], &SrvArgs);
                ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // KeyVerifyPoint
            }
        }

        // Offline all services, except the first one
        for (int SrvIdx = 1; SrvIdx <= CapDesc.ConetMode.MaxSrvNum; SrvIdx++) {
            IOC_offlineService(OnlinedSrvIDs[SrvIdx]);
        }
    }
}

/**
 * @[Name]:
 * <US-2,AC-1,TC-2>verifyConnectMoreThanCapabilityClients_shouldGetTooManyClients_andRepeatableOnDifferentServices
 * @[Steps]:
 *   1) Get the MAX_CLIENT_NUM by IOC_getCapability(CAPID_CONET_MODE) as SETUP.
 *   2) Create multiple test services (e.g., 2-3 services) as SETUP.
 *       |-> SrvURI = {IOC_SRV_PROTO_FIFO, IOC_SRV_HOST_LOCAL_PROCESS, "SrvName(%d)"}
 *   3) For each service, Repeat MxTimes:
 *          a) Connect from [0,MAX_CLIENT_NUM) clients to the service as BEHAVIOR.
 *              |-> ClientArgs = {SrvURI(random of SrvName(%d)), IOC_LinkUsageEvtConsumer}
 *              |-> get IOC_RESULT_SUCCESS as VERIFY.
 *           b) Connect the MAX_CLIENT_NUMth client as BEHAVIOR.
 *              |-> get IOC_RESULT_TOO_MANY_CLIENTS as VERIFY.
 *           c) Disconnect first connected client and retry connect the MAX_CLIENT_NUMth client as BEHAVIOR.
 *              |-> get IOC_RESULT_SUCCESS as VERIFY.
 *           d) Disconnect all clients as CLEANUP.
 *      4) Offline all services as CLEANUP.
 * @[Expect]:
 *      1) get IOC_RESULT_SUCCESS for all services when within limit
 *      2) get IOC_RESULT_TOO_MANY_CLIENTS for all services when exceeding limit
 *      3) get IOC_RESULT_SUCCESS after disconnect-reconnect for all services
 * @[Notes]:
 *      - Test should verify that client limits work independently for each service
 *      - Each service should maintain its own client count limit
 */
TEST(UT_ServiceCapability,
     verifyConnectMoreThanCapabilityClients_shouldGetTooManyClients_andRepeatableOnDifferentServices) {
    //===SETUP===
    IOC_CapabilityDescription_T CapDesc = {.CapID = IOC_CAPID_CONET_MODE};
    IOC_Result_T Result = IOC_getCapability(&CapDesc);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // CheckPoint
    printf("MaxClientNum: %d\n", CapDesc.ConetMode.MaxCliNum);
    ASSERT_TRUE(CapDesc.ConetMode.MaxCliNum > 0);

//===BEHAVIOR===
// create multiple test services
#define _NxServices 2
    IOC_SrvURI_T SrvURIs[_NxServices];
    for (int SrvIdx = 0; SrvIdx < _NxServices; SrvIdx++) {
        char SrvPath[32] = {0};
        snprintf(SrvPath, sizeof(SrvPath), "SrvName(%d)", SrvIdx);
        SrvURIs[SrvIdx] = {.pProtocol = IOC_SRV_PROTO_FIFO, .pHost = IOC_SRV_HOST_LOCAL_PROCESS, .pPath = SrvPath};
    }

    // online all services
    IOC_SrvID_T OnlinedSrvIDs[_NxServices];
    for (int SrvIdx = 0; SrvIdx < _NxServices; SrvIdx++) {
        IOC_SrvArgs_T SrvArgs = {.SrvURI = SrvURIs[SrvIdx], .UsageCapabilites = IOC_LinkUsageEvtProducer};
        Result = IOC_onlineService(&OnlinedSrvIDs[SrvIdx], &SrvArgs);
        ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // CheckPoint
    }

#define _MxTimes 3
    for (int RptCnt = 0; RptCnt < _MxTimes; RptCnt++) {
        int RndSrvIdx = rand() % _NxServices;
        IOC_LinkID_T ConnectedLinkIDs[CapDesc.ConetMode.MaxSrvNum + 1];
        for (int LinkIdx = 0; LinkIdx <= CapDesc.ConetMode.MaxSrvNum; LinkIdx++) {
            ConnectedLinkIDs[LinkIdx] = IOC_ID_INVALID;
        }

        for (int LinkIdx = 0; LinkIdx <= CapDesc.ConetMode.MaxSrvNum; LinkIdx++) {
            IOC_ConnArgs_T ConnArgs = {.SrvURI = SrvURIs[RndSrvIdx], .Usage = IOC_LinkUsageEvtConsumer};
            Result = IOC_connectService(&ConnectedLinkIDs[LinkIdx], &ConnArgs, NULL);
            if (LinkIdx < CapDesc.ConetMode.MaxSrvNum) {
                ASSERT_EQ(IOC_RESULT_SUCCESS, Result);
            } else {
                ASSERT_EQ(IOC_RESULT_TOO_MANY_CLIENTS, Result);

                // disconnect first connected client and retry connect the MAX_CLIENT_NUMth client
                Result = IOC_closeLink(ConnectedLinkIDs[0]);
                ASSERT_EQ(IOC_RESULT_SUCCESS, Result);

                Result = IOC_connectService(&ConnectedLinkIDs[LinkIdx], &ConnArgs, NULL);
                ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // KeyVerifyPoint
            }
        }

        // disconnect all clients, except the first one
        for (int LinkIdx = 1; LinkIdx <= CapDesc.ConetMode.MaxSrvNum; LinkIdx++) {
            IOC_closeLink(ConnectedLinkIDs[LinkIdx]);
        }
    }

    //===CLEANUP===
    for (int SrvIdx = 0; SrvIdx < _NxServices; SrvIdx++) {
        IOC_offlineService(OnlinedSrvIDs[SrvIdx]);
    }
}

//======END OF UNIT TESTING IMPLEMENTATION=========================================================
///////////////////////////////////////////////////////////////////////////////////////////////////
